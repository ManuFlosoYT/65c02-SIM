/* grep.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

MICRODOS_NO_DESKTOP();

static char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }
    return c;
}

static int str_match(const char *text, const char *pattern) {
    if (*pattern == '\0') {
        return 1;
    }
    for (int i = 0; text[i] != '\0'; i++) {
        int j = 0;
        for (; pattern[j] != '\0' && to_lower(text[i+j]) == to_lower(pattern[j]); j++) {}
        if (pattern[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

int main(void) {
    char **args = _args_ptr;
    SD_FILE fp;
    int16_t c = 0;
    char line[128];
    int line_idx = 0;
    int line_num = 1;

    if (arg_count < 2) {
        println("Usage: grep <str> [file]");
        return 1;
    }

    char *search_str = args[1];
    bool from_stdin = (arg_count == 2 || (arg_count >= 3 && strcmp(args[2], "-") == 0));

    if (!from_stdin) {
        if (!sd_open(&fp, args[2], SD_READ)) {
            println("Error: File not found.");
            return 1;
        }
    }

    while (true) {
        if (from_stdin) {
            c = bios_getchar();
            if (c == 0 || c == 4 || c == 26 || c == -1) {
                break;
            }
        } else {
            c = sd_getc(&fp);
            if (c < 0) {
                break;
            }
        }
        if (c == '\n' || c == '\r' || line_idx >= 127) {
            line[line_idx] = '\0';
            if (line_idx > 0) {
                if (str_match(line, search_str)) {
                    print_str("L");
                    print_num(line_num);
                    print_str(": ");
                    println(line);
                }
            }
            if (c == '\n') {
                line_num++;
            }
            line_idx = 0;
        } else {
            line[line_idx++] = (char)c;
        }
    }
    
    if (line_idx > 0) {
        line[line_idx] = '\0';
        if (str_match(line, search_str)) {
            print_str("L");
            print_num(line_num);
            print_str(": ");
            println(line);
        }
    }

    if (!from_stdin) {
        sd_close(&fp);
    }
    return 0;
}
