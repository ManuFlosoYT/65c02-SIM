#pragma once

enum {
    MAX_LINE = 64,
    MAX_ARGS = 4
};

extern char line[MAX_LINE];
extern char* args[MAX_ARGS];
extern uint8_t arg_count;

void tokenize(char* buf);
void os_read_line(char* buf, uint8_t max_len);