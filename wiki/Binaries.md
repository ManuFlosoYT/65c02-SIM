# Binaries — Programas de ejemplo

**Directorio:** `Binaries/`

## Descripción general

El directorio `Binaries/` contiene programas de ejemplo escritos en **C** y **ensamblador 65c02** que demuestran las capacidades del emulador. Son la base ideal para desarrollar nuevos programas.

Todos los programas se compilan con `./compile-bin.sh <nombre>` y el binario resultante se guarda en `output/rom/<nombre>.bin`.

---

## Programas en C

### `echo.c` — Echo serie básico
Programa de referencia mínimo. Lee caracteres de la entrada serie (ACIA) y los reenvía de vuelta a la consola. Útil como plantilla inicial para nuevos proyectos.

### `fullTest.c` — Test completo
Prueba de integración que ejercita múltiples componentes del hardware a la vez: ACIA, VIA, timers, etc. Útil para verificar que el emulador funciona correctamente.

### `testVIA.c` — Test del VIA 6522
Demuestra el uso de los timers y puertos de E/S del chip VIA. Incluye ejemplos de configuración de Timer 1 (modo continuo) y Timer 2 (one-shot).

### `testGPU.c` — Test de la GPU
Demuestra cómo escribir píxeles en la VRAM y mostrar imágenes o gráficos en la ventana **VRAM Viewer**. Requiere la librería `Libs/GPU.h`.

### `testSID.c` — Test del SID
Demuestra el uso del chip de sonido: configuración de osciladores, formas de onda y envolventes ADSR.

### `fillVRAM.c` — Relleno de VRAM
Rellena toda la VRAM con un patrón de píxeles. Útil para probar el rendimiento de escritura gráfica y la correcta sincronización con el blanking interval de la GPU.

### `mat.c` — Multiplicación de matrices (8 bits)
Implementa multiplicación de matrices usando enteros de 8 bits. Demuestra el rendimiento del CPU para operaciones matemáticas y el uso de arrays en memoria.

### `mat64.c` — Multiplicación de matrices (64 bits)
Igual que `mat.c` pero con enteros de 64 bits emulados en software. Útil para medir el rendimiento del 65c02 en operaciones de precisión extendida.

### `cube.c` — Cubo 3D rotante
Renderiza un cubo 3D rotante en la VRAM usando matemáticas de punto fijo. Requiere la librería `Libs/GPU.h`. Excelente demostración de gráficos en tiempo real.

### `room.c` — Escena 3D
Renderiza una escena 3D simple en la VRAM usando ray casting o técnica similar. Requiere `Libs/GPU.h`.

### `game.c` — Juego
Juego interactivo que usa la entrada serie y la GPU para renderizar. Demuestra el bucle de juego clásico: entrada → actualización → renderizado.

### `tetris.c` — Tetris
Implementación completa del juego **Tetris** para el emulador. Usa la VRAM para los gráficos y la ACIA para la entrada de teclado. Una de las demostraciones más completas del simulador.

---

## Programas en ensamblador

### `vga.s` — Driver VGA en ensamblador
Driver de bajo nivel para la GPU escrito directamente en ensamblador 65c02. Muestra cómo acceder directamente a la VRAM y controlar los tiempos de sincronización.

---

## Target especial: `eater`

```bash
./compile-bin.sh eater
```

Compila el ROM histórico de **WozMon + Microsoft BASIC** tal como lo usa Ben Eater en su proyecto 65c02. El binario resultante (`output/rom/eater.bin`) arranca directamente con el intérprete BASIC interactivo.

---

## Librerías de soporte (`Binaries/Libs/`)

| Archivo | Descripción |
|---------|-------------|
| `BIOS.h` | Declaraciones de las rutinas del BIOS (CHROUT, CHRIN, etc.) |
| `GPU.h` | Acceso a VRAM con un solo búfer |
| `GPUDoubleBuffer.h` | Acceso a VRAM con doble búfer (sin tearing) |
| `LCD.h` | Funciones para escribir en la pantalla LCD |
| `SID.h` | Control del chip de sonido SID |
| `VIA.h` | Acceso a los registros del VIA 6522 |

---

## Crear un nuevo programa

### Programa en C mínimo

```c
// Binaries/hello.c
#include "Libs/BIOS.h"

int main() {
    // CHROUT escribe un carácter en la consola serie
    CHROUT('H'); CHROUT('e'); CHROUT('l');
    CHROUT('l'); CHROUT('o'); CHROUT('!');
    CHROUT('\n');
    return 0;
}
```

```bash
./compile-bin.sh hello
# Salida: output/rom/hello.bin
```

### Programa con GPU

```c
// Binaries/pixels.c
#include "Libs/GPU.h"

int main() {
    int x, y;
    for (y = 0; y < 75; y++)
        for (x = 0; x < 100; x++)
            vram[y][x] = (x + y) & 0xFF;  // degradado diagonal
    return 0;
}
```

```bash
./compile-bin.sh pixels
```
