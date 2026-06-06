#include "sbat.h"
#include "pe.h"
#include <stdlib.h>

int sbat_validate(const char *efi_path) {
    if (!efi_path) return -1;
    unsigned char *buf = NULL;
    size_t len = 0;
    int r = pe_read_section(efi_path, ".sbat", &buf, &len);
    if (r < 0) { free(buf); return -1; }      // not a PE / IO error
    int present = (r == 0 && len > 0) ? 1 : 0;
    free(buf);
    return present;
}
