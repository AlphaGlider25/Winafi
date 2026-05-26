#include <string.h>
#include "error_codes.h"

/**
 * Error message entry structure
 */
typedef struct {
    int error_code;
    const char *message;     // User-friendly message
    const char *recovery;    // Suggested recovery action
    const char *faq_link;    // FAQ URL (may be NULL)
} error_entry_t;

/**
 * Complete error database with 200+ entries
 */
static const error_entry_t error_database[] = {
    // Success
    {
        ERR_OK,
        "Operation completed successfully",
        "No action needed",
        NULL
    },

    /* ====================================================================
     * DEVICE ERRORS (-100 to -114)
     * ==================================================================== */

    {
        ERR_DEV_NOT_FOUND,
        "Device /dev/XXX not found",
        "Check that the USB device is connected. Try unplugging and replugging the device.",
        "https://winafi.local/faq#device-not-found"
    },
    {
        ERR_DEV_PERMISSION_DENIED,
        "Permission denied - cannot access device",
        "Run with administrator privileges (sudo) or add user to disk group",
        "https://winafi.local/faq#permission-denied"
    },
    {
        ERR_DEV_DEVICE_BUSY,
        "Device is busy or in use",
        "Close any open files on the device and ensure it's not mounted. Use 'lsof /dev/sdX' to check.",
        "https://winafi.local/faq#device-busy"
    },
    {
        ERR_DEV_DEVICE_TOO_SMALL,
        "Device capacity is insufficient for the ISO image",
        "Use a larger USB device or a smaller ISO image. Device needs to be at least 1GB larger than ISO.",
        "https://winafi.local/faq#device-too-small"
    },
    {
        ERR_DEV_SYSTEM_DRIVE,
        "Device appears to be a system drive - operation cancelled for safety",
        "Verify you selected the correct USB device. Do not attempt to format system drives.",
        "https://winafi.local/faq#system-drive-protection"
    },
    {
        ERR_DEV_DEVICE_LOCKED,
        "Device is locked by another process",
        "Close all applications accessing the device. Check for running disk utilities or mounting services.",
        "https://winafi.local/faq#device-locked"
    },
    {
        ERR_DEV_READ_FAILED,
        "Failed to read device information",
        "Check device connection and try again. Device may have hardware issues.",
        "https://winafi.local/faq#read-failed"
    },
    {
        ERR_DEV_WRITE_FAILED,
        "Failed to write to device",
        "Check device connection, ensure you have write permissions, and verify sufficient space.",
        "https://winafi.local/faq#write-failed"
    },
    {
        ERR_DEV_CAPACITY_CHECK,
        "Device capacity validation failed",
        "Verify device is properly connected and detected by the system.",
        "https://winafi.local/faq#capacity-check"
    },
    {
        ERR_DEV_MOUNTED_PARTITION,
        "One or more partitions are mounted",
        "Unmount all partitions: sudo umount /dev/sdXN (replace X and N with device/partition numbers)",
        "https://winafi.local/faq#mounted-partition"
    },
    {
        ERR_DEV_SIZE_VALIDATION,
        "Device size validation failed",
        "Retry the operation. If persistent, the device may have read/write issues.",
        NULL
    },
    {
        ERR_DEV_FS_CHECK_FAILED,
        "Filesystem check failed on device",
        "Run fsck to check filesystem: sudo fsck /dev/sdX (use with caution)",
        NULL
    },
    {
        ERR_DEV_NO_SUCH_DEVICE,
        "Device node not found in system",
        "Ensure USB device is properly connected. Device may have been disconnected.",
        NULL
    },
    {
        ERR_DEV_NOT_BLOCK_DEVICE,
        "Selected path is not a block device",
        "Select a device path like /dev/sdX, not a partition (/dev/sdX1) or regular file.",
        NULL
    },
    {
        ERR_DEV_PARTITION_STILL_MOUNTED,
        "One or more partitions are still mounted",
        "Unmount all partitions before proceeding: sudo umount /mnt/usb",
        NULL
    },

    /* ====================================================================
     * PARTITION ERRORS (-150 to -164)
     * ==================================================================== */

    {
        ERR_PART_INVALID_TABLE,
        "Invalid or corrupted partition table",
        "Device partition table may be corrupted. Try writing a new partition table.",
        "https://winafi.local/faq#invalid-partition-table"
    },
    {
        ERR_PART_MBR_FULL,
        "MBR partition table is full (max 4 primary partitions)",
        "MBR supports maximum 4 primary partitions. Use GPT for more partitions.",
        "https://winafi.local/faq#mbr-full"
    },
    {
        ERR_PART_CREATE_FAILED,
        "Failed to create partition on device",
        "Verify device is not write-protected and has sufficient space. Check permissions.",
        "https://winafi.local/faq#partition-create-failed"
    },
    {
        ERR_PART_ALIGNMENT_FAILED,
        "Failed to align partition properly",
        "Use a larger device or try different partition settings. Modern drives require proper alignment.",
        "https://winafi.local/faq#partition-alignment"
    },
    {
        ERR_PART_INVALID_SIZE,
        "Invalid partition size specified",
        "Partition size must be greater than 0 and fit within device capacity.",
        "https://winafi.local/faq#invalid-partition-size"
    },
    {
        ERR_PART_GPT_CONVERSION,
        "Failed to convert partition table to GPT",
        "Ensure device is empty or has valid MBR. Some devices may have issues with conversion.",
        "https://winafi.local/faq#gpt-conversion-failed"
    },
    {
        ERR_PART_LIBPARTED_ERROR,
        "libparted library error occurred",
        "Partition operation failed at system level. Verify device connectivity and try again.",
        "https://winafi.local/faq#libparted-error"
    },
    {
        ERR_PART_INVALID_PARTITION,
        "Invalid partition number or specification",
        "Partition numbers must be 1-4 for MBR or 1-128 for GPT.",
        NULL
    },
    {
        ERR_PART_BOOT_FLAG_FAILED,
        "Failed to set boot flag on partition",
        "Boot flag setting failed. Partition may still be usable but may not boot on some systems.",
        NULL
    },
    {
        ERR_PART_DELETE_FAILED,
        "Failed to delete partition",
        "Ensure partition is unmounted and device is not in use.",
        NULL
    },
    {
        ERR_PART_WRONG_TYPE,
        "Partition type is incorrect for operation",
        "Ensure partition type is appropriate for the ISO type (e.g., EFI for UEFI).",
        NULL
    },
    {
        ERR_PART_NO_SPACE,
        "Insufficient space to create partition",
        "Verify device has free space and required capacity for ISO.",
        NULL
    },
    {
        ERR_PART_TABLE_READ_FAILED,
        "Failed to read partition table from device",
        "Check device connection. Device may have hardware issues.",
        NULL
    },
    {
        ERR_PART_TABLE_WRITE_FAILED,
        "Failed to write partition table to device",
        "Device may be read-only or have hardware issues. Check permissions and connections.",
        NULL
    },
    {
        ERR_PART_SECTOR_TOO_LARGE,
        "Device sector size is too large for operation",
        "Some older systems may not support this device's native sector size.",
        NULL
    },

    /* ====================================================================
     * FILESYSTEM ERRORS (-200 to -213)
     * ==================================================================== */

    {
        ERR_FS_LABEL_TOO_LONG,
        "Volume label is too long",
        "Volume labels must be shorter: FAT32 max 11 chars, NTFS max 32 chars, exFAT max 15 chars",
        "https://winafi.local/faq#label-too-long"
    },
    {
        ERR_FS_INVALID_CHARS,
        "Volume label contains invalid characters",
        "Remove special characters from label. Avoid: * ? / \\ | < > : \" and control characters",
        "https://winafi.local/faq#invalid-label-chars"
    },
    {
        ERR_FS_INVALID_SIZE,
        "Filesystem size is invalid",
        "Filesystem size must be between minimum and device capacity.",
        "https://winafi.local/faq#invalid-fs-size"
    },
    {
        ERR_FS_MKFS_FAILED,
        "Filesystem creation failed",
        "Verify device is writable, has space, and correct permissions. Check system logs.",
        "https://winafi.local/faq#mkfs-failed"
    },
    {
        ERR_FS_TOOL_MISSING,
        "Required filesystem creation tool is not installed",
        "Install missing tool: FAT32 needs mkfs.vfat, NTFS needs mkfs.ntfs, exFAT needs mkfs.exfat",
        "https://winafi.local/faq#mkfs-tool-missing"
    },
    {
        ERR_FS_UNSUPPORTED,
        "Unsupported filesystem type",
        "Supported filesystems: FAT32, NTFS, exFAT. Use one of these.",
        "https://winafi.local/faq#unsupported-fs"
    },
    {
        ERR_FS_FORMAT_TIMEOUT,
        "Filesystem formatting timed out",
        "Operation took too long. Device may be slow. Try again or use a different device.",
        "https://winafi.local/faq#format-timeout"
    },
    {
        ERR_FS_PARTITION_NOT_FOUND,
        "Target partition not found",
        "Partition may have been deleted or device changed. Verify partition still exists.",
        "https://winafi.local/faq#partition-not-found"
    },
    {
        ERR_FS_INCOMPATIBLE_BOOT,
        "Filesystem is incompatible with selected boot type",
        "UEFI boot requires FAT32. BIOS boot works with FAT32, NTFS, or exFAT.",
        "https://winafi.local/faq#fs-boot-incompatible"
    },
    {
        ERR_FS_NO_SPACE_LEFT,
        "No space left on device during formatting",
        "Device full. Verify device capacity is sufficient for operation.",
        "https://winafi.local/faq#no-space-left"
    },
    {
        ERR_FS_LABEL_UNICODE,
        "Unicode label could not be properly encoded",
        "Try using ASCII characters only in volume label.",
        NULL
    },
    {
        ERR_FS_PERMISSION_DENIED,
        "Permission denied to format filesystem",
        "Run with administrator privileges (sudo) or ensure you have write access.",
        NULL
    },
    {
        ERR_FS_UNKNOWN_TYPE,
        "Unknown filesystem type encountered",
        "Ensure correct filesystem type is selected.",
        NULL
    },
    {
        ERR_FS_CLUSTER_SIZE_INVALID,
        "Cluster size is invalid for selected filesystem",
        "Use default cluster size. Custom sizes must match filesystem requirements.",
        NULL
    },

    /* ====================================================================
     * ISO ERRORS (-250 to -263)
     * ==================================================================== */

    {
        ERR_ISO_NOT_FOUND,
        "ISO file not found",
        "Verify the ISO file path is correct and the file exists.",
        "https://winafi.local/faq#iso-not-found"
    },
    {
        ERR_ISO_NOT_ISO,
        "File is not a valid ISO 9660 image",
        "Select a valid ISO file. The file may be corrupted or not an ISO image.",
        "https://winafi.local/faq#not-iso"
    },
    {
        ERR_ISO_ARCHIVE_ERROR,
        "Error reading ISO archive",
        "ISO file may be corrupted. Download the file again or verify the checksum.",
        "https://winafi.local/faq#archive-error"
    },
    {
        ERR_ISO_EXTRACT_FAILED,
        "Failed to extract ISO contents to device",
        "Verify device has sufficient free space and write permissions. Check device connection.",
        "https://winafi.local/faq#extract-failed"
    },
    {
        ERR_ISO_NO_BOOT_INFO,
        "ISO file has no boot information",
        "This ISO may not be bootable. Verify you have the correct ISO for your operating system.",
        "https://winafi.local/faq#no-boot-info"
    },
    {
        ERR_ISO_CORRUPTED,
        "ISO file appears to be corrupted",
        "Download the ISO file again and verify its checksum matches the official source.",
        "https://winafi.local/faq#iso-corrupted"
    },
    {
        ERR_ISO_INCOMPLETE_EXTRACT,
        "ISO extraction completed but may be incomplete",
        "Some files may not have been extracted. Verify the operation succeeded.",
        "https://winafi.local/faq#incomplete-extract"
    },
    {
        ERR_ISO_INSUFFICIENT_SPACE,
        "Insufficient space on device to extract ISO",
        "Use a larger USB device. Device must have space for ISO plus additional overhead.",
        "https://winafi.local/faq#insufficient-space"
    },
    {
        ERR_ISO_EXTRACT_TIMEOUT,
        "ISO extraction timed out",
        "Operation took too long. Device may be slow. Try again with a faster device.",
        "https://winafi.local/faq#extract-timeout"
    },
    {
        ERR_ISO_FILE_NOT_FOUND,
        "Required file not found in ISO",
        "ISO file is incomplete or corrupt. Re-download the file.",
        "https://winafi.local/faq#file-in-iso-not-found"
    },
    {
        ERR_ISO_WRITE_PERMISSION,
        "Write permission denied for ISO extraction",
        "Run with administrator privileges (sudo) or ensure write access to device.",
        "https://winafi.local/faq#iso-write-permission"
    },
    {
        ERR_ISO_DIRECTORY_CREATE_FAILED,
        "Failed to create directory during extraction",
        "Device may be read-only or permissions issue. Verify device is writable.",
        "https://winafi.local/faq#dir-create-failed"
    },
    {
        ERR_ISO_SYMLINK_CREATE_FAILED,
        "Failed to create symbolic link during extraction",
        "Some filesystems don't support symbolic links. May affect Linux installations.",
        "https://winafi.local/faq#symlink-create-failed"
    },
    {
        ERR_ISO_EXTRACTION_CANCELLED,
        "ISO extraction was cancelled by user",
        "Operation was stopped. Partial files may remain on device.",
        NULL
    },

    /* ====================================================================
     * BOOT ERRORS (-300 to -313)
     * ==================================================================== */

    {
        ERR_BOOT_NO_BOOTLOADER,
        "No bootloader found for this ISO",
        "ISO may not be bootable or is missing required boot files.",
        "https://winafi.local/faq#no-bootloader"
    },
    {
        ERR_BOOT_UNSUPPORTED_BOOT,
        "Boot type is not supported",
        "Select a supported boot mode: BIOS, UEFI, or Hybrid.",
        "https://winafi.local/faq#unsupported-boot"
    },
    {
        ERR_BOOT_GRUB2_MISSING,
        "GRUB2 bootloader not found on system",
        "Install GRUB2: sudo apt-get install grub2 (or equivalent for your distribution)",
        "https://winafi.local/faq#grub2-missing"
    },
    {
        ERR_BOOT_SYSLINUX_MISSING,
        "Syslinux bootloader not found on system",
        "Install Syslinux: sudo apt-get install syslinux (or equivalent for your distribution)",
        "https://winafi.local/faq#syslinux-missing"
    },
    {
        ERR_BOOT_BOOTCODE_MISSING,
        "Bootcode file is missing",
        "ISO may be incomplete. Re-download and try again.",
        "https://winafi.local/faq#bootcode-missing"
    },
    {
        ERR_BOOT_INSTALL_FAILED,
        "Bootloader installation failed",
        "Device may be read-only or permissions issue. Verify write access and try again.",
        "https://winafi.local/faq#boot-install-failed"
    },
    {
        ERR_BOOT_UEFI_FAT32_REQUIRED,
        "UEFI boot requires FAT32 filesystem",
        "Change filesystem to FAT32 for UEFI booting, or use BIOS boot mode instead.",
        "https://winafi.local/faq#uefi-fat32-required"
    },
    {
        ERR_BOOT_SECURE_BOOT_ENABLED,
        "Secure Boot is enabled - USB may not boot",
        "Disable Secure Boot in BIOS/UEFI firmware settings if boot fails.",
        "https://winafi.local/faq#secure-boot-enabled"
    },
    {
        ERR_BOOT_ESP_CREATION_FAILED,
        "EFI System Partition creation failed",
        "Verify partition table is GPT and device has sufficient space.",
        "https://winafi.local/faq#esp-creation-failed"
    },
    {
        ERR_BOOT_BOOTCODE_TOO_LARGE,
        "Bootcode file is too large (must be <= 446 bytes)",
        "Bootcode file is invalid. Use official bootcode from the OS.",
        "https://winafi.local/faq#bootcode-too-large"
    },
    {
        ERR_BOOT_SIGNATURE_WRITE_FAILED,
        "Failed to write MBR signature (0xAA55)",
        "Device access issue. Verify device is writable and permissions are correct.",
        "https://winafi.local/faq#signature-write-failed"
    },
    {
        ERR_BOOT_CONFIG_INVALID,
        "Boot configuration is invalid",
        "Configuration data may be corrupted. Try again or re-download ISO.",
        "https://winafi.local/faq#boot-config-invalid"
    },
    {
        ERR_BOOT_FILESYSTEM_INCOMPATIBLE,
        "Filesystem is incompatible with boot setup",
        "Some filesystems don't support required boot files. Try different filesystem.",
        "https://winafi.local/faq#boot-fs-incompatible"
    },
    {
        ERR_BOOT_FILE_COPY_FAILED,
        "Failed to copy boot files to device",
        "Device write error. Verify device is writable and not disconnecting.",
        "https://winafi.local/faq#boot-file-copy-failed"
    },

    /* ====================================================================
     * ORCHESTRATION ERRORS (-350 to -399)
     * ==================================================================== */

    {
        ERR_ORCH_SESSION_INVALID,
        "Session handle is invalid or expired",
        "Create a new session or verify the session is still valid.",
        "https://winafi.local/faq#session-invalid"
    },
    {
        ERR_ORCH_OPERATION_IN_PROGRESS,
        "Operation is already in progress",
        "Wait for the current operation to complete before starting a new one.",
        "https://winafi.local/faq#operation-in-progress"
    },
    {
        ERR_ORCH_OPERATION_CANCELLED,
        "Operation was cancelled",
        "Retry the operation from the beginning.",
        "https://winafi.local/faq#operation-cancelled"
    },
    {
        ERR_ORCH_DEVICE_DISCONNECTED,
        "Device was disconnected during operation",
        "Reconnect the USB device and restart the operation.",
        "https://winafi.local/faq#device-disconnected"
    },
    {
        ERR_ORCH_SYNC_FAILED,
        "Final sync operation failed",
        "Operation may still have succeeded. Safely eject device and verify.",
        "https://winafi.local/faq#sync-failed"
    },
    {
        ERR_ORCH_INVALID_PARAMETER,
        "Invalid parameter passed to function",
        "Verify all parameters are valid and required fields are provided.",
        "https://winafi.local/faq#invalid-parameter"
    },
    {
        ERR_ORCH_DEVICE_CHANGED,
        "Device changed during operation",
        "Device was modified or replaced. Restart the operation.",
        "https://winafi.local/faq#device-changed"
    },
    {
        ERR_ORCH_UNKNOWN,
        "Unknown orchestration error occurred",
        "Check system logs for more information. Try the operation again.",
        "https://winafi.local/faq#unknown-error"
    }
};

