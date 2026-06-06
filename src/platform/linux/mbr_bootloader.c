#include "mbr_bootloader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

// Constants
#define SECTOR_SIZE 512
#define BOOTCODE_SIZE 446
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_TABLE_SIZE 64  // 4 partitions x 16 bytes
#define BOOT_SIGNATURE_OFFSET 510
#define BOOT_SIGNATURE_VALUE 0xAA55
#define PARTITION_ENTRY_SIZE 16
#define BOOT_FLAG_OFFSET 0
#define BOOT_FLAG_ACTIVE 0x80
#define BOOT_FLAG_INACTIVE 0x00

// EFI System Partition GUID (big-endian representation)
static const uint8_t EFI_SYSTEM_PARTITION_GUID[] = {
    0xC1, 0x2A, 0x73, 0x28,  // First 4 bytes
    0xF8, 0x1F,              // Next 2 bytes
    0x11, 0xD2,              // Next 2 bytes
    0xBA, 0x4B,              // Next 2 bytes
    0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B  // Last 6 bytes
};

/**
 * get_mbr_error_message - Get human-readable error message
 */
const char *get_mbr_error_message(int error_code)
{
    switch (error_code) {
        case MBR_OK:
            return "Success";
        case MBR_ERR_DEVICE_NOT_FOUND:
            return "Device not found or doesn't exist";
        case MBR_ERR_PERMISSION_DENIED:
            return "Permission denied (requires root)";
        case MBR_ERR_INVALID_PARTITION:
            return "Invalid partition number (must be 1-4 for MBR)";
        case MBR_ERR_BOOTCODE_NOT_FOUND:
            return "Bootcode file not found";
        case MBR_ERR_WRITE_FAILED:
            return "Failed to write to device (disk full or I/O error)";
        case MBR_ERR_GPT_PARTITION_TABLE:
            return "Device has GPT partition table (MBR bootcode not applicable)";
        case MBR_ERR_DEVICE_BUSY:
            return "Device is in use or mounted";
        case MBR_ERR_INVALID_BOOTCODE_SIZE:
            return "Bootcode file must be exactly 446 bytes";
        case MBR_ERR_READ_FAILED:
            return "Failed to read from device";
        case MBR_ERR_SIGNATURE_MISMATCH:
            return "MBR boot signature mismatch (expected 0xAA55)";
        case MBR_ERR_PARTITION_TABLE_CORRUPT:
            return "Partition table appears corrupted";
        default:
            return "Unknown error";
    }
}

/**
 * read_mbr - Read MBR from device
 * @fd: Open file descriptor for device
 * @mbr: Buffer to store 512-byte MBR
 *
 * Returns 0 on success, -1 on error
 */
static int read_mbr(int fd, uint8_t *mbr)
{
    if (lseek(fd, 0, SEEK_SET) == -1) {
        return -1;
    }

    ssize_t n = read(fd, mbr, SECTOR_SIZE);
    if (n != SECTOR_SIZE) {
        return -1;
    }

    return 0;
}

/**
 * write_mbr - Write MBR back to device
 * @fd: Open file descriptor for device
 * @mbr: Buffer containing 512-byte MBR
 *
 * Returns 0 on success, -1 on error
 */
static int write_mbr(int fd, const uint8_t *mbr)
{
    if (lseek(fd, 0, SEEK_SET) == -1) {
        return -1;
    }

    ssize_t n = write(fd, mbr, SECTOR_SIZE);
    if (n != SECTOR_SIZE) {
        return -1;
    }

    // Ensure write is committed to disk
    if (fsync(fd) == -1) {
        return -1;
    }

    return 0;
}

/**
 * read_bootcode_file - Read bootcode from file
 * @path: Path to bootcode file
 * @bootcode: Buffer to store bootcode (will allocate)
 * @size: Pointer to store size (should be 446)
 *
 * Returns MBR_OK on success, MBR_ERR_* on error
 */
