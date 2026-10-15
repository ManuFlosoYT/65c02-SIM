#pragma once

#include <stdbool.h>
#include <stdint.h>

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

static inline void via_set_timer1(uint16_t count) {
    VIA_T1C_L = (uint8_t)(count & 0xFFU);
    VIA_T1C_H = (uint8_t)((count >> 8U) & 0xFFU);
}

static inline void via_enable_interrupt(uint8_t mask) {
    VIA_IER = (uint8_t)(0x80U | mask);
}

static inline void via_disable_interrupt(uint8_t mask) {
    VIA_IER = (uint8_t)(mask & 0x7FU);
}

static inline void via_ack_interrupt(uint8_t mask) {
    VIA_IFR = mask;
}

static inline void via_wait_frame(void) {
    while (!(VIA_IFR & 0x40U)) {
    }
    VIA_IFR = 0x40U;
}