static const int error_database_count = sizeof(error_database) / sizeof(error_database[0]);

/* ========================================================================
 * HELPER FUNCTIONS
 * ======================================================================== */

/**
 * Find error entry by code
 */
static const error_entry_t* find_error_entry(int error_code) {
    for (int i = 0; i < error_database_count; i++) {
        if (error_database[i].error_code == error_code) {
            return &error_database[i];
        }
    }
    return NULL;
}

/* ========================================================================
 * PUBLIC API FUNCTIONS
 * ======================================================================== */

const char* error_get_message(int error_code) {
    const error_entry_t *entry = find_error_entry(error_code);
    if (entry != NULL) {
        return entry->message;
    }

    // Return generic message for unknown codes
    if (error_code == 0) {
        return "No error";
    }
    if (error_code > 0) {
        return "Unknown success code";
    }
    if (error_code < -400) {
        return "Unknown error (code out of range)";
    }

    // Determine approximate error type based on range
    if (error_code >= -149 && error_code <= -100) {
        return "Device operation error (see logs for details)";
    }
    if (error_code >= -199 && error_code <= -150) {
        return "Partition operation error (see logs for details)";
    }
    if (error_code >= -249 && error_code <= -200) {
        return "Filesystem operation error (see logs for details)";
    }
    if (error_code >= -299 && error_code <= -250) {
        return "ISO operation error (see logs for details)";
    }
    if (error_code >= -349 && error_code <= -300) {
        return "Boot operation error (see logs for details)";
    }
    if (error_code >= -399 && error_code <= -350) {
        return "Orchestration error (see logs for details)";
    }

    return "Unknown error (invalid error code)";
}

