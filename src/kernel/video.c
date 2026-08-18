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

#define VIDEO_MEMORY 0xB8000

extern int bg_color;
extern char current_dir[32];

void outb(unsigned short port, unsigned char value);
void print(const char* str, int color);

void set_small_font() {
    outb(0x3CE, 0x09);
    outb(0x3CF, 0x00);
}

void putchar(char c, int color) {
    static int cursor = 0;
    char* video = (char*) VIDEO_MEMORY;
    if (c == '\n') {
        cursor += 80 - (cursor % 80);
    } else {
        video[cursor * 2] = c;
        video[cursor * 2 + 1] = color;
        cursor++;
    }
    if (cursor >= 80 * 25) {
        for (int i = 80; i < 80 * 25; i++) {
            video[(i - 80) * 2] = video[i * 2];
            video[(i - 80) * 2 + 1] = video[i * 2 + 1];
        }
        for (int i = 80 * 24; i < 80 * 25; i++) {
            video[i * 2] = ' ';
            video[i * 2 + 1] = color;
        }
        cursor -= 80;
    }
}

void print(const char* str, int color) {
    while (*str) putchar(*str++, color);
}

void print_int(int num, int color) {
    if (num == 0) { putchar('0', color); return; }
    if (num < 0) { putchar('-', color); num = -num; }
    char buf[16];
    int i = 0;
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) putchar(buf[--i], color);
}

void clear_screen() {
    char* video = (char*)0xB8000;
    for (int i = 0; i < 80; i++) video[i * 2] = ' ', video[i * 2 + 1] = 0x17;
    for (int i = 80; i < 80 * 25; i++) video[i * 2] = ' ', video[i * 2 + 1] = bg_color | 0x0F;
    print("\n", bg_color | 0x0F);
}

void gotoxy(int x, int y) {
    int pos = y * 80 + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void update_prompt() {
    print(current_dir, bg_color | 0x02);
    print("> ", bg_color | 0x02);
}
