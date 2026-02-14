#include "Include/VIA.h"
#include "Include/BIOS.h"

// Simple wait loop
void wait(long cycles) {
    while (cycles--) {
        asm("nop");
    }
}

int main() {
    // 1. Configure VIA
    // Disable all interrupts first
    asm("sei");
    via_disable_interrupt(0x7F);

    // Set ACR to 0 (One-shot mode for T1)
    VIA_ACR = 0x00;

    // Enable T1 Interrupt
    via_enable_interrupt(0x40);

    // 2. Start Timer 1 (e.g., 50000 cycles)
    // If Clock is 1MHz, 50000 -> 50ms
    via_set_timer1(50000U);

    // Output initial message
    print_str("Starting VIA Timer Test...\n");

    while (1) {
        if (VIA_IFR & 0x40) {
            // Timer expired!

            // Ack interrupt (read T1C_L or write to IFR)
            // Reading T1C_L clears T1 interrupt usually
            via_ack_interrupt(0x40);

            // Output message
            print_str("Timer Fired!\n");

            // Restart Timer
            via_set_timer1(50000U);
        }
    }

    return 0;
}
