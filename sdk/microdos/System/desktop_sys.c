#include "Libs/app-bios.h"
#include "Libs/GPU.h"
#include "Libs/Mouse.h"
#include "Libs/Font5x5.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define OS_STARTUP_DONE (*(volatile uint8_t*)0x5504)

enum {
    APPS_PER_PAGE = 12,
    MAX_APPS      = 255
};

static const uint8_t COL_X[4] = {6, 30, 54, 78};
static const uint8_t ROW_Y[3] = {3, 21, 39};

static char app_names[MAX_APPS][9];
static uint16_t total_apps = 0;
static uint8_t total_pages = 1;
static uint8_t current_page = 0;
static int16_t selected_idx = -1;
static bool was_pressed = false;
static uint8_t cursor_bg[8][8];
static int16_t prev_cursor_x = -1;
static int16_t prev_cursor_y = -1;
static bool cursor_visible = false;

static void desktop_draw_wallpaper(void) {
    SD_FILE file;
    if (sd_open(&file, "/SYSTEM/DESKTOP/bliss.bin", SD_READ)) {
        sd_read(&file, (uint8_t*)GPU_VRAM_START, 8192);
        sd_close(&file);
    } else {
        gpu_fill_screen(0x00);
    }
}

static void desktop_scan_apps(void) {
    SD_DIR dir;
    SD_INFO fno;
    total_apps = 0;

    if (!sd_opendir(&dir, "/bin")) {
        return;
    }

    while (sd_readdir(&dir, &fno)) {
        if (fno.fattrib & AM_DIR) {
            continue;
        }
        if (fno.fname[0] == '.') {
            continue;
        }

        size_t len = strlen(fno.fname);
        if (len > 4) {
            if (fno.fname[len - 4] == '.' &&
                (fno.fname[len - 3] == 'a' || fno.fname[len - 3] == 'A') &&
                (fno.fname[len - 2] == 'p' || fno.fname[len - 2] == 'P') &&
                (fno.fname[len - 1] == 'p' || fno.fname[len - 1] == 'P')) {
                size_t base_len = len - 4;
                if (base_len > 8) {
                    base_len = 8;
                }
                memcpy(app_names[total_apps], fno.fname, base_len);
                app_names[total_apps][base_len] = '\0';
                total_apps++;
                if (total_apps >= MAX_APPS) {
                    break;
                }
            }
        }
    }

    sd_closedir(&dir);

    uint16_t count = 0;
    for (uint16_t i = 0; i < total_apps; ++i) {
        char path[32];
        strcpy(path, "/bin/");
        strcat(path, app_names[i]);
        strcat(path, ".app");

        SD_FILE file;
        bool keep = true;
        if (sd_open(&file, path, SD_READ)) {
            uint8_t hdr[16];
            if (sd_read(&file, hdr, 16) == 16) {
                if (hdr[5] & 0x04) {
                    keep = false;
                }
            }
            sd_close(&file);
        }

        if (keep) {
            if (count != i) {
                memcpy(app_names[count], app_names[i], 9);
            }
            count++;
        }
    }

    total_apps = count;
    total_pages = (total_apps == 0) ? 1 : (uint8_t)((total_apps + APPS_PER_PAGE - 1) / APPS_PER_PAGE);
}

static void desktop_draw_box_border(uint8_t slot_idx, uint8_t color) {
    uint8_t col = slot_idx % 4;
    uint8_t row = slot_idx / 4;
    int8_t bx = (int8_t)COL_X[col];
    int8_t by = (int8_t)ROW_Y[row];
    gpu_draw_rect((int8_t)(bx - 1), (int8_t)(by - 1), 18, 1, color);
    gpu_draw_rect((int8_t)(bx - 1), (int8_t)(by + 16), 18, 1, color);
    gpu_draw_rect((int8_t)(bx - 1), by, 1, 16, color);
    gpu_draw_rect((int8_t)(bx + 16), by, 1, 16, color);
}

static void desktop_draw_bar(int16_t sel_idx) {
    gpu_draw_rect(0, 56, 100, 8, 0x00);
    gpu_draw_rect(0, 55, 100, 1, 0x3F);

    if (total_pages > 1) {
        font_draw_char(3, 57, '<', 0x3F);
        font_draw_char(93, 57, '>', 0x3F);
    }

    if (sel_idx >= 0 && sel_idx < (int16_t)total_apps) {
        font_draw_string(14, 57, app_names[sel_idx], 0x3F);
    }
}

