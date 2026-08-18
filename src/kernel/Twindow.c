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
#define WHITE_ON_BLACK  0x0F
#define GREEN_ON_BLACK  0x02
#define RED_ON_BLACK    0x04

extern void print(const char* str, int color);
extern void print_int(int num, int color);
extern char read_keyboard(void);
extern void outb(unsigned short port, unsigned char value);
extern void outw(uint16_t port, uint16_t value);
extern int ata_detect(int port);
extern void ata_get_model(int port, char *model);
extern char saved_note[4096];
extern int bg_color;
extern int mystrlen(const char* s);

static char saved_bg[80 * 25 * 2];

void save_screen_area(int x, int y, int w, int h) {
    char* video = (char*)VIDEO_MEMORY;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int pos = ((y + j) * 80 + x + i) * 2;
            int saved_pos = (j * w + i) * 2;
            saved_bg[saved_pos] = video[pos];
            saved_bg[saved_pos + 1] = video[pos + 1];
        }
    }
}

void restore_screen_area(int x, int y, int w, int h) {
    char* video = (char*)VIDEO_MEMORY;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int pos = ((y + j) * 80 + x + i) * 2;
            int saved_pos = (j * w + i) * 2;
            video[pos] = saved_bg[saved_pos];
            video[pos + 1] = saved_bg[saved_pos + 1];
        }
    }
}

void draw_window_red(int x, int y, int w, int h, char* title, char* content) {
    char* video = (char*)VIDEO_MEMORY;
    int pos;
    int bg = 0xF1;

    save_screen_area(x, y, w, h);

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            pos = ((y + j) * 80 + x + i) * 2;
            video[pos] = ' ';
            video[pos + 1] = bg;
        }
    }

    pos = (y * 80 + x) * 2;
    video[pos] = 0xDA;
    video[pos + 1] = bg | 0x01;
    for (int i = 1; i < w - 1; i++) {
        pos = (y * 80 + x + i) * 2;
        video[pos] = 0xC4;
        video[pos + 1] = bg | 0x01;
    }
    pos = (y * 80 + x + w - 1) * 2;
    video[pos] = 0xBF;
    video[pos + 1] = bg | 0x01;

    for (int j = 1; j < h - 1; j++) {
        pos = ((y + j) * 80 + x) * 2;
        video[pos] = 0xB3;
        video[pos + 1] = bg | 0x01;
        pos = ((y + j) * 80 + x + w - 1) * 2;
        video[pos] = 0xB3;
        video[pos + 1] = bg | 0x01;
    }

    pos = ((y + h - 1) * 80 + x) * 2;
    video[pos] = 0xC0;
    video[pos + 1] = bg | 0x01;
    for (int i = 1; i < w - 1; i++) {
        pos = ((y + h - 1) * 80 + x + i) * 2;
        video[pos] = 0xC4;
        video[pos + 1] = bg | 0x01;
    }
    pos = ((y + h - 1) * 80 + x + w - 1) * 2;
    video[pos] = 0xD9;
    video[pos + 1] = bg | 0x01;

    int tx = x + (w - mystrlen(title)) / 2;
    int ty = y;
    int i = 0;
    while (title[i]) {
        pos = (ty * 80 + tx + i) * 2;
        video[pos] = title[i];
        video[pos + 1] = bg | 0x01;
        i++;
    }

    if (content != NULL) {
        tx = x + 2;
        ty = y + 2;
        i = 0;
        while (content[i]) {
            pos = (ty * 80 + tx + i) * 2;
            video[pos] = content[i];
            video[pos + 1] = bg | 0x01;
            i++;
        }
    }
}

void cmd_windo4(void) {
    int w = 40, h = 10;
    int x = (80 - w) / 2;
    int y = (25 - h) / 2;

    char content[256];
    if (saved_note[0] != '\0') {
        int i = 0;
        while (saved_note[i] && i < 200) {
            content[i] = saved_note[i];
            i++;
        }
        content[i] = '\0';
    } else {
        char* d = content;
        char* msg = "No notes saved. Use 'edit' to create one.";
        while (*msg) *d++ = *msg++;
        *d = '\0';
    }

    draw_window_red(x, y, w, h, " Ocero Notes ", content);
    while (read_keyboard() == 0);
    restore_screen_area(x, y, w, h);
}

void cmd_win_tt(void) {
    char content[512];
    char* d = content;
    char* msg = "Simple window test.";

    while (*msg) *d++ = *msg++;
    *d = '\0';

    draw_window_red(10, 2, 50, 18, " Test ", content);
    while (read_keyboard() == 0);
    restore_screen_area(10, 2, 50, 18);
}

