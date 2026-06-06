#ifndef WINAFI_ERROR_H
#define WINAFI_ERROR_H

#include <stdint.h>

typedef int winafi_error_t;

#define WINAFI_OK 0

// Device errors
#define WINAFI_ERR_DEVICE_NOT_FOUND -1
#define WINAFI_ERR_DEVICE_NOT_USB -2
#define WINAFI_ERR_DEVICE_TOO_SMALL -3
#define WINAFI_ERR_DEVICE_READ_FAILED -4
#define WINAFI_ERR_DEVICE_IS_SYSTEM -5
#define WINAFI_ERR_DEVICE_MOUNTED -6
#define WINAFI_ERR_DEVICE_LOCKED -7
#define WINAFI_ERR_DEVICE_OPEN_FAILED -8
#define WINAFI_ERR_DEVICE_READONLY -9
#define WINAFI_ERR_NO_PERMISSION -10

// ISO errors (continue from -11)
#define WINAFI_ERR_ISO_NOT_FOUND -11
#define WINAFI_ERR_ISO_NOT_READABLE -12
#define WINAFI_ERR_ISO_INVALID -13
#define WINAFI_ERR_ISO_NOT_WINDOWS -14
#define WINAFI_ERR_WINDOWS_VERSION_UNKNOWN -15
#define WINAFI_ERR_ISO_TOO_LARGE -16
#define WINAFI_ERR_ISO_EXTRACTION_FAILED -17

// Partition errors
#define WINAFI_ERR_PARTITION_TABLE_FAILED -20
#define WINAFI_ERR_PARTITION_CREATE_FAILED -21
#define WINAFI_ERR_PARTITION_WRITE_FAILED -22
#define WINAFI_ERR_PARTITION_TOO_SMALL -23
#define WINAFI_ERR_PARTITION_ALIGNMENT_FAILED -24

// Format errors
#define WINAFI_ERR_FAT32_FORMAT_FAILED -30
#define WINAFI_ERR_NTFS_FORMAT_FAILED -31
#define WINAFI_ERR_FORMAT_TIMEOUT -32
#define WINAFI_ERR_FORMAT_MISSING_TOOL -33

// Mount errors
#define WINAFI_ERR_MOUNT_FAILED -40
#define WINAFI_ERR_UNMOUNT_FAILED -41
#define WINAFI_ERR_SYNC_FAILED -42

// Copy errors
#define WINAFI_ERR_COPY_FAILED -50
#define WINAFI_ERR_COPY_INCOMPLETE -51
#define WINAFI_ERR_COPY_TIMEOUT -52
#define WINAFI_ERR_DISK_FULL -53

// Bootloader errors
#define WINAFI_ERR_GRUB_INSTALL_FAILED -60
#define WINAFI_ERR_GRUB_CONFIG_FAILED -61
#define WINAFI_ERR_GRUB_MISSING -62

// System errors
#define WINAFI_ERR_OUT_OF_MEMORY -70
#define WINAFI_ERR_TEMP_DIR_FAILED -71
#define WINAFI_ERR_SYSTEM_CALL_FAILED -72

// Session/error management
typedef struct winafi_session winafi_session_t;

// Load error dictionary from errors.txt
// Note: This MUST be called before using error_lookup() or error_format()
int error_init(void);
void error_cleanup(void);

// Get error message for code
// Returns: pointer to static error message (do NOT free), or NULL if code not found
const char *error_lookup(const char *code);

// Format error with context
// Returns: allocated string (caller MUST free with free()), or NULL on allocation failure
// Note: String codes (e.g., "E-00-A") are the primary error system; integer codes in
// error.h are legacy/deprecated and provided for compatibility only.
char *error_format(const char *code, const char *context);

#endif
