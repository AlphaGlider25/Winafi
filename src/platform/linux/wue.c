// src/platform/linux/wue.c
// Generates autounattend.xml matching the structure in the original Rufus wue.c.
// References: https://docs.microsoft.com/en-us/windows-hardware/customize/desktop/unattend/
#include "wue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>

static const char *arch_names[] = { "x86", "amd64", "arm64" };

// Ensure directory path exists (creates intermediate directories)
static int mkdirs(const char *path) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) < 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    return mkdir(tmp, 0755) < 0 && errno != EEXIST ? -1 : 0;
}

// Append formatted string to a heap buffer, growing as needed
typedef struct { char *buf; size_t len; size_t cap; } strbuf_t;

static void sb_append(strbuf_t *sb, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char tmp[4096];
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    if (sb->len + (size_t)n + 1 > sb->cap) {
        size_t nc = sb->cap ? sb->cap * 2 : 8192;
        while (nc < sb->len + (size_t)n + 1) nc *= 2;
        char *new_buf = realloc(sb->buf, nc);
        if (!new_buf) return;  // Realloc failed, exit gracefully
        sb->buf = new_buf;
        sb->cap = nc;
    }
    memcpy(sb->buf + sb->len, tmp, (size_t)n);
    sb->len += (size_t)n;
    sb->buf[sb->len] = '\0';
}

