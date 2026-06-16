#include "device.h"
#include "device_validate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libudev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

#define WINAFI_DEVICE_ERROR_INIT -1
#define WINAFI_DEVICE_ERROR_ENUM -2
#define WINAFI_DEVICE_ERROR_MEMORY -3
#define WINAFI_DEVICE_ERROR_INVALID -4

typedef struct winafi_device_context {
    struct udev *udev;
} winafi_device_context_t;

/**
 * device_init - Initialize device context
 * Creates and returns a libudev context for device operations.
 *
 * Return: Allocated context on success, NULL on error
 */
winafi_device_context_t *device_init(void) {
    winafi_device_context_t *ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate device context\n");
        return NULL;
    }

    ctx->udev = udev_new();
    if (!ctx->udev) {
        fprintf(stderr, "Failed to create udev context\n");
        free(ctx);
        return NULL;
    }

    return ctx;
}

/**
 * device_cleanup - Cleanup device context
 * Frees the libudev context and allocated memory.
 *
 * @ctx: Device context to cleanup
 */
void device_cleanup(winafi_device_context_t *ctx) {
    if (!ctx) return;
    if (ctx->udev) {
        udev_unref(ctx->udev);
        ctx->udev = NULL;
    }
    free(ctx);
}

/**
 * device_get_property - Helper to get device property
 * Safely retrieves a property from a udev device.
 *
 * @dev: udev device
 * @name: property name
 * @default_val: default value if property not found
 *
 * Return: Property value or default
 */
static const char *device_get_property(struct udev_device *dev,
                                       const char *name,
                                       const char *default_val) {
    const char *val = udev_device_get_property_value(dev, name);
    return val ? val : default_val;
}

/**
 * device_is_usb_device - Check if device is USB device
 * Checks for USB properties (ID_USB_VENDOR) which indicates USB device.
 * Handles both direct USB devices and USB storage devices (via usb-storage driver).
 *
 * @dev: udev device
 *
 * Return: 1 if USB device, 0 otherwise
 */
static int device_is_usb_device(struct udev_device *dev) {
    // Check for USB properties directly (works for usb-storage devices)
    const char *usb_vendor = udev_device_get_property_value(dev, "ID_USB_VENDOR");
    if (usb_vendor) {
        return 1;
    }

    // Fallback: check for USB subsystem parent (original method, less reliable)
    struct udev_device *parent = udev_device_get_parent_with_subsystem_devtype(
        dev, "usb", "usb_device");
    return (parent != NULL) ? 1 : 0;
}

/**
 * device_get_capacity - Get device capacity in bytes
 * Reads capacity from sysfs and converts sectors to bytes.
 *
 * @dev: udev device
 *
 * Return: Capacity in bytes, 0 if unable to determine
 */
static uint64_t device_get_capacity(struct udev_device *dev) {
    // Use sysname (device name like "sdc") instead of sysnum (which may be NULL)
    const char *sysname = udev_device_get_sysname(dev);
    if (!sysname) return 0;

    char sysfs_path[256];
    snprintf(sysfs_path, sizeof(sysfs_path),
             "/sys/block/%s/size", sysname);

    FILE *fp = fopen(sysfs_path, "r");
    if (!fp) return 0;

    uint64_t sectors = 0;
    if (fscanf(fp, "%" PRIu64, &sectors) != 1) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    // Convert 512-byte sectors to bytes
    return sectors * 512;
}

/**
 * device_is_mounted - Check if device is currently mounted
 * Parses /proc/mounts to check if device is mounted.
 *
 * @devnode: Device node path (e.g., /dev/sdb1)
 *
 * Return: 1 if mounted, 0 otherwise
 */
static int device_is_mounted(const char *devnode) {
    if (!devnode) return 0;

    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) return 0;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, devnode, strlen(devnode)) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

/**
 * device_is_system_disk - Check if device appears to be system disk
 * Filters out /dev/sda and other likely system disks.
 *
 * @devnode: Device node path
 *
 * Return: 1 if system disk, 0 otherwise
 */
static int device_is_system_disk(const char *devnode) {
    if (!devnode) return 0;

    // Heuristic: filter out /dev/sda (typically system disk)
    // This is not foolproof but matches Winafi behavior
    if (strcmp(devnode, "/dev/sda") == 0) {
        return 1;
    }

    return 0;
}