static void desktop_draw_page(uint8_t page, int16_t sel_idx) {
    uint16_t start_idx = (uint16_t)page * APPS_PER_PAGE;
    uint8_t count = 0;
    if (start_idx < total_apps) {
        count = (uint8_t)(total_apps - start_idx);
        if (count > APPS_PER_PAGE) {
            count = APPS_PER_PAGE;
        }
    }

    for (uint8_t i = 0; i < count; ++i) {
        uint16_t app_idx = start_idx + i;
        uint8_t col = i % 4;
        uint8_t row = i / 4;
        int8_t bx = (int8_t)COL_X[col];
        int8_t by = (int8_t)ROW_Y[row];

        char path[32];
        strcpy(path, "/bin/");
        strcat(path, app_names[app_idx]);
        strcat(path, ".app");

        SD_FILE file;
        bool has_custom_icon = false;
        if (sd_open(&file, path, SD_READ)) {
            uint8_t hdr[16];
            if (sd_read(&file, hdr, 16) == 16) {
                if (hdr[5] & 0x02) {
                    has_custom_icon = true;
                    for (uint8_t r = 0; r < 16; ++r) {
                        sd_read(&file, (uint8_t*)GPU_VRAM_START + ((uint16_t)(by + r) << 7) + bx, 16);
                    }
                }
            }
            sd_close(&file);
        }

        if (!has_custom_icon) {
            for (uint8_t r = 0; r < 16; ++r) {
                for (uint8_t c = 0; c < 16; ++c) {
                    gpu_put_pixel((int8_t)(bx + c), (int8_t)(by + r), (((c >> 2) ^ (r >> 2)) & 1) ? 0x33 : 0x00);
                }
            }
        }

        desktop_draw_box_border(i, (sel_idx == (int16_t)app_idx) ? 0x3F : 0x00);
    }
}

static void desktop_erase_cursor(void) {
    if (!cursor_visible) {
        return;
    }
    for (uint8_t r = 0; r < 8; ++r) {
        int16_t py = prev_cursor_y + r;
        for (uint8_t c = 0; c < 8; ++c) {
            int16_t px = prev_cursor_x + c;
            if (px >= 0 && px < GPU_WIDTH && py >= 0 && py < GPU_HEIGHT) {
                ((uint8_t*)GPU_VRAM_START)[((uint16_t)py << 7) + (uint16_t)px] = cursor_bg[r][c];
            }
        }
    }
    cursor_visible = false;
}

static void desktop_save_cursor_bg(int16_t x, int16_t y) {
    for (uint8_t r = 0; r < 8; ++r) {
        int16_t py = y + r;
        for (uint8_t c = 0; c < 8; ++c) {
            int16_t px = x + c;
            if (px >= 0 && px < GPU_WIDTH && py >= 0 && py < GPU_HEIGHT) {
                cursor_bg[r][c] = ((uint8_t*)GPU_VRAM_START)[((uint16_t)py << 7) + (uint16_t)px];
            } else {
                cursor_bg[r][c] = 0x00;
            }
        }
    }
}

void desktop_redraw(void) {
    desktop_erase_cursor();
    desktop_draw_wallpaper();
    desktop_draw_page(current_page, selected_idx);
    desktop_draw_bar(selected_idx);
}

void desktop_init(void) {
    current_page = 0;
    selected_idx = -1;
    was_pressed = false;
    cursor_visible = false;
    prev_cursor_x = -1;
    prev_cursor_y = -1;

    desktop_draw_wallpaper();
    desktop_scan_apps();
    desktop_draw_page(current_page, selected_idx);
    desktop_draw_bar(selected_idx);
}

