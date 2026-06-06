# Platform Dependency Analysis

## Overview

This document maps out which Rufus source files have Windows-specific dependencies and will need refactoring or platform abstraction for a Linux port.

## Color Legend

- 🔴 **Critical Windows Dependency** - Requires complete rewrite
- 🟡 **Moderate Windows Dependency** - Needs abstraction layer
- 🟢 **Platform Agnostic** - Can reuse or minor changes only
- ⚪ **N/A** - Not applicable to Linux port

---

## Core Files Analysis

### UI Layer

#### `ui.c` (WIN32 UI)
- **Status**: 🔴 **Critical rewrite needed**
- **Windows Dependencies**: 
  - Dialog/Window creation (Win32 API)
  - Message loops (WM_* messages)
  - Control drawing (HWND, HDC)
  - Dark mode (UxTheme)
- **Linux Approach**: Rewrite using GTK4
- **Effort**: HIGH
- **Estimated Lines**: ~2000+ new lines

#### `darkmode.c`
- **Status**: 🔴 **Complete rewrite**
- **Windows Dependencies**: 
  - UxTheme API
  - Windows registry dark mode detection
  - WM_SETTINGCHANGE messages
- **Linux Approach**: GTK CSS + XDG settings
- **Effort**: MEDIUM
- **Reusable Logic**: Color scheme determination (might be portable)

#### `icon.c`
- **Status**: 🟡 **Partial rewrite**
- **Windows Dependencies**: 
  - HICON resource handling
  - Window icon setting
- **Linux Approach**: GdkPixbuf + GTK icon theme
- **Effort**: LOW
- **Reusable Logic**: Icon resource loading structure

---

### Device Management Layer

#### `dev.c` (Device detection)
- **Status**: 🔴 **Critical rewrite needed**
- **Windows Dependencies**:
  - SetupAPI for device enumeration
  - WMI for device information
  - Registry queries
  - Windows driver model
- **Linux Approach**: libudev + sysfs + /proc
- **Effort**: HIGH
- **Files to Create**: 
  - `platform/linux/dev_udev.c`
  - `platform/linux/device.h` (abstraction)

#### `dev.h` (Device structures)
- **Status**: 🟡 **Partial reuse**
- **Reusable**: Structure definitions for device properties
- **Needs Refactoring**: Windows-specific fields (driver info, bus enumerator, etc.)

---

### Disk Operations Layer

#### `drive.c` (101KB - Largest file)
- **Status**: 🔴 **Critical - Needs major refactoring**
- **Windows Dependencies**:
  - DeviceIoControl() calls (IOCTL codes)
  - WriteFile() / ReadFile()
  - CreateFile() with Windows-specific flags
  - Registry access for device properties
  - Windows error codes
- **Linux Approach**: 
  - Use POSIX open/write/ioctl
  - `ioctl()` with Linux-specific codes
  - /sys/block/ and /dev/ access
- **Effort**: VERY HIGH (largest refactor)
- **Estimated Platform-Specific Code**: ~60% of file

**Critical functions needing refactoring**:
- `GetDrivePartitionSize()` - IOCTL_DISK_GET_DRIVE_GEOMETRY
- `WriteDrive()` - WriteFile() calls
- `GetDriveNumberFromPath()` - Windows path parsing
- All IOCTL_* calls

#### `drive.h`
- **Status**: 🟡 **Partial reuse**
- **Reusable**: Structure definitions
- **Needs Change**: IOCTL codes, Windows-specific macros

---

### File System Layer

#### `format.c` (73KB)
- **Status**: 🟡 **Moderate refactoring**
- **Windows Dependencies**:
  - FormatEx() API calls
  - Windows file system formatters
  - NTFS journal options (Windows-specific)
- **Linux Approach**: 
  - Use libparted for partition operations
  - Delegate to system mkfs.* tools
  - Or use libblkid for detection
- **Effort**: HIGH
- **Reusable Logic**: Format option parsing, progress tracking

#### `format_ext.c` (17KB)
- **Status**: 🟢 **Mostly portable**
- **Dependencies**: Ext2/3 header definitions (standard)
- **Changes Needed**: Minor - mostly struct definitions
- **Effort**: LOW

#### `format_fat32.c` (17KB)
- **Status**: 🟡 **Partial reuse**
- **Windows-specific**: Some Windows-centric comments/logic
- **Reusable**: FAT32 boot sector creation
- **Effort**: LOW-MEDIUM

---

### ISO Handling

#### `iso.c` (90KB)
- **Status**: 🟢 **Mostly platform-agnostic**
- **Dependencies**: libcdio (cross-platform library)
- **Windows-specific Parts**: File path handling (backslashes)
- **Reusable**: ~95% of code
- **Effort**: LOW
- **Changes**: 
  - File path handling (use POSIX paths)
  - Minor path normalization

#### `syslinux.c`
- **Status**: 🟡 **Partial refactor**
- **Reusable**: Syslinux boot logic (standard)
- **Needs Change**: Windows file I/O calls

---

### Hashing & Verification

#### `hash.c` (87KB)
- **Status**: 🟢 **Platform agnostic**
- **Dependencies**: Standard C library + Windows Crypto API
- **Reusable**: ~90% of hashing logic
- **Changes**: 
  - Replace WinCrypt with OpenSSL or libgcrypt
  - File I/O abstraction

#### `badblocks.c` (15KB)
- **Status**: 🟡 **Mostly portable**
- **Windows-specific**: Some I/O patterns
- **Reusable**: Core bad block detection logic
- **Effort**: LOW-MEDIUM

