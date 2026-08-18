/*
 * Copyright (C) 2026 Vlad944467
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>

#define ELF_MAGIC 0x464C457F

extern void print(const char* str, int color);
extern void print_int(int num, int color);
extern void ata_read_sector(uint32_t lba, uint8_t* buffer);

typedef struct {
    uint32_t magic;
    uint8_t  e_class;
    uint8_t  e_data;
    uint8_t  e_version;
    uint8_t  e_osabi;
    uint8_t  e_abiversion;
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version2;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) elf_program_header_t;

int elf_load(uint8_t *elf_data, void **entry_point) {
    elf_header_t *header = (elf_header_t*)elf_data;

    if (header->magic != ELF_MAGIC) {
        print("Not an ELF file\n", 0x0C);
        return -1;
    }

    if (header->e_class != 1) {
        print("Not a 32-bit file\n", 0x0C);
        return -1;
    }

    if (header->e_type != 2) {
        print("Not an executable\n", 0x0C);
        return -1;
    }

    if (header->e_machine != 3) {
        print("Not x86 architecture\n", 0x0C);
        return -1;
    }

    print("ELF OK\n", 0x0A);

    elf_program_header_t *phdr = (elf_program_header_t*)(elf_data + header->e_phoff);

    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type == 1) {  
            uint8_t *dest = (uint8_t*)phdr[i].p_vaddr;
            uint8_t *src = elf_data + phdr[i].p_offset;

            for (int j = 0; j < phdr[i].p_filesz; j++) {
                dest[j] = src[j];
            }

            for (int j = phdr[i].p_filesz; j < phdr[i].p_memsz; j++) {
                dest[j] = 0;
            }

            print("Segment loaded\n", 0x0F);
        }
    }

    *entry_point = (void*)header->e_entry;
    return 0;
}

int elf_run_from_sector(uint32_t sector) {
    uint8_t buffer[32768]; 

    ata_read_sectors(sector, 64, buffer);

    void *entry;
    if (elf_load(buffer, &entry) != 0) {
        print("Failed to load ELF\n", 0x0C);
        return -1;
    }

    print("Entry point: 0x", 0x0F);
    print_int((int)entry, 0x0F);
    print("\n", 0x0F);

    print("Running...\n", 0x0A);

    void (*start)(void) = entry;
    start();

    print("Program finished\n", 0x0F);
    return 0;
}

void cmd_run(char *args) {
    if (!args || args[0] == '\0') {
        print("Usage: run <sector>\n", 0x0F);
        return;
    }

    int sector = 0;
    while (*args >= '0' && *args <= '9') {
        sector = sector * 10 + (*args - '0');
        args++;
    }

    print("Loading from sector ", 0x0F);
    print_int(sector, 0x0F);
    print("\n", 0x0F);

    elf_run_from_sector(sector);
}
