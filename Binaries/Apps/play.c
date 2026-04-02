/*
 * play.c — microDOS SID Player (raw stream playback)
 * Compile with: ./compile-bin.sh play --microDOS
 */
#include <stdint.h>
#include <string.h>
#include "Libs/app-bios.h"

// Argument mapping from microDOS init wrapper
#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))

#define BUF_SIZE 256
#define SD_READ_COST 928

static void do_delay(uint16_t loops) {
    volatile uint16_t i;
    loops >>= 2;
    for (i = 0; i < loops; i++) {
    }
}

// Global buffer variables to avoid heavy stack usage
static SD_FILE file;
static uint8_t buffers[2][BUF_SIZE];
static int b_len[2] = {0, 0};
static uint16_t b_idx = 0;
static uint8_t active_buf = 0;
static uint16_t credit = 0;

static uint8_t next_byte(void) {
    if (b_idx >= b_len[active_buf]) {
        // Active buffer drained
        b_len[active_buf] = 0; 
        active_buf ^= 1; // Swap
        b_idx = 0;
        
        // If the new active buffer is empty, force sync load
        if (b_len[active_buf] == 0) {
            b_len[active_buf] = sd_read(&file, buffers[active_buf], BUF_SIZE);
            if (b_len[active_buf] <= 0) return 0xFF; // EOF signal
        }
    }
    return buffers[active_buf][b_idx++];
}

int main(void) {
    uint8_t cmd, l1, l2, val, inactive_buf;
    char path[32];
    char** args = (char**)(uintptr_t)_args_ptr;
    uint8_t pathp, argp;
    uint16_t loops;

    println("microDOS SID Player");
    if (arg_count < 2) {
        println("Usage: play <file.sid>");
        return 1;
    }

    if (sd_open(&file, args[1], SD_READ) == 0) {
        strcpy(path, "/sid/");
        pathp = 5;
        argp = 0;
        
        while(args[1][argp] && pathp < 28) {
            path[pathp++] = args[1][argp++];
        }
        path[pathp] = '\0';
        
        if (pathp > 4 && 
           !(path[pathp-4] == '.' && path[pathp-3] == 's' && 
             path[pathp-2] == 'i' && path[pathp-1] == 'd')) {
            path[pathp++] = '.'; path[pathp++] = 's';
            path[pathp++] = 'i'; path[pathp++] = 'd';
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
    
    // Initial Pre-loading of double buffer
    b_len[0] = sd_read(&file, buffers[0], BUF_SIZE);
    b_len[1] = sd_read(&file, buffers[1], BUF_SIZE);
    active_buf = 0;
    b_idx = 0;
    
    sid_reset();

    while (1) {
        cmd = next_byte();
        if (cmd == 0xFF) break;
        
        if (cmd < 0x20) {
            val = next_byte();
            if (val == 0xFF && b_len[active_buf] <= 0) break;
            sid_write(cmd, val);
        } else if (cmd == 0x81) {
            l1 = next_byte();
            l2 = next_byte();
            loops = (uint16_t)l1 | ((uint16_t)l2 << 8);
            
            inactive_buf = active_buf ^ 1;

            if (credit < 0xFFFF - loops)
                credit += loops;
            else
                credit = 0xFFFF;

            if (b_len[inactive_buf] == 0 && credit >= SD_READ_COST) {
                b_len[inactive_buf] = sd_read(&file, buffers[inactive_buf], BUF_SIZE);
                credit -= SD_READ_COST;
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
