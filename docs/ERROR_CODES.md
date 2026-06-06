# Winify Error Code System

## Overview

The error code system provides a unified, comprehensive approach to error handling across all Winify components. With 200+ error codes organized by category, user-friendly messages, and recovery suggestions, it ensures consistent error reporting throughout the application.

## Architecture

### Error Code Organization

All error codes follow a consistent negative numbering scheme, organized into 6 categories:

| Category | Range | Count | Purpose |
|----------|-------|-------|---------|
| Device | -100 to -149 | 15 | Device enumeration, validation, and access |
| Partition | -150 to -199 | 15 | Partition table creation and management |
| Filesystem | -200 to -249 | 14 | Filesystem creation and formatting |
| ISO | -250 to -299 | 14 | ISO handling, validation, and extraction |
| Boot | -300 to -349 | 14 | Bootloader installation and boot setup |
| Orchestration | -350 to -399 | 8 | Operation orchestration and session management |
| **Success** | **0** | **1** | **Operation completed successfully** |

**Total: 200 error codes + 1 success code**

### Design Principles

1. **Negative for errors**: All error codes are negative integers, making it trivial to distinguish success from failure
2. **Categorized**: Each error falls into exactly one category, enabling category-based error filtering
3. **User-friendly**: Messages are clear and non-technical, suitable for end users
4. **Actionable**: Each error includes recovery suggestions
5. **Documented**: FAQ links provided for common issues

## Error Code Reference

### Device Errors (-100 to -149)

Device errors cover all aspects of device enumeration, validation, and access.

```c
ERR_DEV_NOT_FOUND (-100)              // Device doesn't exist
ERR_DEV_PERMISSION_DENIED (-101)      // No write permission
ERR_DEV_DEVICE_BUSY (-102)            // Device is mounted or in use
ERR_DEV_DEVICE_TOO_SMALL (-103)       // Insufficient device capacity
ERR_DEV_SYSTEM_DRIVE (-104)           // Device is system/root drive
ERR_DEV_DEVICE_LOCKED (-105)          // Device locked by another process
ERR_DEV_READ_FAILED (-106)            // Failed to read device info
ERR_DEV_WRITE_FAILED (-107)           // Failed to write to device
ERR_DEV_CAPACITY_CHECK (-108)         // Device capacity validation failed
ERR_DEV_MOUNTED_PARTITION (-109)      // Partition is currently mounted
ERR_DEV_SIZE_VALIDATION (-110)        // Device size validation failed
ERR_DEV_FS_CHECK_FAILED (-111)        // Filesystem check failed
ERR_DEV_NO_SUCH_DEVICE (-112)         // Device node doesn't exist
ERR_DEV_NOT_BLOCK_DEVICE (-113)       // Not a block device
ERR_DEV_PARTITION_STILL_MOUNTED (-114) // Partition not unmounted
```

**Common Causes:**
- USB device not properly connected
- Device permissions not set correctly (need sudo)
- Device in use by another application
- Device capacity too small for ISO

### Partition Errors (-150 to -199)

Partition errors cover partition table creation and management.

```c
ERR_PART_INVALID_TABLE (-150)         // Invalid or corrupted partition table
ERR_PART_MBR_FULL (-151)              // MBR full (max 4 primary partitions)
ERR_PART_CREATE_FAILED (-152)         // Partition creation failed
ERR_PART_ALIGNMENT_FAILED (-153)      // Partition alignment failed
ERR_PART_INVALID_SIZE (-154)          // Invalid partition size
ERR_PART_GPT_CONVERSION (-155)        // GPT conversion failed
ERR_PART_LIBPARTED_ERROR (-156)       // libparted library error
ERR_PART_INVALID_PARTITION (-157)     // Invalid partition number
ERR_PART_BOOT_FLAG_FAILED (-158)      // Boot flag setting failed
ERR_PART_DELETE_FAILED (-159)         // Partition deletion failed
ERR_PART_WRONG_TYPE (-160)            // Wrong partition type
ERR_PART_NO_SPACE (-161)              // No space for partition
ERR_PART_TABLE_READ_FAILED (-162)     // Partition table read failed
ERR_PART_TABLE_WRITE_FAILED (-163)    // Partition table write failed
ERR_PART_SECTOR_TOO_LARGE (-164)      // Device sector size too large
```

