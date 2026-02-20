# GPU — Procesador gráfico y VRAM

**Archivo:** `Hardware/GPU.h` / `Hardware/GPU.cpp`  
**Namespace:** `Hardware::GPU`

## Descripción general

El emulador incluye un **procesador gráfico personalizado** con una VRAM de 7.5 KB. Permite mostrar imágenes en color en la resolución 100×75 píxeles. La interfaz gráfica renderiza el contenido de la VRAM en tiempo real en la ventana **VRAM Viewer**.

## Dimensiones

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| `VRAM_WIDTH` | 100 px | Ancho visible |
| `VRAM_HEIGHT` | 75 px | Alto visible |
| `DISPLAY_WIDTH` | 132 px | Ancho total con blanking |
| `DISPLAY_HEIGHT` | 78 px | Alto total con blanking |
| `VRAM_HEIGHT_DRAWABLE_BY_CPU` | 64 filas | Filas accesibles por la CPU vía bus |

El área de **blanking** (132×78) simula el intervalo de borrado de un monitor analógico. Durante el intervalo de blanking, el bus está libre y la CPU puede escribir en VRAM sin conflictos.

## Distribución de la VRAM en memoria

La VRAM ocupa el rango `0x2000`–`0x3FFF` del espacio de direcciones del sistema (configurado en los archivos de linker GPU).

El direccionamiento de píxeles dentro de la VRAM es:
```
bits [6:0]   → coordenada X (0–99)
bits [13:7]  → coordenada Y (0–74)
```

Por ejemplo, para acceder al píxel (x=5, y=10):
```
addr = (10 << 7) | 5 = 0x0505
dirección efectiva = 0x2000 + 0x0505 = 0x2505
```

## Formato de píxel

Cada byte de la VRAM representa **1 píxel en escala de grises** (0–255).

## Escritura desde la CPU

La CPU puede escribir directamente en VRAM mientras el GPU está en intervalo de blanking. La función `IsInBlankingInterval()` indica cuándo es seguro hacerlo:

```asm
; Esperar intervalo de blanking (ejemplo en ensamblador 65c02)
WAIT_BLANK:
    LDA $2000,Y
    ; El bit de estado de blanking indica si es seguro escribir
    BEQ WAIT_BLANK
    STA $2000,Y     ; Escribir píxel
```

En la práctica, el simulador asume que la CPU siempre puede acceder a las primeras 64 filas sin conflicto.

## Herramienta de conversión de imágenes

El directorio `GPU/generator/` incluye un script Python que convierte imágenes PNG, JPG o BMP al formato binario de la VRAM.

```bash
./image-to-bin.sh <nombre_imagen>
# Salida: output/vram/<nombre_imagen>.bin
```

El binario resultante puede cargarse directamente en VRAM a través de la interfaz gráfica.

## Imágenes de ejemplo

En `GPU/` se incluyen varias imágenes de demostración:
- `bocchi.png`
- `rick.jpeg`
- `xdd.jpg`

## Double Buffering

La configuración de linker `C-Runtime-GPUDoubleBuffer.cfg` activa el modo de **doble búfer**: mientras la GPU lee el primer búfer, la CPU escribe en el segundo, eliminando el tearing.

## Integración con el Frontend

La ventana **VRAM Viewer** de la GUI lee el array `gpu.vram[y][x]` directamente y lo renderiza como textura OpenGL, actualizándose en cada frame.
