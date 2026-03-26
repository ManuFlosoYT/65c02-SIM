#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "shell.h"

char line[MAX_LINE];
char* args[MAX_ARGS];
int arg_count;

void tokenize(char* buf) {
    char* p = buf;
    arg_count = 0;

    while (*p && arg_count < MAX_ARGS) {
        while (*p == ' ') p++;
        if (*p == '\0') break;

        args[arg_count++] = p;

        while (*p && *p != ' ') p++;
        if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }
}

void auto_complete(char* buf, int* len, int max_len) {
    int last_space = -1;
    int i;
    char* token;
    int token_len;
    char match[64];
    char* file_prefix;
    int match_len;
    int pref_len;
    static const char* commands[] = {"ls", "cat", "touch", "mkdir", "cd", "rm", "nano", "help", "exit", NULL};

    for (i = 0; i < *len; i++) {
        if (buf[i] == ' ') last_space = i;
    }

    token = &buf[last_space + 1];
    token_len = *len - (last_space + 1);
    file_prefix = token;

    if (token_len == 0) return;

    match[0] = '\0';

    if (last_space == -1) {
        for (i = 0; commands[i] != NULL; i++) {
            if (strncmp(commands[i], token, token_len) == 0) {
                strcpy(match, commands[i]);
                break;
            }
        }
    } else {
        char search_path[64];
        int prefix_len;
        int last_slash = -1;
        int path_len;
        SD_DIR dir;
        SD_INFO fno;

        strcpy(search_path, ".");
        prefix_len = token_len;
        
        for (i = 0; i < token_len; i++) {
            if (token[i] == '/') last_slash = i;
        }
        
        if (last_slash != -1) {
            path_len = last_slash;
            if (path_len > 0 && path_len < 64) {
                strncpy(search_path, token, path_len);
                search_path[path_len] = '\0';
            } else if (path_len == 0) {
                strcpy(search_path, "/");
            }
            file_prefix = &token[last_slash + 1];
            prefix_len = token_len - (last_slash + 1);
        }

        if (sd_opendir(&dir, search_path)) {
            while (sd_readdir(&dir, &fno)) {
                if (strncmp(fno.fname, file_prefix, prefix_len) == 0) {
                    strcpy(match, fno.fname);
                    break;
                }
            }
            sd_closedir(&dir);
        }
    }

    if (match[0] != '\0') {
        match_len = strlen(match);
        pref_len = (last_space == -1) ? token_len : (strlen(token) - (file_prefix - token));
        
        for (i = pref_len; i < match_len && *len < max_len - 1; i++) {
            buf[*len] = match[i];
            (*len)++;
            bios_putchar(match[i]);
        }
    }
}

void os_read_line(char* buf, int max_len) {
    int len = 0;
    char c;
    while (1) {
        c = bios_getchar();
        if (c == '\r' || c == '\n') {
            buf[len] = '\0';
            bios_putchar('\r');
            bios_putchar('\n');
            break;
        } else if (c == '\b' || c == 8 || c == 127) {
            if (len > 0) {
                len--;
                bios_putchar(' ');
                bios_putchar('\b');
            }
        } else if (c == '\t') {
            auto_complete(buf, &len, max_len);
        } else {
            if (len < max_len - 1) {
                buf[len++] = c;
            }
        }
    }
}