/**
 * device_should_skip - Filter virtual/system block devices from enumeration
 */
static int device_should_skip(struct udev_device *dev) {
    const char *dn = udev_device_get_devnode(dev);
    if (!dn) {
        return 1;
    }
    if (strncmp(dn, "/dev/loop", 9) == 0) {
        return 1;
    }
    if (strncmp(dn, "/dev/ram", 8) == 0) {
        return 1;
    }
    if (strncmp(dn, "/dev/dm-", 8) == 0) {
        return 1;
    }
    return 0;
}

/**
 * device_enumerate - Enumerate removable block devices
 * Finds block disks (not partitions) with valid capacity, excluding loop/ram/dm.
 *
 * @ctx: Device context
 * @devices: Output pointer to device array (caller must free)
 * @device_count: Output count of devices found
 *
 * Return: 0 on success, negative on error
 */
int device_enumerate(winafi_device_context_t *ctx,
                    winafi_device_t **devices,
                    int *device_count) {
    if (!ctx || !ctx->udev || !devices || !device_count) {
        fprintf(stderr, "Invalid arguments to device_enumerate\n");
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(ctx->udev);
    if (!enumerate) {
        fprintf(stderr, "Failed to create udev enumerate\n");
        return WINAFI_DEVICE_ERROR_ENUM;
    }

    // Filter for block devices
    udev_enumerate_add_match_subsystem(enumerate, "block");

    // Filter for disks (not partitions)
    udev_enumerate_add_nomatch_sysattr(enumerate, "partition", NULL);

    if (udev_enumerate_scan_devices(enumerate) < 0) {
        fprintf(stderr, "Failed to scan udev devices\n");
        udev_enumerate_unref(enumerate);
        return WINAFI_DEVICE_ERROR_ENUM;
    }

    // First pass: count eligible block devices
    int count = 0;
    struct udev_list_entry *devices_list = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry = NULL;
    udev_list_entry_foreach(entry, devices_list) {
        const char *syspath = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(ctx->udev, syspath);
        if (!dev) continue;

        if (device_should_skip(dev)) {
            udev_device_unref(dev);
            continue;
        }

        const char *devnode = udev_device_get_devnode(dev);
        uint64_t capacity = device_get_capacity(dev);
        if (capacity > 0 && devnode &&
            (device_is_usb_device(dev) || validate_device_is_removable(devnode) == VALIDATE_OK)) {
            count++;
        }
        udev_device_unref(dev);
    }

    if (count == 0) {
        *devices = NULL;
        *device_count = 0;
        udev_enumerate_unref(enumerate);
        return 0;
    }

    // Allocate device array
    *devices = malloc((size_t)count * sizeof(**devices));
    if (!*devices) {
        fprintf(stderr, "Failed to allocate device array\n");
        udev_enumerate_unref(enumerate);
        return WINAFI_DEVICE_ERROR_MEMORY;
    }

    // Second pass: fill device array
    int idx = 0;
    devices_list = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(entry, devices_list) {
        if (idx >= count) break;

        const char *syspath = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(ctx->udev, syspath);
        if (!dev) continue;

        if (device_should_skip(dev)) {
            udev_device_unref(dev);
            continue;
        }

        uint64_t capacity = device_get_capacity(dev);
        const char *devnode = udev_device_get_devnode(dev);
        if (capacity == 0 || !devnode ||
            (!device_is_usb_device(dev) && validate_device_is_removable(devnode) != VALIDATE_OK)) {
            udev_device_unref(dev);
            continue;
        }

        // Fill device info
        winafi_device_t *devinfo = &(*devices)[idx];
        memset(devinfo, 0, sizeof(*devinfo));

        const char *sysname = udev_device_get_sysname(dev);

        if (devnode) {
            strncpy(devinfo->devnode, devnode, sizeof(devinfo->devnode) - 1);
        }

        if (sysname) {
            strncpy(devinfo->sysname, sysname, sizeof(devinfo->sysname) - 1);
        }

        devinfo->capacity_bytes = capacity;

        // Get vendor, model, serial
        const char *vendor = device_get_property(dev, "ID_VENDOR", "Unknown");
        const char *model = device_get_property(dev, "ID_MODEL", "Unknown");
        const char *serial = device_get_property(dev, "ID_SERIAL_SHORT", "Unknown");

        strncpy(devinfo->vendor, vendor, sizeof(devinfo->vendor) - 1);
        strncpy(devinfo->model, model, sizeof(devinfo->model) - 1);
        strncpy(devinfo->serial, serial, sizeof(devinfo->serial) - 1);

        devinfo->is_removable = device_is_usb_device(dev);

        idx++;
        udev_device_unref(dev);
    }

    *device_count = idx;
    udev_enumerate_unref(enumerate);
    return 0;
}

/**
 * device_free_list - Free device enumeration result
 * Deallocates the device array from device_enumerate.
 *
 * @devices: Device array to free
 */
void device_free_list(winafi_device_t *devices) {
    free(devices);
}

/**
 * device_get_info - Get info for single device
 * Retrieves detailed information about a specific device.
 *
 * @ctx: Device context
 * @devnode: Device node path (e.g., /dev/sdb)
 * @out_device: Output device info struct
 *
 * Return: 0 on success, negative on error
 */
int device_get_info(winafi_device_context_t *ctx,
                   const char *devnode,
                   winafi_device_t *out_device) {
    if (!ctx || !ctx->udev || !devnode || !out_device) {
        fprintf(stderr, "Invalid arguments to device_get_info\n");
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    // Get device stat to extract major:minor device numbers
    struct stat st;
    if (stat(devnode, &st) != 0) {
        fprintf(stderr, "Failed to stat device %s\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    if (!S_ISBLK(st.st_mode)) {
        fprintf(stderr, "%s is not a block device\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    // Use proper udev device lookup from actual device number instead of hardcoded makedev
    struct udev_device *dev = udev_device_new_from_devnum(ctx->udev, 'b', st.st_rdev);
    if (!dev) {
        fprintf(stderr, "Failed to get device info for %s\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    memset(out_device, 0, sizeof(*out_device));
    strncpy(out_device->devnode, devnode, sizeof(out_device->devnode) - 1);

    // Get sysname from device
    const char *sysname = udev_device_get_sysname(dev);
    if (sysname) {
        strncpy(out_device->sysname, sysname, sizeof(out_device->sysname) - 1);
    }

    uint64_t capacity = device_get_capacity(dev);
    out_device->capacity_bytes = capacity;

    const char *vendor = device_get_property(dev, "ID_VENDOR", "Unknown");
    const char *model = device_get_property(dev, "ID_MODEL", "Unknown");
    const char *serial = device_get_property(dev, "ID_SERIAL_SHORT", "Unknown");

    strncpy(out_device->vendor, vendor, sizeof(out_device->vendor) - 1);
    strncpy(out_device->model, model, sizeof(out_device->model) - 1);
    strncpy(out_device->serial, serial, sizeof(out_device->serial) - 1);
    out_device->is_removable =
        (device_is_usb_device(dev) || validate_device_is_removable(devnode) == VALIDATE_OK) ? 1 : 0;

    udev_device_unref(dev);
    return 0;
}

/**
 * device_validate - Validate device for use
 * Checks if device is suitable for Winafi operations.
 *
 * @devnode: Device node path
 *
 * Return: 0 if valid, negative if invalid
 */
int device_validate(const char *devnode) {
    if (!devnode) {
        fprintf(stderr, "Invalid device node\n");
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    if (validate_device_is_removable(devnode) != VALIDATE_OK) {
        fprintf(stderr, "Device %s is not removable/USB\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    // Check if device is mounted
    if (device_is_mounted(devnode)) {
        fprintf(stderr, "Device %s is mounted\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    // Check if system disk
    if (device_is_system_disk(devnode)) {
        fprintf(stderr, "Device %s appears to be system disk\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    if (validate_not_system_drive(devnode) == VALIDATE_ERR_SYSTEM_DRIVE) {
        fprintf(stderr, "Device %s contains a protected system mount\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    if (geteuid() == 0 && validate_device_not_locked(devnode) != VALIDATE_OK) {
        fprintf(stderr, "Device %s is locked or busy\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    // Check if device exists
    struct stat st;
    if (stat(devnode, &st) != 0) {
        fprintf(stderr, "Device %s does not exist\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    // Verify it's a block device
    if (!S_ISBLK(st.st_mode)) {
        fprintf(stderr, "%s is not a block device\n", devnode);
        return WINAFI_DEVICE_ERROR_INVALID;
    }

    return 0;
}
