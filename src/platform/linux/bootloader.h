#ifndef RUFUS_BOOTLOADER_H
#define RUFUS_BOOTLOADER_H

typedef enum {
    BOOT_MODE_BIOS = 0,
    BOOT_MODE_UEFI = 1,
    BOOT_MODE_BOTH = 2
} boot_mode_t;

// Setup GRUB bootloader for BIOS boot
// device: /dev/sdX (full device, not partition)
// Returns: 0 on success, negative on error
int bootloader_setup_grub_bios(const char *device);

// Setup UEFI:NTFS bootloader for UEFI boot
// fat_mount: mounted FAT32 partition (/tmp/rufus-mount-XXX/fat)
// Returns: 0 on success, negative on error
int bootloader_setup_uefi_ntfs(const char *fat_mount);

// Generate GRUB configuration
// fat_mount: mounted FAT32 partition
// Returns: 0 on success, negative on error
int bootloader_generate_grub_config(const char *fat_mount);

#endif
