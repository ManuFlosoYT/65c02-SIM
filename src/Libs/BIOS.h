#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

char bios_getchar(void);
void bios_putchar(char c);
char bios_getchar_nb(void);
void INIT_BUFFER(void);

#define MONRDKEY bios_getchar
#define MONCOUT bios_putchar
#define MONGETCHAR_NB bios_getchar_nb

void print_str(const char* s);
void println(const char* s);
void read_line(char* buffer, size_t max_len);
void print_hex_byte(uint8_t v);
void print_hex(uint16_t n);
void print_num(uint16_t n);
void delay(uint16_t ms);

void os_load_app_page(uint8_t page_id);
uint8_t os_swap_save(const char* name, void* addr, uint16_t size);
uint8_t os_swap_load(const char* name, void* addr, uint16_t size);
void* os_alloc(uint16_t size);
void os_free(void* ptr);
void os_clear_swap_blocks(void);

const char* os_get_msg(uint8_t msg_id);
