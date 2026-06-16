#define _GNU_SOURCE
#include "bootloader.h"
#include "assets.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>

extern char **environ;

int bootloader_setup_grub_bios(const char *device) {
    if (!device) return -1;

    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        char *argv[] = {
            (char *)"grub-install",
            (char *)"--target=i386-pc",
            (char *)"--no-floppy",
            (char *)device,
            NULL
        };
        execve("/usr/bin/grub-install", argv, environ);
        execve("/bin/grub-install", argv, environ);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0 ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return -1;  // E-40-A
    }

    return 0;
}

static int mkdir_p(const char *path) {
    char tmp[PATH_MAX]; snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            *p = '/'; }
    }
    return (mkdir(tmp, 0755) != 0 && errno != EEXIST) ? -1 : 0;
}

/* Local byte-exact copy (mirrors windows_boot.c copy_file). */
static int boot_copy_file(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) return -1;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { close(in); return -1; }
    char b[65536]; ssize_t r; int rc = 0;
    while ((r = read(in, b, sizeof(b))) > 0)
        if (write(out, b, (size_t)r) != r) { rc = -1; break; }
    if (r < 0) rc = -1;
    close(in); close(out);
    if (rc) unlink(dst);
    return rc;
}

int bootloader_setup_uefi_ntfs(const char *fat_mount) {
    if (!fat_mount) return -1;

    char efi_dir[PATH_MAX];
    if (snprintf(efi_dir, sizeof(efi_dir), "%s/EFI/BOOT", fat_mount) < 0 ||
        strlen(fat_mount) + strlen("/EFI/BOOT") >= sizeof(efi_dir)) {
        return -1;
    }
    if (mkdir_p(efi_dir) != 0) return -1;

    char asset[PATH_MAX];
    if (assets_find("uefi-ntfs/bootx64.efi", asset, sizeof(asset)) != 0) {
        fprintf(stderr, "ERROR: signed UEFI:NTFS loader asset not found\n");
        return -1;                                  // E-41-A
    }

    char dst[PATH_MAX];
    if (snprintf(dst, sizeof(dst), "%s/BOOTX64.EFI", efi_dir) < 0 ||
        strlen(efi_dir) + strlen("/BOOTX64.EFI") >= sizeof(dst)) {
        return -1;
    }
    if (boot_copy_file(asset, dst) != 0) {
        fprintf(stderr, "ERROR: failed to install UEFI:NTFS loader to ESP\n");
        return -1;                                  // E-41-A
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