---

### Utilities & Infrastructure

#### `localization.c` / `localization.h` (19KB)
- **Status**: 🟢 **Portable with changes**
- **Windows-specific**: Registry-based locale storage
- **Linux Approach**: XDG Base Directory spec + gettext
- **Reusable**: String translation logic
- **Effort**: MEDIUM

#### `net.c` (Network operations)
- **Status**: 🟡 **Partial refactor**
- **Windows-specific**: WinINet / WinHTTP
- **Linux Approach**: libcurl or wget
- **Reusable**: Download logic structure

#### `registry.h`
- **Status**: 🔴 **Not applicable to Linux**
- **Action**: Remove or abstract
- **Replacement**: XDG config files / dconf

#### `xml.c`
- **Status**: 🟢 **Platform agnostic**
- **Reusable**: ~100%

#### `ms-sys` (BSD-licensed)
- **Status**: 🟢 **Platform agnostic**
- **Reusable**: ~100% (boot sector code)

#### Regex files (`cregex_*.c`)
- **Status**: 🟢 **Platform agnostic**
- **Reusable**: ~100%

---

### External Libraries (in subdirectories)

#### `libcdio/`
- **Status**: 🟢 **Cross-platform**
- **Action**: Keep as-is or use system libcdio
- **Effort**: NONE (reuse)

#### `ext2fs/`
- **Status**: 🟢 **Cross-platform**
- **Action**: Keep or use system libe2fs
- **Effort**: MINIMAL

#### `bled/` (Bootloader Embedded)
- **Status**: 🟡 **Portable**
- **Reusable**: Boot code (mostly assembly/binary blobs)
- **Changes**: Path references, build integration

#### `getopt/`
- **Status**: 🟢 **Standard POSIX**
- **Action**: Use system getopt or keep
- **Effort**: NONE

---

## Summary Statistics

| Category | Files | Status | Effort |
|----------|-------|--------|--------|
| **Critical Rewrite** | ui.c, dev.c, drive.c, format.c, darkmode.c | 🔴 | VERY HIGH |
| **Partial Refactor** | drive.h, dev.h, format_fat32.c, iso.c, hash.c, net.c | 🟡 | MEDIUM |
| **Mostly Portable** | format_ext.c, badblocks.c, localization.c, syslinux.c | 🟡 | MEDIUM |
| **Platform Agnostic** | xml.c, regex files, ms-sys, libcdio, ext2fs | 🟢 | LOW |
| **Not Applicable** | registry.h, darkmode-specific code | ⚪ | N/A |

---

## Architecture Abstraction Layers Needed

### 1. Platform Abstraction Layer (`platform/platform.h`)

```c
// Device abstraction
typedef struct device_info { ... } device_info_t;
int platform_enumerate_devices(device_info_t **devices, int *count);

// Disk I/O abstraction  
int platform_open_disk(const char *path);
int platform_write_disk(int fd, const char *data, size_t len);
int platform_ioctl(int fd, int cmd, void *arg);

// File system abstraction
int platform_format_disk(const char *device, const char *fs_type, const char *label);

// Registry/Config abstraction
int platform_get_config(const char *key, char *value, size_t max_len);

// Privilege elevation
int platform_elevate_privileges();
```

### 2. File I/O Abstraction Layer

Unify `WriteFile()`, `ReadFile()` patterns into platform-independent wrappers.

### 3. IOCTL Command Abstraction

Create mapping layer for IOCTL codes:
```c
#ifdef _WIN32
  #define IOCTL_GET_DISK_SIZE IOCTL_DISK_GET_DRIVE_GEOMETRY
#else
  #define IOCTL_GET_DISK_SIZE BLKGETSIZE64
#endif
```

---

## Implementation Priority

1. **Phase 1 (Foundation)**
   - Abstraction headers
   - Device enumeration (platform/linux/dev_udev.c)
   - Basic disk I/O (platform/linux/drive_linux.c)

2. **Phase 2 (Core)**
   - Format operations
   - ISO handling (mostly reuse)
   - Hash/verification

3. **Phase 3 (UI & Polish)**
   - GTK4 UI (platform/linux/ui_gtk.c)
   - Localization
   - Dark mode

4. **Phase 4 (Testing & Hardening)**
   - Error handling
   - Device hotplug
   - Real-world testing

---

## Known Challenges

1. **NTFS Support on Linux**: More complex than on Windows
   - Consider using ntfs-3g or NTFS driver
   - May need fallback strategies

2. **UEFI Bootloader Setup**: Different on Linux
   - Research grub-install, efibootmgr
   - Study WoeUSB-ng implementation

3. **Device Hotplug**: Windows has built-in notification, Linux needs udev monitoring
   - Requires event loop integration with GTK

4. **Privilege Escalation**: No direct equivalent to Windows UAC
   - PolicyKit is the standard approach
   - May require packaging/distribution setup

5. **Windows ISO Creation**: Paradoxically harder on Linux than BIOS
   - WoeUSB-ng solves this - study their approach
   - May involve custom NTFS bootloader setup

---

## Recommended Reading / References

1. **WoeUSB-ng source code** - Best reference for Windows ISO on Linux
2. **libudev documentation** - Device enumeration
3. **Linux kernel ioctl guide** - Understanding disk operations
4. **libparted API docs** - Partition management
5. **GTK4 documentation** - Modern Linux UI toolkit

