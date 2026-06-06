#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "filesystem.h"

// FAT32 invalid characters + shell metacharacters for safety
// Invalid per filesystem spec AND invalid for shell safety
// (Must be safe for use in system() calls with shell interpretation)
static int is_invalid_char(unsigned char ch) {
    // Control characters (< 32)
    if (ch < 32) {
        return 1;
    }

    // Explicitly forbidden characters
    switch (ch) {
        case '\'':  // Single quote - breaks shell quoting
            return 1;
        case '"':   // Double quote
            return 1;
        case '*':   // Asterisk
        case '+':   // Plus
        case ',':   // Comma
        case '/':   // Forward slash
        case ':':   // Colon
        case ';':   // Semicolon (command separator)
        case '<':   // Less than
        case '=':   // Equals
        case '>':   // Greater than
        case '?':   // Question mark
        case '[':   // Left bracket
        case '\\':  // Backslash
        case ']':   // Right bracket
        case '|':   // Pipe
        case '`':   // Backtick (command substitution)
        case '$':   // Dollar (variable/command substitution)
        case '&':   // Ampersand (background process)
        case '(':   // Left paren (subshell)
        case ')':   // Right paren (subshell)
        case '{':   // Left brace (brace expansion)
        case '}':   // Right brace (brace expansion)
        case '~':   // Tilde (home directory expansion)
            return 1;
        default:
            return 0;
    }
}

// Validate FAT32 label
// Returns FILESYSTEM_OK if valid, error code otherwise
// NULL or empty labels are accepted
int validate_label_for_fat32(const char *label) {
    // NULL or empty labels are OK
    if (label == NULL || label[0] == '\0') {
        return FILESYSTEM_OK;
    }

    // Check length (max 11 characters for FAT32)
    size_t len = strlen(label);
    if (len > 11) {
        return FILESYSTEM_ERR_LABEL_TOO_LONG;
    }

    // Check for invalid characters
    for (size_t i = 0; i < len; i++) {
        if (is_invalid_char((unsigned char)label[i])) {
            return FILESYSTEM_ERR_INVALID_CHARS;
        }
    }

    return FILESYSTEM_OK;
}

// Generate mkfs.vfat command
// Returns FILESYSTEM_OK on success, error code otherwise
int gen_mkfs_fat32_command(
    const char *partition,
    const char *label,
    char *out_cmd,
    size_t cmd_size
) {
    // Validate inputs
    if (partition == NULL || out_cmd == NULL || cmd_size == 0) {
        return FILESYSTEM_ERR_INVALID_SIZE;
    }

    // Validate label if provided
    if (label != NULL && label[0] != '\0') {
        int ret = validate_label_for_fat32(label);
        if (ret != FILESYSTEM_OK) {
            return ret;
        }
    }

    // Build command: mkfs.vfat -F 32 [-n LABEL] /dev/partition
    // Label is quoted for shell safety
    int chars_written;
    if (label != NULL && label[0] != '\0') {
        chars_written = snprintf(
            out_cmd,
            cmd_size,
            "mkfs.vfat -F 32 -n '%s' %s",
            label,
            partition
        );
    } else {
        chars_written = snprintf(
            out_cmd,
            cmd_size,
            "mkfs.vfat -F 32 %s",
            partition
        );
    }

    // Check for truncation
    if (chars_written < 0 || (size_t)chars_written >= cmd_size) {
        return FILESYSTEM_ERR_INVALID_SIZE;
    }

    return FILESYSTEM_OK;
}