bool desktop_tick(char* out_app_name) {
    int16_t mx = (int16_t)mouse_get_x();
    int16_t my = (int16_t)mouse_get_y();
    bool is_pressed = mouse_btn_left();
    bool launched = false;
    bool moved = (mx != prev_cursor_x || my != prev_cursor_y);
    bool click = (is_pressed && !was_pressed);

    if (moved || click) {
        desktop_erase_cursor();
    }

    if (is_pressed && !was_pressed) {
        bool handled = false;

        if (total_pages > 1) {
            if (mouse_in_rect(0, 55, 10, 9)) {
                current_page = (current_page > 0) ? (current_page - 1) : (total_pages - 1);
                selected_idx = -1;
                desktop_redraw();
                handled = true;
            } else if (mouse_in_rect(90, 55, 10, 9)) {
                current_page = (current_page + 1 < total_pages) ? (current_page + 1) : 0;
                selected_idx = -1;
                desktop_redraw();
                handled = true;
            }
        }

        if (!handled) {
            uint16_t start_idx = (uint16_t)current_page * APPS_PER_PAGE;
            uint8_t count = 0;
            if (start_idx < total_apps) {
                count = (uint8_t)(total_apps - start_idx);
                if (count > APPS_PER_PAGE) {
                    count = APPS_PER_PAGE;
                }
            }

            for (uint8_t i = 0; i < count; ++i) {
                uint8_t col = i % 4;
                uint8_t row = i / 4;
                uint8_t bx = COL_X[col];
                uint8_t by = ROW_Y[row];

                if (mouse_in_rect(bx, by, 16, 16)) {
                    int16_t global_idx = (int16_t)(start_idx + i);
                    if (selected_idx == global_idx) {
                        if (out_app_name != NULL) {
                            strcpy(out_app_name, app_names[global_idx]);
                        }
                        launched = true;
                    } else {
                        if (selected_idx >= (int16_t)start_idx && selected_idx < (int16_t)(start_idx + count)) {
                            desktop_draw_box_border((uint8_t)(selected_idx - start_idx), 0x00);
                        }
                        selected_idx = global_idx;
                        desktop_draw_box_border(i, 0x3F);
                        desktop_draw_bar(selected_idx);
                    }
                    handled = true;
                    break;
                }
            }
        }

        if (!handled) {
            uint16_t start_idx = (uint16_t)current_page * APPS_PER_PAGE;
            uint8_t count = 0;
            if (start_idx < total_apps) {
                count = (uint8_t)(total_apps - start_idx);
                if (count > APPS_PER_PAGE) {
                    count = APPS_PER_PAGE;
                }
            }
            if (selected_idx >= (int16_t)start_idx && selected_idx < (int16_t)(start_idx + count)) {
                desktop_draw_box_border((uint8_t)(selected_idx - start_idx), 0x00);
            }
            selected_idx = -1;
            desktop_draw_bar(-1);
        }
    }

    was_pressed = is_pressed;
    if (launched) {
        return true;
    }

    if (!cursor_visible || moved) {
        desktop_save_cursor_bg(mx, my);
        mouse_draw_cursor(mx, my);
        prev_cursor_x = mx;
        prev_cursor_y = my;
        cursor_visible = true;
    }

    return false;
}

static bool desktop_dispatch_pipeline(char* cmd, char* out_cmd, const char* prev_temp) {
    char* redir_file = NULL;
    uint8_t append = 0;
    char* p_gt = strchr(cmd, '>');
    if (p_gt) {
        *p_gt = '\0';
        p_gt++;
        if (*p_gt == '>') {
            append = 1;
            p_gt++;
        }
        while (*p_gt == ' ') p_gt++;
        redir_file = p_gt;
        char* end = redir_file;
        while (*end && *end != ' ' && *end != '\r' && *end != '\n') end++;
        *end = '\0';
        if (*redir_file == '\0') redir_file = NULL;
    }

    char* p_pipe = strchr(cmd, '|');
    if (p_pipe) {
        *p_pipe = '\0';
        char* next_stage = p_pipe + 1;
        while (*next_stage == ' ') next_stage++;

        const char* next_temp = (prev_temp && strcmp(prev_temp, "/_p1.tmp") == 0) ? "/_p2.tmp" : "/_p1.tmp";

        SD_FILE pf;
        if (sd_open(&pf, "/_pipe.cmd", SD_WRITE | SD_CREATE_ALWAYS)) {
            sd_puts(&pf, next_temp);
            sd_puts(&pf, "\n");
            sd_puts(&pf, next_stage);
            if (redir_file) {
                sd_puts(&pf, append ? " >> " : " > ");
                sd_puts(&pf, redir_file);
            }
            sd_close(&pf);
        }

        os_redirect_out(next_temp, 0);
    } else {
        if (redir_file) {
            os_redirect_out(redir_file, append);
        } else {
            os_redirect_out(NULL, 0);
        }
    }

    while (*cmd == ' ') cmd++;
    char* end = cmd + strlen(cmd);
    while (end > cmd && (*(end - 1) == ' ' || *(end - 1) == '\r' || *(end - 1) == '\n')) {
        *(--end) = '\0';
    }

    if (prev_temp != NULL) {
        size_t clen = strlen(cmd);
        if (clen > 0 && clen < 124) {
            if (!(clen == 1 && cmd[0] == '-') && !(clen >= 2 && cmd[clen - 1] == '-' && cmd[clen - 2] == ' ')) {
                strcat(cmd, " -");
            }
        }
    }

    strcpy(out_cmd, cmd);
    return true;
}

