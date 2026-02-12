#ifndef BIOS_H
#define BIOS_H

// Introduce un caracter desde el puerto serie.
// Devuelve el caracter en A.
extern char __fastcall__ MONRDKEY(void);
#define bios_getchar MONRDKEY

// Introduce un caracter desde el puerto serie.
// Espera un caracter en A.
extern void __fastcall__ MONCOUT(char c);
#define bios_putchar MONCOUT

// Lee un caracter sin bloquear (retorna 0 si no hay tecla)
extern char __fastcall__ MONGETCHAR_NB(void);
#define bios_getchar_nb MONGETCHAR_NB

// Inicializa el buffer.
extern void __fastcall__ INIT_BUFFER(void);

void print_str(const char* s) {
    while (*s) {
        if (*s == '\n') {
            /* Convertimos \n a \r\n para salto de línea correcto en terminal */
            bios_putchar('\r');
            bios_putchar('\n');
        } else {
            bios_putchar(*s);
        }
        s++;
    }
}

/* Función para leer una cadena de texto desde el teclado */
void read_line(char* buffer, int max_len) {
    char c;
    int i = 0;

    while (i < max_len - 1) {
        /* BUCLE DE ESPERA: */
        /* Seguimos pidiendo la tecla mientras sea 0 (null) */
        do {
            c = bios_getchar();
        } while (c == 0);

        /* Detectar Enter (retorno de carro \r o salto de línea \n) */
        if (c == '\r' || c == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
            break;  // Salir del bucle si se pulsa Enter
        }

        /* Echo: imprimir lo que se escribe */
        bios_putchar(c);

        /* Guardar en el buffer */
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

/* Función: delay
 * Descripción: Genera un retardo en milisegundos.
 * Parámetros: ms (cantidad de milisegundos a esperar)
 */
#define ITERACIONES_POR_MS 4000
void delay(unsigned int ms) {
    volatile unsigned int i;
    volatile unsigned int j;

    for (i = 0; i < ms; i++) {
        /* Este bucle interno consume aprox 1 milisegundo */
        for (j = 0; j < ITERACIONES_POR_MS; j++) {
            /* Bucle vacío para gastar ciclos */
        }
    }
}

#endif
