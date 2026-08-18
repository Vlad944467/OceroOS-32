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
#define MAX_HIST 20

#define BLACK_ON_WHITE 0xF0
#define BLUE_ON_WHITE 0xF1
#define BLUE_ON_GRAY 0x72
#define BLACK_ON_GRAY 0x70

#define say(s, c) print(s, c)
#define cls clear_screen()

extern int ofs_init(uint32_t start_sector);
extern void ofs_list(void);
extern void ofs_format(uint32_t total_sectors);
extern void print_status(const char* msg, int ok);
extern void print_loading(const char* msg);
extern int init_system(void);
extern void cmd_win_ls(void);
extern void cmd_win_clock(void);
extern void cmd_windo4(void);
extern void cmd_win_tt(void);
extern void cmd_win_disk(void);
extern void cmd_settings(void);
extern void cmd_h(char *args);

char current_dir[32] = "/home";
char saved_note[4096];
char *history[MAX_HIST];
int hist_count = 0;
int bg_color = 0x00;
int cursor_pos = 0;

void cmd_help(void);
void cmd_clear(void);
void cmd_about(void);
void cmd_ping(void);
void cmd_disks(void);
void cmd_testdisk(void);
void cmd_mode(void);
void cmd_win(void);
void cmd_say(void);
void cmd_icat(void);
void cmd_fetch(void);
void cmd_cpu(void);
void cmd_version(void);
void cmd_kernel(void);
void cmd_cube(void);
void cmd_req(void);
void cmd_window(void);
void cmd_new(void);
void cmd_line(void);
void cmd_8ball(void);
void cmd_notes(void);
void cmd_edit(void);
void cmd_reboot(void);
void cmd_shutdown(void);
void cmd_save(void);
void cmd_load(void);
void cmd_time(char *args);
void cmd_reb(char *args);
void cmd_sh(char *args);
void cmd_h(char *args);
void cmd_hello(char *args);
void cmd_echo(char *args);
void cmd_calc(char *args);
void cmd_sleep(char *args);
void cmd_type(char *args);
void cmd_bgcolor(char *args);
void cmd_edit_par(char *args);
void clear_cmd(char *args);
void cmd_switch(void);
void cmd_par(char *args);
void parse_table_command(char *cmd);
void cmd_run(char *args);
void cmd_cat1(char* args);
void cmd_ls(void);
void cmd_write(char* args);

void ata_read_sector(uint32_t lba, uint8_t* buffer);
void ata_write_sector(uint32_t lba, uint8_t* buffer);
int ata_detect(int port);
void ata_get_model(int port, char *model);
char read_keyboard(void);

void putchar(char c, int color);
void print(const char* str, int color);
void print_int(int num, int color);
void clear_screen(void);
void gotoxy(int x, int y);
void update_prompt(void);
void set_small_font(void);

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

int strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

void cpuid(int code, int *a, int *b, int *c, int *d) {
    asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(code));
}

void sysinf() {
    int a, b, c, d;
    char vendor[13] = {0};
    cpuid(0, &a, &b, &c, &d);
    *(int*)(vendor) = b;
    *(int*)(vendor + 4) = d;
    *(int*)(vendor + 8) = c;
    say("+-------------------------+\n", bg_color | 0x02);
    say("|     CPU information     |\n", bg_color | 0x02);
    say("+-------------------------+\n", bg_color | 0x02);
    say("CPU: ", bg_color | 0x02);
    say(vendor, bg_color | 0x02);
    say("\n", bg_color | 0x02);
    say("+-------------------------+\n", bg_color | 0x02);
}

void add_history(char *cmd) {
    if (hist_count < MAX_HIST) {
        history[hist_count] = cmd;
        hist_count++;
    }
}

void show_history() {
    for (int i = 0; i < hist_count; i++) {
        say(history[i], WHITE_ON_BLACK);
        say("\n", WHITE_ON_BLACK);
    }
}