static bool desktop_check_pending_pipe(char* out_cmd) {
    if (!sd_exists("/_pipe.cmd")) {
        return false;
    }
    SD_FILE pf;
    if (!sd_open(&pf, "/_pipe.cmd", SD_READ)) {
        return false;
    }
    char pbuf[128];
    int16_t n = sd_read(&pf, pbuf, sizeof(pbuf) - 1);
    sd_close(&pf);
    sd_remove("/_pipe.cmd");
    if (n <= 0) {
        return false;
    }
    pbuf[n] = '\0';

    char* in_file = pbuf;
    char* next_cmd = strchr(pbuf, '\n');
    if (!next_cmd) {
        return false;
    }
    *next_cmd++ = '\0';

    os_redirect_out(NULL, 0);

    if (*in_file != '\0') {
        os_redirect_in(in_file);
    }

    return desktop_dispatch_pipeline(next_cmd, out_cmd, in_file);
}

static void desktop_check_pipe_cleanup(void) {
    os_redirect_out(NULL, 0);
    os_redirect_in(NULL);
    sd_remove("/_p1.tmp");
    sd_remove("/_p2.tmp");
    sd_remove("/_pipe.cln");
}

static uint32_t sd_size(SD_FILE* fp) {
    uint32_t sz;
    memcpy(&sz, (const uint8_t*)fp + 10, sizeof(uint32_t));
    return sz;
}

static bool desktop_queue_batch(const char* path) {
    SD_FILE f_in;
    if (!sd_open(&f_in, path, SD_READ)) {
        print_str("Error opening: ");
        println(path);
        return false;
    }
    SD_FILE f_out;
    bool exists = sd_exists("/_batch.cmd");
    uint8_t mode = exists ? (SD_WRITE | SD_OPEN_ALWAYS) : (SD_WRITE | SD_CREATE_ALWAYS);
    if (!sd_open(&f_out, "/_batch.cmd", mode)) {
        sd_close(&f_in);
        return false;
    }
    if (exists) {
        sd_seek(&f_out, sd_size(&f_out));
    }
    char line[128];
    uint8_t len = 0;
    while (true) {
        int16_t c = sd_getc(&f_in);
        if (c < 0 || c == '\n') {
            if (len > 0) {
                line[len] = '\0';
                while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
                    line[--len] = '\0';
                }
                char* p = line;
                while (*p == ' ' || *p == '\t') {
                    p++;
                }
                bool is_rem = (p[0] == 'R' || p[0] == 'r') &&
                              (p[1] == 'E' || p[1] == 'e') &&
                              (p[2] == 'M' || p[2] == 'm') &&
                              (p[3] == ' ' || p[3] == '\0');
                if (*p != '\0' && *p != '#' && *p != ';' && !is_rem) {
                    sd_puts(&f_out, p);
                    sd_puts(&f_out, "\n");
                }
                len = 0;
            }
            if (c < 0) {
                break;
            }
            continue;
        }
        if (c != '\r' && len < sizeof(line) - 1) {
            line[len++] = (char)c;
        }
    }
    sd_close(&f_in);
    sd_close(&f_out);
    return true;
}

static char batch_buf[2048];

