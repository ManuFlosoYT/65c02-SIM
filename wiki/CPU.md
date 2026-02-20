# CPU — Procesador WDC 65C02

**Archivo:** `Hardware/CPU.h` / `Hardware/CPU.cpp`  
**Namespace:** `Hardware::CPU`

## Descripción general

El núcleo de emulación implementa el procesador **WDC 65C02**, la variante CMOS mejorada del clásico MOS 6502. Añade nuevas instrucciones (bit manipulation, `PHX`, `PHY`, `BRA`, etc.) y corrige algunos comportamientos del 6502 NMOS original.

## Registros

| Registro | Tamaño | Descripción |
|----------|--------|-------------|
| `PC` | 16 bits | Program Counter — dirección de la próxima instrucción |
| `SP` | 16 bits | Stack Pointer — apunta a la página 1 (`0x0100`–`0x01FF`) |
| `A` | 8 bits | Acumulador |
| `X` | 8 bits | Registro de índice X |
| `Y` | 8 bits | Registro de índice Y |

## Registro de estado (flags)

El byte de estado se construye con `GetStatus()` / `SetStatus()`:

| Bit | Flag | Descripción |
|-----|------|-------------|
| 7 | `N` | Negativo (bit 7 del resultado) |
| 6 | `V` | Desbordamiento |
| 5 | — | Siempre 1 (no implementado como campo) |
| 4 | `B` | Break — indica origen de la interrupción en la pila |
| 3 | `D` | Modo BCD (Decimal) |
| 2 | `I` | Deshabilitar interrupciones IRQ |
| 1 | `Z` | Cero (resultado fue 0) |
| 0 | `C` | Carry / acarreo |

## Vectores de interrupción

| Dirección | Vector | Uso |
|-----------|--------|-----|
| `0xFFFA`–`0xFFFB` | NMI | Interrupción no enmascarable |
| `0xFFFC`–`0xFFFD` | RESET | Dirección de inicio tras reset |
| `0xFFFE`–`0xFFFF` | IRQ/BRK | Interrupción enmascarable / instrucción BRK |

Al arrancar, la CPU lee el vector RESET (`0xFFFC`) para obtener la dirección inicial.

## Ciclo de ejecución

```
Reset() → isInit = false
   ↓
Step() → si !isInit: leer vector RESET → PC
   ↓
Dispatch() → leer opcode en PC, ejecutar instrucción
   ↓
Devuelve 0 (OK) / 1 (JAM) / -1 (opcode inválido)
```

### Modo ciclo-exacto (`cycleAccurate`)

Cuando está activo, cada instrucción consume el número correcto de ciclos de reloj:

```cpp
emulator.SetCycleAccurate(true);
```

Internamente, `remainingCycles` decrementa en cada llamada a `Step()` hasta llegar a 0, momento en que se ejecuta la siguiente instrucción. También aplica penalizaciones por cruce de página:

```cpp
cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
```

## Interrupciones

### IRQ (enmascarable)
```
IRQ() → si flag I == 0:
   1. Push PC en pila
   2. Push estado (B=0) en pila
   3. I = 1, D = 0  ← el 65C02 limpia el flag Decimal
   4. PC = mem[0xFFFE]
```

### Modo espera (`WAI`)
La instrucción `WAI` pone la CPU en estado `waiting = true`. Se despierta cuando el bit 7 del registro `ACIA_STATUS` (`0x5001`) cambia a 1.

## Pila

La pila reside en la **página 1** (`0x0100`–`0x01FF`). El puntero de pila (`SP`) apunta al siguiente byte libre y crece hacia abajo. Las operaciones de desbordamiento hacen wrap dentro de la misma página:

```
PushByte: SP < 0x0100 → SP = 0x01FF
PopByte:  SP > 0x01FF → SP = 0x0100
```

## Despacho de instrucciones

El despacho se realiza en `Hardware/CPU/Dispatch.cpp`. Cada opcode tiene su propia clase en `Hardware/CPU/Instructions/`, de forma que el compilador puede optimizar con inline o LTO.

```
Dispatch(cpu, mem)
   └─ FetchByte(PC)  →  opcode
      └─ switch(opcode)  →  Instrucción::Execute(cpu, mem)
```

Consulta la página [CPU Instructions](CPU-Instructions) para ver el listado completo de instrucciones soportadas.
