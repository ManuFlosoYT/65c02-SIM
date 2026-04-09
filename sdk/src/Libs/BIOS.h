#ifndef BIOS_H
#define BIOS_H

#ifndef __fastcall__
#define __fastcall__
#endif

// Introduces a character from the serial port.
// Returns the character in A.
extern char __fastcall__ MONRDKEY(void);
#define bios_getchar MONRDKEY

// Outputs a character to the serial port.
// Expects the character in A.
extern void __fastcall__ MONCOUT(char c);
#define bios_putchar MONCOUT

// Reads a character without blocking (returns 0 if no key)
extern char __fastcall__ MONGETCHAR_NB(void);
#define bios_getchar_nb MONGETCHAR_NB

// Initializes the buffer.
extern void __fastcall__ INIT_BUFFER(void);

void print_str(const char* s);
void println(const char* s);
void read_line(char* buffer, int max_len);
void print_hex_byte(unsigned char v);
void print_hex(unsigned int n);
void print_num(unsigned int n);
void delay(unsigned int ms);

// Memoria Virtual Paginada
void os_load_app_page(unsigned char page_id);

#endif
