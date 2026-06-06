#ifndef WINAFI_MBR_BOOTLOADER_H
#define WINAFI_MBR_BOOTLOADER_H

#include <stdint.h>
#include <sys/types.h>

/**
 * MBR Bootloader Installation Module
 *
 * Handles installation of bootloader code into the Master Boot Record (MBR)
 * and setting boot flags for partition tables.
 *
 * MBR Layout:
 * - Bytes 0-445: Boot code (446 bytes)
 * - Bytes 446-509: Partition table (4 entries x 16 bytes each)
 * - Bytes 510-511: Boot signature (0xAA55)
 *
 * For GPT partitions:
 * - MBR bootcode installation is skipped
 * - EFI System Partition (ESP) is marked with GUID
 * - Protective MBR in first sector is preserved
 */

// Boot type enumeration
typedef enum {
    MBR_BOOT_WINDOWS = 1,  // Windows PE MBR bootcode
    MBR_BOOT_LINUX = 2,    // Linux MBR bootcode (Syslinux or GRUB2 BIOS)
    MBR_BOOT_HYBRID = 3,   // Can boot multiple OSes
} mbr_boot_type_t;

// MBR installation configuration
typedef struct {
    const char *device;                 // /dev/sdc (target USB device)
    mbr_boot_type_t type;               // Type of bootcode to install
    int is_gpt;                         // 1 = GPT partition table, 0 = MBR
    int active_partition;               // Which partition to mark active (1-4 for MBR)
    char bootcode_path[4096];           // Path to bootcode binary file
} mbr_install_info_t;

// Error codes
#define MBR_OK                          0
#define MBR_ERR_DEVICE_NOT_FOUND       -1  // Device doesn't exist
#define MBR_ERR_PERMISSION_DENIED      -2  // No write permission on device
#define MBR_ERR_INVALID_PARTITION      -3  // Active partition out of range
#define MBR_ERR_BOOTCODE_NOT_FOUND     -4  // Bootcode file missing
#define MBR_ERR_WRITE_FAILED           -5  // Failed to write MBR
#define MBR_ERR_GPT_PARTITION_TABLE    -6  // GPT doesn't use MBR bootcode
#define MBR_ERR_DEVICE_BUSY            -7  // Device in use, can't write
#define MBR_ERR_INVALID_BOOTCODE_SIZE  -8  // Bootcode file not exactly 446 bytes
#define MBR_ERR_READ_FAILED            -9  // Failed to read from device
#define MBR_ERR_SIGNATURE_MISMATCH     -10 // MBR signature not 0xAA55
#define MBR_ERR_PARTITION_TABLE_CORRUPT -11 // Partition table seems corrupted

// Progress callback type
typedef void (*mbr_progress_callback_t)(int percent, const char *message, void *user_data);

/**
 * install_mbr_bootcode - Install bootcode into MBR (first 446 bytes)
 * @device: Device path (e.g., "/dev/sdc", not "/dev/sdc1")
 * @bootcode_path: Path to 446-byte bootcode file
 * @progress_cb: Optional progress callback (can be NULL)
 * @user_data: Optional user data passed to callback
 *
 * Installs bootloader code into the first 446 bytes of the device's MBR.
 * Preserves partition table (bytes 446-509) and boot signature (bytes 510-511).
 *
 * Returns:
 *   MBR_OK on success
 *   MBR_ERR_DEVICE_NOT_FOUND if device doesn't exist
 *   MBR_ERR_PERMISSION_DENIED if no write permission
 *   MBR_ERR_BOOTCODE_NOT_FOUND if bootcode file missing
 *   MBR_ERR_INVALID_BOOTCODE_SIZE if bootcode not exactly 446 bytes
 *   MBR_ERR_READ_FAILED if failed to read from device
 *   MBR_ERR_WRITE_FAILED if failed to write to device
 *   MBR_ERR_SIGNATURE_MISMATCH if signature validation failed
 */
int install_mbr_bootcode(const char *device,
                        const char *bootcode_path,
                        mbr_progress_callback_t progress_cb,
                        void *user_data);

