# LCD — Pantalla LCD 2×16 caracteres

**Archivo:** `Hardware/LCD.h` / `Hardware/LCD.cpp`  
**Namespace:** `Hardware::LCD`

## Descripción general

El emulador emula una **pantalla LCD de 2 filas × 16 columnas** compatible con el controlador **HD44780**, tal como la usa Ben Eater en su proyecto 65c02. La LCD se controla a través del Puerto B del VIA 6522.

## Distribución de pines en el Puerto B del VIA

El Port B del VIA se conecta a la LCD del siguiente modo (esquema de Ben Eater):

| Bit PB | Señal LCD | Descripción |
|--------|-----------|-------------|
| 0 | DB4 | Bit de dato 4 |
| 1 | DB5 | Bit de dato 5 |
| 2 | DB6 | Bit de dato 6 |
| 3 | DB7 | Bit de dato 7 |
| 4 | RS | Register Select: 0=comando, 1=dato |
| 5 | RW | Read/Write: 0=escribir (siempre 0 en este diseño) |
| 6 | E | Enable (flanco de bajada activa la transferencia) |

## Modo de 4 bits

La LCD opera en **modo nibble de 4 bits**. Cada byte se envía en dos transferencias:
1. **Nibble alto** (bits 7–4)
2. **Nibble bajo** (bits 3–0)

El flanco de bajada de la señal `E` (bit 6 del Puerto B) indica cuándo el dato está disponible.

## Inicialización

La secuencia de inicialización de la LCD se detecta automáticamente. Hasta que no se complete la inicialización, `IsInitialized()` devuelve `false` y no se procesan caracteres.

## Comandos soportados

Cuando `RS=0`, el byte enviado es un **comando**:

| Comando | Descripción |
|---------|-------------|
| `0x01` | Clear display — borra la pantalla y coloca el cursor en (0,0) |
| `0x02` | Return home — mueve el cursor a (0,0) sin borrar |
| `0x28` | Function Set — modo 4 bits, 2 líneas, fuente 5×8 |
| `0x0C` | Display ON, cursor OFF |
| `0x0E` | Display ON, cursor ON |
| `0x06` | Increment cursor, no scroll |
| `0x80` | Set DDRAM address — cursor en la línea 1 |
| `0xC0` | Set DDRAM address — cursor en la línea 2 |

## Escritura de caracteres

Cuando `RS=1`, el byte se interpreta como un **carácter ASCII** y se escribe en la posición actual del cursor. El cursor avanza automáticamente. Al llegar al final de la línea 1, continúa en la línea 2.

## API del emulador

```cpp
// Actualizar LCD con el valor del Puerto B del VIA
lcd.Update(portBVal);

// Leer el estado actual de la pantalla (array 2×16)
const char (&screen)[2][16] = lcd.GetScreen();

// Saber si la LCD ya fue inicializada
bool ready = lcd.IsInitialized();

// Recibir cada carácter impreso (para debug)
lcd.SetOutputCallback([](char c) {
    std::cout << c;
});
```

## Visualización en el Frontend

La ventana **LCD Window** de la GUI muestra el contenido del array `screen[2][16]` en tiempo real, simulando el aspecto de una pantalla LCD física.
