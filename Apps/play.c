/*
 * play.c — microDOS SID Player (raw stream playback)
 * Compile with: ./compile-bin.sh play --microDOS
 */
#include <stdint.h>
#include <string.h>

#include "Libs/app-bios.h"

// Argument mapping from microDOS init wrapper


#define CHUNK_SIZE 512
#define SD_CHUNK_COST 1444
#define SID_WRITE_CREDIT 4

extern char _CORE_SIZE__[];

static void do_delay(uint16_t loops) {
    volatile uint16_t i;
    loops >>= 2;
    for (i = 0; i < loops; i++) {
    }
}

static SD_FILE file;
static uint8_t* buffer_pool;
static uint8_t num_bufs;
#define BUF_PTR(idx) (buffer_pool + ((uint16_t)(idx) << 9))
static uint8_t head = 0;
static uint8_t tail = 0;
static uint8_t count = 0;
static uint16_t b_idx = 0;
static uint16_t credit = 0;
static uint8_t eof_reached = 0;
static int8_t r = 0;
static uint8_t current_waves[3] = {0, 0, 0};

static uint8_t next_byte(void) {
    if (b_idx >= CHUNK_SIZE) {
        tail = (tail + 1) % num_bufs;
        count--;
        b_idx = 0;
    }
    if (count == 0) {
        if (eof_reached) return 0xFF;
        r = sd_read(&file, BUF_PTR(head), CHUNK_SIZE);
        if (r <= 0) {
            eof_reached = 1;
            return 0xFF;
        }
        head = (head + 1) % num_bufs;
        count++;
    }
    return BUF_PTR(tail)[b_idx++];
}

int main(void) {
    uint8_t cmd, l1, l2, val, i;
    char path[32];
    char** args = _args_ptr;
    uint8_t pathp, argp;
    uint16_t loops;

    uint16_t core_size = (uint16_t)_CORE_SIZE__;

    println("microDOS SID Player");
    
    buffer_pool = (uint8_t*)(APP_SAFE_RAM_START + core_size);
    num_bufs = (APP_SAFE_RAM_END - (uint16_t)buffer_pool + 1) / CHUNK_SIZE;

    if (num_bufs == 0) {
        println("Error: Not enough RAM for buffers");
        return 1;
    }
    
    if (arg_count < 2) {
        println("Usage: play <file.sid>");
        return 1;
    }

    if (sd_open(&file, args[1], SD_READ) == 0) {
        strcpy(path, "/sid/");
        pathp = 5;
        argp = 0;

        while (args[1][argp] && pathp < 28) {
            path[pathp++] = args[1][argp++];
        }
        path[pathp] = '\0';

        if (pathp > 4 &&
            !(path[pathp - 4] == '.' && path[pathp - 3] == 's' && path[pathp - 2] == 'i' && path[pathp - 1] == 'd')) {
            path[pathp++] = '.';
            path[pathp++] = 's';
            path[pathp++] = 'i';
            path[pathp++] = 'd';
            path[pathp] = '\0';
        }

        if (sd_open(&file, path, SD_READ) == 0) {
            print_str("Error: File not found -> ");
            println(args[1]);
            return 1;
        }
    }

    print_str("Playing -> ");
    println(args[1]);

    head = 0;
    tail = 0;
    count = 0;
    b_idx = 0;
    credit = 0;
    for (i = 0; i < num_bufs; i++) {
        if (sd_read(&file, BUF_PTR(head), CHUNK_SIZE) <= 0) break;
        head = (head + 1) % num_bufs;
        count++;
    }

    sid_reset();

    while (1) {
        if (!eof_reached && count < num_bufs && credit >= SD_CHUNK_COST) {
            if (sd_read(&file, BUF_PTR(head), CHUNK_SIZE) > 0) {
                head = (head + 1) % num_bufs;
                count++;
                credit -= SD_CHUNK_COST;
            } else {
                eof_reached = 1;
            }
        }

        cmd = next_byte();
        if (cmd == 0xFF) break;

        if (cmd < 0x20) {
            val = next_byte();
            if (val == 0xFF && count == 0) break;
            sid_write(cmd, val);
            credit += SID_WRITE_CREDIT;
        } else if (cmd >= 0x90 && cmd <= 0x92) {
            uint8_t voice = cmd - 0x90 + 1;
            uint8_t f_lo = next_byte();
            uint8_t f_hi = next_byte();
            uint8_t ctrl = next_byte();
            current_waves[voice - 1] = ctrl;
            sid_trigger_note(voice, f_lo | ((uint16_t)f_hi << 8), ctrl);
            credit += SID_WRITE_CREDIT * 3;
        } else if (cmd >= 0x93 && cmd <= 0x95) {
            uint8_t voice = cmd - 0x93 + 1;
            uint8_t ctrl = current_waves[voice - 1] & ~0x01;
            uint8_t offset = (voice - 1) * 7;
            sid_write(offset + 0x04, ctrl);
            credit += SID_WRITE_CREDIT;
        } else if (cmd == 0x81 || cmd == 0x82) {
            l1 = next_byte();
            if (cmd == 0x81) {
                l2 = next_byte();
                loops = (uint16_t)l1 | ((uint16_t)l2 << 8);
            } else {
                loops = (uint16_t)l1;
            }

            if (credit < 0xFFFF - loops)
                credit += loops;
            else
                credit = 0xFFFF;

            while (!eof_reached && count < num_bufs && credit >= SD_CHUNK_COST) {
                if (sd_read(&file, BUF_PTR(head), CHUNK_SIZE) <= 0) {
                    eof_reached = 1;
                    break;
                }
                head = (head + 1) % num_bufs;
                count++;
                credit -= SD_CHUNK_COST;
            }

            do_delay(loops);
        } else {
            println("Stream warning: Unrecognized byte.");
            break;
        }
    }

    sd_close(&file);
    sid_reset();
    println("Playback finished.");
    return 0;
}
