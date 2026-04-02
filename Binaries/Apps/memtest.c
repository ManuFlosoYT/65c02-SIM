/*
 * memtest.c — microDOS Memory Benchmark
 * Tests read/write speed and usable BSS size.
 * Compile with: ./compile-bin.sh memtest --microDOS
 */
#include <stdint.h>
#include "Libs/app-bios.h"

#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))

#define VIA_T1C_L  (*(volatile uint8_t*)0x6004)
#define VIA_T1C_H  (*(volatile uint8_t*)0x6005)
#define VIA_ACR    (*(volatile uint8_t*)0x600B)
#define VIA_IFR    (*(volatile uint8_t*)0x600D)

// Global to prevent compiler optimization on read loops
volatile uint8_t benchmark_sink;

static void timer_start(void) {
    VIA_ACR &= ~0x40;
    VIA_IFR = 0x40;
    VIA_T1C_L = 0xFF;
    VIA_T1C_H = 0xFF;
}

static uint16_t timer_elapsed(void) {
    uint8_t lo = VIA_T1C_L;
    uint8_t hi = VIA_T1C_H;
    return 0xFFFF - ((uint16_t)lo | ((uint16_t)hi << 8));
}

// Safe RAM region for testing (avoiding Code $1000, GPU $2000, BSS $6010)
#define MEM_START 0x4000
#define MEM_END   0x5000
#define STACK_RESERVE 512

static uint8_t canary_ok(uint16_t addr) {
    volatile uint8_t* p = (volatile uint8_t*)(uintptr_t)addr;
    uint8_t orig = *p;
    *p = 0xA5;
    if (*p != 0xA5) { *p = orig; return 0; }
    *p = 0x5A;
    if (*p != 0x5A) { *p = orig; return 0; }
    *p = orig;
    return 1;
}

int main(void) {
    uint16_t cycles;
    uint16_t addr, end_addr;
    uint16_t usable_bytes;
    volatile uint8_t* ptr;
    uint16_t i;

    println("== Memory Benchmark ==");
    println("");

    // --- Test 1: Usable RAM scan ---
    println("[1] RAM scan");
    print_str("  Range: $");
    print_hex_byte((uint8_t)(MEM_START >> 8));
    print_hex_byte((uint8_t)(MEM_START & 0xFF));
    print_str(" - $");
    print_hex_byte((uint8_t)(MEM_END >> 8));
    print_hex_byte((uint8_t)(MEM_END & 0xFF));
    println("");

    end_addr = MEM_START;
    for (addr = MEM_START; addr < MEM_END; addr++) {
        if (!canary_ok(addr)) break;
        end_addr = addr + 1;
    }

    usable_bytes = end_addr - MEM_START;
    print_str("  Usable: ");
    print_num(usable_bytes);
    println(" bytes");

    // --- Test 2: Sequential write speed ---
    println("");
    println("[2] Write speed (256B)");

    ptr = (volatile uint8_t*)MEM_START;
    timer_start();
    for (i = 0; i < 256; i++) {
        ptr[i] = (uint8_t)i;
    }
    cycles = timer_elapsed();
    print_str("  256B write: ");
    print_num(cycles);
    println(" cyc");

    print_str("  Per byte: ");
    print_num(cycles / 256);
    println(" cyc");

    // --- Test 3: Sequential read speed ---
    println("");
    println("[3] Read speed (256B)");

    {
        uint8_t j;
        timer_start();
        for (j = 0; j < 255; j++) {
            benchmark_sink = ptr[j];
        }
        benchmark_sink = ptr[255]; // Use full 256B
        cycles = timer_elapsed();
    }
    print_str("  256B read: ");
    print_num(cycles);
    println(" cyc");

    print_str("  Per byte: ");
    print_num(cycles / 256);
    println(" cyc");

    // --- Test 4: Bulk write 1KB ---
    println("");
    println("[4] Bulk write (1KB)");

    ptr = (volatile uint8_t*)MEM_START;
    timer_start();
    for (i = 0; i < 1024; i++) {
        ptr[i] = 0xAA;
    }
    cycles = timer_elapsed();
    print_str("  1KB write: ");
    print_num(cycles);
    println(" cyc");

    // --- Test 5: Bulk read 1KB ---
    println("");
    println("[5] Bulk read (1KB)");

    {
        timer_start();
        for (i = 0; i < 1024; i++) {
            benchmark_sink = ptr[i];
        }
        cycles = timer_elapsed();
    }
    print_str("  1KB read: ");
    print_num(cycles);
    println(" cyc");

    // --- Test 6: Buffer count estimation ---
    println("");
    println("[6] Buffer estimation");

    {
        static const uint16_t buf_sizes[] = {64, 128, 256};
        uint8_t s;
        uint16_t avail = usable_bytes - STACK_RESERVE;

        print_str("  Available: ");
        print_num(avail);
        println(" bytes");

        for (s = 0; s < 3; s++) {
            uint16_t count = avail / buf_sizes[s];
            print_str("  ");
            print_num(buf_sizes[s]);
            print_str("B bufs: ");
            print_num(count);
            println("");
        }
    }

    // --- Test 7: Paged Virtual Memory Window ($2000) ---
    println("");
    println("[7] Paged memory test");
    
    // Switch to page 0 (benchmarking SD -> RAM swap)
    print_str("  Swapping Page 0... ");
    timer_start();
    os_load_app_page(0);
    cycles = timer_elapsed();
    print_num(cycles);
    println(" cyc (swap)");

    // Window usability and speed
    ptr = (volatile uint8_t*)0x2000;
    timer_start();
    for (i = 0; i < 1024; i++) {
        ptr[i] = 0x55;
    }
    cycles = timer_elapsed();
    print_str("  $2000 window write: ");
    print_num(cycles);
    println(" cyc (1KB)");

    timer_start();
    for (i = 0; i < 1024; i++) {
        benchmark_sink = ptr[i];
    }
    cycles = timer_elapsed();
    print_str("  $2000 window read: ");
    print_num(cycles);
    println(" cyc (1KB)");

    println("");
    println("== Done ==");
    return 0;
}