static bool desktop_check_pending_batch(char* out_cmd) {
    if (!sd_exists("/_batch.cmd")) {
        return false;
    }
    SD_FILE f;
    if (!sd_open(&f, "/_batch.cmd", SD_READ)) {
        return false;
    }
    int16_t n = sd_read(&f, batch_buf, sizeof(batch_buf) - 1);
    sd_close(&f);
    if (n <= 0) {
        sd_remove("/_batch.cmd");
        return false;
    }
    batch_buf[n] = '\0';

    char* p = batch_buf;
    uint8_t out_len = 0;
    while (*p && *p != '\n' && *p != '\r') {
        if (out_len < 127) {
            out_cmd[out_len++] = *p;
        }
        p++;
    }
    out_cmd[out_len] = '\0';

    char* next_line = strchr(batch_buf, '\n');
    char* remaining = next_line ? (next_line + 1) : NULL;
    uint16_t remaining_len = remaining ? (uint16_t)strlen(remaining) : 0;
    if (remaining_len > 0) {
        if (sd_open(&f, "/_batch.cmd", SD_WRITE | SD_CREATE_ALWAYS)) {
            sd_write(&f, remaining, remaining_len);
            sd_close(&f);
        }
    } else {
        sd_remove("/_batch.cmd");
    }

    print_str("> ");
    println(out_cmd);
    desktop_dispatch_pipeline(out_cmd, out_cmd, NULL);
    return true;
}

static bool desktop_scan_startup(char* out_cmd) {
    if (OS_STARTUP_DONE == 0) {
        OS_STARTUP_DONE = 1;
        SD_DIR dir;
        if (sd_opendir(&dir, "/SYSTEM/STARTUP")) {
            SD_INFO fno;
            while (sd_readdir(&dir, &fno) && fno.fname[0] != '\0') {
                if (fno.fname[0] == '.') {
                    continue;
                }
                if (!(fno.fattrib & AM_DIR)) {
                    char spath[64];
                    strcpy(spath, "/SYSTEM/STARTUP/");
                    strcat(spath, fno.fname);
                    desktop_queue_batch(spath);
                }
            }
            sd_closedir(&dir);
            if (sd_exists("/_batch.cmd")) {
                return desktop_check_pending_batch(out_cmd);
            }
        }
    }
    return false;
}

static bool is_batch_file(const char* name) {
    size_t len = strlen(name);
    if (len < 4) {
        return false;
    }
    const char* ext = name + len - 4;
    if (ext[0] != '.') {
        return false;
    }
    bool is_bat = (ext[1] == 'b' || ext[1] == 'B') &&
                  (ext[2] == 'a' || ext[2] == 'A') &&
                  (ext[3] == 't' || ext[3] == 'T');
    bool is_cmd = (ext[1] == 'c' || ext[1] == 'C') &&
                  (ext[2] == 'm' || ext[2] == 'M') &&
                  (ext[3] == 'd' || ext[3] == 'D');
    return is_bat || is_cmd;
}

enum {
    HIST_MAX = 16,
    HIST_LEN = 128
};

static char hist_entries[HIST_MAX][HIST_LEN];
static uint8_t hist_count = 0;
static int8_t hist_idx = -1;
static char hist_draft[HIST_LEN];

static void desktop_hist_load(void) {
    hist_count = 0;
    hist_idx = -1;
    SD_FILE f;
    if (!sd_open(&f, "/SYSTEM/HIST.SYS", SD_READ)) {
        return;
    }
    char buf[HIST_LEN];
    uint8_t len = 0;
    while (true) {
        int16_t c = sd_getc(&f);
        if (c < 0 || c == '\n') {
            if (len > 0) {
                buf[len] = '\0';
                if (hist_count < HIST_MAX) {
                    strcpy(hist_entries[hist_count++], buf);
                } else {
                    for (uint8_t i = 0; i < HIST_MAX - 1; ++i) {
                        strcpy(hist_entries[i], hist_entries[i + 1]);
                    }
                    strcpy(hist_entries[HIST_MAX - 1], buf);
                }
                len = 0;
            }
            if (c < 0) {
                break;
            }
            continue;
        }
        if (c != '\r' && len < HIST_LEN - 1) {
            buf[len++] = (char)c;
        }
    }
    sd_close(&f);
}

