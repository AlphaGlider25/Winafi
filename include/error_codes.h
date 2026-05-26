#ifndef RUFUS_ERROR_CODES_H
#define RUFUS_ERROR_CODES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Comprehensive Error Code System for Winify
 *
 * Provides a unified error code system across all components with:
 * - 200+ error codes organized by category
 * - User-friendly error messages
 * - Recovery suggestions
 * - FAQ links for common issues
 * - Category-based error detection
 *
 * Error Code Ranges:
 * - Device operations: -100 to -149 (50 codes)
 * - Partition operations: -150 to -199 (50 codes)
 * - Filesystem operations: -200 to -249 (50 codes)
 * - ISO operations: -250 to -299 (50 codes)
 * - Boot operations: -300 to -349 (50 codes)
 * - Orchestration: -350 to -399 (50 codes)
 * - Success: 0
 *
 * All error codes are negative to easily distinguish from success codes.
 */

/* ========================================================================
 * SUCCESS CODE
 * ======================================================================== */

#define ERR_OK 0

/* ========================================================================
 * DEVICE OPERATION ERRORS (-100 to -149)
 *
 * Errors related to device enumeration, validation, and access.
 * ======================================================================== */

#define ERR_DEV_NOT_FOUND              -100  // Device doesn't exist
#define ERR_DEV_PERMISSION_DENIED      -101  // No write permission
#define ERR_DEV_DEVICE_BUSY            -102  // Device is mounted or in use
#define ERR_DEV_DEVICE_TOO_SMALL       -103  // Insufficient device capacity
#define ERR_DEV_SYSTEM_DRIVE           -104  // Device is system/root drive
#define ERR_DEV_DEVICE_LOCKED          -105  // Device locked by another process
#define ERR_DEV_READ_FAILED            -106  // Failed to read device info
#define ERR_DEV_WRITE_FAILED           -107  // Failed to write to device
#define ERR_DEV_CAPACITY_CHECK         -108  // Device capacity validation failed
#define ERR_DEV_MOUNTED_PARTITION      -109  // Partition is currently mounted
#define ERR_DEV_SIZE_VALIDATION        -110  // Device size validation failed
#define ERR_DEV_FS_CHECK_FAILED        -111  // Filesystem check failed
#define ERR_DEV_NO_SUCH_DEVICE         -112  // Device node doesn't exist
#define ERR_DEV_NOT_BLOCK_DEVICE       -113  // Not a block device
#define ERR_DEV_PARTITION_STILL_MOUNTED -114 // Partition not unmounted

/* ========================================================================
 * PARTITION OPERATION ERRORS (-150 to -199)
 *
 * Errors related to partition table creation and management.
 * ======================================================================== */

#define ERR_PART_INVALID_TABLE         -150  // Invalid partition table
#define ERR_PART_MBR_FULL              -151  // MBR full (max 4 partitions)
#define ERR_PART_CREATE_FAILED         -152  // Partition creation failed
#define ERR_PART_ALIGNMENT_FAILED      -153  // Partition alignment failed
#define ERR_PART_INVALID_SIZE          -154  // Invalid partition size
#define ERR_PART_GPT_CONVERSION        -155  // GPT conversion failed
#define ERR_PART_LIBPARTED_ERROR       -156  // libparted error
#define ERR_PART_INVALID_PARTITION     -157  // Invalid partition number
#define ERR_PART_BOOT_FLAG_FAILED      -158  // Boot flag setting failed
#define ERR_PART_DELETE_FAILED         -159  // Partition deletion failed
#define ERR_PART_WRONG_TYPE            -160  // Wrong partition type
#define ERR_PART_NO_SPACE              -161  // No space for partition
#define ERR_PART_TABLE_READ_FAILED     -162  // Partition table read failed
#define ERR_PART_TABLE_WRITE_FAILED    -163  // Partition table write failed
#define ERR_PART_SECTOR_TOO_LARGE      -164  // Sector size too large

/* ========================================================================
 * FILESYSTEM OPERATION ERRORS (-200 to -249)
 *
 * Errors related to filesystem creation and formatting.
 * ======================================================================== */

