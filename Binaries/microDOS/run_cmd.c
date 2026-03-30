/*
 * run_cmd.c — microDOS `run` command + transparent app fallback loader
 *
 * Format of a .app v3 file:
 *   Byte 0-3: Magic  "uDOS"
 *   Byte 4:   Version (3)
 *   Byte 5:   Flags
 *   Byte 6-7: Entry point absolute
 *   Byte 8-9: Core Size
 *   Byte 10-11: Page Size
 *   Byte 12-13: Page Window Addr
 *   Byte 14-15: Reserved
 */
#include "../Libs/BIOS.h"
#include "../Libs/SD.h"
#include "shell.h"
#include "msg.h"
#include "commands.h"
#include <string.h>

/* Shared area at a fixed ZP address; apps read their args from here.
   Layout: [arg_count : uint8] [arg_ptrs : char*[8]] */
#define APP_ARGS_ZP  ((volatile uint8_t*)0x60)

/* The app binary is loaded here */
#define APP_LOAD_ADDR 0x1000

SD_FILE active_app_file;
uint8_t app_hdr[16];

/* Syscall expuesta por la BIOS (JUMPTABLE $FFC3) */
void os_load_app_page(uint8_t page_id) {
    uint16_t core_size = *(uint16_t*)&app_hdr[8];
    uint16_t page_size = *(uint16_t*)&app_hdr[10];
    uint16_t window_addr = *(uint16_t*)&app_hdr[12];
    
    /* El offset será el tamaño del core residente (que ya incluye offset 0-15 del header) + N páginas */
    uint32_t file_offset = core_size + ((uint32_t)page_id * page_size);
    
    sd_seek(&active_app_file, file_offset);
    sd_read(&active_app_file, (void*)(uintptr_t)window_addr, page_size);
}

/* ---- Internal loader ---- */
static int load_and_run(const char* path) {
    uint16_t core_size;
    uint16_t entry_point;

    if (!sd_open(&active_app_file, path, SD_READ)) {
        return 0;
    }

    /* Read and validate 16-byte header */
    if (sd_read(&active_app_file, app_hdr, 16) != 16 || 
        app_hdr[0] != 'u' || app_hdr[1] != 'D' || app_hdr[2] != 'O' || app_hdr[3] != 'S' ||
        app_hdr[4] != 3) {
        sd_close(&active_app_file);
        print_str(M_ERR); print_str("Invalid App Format / Not v3. Magic: ");
        print_hex_byte(app_hdr[0]); print_str(" ");
        print_hex_byte(app_hdr[1]); print_str(" ");
        print_hex_byte(app_hdr[2]); print_str(" ");
        print_hex_byte(app_hdr[3]); print_str(" v");
        print_hex_byte(app_hdr[4]); println("");
        return 1;
    }

    core_size = *(uint16_t*)&app_hdr[8];
    entry_point = *(uint16_t*)&app_hdr[6];

    /* Rewind and load Resident Core into APP_LOAD_ADDR */
    sd_seek(&active_app_file, 0);
    if (sd_read(&active_app_file, (void*)APP_LOAD_ADDR, core_size) != core_size) {
        sd_close(&active_app_file);
        print_str(M_ERR); println("Error reading Resident Core");
        return 1;
    }

    /* Forward shell args via ZP[$60...$62] */
    {
        uint16_t args_addr = (uint16_t)(uintptr_t)args;
        APP_ARGS_ZP[0] = (uint8_t)arg_count;
        APP_ARGS_ZP[1] = (uint8_t)(args_addr & 0xFF);
        APP_ARGS_ZP[2] = (uint8_t)(args_addr >> 8);
    }

    /* JSR to loaded app via its absolute entry point */
    ((void(*)(void))(uintptr_t)entry_point)();

    /* We return here after app finishes. Close the virtual memory backend file */
    sd_close(&active_app_file);

    return 1;
}

/* ---- cmd_run: explicit `run <path>` command ---- */
void cmd_run(void) {
    char path[64];

    if (arg_count < 2) {
        print_str(M_USE); println("run <app>");
        return;
    }

    if (args[1][0] == '/' || args[1][0] == '.') {
        strncpy(path, args[1], 63);
        path[63] = '\0';
    } else {
        strcpy(path, "/bin/");
        strncat(path, args[1], 57);
        strcat(path, ".app");
    }

    if (!load_and_run(path)) {
        print_str(M_ERR); println(path);
    }
}

/* ---- try_run_app: fallback used by exec_command() ---- */
int try_run_app(const char* name) {
    char path[32];
    strcpy(path, "/bin/");
    strncat(path, name, 24);
    strcat(path, ".app");

    if (!sd_exists(path)) return 0;
    return load_and_run(path);
}
