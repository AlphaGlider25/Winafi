#include "sbat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

/**
 * Validate SBAT section in EFI binary
 *
 * Implementation: Check for ".sbat" section in ELF header by:
 * 1. Verifying ELF magic number (0x7f 'E' 'L' 'F')
 * 2. Reading section header string table
 * 3. Iterating through section headers to find ".sbat"
 */
int sbat_validate(const char *efi_path) {
    if (!efi_path) {
        return -1;
    }

    int fd = open(efi_path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    // Read and verify ELF header
    unsigned char elf_header[20];
    if (read(fd, elf_header, sizeof(elf_header)) != sizeof(elf_header)) {
        close(fd);
        return -1;
    }

    // Verify ELF magic number
    if (elf_header[0] != 0x7f || elf_header[1] != 'E' ||
        elf_header[2] != 'L' || elf_header[3] != 'F') {
        close(fd);
        return -1;  // Not an ELF file
    }

    // Determine if 32-bit or 64-bit
    int is_64bit = (elf_header[4] == 2);  // ELFCLASS64 = 2

    if (!is_64bit) {
        // For 32-bit, we need to read the 32-bit ELF header
        Elf32_Ehdr ehdr32;
        lseek(fd, 0, SEEK_SET);
        if (read(fd, &ehdr32, sizeof(ehdr32)) != sizeof(ehdr32)) {
            close(fd);
            return -1;
        }

        // Simple validation: check if shstrndx is reasonable
        if (ehdr32.e_shstrndx >= ehdr32.e_shnum || ehdr32.e_shoff == 0) {
            close(fd);
            return 0;  // No SBAT section found
        }

        // For minimal implementation, we just check basic validity
        // A more complete implementation would iterate through sections
        close(fd);
        return 0;  // Section table present but SBAT not explicitly found
    } else {
        // Read 64-bit ELF header
        Elf64_Ehdr ehdr64;
        lseek(fd, 0, SEEK_SET);
        if (read(fd, &ehdr64, sizeof(ehdr64)) != sizeof(ehdr64)) {
            close(fd);
            return -1;
        }

        // Validate section header table offset and count
        if (ehdr64.e_shoff == 0 || ehdr64.e_shnum == 0) {
            close(fd);
            return 0;  // No section headers
        }

        if (ehdr64.e_shstrndx >= ehdr64.e_shnum) {
            close(fd);
            return 0;  // Invalid string table index
        }

        // Read section header string table offset
        Elf64_Shdr shdr_strtab;
        off_t strtab_offset = ehdr64.e_shoff + (ehdr64.e_shstrndx * ehdr64.e_shentsize);

        if (lseek(fd, strtab_offset, SEEK_SET) < 0) {
            close(fd);
            return -1;
        }

        if (read(fd, &shdr_strtab, sizeof(shdr_strtab)) != sizeof(shdr_strtab)) {
            close(fd);
            return -1;
        }

        // Read the string table into memory (limit to 64KB for safety)
        size_t strtab_size = shdr_strtab.sh_size;
        if (strtab_size > 65536) {
            strtab_size = 65536;
        }

        char *strtab = (char *)malloc(strtab_size);
        if (!strtab) {
            close(fd);
            return -1;
        }

        if (lseek(fd, shdr_strtab.sh_offset, SEEK_SET) < 0) {
            free(strtab);
            close(fd);
            return -1;
        }

        if ((size_t)read(fd, strtab, strtab_size) < strtab_size) {
            free(strtab);
            close(fd);
            return 0;
        }

        // Iterate through section headers looking for ".sbat"
        int found_sbat = 0;
        for (size_t i = 0; i < ehdr64.e_shnum; i++) {
            Elf64_Shdr shdr;
            off_t shdr_offset = ehdr64.e_shoff + (i * ehdr64.e_shentsize);

            if (lseek(fd, shdr_offset, SEEK_SET) < 0) {
                break;
            }

            if (read(fd, &shdr, sizeof(shdr)) != sizeof(shdr)) {
                break;
            }

            // Check section name
            if (shdr.sh_name < strtab_size) {
                const char *section_name = strtab + shdr.sh_name;
                if (strcmp(section_name, ".sbat") == 0) {
                    found_sbat = 1;
                    break;
                }
            }
        }

        free(strtab);
        close(fd);
        return found_sbat ? 1 : 0;
    }
}
