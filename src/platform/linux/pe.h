#ifndef WINAFI_PE_H
#define WINAFI_PE_H
#include <stddef.h>
/* Read the contents of a named section from a PE/COFF (PE32/PE32+) image.
 * efi_path: path to the .efi file.
 * section_name: e.g. ".sbat" (max 8 chars per COFF spec).
 * out_buf: receives malloc'd section bytes (caller frees). Set to NULL if absent.
 * out_len: receives section length.
 * Returns 0 if section found, 1 if file is valid PE but section absent,
 *        -1 on error (not a PE file / IO error). */
int pe_read_section(const char *efi_path, const char *section_name,
                    unsigned char **out_buf, size_t *out_len);
#endif
