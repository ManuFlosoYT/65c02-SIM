#include <stdbool.h>
#include <stdint.h>

#include "Libs/BIOS.h"
#include "Libs/VIA.h"

void wait(int32_t cycles) {
    while (cycles--) {
        asm("nop");
    }
}

int main(void) {
    asm("sei");
    via_disable_interrupt(0x7FU);

    VIA_ACR = 0x00U;

    via_enable_interrupt(0x40U);

    via_set_timer1(50000U);

    print_str("Starting VIA Timer Test...\n");

    while (true) {
        if ((VIA_IFR & 0x40U) != 0U) {
            via_ack_interrupt(0x40U);

            print_str("Timer Fired!\n");

            via_set_timer1(50000U);
        }
    }

    return 0;
}