void readline(char *buf) {
    int i = 0;
    char c, str[2] = {0, 0};
    while (1) {
        c = read_keyboard();
        if (c == '\n' || c == '\r') {
            buf[i] = '\0';
            say("\n", WHITE_ON_BLACK);
            break;
        }
        if (c == '\b' && i > 0) {
            i--;
            say("\b \b", WHITE_ON_BLACK);
            continue;
        }
        if (i < 63 && c >= ' ' && c <= '~') {
            buf[i++] = c;
            str[0] = c;
            say(str, WHITE_ON_BLACK);
        }
    }
}

typedef struct {
    char *name;
    void (*func)(char *args);
    char *desc;
} command_t;

command_t commands[] = {
    {"help",  cmd_h,     "Show table commands"},
    {"hello", cmd_hello, "Say hello"},
    {"time",  cmd_time,  "Show time"},
    {"echo", cmd_echo,   "echo text"},
    {"calc", cmd_calc,   "calculator"},
    {"reboot", cmd_reb,  "rebooting"},
    {"shutdown", cmd_sh, "shutdowning pc"},
    {"sleep", cmd_sleep, "Wait for N seconds"},
    {"bgcolor", cmd_bgcolor, "Change background color 0-F"},
    {"edit", cmd_edit_par, "Text editor"},
    {"clear", clear_cmd, "clear screen"},
    {NULL, NULL, NULL}
};

void parse_table_command(char *cmd) {
    char *args = cmd;
    while (*args && *args != ' ') args++;
    if (*args == ' ') { *args = '\0'; args++; } else args = NULL;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, cmd) == 0) {
            commands[i].func(args);
            return;
        }
    }
    say("Table command not found\n", 0x0C);
}

void cmd_par(char *args) {
    if (!args || args[0] == '\0') {
        say("Usage: par <table_command>\n", 0x0F);
        say("Available: ", 0x0F);
        for (int i = 0; commands[i].name != NULL; i++) {
            say(commands[i].name, 0x0E);
            say(" ", 0x0E);
        }
        say("\n", 0x0F);
        return;
    }
    parse_table_command(args);
}

