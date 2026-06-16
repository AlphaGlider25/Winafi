#include "fs_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// Declare extern environ for execve() calls
extern char **environ;

int fs_format_fat32(const char *device, const char *label) {
    // E-21-A: Invalid parameters or fork failed
    // E-21-B: Tool not available
    // E-21-C: Format operation failed or timed out (30 seconds)

    if (!device) {
        return -1;  // E-21-A: Invalid device parameter
    }

    // Validate that mkfs.vfat is available before attempting format
    if (!fs_check_tool_available("mkfs.vfat")) {
        return -1;  // E-21-B: Tool not found
    }

    const char *actual_label = label ? label : "BOOT";

    // Use fork/execve to avoid shell injection vulnerability
    // Pass arguments as array instead of interpolating into shell command
    pid_t pid = fork();
    if (pid < 0) {
        return -1;  // E-21-A: fork failed
    }

    if (pid == 0) {
        // Child process: execute mkfs.vfat with timeout
        // timeout 30 mkfs.vfat -n LABEL /dev/sdX1
        char *argv[] = {
            (char *)"timeout",
            (char *)"30",
            (char *)"mkfs.vfat",
            (char *)"-n",
            (char *)actual_label,
            (char *)device,
            NULL
        };
        execve("/usr/bin/timeout", argv, environ);
        // If execve fails, exit with error
        exit(127);
    } else {
        // Parent process: wait for child to complete
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            return -1;  // E-21-A: waitpid failed
        }

        // Properly decode wait status
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            // timeout returns 124 if process times out
            if (exit_code == 124) {
                return -1;  // E-21-C: timeout (30 seconds exceeded)
            }
            if (exit_code != 0) {
                return -1;  // E-21-C: mkfs.vfat returned error
            }
        } else if (WIFSIGNALED(status)) {
            return -1;  // E-21-C: Process killed by signal
        }
    }

    return 0;
}

int fs_format_ntfs(const char *device, const char *label) {
    // E-21-A: Invalid parameters or fork failed
    // E-21-B: Tool not available
    // E-21-C: Format operation failed or timed out (30 seconds)

    if (!device) {
        return -1;  // E-21-A: Invalid device parameter
    }

    // Validate that mkfs.ntfs is available before attempting format
    if (!fs_check_tool_available("mkfs.ntfs")) {
        return -1;  // E-21-B: Tool not found
    }

    const char *actual_label = label ? label : "WINDOWS";

    // Use fork/execve to avoid shell injection vulnerability
    // Pass arguments as array instead of interpolating into shell command
    pid_t pid = fork();
    if (pid < 0) {
        return -1;  // E-21-A: fork failed
    }

    if (pid == 0) {
        // Child process: execute mkfs.ntfs with timeout
        // timeout 30 mkfs.ntfs -f -L LABEL /dev/sdX2
        char *argv[] = {
            (char *)"timeout",
            (char *)"30",
            (char *)"mkfs.ntfs",
            (char *)"-f",
            (char *)"-L",
            (char *)actual_label,
            (char *)device,
            NULL
        };
        execve("/usr/bin/timeout", argv, environ);
        // If execve fails, exit with error
        exit(127);
    } else {
        // Parent process: wait for child to complete
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            return -1;  // E-21-A: waitpid failed
        }

        // Properly decode wait status
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            // timeout returns 124 if process times out
            if (exit_code == 124) {
                return -1;  // E-21-C: timeout (30 seconds exceeded)
            }
            if (exit_code != 0) {
                return -1;  // E-21-C: mkfs.ntfs returned error
            }
        } else if (WIFSIGNALED(status)) {
            return -1;  // E-21-C: Process killed by signal
        }
    }

    return 0;
}

int fs_check_tool_available(const char *tool_name) {
    if (!tool_name) {
        return 0;  // Invalid input, tool not available
    }

    // Use fork/execve to check if tool is available via 'which' command
    // Avoids shell injection and properly interprets exit codes
    pid_t pid = fork();
    if (pid < 0) {
        return 0;  // Fork failed, assume tool not available
    }

    if (pid == 0) {
        // Child process: execute 'which' to check if tool exists
        char *argv[] = {
            (char *)"which",
            (char *)tool_name,
            NULL
        };
        execve("/usr/bin/which", argv, environ);
        // If execve fails, exit with error
        exit(127);
    } else {
        // Parent process: wait for child and check exit status
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            return 0;  // waitpid failed, assume tool not available
        }

        // Properly decode wait status
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code == 0) {
                return 1;  // Tool is available
            }
        }
    }

    return 0;  // Tool not available
}
