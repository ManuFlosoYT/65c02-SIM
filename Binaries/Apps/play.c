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

#define BUF_SIZE 512

static void do_delay(uint16_t loops) {
    volatile uint16_t i;
    // CC65 volatile 16-bit comparison and increment takes around 60-70 cycles.
    // The original ASM loop took exactly 15 cycles.
    // We divide loops by 4 to approximate the right time.
    loops >>= 2;
    for (i = 0; i < loops; i++) {
    }
}

int main(void) {
    SD_FILE file;
    uint8_t buffer[BUF_SIZE];
    uint16_t b_idx = 0;
    int b_len = 0;
    uint8_t cmd, l1, l2, val;
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
    
    sid_reset();

    while (1) {
        if (b_idx >= b_len) {
            b_len = sd_read(&file, buffer, BUF_SIZE);
            b_idx = 0;
            if (b_len == 0) break;
        }
        
        cmd = buffer[b_idx++];
        
        if (cmd < 0x20) {
            if (b_idx >= b_len) {
                b_len = sd_read(&file, buffer, BUF_SIZE);
                b_idx = 0;
                if (b_len == 0) break;
            }
            val = buffer[b_idx++];
            sid_write(cmd, val);
        } else if (cmd == 0x81) {
            if (b_idx < b_len) { l1 = buffer[b_idx++]; } 
            else { 
                b_len = sd_read(&file, buffer, BUF_SIZE); b_idx = 0; 
                if (b_len > 0) l1 = buffer[b_idx++]; else break;
            }
            
            if (b_idx < b_len) { l2 = buffer[b_idx++]; } 
            else { 
                b_len = sd_read(&file, buffer, BUF_SIZE); b_idx = 0; 
                if (b_len > 0) l2 = buffer[b_idx++]; else break;
            }
            
            loops = (uint16_t)l1 | ((uint16_t)l2 << 8);
            do_delay(loops);
        } else if (cmd == 0xFF) {
            break;
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