void main(void) {
    set_small_font();

    char* video = (char*) VIDEO_MEMORY;
    for (int i = 0; i < 80; i++) video[i * 2] = ' ', video[i * 2 + 1] = 0x17;
    bg_color = 0x00;
    for (int i = 80; i < 80 * 25; i++) video[i * 2] = ' ', video[i * 2 + 1] = bg_color | 0x0F;

    saved_note[0] = '\0';
    init_system();

    //ofs_format(10000);
    ofs_list();

    outb(0x61, inb(0x61) | 3);
    outb(0x42, 0xFF);
    outb(0x42, 0xFF);
    for (int i = 0; i < 100000; i++);
    outb(0x61, inb(0x61) & 0xFC);
    for (int i = 0; i < 50000; i++);
    outb(0x61, inb(0x61) | 3);
    outb(0x42, 0x80);
    outb(0x42, 0x80);
    for (int i = 0; i < 100000; i++);
    outb(0x61, inb(0x61) & 0xFC);

    say("\n", WHITE_ON_BLACK);
    say("Ocero Kernel v2.1\n", bg_color | 0x04);
    update_prompt();

    char cmd[256];
    int idx = 0;

    while (1) {
        char ch = read_keyboard();
        if (ch == 0) continue;

        if (ch == 0x3B) {
            say("\n============== HELP ================\n", WHITE_ON_BLACK);
            say("Commands: help, clear, edit, notes, load, reboot, datetime, echo\n", WHITE_ON_BLACK);
            say("======================================\n", WHITE_ON_BLACK);
            say("/home> ", bg_color | 0x02);
            continue;
        }

        if (ch == 0x3C) {
            say("\n======== SYSTEM INFO ========\n", WHITE_ON_BLACK);
            say("Ocero-32 | 32-bit\n", WHITE_ON_BLACK);
            say("ATA: OK | BFS: files\n", WHITE_ON_BLACK);
            say("===============================\n", WHITE_ON_BLACK);
            say("/home> ", bg_color | 0x02);
            continue;
        }

        if (ch == '\n' || ch == '\r') {
            cmd[idx] = '\0';
            say("\n", WHITE_ON_BLACK);

            if (strcmp(cmd, "help") == 0) cmd_help();
            else if (strcmp(cmd, "clear") == 0) clear_screen();
            else if (strcmp(cmd, "about") == 0) cmd_about();
            else if (strcmp(cmd, "ping") == 0) cmd_ping();
            else if (strcmp(cmd, "disks") == 0) cmd_disks();
            else if (strcmp(cmd, "testdisk") == 0) cmd_testdisk();
            else if (strcmp(cmd, "mode") == 0) cmd_mode();
            else if (strcmp(cmd, "par") == 0) cmd_par(NULL);
            else if (strncmp(cmd, "par ", 4) == 0) cmd_par(cmd + 4);
            else if (strcmp(cmd, "switch") == 0) cmd_switch();
            else if (strcmp(cmd, "cat1") == 0) cmd_icat();
            else if (strncmp(cmd, "cat ", 4) == 0) cmd_cat1(cmd + 4);
            else if (strcmp(cmd, "save") == 0) cmd_save();
            else if (strcmp(cmd, "load") == 0) cmd_load();
            else if (strcmp(cmd, "8ball") == 0) cmd_8ball();
            else if (strcmp(cmd, "datetime") == 0) cmd_time(NULL);
            else if (strcmp(cmd, "win") == 0) cmd_win();
            else if (strcmp(cmd, "say") == 0) cmd_say();
            else if (strcmp(cmd, "cube") == 0) cmd_cube();
            else if (strcmp(cmd, "req") == 0) cmd_req();
            else if (strcmp(cmd, "window") == 0) cmd_window();
            else if (strcmp(cmd, "new") == 0) cmd_new();
            else if (strcmp(cmd, "cpu") == 0) cmd_cpu();
            else if (strcmp(cmd, "history") == 0) show_history();
            else if (strcmp(cmd, "line") == 0) cmd_line();
            else if (strcmp(cmd, "edit") == 0) cmd_edit();
            else if (strcmp(cmd, "notes") == 0) cmd_notes();
            else if (strcmp(cmd, "kernel") == 0) cmd_kernel();
            else if (strcmp(cmd, "fetch") == 0) cmd_fetch();
            else if (strncmp(cmd, "run ", 4) == 0) cmd_run(cmd + 4);
            else if (strcmp(cmd, "shutdown") == 0) cmd_shutdown();
            else if (strcmp(cmd, "ls") == 0) cmd_ls();
            else if (strncmp(cmd, "write ", 6) == 0) cmd_write(cmd + 6);
            else if (strcmp(cmd, "note2") == 0) cmd_windo4();
            else if (strcmp(cmd, "disk") == 0) cmd_win_disk();
            else if (strcmp(cmd, "tt") == 0) cmd_win_tt();
            else if (strcmp(cmd, "settings") == 0) cmd_settings();
            else if (strcmp(cmd, "reboot") == 0) cmd_reboot();
            else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
                say(cmd + 5, WHITE_ON_BLACK);
                say("\n", WHITE_ON_BLACK);
            } else if (cmd[0] != '\0') {
                say("Unknown command\n", RED_ON_BLACK);
            }

            say("> ", bg_color | 0x02);
            idx = 0;
            continue;
        }

        if (ch == '\b') {
            if (idx > 0) {
                idx--;
                say("\b \b", bg_color | 0x0F);
            }
            continue;
        }

        if (idx < 255) {
            cmd[idx++] = ch;
            putchar(ch, bg_color | 0x0F);
        }
    }
}
