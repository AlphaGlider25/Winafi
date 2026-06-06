#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>

// Return codes
#define FILESYSTEM_OK                0
#define FILESYSTEM_ERR_LABEL_TOO_LONG -1
#define FILESYSTEM_ERR_INVALID_CHARS  -2
#define FILESYSTEM_ERR_INVALID_SIZE   -3
#define FILESYSTEM_ERR_MKFS_FAILED    -4
#define FILESYSTEM_ERR_TOOL_MISSING   -5

// Label validation for FAT32 (max 11 chars, no special chars)
int validate_label_for_fat32(const char *label);

// Generate mkfs command for testing
int gen_mkfs_fat32_command(
    const char *partition,
    const char *label,
    char *out_cmd,
    size_t cmd_size
);

// Execute filesystem creation
int create_fat32_filesystem(
    const char *partition,
    const char *label
);

// Label validation for NTFS (max 32 chars, no special chars)
int validate_label_for_ntfs(const char *label);

// Generate mkfs command for NTFS
int gen_mkfs_ntfs_command(
    const char *partition,
    const char *label,
    char *out_cmd,
    size_t cmd_size
);

// Execute NTFS filesystem creation
int create_ntfs_filesystem(
    const char *partition,
    const char *label
);

// Label validation for exFAT (max 15 chars, no special chars)
int validate_label_for_exfat(const char *label);

// Generate mkfs command for exFAT
int gen_mkfs_exfat_command(
    const char *partition,
    const char *label,
    char *out_cmd,
    size_t cmd_size
);

// Execute exFAT filesystem creation
int create_exfat_filesystem(
    const char *partition,
    const char *label
);

#endif
