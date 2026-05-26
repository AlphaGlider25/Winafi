#ifndef SBAT_H
#define SBAT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Validate SBAT (Secure Boot Advanced Targeting) section in EFI binary
 *
 * Performs basic validation by checking for the ".sbat" section in the ELF header
 * of the provided EFI file.
 *
 * @param efi_path Path to the EFI binary file
 * @return 1 if SBAT section is found and valid
 *         0 if SBAT section is not found
 *         -1 on error (file not found, permission denied, etc.)
 */
int sbat_validate(const char *efi_path);

#ifdef __cplusplus
}
#endif

#endif // SBAT_H
