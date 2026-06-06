/*
 * process.c - /proc-based process and device-in-use detection for Linux
 *
 * Uses /proc/<pid>/fd/ symlinks and /proc/<pid>/comm to detect which
 * processes have a device node open, without any external libraries.
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>

#include "process.h"

/*
 * process_is_root - check if current process runs as root
 */
int process_is_root(void)
{
    return getuid() == 0;
}

/*
 * is_numeric - return 1 if str consists entirely of decimal digits
 */
static int is_numeric(const char *str)
{
    if (!str || !*str)
        return 0;
    for (; *str; str++) {
        if (*str < '0' || *str > '9')
            return 0;
    }
    return 1;
}

/*
 * pid_has_device_open - return 1 if process <pid> has resolved_dev open
 *                       via any fd symlink in /proc/<pid>/fd/
 *                       return 0 if not found, -1 on permission error
 */
static int pid_has_device_open(const char *pid_str, const char *resolved_dev)
{
    char fd_dir[64];
    snprintf(fd_dir, sizeof(fd_dir), "/proc/%s/fd", pid_str);

    DIR *fd_dp = opendir(fd_dir);
    if (!fd_dp) {
        /* EACCES or ENOENT — process gone or no permission, skip silently */
        return 0;
    }

    struct dirent *fde;
    int found = 0;
    while ((fde = readdir(fd_dp)) != NULL) {
        if (fde->d_name[0] == '.')
            continue;

        char fd_path[128];
        snprintf(fd_path, sizeof(fd_path), "%s/%s", fd_dir, fde->d_name);

        char link_target[PATH_MAX];
        ssize_t len = readlink(fd_path, link_target, sizeof(link_target) - 1);
        if (len < 0)
            continue;
        link_target[len] = '\0';

        if (strcmp(link_target, resolved_dev) == 0) {
            found = 1;
            break;
        }
    }

    closedir(fd_dp);
    return found;
}

/*
 * read_comm - read process name from /proc/<pid>/comm into buf (max 256 bytes)
 *             returns 1 on success, 0 on failure
 */
static int read_comm(const char *pid_str, char *buf, size_t bufsz)
{
    char comm_path[64];
    snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", pid_str);

    FILE *f = fopen(comm_path, "r");
    if (!f)
        return 0;

    if (!fgets(buf, (int)bufsz, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);

    /* Strip trailing newline */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';

    return 1;
}

/*
 * process_device_in_use - return 1 if devnode is open by any process,
 *                          0 if not, -1 on error
 */
int process_device_in_use(const char *devnode)
{
    if (!devnode)
        return -1;

    /* Resolve to canonical path; if it doesn't exist realpath returns NULL */
    char resolved[PATH_MAX];
    if (!realpath(devnode, resolved)) {
        /* Device doesn't exist — definitely not in use */
        return 0;
    }

    DIR *proc_dp = opendir("/proc");
    if (!proc_dp)
        return -1;

    struct dirent *de;
    int found = 0;
    while (!found && (de = readdir(proc_dp)) != NULL) {
        if (!is_numeric(de->d_name))
            continue;

        if (pid_has_device_open(de->d_name, resolved))
            found = 1;
    }

    closedir(proc_dp);
    return found;
}

/*
 * process_list_users - fill out[] with names of processes that have devnode open
 *                      returns count written (0 if none), -1 on error
 */
int process_list_users(const char *devnode, char out[][256], int max_out)
{
    if (!devnode || !out || max_out <= 0)
        return -1;

    char resolved[PATH_MAX];
    if (!realpath(devnode, resolved)) {
        /* Device doesn't exist */
        return 0;
    }

    DIR *proc_dp = opendir("/proc");
    if (!proc_dp)
        return -1;

    struct dirent *de;
    int count = 0;

    while ((de = readdir(proc_dp)) != NULL) {
        if (!is_numeric(de->d_name))
            continue;

        if (!pid_has_device_open(de->d_name, resolved))
            continue;

        /* Process has device open — get its name */
        char comm[256] = {0};
        if (!read_comm(de->d_name, comm, sizeof(comm)))
            snprintf(comm, sizeof(comm), "pid:%s", de->d_name);

        /* Avoid duplicate names */
        int dup = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(out[i], comm) == 0) {
                dup = 1;
                break;
            }
        }
        if (!dup) {
            if (count >= max_out)
                break;
            strncpy(out[count], comm, 255);
            out[count][255] = '\0';
            count++;
        }
    }

    closedir(proc_dp);
    return count;
}