/**
 * set_boot_flag - Set boot flag on MBR partition table
 * @device: Device path (e.g., "/dev/sdc", not "/dev/sdc1")
 * @partition_number: Partition number to mark active (1-4)
 * @progress_cb: Optional progress callback (can be NULL)
 * @user_data: Optional user data passed to callback
 *
 * Sets the boot flag (0x80) on the specified MBR partition and clears it
 * from other partitions. Only one partition can have the boot flag set.
 *
 * MBR Partition Entry Layout (16 bytes):
 * - Byte 0: Boot indicator (0x00=inactive, 0x80=active)
 * - Bytes 1-3: CHS address of first sector
 * - Byte 4: Partition type
 * - Bytes 5-7: CHS address of last sector
 * - Bytes 8-11: LBA of first absolute sector
 * - Bytes 12-15: Number of sectors in partition
 *
 * Returns:
 *   MBR_OK on success
 *   MBR_ERR_DEVICE_NOT_FOUND if device doesn't exist
 *   MBR_ERR_PERMISSION_DENIED if no write permission
 *   MBR_ERR_INVALID_PARTITION if partition number out of range (1-4)
 *   MBR_ERR_READ_FAILED if failed to read MBR
 *   MBR_ERR_WRITE_FAILED if failed to write MBR
 *   MBR_ERR_GPT_PARTITION_TABLE if device has GPT (use mark_efi_system_partition instead)
 */
int set_boot_flag(const char *device,
                 int partition_number,
                 mbr_progress_callback_t progress_cb,
                 void *user_data);

/**
 * mark_efi_system_partition - Mark partition as EFI System Partition (GPT only)
 * @device: Device path (e.g., "/dev/sdc", not "/dev/sdc1")
 * @partition_number: Partition number to mark as ESP (1-based)
 * @progress_cb: Optional progress callback (can be NULL)
 * @user_data: Optional user data passed to callback
 *
 * For GPT partition tables, marks the specified partition with the EFI System
 * Partition GUID: C12A7328-F81F-11D2-BA4B-00A0C93EC93B
 *
 * Note: This is typically done by libparted during partition creation,
 * but this function allows for verification or correction.
 *
 * Returns:
 *   MBR_OK on success
 *   MBR_ERR_DEVICE_NOT_FOUND if device doesn't exist
 *   MBR_ERR_PERMISSION_DENIED if no write permission
 *   MBR_ERR_INVALID_PARTITION if partition number out of range
 *   MBR_ERR_READ_FAILED if failed to read GPT
 *   MBR_ERR_WRITE_FAILED if failed to write GPT
 *   MBR_ERR_GPT_PARTITION_TABLE if device doesn't have GPT (not an error for this context)
 */
int mark_efi_system_partition(const char *device,
                             int partition_number,
                             mbr_progress_callback_t progress_cb,
                             void *user_data);

/**
 * install_mbr_bootloader - Main entry point for bootloader installation
 * @device: Device path (e.g., "/dev/sdc", not "/dev/sdc1")
 * @info: Configuration structure with boot type and partition info
 * @progress_cb: Optional progress callback (can be NULL)
 * @user_data: Optional user data passed to callback
 *
 * Orchestrates the full bootloader installation process:
 * - For MBR: installs bootcode and sets boot flag
 * - For GPT: marks EFI System Partition
 *
 * Device must not be mounted and must not be system drive (caller's responsibility).
 *
 * Returns:
 *   MBR_OK on success
 *   Appropriate MBR_ERR_* code on failure
 */
int install_mbr_bootloader(const char *device,
                          const mbr_install_info_t *info,
                          mbr_progress_callback_t progress_cb,
                          void *user_data);

/**
 * get_mbr_error_message - Get human-readable error message
 * @error_code: Error code returned from mbr_* functions
 *
 * Returns:
 *   Pointer to static error message string
 */
const char *get_mbr_error_message(int error_code);

#endif // WINAFI_MBR_BOOTLOADER_H
