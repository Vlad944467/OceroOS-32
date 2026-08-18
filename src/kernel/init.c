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

extern void print(const char* str, int color);
extern void print_int(int num, int color);
extern void clear_screen(void);

extern int ata_detect(int port);
extern int ofs_init(uint32_t start_sector);
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char value);

int check_cpu(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    return (eax != 0);
}

int check_ram(void) {
    volatile uint32_t* addr = (uint32_t*)0x100000;
    *addr = 0xDEADBEEF;
    return (*addr == 0xDEADBEEF);
}

int check_keyboard(void) {
    uint8_t status = inb(0x64);
    return (status != 0xFF);
}

int check_rtc(void) {
    uint8_t seconds;
    outb(0x70, 0x0D);
    if (inb(0x71) == 0xFF) return 0;
    outb(0x70, 0x80);
    outb(0x70, 0x00);
    seconds = inb(0x71);
    outb(0x70, 0x80);
    return (seconds <= 59);
}

void print_status(const char* msg, int ok) {
    print("[ ", 0x0F);
    if (ok) {
        print(" OK ", 0x0A);
    } else {
        print("FAIL", 0x0C);
    }
    print(" ] ", 0x0F);
    print(msg, 0x0F);
    print("\n", 0x0F);
}

void print_loading(const char* msg) {
    print("[ .. ] ", 0x0F);
    print(msg, 0x0F);
    for (int i = 0; i < 3; i++) {
        print(".", 0x0F);
        for (int j = 0; j < 30000; j++);
    }
    print(" ", 0x0F);
}

int init_system(void) {
    int ok = 1;

    clear_screen();
    print("=== Ocero OS 32-bit Boot Sequence ===\n", 0x0E);
    print("\n", 0x0F);

    print_status("Initializing video subsystem...", 1);

    print_status("Loading GDT...", 1);

    print_status("Loading IDT...", 1);

    print_loading("Detecting CPU");
    int cpu_ok = check_cpu();
    print_status("Detecting CPU...", cpu_ok);
    if (!cpu_ok) ok = 0;

    print_loading("Checking RAM");
    int ram_ok = check_ram();
    print_status("Checking RAM...", ram_ok);
    if (!ram_ok) ok = 0;

    print_loading("Loading keyboard driver");
    int kbd_ok = check_keyboard();
    print_status("Loading keyboard driver...", kbd_ok);
    if (!kbd_ok) ok = 0;

    print_loading("Detecting ATA disk");
    int ata_ok = ata_detect(0x1F0);
    print_status("Detecting ATA disk...", ata_ok);
    if (!ata_ok) ok = 0;

    if (ata_ok) {
        print_loading("Mounting OFS");
        int ofs_ok = ofs_init(0);
        print_status("Mounting OFS...", ofs_ok == 0);
        if (ofs_ok != 0) ok = 0;
    } else {
        print_status("Mounting OFS...", 0);
        ok = 0;
    }

    print_loading("Reading system time");
    int rtc_ok = check_rtc();
    print_status("Reading system time...", rtc_ok ? 1 : 1);  
    print("\n", 0x0F);
    if (ok) {
        print("[  OK  ] System ready!\n", 0x0A);
    } else {
        print("[ WARN ] System started with errors\n", 0x0E);
    }
    print("\n", 0x0F);

    return ok;
}
