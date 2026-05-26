// src/platform/linux/wue.h
#ifndef RUFUS_WUE_H
#define RUFUS_WUE_H

// Flags matching original Rufus UNATTEND_* constants
#define WUE_BYPASS_TPM            0x0001  // Add BypassTPMCheck, BypassSecureBootCheck, BypassRAMCheck
#define WUE_BYPASS_SECUREBOOT     0x0001  // Alias (same flag block)
#define WUE_NO_ONLINE_ACCOUNT     0x0004  // OOBE: skip Microsoft account requirement
#define WUE_NO_DATA_COLLECTION    0x0008  // OOBE: disable diagnostic data
#define WUE_OFFLINE_DRIVES        0x0010  // offlineServicing: disable internal drives
#define WUE_DUPLICATE_LOCALE      0x0020  // OOBE: match locale to ISO language
#define WUE_SET_USER              0x0040  // Create a local account with given username
#define WUE_DISABLE_BITLOCKER     0x0080  // Disable BitLocker during install
#define WUE_SILENT_INSTALL        0x0800  // Suppress install UI (windowsPE pass)
#define WUE_QOL_ENHANCEMENTS      0x1000  // Disable suggested apps, Cortana, Teams, etc.

typedef enum {
    WUE_ARCH_X86_32 = 0,
    WUE_ARCH_X86_64 = 1,
    WUE_ARCH_ARM_64 = 2,
} wue_arch_t;

/* Generate autounattend.xml content for the given flags.
 * username: local account name (used when WUE_SET_USER is set); may be NULL.
 * Returns heap-allocated XML string. Caller must free().
 * Returns NULL if flags == 0 (no customisation needed). */
char *wue_generate_xml(int flags, const char *username, wue_arch_t arch);

/* Write xml to <mount_point>/sources/$OEM$/$$/Panther/unattend.xml,
 * creating parent directories as needed.
 * Returns 0 on success, -1 on error. */
int wue_inject_xml(const char *xml, const char *mount_point);

#endif