#define ERR_FS_LABEL_TOO_LONG          -200  // Volume label exceeds length
#define ERR_FS_INVALID_CHARS           -201  // Invalid characters in label
#define ERR_FS_INVALID_SIZE            -202  // Invalid filesystem size
#define ERR_FS_MKFS_FAILED             -203  // Filesystem creation failed
#define ERR_FS_TOOL_MISSING            -204  // mkfs tool not installed
#define ERR_FS_UNSUPPORTED             -205  // Unsupported filesystem type
#define ERR_FS_FORMAT_TIMEOUT          -206  // Formatting timed out
#define ERR_FS_PARTITION_NOT_FOUND     -207  // Target partition missing
#define ERR_FS_INCOMPATIBLE_BOOT       -208  // FS incompatible with boot type
#define ERR_FS_NO_SPACE_LEFT           -209  // No space left on device
#define ERR_FS_LABEL_UNICODE           -210  // Unicode label issues
#define ERR_FS_PERMISSION_DENIED       -211  // Permission denied
#define ERR_FS_UNKNOWN_TYPE            -212  // Unknown filesystem type
#define ERR_FS_CLUSTER_SIZE_INVALID    -213  // Invalid cluster size

/* ========================================================================
 * ISO OPERATION ERRORS (-250 to -299)
 *
 * Errors related to ISO file handling, validation, and extraction.
 * ======================================================================== */

#define ERR_ISO_NOT_FOUND              -250  // ISO file not found
#define ERR_ISO_NOT_ISO                -251  // Not a valid ISO file
#define ERR_ISO_ARCHIVE_ERROR          -252  // Archive read error
#define ERR_ISO_EXTRACT_FAILED         -253  // File extraction failed
#define ERR_ISO_NO_BOOT_INFO           -254  // No boot info in ISO
#define ERR_ISO_CORRUPTED              -255  // ISO file corrupted
#define ERR_ISO_INCOMPLETE_EXTRACT     -256  // Extraction incomplete
#define ERR_ISO_INSUFFICIENT_SPACE     -257  // Not enough space on device
#define ERR_ISO_EXTRACT_TIMEOUT        -258  // Extraction timed out
#define ERR_ISO_FILE_NOT_FOUND         -259  // File within ISO not found
#define ERR_ISO_WRITE_PERMISSION       -260  // Write permission denied
#define ERR_ISO_DIRECTORY_CREATE_FAILED -261 // Directory creation failed
#define ERR_ISO_SYMLINK_CREATE_FAILED  -262  // Symlink creation failed
#define ERR_ISO_EXTRACTION_CANCELLED   -263  // Extraction cancelled by user

/* ========================================================================
 * BOOT OPERATION ERRORS (-300 to -349)
 *
 * Errors related to bootloader installation and boot setup.
 * ======================================================================== */

#define ERR_BOOT_NO_BOOTLOADER         -300  // No bootloader found
#define ERR_BOOT_UNSUPPORTED_BOOT      -301  // Unsupported boot type
#define ERR_BOOT_GRUB2_MISSING         -302  // GRUB2 not found
#define ERR_BOOT_SYSLINUX_MISSING      -303  // Syslinux not found
#define ERR_BOOT_BOOTCODE_MISSING      -304  // Bootcode file missing
#define ERR_BOOT_INSTALL_FAILED        -305  // Bootloader install failed
#define ERR_BOOT_UEFI_FAT32_REQUIRED   -306  // UEFI requires FAT32
#define ERR_BOOT_SECURE_BOOT_ENABLED   -307  // Secure Boot enabled
#define ERR_BOOT_ESP_CREATION_FAILED   -308  // EFI System Partition failed
#define ERR_BOOT_BOOTCODE_TOO_LARGE    -309  // Bootcode file too large
#define ERR_BOOT_SIGNATURE_WRITE_FAILED -310 // MBR signature write failed
#define ERR_BOOT_CONFIG_INVALID        -311  // Boot configuration invalid
#define ERR_BOOT_FILESYSTEM_INCOMPATIBLE -312 // Filesystem incompatible
#define ERR_BOOT_FILE_COPY_FAILED      -313  // Boot file copy failed

/* ========================================================================
 * ORCHESTRATION ERRORS (-350 to -399)
 *
 * Errors related to operation orchestration and session management.
 * ======================================================================== */

