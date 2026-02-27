#ifndef BIOS_H
#define BIOS_H

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

void print_str(const char* s) {
    while (*s) {
        if (*s == '\n') {
            /* Convert \n to \r\n for correct newline in terminal */
            bios_putchar('\r');
            bios_putchar('\n');
        } else {
            bios_putchar(*s);
        }
        s++;
    }
}

void println(const char* s) {
    print_str(s);
    bios_putchar('\n');
}

/* Function to read a string from the keyboard */
void read_line(char* buffer, int max_len) {
    char c;
    int i = 0;

    while (i < max_len - 1) {
        /* WAIT LOOP: */
        /* Continue asking for key while it is 0 (null) */
        do {
            c = bios_getchar();
        } while (c == 0);

        /* Detect Enter (carriage return \r or newline \n) */
        if (c == '\r' || c == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
            break;  // Exit loop if Enter is pressed
        }

        /* Save to buffer */
        buffer[i] = c;
        i++;
    }

    buffer[i] = '\0';
}

void print_hex_byte(unsigned char v) {
    const char hex[] = "0123456789ABCDEF";
    bios_putchar(hex[(v >> 4) & 0xF]);
    bios_putchar(hex[v & 0xF]);
}

void print_hex(unsigned int n) {
    const char hex_chars[] = "0123456789ABCDEF";
    bios_putchar('0');
    bios_putchar('x');
    if (n > 255) {
        bios_putchar(hex_chars[(n >> 12) & 0xF]);
        bios_putchar(hex_chars[(n >> 8) & 0xF]);
    }
    bios_putchar(hex_chars[(n >> 4) & 0xF]);
    bios_putchar(hex_chars[n & 0xF]);
}

void print_num(unsigned int n) {
    char buffer[10];
    int idx = 0;
    if (n == 0) {
        bios_putchar('0');
        return;
    }
    while (n > 0) {
        buffer[idx++] = (n % 10) + '0';
        n /= 10;
    }
    while (idx > 0) {
        bios_putchar(buffer[--idx]);
    }
}

/* Function: delay
 * Description: Generates a delay in milliseconds.
 * Parameters: ms (amount of milliseconds to wait)
 * IMPORTANT!!!!: This timing is only valid if the GPU is disabled.
 * If the GPU is enabled refer to GPU.h for the delay function.
 */
#define ITERATIONS_PER_MS 4000
void delay(unsigned int ms) {
    volatile unsigned int i;
    volatile unsigned int j;

    for (i = 0; i < ms; i++) {
        /* This inner loop consumes approx 1 millisecond */
        for (j = 0; j < ITERATIONS_PER_MS; j++) {
            /* Empty loop to waste cycles */
        }
    }
}

#endif
