#include "iso.h"
#include <archive.h>
#include <archive_entry.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int iso_validate_windows(const char *iso_path, iso_info_t *out_info) {
    if (!iso_path || !out_info) return -1;

    // Open ISO with libarchive
    struct archive *a = archive_read_new();
    if (!a) return -1;

    archive_read_support_format_iso9660(a);
    archive_read_support_filter_all(a);

    int ret = archive_read_open_filename(a, iso_path, 10240);
    if (ret != ARCHIVE_OK) {
        // TODO: Capture archive_error_string(a) for E-10-C error message
        fprintf(stderr, "ISO error: %s\n", archive_error_string(a));
        archive_read_free(a);
        return -1;  // E-10-C
    }

    // Check for install.wim
    int has_install_wim = 0;
    uint64_t file_count = 0;
    uint64_t total_size = 0;

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname(entry);

        if (strstr(name, "install.wim")) {
            has_install_wim = 1;
        }

        file_count++;
        total_size += (uint64_t)archive_entry_size(entry);

        archive_read_data_skip(a);
    }

    archive_read_free(a);

    // TODO (Phase 2 Refinement): Handle ESD format Windows ISOs that may not have install.wim in expected location
    // For now: Allow ISO if it was opened successfully and has content (file_count > 0)
    // In future: Improve ESD format detection
    if (!has_install_wim && file_count == 0) {
        return -1;  // E-10-D: ISO is empty or unreadable
    }

    // Detect Windows version (simplified: all are "Windows")
    strncpy(out_info->version, "Windows", 31);
    strncpy(out_info->detected_os_str, "Windows", sizeof(out_info->detected_os_str) - 1);
    out_info->detected_os_str[sizeof(out_info->detected_os_str) - 1] = '\0';
    out_info->file_count = file_count;
    out_info->total_size = total_size;
    out_info->total_size_bytes = total_size;
    out_info->os_type = 1;  // ISO_OS_WINDOWS (avoid including iso_extract.h)
    out_info->has_boot_files = has_install_wim ? 1 : 0;

    return 0;
}


#define ISO_PATH_MAX 4096

int iso_extract_to_directory(const char *iso_path, const char *extract_to) {
    if (!iso_path || !extract_to) return -1;

    // Validate extract_to is a writable directory
    if (access(extract_to, W_OK | X_OK) != 0) {
        fprintf(stderr, "Extract directory not writable: %s\n", extract_to);
        return -1;  // E-22-A: Mount Failed or access denied
    }

    // Try direct 7z extraction first (for modern Windows ISOs in UDF format)
    // This is faster and more reliable than wimapply for many systems
    if (access("/usr/bin/7z", X_OK) == 0) {
        fprintf(stdout, "[7z] Attempting direct extraction from ISO\n");

        pid_t pid = fork();
        if (pid == 0) {
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            // Extract everything from ISO to target directory
            execl("/usr/bin/7z", "7z", "x", "-y", iso_path, "-o", extract_to, (char*)NULL);
            exit(127);
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                fprintf(stdout, "[7z] Direct extraction successful\n");
                return 0;
            }
        }
    }

    // Standard ISO 9660 extraction path
    struct archive *a = archive_read_new();
    if (!a) return -1;

    archive_read_support_format_iso9660(a);
    archive_read_support_filter_all(a);

    int ret = archive_read_open_filename(a, iso_path, 10240);
    if (ret != ARCHIVE_OK) {
        // TODO: Capture archive_error_string(a) for E-10-G error message
        fprintf(stderr, "ISO error: %s\n", archive_error_string(a));
        archive_read_free(a);
        return -1;  // E-10-G: ISO Invalid
    }

    struct archive *ae = archive_write_disk_new();
    if (!ae) {
        archive_read_free(a);
        return -1;
    }

    archive_write_disk_set_options(ae, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);

    struct archive_entry *entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        // Check for symlinks (security policy: reject symlinks)
        if (archive_entry_filetype(entry) == AE_IFLNK) {
            fprintf(stderr, "Security: Symlink rejected in archive: %s\n", archive_entry_pathname(entry));
            archive_write_free(ae);
            archive_read_free(a);
            return -1;  // E-30-F: Symlink rejected
        }

        // Modify path to extract_to
        const char *original_path = archive_entry_pathname(entry);
        char *new_path = malloc(ISO_PATH_MAX);
        if (!new_path) {
            fprintf(stderr, "Memory allocation failed for path\n");
            archive_write_free(ae);
            archive_read_free(a);
            return -1;  // E-30-A: Write failed
        }
        snprintf(new_path, ISO_PATH_MAX, "%s/%s", extract_to, original_path);
        archive_entry_set_pathname(entry, new_path);

        int write_ret = archive_write_header(ae, entry);
        if (write_ret != ARCHIVE_OK) {
            fprintf(stderr, "Archive write header error: %s\n", archive_error_string(ae));
            free(new_path);
            archive_write_free(ae);
            archive_read_free(a);
            return -1;  // E-30-A
        }

        // For directories: only write header, don't read/write data
        if (archive_entry_filetype(entry) == AE_IFDIR) {
            free(new_path);
            continue;  // Skip to next entry
        }

        // For regular files: read and write data
        if (archive_entry_size(entry) > 0) {
            la_ssize_t bytes;
            char buf[65536];
            while ((bytes = archive_read_data(a, buf, sizeof(buf))) > 0) {
                ssize_t written = archive_write_data(ae, buf, (size_t)bytes);
                if (written != bytes) {
                    // Write failed or incomplete
                    fprintf(stderr, "Archive write data error: wrote %ld of %ld bytes\n", written, bytes);
                    free(new_path);
                    archive_write_free(ae);
                    archive_read_free(a);
                    return -1;  // E-30-A: Write failed
                }
            }
        }

        archive_write_finish_entry(ae);
        free(new_path);
    }

    archive_write_free(ae);
    archive_read_free(a);

    return 0;
}
#undef ISO_PATH_MAX

int iso_has_windows_install_wim(const char *iso_path) {
    if (!iso_path) return -1;

    struct archive *a = archive_read_new();
    if (!a) return -1;

    archive_read_support_format_iso9660(a);
    archive_read_support_filter_all(a);

    int ret = archive_read_open_filename(a, iso_path, 10240);
    if (ret != ARCHIVE_OK) {
        // TODO: Capture archive_error_string(a) for error message
        fprintf(stderr, "ISO error: %s\n", archive_error_string(a));
        archive_read_free(a);
        return -1;
    }

    struct archive_entry *entry;
    int found = 0;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (strstr(archive_entry_pathname(entry), "install.wim")) {
            found = 1;
            break;
        }
        archive_read_data_skip(a);
    }

    archive_read_free(a);
    return found ? 1 : 0;
}
