// tests/unit/test_wue.c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "platform/linux/wue.h"

static void test_generate_xml_bypass_tpm(void) {
    char *xml = wue_generate_xml(WUE_BYPASS_TPM | WUE_BYPASS_SECUREBOOT, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(strstr(xml, "BypassTPMCheck") != NULL);
    assert(strstr(xml, "BypassSecureBootCheck") != NULL);
    assert(strstr(xml, "<unattend") != NULL);
    free(xml);
}

static void test_generate_xml_no_online_account(void) {
    char *xml = wue_generate_xml(WUE_NO_ONLINE_ACCOUNT, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(strstr(xml, "HideOnlineAccountScreens") != NULL || strstr(xml, "SkipMachineOOBE") != NULL);
    free(xml);
}

static void test_generate_xml_with_username(void) {
    char *xml = wue_generate_xml(WUE_SET_USER, "alice", WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(strstr(xml, "alice") != NULL);
    free(xml);
}

static void test_generate_xml_zero_flags_returns_null(void) {
    char *xml = wue_generate_xml(0, NULL, WUE_ARCH_X86_64);
    assert(xml == NULL);
}

static void test_inject_xml_to_mount(void) {
    char tmpdir[] = "/tmp/wue_test_XXXXXX";
    assert(mkdtemp(tmpdir) != NULL);

    char *xml = wue_generate_xml(WUE_BYPASS_TPM, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    int rc = wue_inject_xml(xml, tmpdir);
    assert(rc == 0);
    free(xml);

    // Verify file was written to correct location
    char expected[512];
    snprintf(expected, sizeof(expected),
             "%s/sources/$OEM$/$$/Panther/unattend.xml", tmpdir);
    assert(access(expected, F_OK) == 0);

    // Cleanup: Remove temporary directory and contents
    char cleanup_cmd[512];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", tmpdir);
    system(cleanup_cmd);
}

int main(void) {
    test_generate_xml_bypass_tpm();
    test_generate_xml_no_online_account();
    test_generate_xml_with_username();
    test_generate_xml_zero_flags_returns_null();
    test_inject_xml_to_mount();
    printf("All wue tests passed\n");
    return 0;
}
