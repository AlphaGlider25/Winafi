# Winify Public API Documentation

**Version**: 4.0.0  
**Status**: Phase 2 (Core Implementation)  
**Last Updated**: 2026-05-25

## Overview

The Winify library (`librufus`) provides a complete API for creating bootable Windows USB drives on Linux. The library handles:
- USB device enumeration and validation
- Partition table creation (MBR)
- Filesystem formatting (FAT32 + NTFS)
- ISO extraction and file placement
- Bootloader setup (BIOS and UEFI)
- State machine orchestration

All operations are synchronous (blocking). Error handling uses standard error codes.

## Header Files

### Main Header: `<rufus.h>`

Include this for all library functionality:
```c
#include <rufus.h>
```

Public API types and functions are defined here. All library operations require initialization through this API.

---

## Core Types

### Session Handle
```c
typedef struct rufus_session rufus_session_t;
```

Opaque handle to a session state. Created by `rufus_session_create()` and destroyed by `rufus_session_destroy()`.

### Device Structure
```c
typedef struct {
    char devnode[256];           // e.g., "/dev/sdb"
    char sysname[32];            // e.g., "sdb"
    uint64_t capacity_bytes;     // Total capacity in bytes
    char vendor[256];            // e.g., "Kingston"
    char model[256];             // e.g., "DataTraveler 3.0"
    char serial[256];            // e.g., "ABC123XYZ"
} rufus_device_t;
```

### Error Codes

All functions return integer error codes:
- **0** = Success
- **Non-zero** = Error (see Error Code Reference section)

Error messages are retrieved via `error_lookup()` function.

---

## Core Infrastructure API

### Error Handling

#### `int error_init(void)`
**Purpose**: Initialize error dictionary at startup

**Parameters**: None

**Returns**: 
- 0 on success
- -1 if error dictionary cannot be found

**Notes**:
- Must be called once during application startup
- Searches for errors.txt in multiple locations:
  1. `src/errors.txt` (development)
  2. `../src/errors.txt` (relative fallback)
  3. `/usr/share/winify/errors.txt` (installed)
- Failure is fatal; cannot proceed without error dictionary

**Example**:
```c
if (error_init() < 0) {
    fprintf(stderr, "Could not load error dictionary\n");
    return 1;
}
```

#### `const char *error_lookup(int code)`
**Purpose**: Get error message for a code

**Parameters**:
- `code`: Error code (E-XX-Y format stored as integer)

