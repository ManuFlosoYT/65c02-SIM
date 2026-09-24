#include "../Libs/app-bios.h"

static void print_hexdump_line(uint32_t offset, const uint8_t* buffer, uint16_t len) {
    uint16_t i;

    /* Print offset using two print_hex_byte calls */
    print_str("0x");
    print_hex_byte((uint8_t)(offset >> 8));
    print_hex_byte((uint8_t)(offset & 0xFF));
    print_str("  ");

    /* Print hex values */
    for (i = 0; i < 16; i++) {
        if (i < len) {
            print_hex_byte(buffer[i]);
            bios_putchar(' ');
        } else {
            print_str("   ");
        }
        if (i == 7) {
            bios_putchar(' '); /* Extra space in the middle */
        }
    }

    print_str(" |");
    
    /* Print ASCII representation */
    for (i = 0; i < len; i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) {
            bios_putchar(buffer[i]);
        } else {
            bios_putchar('.');
        }
    }
    println("|");
}

static int parse_hex16(const char* str, uint16_t* out_val, const char** end_ptr) {
    uint16_t val = 0;
    int parsed = 0;
    
    /* Skip optional 0x */
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            val = (val << 4) | (*str - '0');
        } else if (*str >= 'a' && *str <= 'f') {
            val = (val << 4) | (*str - 'a' + 10);
        } else if (*str >= 'A' && *str <= 'F') {
            val = (val << 4) | (*str - 'A' + 10);
        } else {
            break;
        }
        str++;
        parsed = 1;
    }
    
    if (out_val) *out_val = val;
    if (end_ptr) *end_ptr = str;
    
    return parsed;
}

int main(void) {
    SD_FILE f;
    char* arg;
    uint8_t buffer[16];
    int16_t bytes_read;
    uint32_t offset = 0;
    uint16_t start_addr, end_addr;
    const char* end_ptr;

    if (arg_count < 2) {
        println("Usage: hexdump <file | 0xADDR[-0xADDR]>");
        return 1;
    }

    arg = _args_ptr[1];

    if (arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
        if (!parse_hex16(arg, &start_addr, &end_ptr)) {
            println("Invalid address");
            return 1;
        }
        
        end_addr = start_addr;
        if (*end_ptr == '-') {
            if (!parse_hex16(end_ptr + 1, &end_addr, 0)) {
                println("Invalid end address");
                return 1;
            }
        }
        
        while (start_addr <= end_addr) {
            uint16_t i;
            uint32_t diff = (uint32_t)end_addr - (uint32_t)start_addr + 1;
            uint16_t chunk = (diff > 16) ? 16 : (uint16_t)diff;
            volatile uint8_t* ptr = (volatile uint8_t*)start_addr;
            
            for (i = 0; i < chunk; i++) {
                buffer[i] = ptr[i];
            }
            
            print_hexdump_line(start_addr, buffer, chunk);
            
            if (start_addr + chunk <= start_addr) {
                break;
            }
            start_addr += chunk;
        }
        
        return 0;
    }

    if (!sd_open(&f, arg, SD_READ)) {
        print_str("Error opening file: ");
        println(arg);
        return 1;
    }

    while ((bytes_read = sd_read(&f, buffer, 16)) > 0) {
        print_hexdump_line(offset, buffer, bytes_read);
        offset += bytes_read;
    }

    sd_close(&f);
    return 0;
}
