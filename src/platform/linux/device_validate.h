#ifndef WINAFI_DEVICE_VALIDATE_H
#define WINAFI_DEVICE_VALIDATE_H

#include <stdint.h>

/**
 * Device validation return codes
 */
#define VALIDATE_OK 0
#define VALIDATE_ERR_DEVICE_TOO_SMALL -1
#define VALIDATE_ERR_SYSTEM_DRIVE -2
#define VALIDATE_ERR_DEVICE_LOCKED -3
#define VALIDATE_ERR_DEVICE_BUSY -4
#define VALIDATE_ERR_NOT_FOUND -5

/**
 * validate_device_capacity - Check if device has sufficient capacity for ISO
 * @device_bytes: Total capacity of the device in bytes
 * @iso_bytes: Size of the ISO image in bytes
 *
 * Ensures device capacity >= ISO size + 1GB (1024*1024*1024 bytes) headroom
 * for filesystem overhead, partition table, and bootloader.
 *
 * Returns:
 *   VALIDATE_OK if device has sufficient capacity
 *   VALIDATE_ERR_DEVICE_TOO_SMALL if device is too small
 */
int validate_device_capacity(uint64_t device_bytes, uint64_t iso_bytes);

/**
 * validate_not_system_drive - Check if device is a system drive
 * @devnode: Device path (e.g. "/dev/sda1")
 *
 * Detects if device is mounted at protected mountpoints using dual fallback:
 * - / (root)
 * - /boot (boot partition)
 * - /home (home partition)
 * - /var (system variable data)
 * - /usr (user programs and libraries)
 *
 * Primary source: /etc/mtab (current mount table)
 * Fallback source: /proc/mounts (kernel mount table)
 *
 * Prevents accidental formatting of system partitions.
 *
 * Returns:
 *   VALIDATE_OK if device is not a system drive
 *   VALIDATE_ERR_SYSTEM_DRIVE if device is mounted at protected location
 *   VALIDATE_ERR_NOT_FOUND if device not found in mount tables (not mounted)
 */
int validate_not_system_drive(const char *devnode);

/**
 * validate_device_not_locked - Check if device is in use or mounted
 * @devnode: Device path (e.g. "/dev/sdb")
 *
 * Attempts to open device with O_RDONLY | O_EXCL flags. If the open succeeds,
 * the device is not locked. If it fails with EBUSY, the device is in use.
 *
 * References:
 * - Linux open(2) man page: O_EXCL flag
 * - Device locking mechanism prevents simultaneous writes
 *
 * Returns:
 *   VALIDATE_OK if device can be opened exclusively
 *   VALIDATE_ERR_DEVICE_LOCKED if device is in use/mounted
 *   VALIDATE_ERR_NOT_FOUND if device does not exist
 */
int validate_device_not_locked(const char *devnode);

#endif // WINAFI_DEVICE_VALIDATE_H