// Execute filesystem creation
// Returns FILESYSTEM_OK on success, error code otherwise
int create_fat32_filesystem(
    const char *partition,
    const char *label
) {
    // Check if mkfs.vfat is available
    if (access("/sbin/mkfs.vfat", X_OK) != 0 &&
        access("/usr/sbin/mkfs.vfat", X_OK) != 0) {
        return FILESYSTEM_ERR_TOOL_MISSING;
    }

    // Validate label
    if (label != NULL && label[0] != '\0') {
        int ret = validate_label_for_fat32(label);
        if (ret != FILESYSTEM_OK) {
            return ret;
        }
    }

    // Generate command
    char cmd[512];
    int ret = gen_mkfs_fat32_command(partition, label, cmd, sizeof(cmd));
    if (ret != FILESYSTEM_OK) {
        return ret;
    }

    // Execute mkfs command
    int exit_status = system(cmd);
    if (exit_status == -1) {
        return FILESYSTEM_ERR_MKFS_FAILED;
    }

    // Check exit status
    if (WIFEXITED(exit_status)) {
        if (WEXITSTATUS(exit_status) == 0) {
            return FILESYSTEM_OK;
        }
    }

    return FILESYSTEM_ERR_MKFS_FAILED;
}

// Validate NTFS label
// Returns FILESYSTEM_OK if valid, error code otherwise
// NULL or empty labels are accepted
// NTFS labels: max 32 characters, same invalid characters as FAT32
int validate_label_for_ntfs(const char *label) {
    // NULL or empty labels are OK
    if (label == NULL || label[0] == '\0') {
        return FILESYSTEM_OK;
    }

    // Check length (max 32 characters for NTFS)
    size_t len = strlen(label);
    if (len > 32) {
        return FILESYSTEM_ERR_LABEL_TOO_LONG;
    }

    // Check for invalid characters (same as FAT32)
    for (size_t i = 0; i < len; i++) {
        if (is_invalid_char((unsigned char)label[i])) {
            return FILESYSTEM_ERR_INVALID_CHARS;
        }
    }

    return FILESYSTEM_OK;
}

// Generate mkfs.ntfs command
// Returns FILESYSTEM_OK on success, error code otherwise
int gen_mkfs_ntfs_command(
    const char *partition,
    const char *label,
    char *out_cmd,
    size_t cmd_size
) {
    // Validate inputs
    if (partition == NULL || out_cmd == NULL || cmd_size == 0) {
        return FILESYSTEM_ERR_INVALID_SIZE;
    }

    // Validate label if provided
    if (label != NULL && label[0] != '\0') {
        int ret = validate_label_for_ntfs(label);
        if (ret != FILESYSTEM_OK) {
            return ret;
        }
    }

    // Build command: mkfs.ntfs [-L LABEL] /dev/partition
    // Label is quoted for shell safety
    int chars_written;
    if (label != NULL && label[0] != '\0') {
        chars_written = snprintf(
            out_cmd,
            cmd_size,
            "mkfs.ntfs -L '%s' %s",
            label,
            partition
        );
    } else {
        chars_written = snprintf(
            out_cmd,
            cmd_size,
            "mkfs.ntfs %s",
            partition
        );
    }

    // Check for truncation
    if (chars_written < 0 || (size_t)chars_written >= cmd_size) {
        return FILESYSTEM_ERR_INVALID_SIZE;
    }

    return FILESYSTEM_OK;
}

// Execute NTFS filesystem creation
// Returns FILESYSTEM_OK on success, error code otherwise
int create_ntfs_filesystem(
    const char *partition,
    const char *label
) {
    // Check if mkfs.ntfs is available
    if (access("/sbin/mkfs.ntfs", X_OK) != 0 &&
        access("/usr/sbin/mkfs.ntfs", X_OK) != 0) {
        return FILESYSTEM_ERR_TOOL_MISSING;
    }

    // Validate label
    if (label != NULL && label[0] != '\0') {
        int ret = validate_label_for_ntfs(label);
        if (ret != FILESYSTEM_OK) {
            return ret;
        }
    }

    // Generate command
    char cmd[512];
    int ret = gen_mkfs_ntfs_command(partition, label, cmd, sizeof(cmd));
    if (ret != FILESYSTEM_OK) {
        return ret;
    }

    // Execute mkfs command
    int exit_status = system(cmd);
    if (exit_status == -1) {
        return FILESYSTEM_ERR_MKFS_FAILED;
    }

    // Check exit status
    if (WIFEXITED(exit_status)) {
        if (WEXITSTATUS(exit_status) == 0) {
            return FILESYSTEM_OK;
        }
    }

    return FILESYSTEM_ERR_MKFS_FAILED;
}

