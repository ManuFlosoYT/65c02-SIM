#include "Libs/app-bios.h"
#include "Libs/GPU.h"
#include "Libs/Mouse.h"
#include "Libs/Font5x5.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
    APPS_PER_PAGE = 12,
    MAX_APPS      = 36
};

static const uint8_t COL_X[4] = {6, 30, 54, 78};
static const uint8_t ROW_Y[3] = {3, 21, 39};

static char app_names[MAX_APPS][9];
static uint8_t total_apps = 0;
static uint8_t total_pages = 1;
static uint8_t current_page = 0;
static int8_t selected_idx = -1;
static bool was_pressed = false;

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

static void desktop_draw_bar(int8_t sel_idx) {
    gpu_draw_rect(0, 56, 100, 8, 0x00);
    gpu_draw_rect(0, 55, 100, 1, 0x3F);

    if (total_pages > 1) {
        font_draw_char(3, 57, '<', 0x3F);
        font_draw_char(93, 57, '>', 0x3F);
    }

    if (sel_idx >= 0 && sel_idx < (int8_t)total_apps) {
        font_draw_string(14, 57, app_names[sel_idx], 0x3F);
    }
}

static void desktop_draw_page(uint8_t page, int8_t sel_idx) {
    uint8_t start_idx = page * APPS_PER_PAGE;
    uint8_t count = 0;
    if (start_idx < total_apps) {
        count = total_apps - start_idx;
        if (count > APPS_PER_PAGE) {
            count = APPS_PER_PAGE;
        }
    }

    for (uint8_t i = 0; i < count; ++i) {
        uint8_t app_idx = start_idx + i;
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

        desktop_draw_box_border(i, (sel_idx == (int8_t)app_idx) ? 0x3F : 0x00);
    }
}

void desktop_redraw(void) {
    desktop_draw_wallpaper();
    desktop_draw_page(current_page, selected_idx);
    desktop_draw_bar(selected_idx);
}

void desktop_init(void) {
    current_page = 0;
    selected_idx = -1;
    was_pressed = false;

    desktop_draw_wallpaper();
    desktop_scan_apps();
    desktop_draw_page(current_page, selected_idx);
    desktop_draw_bar(selected_idx);
}

bool desktop_tick(char* out_app_name) {
    bool is_pressed = mouse_btn_left();
    bool launched = false;

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
            uint8_t start_idx = current_page * APPS_PER_PAGE;
            uint8_t count = 0;
            if (start_idx < total_apps) {
                count = total_apps - start_idx;
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
                    int8_t global_idx = (int8_t)(start_idx + i);
                    if (selected_idx == global_idx) {
                        SD_FILE f;
                        if (sd_open(&f, "/SYSTEM/DESKTOP/launch.dat", SD_WRITE | SD_CREATE_ALWAYS)) {
                            sd_puts(&f, app_names[global_idx]);
                            sd_close(&f);
                        }
                        if (out_app_name != NULL) {
                            strcpy(out_app_name, app_names[global_idx]);
                        }
                        launched = true;
                    } else {
                        if (selected_idx >= (int8_t)start_idx && selected_idx < (int8_t)(start_idx + count)) {
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
            uint8_t start_idx = current_page * APPS_PER_PAGE;
            uint8_t count = 0;
            if (start_idx < total_apps) {
                count = total_apps - start_idx;
                if (count > APPS_PER_PAGE) {
                    count = APPS_PER_PAGE;
                }
            }
            if (selected_idx >= (int8_t)start_idx && selected_idx < (int8_t)(start_idx + count)) {
                desktop_draw_box_border((uint8_t)(selected_idx - start_idx), 0x00);
            }
            selected_idx = -1;
            desktop_draw_bar(-1);
        }
    }

    was_pressed = is_pressed;
    return launched;
}

bool desktop_shell(char* out_cmd) {
    desktop_init();

    char cwd[32];
    if (!sd_getcwd(cwd, sizeof(cwd))) {
        strcpy(cwd, "?");
    }
    print_str("uDOS:");
    print_str(cwd);
    print_str("> ");

    uint8_t len = 0;
    while (true) {
        char c = bios_getchar_nb();
        if (c != 0) {
            if (c == '\r' || c == '\n') {
                out_cmd[len] = '\0';
                bios_putchar('\r');
                bios_putchar('\n');
                return true;
            }
            if (c == '\b' || c == 8 || c == 127) {
                if (len > 0) {
                    len--;
                    bios_putchar('\b');
                    bios_putchar(' ');
                    bios_putchar('\b');
                }
            } else if (len < 63) {
                out_cmd[len++] = c;
                bios_putchar(c);
            }
        }

        if (desktop_tick(out_cmd)) {
            print_str(out_cmd);
            print_str("\r\n");
            return true;
        }
    }
}

int main(void) {
    char buf[64];
    return desktop_shell(buf) ? 0 : 1;
}

