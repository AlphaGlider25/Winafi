#define _POSIX_C_SOURCE 200809L

#include "badblocks_check.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define BB_BLOCKS_AT_ONCE (32)   // read 32 blocks at a time
#define BB_BLOCK_SIZE (512)       // sector size

int badblocks_check_device(const char *devnode, int num_passes,
                          badblocks_progress_cb_t callback, void *user_data) {
    if (!devnode || num_passes < 1 || num_passes > 4) return -1;

    // Open device read-only
    int fd = open(devnode, O_RDONLY);
    if (fd < 0) return -1;

    // Get device capacity
    uint64_t device_size = (uint64_t)lseek(fd, 0, SEEK_END);
    if (device_size <= 0) {
        close(fd);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);

    uint64_t total_blocks = device_size / BB_BLOCK_SIZE;
    uint64_t bad_count = 0;

    badblocks_progress_t progress;
    memset(&progress, 0, sizeof(progress));
    progress.total_blocks = total_blocks;
    progress.test_passes_total = num_passes;

    // For each pass, read entire device with different patterns
    for (int pass = 0; pass < num_passes; pass++) {
        lseek(fd, 0, SEEK_SET);
        progress.test_passes_completed = pass;

        // Read blocks in chunks
        unsigned char *buffer = malloc(BB_BLOCKS_AT_ONCE * BB_BLOCK_SIZE);
        if (!buffer) {
            close(fd);
            return -1;
        }

        uint64_t blocks_read = 0;

        while (blocks_read < total_blocks) {
            size_t to_read = ((total_blocks - blocks_read) < (uint64_t)BB_BLOCKS_AT_ONCE) ?
                           (size_t)(total_blocks - blocks_read) * BB_BLOCK_SIZE :
                           (size_t)BB_BLOCKS_AT_ONCE * BB_BLOCK_SIZE;

            ssize_t n = read(fd, buffer, to_read);
            if (n < 0) {
                // Read error — bad block found
                bad_count++;
                lseek(fd, (off_t)to_read, SEEK_CUR);  // skip to next block
            }

            blocks_read += (n > 0) ? (uint64_t)(n / BB_BLOCK_SIZE) : 1;
            progress.current_block = blocks_read;

            if (callback) {
                callback(&progress, user_data);
            }
        }

        free(buffer);
    }

    progress.test_passes_completed = num_passes;
    progress.bad_blocks_count = bad_count;
    if (callback) {
        callback(&progress, user_data);
    }

    close(fd);
    return 0;
}