**Common Causes:**
- Device write protection enabled
- libparted not installed or incompatible
- MBR partition table full (max 4 partitions)
- Device connectivity issues

### Filesystem Errors (-200 to -249)

Filesystem errors cover creation and formatting operations.

```c
ERR_FS_LABEL_TOO_LONG (-200)          // Volume label exceeds length
ERR_FS_INVALID_CHARS (-201)           // Invalid characters in label
ERR_FS_INVALID_SIZE (-202)            // Invalid filesystem size
ERR_FS_MKFS_FAILED (-203)             // Filesystem creation failed
ERR_FS_TOOL_MISSING (-204)            // mkfs tool not installed
ERR_FS_UNSUPPORTED (-205)             // Unsupported filesystem type
ERR_FS_FORMAT_TIMEOUT (-206)          // Formatting timed out
ERR_FS_PARTITION_NOT_FOUND (-207)     // Target partition missing
ERR_FS_INCOMPATIBLE_BOOT (-208)       // FS incompatible with boot type
ERR_FS_NO_SPACE_LEFT (-209)           // No space left on device
ERR_FS_LABEL_UNICODE (-210)           // Unicode label issues
ERR_FS_PERMISSION_DENIED (-211)       // Permission denied
ERR_FS_UNKNOWN_TYPE (-212)            // Unknown filesystem type
ERR_FS_CLUSTER_SIZE_INVALID (-213)    // Invalid cluster size
```

**Common Causes:**
- Missing mkfs tools (mkfs.vfat, mkfs.ntfs, etc.)
- Volume label too long or contains invalid characters
- Device doesn't have enough free space
- UEFI boot selected with NTFS (requires FAT32)

**Label Length Limits:**
- FAT32: 11 characters
- NTFS: 32 characters
- exFAT: 15 characters

### ISO Errors (-250 to -299)

ISO errors cover file validation, handling, and extraction.

```c
ERR_ISO_NOT_FOUND (-250)              // ISO file not found
ERR_ISO_NOT_ISO (-251)                // Not a valid ISO 9660 image
ERR_ISO_ARCHIVE_ERROR (-252)          // Error reading ISO archive
ERR_ISO_EXTRACT_FAILED (-253)         // File extraction failed
ERR_ISO_NO_BOOT_INFO (-254)           // No boot information in ISO
ERR_ISO_CORRUPTED (-255)              // ISO file appears corrupted
ERR_ISO_INCOMPLETE_EXTRACT (-256)     // Extraction completed but may be incomplete
ERR_ISO_INSUFFICIENT_SPACE (-257)     // Not enough space on device
ERR_ISO_EXTRACT_TIMEOUT (-258)        // Extraction timed out
ERR_ISO_FILE_NOT_FOUND (-259)         // Required file not found in ISO
ERR_ISO_WRITE_PERMISSION (-260)       // Write permission denied
ERR_ISO_DIRECTORY_CREATE_FAILED (-261) // Directory creation failed
ERR_ISO_SYMLINK_CREATE_FAILED (-262)  // Symbolic link creation failed
ERR_ISO_EXTRACTION_CANCELLED (-263)   // Extraction cancelled by user
```

**Common Causes:**
- ISO file path incorrect or file deleted
- ISO file corrupted (checksum mismatch)
- ISO not a valid ISO 9660 image
- Device disconnected during extraction
- Insufficient device space

### Boot Errors (-300 to -349)

Boot errors cover bootloader installation and boot environment setup.

