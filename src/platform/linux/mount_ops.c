#define _DEFAULT_SOURCE
#include "mount_ops.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>

int mount_create_temp_dirs(mount_context_t *ctx) {
    if (!ctx) return -1;

    // Create base temp directory
    char template[] = "/tmp/winafi-mount-XXXXXX";
    char *result = mkdtemp(template);
    if (!result) {
        return -1;  // E-50-B
    }

    strncpy(ctx->temp_dir, template, 255);
    ctx->temp_dir[255] = '\0';

    // Create subdirectories
    snprintf(ctx->fat_mount, 255, "%s/fat", ctx->temp_dir);
    snprintf(ctx->ntfs_mount, 255, "%s/ntfs", ctx->temp_dir);

    if (mkdir(ctx->fat_mount, 0700) != 0) {
        rmdir(ctx->temp_dir);
        return -1;  // E-50-B
    }
    if (mkdir(ctx->ntfs_mount, 0700) != 0) {
        rmdir(ctx->fat_mount);
        rmdir(ctx->temp_dir);
        return -1;  // E-50-B
    }

    return 0;
}

int mount_fat32(const char *device, const char *mount_point) {
    if (!device || !mount_point) return -1;

    // Use system mount command with appropriate FAT32 options
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "mount -t vfat -o uid=0,gid=0,umask=0,fmask=0111,flush %s %s 2>/dev/null",
        device, mount_point);

    if (system(cmd) != 0) {
        return -1;  // E-22-A
    }

    return 0;
}

int mount_ntfs(const char *device, const char *mount_point) {
    if (!device || !mount_point) return -1;

    // Try ntfs3 (modern kernel) first via system mount
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "mount -t ntfs3 -o uid=0,gid=0,umask=0,fmask=0111,flush %s %s 2>/dev/null",
        device, mount_point);

    if (system(cmd) == 0) {
        return 0;
    }

    // Fall back to ntfs-3g with alternative options
    snprintf(cmd, sizeof(cmd),
        "mount -t ntfs -o uid=0,gid=0,umask=0 %s %s 2>/dev/null",
        device, mount_point);

    if (system(cmd) != 0) {
        return -1;  // E-22-A
    }

    return 0;
}

int unmount_and_cleanup(mount_context_t *ctx) {
    if (!ctx) return -1;

    int ret = 0;

    // Unmount NTFS first
    if (strlen(ctx->ntfs_mount) > 0) {
        if (umount(ctx->ntfs_mount) != 0) {
            ret = -1;  // E-22-B (but continue cleanup)
        }
        rmdir(ctx->ntfs_mount);
    }

    // Unmount FAT32
    if (strlen(ctx->fat_mount) > 0) {
        if (umount(ctx->fat_mount) != 0) {
            ret = -1;
        }
        rmdir(ctx->fat_mount);
    }

    // Remove temp directory
    if (strlen(ctx->temp_dir) > 0) {
        if (rmdir(ctx->temp_dir) != 0) {
            ret = -1;  // E-50-F (non-fatal)
        }
    }

    return ret;
}

int mount_sync(void) {
    sync();
    return 0;
}
