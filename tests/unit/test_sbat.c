#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/linux/sbat.h"

static void test_sbat_validate_nonexistent_file(void) {
    // Should return -1 for non-existent file
    assert(sbat_validate("/tmp/nonexistent_efi_xyz_12345.efi") == -1);
}

static void test_sbat_validate_null_path(void) {
    // Should handle NULL input gracefully
    assert(sbat_validate(NULL) == -1);
}

static void test_sbat_validate_invalid_file(void) {
    // Create a temporary file with non-ELF content
    FILE *fp = fopen("/tmp/test_invalid_efi.bin", "wb");
    assert(fp != NULL);

    // Write non-ELF data
    const char *invalid_data = "NOT_AN_ELF_FILE";
    fwrite(invalid_data, 1, strlen(invalid_data), fp);
    fclose(fp);

    // Should return -1 for invalid file (not ELF)
    assert(sbat_validate("/tmp/test_invalid_efi.bin") == -1);

    // Clean up
    remove("/tmp/test_invalid_efi.bin");
}

static void test_sbat_validate_empty_file(void) {
    // Create an empty file
    FILE *fp = fopen("/tmp/test_empty_efi.bin", "wb");
    assert(fp != NULL);
    fclose(fp);

    // Should return -1 for file too small to be ELF
    assert(sbat_validate("/tmp/test_empty_efi.bin") == -1);

    // Clean up
    remove("/tmp/test_empty_efi.bin");
}

int main(void) {
    test_sbat_validate_nonexistent_file();
    test_sbat_validate_null_path();
    test_sbat_validate_invalid_file();
    test_sbat_validate_empty_file();
    printf("All sbat tests passed\n");
    return 0;
}
