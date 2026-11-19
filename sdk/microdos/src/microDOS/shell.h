#pragma once

#include <stdbool.h>
#include <stdint.h>

enum {
    MAX_LINE = 128,
    MAX_ARGS = 4
};

extern char line[MAX_LINE];
extern char* args[MAX_ARGS];
extern uint8_t arg_count;

void tokenize(char* buf);
void os_read_line(char* buf, uint8_t max_len);
void os_redirect_out(const char* path, uint8_t append);
void os_redirect_in(const char* path);