static int read_bootcode_file(const char *path, uint8_t **bootcode, size_t *size)
{
    struct stat st;
    if (stat(path, &st) == -1) {
        return MBR_ERR_BOOTCODE_NOT_FOUND;
    }

    // Bootcode must be exactly 446 bytes
    if (st.st_size != BOOTCODE_SIZE) {
        return MBR_ERR_INVALID_BOOTCODE_SIZE;
    }

    *bootcode = (uint8_t *)malloc(BOOTCODE_SIZE);
    if (*bootcode == NULL) {
        return MBR_ERR_WRITE_FAILED;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        free(*bootcode);
        *bootcode = NULL;
        return MBR_ERR_BOOTCODE_NOT_FOUND;
    }

    ssize_t n = read(fd, *bootcode, BOOTCODE_SIZE);
    close(fd);

    if (n != BOOTCODE_SIZE) {
        free(*bootcode);
        *bootcode = NULL;
        if (n == -1) {
            return MBR_ERR_READ_FAILED;
        }
        return MBR_ERR_INVALID_BOOTCODE_SIZE;
    }

    *size = (size_t)n;
    return MBR_OK;
}

/**
 * get_boot_signature - Extract boot signature from MBR
 */
static uint16_t get_boot_signature(const uint8_t *mbr)
{
    // Little-endian: bytes 510-511
    return (uint16_t)(mbr[510] | (mbr[511] << 8));
}

/**
 * set_boot_signature - Set boot signature in MBR
 */
static void set_boot_signature(uint8_t *mbr, uint16_t signature)
{
    // Little-endian: bytes 510-511
    mbr[510] = (uint8_t)(signature & 0xFF);
    mbr[511] = (uint8_t)((signature >> 8) & 0xFF);
}

/**
 * install_mbr_bootcode - Install bootcode into MBR
 */
