/*
 * run_cmd.c — microDOS `run` command + transparent app fallback loader
 *
 * Format of a .app file:
 *   Byte 0-1: Magic  $55 $44  ("UD")
 *   Byte 2-3: Entry  little-endian offset from $0800 (normally 0x0004)
 *   Byte 4+:  Code, RODATA, DATA (loaded verbatim to $0800)
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

/* ---- Internal loader ---- */
static int load_and_run(const char* path) {
    SD_FILE fp;
    uint8_t header[4];
    uint8_t* dest = (uint8_t*)APP_LOAD_ADDR;
    int chunk;

    if (!sd_open(&fp, path, SD_READ)) {
        return 0;
    }

    /* Read and validate 4-byte header */
    if (sd_read(&fp, header, 4) != 4 || header[0] != 0x55 || header[1] != 0x44) {
        sd_close(&fp);
        print_str(M_ERR); println("Invalid App Format");
        return 1;
    }

    /* Stream code into RAM from byte 4 onward */
    while ((chunk = sd_read(&fp, dest, 128)) > 0) {
        dest += chunk;
    }
    sd_close(&fp);

    /* Forward shell args via ZP[$60...$62] */
    {
        uint16_t args_addr = (uint16_t)(uintptr_t)args;
        APP_ARGS_ZP[0] = (uint8_t)arg_count;
        APP_ARGS_ZP[1] = (uint8_t)(args_addr & 0xFF);
        APP_ARGS_ZP[2] = (uint8_t)(args_addr >> 8);
    }

    /* JSR to loaded app via function pointer */
    ((void(*)(void))APP_LOAD_ADDR)();

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
