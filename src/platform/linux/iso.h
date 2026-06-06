#ifndef WINAFI_ISO_H
#define WINAFI_ISO_H

#include <stdint.h>

/**
 * ISO Detection Information Structure
 * Contains details about detected OS type, boot mode, and file structure
 *
 * Note: This is the same as iso_info_t in iso_extract.h - intentionally duplicated
 * to avoid circular dependencies between iso.h and iso_extract.h
 */
typedef struct {
    int os_type;                    // ISO_OS_WINDOWS=1, ISO_OS_LINUX=2, ISO_OS_UNKNOWN=0
    int has_boot_files;             // 1 if bootable, 0 if not
    int boot_mode;                  // ISO_BOOT_BIOS=0, ISO_BOOT_UEFI=1, ISO_BOOT_HYBRID=2
    uint64_t total_size_bytes;      // Total ISO size in bytes
    char detected_os_str[256];      // Human-readable OS name (e.g., "Windows 10 Pro")

    // Legacy fields for backwards compatibility
    char version[32];               // Same as detected_os_str
    uint64_t file_count;            // Number of files (deprecated - not populated)
    uint64_t total_size;            // Same as total_size_bytes (for compatibility)
} iso_info_t;

// Validate Windows ISO and extract metadata
// iso_path: path to ISO file
// out_info: populated with ISO metadata
int iso_validate_windows(const char *iso_path, iso_info_t *out_info);

// Extract ISO contents to directory
// iso_path: path to ISO file
// extract_to: mounted NTFS partition mount point
// Returns: 0 on success, negative on error
int iso_extract_to_directory(const char *iso_path, const char *extract_to);

// Check if install.wim exists in ISO (indicates Windows ISO)
int iso_has_windows_install_wim(const char *iso_path);

#endif
