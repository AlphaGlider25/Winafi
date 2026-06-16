#define _DEFAULT_SOURCE
#include "mount_ops.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>

extern char **environ;

static int run_mount(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execve("/usr/bin/mount", argv, environ);
        execve("/bin/mount", argv, environ);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return -1;
    return 0;
}

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
    if (snprintf(ctx->fat_mount, sizeof(ctx->fat_mount), "%s/fat", ctx->temp_dir) < 0 ||
        strlen(ctx->temp_dir) + strlen("/fat") >= sizeof(ctx->fat_mount)) {
        rmdir(ctx->temp_dir);
        return -1;
    }
    if (snprintf(ctx->ntfs_mount, sizeof(ctx->ntfs_mount), "%s/ntfs", ctx->temp_dir) < 0 ||
        strlen(ctx->temp_dir) + strlen("/ntfs") >= sizeof(ctx->ntfs_mount)) {
        rmdir(ctx->temp_dir);
        return -1;
    }

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

    char *argv[] = {
        (char *)"mount", (char *)"-t", (char *)"vfat", (char *)"-o",
        (char *)"uid=0,gid=0,umask=0,fmask=0111,flush",
        (char *)device, (char *)mount_point, NULL
    };
    if (run_mount(argv) != 0) {
        return -1;  // E-22-A
    }

    return 0;
}

int mount_ntfs(const char *device, const char *mount_point) {
    if (!device || !mount_point) return -1;

    char *ntfs3_argv[] = {
        (char *)"mount", (char *)"-t", (char *)"ntfs3", (char *)"-o",
        (char *)"uid=0,gid=0,umask=0,fmask=0111,flush",
        (char *)device, (char *)mount_point, NULL
    };
    if (run_mount(ntfs3_argv) == 0) {
        return 0;
    }

    char *ntfs_argv[] = {
        (char *)"mount", (char *)"-t", (char *)"ntfs", (char *)"-o",
        (char *)"uid=0,gid=0,umask=0",
        (char *)device, (char *)mount_point, NULL
    };
    if (run_mount(ntfs_argv) != 0) {
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
