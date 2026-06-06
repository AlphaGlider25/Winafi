#ifndef WINAFI_BADBLOCKS_CHECK_H
#define WINAFI_BADBLOCKS_CHECK_H

#include <stdint.h>

/**
 * Progress tracking for bad blocks checking operations
 */
typedef struct {
    uint64_t total_blocks;          // Total number of blocks on device
    uint64_t bad_blocks_count;      // Number of bad blocks found so far
    uint64_t current_block;         // Current block being checked
    int test_passes_completed;      // Number of test passes completed
    int test_passes_total;          // Total number of test passes to perform
} badblocks_progress_t;

/**
 * Progress callback for bad blocks checking.
 * Called during badblocks_check_device() to report progress.
 *
 * progress: pointer to badblocks_progress_t with current status
 * user_data: opaque user-provided context
 */
typedef void (*badblocks_progress_cb_t)(const badblocks_progress_t *progress, void *user_data);

/**
 * Perform non-destructive bad blocks check on device.
 * Uses read-only tests to identify bad blocks without destroying data.
 *
 * devnode: device path (e.g., /dev/sdb1)
 * num_passes: number of test passes to perform (1-5 typical)
 * callback: optional progress callback (NULL if not needed)
 * user_data: opaque context passed to progress callback
 *
 * Returns: 0 on success (check completed, may have found bad blocks),
 *          -1 on error (device error, permission denied, etc.)
 */
int badblocks_check_device(const char *devnode, int num_passes,
                          badblocks_progress_cb_t callback, void *user_data);

#endif