void cmd_win_disk(void) {
    int w = 50, h = 6;
    int x = (80 - w) / 2;
    int y = (25 - h) / 2;

    int ata_ok = ata_detect(0x1F0);
    char* status = ata_ok ? "OK" : "FAIL";

    char model[41];
    if (ata_ok) {
        ata_get_model(0x1F0, model);
    } else {
        char* m = "No disk";
        int i = 0;
        while (*m) model[i++] = *m++;
        model[i] = '\0';
    }

    char content[64];
    char* d = content;
    char* s = "ATA: ";
    while (*s) *d++ = *s++;
    s = status;
    while (*s) *d++ = *s++;
    s = " | Model: ";
    while (*s) *d++ = *s++;
    s = model;
    while (*s) *d++ = *s++;
    *d = '\0';

    draw_window_red(x, y, w, h, " Disk Info ", content);
    while (read_keyboard() == 0);
    restore_screen_area(x, y, w, h);
}

void cmd_settings(void) {
    int w = 40, h = 12;
    int x = (80 - w) / 2;
    int y = (25 - h) / 2;

    char* items[] = {"Reboot", "Shutdown", "Cancel"};
    int selected = 0;
    int num_items = 3;
    char ch;

    while (1) {
        save_screen_area(x, y, w, h);

        char* video = (char*)VIDEO_MEMORY;
        int pos;
        int bg = 0x40;

        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                pos = ((y + j) * 80 + x + i) * 2;
                video[pos] = ' ';
                video[pos + 1] = bg;
            }
        }

        pos = (y * 80 + x) * 2;
        video[pos] = 0xDA;
        video[pos + 1] = bg | 0x0F;
        for (int i = 1; i < w - 1; i++) {
            pos = (y * 80 + x + i) * 2;
            video[pos] = 0xC4;
            video[pos + 1] = bg | 0x0F;
        }
        pos = (y * 80 + x + w - 1) * 2;
        video[pos] = 0xBF;
        video[pos + 1] = bg | 0x0F;

        for (int j = 1; j < h - 1; j++) {
            pos = ((y + j) * 80 + x) * 2;
            video[pos] = 0xB3;
            video[pos + 1] = bg | 0x0F;
            pos = ((y + j) * 80 + x + w - 1) * 2;
            video[pos] = 0xB3;
            video[pos + 1] = bg | 0x0F;
        }

        pos = ((y + h - 1) * 80 + x) * 2;
        video[pos] = 0xC0;
        video[pos + 1] = bg | 0x0F;
        for (int i = 1; i < w - 1; i++) {
            pos = ((y + h - 1) * 80 + x + i) * 2;
            video[pos] = 0xC4;
            video[pos + 1] = bg | 0x0F;
        }
        pos = ((y + h - 1) * 80 + x + w - 1) * 2;
        video[pos] = 0xD9;
        video[pos + 1] = bg | 0x0F;

        char* title = " Settings ";
        int tx = x + (w - mystrlen(title)) / 2;
        int ty = y;
        int i = 0;
        while (title[i]) {
            pos = (ty * 80 + tx + i) * 2;
            video[pos] = title[i];
            video[pos + 1] = bg | 0x0E;
            i++;
        }

        for (int i = 0; i < num_items; i++) {
            tx = x + 5;
            ty = y + 3 + i * 2;

            if (i == selected) {
                pos = ((ty) * 80 + tx) * 2;
                video[pos] = 0x1E;
                video[pos + 1] = bg | 0x0E;
                tx += 2;
            } else {
                pos = ((ty) * 80 + tx) * 2;
                video[pos] = ' ';
                video[pos + 1] = bg | 0x0F;
                pos = ((ty) * 80 + tx + 1) * 2;
                video[pos] = ' ';
                video[pos + 1] = bg | 0x0F;
                tx += 2;
            }

            for (int j = 0; items[i][j]; j++) {
                pos = (ty * 80 + tx + j) * 2;
                if (i == selected) {
                    video[pos] = items[i][j];
                    video[pos + 1] = bg | 0x0E;
                } else {
                    video[pos] = items[i][j];
                    video[pos + 1] = bg | 0x0F;
                }
            }
        }

        char* hint = "1,2,3 - select  Enter - ok";
        tx = x + (w - mystrlen(hint)) / 2;
        ty = y + h - 2;
        i = 0;
        while (hint[i]) {
            pos = (ty * 80 + tx + i) * 2;
            video[pos] = hint[i];
            video[pos + 1] = bg | 0x0F;
            i++;
        }

        ch = read_keyboard();

        if (ch == '1') selected = 0;
        else if (ch == '2') selected = 1;
        else if (ch == '3') selected = 2;
        else if (ch == '\n' || ch == '\r') break;
        else if (ch == 0x1B) { selected = 2; break; }

        restore_screen_area(x, y, w, h);
    }

    restore_screen_area(x, y, w, h);

    if (selected == 0) {
        print("Rebooting...\n", RED_ON_BLACK);
        outb(0x64, 0xFE);
    } else if (selected == 1) {
        print("Shutting down...\n", WHITE_ON_BLACK);
        outw(0x604, 0x2000);
        while (1);
    }
}