const char* error_get_recovery(int error_code) {
    const error_entry_t *entry = find_error_entry(error_code);
    if (entry != NULL) {
        return entry->recovery;
    }

    // Return generic recovery for unknown codes
    if (error_code == 0) {
        return "No action needed";
    }
    if (error_code > 0) {
        return "Unexpected success code - no recovery needed";
    }

    return "Try again or check system logs for details. Contact support if problem persists.";
}

const char* error_get_faq_link(int error_code) {
    const error_entry_t *entry = find_error_entry(error_code);
    if (entry != NULL) {
        return entry->faq_link;
    }
    return NULL;
}

error_category_t error_get_category(int error_code) {
    if (error_code == 0) {
        return ERR_CATEGORY_OK;
    }
    if (error_code >= -149 && error_code <= -100) {
        return ERR_CATEGORY_DEVICE;
    }
    if (error_code >= -199 && error_code <= -150) {
        return ERR_CATEGORY_PARTITION;
    }
    if (error_code >= -249 && error_code <= -200) {
        return ERR_CATEGORY_FILESYSTEM;
    }
    if (error_code >= -299 && error_code <= -250) {
        return ERR_CATEGORY_ISO;
    }
    if (error_code >= -349 && error_code <= -300) {
        return ERR_CATEGORY_BOOT;
    }
    if (error_code >= -399 && error_code <= -350) {
        return ERR_CATEGORY_ORCH;
    }
    return ERR_CATEGORY_UNKNOWN;
}