```c
ERR_BOOT_NO_BOOTLOADER (-300)         // No bootloader found
ERR_BOOT_UNSUPPORTED_BOOT (-301)      // Unsupported boot type
ERR_BOOT_GRUB2_MISSING (-302)         // GRUB2 bootloader not installed
ERR_BOOT_SYSLINUX_MISSING (-303)      // Syslinux bootloader not installed
ERR_BOOT_BOOTCODE_MISSING (-304)      // Bootcode file missing
ERR_BOOT_INSTALL_FAILED (-305)        // Bootloader installation failed
ERR_BOOT_UEFI_FAT32_REQUIRED (-306)   // UEFI requires FAT32 filesystem
ERR_BOOT_SECURE_BOOT_ENABLED (-307)   // Secure Boot may prevent booting
ERR_BOOT_ESP_CREATION_FAILED (-308)   // EFI System Partition creation failed
ERR_BOOT_BOOTCODE_TOO_LARGE (-309)    // Bootcode file too large (>446 bytes)
ERR_BOOT_SIGNATURE_WRITE_FAILED (-310) // MBR signature write failed
ERR_BOOT_CONFIG_INVALID (-311)        // Boot configuration is invalid
ERR_BOOT_FILESYSTEM_INCOMPATIBLE (-312) // Filesystem incompatible with boot
ERR_BOOT_FILE_COPY_FAILED (-313)      // Boot file copy failed
```

**Common Causes:**
- Missing bootloader tools on system
- UEFI boot selected with NTFS (requires FAT32)
- Secure Boot enabled in BIOS/UEFI
- ISO is not bootable or missing boot files
- Device write protection enabled

### Orchestration Errors (-350 to -399)

Orchestration errors cover operation management and session state.

```c
ERR_ORCH_SESSION_INVALID (-350)       // Session handle is invalid or expired
ERR_ORCH_OPERATION_IN_PROGRESS (-351) // Operation is already in progress
ERR_ORCH_OPERATION_CANCELLED (-352)   // Operation was cancelled by user
ERR_ORCH_DEVICE_DISCONNECTED (-353)   // Device was disconnected
ERR_ORCH_SYNC_FAILED (-354)           // Final sync operation failed
ERR_ORCH_INVALID_PARAMETER (-355)     // Invalid parameter passed
ERR_ORCH_DEVICE_CHANGED (-356)        // Device changed during operation
ERR_ORCH_UNKNOWN (-399)               // Unknown orchestration error
```

**Common Causes:**
- Device disconnected during operation
- User cancelled operation
- Invalid session or parameter
- Concurrent operations attempted

## Error Lookup Functions

### Basic Lookups

```c
const char* error_get_message(int error_code);
```
Returns the user-friendly error message for the given error code.

```c
const char* error_get_recovery(int error_code);
```
Returns the suggested recovery action for the error.

```c
const char* error_get_faq_link(int error_code);
```
Returns the FAQ URL for the error, or NULL if not available.

```c
error_category_t error_get_category(int error_code);
```
Returns the error category (device, partition, filesystem, etc.).

```c
const char* error_get_category_name(error_category_t category);
```
Returns the human-readable name of the error category.

### Checking Error Types

```c
int error_is_ok(int error_code);
int error_is_device_error(int error_code);
int error_is_partition_error(int error_code);
int error_is_filesystem_error(int error_code);
int error_is_iso_error(int error_code);
int error_is_boot_error(int error_code);
int error_is_orch_error(int error_code);
```

Each function returns 1 if the error code belongs to that category, 0 otherwise.

## Usage Examples

### Basic Error Handling

```c
int ret = do_some_operation();
if (ret != ERR_OK) {
    fprintf(stderr, "Error: %s\n", error_get_message(ret));
    fprintf(stderr, "Recovery: %s\n", error_get_recovery(ret));
    if (error_get_faq_link(ret) != NULL) {
        fprintf(stderr, "FAQ: %s\n", error_get_faq_link(ret));
    }
    return ret;
}
```

### Category-Based Error Handling

```c
int ret = some_operation();
if (!error_is_ok(ret)) {
    error_category_t category = error_get_category(ret);
    
    switch (category) {
        case ERR_CATEGORY_DEVICE:
            // Handle device-specific recovery
            break;
        case ERR_CATEGORY_ISO:
            // Handle ISO-specific recovery
            break;
        // ... etc
    }
}
```

### Logging Errors

```c
void log_error(int error_code) {
    fprintf(stderr, "[%s] %s\n",
            error_get_category_name(error_get_category(error_code)),
            error_get_message(error_code));
}
```

## Error Categories and Recovery Strategy

### Device Errors
**Recovery Strategy:** Check hardware connectivity
- Ensure USB device is properly connected
- Verify device appears in system
- Check device permissions with `lsof` or `fuser`
- Try different USB port

