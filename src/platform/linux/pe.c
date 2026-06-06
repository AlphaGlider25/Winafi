#include "pe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static uint16_t rd16(const unsigned char *p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t rd32(const unsigned char *p){return (uint32_t)(p[0]|(p[1]<<8)|(p[2]<<16)|((uint32_t)p[3]<<24));}

int pe_read_section(const char *efi_path, const char *section_name,
                    unsigned char **out_buf, size_t *out_len) {
    if (out_buf) *out_buf = NULL;
    if (out_len) *out_len = 0;
    if (!efi_path || !section_name) return -1;

    FILE *f = fopen(efi_path, "rb");
    if (!f) return -1;
    unsigned char dos[64];
    if (fread(dos, 1, 64, f) != 64 || dos[0] != 'M' || dos[1] != 'Z') { fclose(f); return -1; }
    uint32_t e_lfanew = rd32(dos + 0x3C);
    if (fseek(f, e_lfanew, SEEK_SET) != 0) { fclose(f); return -1; }
    unsigned char pehdr[24];                       // PE sig(4) + COFF header(20)
    if (fread(pehdr, 1, 24, f) != 24 ||
        memcmp(pehdr, "PE\0\0", 4) != 0) { fclose(f); return -1; }
    uint16_t num_sections   = rd16(pehdr + 6);
    uint16_t opt_hdr_size   = rd16(pehdr + 20);
    long sec_table = (long)e_lfanew + 24 + opt_hdr_size;  // sections follow optional header
    if (fseek(f, sec_table, SEEK_SET) != 0) { fclose(f); return -1; }

    char want[9] = {0};
    strncpy(want, section_name, 8);
    for (uint16_t i = 0; i < num_sections; i++) {
        unsigned char sh[40];                      // IMAGE_SECTION_HEADER
        if (fread(sh, 1, 40, f) != 40) { fclose(f); return -1; }
        char name[9] = {0}; memcpy(name, sh, 8);
        if (strncmp(name, want, 8) == 0) {
            uint32_t vsize = rd32(sh + 8);
            uint32_t rsize = rd32(sh + 16);
            uint32_t roff  = rd32(sh + 20);
            uint32_t n = vsize && vsize < rsize ? vsize : rsize;  // trim padding
            unsigned char *b = malloc(n ? n : 1);
            if (!b) { fclose(f); return -1; }
            if (n && (fseek(f, roff, SEEK_SET) != 0 || fread(b, 1, n, f) != n)) {
                free(b); fclose(f); return -1;
            }
            fclose(f);
            if (out_buf) *out_buf = b; else free(b);
            if (out_len) *out_len = n;
            return 0;
        }
    }
    fclose(f);
    return 1;   // valid PE, section not present
}
