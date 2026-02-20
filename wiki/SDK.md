# SDK — Herramientas de desarrollo

El SDK incluye tres scripts de automatización para compilar programas y convertir activos para el emulador.

> **Windows:** Todos los scripts son Bash. Los usuarios de Windows deben usar **WSL2** (ver sección de instalación en el [README](https://github.com/ManuFlosoYT/65c02-SIM/blob/master/README.md)).

---

## `compile-bin.sh` — Compilar programas

Compila programas en **C** o **ensamblador 65c02** y genera un binario `.bin` listo para cargar en el simulador.

### Uso

```bash
./compile-bin.sh <nombre_programa>
./compile-bin.sh all          # Compila todos los programas
./compile-bin.sh eater        # Compila el ROM de WOZMON + Microsoft BASIC
```

La salida se guarda en `output/rom/<nombre>.bin`.

### Requisitos

- **cc65** (`ca65`, `ld65`, `cl65`) — Toolchain de ensamblado/compilación para 6502
- **Python 3** (para los scripts de conversión)

### Flujo de compilación

#### Programas en ensamblador (`.s`)

```
Binaries/<nombre>.s
       ↓ ca65 --cpu 65C02
Binaries/build/<nombre>.o
       ↓ ld65 -C Linker/raw.cfg
output/rom/<nombre>.bin
```

#### Programas en C (`.c`)

```
Binaries/<nombre>.c
       ↓ cl65 -O --cpu 65C02 -S     (compilar a ensamblador)
Binaries/build/<nombre>.s
       ↓ cl65 --cpu 65C02 -C <cfg>  (ensamblar + enlazar + BIOS)
       │  Incluye: Linker/bios.s + Linker/C-Runtime.s
output/rom/<nombre>.bin
```

#### Detección automática del linker

El script detecta qué configuración de linker usar según las cabeceras incluidas en el programa:

| Cabecera detectada | Configuración usada |
|--------------------|--------------------|
| `#include "Libs/GPUDoubleBuffer.h"` | `C-Runtime-GPUDoubleBuffer.cfg` |
| `#include "Libs/GPU.h"` | `C-Runtime-GPU.cfg` |
| (ninguna) | `C-Runtime.cfg` |

#### Target especial `eater`

Compila el ROM de **WozMon + Microsoft BASIC** usando el Makefile de `Linker/msbasic/`.

---

## `image-to-bin.sh` — Convertir imágenes a VRAM

Convierte una imagen (PNG, JPG o BMP) al formato binario de la VRAM del emulador (100×75 píxeles, 1 byte por píxel en escala de grises).

### Uso

```bash
./image-to-bin.sh <nombre_imagen>
# Ejemplo: ./image-to-bin.sh GPU/bocchi.png
```

La salida se guarda en `output/vram/<nombre_imagen>.bin`.

### Requisitos

- **Python 3** con **Pillow** (`pip install Pillow`)

### Proceso de conversión

1. Carga la imagen con Pillow
2. Redimensiona a 100×75 píxeles
3. Convierte a escala de grises (8 bits por píxel)
4. Guarda el array de bytes como fichero binario

El binario resultante puede cargarse en la VRAM del emulador a través de la interfaz gráfica.

---

## `midi-to-bin.sh` — Convertir MIDI a código SID

Convierte un archivo MIDI en código ensamblador 65c02 para el chip SID del emulador. El resultado puede compilarse y ejecutarse directamente.

### Uso

```bash
./midi-to-bin.sh <archivo_midi>
# Ejemplo: ./midi-to-bin.sh SID/overworld.mid
```

La salida se guarda en `output/midi/<cancion>.bin`.

### Requisitos

- **Python 3** con **mido** (`pip install mido`)
- **cc65** (`ca65`, `ld65`) para compilar el ensamblador generado

### Modos de optimización

El script tiene **8 modos de compresión** (granularidades de tiempo desde 1 ms hasta 100+ ms). Prueba automáticamente desde el modo más preciso hasta el más comprimido y selecciona el primero que cabe en la ROM de 32 KB:

| Modo | Granularidad | Precisión |
|------|-------------|-----------|
| 1 | ~1 ms | Máxima fidelidad |
| 2 | ~2 ms | Alta fidelidad |
| 3 | ~5 ms | Buena fidelidad |
| 4 | ~10 ms | Fidelidad media |
| 5 | ~20 ms | Compresión media |
| 6 | ~40 ms | Alta compresión |
| 7 | ~80 ms | Muy comprimido |
| 8 | ~100+ ms | Compresión máxima |

Si ningún modo cabe en 32 KB, el script informa del error.

### Proceso de conversión

1. Parsea el archivo MIDI con `mido`
2. Genera código ensamblador 65c02 que escribe en los registros del SID para reproducir la canción
3. Compila con `ca65` + `ld65` usando `Linker/raw.cfg`
4. El binario final se carga en ROM y la CPU ejecuta las instrucciones para reproducir el sonido

---

## Librerías para programas en C

En `Binaries/Libs/` hay cabeceras C que facilitan el uso del hardware desde código C:

| Archivo | Descripción |
|---------|-------------|
| `GPU.h` | Funciones para escribir en VRAM (1 búfer) |
| `GPUDoubleBuffer.h` | Funciones para double-buffering en VRAM |
| `SID.h` | Funciones para controlar el chip SID |
| `VIA.h` | Funciones para acceder al VIA 6522 |

Ejemplo de uso:

```c
#include "Libs/GPU.h"

int main() {
    // Escribir un píxel en (x=10, y=20) con valor 128
    vram[20][10] = 128;
    return 0;
}
```
