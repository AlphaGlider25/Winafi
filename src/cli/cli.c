#include "winafi.h"
#include "core/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>

// Global flag to track if we're in non-interactive mode
static int g_interactive = 1;

/**
 * Check if running as root
 */
static int check_root(void) {
    return geteuid() == 0 ? 1 : 0;
}

/**
 * Display help message
 */
static void show_help(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Create bootable Windows USB drives from ISO on Linux\n\n");
    printf("OPTIONS:\n");
    printf("  --iso PATH               Path to Windows ISO file\n");
    printf("  --device /dev/sdX        Target USB device\n");
    printf("  --list                   List drives and exit\n");
    printf("  --dangerous              Required to actually write to USB\n");
    printf("  --verbose                Enable verbose logging (debug output)\n");
    printf("  --help                   Show this help message\n\n");
    printf("EXAMPLES:\n");
    printf("  %s --list                  # List drives\n", program_name);
    printf("  %s --iso win10.iso --device /dev/sdb --dangerous\n", program_name);
    printf("  %s                         # Interactive mode\n\n", program_name);
    printf("INTERACTIVE MODE (default):\n");
    printf("  If --iso or --device not provided, you will be prompted interactively.\n");
    printf("  A confirmation prompt will require you to type 'YES' before writing.\n");
}

/**
 * Format bytes as human-readable size (KB, MB, GB)
 */