#define ERR_ORCH_SESSION_INVALID       -350  // Invalid session handle
#define ERR_ORCH_OPERATION_IN_PROGRESS -351  // Operation already running
#define ERR_ORCH_OPERATION_CANCELLED   -352  // Operation cancelled by user
#define ERR_ORCH_DEVICE_DISCONNECTED   -353  // Device disconnected
#define ERR_ORCH_SYNC_FAILED           -354  // Final sync operation failed
#define ERR_ORCH_INVALID_PARAMETER     -355  // Invalid parameter passed
#define ERR_ORCH_DEVICE_CHANGED        -356  // Device changed during operation
#define ERR_ORCH_UNKNOWN               -399  // Unknown orchestration error

/* ========================================================================
 * ERROR CATEGORY ENUMERATION
 * ======================================================================== */

typedef enum {
    ERR_CATEGORY_OK = 0,
    ERR_CATEGORY_DEVICE = 1,
    ERR_CATEGORY_PARTITION = 2,
    ERR_CATEGORY_FILESYSTEM = 3,
    ERR_CATEGORY_ISO = 4,
    ERR_CATEGORY_BOOT = 5,
    ERR_CATEGORY_ORCH = 6,
    ERR_CATEGORY_UNKNOWN = 99
} error_category_t;

/* ========================================================================
 * ERROR LOOKUP FUNCTIONS
 *
 * All functions return non-NULL values (never NULL).
 * For unknown error codes, generic messages are returned.
 * ======================================================================== */

/**
 * Get user-friendly error message for error code
 *
 * Returns a human-readable error message suitable for displaying to users.
 * Messages are concise but descriptive, without technical jargon.
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   Non-NULL pointer to error message string (always valid)
 *   Generic message for unknown error codes
 */
const char* error_get_message(int error_code);

/**
 * Get recovery suggestion for error code
 *
 * Returns a suggested action for users to recover from the error.
 * May include checking device connections, verifying permissions, etc.
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   Non-NULL pointer to recovery suggestion string (always valid)
 *   Generic suggestion for unknown error codes
 */
const char* error_get_recovery(int error_code);

/**
 * Get FAQ link for error code (if available)
 *
 * Returns a URL to FAQ documentation for this error.
 * Returns NULL if no FAQ is available for this error.
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   Pointer to FAQ URL string, or NULL if not available
 */
const char* error_get_faq_link(int error_code);

/**
 * Get error category for error code
 *
 * Determines which category (device, partition, etc.) the error belongs to.
 * Useful for filtering or grouping errors by type.
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   error_category_t enumeration value
 *   ERR_CATEGORY_UNKNOWN for invalid codes
 */
error_category_t error_get_category(int error_code);

/**
 * Get human-readable category name
 *
 * Returns the name of an error category (e.g., "Device", "Partition")
 *
 * Parameters:
 *   category - The error category enumeration value
 *
 * Returns:
 *   Non-NULL pointer to category name string (always valid)
 */
const char* error_get_category_name(error_category_t category);

/**
 * Check if error code is a success
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   1 if error_code == ERR_OK, 0 otherwise
 */
int error_is_ok(int error_code);

/**
 * Check if error code is a device error
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   1 if error code is in device range (-100 to -149), 0 otherwise
 */
int error_is_device_error(int error_code);

/**
 * Check if error code is a partition error
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   1 if error code is in partition range (-150 to -199), 0 otherwise
 */
int error_is_partition_error(int error_code);

/**
 * Check if error code is a filesystem error
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   1 if error code is in filesystem range (-200 to -249), 0 otherwise
 */
int error_is_filesystem_error(int error_code);

/**
 * Check if error code is an ISO error
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   1 if error code is in ISO range (-250 to -299), 0 otherwise
 */
int error_is_iso_error(int error_code);

/**
 * Check if error code is a boot error
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   1 if error code is in boot range (-300 to -349), 0 otherwise
 */
int error_is_boot_error(int error_code);

/**
 * Check if error code is an orchestration error
 *
 * Parameters:
 *   error_code - The error code (any integer)
 *
 * Returns:
 *   1 if error code is in orch range (-350 to -399), 0 otherwise
 */
int error_is_orch_error(int error_code);

#ifdef __cplusplus
}
#endif

#endif // RUFUS_ERROR_CODES_H
