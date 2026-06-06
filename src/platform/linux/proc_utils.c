/*
 * proc_utils.c - Utilities for reading /proc filesystem
 *
 * Pure C implementation for detecting open devices and checking mount status
 * using /proc/<pid>/fd/ symlinks and /proc/mounts.
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <linux/limits.h>

/*
 * proc_running_as_root - Check if current process is running as root (UID 0)
 * Returns: 1 if effective UID is 0, 0 otherwise
 */
int proc_running_as_root(void)
{
    return geteuid() == 0;
}

/*
 * proc_device_open_by_pid - Scan /proc/<pid>/fd/ for a process with device open
 * Parameters:
 *   devnode - device node path (e.g., "/dev/sdb1")
 * Returns:
 *   PID of first process found with device open, 0 if not found, -1 on error
 */
int proc_device_open_by_pid(const char *devnode)
{
    if (!devnode)
        return -1;

    /* Resolve the target device to a real path for comparison */
    char real_dev[PATH_MAX];
    if (!realpath(devnode, real_dev)) {
        /* Device may not exist; treat as not open */
        return 0;
    }

    DIR *proc = opendir("/proc");
    if (!proc)
        return -1;

    struct dirent *ent;
    int result_pid = 0;

    while ((ent = readdir(proc)) != NULL) {
        /* Only numeric directory entries (PIDs) */
        char *end;
        long pid = strtol(ent->d_name, &end, 10);
        if (*end != '\0' || pid <= 0)
            continue;

        /* Build path to /proc/<pid>/fd */
        char fd_dir[64];
        snprintf(fd_dir, sizeof(fd_dir), "/proc/%ld/fd", pid);
        DIR *fdd = opendir(fd_dir);
        if (!fdd)
            continue;

        struct dirent *fd_ent;
        int found = 0;

        while ((fd_ent = readdir(fdd)) != NULL) {
            char fd_path[PATH_MAX];
            char link_target[PATH_MAX];

            snprintf(fd_path, sizeof(fd_path), "%s/%s", fd_dir, fd_ent->d_name);
            ssize_t len = readlink(fd_path, link_target, sizeof(link_target) - 1);
            if (len <= 0)
                continue;

            link_target[len] = '\0';

            if (strcmp(link_target, real_dev) == 0) {
                result_pid = (int)pid;
                found = 1;
                break;
            }
        }

        closedir(fdd);

        if (found)
            break;
    }

    closedir(proc);
    return result_pid;
}

/*
 * proc_device_is_mounted - Check if device appears in /proc/mounts
 * Parameters:
 *   devnode - device node path (e.g., "/dev/sdb1")
 * Returns:
 *   1 if device is mounted, 0 if not or on error (or NULL parameter)
 */
int proc_device_is_mounted(const char *devnode)
{
    if (!devnode)
        return 0;

    FILE *f = fopen("/proc/mounts", "r");
    if (!f)
        return 0;

    char line[512];
    int found = 0;

    while (fgets(line, sizeof(line), f)) {
        /* Extract device name (first field, space-delimited) */
        char *sp = strchr(line, ' ');
        if (!sp)
            continue;

        *sp = '\0';

        if (strcmp(line, devnode) == 0) {
            found = 1;
            break;
        }
    }

    fclose(f);
    return found;
}
