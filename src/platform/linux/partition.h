#ifndef WINAFI_PARTITION_H
#define WINAFI_PARTITION_H

#include <stdint.h>
#include <parted/parted.h>

// Partition table types
#define PARTITION_MBR  0
#define PARTITION_GPT  1

// Return codes
#define PARTITION_OK                0
#define PARTITION_ERR_LIBPARTED    -1
#define PARTITION_ERR_NO_SPACE     -2
#define PARTITION_ERR_INVALID_SIZE -3

// Single partition definition
typedef struct {
    uint64_t start_sector;     // Start sector (aligned to 2048 for 1MB)
    uint64_t end_sector;       // End sector
    int boot;                  // Bootable flag (1 for MBR, 0 for others)
    int partition_type;        // MBR type (0x83 for Linux, 0x07 for NTFS, etc)
} partition_info_t;

// Partition plan (validated before applying)
typedef struct {
    int table_type;            // MBR or GPT
    int num_partitions;        // Number of partitions
    partition_info_t partitions[4];  // Max 4 partitions (MBR limit)
} partition_plan_t;

// Legacy structures for backward compatibility
typedef struct {
    uint64_t boot_size;      // FAT32 boot partition size (bytes)
    uint64_t data_start;     // Data partition start (sectors)
    uint64_t data_size;      // Data partition size (sectors)
} partition_layout_t;

typedef struct winafi_partition_ctx winafi_partition_ctx_t;

// Initialize partition context
winafi_partition_ctx_t *partition_init(void);

// Cleanup partition context
void partition_cleanup(winafi_partition_ctx_t *ctx);

// Plan MBR partition table (no device access yet)
// Returns PARTITION_OK if plan is valid
// References:
//   - https://www.diskgenius.com/how-to/4k-alignment.html
//   - https://rainbow.chard.org/2013/01/30/how-to-align-partitions-for-best-performance-using-parted/
int plan_mbr_partition(
    uint64_t device_size_bytes,
    int num_partitions,
    partition_plan_t *out_plan
);

// Plan GPT partition table
// Returns PARTITION_OK if plan is valid
int plan_gpt_partition(
    uint64_t device_size_bytes,
    int num_partitions,
    partition_plan_t *out_plan
);

// Calculate partition layout for a device (legacy)
// boot_size_bytes: requested boot partition size (e.g., 100MB)
// total_sectors: total device size in sectors
// Returns: 0 on success, negative on error
int partition_calculate_layout(uint64_t boot_size_bytes,
                              uint64_t total_sectors,
                              partition_layout_t *out_layout);

// Create partition table on device (REQUIRES ROOT)
// device: /dev/sdX (block device, not partition)
// boot_size_bytes: size of FAT32 boot partition
// Returns: 0 on success, negative on error
int partition_wipe_and_create(const char *device,
                             uint64_t total_sectors,
                             uint64_t boot_size_bytes);

// Set partition flags (boot, esp)
int partition_set_boot_flag(const char *device, int partition_number);

#endif
