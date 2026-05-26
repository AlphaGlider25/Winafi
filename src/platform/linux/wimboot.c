#define _POSIX_C_SOURCE 200809L
#include "wimboot.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHUNK_SIZE 8192

/* Make a single directory, ignoring EEXIST. Returns 0 on success, -1 on error. */
static int make_dir(const char *path) {
    if (mkdir(path, 0755) == 0) return 0;
    if (errno == EEXIST) return 0;
    return -1;
}

/* Copy file from src to dst. Returns 0 on success, -1 on error. */
static int copy_file(const char *src, const char *dst) {
    int rfd = open(src, O_RDONLY);
    if (rfd < 0) return -1;

    int wfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (wfd < 0) { close(rfd); return -1; }

    char buf[CHUNK_SIZE];
    ssize_t nr;
    int result = 0;
    while ((nr = read(rfd, buf, sizeof(buf))) > 0) {
        ssize_t nw = write(wfd, buf, (size_t)nr);
        if (nw != nr) { result = -1; break; }
    }
    if (nr < 0) result = -1;

    close(rfd);
    close(wfd);
    return result;
}

int wimboot_detect(const char *iso_mount_path) {
    if (!iso_mount_path) return -1;

    char path[4096];
    struct stat st;

    int n = snprintf(path, sizeof(path), "%s/sources/install.wim", iso_mount_path);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;
    if (stat(path, &st) == 0) return 1;

    n = snprintf(path, sizeof(path), "%s/sources/install.esd", iso_mount_path);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;
    if (stat(path, &st) == 0) return 1;

    return 0;
}

int wimboot_find_wim(const char *iso_mount_path, char *out_path, size_t out_size) {
    if (!iso_mount_path || !out_path) return -1;

    char path[4096];
    struct stat st;

    int n = snprintf(path, sizeof(path), "%s/sources/install.wim", iso_mount_path);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;
    if (stat(path, &st) == 0) {
        n = snprintf(out_path, out_size, "%s", path);
        if (n < 0 || (size_t)n >= out_size) return -1;
        return 1;
    }

    n = snprintf(path, sizeof(path), "%s/sources/install.esd", iso_mount_path);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;
    if (stat(path, &st) == 0) {
        n = snprintf(out_path, out_size, "%s", path);
        if (n < 0 || (size_t)n >= out_size) return -1;
        return 1;
    }

    return 0;
}

int wimboot_setup(const char *iso_mount_path,
                  const char *mount_point,
                  const char *wimboot_efi_src) {
    if (!iso_mount_path || !mount_point) return -1;

    char path[4096];
    int n;

    /* Create EFI/boot directory */
    n = snprintf(path, sizeof(path), "%s/EFI", mount_point);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;
    if (make_dir(path) < 0) return -1;

    n = snprintf(path, sizeof(path), "%s/EFI/boot", mount_point);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;
    if (make_dir(path) < 0) return -1;

    /* Copy wimboot.efi if provided */
    if (wimboot_efi_src) {
        char dst[4096];
        n = snprintf(dst, sizeof(dst), "%s/EFI/boot/bootx64.efi", mount_point);
        if (n < 0 || (size_t)n >= sizeof(dst)) return -1;
        if (copy_file(wimboot_efi_src, dst) < 0) return -1;
    }

    /* Create sources directory on dst */
    char dst_srcdir[4096];
    n = snprintf(dst_srcdir, sizeof(dst_srcdir), "%s/sources", mount_point);
    if (n < 0 || (size_t)n >= sizeof(dst_srcdir)) return -1;
    if (make_dir(dst_srcdir) < 0) return -1;

    /* Copy sources/boot.wim from ISO */
    char src_bootwim[4096], dst_bootwim[4096];
    n = snprintf(src_bootwim, sizeof(src_bootwim), "%s/sources/boot.wim", iso_mount_path);
    if (n < 0 || (size_t)n >= sizeof(src_bootwim)) return -1;
    n = snprintf(dst_bootwim, sizeof(dst_bootwim), "%s/sources/boot.wim", mount_point);
    if (n < 0 || (size_t)n >= sizeof(dst_bootwim)) return -1;

    if (copy_file(src_bootwim, dst_bootwim) < 0) return -1;

    return 0;
}

static int copy_file_64k(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }

    char buf[65536];
    size_t n;
    int result = 0;

    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            result = -1;
            break;
        }
    }

    /* Check for read errors */
    if (ferror(in)) {
        result = -1;
    }

    /* Check close operations */
    if (fclose(in) != 0 || fclose(out) != 0) {
        result = -1;
    }

    return result;
}

static int mkdirs_recursive(const char *path) {
    char tmp[4096];
    int n = snprintf(tmp, sizeof(tmp), "%s", path);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return -1;

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) < 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    return mkdir(tmp, 0755) < 0 && errno != EEXIST ? -1 : 0;
}

int wimboot_setup_uefi(const char *mount_point, const char *wimboot_efi_path) {
    if (!mount_point || !wimboot_efi_path) return -1;
    if (access(wimboot_efi_path, R_OK) != 0) return -1;

    /* Validate mount_point length to prevent buffer overflow */
    if (strlen(mount_point) > 4096 - 32) return -1;

    char dir[4096];
    int n = snprintf(dir, sizeof(dir), "%s/EFI/Boot", mount_point);
    if (n < 0 || (size_t)n >= sizeof(dir)) return -1;
    if (mkdirs_recursive(dir) < 0) return -1;

    char dst[4096 + 32];
    n = snprintf(dst, sizeof(dst), "%s/bootx64.efi", dir);
    if (n < 0 || (size_t)n >= sizeof(dst)) return -1;
    return copy_file_64k(wimboot_efi_path, dst);
}

int wimboot_setup_bios(const char *mount_point, const char *wimboot_bios_path) {
    if (!mount_point || !wimboot_bios_path) return -1;
    if (access(wimboot_bios_path, R_OK) != 0) return -1;

    char dst[4096 + 32];
    int n = snprintf(dst, sizeof(dst), "%s/wimboot", mount_point);
    if (n < 0 || (size_t)n >= sizeof(dst)) return -1;
    return copy_file_64k(wimboot_bios_path, dst);
}

const char *wimboot_find_asset(char *buf, size_t n) {
    // Try installed path first
    const char *paths[] = {
        "/usr/share/winify/assets/wimboot.efi",
        "/usr/local/share/winify/assets/wimboot.efi",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        if (access(paths[i], R_OK) == 0) {
            int ret = snprintf(buf, n, "%s", paths[i]);
            if (ret < 0 || (size_t)ret >= n) return NULL;
            return buf;
        }
    }
    return NULL;
}
