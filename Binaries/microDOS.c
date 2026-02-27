#include <string.h>

#include "Libs/BIOS.h"
#include "Libs/SD.h"

#define MAX_LINE 64
#define MAX_ARGS 4

char line[MAX_LINE];
char* args[MAX_ARGS];
int arg_count;

void tokenize(char* buf) {
    char* p;
    p = buf;
    arg_count = 0;

    while (*p && arg_count < MAX_ARGS) {
        /* Skip spaces */
        while (*p == ' ') p++;
        if (*p == '\0') break;

        args[arg_count++] = p;

        /* Find end of token */
        while (*p && *p != ' ') p++;
        if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }
}

void cmd_ls() {
    SD_DIR dir;
    SD_INFO fno;
    char* path;
    char size_str[11];

    path = (arg_count > 1) ? args[1] : ".";

    if (sd_opendir(&dir, path)) {
        while (sd_readdir(&dir, &fno)) {
            if (fno.fattrib & AM_DIR) {
                print_str("[DIR]      ");
            } else {
                /* Simple number to string for size */
                unsigned long sz = fno.fsize;
                int i = 9;
                size_str[10] = '\0';
                for (; i >= 0; i--) {
                    size_str[i] = (sz > 0 || i == 9) ? (char)((sz % 10) + '0') : ' ';
                    sz /= 10;
                }
                print_str(size_str);
                print_str(" ");
            }
            println(fno.fname);
        }
        sd_closedir(&dir);
    } else {
        println("ERROR: ls failed");
    }
}

void cmd_cat() {
    SD_FILE fp;
    int c;
    if (arg_count < 2) {
        println("Usage: cat <file>");
        return;
    }
    if (sd_open(&fp, args[1], SD_READ)) {
        while ((c = sd_getc(&fp)) != -1) {
            bios_putchar((char)c);
        }
        println("");
        sd_close(&fp);
    } else {
        println("ERROR: cat failed");
    }
}

void cmd_touch() {
    SD_FILE fp;
    if (arg_count < 2) {
        println("Usage: touch <file>");
        return;
    }
    if (sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_close(&fp);
    } else {
        println("ERROR: touch failed");
    }
}

void cmd_mkdir() {
    if (arg_count < 2) {
        println("Usage: mkdir <dir>");
        return;
    }
    if (!sd_mkdir(args[1])) {
        println("ERROR: mkdir failed");
    }
}

void cmd_cd() {
    if (arg_count < 2) {
        println("Usage: cd <dir>");
        return;
    }
    if (!sd_chdir(args[1])) {
        println("ERROR: cd failed");
    }
}

void cmd_rm() {
    if (arg_count < 2) {
        println("Usage: rm <path>");
        return;
    }
    if (!sd_remove(args[1])) {
        println("ERROR: rm failed");
    }
}

void cmd_nano() {
    SD_FILE fp;
    char c;
    char s[2];

    s[1] = '\0';

    if (arg_count < 2) {
        println("Usage: nano <file>");
        return;
    }

    if (!sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        println("ERROR: nano failed (open)");
        return;
    }

    println("--- NANO EDITOR ---");
    println("Type your text. Press ESC to save and exit.");

    while (1) {
        c = bios_getchar();
        if (c == 27) break; /* ESC */

        if (c == '\r' || c == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
            sd_puts(&fp, "\n");
        } else if (c == 8 || c == 127) {
            /* Backspace ignored for simplicity */
        } else {
            s[0] = c;
            sd_puts(&fp, s);
        }
    }

    sd_close(&fp);
    println("");
    println("--- SAVED ---");
}

int main(void) {
    char cwd[64];
    INIT_BUFFER();

    println("microDOS v1.0");
    println("Mounting SD...");

    if (!sd_mount()) {
        println("CRITICAL: Could not mount SD card.");
        return 1;
    }

    println("Ready.");

    while (1) {
        if (!sd_getcwd(cwd, 64)) {
            strcpy(cwd, "?");
        }
        print_str("microDOS: ");
        print_str(cwd);
        print_str(" > ");

        read_line(line, MAX_LINE);
        tokenize(line);

        if (arg_count == 0) continue;

        if (strcmp(args[0], "ls") == 0) {
            cmd_ls();
        } else if (strcmp(args[0], "cat") == 0) {
            cmd_cat();
        } else if (strcmp(args[0], "touch") == 0) {
            cmd_touch();
        } else if (strcmp(args[0], "mkdir") == 0) {
            cmd_mkdir();
        } else if (strcmp(args[0], "cd") == 0) {
            cmd_cd();
        } else if (strcmp(args[0], "rm") == 0) {
            cmd_rm();
        } else if (strcmp(args[0], "nano") == 0) {
            cmd_nano();
        } else if (strcmp(args[0], "help") == 0) {
            println("Commands: ls, cd, mkdir, touch, cat, rm, nano, help");
        } else if (strcmp(args[0], "exit") == 0) {
            break;
        } else {
            print_str("Unknown command: ");
            println(args[0]);
        }
    }

    return 0;
}