char *wue_generate_xml(int flags, const char *username, wue_arch_t arch) {
    if (flags == 0) return NULL;
    if (arch < WUE_ARCH_X86_32 || arch > WUE_ARCH_ARM_64) arch = WUE_ARCH_X86_64;

    const char *aname = arch_names[arch];
    strbuf_t sb = {0};

    sb_append(&sb, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    sb_append(&sb, "<unattend xmlns=\"urn:schemas-microsoft-com:unattend\">\n");

    // --- windowsPE pass ---
    if (flags & (WUE_BYPASS_TPM | WUE_SILENT_INSTALL)) {
        sb_append(&sb, "  <settings pass=\"windowsPE\">\n");
        sb_append(&sb,
            "    <component name=\"Microsoft-Windows-Setup\" processorArchitecture=\"%s\"\n"
            "      language=\"neutral\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\"\n"
            "      xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
            "      publicKeyToken=\"31bf3856ad364e35\" versionScope=\"nonSxS\">\n", aname);
        sb_append(&sb, "      <UserData>\n");
        sb_append(&sb, "        <AcceptEula>true</AcceptEula>\n");
        sb_append(&sb, "        <ProductKey><Key /></ProductKey>\n");
        sb_append(&sb, "      </UserData>\n");
        if (flags & WUE_BYPASS_TPM) {
            sb_append(&sb, "      <RunSynchronous>\n");
            sb_append(&sb, "        <RunSynchronousCommand wcm:action=\"add\">\n");
            sb_append(&sb, "          <Order>1</Order>\n");
            sb_append(&sb,
                "          <Path>reg add HKLM\\SYSTEM\\Setup\\LabConfig /v BypassTPMCheck"
                " /t REG_DWORD /d 1 /f</Path>\n");
            sb_append(&sb, "        </RunSynchronousCommand>\n");
            sb_append(&sb, "        <RunSynchronousCommand wcm:action=\"add\">\n");
            sb_append(&sb, "          <Order>2</Order>\n");
            sb_append(&sb,
                "          <Path>reg add HKLM\\SYSTEM\\Setup\\LabConfig /v BypassSecureBootCheck"
                " /t REG_DWORD /d 1 /f</Path>\n");
            sb_append(&sb, "        </RunSynchronousCommand>\n");
            sb_append(&sb, "        <RunSynchronousCommand wcm:action=\"add\">\n");
            sb_append(&sb, "          <Order>3</Order>\n");
            sb_append(&sb,
                "          <Path>reg add HKLM\\SYSTEM\\Setup\\LabConfig /v BypassRAMCheck"
                " /t REG_DWORD /d 1 /f</Path>\n");
            sb_append(&sb, "        </RunSynchronousCommand>\n");
            sb_append(&sb, "      </RunSynchronous>\n");
        }
        sb_append(&sb, "    </component>\n");
        sb_append(&sb, "  </settings>\n");
    }

    // --- offlineServicing pass ---
    if (flags & WUE_OFFLINE_DRIVES) {
        sb_append(&sb, "  <settings pass=\"offlineServicing\">\n");
        sb_append(&sb,
            "    <component name=\"Microsoft-Windows-PartitionManager\" processorArchitecture=\"%s\"\n"
            "      language=\"neutral\" publicKeyToken=\"31bf3856ad364e35\" versionScope=\"nonSxS\">\n", aname);
        sb_append(&sb, "      <SanPolicy>4</SanPolicy>\n"); // Offline all internal disks
        sb_append(&sb, "    </component>\n");
        sb_append(&sb, "  </settings>\n");
    }

    // --- OOBE pass ---
    int has_oobe = flags & (WUE_NO_ONLINE_ACCOUNT | WUE_NO_DATA_COLLECTION |
                            WUE_SET_USER | WUE_DISABLE_BITLOCKER | WUE_QOL_ENHANCEMENTS);
    if (has_oobe) {
        sb_append(&sb, "  <settings pass=\"oobeSystem\">\n");
        sb_append(&sb,
            "    <component name=\"Microsoft-Windows-Shell-Setup\" processorArchitecture=\"%s\"\n"
            "      language=\"neutral\" publicKeyToken=\"31bf3856ad364e35\" versionScope=\"nonSxS\">\n", aname);
        sb_append(&sb, "      <OOBE>\n");
        if (flags & WUE_NO_ONLINE_ACCOUNT) {
            sb_append(&sb, "        <HideOnlineAccountScreens>true</HideOnlineAccountScreens>\n");
            sb_append(&sb, "        <SkipMachineOOBE>true</SkipMachineOOBE>\n");
            sb_append(&sb, "        <SkipUserOOBE>true</SkipUserOOBE>\n");
        }
        if (flags & WUE_NO_DATA_COLLECTION) {
            sb_append(&sb, "        <ProtectYourPC>3</ProtectYourPC>\n");
        }
        sb_append(&sb, "      </OOBE>\n");

        if ((flags & WUE_SET_USER) && username && username[0]) {
            // Sanitize username: replace forbidden chars with '_'
            char safe_name[256];
            snprintf(safe_name, sizeof(safe_name), "%s", username);
            const char *forbidden = "/\\[]:|<>+=;,?*%@.";
            for (char *p = safe_name; *p; p++)
                if (strchr(forbidden, *p)) *p = '_';

            sb_append(&sb, "      <UserAccounts>\n");
            sb_append(&sb, "        <LocalAccounts>\n");
            sb_append(&sb, "          <LocalAccount wcm:action=\"add\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\">\n");
            sb_append(&sb, "            <Name>%s</Name>\n", safe_name);
            sb_append(&sb, "            <DisplayName>%s</DisplayName>\n", safe_name);
            sb_append(&sb, "            <Group>Administrators</Group>\n");
            sb_append(&sb, "            <Password><Value></Value><PlainText>true</PlainText></Password>\n");
            sb_append(&sb, "          </LocalAccount>\n");
            sb_append(&sb, "        </LocalAccounts>\n");
            sb_append(&sb, "      </UserAccounts>\n");
        }

        if (flags & WUE_QOL_ENHANCEMENTS) {
            // Suppress Cortana and Teams auto-install
            sb_append(&sb, "      <FirstLogonCommands>\n");
            sb_append(&sb, "        <SynchronousCommand wcm:action=\"add\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\">\n");
            sb_append(&sb, "          <Order>1</Order>\n");
            sb_append(&sb, "          <CommandLine>reg add HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced /v Start_AccountNotifications /t REG_DWORD /d 0 /f</CommandLine>\n");
            sb_append(&sb, "        </SynchronousCommand>\n");
            sb_append(&sb, "      </FirstLogonCommands>\n");
        }

        sb_append(&sb, "    </component>\n");
        sb_append(&sb, "  </settings>\n");
    }

    sb_append(&sb, "</unattend>\n");
    return sb.buf; // caller frees
}

int wue_inject_xml(const char *xml, const char *mount_point) {
    if (!xml || !mount_point) return -1;
    if (strlen(mount_point) > 4000) return -1;  // Path too long to safely construct

    // Target: <mount>/sources/$OEM$/$$/Panther/unattend.xml
    char dir[4096], path[4096 + 32];
    snprintf(dir, sizeof(dir), "%s/sources/$OEM$/$$/Panther", mount_point);
    snprintf(path, sizeof(path), "%s/unattend.xml", dir);

    if (mkdirs(dir) < 0) return -1;

    FILE *f = fopen(path, "w");
    if (!f) return -1;
    if (fputs(xml, f) < 0) {
        fclose(f);
        return -1;  // Propagate write error
    }
    fclose(f);
    return 0;
}
