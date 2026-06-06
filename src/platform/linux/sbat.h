#ifndef SBAT_H
#define SBAT_H
#ifdef __cplusplus
extern "C" {
#endif
/* Validate the SBAT section of a UEFI PE/COFF binary.
 * @return 1 if a non-empty ".sbat" section is present (CSV metadata),
 *         0 if the file is a valid PE but has no .sbat section,
 *        -1 on error (not a PE file / IO error). */
int sbat_validate(const char *efi_path);
#ifdef __cplusplus
}
#endif
#endif // SBAT_H
