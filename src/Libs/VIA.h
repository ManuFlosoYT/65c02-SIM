#ifndef VIA_H
#define VIA_H

#include <stdint.h>

// Registers
#define VIA_PORTB (*(volatile uint8_t*)0x6000)
#define VIA_PORTA (*(volatile uint8_t*)0x6001)
#define VIA_DDRB (*(volatile uint8_t*)0x6002)
#define VIA_DDRA (*(volatile uint8_t*)0x6003)
#define VIA_T1C_L (*(volatile uint8_t*)0x6004)
#define VIA_T1C_H (*(volatile uint8_t*)0x6005)
#define VIA_T1L_L (*(volatile uint8_t*)0x6006)
#define VIA_T1L_H (*(volatile uint8_t*)0x6007)
#define VIA_T2C_L (*(volatile uint8_t*)0x6008)
#define VIA_T2C_H (*(volatile uint8_t*)0x6009)
#define VIA_SR (*(volatile uint8_t*)0x600A)
#define VIA_ACR (*(volatile uint8_t*)0x600B)
#define VIA_PCR (*(volatile uint8_t*)0x600C)
#define VIA_IFR (*(volatile uint8_t*)0x600D)
#define VIA_IER (*(volatile uint8_t*)0x600E)
#define VIA_ORA_NH (*(volatile uint8_t*)0x600F)

// Helpers
static void via_set_timer1(uint16_t count) {
    VIA_T1C_L = count & 0xFF;
    VIA_T1C_H = (count >> 8) & 0xFF;
}

static void via_enable_interrupt(uint8_t mask) {
    VIA_IER = 0x80 | mask;  // Bit 7 = 1 (Set)
}

static void via_disable_interrupt(uint8_t mask) {
    VIA_IER = mask & 0x7F;  // Bit 7 = 0 (Clear)
}

static void via_ack_interrupt(uint8_t mask) { VIA_IFR = mask; }

// Wait for Timer 1 interrupt flag (Bit 6) and clear it
static void via_wait_frame() {
    while (!(VIA_IFR & 0x40));
    VIA_IFR = 0x40;
}

#endif
