#include <string.h>

#include "Libs/BIOS.h"
#include "Libs/SD.h"

#define MAX_LINE 64
#define MAX_ARGS 4

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

void cmd_ls() {
    SD_DIR dir;
    SD_INFO fno;
    char* path;
    char size_str[11];
    unsigned long sz;
    int i;

    path = (arg_count > 1) ? args[1] : ".";

    if (sd_opendir(&dir, path)) {
        while (sd_readdir(&dir, &fno)) {
            if (fno.fattrib & AM_DIR) {
                print_str("[DIR]      ");
            } else {
                sz = fno.fsize;
                i = 9;
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
    static char editor_buf[4096];
    int editor_len = 0;

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
        if (c == 27) break;

        if (c == '\b' || c == 8 || c == 127) {
            if (editor_len > 0) {
                editor_len--;
                bios_putchar(' ');
                bios_putchar('\b');
            }
        } else if (c == '\r' || c == '\n') {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = '\n';
                bios_putchar('\r');
                bios_putchar('\n');
            }
        } else {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = c;
            }
        }
    }

    editor_buf[editor_len] = '\0';
    sd_puts(&fp, editor_buf);
    
    sd_close(&fp);
    println("");
    println("--- SAVED ---");
}

int main(void) {
    char cwd[64];
    INIT_BUFFER();

    println("microDOS v1.1");
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

        os_read_line(line, MAX_LINE);
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
            println("Commands: ls, cd, mkdir, touch, cat, rm, nano, help, exit");
        } else if (strcmp(args[0], "exit") == 0) {
            break;
        } else {
            print_str("Unknown command: ");
            println(args[0]);
        }
    }

    return 0;
}