#include "../Libs/app-bios.h"

static void fill_data(uint8_t* ptr, uint16_t size, uint8_t seed) {
    uint16_t i;
    for(i = 0; i < size; i++) {
        ptr[i] = (uint8_t)(seed + (i & 0xFF));
    }
}

static uint8_t verify_data(uint8_t* ptr, uint16_t size, uint8_t seed) {
    uint16_t i;
    for(i = 0; i < size; i++) {
        if (ptr[i] != (uint8_t)(seed + (i & 0xFF))) {
            print_str("Mismatch at ");
            print_num(i);
            print_str(": expected ");
            print_hex_byte((uint8_t)(seed + (i & 0xFF)));
            print_str(", got ");
            print_hex_byte(ptr[i]);
            println("");
            return 0;
        }
    }
    return 1;
}

int main(void) {
    uint8_t* p1;
    uint8_t* p2;

    println("--- testmem ---");

    p1 = (uint8_t*)os_alloc(4000);
    p2 = (uint8_t*)os_alloc(4000);

    if (!p1 || !p2) {
        println("Alloc failed.");
        return 1;
    }

    fill_data(p1, 4000, 0x11);
    fill_data(p2, 4000, 0x22);

    println("Verifying p1...");
    if (!verify_data(p1, 4000, 0x11)) {
        println("p1 corrupted!");
    } else {
        println("p1 OK");
    }

    println("Verifying p2...");
    if (!verify_data(p2, 4000, 0x22)) {
        println("p2 corrupted!");
    } else {
        println("p2 OK");
    }

    os_free(p1);
    os_free(p2);
    
    return 0;
}
