#include <stdbool.h>
#include <stddef.h>
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

extern void (*os_putchar_hook)(char c);
extern char (*os_getchar_hook)(void);

static SD_FILE redir_in_f;
static SD_FILE redir_out_f;
static bool redir_in_open = false;
static bool redir_out_open = false;

static void os_pipe_putchar(char c) {
    sd_write(&redir_out_f, &c, 1);
}

static char os_pipe_getchar(void) {
    int16_t c = sd_getc(&redir_in_f);
    return (c < 0) ? 0 : (char)c;
}

void os_redirect_out(const char* path, uint8_t append) {
    if (redir_out_open) {
        os_putchar_hook = NULL;
        sd_close(&redir_out_f);
        redir_out_open = false;
    }
    if (path) {
        uint8_t mode = append ? (SD_WRITE | SD_OPEN_ALWAYS) : (SD_WRITE | SD_CREATE_ALWAYS);
        if (sd_open(&redir_out_f, path, mode)) {
            if (append) {
                sd_seek(&redir_out_f, sd_size(&redir_out_f));
            }
            redir_out_open = true;
            os_putchar_hook = os_pipe_putchar;
        }
    }
}

void os_redirect_in(const char* path) {
    if (redir_in_open) {
        os_getchar_hook = NULL;
        sd_close(&redir_in_f);
        redir_in_open = false;
    }
    if (path) {
        if (sd_open(&redir_in_f, path, SD_READ)) {
            redir_in_open = true;
            os_getchar_hook = os_pipe_getchar;
        }
    }
}