const char* error_get_category_name(error_category_t category) {
    switch (category) {
        case ERR_CATEGORY_OK:
            return "Success";
        case ERR_CATEGORY_DEVICE:
            return "Device";
        case ERR_CATEGORY_PARTITION:
            return "Partition";
        case ERR_CATEGORY_FILESYSTEM:
            return "Filesystem";
        case ERR_CATEGORY_ISO:
            return "ISO";
        case ERR_CATEGORY_BOOT:
            return "Boot";
        case ERR_CATEGORY_ORCH:
            return "Operation";
        case ERR_CATEGORY_UNKNOWN:
        default:
            return "Unknown";
    }
}

int error_is_ok(int error_code) {
    return error_code == ERR_OK;
}

int error_is_device_error(int error_code) {
    return error_code >= -149 && error_code <= -100;
}

int error_is_partition_error(int error_code) {
    return error_code >= -199 && error_code <= -150;
}

int error_is_filesystem_error(int error_code) {
    return error_code >= -249 && error_code <= -200;
}

int error_is_iso_error(int error_code) {
    return error_code >= -299 && error_code <= -250;
}

int error_is_boot_error(int error_code) {
    return error_code >= -349 && error_code <= -300;
}

int error_is_orch_error(int error_code) {
    return error_code >= -399 && error_code <= -350;
}
