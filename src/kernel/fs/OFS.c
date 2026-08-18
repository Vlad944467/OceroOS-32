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

#define SECTOR_SIZE 512
#define MAX_NAME 32
#define MAGIC "OCEROFS"

typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t total_sectors;
    uint32_t root_start;
    uint32_t root_size;
    uint32_t data_start;
} __attribute__((packed)) superblock_t;

typedef struct {
    char name[MAX_NAME];
    uint32_t size;
    uint32_t first_sector;
    uint32_t flags;  
} __attribute__((packed)) entry_t;

extern void ata_read_sector(uint32_t lba, uint8_t* buffer);
extern void ata_write_sector(uint32_t lba, uint8_t* buffer);
extern void print(const char* str, int color);
extern void print_int(int num, int color);
extern int bg_color; 

static superblock_t sb;
static int ready = 0;

int str_cmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

void str_cpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

int ofs_init(uint32_t start) {
    uint8_t buf[SECTOR_SIZE];
    ata_read_sector(start, buf);

    superblock_t* s = (superblock_t*)buf;
    if (str_cmp(s->magic, MAGIC) != 0) {
        print("OFS: Not found\n", 0x0C);
        return -1;
    }

    sb = *s;
    ready = 1;
    print("OFS: Ready\n", 0x0A);
    return 0;
}

int ofs_find(const char* name, entry_t* out) {
    if (!ready) return -1;

    uint8_t buf[SECTOR_SIZE * 4];
    for (int i = 0; i < 4; i++) {
        ata_read_sector(sb.root_start + i, buf + i * SECTOR_SIZE);
    }

    entry_t* entries = (entry_t*)buf;
    for (int i = 0; i < sb.root_size; i++) {
        if (entries[i].name[0] && str_cmp(entries[i].name, name) == 0) {
            *out = entries[i];
            return 0;
        }
    }
    return -1;
}

int ofs_read(const char* name, uint8_t* buffer, uint32_t max_size) {
    entry_t e;
    if (ofs_find(name, &e) != 0) {
        print("File not found: ", 0x0C);
        print(name, 0x0C);
        print("\n", 0x0C);
        return -1;
    }

    if (e.size > max_size) {
        print("Buffer too small\n", 0x0C);
        return -1;
    }

    uint32_t sectors = (e.size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; i++) {
        ata_read_sector(e.first_sector + i, buffer + i * SECTOR_SIZE);
    }

    return e.size;
}

int ofs_write(const char* name, uint8_t* buffer, uint32_t size) {
    if (!ready) return -1;

    uint32_t free_sector = sb.data_start;
    uint32_t sectors = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;

    for (uint32_t i = 0; i < sectors; i++) {
        ata_write_sector(free_sector + i, buffer + i * SECTOR_SIZE);
    }

    uint8_t root_buf[SECTOR_SIZE * 4];
    for (int i = 0; i < 4; i++) {
        ata_read_sector(sb.root_start + i, root_buf + i * SECTOR_SIZE);
    }

    entry_t* entries = (entry_t*)root_buf;

    int slot = -1;
    for (int i = 0; i < sb.root_size; i++) {
        if (entries[i].name[0] == '\0') {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        print("Root full\n", 0x0C);
        return -1;
    }

    str_cpy(entries[slot].name, name);
    entries[slot].size = size;
    entries[slot].first_sector = free_sector;
    entries[slot].flags = 0;

    for (int i = 0; i < 4; i++) {
        ata_write_sector(sb.root_start + i, root_buf + i * SECTOR_SIZE);
    }

    return size;
}

void ofs_list(void) {
    if (!ready) {
        print("OFS: Not ready\n", 0x0C);
        return;
    }

    uint8_t buf[SECTOR_SIZE * 4];
    for (int i = 0; i < 4; i++) {
        ata_read_sector(sb.root_start + i, buf + i * SECTOR_SIZE);
    }

    entry_t* entries = (entry_t*)buf;
    print("Files:\n", 0x0F);
    for (int i = 0; i < sb.root_size; i++) {
        if (entries[i].name[0]) {
            print("  ", 0x0F);
            print(entries[i].name, 0x0F);
            print(" (", 0x0F);
            print_int(entries[i].size, 0x0F);
            print(" bytes)\n", 0x0F);
        }
    }
}

void cmd_ls(void) {
    ofs_list();
}

void cmd_cat1(char* args) {
    if (!args || args[0] == '\0') {
        print("Usage: cat <file>\n", 0x0F);
        return;
    }

    uint8_t buf[4096];
    int size = ofs_read(args, buf, sizeof(buf));

    if (size > 0) {
        for (int i = 0; i < size; i++) {
            char str[2] = {buf[i], '\0'};
            print(str, bg_color | 0x0F); 
        }
        print("\n", 0x0F);
    }
}

void cmd_write(char* args) {
    if (!args || args[0] == '\0') {
        print("Usage: write <file> <text>\n", 0x0F);
        return;
    }

    char name[MAX_NAME];
    char text[512];

    int i = 0;
    while (args[i] != ' ' && args[i] && i < MAX_NAME - 1) {
        name[i] = args[i];
        i++;
    }
    name[i] = '\0';

    while (args[i] == ' ') i++;

    int j = 0;
    while (args[i] && j < 511) {
        text[j] = args[i];
        i++;
        j++;
    }
    text[j] = '\n';
    text[j+1] = '\0';

    ofs_write(name, (uint8_t*)text, j + 1);
}

void ofs_format(uint32_t total_sectors) {
    print("Formatting OFS...\n", 0x0F);

    superblock_t s;
    str_cpy(s.magic, MAGIC);
    s.version = 1;
    s.total_sectors = total_sectors;
    s.root_start = 2;
    s.root_size = 64;
    s.data_start = 4;

    ata_write_sector(0, (uint8_t*)&s);

    uint8_t zero[SECTOR_SIZE] = {0};
    for (int i = 0; i < 4; i++) {
        ata_write_sector(s.root_start + i, zero);
    }

    print("OFS: Formatted\n", 0x0A);
}
