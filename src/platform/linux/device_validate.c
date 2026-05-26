#include "device_validate.h"
#include <errno.h>
#include <fcntl.h>
#include <mntent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * System Drive Protection Strategy
 * ================================
 *
 * Protection sources:
 * 1. /etc/mtab: Current mount table (primary method)
 *    - Standard mount information
 *    - May be unavailable in some environments
 *
 * 2. /proc/mounts: Kernel mount table (fallback method)
 *    - Kernel-managed mount information
 *    - Always available in Linux systems
 *    - More reliable in restricted environments
 *
 * 3. Protected mount points (critical system partitions):
 *    - / (root partition)
 *    - /boot (bootloader and kernel)
 *    - /home (user home directories)
 *    - /var (system variable data)
 *    - /usr (user programs and libraries)
 *
 * References:
 * - [Safe remove drive vs sync](https://www.linux.org/threads/safe-remove-drive-vs-sync-command.51196/)
 * - Linux getmntent(3) documentation for mount table parsing
 * - /proc/mounts kernel interface for mount information
 */

/**
 * validate_device_capacity - Check if device has sufficient capacity for ISO
 * @device_bytes: Total capacity of the device in bytes
 * @iso_bytes: Size of the ISO image in bytes
 *
 * Returns VALIDATE_OK if device_bytes >= iso_bytes + 1GB (1024*1024*1024),
 * otherwise returns VALIDATE_ERR_DEVICE_TOO_SMALL.
 */
int validate_device_capacity(uint64_t device_bytes, uint64_t iso_bytes)
{
    const uint64_t ONE_GB = 1024ULL * 1024 * 1024;
    uint64_t required_bytes = iso_bytes + ONE_GB;

    if (device_bytes >= required_bytes) {
        return VALIDATE_OK;
    }

    return VALIDATE_ERR_DEVICE_TOO_SMALL;
}

/**
 * validate_not_system_drive - Check if device is a system drive
 * @devnode: Device path (e.g. "/dev/sda1")
 *
 * Checks if device is mounted at protected system locations:
 * - / (root)
 * - /boot (boot partition)
 * - /home (home partition)
 * - /var (system variable data)
 * - /usr (user programs and libraries)
 *
 * Uses dual fallback strategy:
 * 1. Primary: /etc/mtab (current mount table)
 * 2. Fallback: /proc/mounts (kernel mount table)
 *
 * Returns VALIDATE_OK if device is not a system drive,
 * VALIDATE_ERR_SYSTEM_DRIVE if mounted at protected location,
 * VALIDATE_ERR_NOT_FOUND if device not found in mount tables.
 */
int validate_not_system_drive(const char *devnode)
{
    if (!devnode) {
        return VALIDATE_ERR_NOT_FOUND;
    }

    // Protected mountpoints that should never be formatted
    const char *protected_mounts[] = {
        "/",      // root partition
        "/boot",  // boot partition
        "/home",  // home partition
        "/var",   // system variable data
        "/usr"    // user programs and libraries
    };
    const int num_protected = 5;

    // Method 1: Check /etc/mtab (primary method)
    FILE *fp = fopen("/etc/mtab", "r");
    if (fp) {
        struct mntent *entry;
        while ((entry = getmntent(fp)) != NULL) {
            // Check if this entry matches our device
            if (strcmp(entry->mnt_fsname, devnode) == 0) {
                // Check if mounted at a protected location
                for (int i = 0; i < num_protected; i++) {
                    if (strcmp(entry->mnt_dir, protected_mounts[i]) == 0) {
                        fclose(fp);
                        return VALIDATE_ERR_SYSTEM_DRIVE;
                    }
                }
                // Device found but not at protected location
                fclose(fp);
                return VALIDATE_OK;
            }
        }
        fclose(fp);
        // Device not found in mtab, try fallback
    }

    // Method 2: Check /proc/mounts as fallback (if /etc/mtab not available)
    FILE *proc_mounts = fopen("/proc/mounts", "r");
    if (proc_mounts) {
        struct mntent *entry;
        while ((entry = getmntent(proc_mounts)) != NULL) {
            // Check if this entry matches our device
            if (strcmp(entry->mnt_fsname, devnode) == 0) {
                // Check if mounted at a protected location
                for (int i = 0; i < num_protected; i++) {
                    if (strcmp(entry->mnt_dir, protected_mounts[i]) == 0) {
                        fclose(proc_mounts);
                        return VALIDATE_ERR_SYSTEM_DRIVE;
                    }
                }
                // Device found but not at protected location
                fclose(proc_mounts);
                return VALIDATE_OK;
            }
        }
        fclose(proc_mounts);
    }

    // Device not found in either mtab or /proc/mounts (not currently mounted)
    return VALIDATE_ERR_NOT_FOUND;
}

/**
 * validate_device_not_locked - Check if device is in use or mounted
 * @devnode: Device path (e.g. "/dev/sdb")
 *
 * Attempts to open device with O_RDONLY | O_EXCL flags.
 * O_EXCL fails if the device is already open elsewhere (in use/mounted).
 *
 * Returns VALIDATE_OK if device can be opened exclusively,
 * VALIDATE_ERR_DEVICE_LOCKED if device is in use,
 * VALIDATE_ERR_NOT_FOUND if device does not exist.
 */
int validate_device_not_locked(const char *devnode)
{
    if (!devnode) {
        return VALIDATE_ERR_NOT_FOUND;
    }

    // Try to open device with exclusive access
    int fd = open(devnode, O_RDONLY | O_EXCL);

    if (fd >= 0) {
        // Successfully opened - device is not locked
        close(fd);
        return VALIDATE_OK;
    }

    // Check specific error codes
    if (errno == ENOENT) {
        return VALIDATE_ERR_NOT_FOUND;
    }

    if (errno == EBUSY) {
        return VALIDATE_ERR_DEVICE_LOCKED;
    }

    // For other errors (EACCES, etc.), treat as locked to be safe
    return VALIDATE_ERR_DEVICE_LOCKED;
}
