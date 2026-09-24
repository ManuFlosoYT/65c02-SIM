/* grep.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

static int str_match(const char *text, const char *pattern) {
    int i, j;
    if (*pattern == '\0') return 1;
    for (i = 0; text[i] != '\0'; i++) {
        for (j = 0; pattern[j] != '\0' && text[i+j] == pattern[j]; j++) {}
        if (pattern[j] == '\0') return 1;
    }
    return 0;
}

int main(void) {
    char **args = _args_ptr;
    SD_FILE fp;
    int16_t c;
    char line[128];
    int line_idx = 0;
    int line_num = 1;
    char *search_str;

    if (arg_count < 3) {
        println("Usage: grep <str> <file>");
        return 1;
    }

    search_str = args[1];

    if (!sd_open(&fp, args[2], SD_READ)) {
        println("Error: File not found.");
        return 1;
    }

    while ((c = sd_getc(&fp)) >= 0) {
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

    sd_close(&fp);
    return 0;
}
