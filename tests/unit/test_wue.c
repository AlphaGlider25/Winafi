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

// Each bypass must be independently controllable (Feature 1).
static void test_generate_xml_individual_bypasses(void) {
    // TPM only: must emit BypassTPMCheck and NOT the others.
    char *xml = wue_generate_xml(WUE_BYPASS_TPM, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(strstr(xml, "BypassTPMCheck") != NULL);
    assert(strstr(xml, "BypassSecureBootCheck") == NULL);
    assert(strstr(xml, "BypassRAMCheck") == NULL);
    assert(strstr(xml, "BypassCPUCheck") == NULL);
    assert(strstr(xml, "BypassStorageCheck") == NULL);
    free(xml);

    // CPU only: BypassCPUCheck present, TPM absent.
    xml = wue_generate_xml(WUE_BYPASS_CPU, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(strstr(xml, "BypassCPUCheck") != NULL);
    assert(strstr(xml, "BypassTPMCheck") == NULL);
    free(xml);

    // Storage only: BypassStorageCheck present.
    xml = wue_generate_xml(WUE_BYPASS_STORAGE, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(strstr(xml, "BypassStorageCheck") != NULL);
    free(xml);

    // All five: every key present.
    xml = wue_generate_xml(WUE_BYPASS_ALL, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(strstr(xml, "BypassTPMCheck") != NULL);
    assert(strstr(xml, "BypassSecureBootCheck") != NULL);
    assert(strstr(xml, "BypassRAMCheck") != NULL);
    assert(strstr(xml, "BypassCPUCheck") != NULL);
    assert(strstr(xml, "BypassStorageCheck") != NULL);
    free(xml);
}

// The bypass autounattend must be placeable at the media ROOT, where Setup reads it.
static void test_inject_autounattend_root(void) {
    char tmpdir[] = "/tmp/wue_root_XXXXXX";
    assert(mkdtemp(tmpdir) != NULL);
    char *xml = wue_generate_xml(WUE_BYPASS_ALL, NULL, WUE_ARCH_X86_64);
    assert(xml != NULL);
    assert(wue_inject_autounattend(xml, tmpdir) == 0);
    free(xml);
    char expected[512];
    snprintf(expected, sizeof(expected), "%s/autounattend.xml", tmpdir);
    assert(access(expected, F_OK) == 0);
    char cleanup[512];
    snprintf(cleanup, sizeof(cleanup), "rm -rf %s", tmpdir);
    system(cleanup);
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
    test_generate_xml_individual_bypasses();
    test_inject_autounattend_root();
    test_generate_xml_no_online_account();
    test_generate_xml_with_username();
    test_generate_xml_zero_flags_returns_null();
    test_inject_xml_to_mount();
    printf("All wue tests passed\n");
    return 0;
}
