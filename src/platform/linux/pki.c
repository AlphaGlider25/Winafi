#define _GNU_SOURCE
#include "pki.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* --- Little-endian helpers --- */

static int read_u16_le(FILE *f, uint16_t *out)
{
    unsigned char b[2];
    if (fread(b, 1, 2, f) != 2)
        return -1;
    *out = (uint16_t)(b[0] | (b[1] << 8));
    return 0;
}

static int read_u32_le(FILE *f, uint32_t *out)
{
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4)
        return -1;
    *out = (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    return 0;
}

/* Open a PE file and position the file pointer just after the COFF header,
   returning NumberOfSections and SizeOfOptionalHeader.
   Also sets *pe_offset to where the PE signature starts.
   Returns 0 on success, -1 on error. */
static int open_pe(FILE *f,
                   uint32_t *pe_offset_out,
                   uint16_t *num_sections_out,
                   uint16_t *opt_hdr_size_out)
{
    /* DOS signature */
    unsigned char dos_sig[2];
    if (fread(dos_sig, 1, 2, f) != 2)
        return -1;
    if (dos_sig[0] != 0x4D || dos_sig[1] != 0x5A) /* "MZ" */
        return -1;

    /* PE offset at 0x3C */
    if (fseek(f, 0x3C, SEEK_SET) != 0)
        return -1;
    uint32_t pe_offset;
    if (read_u32_le(f, &pe_offset) != 0)
        return -1;

    /* PE signature */
    if (fseek(f, (long)pe_offset, SEEK_SET) != 0)
        return -1;
    unsigned char pe_sig[4];
    if (fread(pe_sig, 1, 4, f) != 4)
        return -1;
    if (pe_sig[0] != 'P' || pe_sig[1] != 'E' || pe_sig[2] != 0 || pe_sig[3] != 0)
        return -1;

    /* COFF header: Machine(2), NumberOfSections(2), TimeDateStamp(4),
       PointerToSymbolTable(4), NumberOfSymbols(4), SizeOfOptionalHeader(2),
       Characteristics(2) */
    uint16_t machine, num_sections, size_of_opt;
    if (read_u16_le(f, &machine) != 0)
        return -1;
    if (read_u16_le(f, &num_sections) != 0)
        return -1;
    /* skip TimeDateStamp, PointerToSymbolTable, NumberOfSymbols (12 bytes) */
    if (fseek(f, 12, SEEK_CUR) != 0)
        return -1;
    if (read_u16_le(f, &size_of_opt) != 0)
        return -1;
    /* skip Characteristics (2 bytes) */
    if (fseek(f, 2, SEEK_CUR) != 0)
        return -1;

    if (pe_offset_out)    *pe_offset_out    = pe_offset;
    if (num_sections_out) *num_sections_out = num_sections;
    if (opt_hdr_size_out) *opt_hdr_size_out = size_of_opt;
    return 0;
}

/* ---- pki_read_sbat ---- */

