#ifndef LCD_H
#define LCD_H

#define PORTB (*(volatile unsigned char*)0x6000)
#define DDRB (*(volatile unsigned char*)0x6002)

// Pin mapping according to LCD.cpp (Bit 6=E, Bit 5=RW, Bit 4=RS)
#define E 0x40
#define RW 0x20
#define RS 0x10

/*
   Helper to send 4 bits
   mode: 0 for Instruction, RS (0x10) for Data
   This function triggers the "WriteHook" in LCD.cpp
*/
void lcd_send_nibble(unsigned char data, unsigned char mode) {
    data &= 0x0F;  // Ensure only low 4 bits

    // Prepare data and signals (Set RW=0, E=1)
    PORTB = data | mode | E;

    // Falling edge (E=1 -> E=0)
    // This triggers the logic inside mem.SetWriteHook in LCD.cpp
    PORTB = data | mode;
}

/*
   Sends a command (RS=0)
*/
void lcd_instruction(unsigned char cmd) {
    lcd_send_nibble(cmd >> 4, 0);  // High nibble
    lcd_send_nibble(cmd, 0);       // Low nibble
}

/*
   Sends a character (RS=1)
   This triggers LCD::WriteCharToScreen in the simulator
*/
void lcd_print_char(unsigned char c) {
    lcd_send_nibble(c >> 4, RS);
    lcd_send_nibble(c, RS);
}

/*
   Triggers: LCD::HandleCommand with cmd 0x01
*/
void lcd_clear(void) { lcd_instruction(0x01); }

/*
   Triggers: LCD::HandleCommand with cmd 0x02
*/
void lcd_return_home(void) { lcd_instruction(0x02); }

/*
   Triggers: LCD::HandleCommand with cmd 0x06
*/
void lcd_entry_mode_increment(void) { lcd_instruction(0x06); }

/*
   Triggers: LCD::HandleCommand with cmd 0x0C
*/
void lcd_display_on_cursor_off(void) { lcd_instruction(0x0C); }

/*
   Triggers: LCD::HandleCommand with cmd 0x0E
*/
void lcd_display_on_cursor_on(void) { lcd_instruction(0x0E); }

/*
   Triggers: LCD::WriteCharToScreen with '\n', which resets cursorX and
   increments cursorY
*/
void lcd_newline(void) { lcd_print_char('\n'); }

void lcd_init(void) {
    // Configure pins as output
    DDRB = 0xFF;

    // Hardware initialization sequence (HD44780 Protocol)

    // 1. Function Set 8-bit (3 times to reset state)
    lcd_send_nibble(0x03, 0);
    lcd_send_nibble(0x03, 0);
    lcd_send_nibble(0x03, 0);

    // 2. Switch to 4-bit mode (Detected by LCD.cpp: if (!rs && data_nibble ==
    // 0x02))
    lcd_send_nibble(0x02, 0);

    // 3. Configure Function Set: 4-bit, 2 lines, font 5x8 (0x28)
    lcd_instruction(0x28);

    // 4. Display On, Cursor On, Blink Off (0x0E)
    lcd_instruction(0x0E);

    // 5. Entry Mode Set: Increment, No Shift (0x06)
    lcd_instruction(0x06);

    // 6. Clear Display using the new function
    lcd_clear();
}

void lcd_print(const char* str) {
    while (*str) {
        lcd_print_char(*str++);
    }
}

void lcd_print_number(unsigned int num) {
    unsigned char hundreds, remainder, tens, units;

    hundreds = num / 100;
    remainder = num % 100;
    tens = remainder / 10;
    units = remainder % 10;

    if (hundreds > 0) {
        lcd_print_char('0' + hundreds);
        lcd_print_char('0' + tens);
        lcd_print_char('0' + units);
    } else if (tens > 0) {
        lcd_print_char('0' + tens);
        lcd_print_char('0' + units);
    } else {
        lcd_print_char('0' + units);
    }
}

#endif