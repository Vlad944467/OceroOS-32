
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

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char value);
extern uint16_t inw(uint16_t port);
extern void outw(uint16_t port, uint16_t value);

extern void print(const char* str, int color);
extern void print_int(int num, int color);

void ata_wait() {
    int timeout = 10000000;
    while (timeout-- && ((inb(0x1F7) & 0xC0) != 0x40));
    if (timeout == 0) {
        print("ATA wait timeout\n", 0x0C);
    }
}

void ata_wait_bsy() {
    int timeout = 10000000;
    while (timeout-- && (inb(0x1F7) & 0x80));
    if (timeout == 0) {
        print("ATA busy timeout\n", 0x0C);
    }
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_bsy();
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);
    ata_wait();
    for (int i = 0; i < 256; i++) {
        ((uint16_t*)buffer)[i] = inw(0x1F0);
    }
}

void ata_read_sectors(uint32_t lba, uint32_t count, uint8_t* buffer) {
    for (uint32_t i = 0; i < count; i++) {
        ata_read_sector(lba + i, buffer + i * 512);
    }
}

void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_bsy();
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);
    ata_wait();
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ((uint16_t*)buffer)[i]);
    }
    ata_wait();
}

void ata_write_sectors(uint32_t lba, uint32_t count, uint8_t* buffer) {
    for (uint32_t i = 0; i < count; i++) {
        ata_write_sector(lba + i, buffer + i * 512);
    }
}

int ata_detect(int port) {
    outb(port + 6, 0xA0);
    outb(port + 2, 0xEC);
    int timeout = 10000000;
    while (timeout-- && ((inb(port + 7) & 0x80) == 0));
    if (timeout == 0) return 0;
    if (inb(port + 7) == 0x00) return 0;
    return 1;
}

void ata_get_model(int port, char *model) {
    uint16_t buffer[256];
    outb(port + 6, 0xA0);
    outb(port + 2, 0xEC);
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(port);
    }
    for (int i = 0; i < 20; i++) {
        model[i * 2] = buffer[27 + i] >> 8;
        model[i * 2 + 1] = buffer[27 + i] & 0xFF;
    }
    model[40] = '\0';
    for (int i = 0; i < 40; i++) {
        if (model[i] == ' ') {
            for (int j = i; j < 40; j++) {
                model[j] = model[j + 1];
            }
        }
    }
}

int ata_ready() {
    return (inb(0x1F7) & 0x40) != 0;
}

void ata_reset() {
    outb(0x1F6, 0xE0);
    int timeout = 10000000;
    while (timeout-- && ((inb(0x1F7) & 0x80) == 0));
}
