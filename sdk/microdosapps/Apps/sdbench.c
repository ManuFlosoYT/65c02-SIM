/*
 * sdbench.c — microDOS SD Card Benchmark
 * Uses VIA Timer 1 for cycle-accurate latency measurement.
 * Compile with: ./compile-bin.sh sdbench --microDOS
 */
#include <stdint.h>
#include "Libs/app-bios.h"

#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))

#define VIA_T1C_L  (*(volatile uint8_t*)0x6004)
#define VIA_T1C_H  (*(volatile uint8_t*)0x6005)
#define VIA_T1L_L  (*(volatile uint8_t*)0x6006)
#define VIA_T1L_H  (*(volatile uint8_t*)0x6007)
#define VIA_ACR    (*(volatile uint8_t*)0x600B)
#define VIA_IFR    (*(volatile uint8_t*)0x600D)

#define BUF_SIZE 512
#define NUM_SAMPLES 16

static SD_FILE file;
static uint8_t bench_buf[BUF_SIZE];

static void timer_start(void) {
    VIA_ACR &= ~0x40;
    VIA_IFR = 0x40;
    VIA_T1C_L = 0xFF;
    VIA_T1C_H = 0xFF;
}

static uint16_t timer_read(void) {
    uint8_t lo = VIA_T1C_L;
    uint8_t hi = VIA_T1C_H;
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

static uint16_t timer_elapsed(void) {
    return 0xFFFF - timer_read();
}

int main(void) {
    char** args = (char**)(uintptr_t)_args_ptr;
    uint16_t cycles, min_c, max_c;
    uint32_t total_c;
    uint16_t avg;
    uint16_t i, count;
    int bytes_read;

    println("== SD Benchmark ==");
    println("");

    if (arg_count < 2) {
        println("Usage: sdbench <file>");
        println("File should be >=16KB");
        return 1;
    }

    if (!sd_open(&file, args[1], SD_READ)) {
        print_str("ERR: ");
        println(args[1]);
        return 1;
    }

    println("[1] Timer overhead");
    timer_start();
    cycles = timer_elapsed();
    print_str("  Baseline: ");
    print_num(cycles);
    println(" cyc");

    sd_close(&file);

    // --- Test 1: Single 512B read latency ---
    println("");
    println("[2] sd_read 512B latency");

    if (!sd_open(&file, args[1], SD_READ)) {
        println("ERR: reopen");
        return 1;
    }

    min_c = 0xFFFF;
    max_c = 0;
    total_c = 0;
    count = 0;

    for (i = 0; i < NUM_SAMPLES; i++) {
        timer_start();
        bytes_read = sd_read(&file, bench_buf, BUF_SIZE);
        cycles = timer_elapsed();

        if (bytes_read <= 0) break;

        print_str("  #");
        print_num(i);
        print_str(": ");
        print_num(cycles);
        println(" cyc");

        total_c += cycles;
        if (cycles < min_c) min_c = cycles;
        if (cycles > max_c) max_c = cycles;
        count++;
    }

    sd_close(&file);

    if (count > 0) {
        avg = (uint16_t)(total_c / count);
        println("");
        print_str("  Min: ");
        print_num(min_c);
        println(" cyc");
        print_str("  Max: ");
        print_num(max_c);
        println(" cyc");
        print_str("  Avg: ");
        print_num(avg);
        println(" cyc");
    }

    // --- Test 2: Smaller read sizes ---
    println("");
    println("[3] Read size comparison");

    {
        static const uint16_t sizes[] = {64, 128, 256, 512};
        uint8_t s;

        for (s = 0; s < 4; s++) {
            if (!sd_open(&file, args[1], SD_READ)) break;

            total_c = 0;
            for (i = 0; i < 8; i++) {
                timer_start();
                bytes_read = sd_read(&file, bench_buf, sizes[s]);
                cycles = timer_elapsed();
                if (bytes_read <= 0) break;
                total_c += cycles;
            }
            sd_close(&file);

            if (i > 0) {
                avg = (uint16_t)(total_c / i);
                print_str("  ");
                print_num(sizes[s]);
                print_str("B avg: ");
                print_num(avg);
                println(" cyc");
            }
        }
    }

    // --- Test 3: Sequential burst ---
    println("");
    println("[4] Burst 32KB");

    if (!sd_open(&file, args[1], SD_READ)) {
        println("ERR: reopen");
        return 1;
    }

    total_c = 0;
    count = 0;
    timer_start();
    while (count < 64) {
        bytes_read = sd_read(&file, bench_buf, BUF_SIZE);
        if (bytes_read <= 0) break;
        count++;
    }
    cycles = timer_elapsed();
    sd_close(&file);

    print_str("  ");
    print_num(count);
    print_str(" reads, ");
    print_num(cycles);
    println(" cyc total");
    if (count > 0) {
        print_str("  Per-read: ");
        print_num(cycles / count);
        println(" cyc");
    }

    // --- Test 4: do_delay loop cost ---
    println("");
    println("[5] Loop calibration");
    {
        volatile uint16_t j;

        timer_start();
        for (j = 0; j < 500; j++) {}
        cycles = timer_elapsed();

        print_str("  500 loops = ");
        print_num(cycles);
        println(" cyc");

        timer_start();
        for (j = 0; j < 1000; j++) {}
        cycles = timer_elapsed();

        print_str("  1000 loops = ");
        print_num(cycles);
        println(" cyc");
    }

    // --- Summary ---
    println("");
    println("== SD_READ_COST ==");
    print_str("Recommended: ");

    if (!sd_open(&file, args[1], SD_READ)) {
        println("ERR");
        return 1;
    }

    total_c = 0;
    count = 0;
    for (i = 0; i < NUM_SAMPLES; i++) {
        timer_start();
        bytes_read = sd_read(&file, bench_buf, BUF_SIZE);
        cycles = timer_elapsed();
        if (bytes_read <= 0) break;
        total_c += cycles;
        count++;
    }
    sd_close(&file);

    if (count > 0) {
        avg = (uint16_t)(total_c / count);
        print_num(avg);
        println(" cycles");
        println("(use for SD_READ_COST)");
    }

    println("");
    println("== Done ==");
    return 0;
}
