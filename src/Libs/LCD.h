#pragma once

#include <stdint.h>

#ifndef PORTB
#define PORTB (*(volatile uint8_t*)0x6000)
#endif

#ifndef DDRB
#define DDRB (*(volatile uint8_t*)0x6002)
#endif

#define E 0x40u
#define RW 0x20u
#define RS 0x10u

static inline void lcd_send_nibble(uint8_t data, uint8_t mode) {
    data &= 0x0Fu;
    PORTB = (uint8_t)(data | mode | E);
    PORTB = (uint8_t)(data | mode);
}

static inline void lcd_instruction(uint8_t cmd) {
    lcd_send_nibble((uint8_t)(cmd >> 4u), 0u);
    lcd_send_nibble(cmd, 0u);
}

static inline void lcd_print_char(char c) {
    lcd_send_nibble((uint8_t)((uint8_t)c >> 4u), RS);
    lcd_send_nibble((uint8_t)c, RS);
}

static inline void lcd_clear(void) {
    lcd_instruction(0x01u);
}

static inline void lcd_return_home(void) {
    lcd_instruction(0x02u);
}

static inline void lcd_entry_mode_increment(void) {
    lcd_instruction(0x06u);
}

static inline void lcd_display_on_cursor_off(void) {
    lcd_instruction(0x0Cu);
}

static inline void lcd_display_on_cursor_on(void) {
    lcd_instruction(0x0Eu);
}

static inline void lcd_newline(void) {
    lcd_print_char('\n');
}

static inline void lcd_init(void) {
    DDRB = 0xFFu;

    lcd_send_nibble(0x03u, 0u);
    lcd_send_nibble(0x03u, 0u);
    lcd_send_nibble(0x03u, 0u);

    lcd_send_nibble(0x02u, 0u);

    lcd_instruction(0x28u);
    lcd_instruction(0x0Eu);
    lcd_instruction(0x06u);
    lcd_clear();
}

static inline void lcd_print(const char* str) {
    while (*str) {
        lcd_print_char(*str++);
    }
}

static inline void lcd_print_number(uint16_t num) {
    uint8_t hundreds = (uint8_t)(num / 100u);
    uint8_t remainder = (uint8_t)(num % 100u);
    uint8_t tens = (uint8_t)(remainder / 10u);
    uint8_t units = (uint8_t)(remainder % 10u);

    if (hundreds > 0u) {
        lcd_print_char((char)('0' + hundreds));
        lcd_print_char((char)('0' + tens));
        lcd_print_char((char)('0' + units));
    } else if (tens > 0u) {
        lcd_print_char((char)('0' + tens));
        lcd_print_char((char)('0' + units));
    } else {
        lcd_print_char((char)('0' + units));
    }
}