int install_mbr_bootcode(const char *device,
                        const char *bootcode_path,
                        mbr_progress_callback_t progress_cb,
                        void *user_data)
{
    if (device == NULL || bootcode_path == NULL) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    // Check if device exists
    struct stat st;
    if (stat(device, &st) == -1) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    // Read bootcode file
    uint8_t *bootcode = NULL;
    size_t bootcode_size = 0;

    int bootcode_ret = read_bootcode_file(bootcode_path, &bootcode, &bootcode_size);
    if (bootcode_ret != MBR_OK) {
        return bootcode_ret;
    }

    if (progress_cb != NULL) {
        progress_cb(10, "Reading MBR from device...", user_data);
    }

    // Open device for reading and writing
    int fd = open(device, O_RDWR);
    if (fd == -1) {
        free(bootcode);
        if (errno == EACCES) {
            return MBR_ERR_PERMISSION_DENIED;
        } else if (errno == EBUSY) {
            return MBR_ERR_DEVICE_BUSY;
        }
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    // Read current MBR
    uint8_t *mbr = (uint8_t *)malloc(SECTOR_SIZE);
    if (mbr == NULL) {
        close(fd);
        free(bootcode);
        return MBR_ERR_READ_FAILED;
    }

    if (read_mbr(fd, mbr) == -1) {
        close(fd);
        free(mbr);
        free(bootcode);
        return MBR_ERR_READ_FAILED;
    }

    if (progress_cb != NULL) {
        progress_cb(30, "Installing bootloader code...", user_data);
    }

    // Replace first 446 bytes with bootcode
    memcpy(mbr, bootcode, BOOTCODE_SIZE);

    // Ensure boot signature is correct
    uint16_t signature = get_boot_signature(mbr);
    if (signature != BOOT_SIGNATURE_VALUE) {
        set_boot_signature(mbr, BOOT_SIGNATURE_VALUE);
    }

    if (progress_cb != NULL) {
        progress_cb(60, "Writing MBR to device...", user_data);
    }

    // Write modified MBR back
    if (write_mbr(fd, mbr) == -1) {
        close(fd);
        free(mbr);
        free(bootcode);
        return MBR_ERR_WRITE_FAILED;
    }

    if (progress_cb != NULL) {
        progress_cb(100, "Bootloader installation complete", user_data);
    }

    close(fd);
    free(mbr);
    free(bootcode);

    return MBR_OK;
}

/**
 * set_boot_flag - Set boot flag on partition
 */
int set_boot_flag(const char *device,
                 int partition_number,
                 mbr_progress_callback_t progress_cb,
                 void *user_data)
{
    if (device == NULL) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    // Validate partition number
    if (partition_number < 1 || partition_number > 4) {
        return MBR_ERR_INVALID_PARTITION;
    }

    // Check if device exists
    struct stat st;
    if (stat(device, &st) == -1) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    if (progress_cb != NULL) {
        progress_cb(20, "Reading partition table...", user_data);
    }

    // Open device
    int fd = open(device, O_RDWR);
    if (fd == -1) {
        if (errno == EACCES) {
            return MBR_ERR_PERMISSION_DENIED;
        } else if (errno == EBUSY) {
            return MBR_ERR_DEVICE_BUSY;
        }
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    // Read MBR
    uint8_t *mbr = (uint8_t *)malloc(SECTOR_SIZE);
    if (mbr == NULL) {
        close(fd);
        return MBR_ERR_READ_FAILED;
    }

    if (read_mbr(fd, mbr) == -1) {
        close(fd);
        free(mbr);
        return MBR_ERR_READ_FAILED;
    }

    if (progress_cb != NULL) {
        progress_cb(50, "Setting boot partition flags...", user_data);
    }

    // Iterate through partition table and set/clear boot flags
    for (int i = 0; i < 4; i++) {
        int partition_offset = PARTITION_TABLE_OFFSET + (i * PARTITION_ENTRY_SIZE);

        if (i == (partition_number - 1)) {
            // Set boot flag for this partition
            mbr[partition_offset + BOOT_FLAG_OFFSET] = BOOT_FLAG_ACTIVE;
        } else {
            // Clear boot flag for other partitions
            mbr[partition_offset + BOOT_FLAG_OFFSET] = BOOT_FLAG_INACTIVE;
        }
    }

    // Verify boot signature
    uint16_t signature = get_boot_signature(mbr);
    if (signature != BOOT_SIGNATURE_VALUE) {
        close(fd);
        free(mbr);
        return MBR_ERR_SIGNATURE_MISMATCH;
    }

    if (progress_cb != NULL) {
        progress_cb(75, "Writing partition table...", user_data);
    }

    // Write MBR back
    if (write_mbr(fd, mbr) == -1) {
        close(fd);
        free(mbr);
        return MBR_ERR_WRITE_FAILED;
    }

    if (progress_cb != NULL) {
        progress_cb(100, "Boot flag set successfully", user_data);
    }

    close(fd);
    free(mbr);

    return MBR_OK;
}

/**
 * mark_efi_system_partition - Mark partition as ESP (GPT)
 *
 * Modifies the partition type GUID in a GPT partition table to mark
 * the specified partition as an EFI System Partition.
 */
int mark_efi_system_partition(const char *device,
                             int partition_number,
                             mbr_progress_callback_t progress_cb,
                             void *user_data)
{
    if (device == NULL) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    // Validate partition number (1-128 for GPT)
    if (partition_number < 1 || partition_number > 128) {
        return MBR_ERR_INVALID_PARTITION;
    }

    // Check if device exists
    struct stat st;
    if (stat(device, &st) == -1) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    if (progress_cb != NULL) {
        progress_cb(20, "Reading GPT header...", user_data);
    }

    // Open device for reading and writing
    int fd = open(device, O_RDWR);
    if (fd == -1) {
        if (errno == EACCES) {
            return MBR_ERR_PERMISSION_DENIED;
        } else if (errno == EBUSY) {
            return MBR_ERR_DEVICE_BUSY;
        }
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    // Read GPT header (at sector 1 = offset 512)
    unsigned char gpt_header[512] = {0};
    if (lseek(fd, 512, SEEK_SET) < 0) {
        close(fd);
        return MBR_ERR_READ_FAILED;
    }
    if (read(fd, gpt_header, 512) != 512) {
        close(fd);
        return MBR_ERR_READ_FAILED;
    }

    // Verify GPT signature "EFI PART" at offset 0
    if (memcmp(gpt_header, "EFI PART", 8) != 0) {
        close(fd);
        return MBR_ERR_GPT_PARTITION_TABLE;
    }

    if (progress_cb != NULL) {
        progress_cb(40, "Locating partition entry...", user_data);
    }

    // Parse GPT header to find partition entry location
    // Offset 72: first partition entry LBA (little-endian 64-bit)
    uint64_t partition_entry_lba = 0;
    memcpy(&partition_entry_lba, &gpt_header[72], 8);

    // Offset 84: size of partition entry (usually 128)
    uint32_t entry_size = 0;
    memcpy(&entry_size, &gpt_header[84], 4);

    // Validate entry size (should be 128)
    if (entry_size < 128 || entry_size > 512) {
        close(fd);
        return MBR_ERR_PARTITION_TABLE_CORRUPT;
    }

    // Calculate offset to target partition entry
    // partition_number is 1-based, convert to 0-based index
    uint64_t entry_offset = (partition_entry_lba * 512) +
                           (((uint32_t)(partition_number - 1)) * entry_size);

    if (progress_cb != NULL) {
        progress_cb(50, "Reading partition entry...", user_data);
    }

    // Read the partition entry
    unsigned char partition_entry[512] = {0};
    if (lseek(fd, entry_offset, SEEK_SET) < 0) {
        close(fd);
        return MBR_ERR_READ_FAILED;
    }
    if (read(fd, partition_entry, entry_size) != (ssize_t)entry_size) {
        close(fd);
        return MBR_ERR_READ_FAILED;
    }

    // Check if partition is in use (attribute bit 0, at offset 48)
    uint64_t attributes = 0;
    memcpy(&attributes, &partition_entry[48], 8);
    if (!(attributes & 1)) {
        // Partition not in use
        close(fd);
        return MBR_ERR_INVALID_PARTITION;
    }

    if (progress_cb != NULL) {
        progress_cb(60, "Updating partition type GUID...", user_data);
    }

    // Set partition type GUID to EFI System Partition
    // GUID: C12A7328-F81F-11D2-BA4B-00A0C93EC93B
    // Note: The GUID constant is already defined at the top of the file
    // but we need to use it in the correct byte order for the partition entry
    memcpy(&partition_entry[0], EFI_SYSTEM_PARTITION_GUID, 16);

    if (progress_cb != NULL) {
        progress_cb(70, "Writing partition entry...", user_data);
    }

    // Write partition entry back
    if (lseek(fd, entry_offset, SEEK_SET) < 0) {
        close(fd);
        return MBR_ERR_WRITE_FAILED;
    }
    if (write(fd, partition_entry, entry_size) != (ssize_t)entry_size) {
        close(fd);
        return MBR_ERR_WRITE_FAILED;
    }

    if (progress_cb != NULL) {
        progress_cb(90, "Syncing to disk...", user_data);
    }

    // Sync to disk
    if (fsync(fd) < 0) {
        close(fd);
        return MBR_ERR_WRITE_FAILED;
    }

    close(fd);

    if (progress_cb != NULL) {
        progress_cb(100, "EFI System Partition marked successfully", user_data);
    }

    return MBR_OK;
}

/**
 * install_mbr_bootloader - Main orchestration function
 */
int install_mbr_bootloader(const char *device,
                          const mbr_install_info_t *info,
                          mbr_progress_callback_t progress_cb,
                          void *user_data)
{
    if (device == NULL || info == NULL) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    if (progress_cb != NULL) {
        progress_cb(0, "Starting bootloader installation...", user_data);
    }

    // Check device exists
    struct stat st;
    if (stat(device, &st) == -1) {
        return MBR_ERR_DEVICE_NOT_FOUND;
    }

    int ret = MBR_OK;

    if (info->is_gpt) {
        // GPT: Mark EFI System Partition
        if (progress_cb != NULL) {
            progress_cb(20, "Processing GPT partition table...", user_data);
        }

        ret = mark_efi_system_partition(device, info->active_partition, progress_cb, user_data);
    } else {
        // MBR: Install bootcode and set boot flag
        if (progress_cb != NULL) {
            progress_cb(20, "Installing MBR bootcode...", user_data);
        }

        ret = install_mbr_bootcode(device, info->bootcode_path, progress_cb, user_data);
        if (ret != MBR_OK) {
            return ret;
        }

        if (progress_cb != NULL) {
            progress_cb(60, "Setting active boot partition...", user_data);
        }

        ret = set_boot_flag(device, info->active_partition, progress_cb, user_data);
    }

    if (ret == MBR_OK && progress_cb != NULL) {
        progress_cb(100, "Bootloader installation complete", user_data);
    }

    return ret;
}