**Returns**: 
- Pointer to human-readable error message (static, don't free)
- "Unknown error" if code not found

**Example**:
```c
int ret = session_execute(session);
if (ret < 0) {
    printf("Error: %s\n", error_lookup(ret));
}
```

#### `char *error_format(int code, const char *context)`
**Purpose**: Format error message with additional context

**Parameters**:
- `code`: Error code
- `context`: Additional information (e.g., device path)

**Returns**: 
- Malloc'd formatted message (caller must free)
- NULL if malloc fails

**Example**:
```c
char *msg = error_format(E_00_A, "/dev/sdb");
printf("%s\n", msg);
free(msg);
```

#### `void error_cleanup(void)`
**Purpose**: Cleanup error dictionary

**Parameters**: None

**Returns**: None

**Notes**: Optional; called automatically at program exit. Useful for valgrind cleanliness.

---

### Progress & Logging

#### `void progress_set_callback(rufus_progress_callback_t callback, void *user_data)`
**Purpose**: Register callback for progress updates

**Parameters**:
- `callback`: Function to call with progress updates
- `user_data`: Opaque pointer passed to callback

**Callback Signature**:
```c
typedef void (*rufus_progress_callback_t)(
    int percent,              // 0-100
    const char *message,      // Status message
    void *user_data          // User context
);
```

**Example**:
```c
static void on_progress(int percent, const char *msg, void *data) {
    printf("[%d%%] %s\n", percent, msg);
}

progress_set_callback(on_progress, NULL);
```

#### `void progress_fire(int percent, const char *message)`
**Purpose**: Fire a progress event

**Parameters**:
- `percent`: Progress percentage (0-100, auto-clamped)
- `message`: Status message

**Returns**: None

**Notes**: Directly invokes the registered callback (synchronous). Safe to call frequently; not performance-sensitive.

#### `void log_init(int level)`
**Purpose**: Set logging level

**Parameters**:
- `level`: RUFUS_LOG_DEBUG (0), RUFUS_LOG_INFO (1), or RUFUS_LOG_ERROR (2)

**Returns**: None

#### `void log_msg(int level, const char *fmt, ...)`
**Purpose**: Log a message

**Parameters**:
- `level`: Log level
- `fmt`: printf-style format string
- `...`: Arguments

**Example**:
```c
log_msg(RUFUS_LOG_INFO, "Device capacity: %.1f GB", capacity_gb);
```

#### Logging Macros
```c
#define log_debug(fmt, ...)   // DEBUG level
#define log_info(fmt, ...)    // INFO level
#define log_error(fmt, ...)   // ERROR level
```

---

## Device API

### Device Enumeration

#### `int device_init(void)`
**Purpose**: Initialize device enumeration (libudev context)

**Returns**: 0 on success, -1 on error

#### `int device_enumerate(rufus_device_t **out_devices, int *out_count)`
**Purpose**: Enumerate all USB devices

**Parameters**:
- `out_devices`: Pointer to device array (malloc'd by function)
- `out_count`: Number of devices found

**Returns**: 
- 0 on success
- -1 on error

**Memory Management**: Caller must free `out_devices` using `device_free_list()`.

**Example**:
```c
rufus_device_t *devices = NULL;
int count = 0;
if (device_enumerate(&devices, &count) < 0) {
    printf("Failed to enumerate devices\n");
    return -1;
}

for (int i = 0; i < count; i++) {
    printf("Device: %s (%s)\n", devices[i].vendor, devices[i].devnode);
}

device_free_list(devices);
```

#### `int device_get_info(const char *devnode, rufus_device_t *out_info)`
**Purpose**: Get detailed information about a device

**Parameters**:
- `devnode`: Device path (e.g., "/dev/sdb")
- `out_info`: Output device structure

**Returns**: 0 on success, -1 on error

#### `int device_validate(const char *devnode)`
**Purpose**: Validate device is safe to format

**Parameters**:
- `devnode`: Device path

**Returns**: 
- 0 if safe to format
- -1 if invalid (mounted, system disk, etc.)

**Checks**:
- Not currently mounted
- Not system disk (heuristic: must have partition suffix)
- Valid block device

#### `void device_free_list(rufus_device_t *devices)`
**Purpose**: Free device list from enumeration

**Parameters**: Pointer from `device_enumerate()`

#### `void device_cleanup(void)`
**Purpose**: Cleanup device enumeration resources

**Parameters**: None

---

## Partition API

### Partition Creation

#### `int partition_init(void)`
**Purpose**: Initialize partition module (libparted context)

**Returns**: 0 on success, -1 on error

#### `int partition_wipe_and_create(const char *device, uint64_t boot_size_bytes)`
**Purpose**: Wipe device and create MBR partition table

**Parameters**:
- `device`: Device path (e.g., "/dev/sdb")
- `boot_size_bytes`: Size of boot partition (100 MB recommended)

**Returns**: 0 on success, -1 on error

**Partition Layout**:
- Partition 1: `boot_size_bytes` FAT32 (boot)
- Partition 2: Remaining space NTFS (data)

**Example**:
```c
// Create 100 MB boot partition
int ret = partition_wipe_and_create("/dev/sdb", 100 * 1024 * 1024);
if (ret < 0) {
    printf("Partition creation failed\n");
    return -1;
}
```

#### `int partition_set_boot_flag(const char *device)`
**Purpose**: Set boot flag on first partition

**Parameters**:
- `device`: Device path

**Returns**: 0 on success, -1 on error

#### `void partition_cleanup(void)`
**Purpose**: Cleanup partition resources

**Parameters**: None

---

## Filesystem API

### Formatting

#### `int fs_format_fat32(const char *device, const char *label)`
**Purpose**: Format partition as FAT32

**Parameters**:
- `device`: Partition path (e.g., "/dev/sdb1")
- `label`: Volume label (max 11 chars, or NULL for "BOOT")

**Returns**: 
- 0 on success
- -1 on error

**Error Codes**:
- E-21-A: Format failed
- E-21-B: mkfs.vfat not found
- E-21-C: Format timed out

**Example**:
```c
int ret = fs_format_fat32("/dev/sdb1", "BOOT");
if (ret < 0) {
    printf("FAT32 formatting failed\n");
    return -1;
}
```

#### `int fs_format_ntfs(const char *device, const char *label)`
**Purpose**: Format partition as NTFS

**Parameters**:
- `device`: Partition path (e.g., "/dev/sdb2")
- `label`: Volume label (or NULL for "WINDOWS")

**Returns**: 0 on success, -1 on error

---

## Mount/Unmount API

### Mount Operations

#### `int mount_fat32(const char *partition, const char **out_mountpoint)`
**Purpose**: Mount FAT32 partition to temporary directory

**Parameters**:
- `partition`: Partition path (e.g., "/dev/sdb1")
- `out_mountpoint`: Pointer to mount path string (malloc'd)

**Returns**: 0 on success, -1 on error

**Memory Management**: Caller must free `out_mountpoint`.

**Example**:
```c
char *mountpoint = NULL;
if (mount_fat32("/dev/sdb1", &mountpoint) < 0) {
    printf("Mount failed\n");
    return -1;
}

// Use mountpoint...
printf("Mounted at: %s\n", mountpoint);

// Unmount when done
umount_partition(mountpoint);
free(mountpoint);
```

#### `int mount_ntfs(const char *partition, const char **out_mountpoint)`
**Purpose**: Mount NTFS partition to temporary directory

**Parameters**: Same as `mount_fat32()`

**Returns**: 0 on success, -1 on error

#### `int umount_partition(const char *mountpoint)`
**Purpose**: Unmount partition and cleanup temporary directory

**Parameters**:
- `mountpoint`: Mount path from `mount_*()` call

**Returns**: 0 on success, -1 on error

#### `int mount_sync(void)`
**Purpose**: Sync filesystem buffers to disk

**Parameters**: None

**Returns**: 0 on success, -1 on error

---

## ISO & Extraction API

### ISO Operations

#### `int iso_validate_windows(const char *iso_path, iso_info_t *out_info)`
**Purpose**: Validate Windows ISO file

**Parameters**:
- `iso_path`: Path to ISO file
- `out_info`: Output info structure (version, file count, total size)

**Returns**: 0 on success, -1 on error

**Example**:
```c
iso_info_t info;
if (iso_validate_windows("Windows10.iso", &info) < 0) {
    printf("Invalid Windows ISO\n");
    return -1;
}

printf("ISO: %s (%llu files, %.1f MB)\n", 
       info.version, info.file_count, 
       info.total_size / (1024.0 * 1024.0));
```

#### `int iso_extract_to_directory(const char *iso_path, const char *extract_to)`
**Purpose**: Extract ISO to directory

**Parameters**:
- `iso_path`: Path to ISO file
- `extract_to`: Directory to extract to

**Returns**: 0 on success, -1 on error

**Features**:
- Preserves file permissions
- Handles symlinks safely
- Creates directories as needed

#### `int iso_has_windows_install_wim(const char *iso_path)`
**Purpose**: Check if ISO contains Windows install.wim

**Parameters**:
- `iso_path`: Path to ISO file

**Returns**: 
- 1 if install.wim found
- 0 if not found
- -1 on error

---

## Bootloader API

### Boot Setup

#### `int bootloader_setup_bios(const char *device)`
**Purpose**: Install GRUB bootloader for BIOS boot

**Parameters**:
- `device`: Device path (e.g., "/dev/sdb")

**Returns**: 0 on success, -1 on error

**Requirements**:
- `grub-install` must be available
- MBR partition table must be created
- Boot flag must be set

#### `int bootloader_setup_uefi(const char *device, const char *fat32_mp)`
**Purpose**: Setup UEFI:NTFS bootloader

**Parameters**:
- `device`: Device path
- `fat32_mp`: Mount point of FAT32 partition

**Returns**: 0 on success, -1 on error

**Approach**: Installs UEFI bootloader to FAT32 boot partition for UEFI compatibility.

---

## Session API (State Machine)

### Session Lifecycle

#### `rufus_session_t *session_create(void)`
**Purpose**: Create a new session

**Returns**: 
- Session handle on success
- NULL on error

**Example**:
```c
rufus_session_t *session = session_create();
if (!session) {
    printf("Failed to create session\n");
    return -1;
}
```

#### `void session_destroy(rufus_session_t *session)`
**Purpose**: Destroy session and cleanup resources

**Parameters**: Session handle

**Returns**: None

### Session Operations

#### `int session_select_device(rufus_session_t *session, const char *device)`
**Purpose**: Select USB device for write

**Parameters**:
- `session`: Session handle
- `device`: Device path (e.g., "/dev/sdb")

**Returns**: 0 on success, -1 on error

#### `int session_set_iso(rufus_session_t *session, const char *iso_path)`
**Purpose**: Set ISO file to extract

**Parameters**:
- `session`: Session handle
- `iso_path`: Path to Windows ISO

**Returns**: 0 on success, -1 on error

#### `int session_execute(rufus_session_t *session)`
**Purpose**: Execute the write operation

**Parameters**: Session handle

**Returns**: 0 on success, -1 on error

**State Flow**:
1. IDLE → DEVICE_SELECTED (via `session_select_device()`)
2. DEVICE_SELECTED → PARTITIONED (via `session_execute()` - step 1)
3. PARTITIONED → FORMATTED (via `session_execute()` - step 2)
4. FORMATTED → COMPLETE (via `session_execute()` - step 3)

**Progress Events**:
- 0% - Initialization
- 20% - Partitioning
- 40% - Formatting
- 60% - Extracting ISO
- 80% - Setting bootloader
- 100% - Complete

**Error Recovery**: On failure, attempts cleanup (unmount, etc.)

---

## Error Code Reference

### Device & Privilege Errors (E-00, E-01)

| Code | Meaning |
|------|---------|
| E-00-A | No USB devices found |
| E-00-B | USB device not found |
| E-00-C | Device is mounted |
| E-00-D | Device is system disk |
| E-00-E | Invalid device path |
| E-01-A | Not running as root |
| E-01-B | Permission denied |

### ISO Validation Errors (E-10, E-11)

| Code | Meaning |
|------|---------|
| E-10-A | ISO file not found |
| E-10-B | Not a valid Windows ISO |
| E-10-C | ISO file is corrupted |
| E-10-D | ISO file too large |
| E-11-A | install.wim not found |
| E-11-B | ISO extraction failed |

### Partition & Mount Errors (E-20, E-21, E-22)

| Code | Meaning |
|------|---------|
| E-20-A | Partition creation failed |
| E-20-B | Device capacity too small |
| E-20-C | Partition table corruption |
| E-21-A | Filesystem format failed |
| E-21-B | Format tool not available |
| E-21-C | Format operation timeout |
| E-22-A | Mount operation failed |
| E-22-B | Mount point creation failed |

### File Copy/Extraction Errors (E-30)

| Code | Meaning |
|------|---------|
| E-30-A | File copy failed |
| E-30-B | Destination full |
| E-30-C | Permission denied on file |

### Bootloader Errors (E-40, E-41)

| Code | Meaning |
|------|---------|
| E-40-A | GRUB installation failed |
| E-40-B | grub-install not found |
| E-41-A | UEFI bootloader setup failed |

### System Errors (E-50, E-51)

| Code | Meaning |
|------|---------|
| E-50-A | Out of memory |
| E-50-B | System command failed |
| E-51-A | Filesystem sync failed |

### User Cancellation (E-60)

| Code | Meaning |
|------|---------|
| E-60-A | Operation cancelled by user |

---

## Typical Usage Flow

### Interactive CLI Usage
```c
#include <rufus.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // Initialize
    if (error_init() < 0) {
        fprintf(stderr, "Error init failed\n");
        return 1;
    }
    device_init();
    partition_init();
    
    // Enumerate devices
    rufus_device_t *devices = NULL;
    int count = 0;
    if (device_enumerate(&devices, &count) < 0) {
        printf("Failed to enumerate devices\n");
        return 1;
    }
    
    // Show devices
    for (int i = 0; i < count; i++) {
        printf("[%d] %s: %s %s (%.1f GB)\n", i,
               devices[i].devnode, devices[i].vendor, devices[i].model,
               devices[i].capacity_bytes / (1024.0 * 1024.0 * 1024.0));
    }
    
    // Select device and ISO
    const char *device = "/dev/sdb";
    const char *iso_path = "Windows10.iso";
    
    // Create session
    rufus_session_t *session = session_create();
    if (!session) {
        printf("Session creation failed\n");
        device_free_list(devices);
        return 1;
    }
    
    // Configure and execute
    session_select_device(session, device);
    session_set_iso(session, iso_path);
    
    // Register progress callback
    progress_set_callback(my_progress_handler, NULL);
    
    // Execute (blocking)
    int ret = session_execute(session);
    if (ret < 0) {
        printf("Write failed: %s\n", error_lookup(ret));
        session_destroy(session);
        device_free_list(devices);
        return 1;
    }
    
    // Cleanup
    session_destroy(session);
    device_free_list(devices);
    device_cleanup();
    partition_cleanup();
    error_cleanup();
    
    printf("Write complete!\n");
    return 0;
}

static void my_progress_handler(int percent, const char *msg, void *data) {
    printf("[%d%%] %s\n", percent, msg);
}
```

### Scripted CLI Usage
```c
// Command-line: winify --iso Windows10.iso --device /dev/sdb --dangerous

// Library handles:
// 1. Device validation
// 2. ISO validation
// 3. Partitioning
// 4. Formatting
// 5. ISO extraction
// 6. Bootloader setup
// All with progress callbacks and error handling
```

---

## Safety Guarantees

1. **Device Validation**: Refuses to format system disks or currently-mounted devices
2. **Error Handling**: All operations have clear error codes
3. **Memory Safety**: Proper allocation and deallocation
4. **Resource Cleanup**: Auto-cleanup on error (unmount, temp directory removal)
5. **Progress Reporting**: Real-time feedback for long operations
6. **Logging**: Full operation logging for debugging

---

## Thread Safety

**NOT thread-safe** in Phase 2. All operations are synchronous and must be called from a single thread.

For GUI applications: Thread-wrap the `session_execute()` call in your UI framework (e.g., GTK4's g_idle_add with blocking g_main_loop_run).

---

## Future Extensions (Phase 3+)

- Async API with callbacks
- Multiple ISO support (Ventoy-style)
- Persistent partition support
- Additional filesystems (exFAT, Ext4)
- GTK4 UI layer
- Localization support
- Configuration file support

---

**Questions?** Check `docs/phase2-progress/CONTEXT.md` for implementation details and design decisions.