static void desktop_hist_save(void) {
    SD_FILE f;
    if (!sd_open(&f, "/SYSTEM/HIST.SYS", SD_WRITE | SD_CREATE_ALWAYS)) {
        return;
    }
    for (uint8_t i = 0; i < hist_count; ++i) {
        sd_puts(&f, hist_entries[i]);
        sd_puts(&f, "\n");
    }
    sd_close(&f);
}

static void desktop_hist_add(const char* cmd) {
    if (!cmd) {
        return;
    }
    const char* p = cmd;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    if (*p == '\0') {
        return;
    }
    if (hist_count > 0 && strcmp(hist_entries[hist_count - 1], cmd) == 0) {
        return;
    }
    if (hist_count < HIST_MAX) {
        strncpy(hist_entries[hist_count], cmd, HIST_LEN - 1);
        hist_entries[hist_count][HIST_LEN - 1] = '\0';
        hist_count++;
    } else {
        for (uint8_t i = 0; i < HIST_MAX - 1; ++i) {
            strcpy(hist_entries[i], hist_entries[i + 1]);
        }
        strncpy(hist_entries[HIST_MAX - 1], cmd, HIST_LEN - 1);
        hist_entries[HIST_MAX - 1][HIST_LEN - 1] = '\0';
    }
    desktop_hist_save();
}

static bool prefix_match_nocase(const char* str, const char* prefix, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        char c1 = str[i];
        char c2 = prefix[i];
        if (c1 >= 'a' && c1 <= 'z') {
            c1 -= 32;
        }
        if (c2 >= 'a' && c2 <= 'z') {
            c2 -= 32;
        }
        if (c1 != c2 || c1 == '\0') {
            return false;
        }
    }
    return true;
}

static void desktop_autocomplete(char* out_cmd, uint8_t* p_len) {
    uint8_t len = *p_len;
    uint8_t start = len;
    while (start > 0 && out_cmd[start - 1] != ' ') {
        start--;
    }
    uint8_t token_len = len - start;
    char token[128];
    memcpy(token, &out_cmd[start], token_len);
    token[token_len] = '\0';

    char dir_path[128];
    const char* prefix = token;
    char* last_slash = strrchr(token, '/');
    if (!last_slash) {
        strcpy(dir_path, ".");
        prefix = token;
    } else if (last_slash == token) {
        strcpy(dir_path, "/");
        prefix = token + 1;
    } else {
        size_t dlen = (size_t)(last_slash - token + 1);
        memcpy(dir_path, token, dlen);
        dir_path[dlen] = '\0';
        prefix = last_slash + 1;
    }

    SD_DIR dir;
    if (!sd_opendir(&dir, dir_path)) {
        return;
    }

    size_t prefix_len = strlen(prefix);
    SD_INFO fno;
    bool found = false;

    while (sd_readdir(&dir, &fno)) {
        if (fno.fname[0] == '\0') {
            break;
        }
        if (fno.fname[0] == '.' && prefix[0] != '.') {
            continue;
        }
        if (prefix_match_nocase(fno.fname, prefix, prefix_len)) {
            found = true;
            break;
        }
    }

    if (found) {
        const char* s = fno.fname + prefix_len;
        while (*s && len < 126) {
            out_cmd[len++] = *s;
            bios_putchar(*s);
            s++;
        }
        char end_char = (fno.fattrib & AM_DIR) ? '/' : ' ';
        if (len < 127) {
            out_cmd[len++] = end_char;
            bios_putchar(end_char);
        }
        out_cmd[len] = '\0';
        *p_len = len;
    }

    sd_closedir(&dir);
}

