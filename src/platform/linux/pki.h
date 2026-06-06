#ifndef WINAFI_PKI_H
#define WINAFI_PKI_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Parse the .sbat section from a PE/COFF binary file.
   pe_path: path to the PE binary (EFI file)
   out_buf: caller-provided buffer for the raw .sbat section content
   out_size: size of out_buf in bytes
   Returns number of bytes written to out_buf (0 if no .sbat section),
   -1 on error (file not found, not a PE, etc.) */
int pki_read_sbat(const char *pe_path, char *out_buf, size_t out_size);

/* Parse SBAT CSV content and find the generation for a named component.
   sbat_content: raw SBAT section bytes (NUL-terminated or length-bounded)
   sbat_len: length of sbat_content in bytes
   component: component name to search (e.g. "shim", "grub")
   Returns the generation number (>= 1) if found, 0 if not found, -1 on parse error. */
int pki_sbat_get_generation(const char *sbat_content, size_t sbat_len,
                             const char *component);

/* Check if a PE binary has an Authenticode signature (WIN_CERTIFICATE table entry).
   Returns 1 if signed, 0 if unsigned, -1 on error (file not found, not a PE). */
int pki_is_signed(const char *pe_path);

#ifdef __cplusplus
}
#endif
#endif
