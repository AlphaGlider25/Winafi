#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// Simple hash table for error codes
#define MAX_ERRORS 100
#define MAX_CODE_LEN 8
#define MAX_MSG_LEN 256

typedef struct {
    char code[MAX_CODE_LEN];
    char message[MAX_MSG_LEN];
} error_entry_t;

static error_entry_t error_table[MAX_ERRORS];
static int error_count = 0;

// Helper function to trim trailing whitespace from a string
static void trim_end(char *str) {
    if (!str) return;
    size_t len = strlen(str);
    while (len > 0 && isspace(str[len - 1])) {
        str[--len] = '\0';
    }
}

int error_init(void) {
    FILE *fp = fopen("src/errors.txt", "r");
    if (!fp) {
        // Try alternative path relative to binary
        fp = fopen("../src/errors.txt", "r");
    }
    if (!fp) {
        // Try when installed
        fp = fopen("/usr/share/winify/errors.txt", "r");
    }
    if (!fp) {
        perror("Could not find errors.txt");
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp) && error_count < MAX_ERRORS) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char code[MAX_CODE_LEN];
        char category[128];
        char message[MAX_MSG_LEN];
        char details[256];

        int parsed = sscanf(line, "%7[^|] | %127[^|] | %255[^|] | %255[^\n]",
                           code, category, message, details);
        if (parsed >= 3) {
            // Trim whitespace
            trim_end(code);
            trim_end(message);

            snprintf(error_table[error_count].code,
                     sizeof(error_table[error_count].code), "%s", code);
            snprintf(error_table[error_count].message,
                     sizeof(error_table[error_count].message), "%s", message);
            error_count++;
        }
    }

    fclose(fp);
    return error_count > 0 ? 0 : -1;
}

void error_cleanup(void) {
    // No-op in current implementation
}

const char *error_lookup(const char *code) {
    if (!code) return NULL;

    // Note: error_init() must be called first to populate the error table
    // This function does NOT perform initialization to avoid hidden state changes

    for (int i = 0; i < error_count; i++) {
        if (strcmp(error_table[i].code, code) == 0) {
            return error_table[i].message;
        }
    }

    return NULL;
}

char *error_format(const char *code, const char *context) {
    const char *msg = error_lookup(code);
    if (!msg) msg = "Unknown error";

    // Safety margin increased to 64 bytes for format string overhead
    size_t len = strlen(code) + strlen(msg) + (context ? strlen(context) : 0) + 64;
    char *buf = malloc(len);
    if (!buf) {
        // Memory allocation failed, return NULL
        return NULL;
    }

    if (context) {
        snprintf(buf, len, "[%s] %s (%s)", code, msg, context);
    } else {
        snprintf(buf, len, "[%s] %s", code, msg);
    }

    return buf;
}
