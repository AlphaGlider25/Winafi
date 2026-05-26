#include "bootloader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int bootloader_setup_grub_bios(const char *device) {
    if (!device) return -1;

    // Call grub-install to set up MBR bootloader
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "grub-install --target=i386-pc --no-floppy %s 2>/dev/null", device);

    int ret = system(cmd);
    if (ret != 0) {
        return -1;  // E-40-A
    }

    return 0;
}

int bootloader_setup_uefi_ntfs(const char *fat_mount) {
    if (!fat_mount) return -1;

    // Create EFI/Boot directory
    char efi_dir[512];
    snprintf(efi_dir, sizeof(efi_dir), "%s/EFI/Boot", fat_mount);

    // Create directories
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", efi_dir);
    system(cmd);

    // Copy bundled bootx64.efi
    snprintf(cmd, sizeof(cmd), "cp share/winify/assets/bootx64.efi %s/ 2>/dev/null", efi_dir);
    int ret = system(cmd);
    if (ret != 0) {
        // Try alternative path (source tree)
        snprintf(cmd, sizeof(cmd), "cp src/assets/uefi-ntfs/bootx64.efi %s/ 2>/dev/null", efi_dir);
        ret = system(cmd);
    }

    if (ret != 0) {
        return -1;  // E-41-A
    }

    return 0;
}

int bootloader_generate_grub_config(const char *fat_mount) {
    if (!fat_mount) return -1;

    char grub_cfg[512];
    snprintf(grub_cfg, sizeof(grub_cfg), "%s/grub/grub.cfg", fat_mount);

    FILE *f = fopen(grub_cfg, "w");
    if (!f) {
        return -1;  // E-40-B
    }

    fprintf(f, "menuentry 'Windows' --class windows {\n");
    fprintf(f, "    insmod ntfs\n");
    fprintf(f, "    insmod chain\n");
    fprintf(f, "    set root='(hd0,msdos2)'\n");
    fprintf(f, "    chainloader +1\n");
    fprintf(f, "}\n");

    fclose(f);
    return 0;
}
