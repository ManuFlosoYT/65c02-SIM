#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "Libs/System.h"
#include "shell.h"
#include "msg.h"
#include "commands.h"

#define APP_ARGS_ZP  ((volatile uint8_t*)0x60)

enum {
    APP_LOAD_ADDR = 0x0200
};

SD_FILE active_app_file;
uint8_t app_hdr[16];
static char app_path[48];

void os_load_app_page(uint8_t page_id) {
    const uint16_t core_size = *(uint16_t*)&app_hdr[8];
    const uint16_t page_size = *(uint16_t*)&app_hdr[10];
    const uint16_t window_addr = *(uint16_t*)&app_hdr[12];

    uint32_t file_offset = core_size;
    while (page_id--) {
        file_offset += page_size;
    }

    sd_seek(&active_app_file, file_offset);
    sd_read(&active_app_file, (void*)(uintptr_t)window_addr, page_size);
}

static void build_bin_path(const char* name) {
    if (name[0] == '/' || name[0] == '.') {
        strcpy(app_path, name);
    } else {
        strcpy(app_path, "/bin/");
        strcat(app_path, name);
        strcat(app_path, ".app");
    }
}

static bool load_and_run(const char* path) {
    if (!sd_open(&active_app_file, path, SD_READ)) {
        return false;
    }

    if (sd_read(&active_app_file, app_hdr, 16) != 16 ||
        app_hdr[0] != 'u' || app_hdr[1] != 'D' || app_hdr[2] != 'O' || app_hdr[3] != 'S' ||
        app_hdr[4] != 3) {
        sd_close(&active_app_file);
        print_str(M_ERR); print_str(os_get_msg(20));
        for (uint8_t i = 0; i < 4; ++i) {
            bios_putchar((char)app_hdr[i]);
        }
        print_str(os_get_msg(21)); print_num(app_hdr[4]); println("");
        return true;
    }

    const uint16_t core_size = *(uint16_t*)&app_hdr[8];
    const uint16_t entry_point = *(uint16_t*)&app_hdr[6];

    sd_seek(&active_app_file, 0);
    if (sd_read(&active_app_file, (void*)APP_LOAD_ADDR, core_size) != core_size) {
        sd_close(&active_app_file);
        print_str(M_ERR); println(os_get_msg(22));
        return true;
    }

    {
        const uint16_t args_addr = (uint16_t)(uintptr_t)args;
        APP_ARGS_ZP[0] = (uint8_t)arg_count;
        APP_ARGS_ZP[1] = (uint8_t)(args_addr & 0xFF);
        APP_ARGS_ZP[2] = (uint8_t)(args_addr >> 8);
    }

    const int ret_code = ((int(*)(void))(uintptr_t)entry_point)();

    sd_close(&active_app_file);

    if (ret_code == 0) {
        os_clear_swap_blocks();
    } else {
        println("[E] App error");
    }

    return true;
}

void cmd_run(void) {
    if (arg_count < 2) {
        print_str(M_USE); println("run <app>");
        return;
    }

    build_bin_path(args[1]);
    if (!load_and_run(app_path)) {
        print_str(M_ERR); print_str(os_get_msg(23)); println(app_path);
    }
}

bool try_run_app(const char* name) {
    build_bin_path(name);
    if (!sd_exists(app_path)) {
        return false;
    }
    return load_and_run(app_path);
}