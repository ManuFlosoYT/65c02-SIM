#pragma once

#include <stdint.h>

void os_load_app_page(uint8_t page_id);
uint8_t os_swap_save(const char* name, void* addr, uint16_t size);
uint8_t os_swap_load(const char* name, void* addr, uint16_t size);
void* os_alloc(uint16_t size);
void os_free(void* ptr);
void os_clear_swap_blocks(void);

const char* os_get_msg(uint8_t msg_id);
