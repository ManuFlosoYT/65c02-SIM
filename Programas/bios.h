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

// Inicializa el buffer.
extern void __fastcall__ INIT_BUFFER(void);

#endif