// Validate exFAT label
// Returns FILESYSTEM_OK if valid, error code otherwise
// NULL or empty labels are accepted
// exFAT labels: max 15 characters, same invalid characters as FAT32 and NTFS
int validate_label_for_exfat(const char *label) {
    // NULL or empty labels are OK
    if (label == NULL || label[0] == '\0') {
        return FILESYSTEM_OK;
    }

    // Check length (max 15 characters for exFAT)
    size_t len = strlen(label);
    if (len > 15) {
        return FILESYSTEM_ERR_LABEL_TOO_LONG;
    }

    // Check for invalid characters (same as FAT32 and NTFS)
    for (size_t i = 0; i < len; i++) {
        if (is_invalid_char((unsigned char)label[i])) {
            return FILESYSTEM_ERR_INVALID_CHARS;
        }
    }

    return FILESYSTEM_OK;
}

// Generate mkfs.exfat command
// Returns FILESYSTEM_OK on success, error code otherwise
int gen_mkfs_exfat_command(
    const char *partition,
    const char *label,
    char *out_cmd,
    size_t cmd_size
) {
    // Validate inputs
    if (partition == NULL || out_cmd == NULL || cmd_size == 0) {
        return FILESYSTEM_ERR_INVALID_SIZE;
    }

    // Validate label if provided
    if (label != NULL && label[0] != '\0') {
        int ret = validate_label_for_exfat(label);
        if (ret != FILESYSTEM_OK) {
            return ret;
        }
    }

    // Build command: mkfs.exfat [-L LABEL] /dev/partition
    // Label is quoted for shell safety
    int chars_written;
    if (label != NULL && label[0] != '\0') {
        chars_written = snprintf(
            out_cmd,
            cmd_size,
            "mkfs.exfat -L '%s' %s",
            label,
            partition
        );
    } else {
        chars_written = snprintf(
            out_cmd,
            cmd_size,
            "mkfs.exfat %s",
            partition
        );
    }

    // Check for truncation
    if (chars_written < 0 || (size_t)chars_written >= cmd_size) {
        return FILESYSTEM_ERR_INVALID_SIZE;
    }

    return FILESYSTEM_OK;
}

// Execute exFAT filesystem creation
// Returns FILESYSTEM_OK on success, error code otherwise
int create_exfat_filesystem(
    const char *partition,
    const char *label
) {
    // Check if mkfs.exfat is available
    if (access("/sbin/mkfs.exfat", X_OK) != 0 &&
        access("/usr/sbin/mkfs.exfat", X_OK) != 0) {
        return FILESYSTEM_ERR_TOOL_MISSING;
    }

    // Validate label
    if (label != NULL && label[0] != '\0') {
        int ret = validate_label_for_exfat(label);
        if (ret != FILESYSTEM_OK) {
            return ret;
        }
    }

    // Generate command
    char cmd[512];
    int ret = gen_mkfs_exfat_command(partition, label, cmd, sizeof(cmd));
    if (ret != FILESYSTEM_OK) {
        return ret;
    }

    // Execute mkfs command
    int exit_status = system(cmd);
    if (exit_status == -1) {
        return FILESYSTEM_ERR_MKFS_FAILED;
    }

    // Check exit status
    if (WIFEXITED(exit_status)) {
        if (WEXITSTATUS(exit_status) == 0) {
            return FILESYSTEM_OK;
        }
    }

    return FILESYSTEM_ERR_MKFS_FAILED;
}