bool desktop_shell(char* out_cmd) {
    if (desktop_check_pending_pipe(out_cmd)) {
        return true;
    }
    if (desktop_check_pending_batch(out_cmd)) {
        return true;
    }
    if (desktop_scan_startup(out_cmd)) {
        return true;
    }
    desktop_check_pipe_cleanup();

    desktop_hist_load();
    desktop_init();

    char cwd[32];
    if (!sd_getcwd(cwd, sizeof(cwd))) {
        strcpy(cwd, "?");
    }
    print_str("uDOS:");
    print_str(cwd);
    print_str("> ");

    uint8_t len = 0;
    uint8_t esc_state = 0;
    while (true) {
        char c = bios_getchar_nb();
        if (c != 0) {
            if (esc_state == 1) {
                esc_state = (c == '[') ? 2 : 0;
                continue;
            }
            if (esc_state == 2) {
                if (c == 'A') {
                    if (hist_count > 0) {
                        if (hist_idx == -1) {
                            out_cmd[len] = '\0';
                            strcpy(hist_draft, out_cmd);
                            hist_idx = (int8_t)(hist_count - 1);
                        } else if (hist_idx > 0) {
                            hist_idx--;
                        }
                        while (len > 0) {
                            bios_putchar('\b');
                            bios_putchar(' ');
                            bios_putchar('\b');
                            len--;
                        }
                        strcpy(out_cmd, hist_entries[hist_idx]);
                        len = (uint8_t)strlen(out_cmd);
                        print_str(out_cmd);
                    }
                    esc_state = 0;
                    continue;
                }
                if (c == 'B') {
                    if (hist_idx != -1) {
                        hist_idx++;
                        while (len > 0) {
                            bios_putchar('\b');
                            bios_putchar(' ');
                            bios_putchar('\b');
                            len--;
                        }
                        if (hist_idx < (int8_t)hist_count) {
                            strcpy(out_cmd, hist_entries[hist_idx]);
                        } else {
                            hist_idx = -1;
                            strcpy(out_cmd, hist_draft);
                        }
                        len = (uint8_t)strlen(out_cmd);
                        print_str(out_cmd);
                    }
                    esc_state = 0;
                    continue;
                }
                if (c >= '0' && c <= '9') {
                    esc_state = 3;
                    continue;
                }
                esc_state = 0;
                continue;
            }
            if (esc_state == 3) {
                if (c == '~' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                    esc_state = 0;
                }
                continue;
            }
            if (c == '\x1b') {
                esc_state = 1;
                continue;
            }
            if (c == '\t' || c == 9) {
                desktop_autocomplete(out_cmd, &len);
                continue;
            }
            if (c == '\r' || c == '\n') {
                out_cmd[len] = '\0';
                if (len > 0) {
                    int16_t last = (int16_t)len - 1;
                    while (last >= 0 && (out_cmd[last] == ' ' || out_cmd[last] == '\t')) {
                        last--;
                    }
                    if (last >= 0 && (out_cmd[last] == '|' || out_cmd[last] == '>')) {
                        print_str("\r\nSyntax error\r\n");
                        print_str("uDOS:");
                        print_str(cwd);
                        print_str("> ");
                        len = 0;
                        hist_idx = -1;
                        continue;
                    }
                }
                bios_putchar('\r');
                bios_putchar('\n');
                desktop_erase_cursor();
                hist_idx = -1;
                desktop_hist_add(out_cmd);
                if (strncmp(out_cmd, "batch ", 6) == 0) {
                    const char* spath = out_cmd + 6;
                    while (*spath == ' ') {
                        spath++;
                    }
                    if (desktop_queue_batch(spath)) {
                        return desktop_check_pending_batch(out_cmd);
                    }
                } else if (is_batch_file(out_cmd)) {
                    if (desktop_queue_batch(out_cmd)) {
                        return desktop_check_pending_batch(out_cmd);
                    }
                }
                char input_copy[128];
                strcpy(input_copy, out_cmd);
                return desktop_dispatch_pipeline(input_copy, out_cmd, NULL);
            }
            if (c == '\b' || c == 8 || c == 127) {
                if (len > 0) {
                    len--;
                    bios_putchar('\b');
                    bios_putchar(' ');
                    bios_putchar('\b');
                }
            } else if (len < 127) {
                out_cmd[len++] = c;
                bios_putchar(c);
            }
        }

        if (desktop_tick(out_cmd)) {
            print_str(out_cmd);
            print_str("\r\n");
            char tick_copy[128];
            strcpy(tick_copy, out_cmd);
            return desktop_dispatch_pipeline(tick_copy, out_cmd, NULL);
        }
    }
}

int main(void) {
    char buf[128];
    return desktop_shell(buf) ? 0 : 1;
}