static void format_size(uint64_t bytes, char *buf, size_t buflen) {
    if (bytes < 1024) {
        snprintf(buf, buflen, "%lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, buflen, "%.1f KB", (double)bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        snprintf(buf, buflen, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buf, buflen, "%.1f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

/**
 * Display available USB devices
 */
static int show_devices(winafi_session_t *session) {
    winafi_device_t *devices = NULL;
    int device_count = 0;

    if (winafi_enumerate_devices(session, &devices, &device_count) != 0) {
        fprintf(stderr, "Error: Failed to enumerate drives\n");
        return -1;
    }

    if (device_count == 0) {
        printf("No drives found.\n");
        return 0;
    }

    printf("\nAvailable USB Devices:\n");
    printf("==========================================\n");
    for (int i = 0; i < device_count; i++) {
        char size_buf[32];
        format_size(devices[i].capacity_bytes, size_buf, sizeof(size_buf));

        printf("[%d] %s (%s)\n", i + 1, devices[i].devnode, size_buf);
        if (devices[i].vendor[0]) {
            printf("    Vendor: %s\n", devices[i].vendor);
        }
        if (devices[i].model[0]) {
            printf("    Model: %s\n", devices[i].model);
        }
    }
    printf("==========================================\n");

    return 0;
}

/**
 * Prompt user for ISO path interactively
 */
static char *get_iso_path_interactive(void) {
    char *path = malloc(512);
    if (!path) return NULL;

    while (1) {
        printf("\nEnter path to Windows ISO file: ");
        fflush(stdout);

        if (!fgets(path, 512, stdin)) {
            free(path);
            return NULL;
        }

        // Remove trailing newline
        size_t len = strlen(path);
        if (len > 0 && path[len - 1] == '\n') {
            path[len - 1] = '\0';
        }

        // Check if file exists
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            return path;
        }

        printf("Error: File not found or not readable: %s\n", path);
    }
}

/**
 * Prompt user to select device interactively
 */
static char *get_device_interactive(winafi_session_t *session) {
    winafi_device_t *devices = NULL;
    int device_count = 0;

    if (winafi_enumerate_devices(session, &devices, &device_count) != 0) {
        fprintf(stderr, "Error: Failed to enumerate drives\n");
        return NULL;
    }

    if (device_count == 0) {
        fprintf(stderr, "Error: No drives found\n");
        return NULL;
    }

    // Show devices
    printf("\nAvailable USB Devices:\n");
    for (int i = 0; i < device_count; i++) {
        char size_buf[32];
        format_size(devices[i].capacity_bytes, size_buf, sizeof(size_buf));
        printf("[%d] %s (%s) - %s %s\n", i + 1, devices[i].devnode, size_buf,
               devices[i].vendor, devices[i].model);
    }

    // Prompt for selection
    char input[256];
    while (1) {
        printf("\nSelect device (by number or path): ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            return NULL;
        }

        // Remove trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Check if it's a number
        if (strlen(input) > 0 && input[0] >= '1' && input[0] <= '9') {
            int idx = atoi(input) - 1;
            if (idx >= 0 && idx < device_count) {
                char *result = malloc(64);
                if (result) {
                    strncpy(result, devices[idx].devnode, 63);
                    result[63] = '\0';
                }
                return result;
            }
        }

        // Check if it's a device path
        for (int i = 0; i < device_count; i++) {
            if (strcmp(input, devices[i].devnode) == 0) {
                char *result = malloc(64);
                if (result) {
                    strncpy(result, input, 63);
                    result[63] = '\0';
                }
                return result;
            }
        }

        printf("Error: Invalid selection\n");
    }
}

/**
 * Get user confirmation (requires typing "YES")
 */
static int get_confirmation(const char *device, const char *iso,
                           uint64_t device_size, uint64_t iso_size) {
    char device_size_buf[32], iso_size_buf[32];
    format_size(device_size, device_size_buf, sizeof(device_size_buf));
    format_size(iso_size, iso_size_buf, sizeof(iso_size_buf));

    printf("\n");
    printf("==========================================\n");
    printf("SUMMARY:\n");
    printf("  Device:  %s (%s)\n", device, device_size_buf);
    printf("  ISO:     %s (%s)\n", iso, iso_size_buf);
    printf("==========================================\n");
    printf("\n");
    printf("WARNING: This will ERASE all data on %s\n", device);
    printf("Type 'YES' (in capitals) to confirm, or anything else to cancel: ");
    fflush(stdout);

    char input[256];
    if (!fgets(input, sizeof(input), stdin)) {
        return 0;
    }

    // Remove trailing newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    if (strcmp(input, "YES") == 0) {
        return 1;
    }

    printf("Operation cancelled.\n");
    return 0;
}

/**
 * Progress callback for display
 */
static void progress_callback(int percent, const char *message, void *user_data) {
    (void)user_data;  // Unused
    printf("\r[%3d%%] %-50s", percent, message);
    fflush(stdout);
    if (percent == 100) {
        printf("\n");
    }
}

/**
 * Main entry point
 */
int cli_main(int argc, char *argv[]) {
    const char *iso_path = NULL;
    const char *device_path = NULL;
    int list_devices = 0;
    int dangerous = 0;
    int verbose = 0;
    int help = 0;

    // Parse arguments
    struct option long_options[] = {
        {"iso", required_argument, NULL, 'i'},
        {"device", required_argument, NULL, 'd'},
        {"list", no_argument, NULL, 'l'},
        {"dangerous", no_argument, NULL, 'D'},
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:d:lDvh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i':
                iso_path = optarg;
                g_interactive = 0;  // Non-interactive if --iso provided
                break;
            case 'd':
                device_path = optarg;
                g_interactive = 0;  // Non-interactive if --device provided
                break;
            case 'l':
                list_devices = 1;
                break;
            case 'D':
                dangerous = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                help = 1;
                break;
            case '?':
            default:
                show_help(argv[0]);
                return 1;
        }
    }

    // Show help if requested
    if (help) {
        show_help(argv[0]);
        return 0;
    }

    // Initialize logging
    winafi_set_log_level(verbose ? WINAFI_LOG_DEBUG : WINAFI_LOG_INFO);
#ifdef WINAFI_VERSION
    log_info("Winafi v%s starting", WINAFI_VERSION);
#else
    log_info("Winafi v0.0.5 starting");
#endif

    // Create session
    winafi_session_t *session = winafi_session_create();
    if (!session) {
        fprintf(stderr, "Error: Failed to create session\n");
        return 1;
    }

    // Handle --list (show devices and exit)
    if (list_devices) {
        int ret = show_devices(session);
        winafi_session_destroy(session);
        return ret;
    }

    // If we have both --iso and --device but no --dangerous, warn and exit
    if (iso_path && device_path && !dangerous) {
        fprintf(stderr, "Error: --dangerous flag is required to write to USB\n");
        fprintf(stderr, "This prevents accidental data loss.\n");
        winafi_session_destroy(session);
        return 1;
    }

    // Check root privileges
    if (!check_root()) {
        fprintf(stderr, "Error: This tool must be run as root (uid 0)\n");
        fprintf(stderr, "Please run with 'sudo' or as root user\n");
        winafi_session_destroy(session);
        return 1;
    }

    // Get ISO path
    if (!iso_path) {
        iso_path = get_iso_path_interactive();
        if (!iso_path) {
            fprintf(stderr, "Error: No ISO path provided\n");
            winafi_session_destroy(session);
            return 1;
        }
    }

    // Load ISO
    log_info("Loading ISO: %s", iso_path);
    if (winafi_session_load_iso(session, iso_path) != 0) {
        fprintf(stderr, "Error: %s\n", winafi_get_error_message(session));
        winafi_session_destroy(session);
        return 1;
    }

    // Get device path
    if (!device_path) {
        device_path = get_device_interactive(session);
        if (!device_path) {
            fprintf(stderr, "Error: No device selected\n");
            winafi_session_destroy(session);
            return 1;
        }
    }

    // Get device info for confirmation
    winafi_device_t *devices = NULL;
    int device_count = 0;
    uint64_t device_capacity = 0;

    if (winafi_enumerate_devices(session, &devices, &device_count) == 0) {
        for (int i = 0; i < device_count; i++) {
            if (strcmp(devices[i].devnode, device_path) == 0) {
                device_capacity = devices[i].capacity_bytes;
                break;
            }
        }
    }

    // Get ISO size (from session after loading)
    // Note: This would be stored in session after load_iso, we'd need to expose it
    // For now, use a placeholder - in production would get from session->iso_info
    uint64_t iso_size = 4700000000ULL;  // ~4.7GB average Windows ISO

    // Select device
    log_info("Selecting device: %s", device_path);
    if (winafi_session_select_device(session, device_path) != 0) {
        fprintf(stderr, "Error: %s\n", winafi_get_error_message(session));
        winafi_session_destroy(session);
        return 1;
    }

    // Get confirmation (unless non-interactive and --dangerous provided)
    if (g_interactive || !dangerous) {
        if (!get_confirmation(device_path, iso_path, device_capacity, iso_size)) {
            winafi_session_destroy(session);
            return 0;  // User cancelled, exit cleanly
        }
    }

    // Require --dangerous flag if we made it here
    if (!dangerous) {
        fprintf(stderr, "Error: --dangerous flag is required to write to USB\n");
        fprintf(stderr, "This prevents accidental data loss.\n");
        winafi_session_destroy(session);
        return 1;
    }

    // Prepare session
    log_info("%s", "Preparing session");
    if (winafi_session_prepare(session) != 0) {
        fprintf(stderr, "Error: %s\n", winafi_get_error_message(session));
        winafi_session_destroy(session);
        return 1;
    }

    // Set progress callback
    winafi_set_progress_callback(session, progress_callback, NULL);

    // Execute
    log_info("%s", "Starting write operation");
    printf("\nWriting to USB drive...\n");
    if (winafi_session_execute(session) != 0) {
        const char *error_msg = winafi_get_error_message(session);
        const char *error_code = winafi_get_error_code(session);
        fprintf(stderr, "\nError: [%s] %s\n", error_code ? error_code : "UNKNOWN",
                error_msg ? error_msg : "Unknown error");
        log_error("Execution failed: %s", error_msg ? error_msg : "unknown");

        // Attempt cleanup on error
        log_info("%s", "Attempting cleanup after error");

        winafi_session_destroy(session);
        return 1;
    }

    // Success
    printf("\nSuccess! USB drive is ready to boot.\n");
    log_info("%s", "Write operation completed successfully");

    winafi_session_destroy(session);
    return 0;
}