int pki_read_sbat(const char *pe_path, char *out_buf, size_t out_size)
{
    if (!pe_path || !out_buf || out_size == 0)
        return -1;

    FILE *f = fopen(pe_path, "rb");
    if (!f)
        return -1;

    uint32_t pe_offset;
    uint16_t num_sections, opt_hdr_size;
    if (open_pe(f, &pe_offset, &num_sections, &opt_hdr_size) != 0) {
        fclose(f);
        return -1;
    }

    /* Current position: start of optional header.
       We need to skip it to reach the section table. */
    long opt_hdr_start = ftell(f);
    if (opt_hdr_start < 0) {
        fclose(f);
        return -1;
    }

    /* Seek past optional header to section table */
    if (fseek(f, opt_hdr_start + opt_hdr_size, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    /* Section table: each entry is 40 bytes.
       Fields:
         Name[8]             char[8]
         VirtualSize         uint32
         VirtualAddress      uint32
         SizeOfRawData       uint32
         PointerToRawData    uint32
         (remaining 16 bytes) */
    for (uint16_t i = 0; i < num_sections; i++) {
        char name[9] = {0};
        uint32_t virtual_size, virtual_addr, raw_size, raw_offset;

        if (fread(name, 1, 8, f) != 8) {
            fclose(f);
            return -1;
        }
        if (read_u32_le(f, &virtual_size) != 0 ||
            read_u32_le(f, &virtual_addr) != 0 ||
            read_u32_le(f, &raw_size)     != 0 ||
            read_u32_le(f, &raw_offset)   != 0) {
            fclose(f);
            return -1;
        }
        /* skip remaining 16 bytes of section header */
        if (fseek(f, 16, SEEK_CUR) != 0) {
            fclose(f);
            return -1;
        }

        if (strncmp(name, ".sbat", 8) == 0) {
            /* Found .sbat section */
            if (raw_size == 0 || raw_offset == 0) {
                /* section exists but has no raw data */
                out_buf[0] = '\0';
                fclose(f);
                return 0;
            }

            if (fseek(f, (long)raw_offset, SEEK_SET) != 0) {
                fclose(f);
                return -1;
            }

            size_t to_read = raw_size < (out_size - 1) ? raw_size : (out_size - 1);
            size_t nread = fread(out_buf, 1, to_read, f);
            out_buf[nread] = '\0';
            fclose(f);
            return (int)nread;
        }
    }

    /* No .sbat section found */
    out_buf[0] = '\0';
    fclose(f);
    return 0;
}

/* ---- pki_sbat_get_generation ---- */

int pki_sbat_get_generation(const char *sbat_content, size_t sbat_len,
                             const char *component)
{
    if (!sbat_content || !component)
        return -1;

    /* Make a mutable copy */
    char *buf = malloc(sbat_len + 1);
    if (!buf)
        return -1;
    memcpy(buf, sbat_content, sbat_len);
    buf[sbat_len] = '\0';

    int result = 0;
    char *saveptr = NULL;
    char *line = strtok_r(buf, "\n", &saveptr);
    while (line) {
        /* Skip leading/trailing \r */
        size_t llen = strlen(line);
        if (llen > 0 && line[llen - 1] == '\r')
            line[--llen] = '\0';

        if (llen == 0) {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        /* Extract first two comma-separated fields */
        char *comma1 = strchr(line, ',');
        if (!comma1) {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }
        *comma1 = '\0';
        char *field_component = line;
        char *rest = comma1 + 1;

        char *comma2 = strchr(rest, ',');
        if (comma2)
            *comma2 = '\0';
        char *field_gen = rest;

        if (strcmp(field_component, component) == 0) {
            result = atoi(field_gen);
            free(buf);
            return result;
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(buf);
    return 0; /* not found */
}

/* ---- pki_is_signed ---- */

int pki_is_signed(const char *pe_path)
{
    if (!pe_path)
        return -1;

    FILE *f = fopen(pe_path, "rb");
    if (!f)
        return -1;

    uint32_t pe_offset;
    uint16_t num_sections, opt_hdr_size;
    if (open_pe(f, &pe_offset, &num_sections, &opt_hdr_size) != 0) {
        fclose(f);
        return -1;
    }

    if (opt_hdr_size < 4) {
        /* No optional header — cannot be signed */
        fclose(f);
        return 0;
    }

    /* Read optional header magic */
    long opt_hdr_start = ftell(f);
    if (opt_hdr_start < 0) {
        fclose(f);
        return -1;
    }

    uint16_t magic;
    if (read_u16_le(f, &magic) != 0) {
        fclose(f);
        return -1;
    }

    /* DataDirectory[4] = Security directory (Authenticode)
       For PE32  (0x010B): DataDirectory starts at offset 96 from opt header start
       For PE32+ (0x020B): DataDirectory starts at offset 112 from opt header start */
    long dd_offset;
    if (magic == 0x010B)       /* PE32 */
        dd_offset = opt_hdr_start + 96;
    else if (magic == 0x020B)  /* PE32+ */
        dd_offset = opt_hdr_start + 112;
    else {
        fclose(f);
        return -1;
    }

    /* DataDirectory[4]: skip 4 entries (each 8 bytes) = 32 bytes */
    if (fseek(f, dd_offset + 4 * 8, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    uint32_t sec_va, sec_size;
    if (read_u32_le(f, &sec_va)   != 0 ||
        read_u32_le(f, &sec_size) != 0) {
        fclose(f);
        return -1;
    }

    fclose(f);

    if (sec_va != 0 && sec_size != 0)
        return 1; /* signed */
    return 0;     /* unsigned */
}
