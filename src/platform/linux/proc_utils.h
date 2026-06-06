/*
 * proc_utils.h - Utilities for reading /proc filesystem
 *
 * Pure C utilities for detecting open devices and checking permissions
 * using /proc/<pid>/fd/ symlinks and /proc/mounts.
 */

#ifndef WINAFI_PROC_UTILS_H
#define WINAFI_PROC_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 if the current process is running as root (effective UID 0), 0 otherwise. */
int proc_running_as_root(void);

/* Scan /proc/<pid>/fd/ for any process that has devnode open.
 * Returns the PID of the first found process, or 0 if none.
 * Returns -1 on scan error (e.g., NULL parameter). */
int proc_device_open_by_pid(const char *devnode);

/* Returns 1 if devnode is currently mounted (appears in /proc/mounts), 0 otherwise.
 * Returns 0 if devnode is NULL. */
int proc_device_is_mounted(const char *devnode);

#ifdef __cplusplus
}
#endif

#endif