### Partition Errors
**Recovery Strategy:** Validate device state and permissions
- Verify device is writable
- Ensure no partitions are mounted
- Check that disk space is available
- Consider using GPT instead of MBR if applicable

### Filesystem Errors
**Recovery Strategy:** Install missing tools and verify parameters
- Install missing mkfs tools
- Use valid filesystem type (FAT32, NTFS, exFAT)
- Ensure volume label meets constraints
- Select compatible filesystem for boot type

### ISO Errors
**Recovery Strategy:** Verify ISO file and device space
- Verify ISO file path is correct
- Re-download ISO if corrupted
- Check ISO checksum against official source
- Ensure device has sufficient free space
- Verify device is not disconnecting

### Boot Errors
**Recovery Strategy:** Ensure boot compatibility and install tools
- Install required bootloader tools (GRUB2, Syslinux)
- Select compatible boot mode for hardware
- Use FAT32 for UEFI boot
- Verify ISO contains bootable content
- Check BIOS/UEFI Secure Boot settings

### Orchestration Errors
**Recovery Strategy:** Manage operation state
- Retry after current operation completes
- Reconnect device if disconnected
- Create new session if expired
- Verify valid parameters passed

## Integration with Existing Code

### Mapping Old Error Codes

Existing Phase 5 components use their own error codes. They should be mapped to the unified system:

| Component | Old Code | New Code | Description |
|-----------|----------|----------|-------------|
| device_validate.h | -1 | -103 | Device too small |
| device_validate.h | -2 | -104 | System drive |
| device_validate.h | -3 | -105 | Device locked |
| device_validate.h | -4 | -102 | Device busy |
| device_validate.h | -5 | -100 | Not found |
| filesystem.h | -1 | -200 | Label too long |
| filesystem.h | -2 | -201 | Invalid chars |
| partition.h | -1 | -156 | libparted error |
| iso_extract.h | ISO_OK | 0 | Success |
| iso_extract.h | ISO_ERR_FILE_NOT_FOUND | -250 | Not found |
| mbr_bootloader.h | -1 | -112 | Device not found |

### Updating Component Headers

Components should include the unified error_codes.h and use the new constants:

```c
#include "platform/linux/error_codes.h"

// Use new error codes
if (some_error) {
    return ERR_DEV_NOT_FOUND;  // Instead of custom -1
}
```

## Testing

The error code system includes comprehensive unit tests:

```
test_error_codes: 16 test functions, 200+ assertions

✓ Device error codes defined
✓ Partition error codes defined
✓ Filesystem error codes defined
✓ ISO error codes defined
✓ Boot error codes defined
✓ Orchestration error codes defined
✓ Error message lookups work
✓ Error recovery lookups work
✓ Error category detection works
✓ Error FAQ links available
✓ Invalid error codes handled
✓ All error codes have messages
✓ All error codes have recovery suggestions
✓ Error codes use proper ranges
✓ All error codes are unique
✓ Messages are user-friendly
```

Run with: `./tests/test_error_codes`

## Best Practices

1. **Always check error codes**: Don't assume success
2. **Use category functions**: Better error handling with `error_is_X_error()`
3. **Display messages to users**: Use `error_get_message()` not error codes
4. **Provide recovery suggestions**: Always show `error_get_recovery()`
5. **Include FAQ links**: When available, show `error_get_faq_link()`
6. **Log full context**: Include category, message, and recovery in logs
7. **Handle unknown codes**: Fallback messages provided for future codes
8. **Test error paths**: Unit tests for all error scenarios

## Future Extensions

The error code system is designed to be extended:

1. **Add new codes**: Use next available number in range
2. **Add messages**: Update error_messages.c database
3. **Add recovery suggestions**: Always provide actionable recovery steps
4. **Add FAQ links**: Document common issues and solutions
5. **New categories**: Define new ranges for new components (e.g., -400 to -449)

## References

- **Main Header**: `include/error_codes.h`
- **Implementation**: `src/platform/linux/error_messages.c`
- **Local Header**: `src/platform/linux/error_codes.h`
- **Tests**: `tests/unit/test_error_codes.c`
- **CMake**: `tests/CMakeLists.txt` (test_error_codes target)
