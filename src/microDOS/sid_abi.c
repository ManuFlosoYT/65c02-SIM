#include <stdint.h>

enum {
    SID_BASE = 0x4800
};

void sid_write(uint8_t reg, uint8_t val) {
    *(volatile uint8_t*)(SID_BASE + reg) = val;
}

void sid_reset(void) {
    for (uint8_t i = 0; i < 32; ++i) {
        sid_write(i, 0);
    }
}
