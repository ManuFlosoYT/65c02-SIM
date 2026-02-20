# Linker y Mapa de Memoria

**Directorio:** `Linker/`

## Descripción general

El directorio `Linker/` contiene los archivos de configuración del enlazador (`ld65` de la suite **cc65**), el BIOS del sistema y el intérprete Microsoft BASIC. Estos archivos definen cómo se organiza el código y los datos en el espacio de direcciones de 64 KB del 65c02.

## Mapa de memoria completo

```
0x0000 – 0x00FF   Página cero (Zero Page) — variables de acceso rápido
0x0100 – 0x01FF   Pila (Stack)
0x0200 – 0x02FF   (libre)
0x0300 – 0x03FF   INPUT_BUFFER — búfer circular de entrada BIOS
0x0400 – 0x47FF   RAM_1 — variables C y heap principal
0x4800 – 0x481F   SID — 32 registros del chip de sonido
0x4820 – 0x5FFF   RAM_2 — memoria RAM adicional
0x5000 – 0x5003   ACIA — registros de comunicación serie (dentro de RAM_2)
0x6000 – 0x600F   VIA — 16 registros de E/S
0x6010 – 0x7FFF   RAM_3 — memoria RAM adicional
0x8000 – 0xFFF9   ROM — código del programa + BASIC + BIOS
0xFFFA – 0xFFFB   Vector NMI
0xFFFC – 0xFFFD   Vector RESET
0xFFFE – 0xFFFF   Vector IRQ/BRK
```

> Con GPU habilitada, `0x2000`–`0x3FFF` (dentro de RAM_1) se usa como VRAM.

## Archivos de configuración del enlazador

### `C-Runtime.cfg` — Runtime C estándar

Configuración por defecto para programas en C. Define:
- Página cero para variables ZP del runtime cc65
- Stack C en `$7000`
- Código en ROM (`$8000`–`$FFFA`)
- Segmentos `DATA` (copiado de ROM a RAM en el arranque), `BSS`, `CODE`, `RODATA`, `BIOS`

### `C-Runtime-GPU.cfg` — Runtime C con GPU

Igual que `C-Runtime.cfg` pero reserva `0x2000`–`0x3FFF` como VRAM para la GPU. El heap C comienza después de la VRAM.

### `C-Runtime-GPUDoubleBuffer.cfg` — GPU con doble búfer

Igual que `C-Runtime-GPU.cfg` pero divide la VRAM en dos mitades para double-buffering (elimina el tearing en animaciones).

### `raw.cfg` — Ensamblador puro

Configuración mínima para programas en ensamblador sin runtime C. El código comienza directamente en `$8000` y los vectores de interrupción están en `$FFFA`–`$FFFF`.

## BIOS (`bios.s`)

El BIOS proporciona rutinas básicas de entrada/salida que son usadas tanto por Microsoft BASIC como por los programas en C a través del runtime cc65.

### Rutinas exportadas

| Símbolo | Descripción |
|---------|-------------|
| `MONRDKEY` / `CHRIN` | Lee un carácter del búfer de entrada (bloqueante) |
| `MONGETCHAR_NB` | Lee un carácter del búfer sin bloquear |
| `MONPEEK` | Inspecciona el siguiente carácter sin extraerlo |
| `MONCOUT` / `CHROUT` | Envía un carácter por la ACIA (consola) |
| `INIT_BUFFER` | Inicializa el búfer circular de entrada |
| `IRQ_HANDLER` | Manejador de interrupciones (recibe bytes por ACIA) |
| `LOAD` / `SAVE` | Stubs vacíos (para compatibilidad con BASIC) |

### Búfer circular de entrada

El BIOS implementa un **búfer circular de 256 bytes** en `$0300`–`$03FF`:
- `WRITE_PTR` (`$00`): índice de escritura (ISR → buffer)
- `READ_PTR` (`$01`): índice de lectura (programa → buffer)
- La ISR (`IRQ_HANDLER`) escribe cada byte recibido por ACIA en el búfer
- `MONRDKEY` lee del búfer de forma bloqueante
- Cuando el búfer está casi lleno (`>= 0xB0`), activa el control de flujo por hardware (bit 0 de PORTA)

### Rutina de salida `MONCOUT`

```asm
MONCOUT:
    pha
    sta ACIA_DATA       ; Enviar carácter por serie
    lda #$FF
@txdelay:
    dec                 ; Retardo para simular el baud rate
    bne @txdelay
    pla
    rts
```

## C-Runtime (`C-Runtime.s`)

Código de inicialización del runtime C para cc65. Se ejecuta antes de `main()` y se encarga de:
1. Copiar el segmento `DATA` de ROM a RAM
2. Limpiar el segmento `BSS` (poner a cero)
3. Inicializar el búfer de entrada BIOS
4. Llamar a los constructores globales de C++
5. Llamar a `main()`

## WozMon (`wozmon.s`)

Monitor de sistema Apple 1 escrito originalmente por **Steve Wozniak** (© 1976 Apple Computer, Inc.). Permite inspeccionar y modificar la memoria directamente desde una consola serie.

## Microsoft BASIC (`msbasic/`)

Directorio con el puerto de **Microsoft BASIC para 6502** (© 1977 Microsoft), basado en las restauraciones de [mist64](https://github.com/mist64/msbasic) y [Ben Eater](https://github.com/beneater/msbasic).

- Compilar con `./msbasic/make.sh`
- El binario resultante se carga en ROM como un programa de ejemplo
- Permite escribir programas BASIC directamente en la consola del emulador

## Herramienta de compilación

La suite cc65 se usa para compilar y enlazar programas:

```bash
# Ensamblar
ca65 program.s -o program.o

# Enlazar con el runtime C
ld65 -C Linker/C-Runtime.cfg program.o Linker/C-Runtime.o -o output/rom/program.bin
```

El script `compile-bin.sh` automatiza todo este proceso. Consulta la página [SDK](SDK) para más detalles.
