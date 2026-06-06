# Rufus Linux Port Strategy

## Overview

This document outlines a strategy for porting Rufus (Windows USB bootable drive creation utility) to Linux while maintaining code quality, avoiding platform-specific hacks, and following Linux community best practices.

**Current Status**: Planning and architecture phase. No functional implementation yet.

## Project Goals

1. **Create a Linux-native USB bootable drive creation tool** inspired by Rufus
2. **Maintain feature parity** where applicable (ISO handling, file system support, verification)
3. **Follow Linux best practices** and architectural patterns
4. **Use modern tooling** (CMake, GTK4/Qt6, libudev)
5. **Avoid slop code** by researching patterns before implementation

## Fundamental Challenges When Porting Windows to Linux

### 1. **UI Framework Migration**
- **Current**: Rufus uses Win32 API (legacy Windows GUI toolkit)
- **Linux alternatives**:
  - **GTK4** (GNOME-based, what Popsicle uses) - lighter, simpler
  - **Qt6** (heavier, more powerful, cross-platform potential)
  - **wxWidgets** (actual cross-platform wrapper - requires different approach)
  
**Decision**: GTK4 recommended for native Linux feel and lighter footprint

### 2. **System API Abstraction**
- **Current**: Windows IOCTL, WinAPI registry, Win32 device enumeration
- **Linux equivalents**:
  - Device enumeration: `libudev`, `/sys/block/`, `/dev/`
  - Disk I/O: `ioctl()` calls (different semantics than Windows)
  - Storage info: `libblkid`, `/proc/partitions`, `/sys/block/sdX/`
  - Privilege elevation: PolicyKit, sudo, or capabilities

**Key files needing refactoring**:
- `drive.c` (device enumeration) → Linux ioctl + udev
- `dev.c` (device detection) → libudev + sysfs
- `format.c` (formatting operations) → mkfs, libparted, or libblkid
- `darkmode.c` (UI theming) → GTK CSS
- All Win32 registry/config code → XDG Base Directory spec

### 3. **File System Support**
- **Current**: FAT32, NTFS, exFAT, ReFS (Windows-specific), UDF, ext2/3
- **Linux approach**:
  - Use `libparted` for partition management
  - Use `libblkid` for file system detection
  - Delegate formatting to system tools: `mkfs.vfat`, `mkfs.ntfs`, `mkfs.ext4`
  - NTFS support via `ntfs-3g` or `ntfs-3g` tools

**Decision**: Delegate to system tools initially, use libparted for partition operations

