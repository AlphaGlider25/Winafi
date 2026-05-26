#ifndef RUFUS_WIMBOOT_H
#define RUFUS_WIMBOOT_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Detects if a mounted ISO at iso_mount_path contains an install.wim file.
   Searches in: /sources/install.wim and /sources/install.esd
   Returns 1 if found, 0 if not found, -1 on error (bad path). */
int wimboot_detect(const char *iso_mount_path);

/* Returns the path to install.wim (or install.esd) within iso_mount_path.
   Fills out_path with the found path (up to out_size bytes, NUL-terminated).
   Returns 1 if found and out_path filled, 0 if not found, -1 on error.
   Caller provides out_path buffer. */
int wimboot_find_wim(const char *iso_mount_path, char *out_path, size_t out_size);

/* Sets up wimboot on the target USB partition mounted at mount_point.
   iso_mount_path: where the source ISO is mounted
   mount_point: where the target USB partition is mounted
   wimboot_efi_src: path to wimboot.efi to copy (may be NULL to skip copying wimboot.efi)

   Actions:
   - Creates /EFI/boot/ on mount_point if not present
   - Copies wimboot.efi to mount_point/EFI/boot/bootx64.efi (if wimboot_efi_src non-NULL)
   - Copies /sources/boot.wim from iso_mount_path to mount_point/sources/boot.wim
   - Creates mount_point/sources/ directory if needed

   Returns 0 on success, -1 on error. */
int wimboot_setup(const char *iso_mount_path,
                  const char *mount_point,
                  const char *wimboot_efi_src);

/* Set up wimboot.efi for UEFI boot:
 * - Copies wimboot_efi_path to <mount_point>/EFI/Boot/bootx64.efi
 * - Creates /EFI/Boot/ directory if absent
 * Returns 0 on success, -1 on error. */
int wimboot_setup_uefi(const char *mount_point, const char *wimboot_efi_path);

/* Set up wimboot for BIOS boot:
 * - Copies wimboot_bios_path to <mount_point>/wimboot
 * Returns 0 on success, -1 on error. */
int wimboot_setup_bios(const char *mount_point, const char *wimboot_bios_path);

/* Returns the default system asset path for wimboot.efi:
 * /usr/share/winify/assets/wimboot.efi  (or from build tree).
 * Writes into buf[n]. Returns buf, or NULL if not found. */
const char *wimboot_find_asset(char *buf, size_t n);

#ifdef __cplusplus
}
#endif
#endif
