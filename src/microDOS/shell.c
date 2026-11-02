#include <stdbool.h>
#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "shell.h"

char line[MAX_LINE];
char* args[MAX_ARGS];
uint8_t arg_count;

void tokenize(char* buf) {
    char* p = buf;
    arg_count = 0;

    while (*p && arg_count < MAX_ARGS) {
        while (*p == ' ') {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        args[arg_count++] = p;

        while (*p && *p != ' ') {
            p++;
        }
        if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }
}

void os_read_line(char* buf, uint8_t max_len) {
    uint8_t len = 0;
    while (len < max_len - 1) {
        char c = bios_getchar();
        if (c == '\r' || c == '\n') break;
        buf[len++] = c;
    }
    buf[len] = '\0';
}