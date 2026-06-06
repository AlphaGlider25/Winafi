#ifndef WINAFI_ASSETS_H
#define WINAFI_ASSETS_H
/* Resolve the absolute path of a bundled asset (e.g. "uefi-ntfs/bootx64.efi").
 * Search order:
 *   1) $WINAFI_DATADIR/<rel>
 *   2) <dir-of-executable>/../share/winafi/assets/<rel>   (installed layout)
 *   3) <dir-of-executable>/../src/assets/<rel>            (build-tree layout)
 *   4) ./src/assets/<rel>                                 (run-from-source)
 * Writes the first existing path into out (size out_size).
 * Returns 0 on success, -1 if not found. */
int assets_find(const char *rel, char *out, unsigned long out_size);
#endif
