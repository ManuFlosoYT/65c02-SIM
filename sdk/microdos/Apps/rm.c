/* rm.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

static char path_buf[128];
static uint8_t flag_r = 0;
static uint8_t flag_f = 0;

static char to_lower_char(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static uint8_t is_dot_or_dotdot(const char* name) {
    return (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')));
}

static uint8_t is_root_dir(const char* s) {
    if (*s != '/') return 0;
    while (*s == '/') s++;
    return (*s == '\0');
}

static uint8_t has_wildcard(const char* s) {
    while (*s) {
        if (*s == '*' || *s == '?') return 1;
        s++;
    }
    return 0;
}

static uint8_t wildcard_match(const char* pat, const char* str) {
    while (*pat) {
        if (*pat == '*') {
            while (*(pat + 1) == '*') pat++;
            if (*(pat + 1) == '\0') return 1;
            while (*str) {
                if (wildcard_match(pat + 1, str)) return 1;
                str++;
            }
            return wildcard_match(pat + 1, str);
        }
        if (*str == '\0') return 0;
        if (*pat != '?' && to_lower_char(*pat) != to_lower_char(*str)) {
            return 0;
        }
        pat++;
        str++;
    }
    return (*str == '\0');
}

static void print_error(const char* path, const char* reason) {
    print_str("Error: '");
    print_str(path);
    print_str("': ");
    println(reason);
}

static uint8_t set_subpath(char* buf, const char* dir, const char* name, uint8_t max_len) {
    uint8_t len = 0;
    if (dir && dir[0] != '\0' && (dir[0] != '.' || dir[1] != '\0')) {
        while (*dir && len < max_len - 2) {
            buf[len++] = *dir++;
        }
        if (len > 0 && buf[len - 1] != '/' && len < max_len - 2) {
            buf[len++] = '/';
        }
    }
    while (*name && len < max_len - 1) {
        buf[len++] = *name++;
    }
    buf[len] = '\0';
    return len;
}

static uint8_t rm_tree(char* current_path) {
    SD_DIR dir;
    SD_INFO fno;
    uint8_t success = 1;
    uint8_t base_len = (uint8_t)strlen(current_path);

    if (!sd_opendir(&dir, current_path)) {
        if (!flag_f) {
            print_error(current_path, "Could not open directory");
        }
        return 0;
    }

    while (sd_readdir(&dir, &fno) && fno.fname[0] != '\0') {
        if (is_dot_or_dotdot(fno.fname)) continue;

        uint8_t prev_len = base_len;
        if (prev_len > 0 && current_path[prev_len - 1] != '/' && prev_len < sizeof(path_buf) - 2) {
            current_path[prev_len++] = '/';
        }
        uint8_t cur = prev_len;
        const char* p = fno.fname;
        while (*p && cur < sizeof(path_buf) - 1) {
            current_path[cur++] = *p++;
        }
        current_path[cur] = '\0';

        if (fno.fattrib & AM_DIR) {
            if (!rm_tree(current_path)) {
                success = 0;
            }
        } else {
            if (!sd_remove(current_path)) {
                success = 0;
                if (!flag_f) {
                    print_error(current_path, "Permission denied or removal failed");
                }
            }
        }

        current_path[base_len] = '\0';
    }
    sd_closedir(&dir);

    if (strcmp(current_path, "/") != 0) {
        if (!sd_remove(current_path)) {
            if (!flag_f) {
                print_error(current_path, "Permission denied or removal failed");
            }
            return 0;
        }
    }
    return success;
}

static uint8_t rm_target(const char* target) {
    if (is_dot_or_dotdot(target)) {
        if (!flag_f) {
            print_error(target, "Cannot remove '.' or '..'");
        }
        return 0;
    }

    if (is_root_dir(target)) {
        if (!flag_r) {
            if (!flag_f) {
                print_error(target, "Is a directory");
            }
            return 0;
        }
        path_buf[0] = '/';
        path_buf[1] = '\0';
        return rm_tree(path_buf);
    }

    SD_DIR d;
    if (sd_opendir(&d, target)) {
        sd_closedir(&d);
        if (!flag_r) {
            if (!flag_f) {
                print_error(target, "Is a directory");
            }
            return 0;
        }
        uint8_t i = 0;
        while (target[i] && i < sizeof(path_buf) - 1) {
            path_buf[i] = target[i];
            i++;
        }
        path_buf[i] = '\0';
        return rm_tree(path_buf);
    }

    if (!sd_exists(target)) {
        if (flag_f) {
            return 1;
        }
        print_error(target, "No such file or directory");
        return 0;
    }

    if (!sd_remove(target)) {
        if (!flag_f) {
            print_error(target, "Permission denied or removal failed");
        }
        return 0;
    }
    return 1;
}

static uint8_t rm_wildcard(const char* target) {
    char dir_buf[64];
    const char* pat;
    int16_t last_slash = -1;
    for (int16_t i = 0; target[i]; i++) {
        if (target[i] == '/') last_slash = i;
    }

    if (last_slash >= 0) {
        if (last_slash == 0) {
            strcpy(dir_buf, "/");
        } else {
            uint8_t len = (last_slash < 63) ? (uint8_t)last_slash : 63;
            memcpy(dir_buf, target, len);
            dir_buf[len] = '\0';
        }
        pat = target + last_slash + 1;
    } else {
        strcpy(dir_buf, ".");
        pat = target;
    }

    SD_DIR dir;
    SD_INFO fno;
    if (!sd_opendir(&dir, dir_buf)) {
        if (!flag_f) {
            print_error(dir_buf, "Could not open directory");
        }
        return 0;
    }

    uint16_t match_count = 0;
    uint8_t success = 1;

    while (sd_readdir(&dir, &fno) && fno.fname[0] != '\0') {
        if (is_dot_or_dotdot(fno.fname)) continue;
        if (wildcard_match(pat, fno.fname)) {
            match_count++;
            set_subpath(path_buf, dir_buf, fno.fname, sizeof(path_buf));
            if (fno.fattrib & AM_DIR) {
                if (!flag_r) {
                    success = 0;
                    if (!flag_f) {
                        print_error(path_buf, "Is a directory");
                    }
                } else {
                    if (!rm_tree(path_buf)) {
                        success = 0;
                    }
                }
            } else {
                if (!sd_remove(path_buf)) {
                    success = 0;
                    if (!flag_f) {
                        print_error(path_buf, "Permission denied or removal failed");
                    }
                }
            }
        }
    }
    sd_closedir(&dir);

    if (match_count == 0) {
        if (!flag_f) {
            print_error(target, "No match found");
        }
        return 0;
    }
    return success;
}

int main(void) {
    char** args = _args_ptr;
    uint8_t overall_status = 0;
    uint8_t target_count = 0;

    flag_r = 0;
    flag_f = 0;

    for (uint8_t i = 1; i < arg_count; i++) {
        char* arg = args[i];
        if (arg[0] == '-' && arg[1] != '\0') {
            for (uint8_t j = 1; arg[j] != '\0'; j++) {
                char c = arg[j];
                if (c == 'r' || c == 'R') {
                    flag_r = 1;
                } else if (c == 'f' || c == 'F') {
                    flag_f = 1;
                } else {
                    if (!flag_f) {
                        print_str("Error: Unknown option '-");
                        bios_putchar(c);
                        println("'");
                    }
                    return 1;
                }
            }
        } else {
            target_count++;
        }
    }

    if (target_count == 0) {
        if (!flag_f) {
            println("Usage: rm [-r] [-f] <path|pattern> ...");
            return 1;
        }
        return 0;
    }

    for (uint8_t i = 1; i < arg_count; i++) {
        char* arg = args[i];
        if (arg[0] == '-' && arg[1] != '\0') {
            continue;
        }

        uint8_t ok = 0;
        if (has_wildcard(arg)) {
            ok = rm_wildcard(arg);
        } else {
            ok = rm_target(arg);
        }

        if (!ok && !flag_f) {
            overall_status = 1;
        }
    }

    return (flag_f ? 0 : overall_status);
}
