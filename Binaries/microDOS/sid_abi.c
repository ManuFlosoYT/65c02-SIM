#include <stdint.h>

#define SID_BASE 0x4800
#define SID_WRITE(offset, val) (*(volatile uint8_t*)(SID_BASE + offset) = (val))

void sid_write(uint8_t reg, uint8_t val) {
    SID_WRITE(reg, val);
}

void sid_reset(void) {
    uint8_t i;
    for (i = 0; i < 32; i++) {
        SID_WRITE(i, 0);
    }
}