### 4. **ISO Handling & Boot Creation**
- **Current**: libcdio for ISO 9660, custom boot sector handling
- **Linux approach**:
  - Keep libcdio (it's cross-platform)
  - Use `isohybrid` or `xorriso` for hybrid ISO detection
  - Use `grub-install` / `syslinux` or `grub2` for bootloader setup
  - Study how existing tools (WoeUSB-ng, Ventoy) handle this

**Note**: This is complex - existing tools like WoeUSB-ng, Ventoy, and BalenaEtcher handle this well; consider what unique value Winify would add.

### 5. **Privilege Escalation & Permissions**
- **Current**: Windows UAC prompts
- **Linux approach**:
  - Use PolicyKit (org.freedesktop.PolicyKit1) for privilege elevation
  - Alternative: Require `sudo` or capability-based approach
  - Store PolicyKit rules in `/usr/share/polkit-1/actions/`

**Decision**: PolicyKit preferred for user-friendly elevation

## Recommended Architecture

```
src/
  platform/
    win32/              [DEPRECATED - reference only]
      ui.c
      dev_windows.c
      drive_windows.c
      ...
    linux/              [NEW - Linux-specific code]
      ui_gtk.c          [GTK4 UI]
      dev_udev.c        [Device enumeration via libudev]
      drive_linux.c     [Linux disk I/O operations]
      fs_ops.c          [Filesystem operations]
      privilege.c       [PolicyKit integration]
  core/                 [Platform-agnostic core]
    iso.c               [ISO parsing - keep libcdio]
    format.c            [Format logic - refactored to use libparted]
    hash.c              [Checksumming]
    badblocks.c         [Bad block detection]
    localization.c      [i18n]
  
cmake/
  FindLibUDev.cmake
  FindGTK4.cmake
  FindLibParted.cmake
  FindNTFS.cmake
  
CMakeLists.txt          [Modern build system instead of autotools]
docs/
  ARCHITECTURE.md
  BUILDING.md
  PORTING_NOTES.md
  CONTRIBUTORS.md
```

## Phase-Based Implementation Plan

### Phase 1: Foundation (No Commits Yet)
- [ ] Analyze `drive.c` and `dev.c` for Windows API dependencies
- [ ] Create platform abstraction headers: `platform.h`, `device.h`, `filesystem.h`
- [ ] Set up CMake build system for Linux
- [ ] Add libudev dependency and device enumeration layer
- [ ] Research PolicyKit integration patterns
- [ ] Document findings in `docs/PHASE1_FINDINGS.md`

### Phase 2: Core System Integration (First commits)
- [ ] Implement `platform/linux/dev_udev.c` (device enumeration)
- [ ] Implement `platform/linux/drive_linux.c` (disk operations)
- [ ] Create abstraction layer that existing code calls
- [ ] Build & test on Linux (no GUI yet)
- [ ] Commit: "feat: add Linux device enumeration via libudev"

### Phase 3: UI Migration
- [ ] Analyze `ui.c` and Win32 UI code
- [ ] Design GTK4 UI layout (match Rufus functionality)
- [ ] Implement `platform/linux/ui_gtk.c`
- [ ] Integrate with core device/format logic
- [ ] Commit: "feat: add GTK4 user interface"

### Phase 4: File System Operations
- [ ] Refactor `format.c` to use libparted
- [ ] Implement `platform/linux/fs_ops.c`
- [ ] Support ext2/3/4, FAT32, NTFS, exFAT where possible
- [ ] Commit: "feat: Linux filesystem operations with libparted"

### Phase 5: ISO & Boot Handling
- [ ] Analyze current boot creation logic
- [ ] Implement isohybrid detection
- [ ] Test with various ISO types (Windows, Linux, etc.)
- [ ] Commit: "feat: ISO hybrid detection and bootloader setup"

### Phase 6: Polish & Testing
- [ ] Dark mode support (GTK CSS)
- [ ] Localization/i18n support
- [ ] Error handling and user feedback
- [ ] Manual testing with real USB drives
- [ ] Multiple distro testing (Ubuntu, Fedora, Arch, etc.)

## Key Dependencies

### Build Tools
- CMake 3.20+
- GCC/Clang with C17 support
- pkg-config

### Required Libraries
- GTK4 (UI) - `libgtk-4-dev` on Debian/Ubuntu
- libudev (device enumeration) - `libudev-dev`
- libparted (partition management) - `libparted-dev`
- libblkid (filesystem detection) - `libblkid-dev`
- libcdio (ISO parsing) - `libcdio-dev` (keep from original)

### Optional Libraries
- ntfs-3g (NTFS support) - `ntfs-3g`
- libexfat (exFAT support) - `libexfat-dev`
- e2fsprogs (ext2/3/4 tools) - standard in most distros

### System Tools
- polkit (privilege elevation)
- syslinux / grub2 (bootloader)
- xorriso (ISO operations)

## Research Tasks (Before Implementation)

1. **Deep-dive into WoeUSB-ng source code** - study NTFS bootable creation
   - Reference: https://github.com/slacka/WoeUSB-ng
   
2. **Study libudev best practices** - device enumeration patterns
   - How to watch for device hotplug
   - Proper error handling

3. **Analyze Popsicle architecture** - Rust + GTK reference
   - How they structure parallel flashing
   - Error handling patterns

4. **Research PolicyKit integration** - proper privilege escalation
   - How GNOME Disks does it
   - Packaging .pkla files

5. **ISO hybrid detection algorithm** - how existing tools detect isohybrid
   - What makes an ISO "hybrid"
   - How bootloaders are embedded

## Testing Strategy

### Unit Testing
- Device enumeration logic
- File system detection
- Hash computation
- Partition table handling

### Integration Testing
- Real USB device detection (in VM)
- Actual USB formatting (destructive - use test drives)
- ISO to USB creation end-to-end
- Boot verification in QEMU/VirtualBox

### Compatibility Testing
- Multiple Linux distros (Fedora, Debian, Arch, openSUSE)
- Various GTK themes and dark mode
- Different USB controller chips
- UEFI and Legacy BIOS modes

## Commit Philosophy

**Important**: Do not commit until:
1. Functionality is tested (even if just unit tests)
2. Code follows the project style (see below)
3. No "work in progress" code
4. Descriptive commit messages following the original Rufus format

**Commit format** (matching original Rufus):
```
[component] short description

Optional longer explanation if needed.

- List specific changes if helpful
- Reference architectur decisions
```

Example:
```
[linux] add device enumeration via libudev

Replaces Windows device enumeration (dev.c) with Linux-native
libudev-based approach. Implements hot-plug detection.

- Add platform/linux/dev_udev.c
- Add FindLibUDev.cmake
- Update platform abstraction headers
```

## No Slop Code Rules

1. **Always search before coding** - research the proper Linux way to do things
2. **Use established libraries** - don't reimplement libparted or libudev functionality
3. **Follow POSIX where possible** - Linux standard APIs
4. **Platform abstraction** - never hardcode `/dev/sda` patterns; use libudev
5. **Error handling** - proper errno checking and user-facing messages
6. **Documentation** - code comments for non-obvious logic
7. **Testing** - unit tests for critical paths

## References & Existing Tools

- **WoeUSB-ng**: https://github.com/slacka/WoeUSB-ng - Create Windows bootable USBs on Linux
- **Ventoy**: https://www.ventoy.net/ - Universal bootable USB solution
- **Popsicle**: https://github.com/pop-os/popsicle - PopOS USB flashing tool (Rust + GTK)
- **GNOME Disks**: Partition management tool (C + GTK)
- **BalenaEtcher**: https://www.balena.io/etcher/ - Multi-platform flashing tool

## Status Log

- **2026-05-25**: Initial planning and research phase
  - Folder structure created
  - Key challenges identified
  - Dependencies catalogued
  - Architecture proposed

---

**Next Step**: Phase 1 research tasks to be completed before any code changes.
