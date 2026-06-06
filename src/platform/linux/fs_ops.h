#ifndef WINAFI_FS_OPS_H
#define WINAFI_FS_OPS_H

#include <stdint.h>

// Format partition with FAT32
// device: /dev/sdX1
// label: volume label (can be NULL)
int fs_format_fat32(const char *device, const char *label);

// Format partition with NTFS
// device: /dev/sdX2
// label: volume label (can be NULL)
int fs_format_ntfs(const char *device, const char *label);

// Check if tool is available (mkfs.vfat, mkfs.ntfs)
int fs_check_tool_available(const char *tool_name);

#endif
