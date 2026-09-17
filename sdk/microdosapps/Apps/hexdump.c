#include "../Libs/app-bios.h"

int main(void) {
    SD_FILE f;
    char* filepath;
    uint8_t buffer[16];
    int16_t bytes_read;
    uint16_t i;
    uint32_t offset = 0;

    if (arg_count < 2) {
        println("Usage: hexdump <file>");
        return 1;
    }

    filepath = _args_ptr[1];

    if (!sd_open(&f, filepath, SD_READ)) {
        print_str("Error opening file: ");
        println(filepath);
        return 1;
    }

    while ((bytes_read = sd_read(&f, buffer, 16)) > 0) {
        /* Print offset using two print_hex_byte calls */
        print_str("0x");
        print_hex_byte((uint8_t)(offset >> 8));
        print_hex_byte((uint8_t)(offset & 0xFF));
        print_str("  ");

        /* Print hex values */
        for (i = 0; i < 16; i++) {
            if (i < bytes_read) {
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
        for (i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                bios_putchar(buffer[i]);
            } else {
                bios_putchar('.');
            }
        }
        println("|");
        
        offset += bytes_read;
    }

    sd_close(&f);
    return 0;
}
