# Hardware — Emulador principal y sistema de memoria

## Clase `Emulator`

`Core::Emulator` es el **orquestador central** del simulador. Coordina todos los componentes de hardware, gestiona el hilo de emulación y expone una API limpia para la interfaz gráfica.

**Archivo:** `Hardware/Emulator.h` / `Hardware/Emulator.cpp`

### Instanciación y ciclo de vida

```cpp
Core::Emulator emulator;

// Cargar un binario y arrancar
std::string errorMsg;
if (emulator.Init("program.bin", errorMsg)) {
    emulator.Start();    // Arranca el hilo de emulación
}

emulator.Pause();        // Pausa la ejecución
emulator.Resume();       // Reanuda la ejecución
emulator.Stop();         // Detiene el hilo
```

### Componentes internos

| Campo | Tipo | Descripción |
|-------|------|-------------|
| `mem` | `Hardware::Mem` | Sistema de memoria (RAM + ROM + hooks) |
| `cpu` | `Hardware::CPU` | Núcleo del procesador 65c02 |
| `lcd` | `Hardware::LCD` | Pantalla LCD 2×16 |
| `acia` | `Hardware::ACIA` | Interfaz serie (6551) |
| `gpu` | `Hardware::GPU` | Procesador gráfico + VRAM |
| `sid` | `Hardware::SID` | Chip de sonido |
| `via` | `Hardware::VIA` | Interfaz de E/S versátil (6522) |

### Hilo de emulación

El emulador corre en un hilo dedicado (`std::thread`). La velocidad se controla mediante `SetTargetIPS(int ips)` (instrucciones por segundo). En tiempo real se puede consultar `GetActualIPS()`.

```cpp
emulator.SetTargetIPS(1'000'000);   // 1 MHz
int actual = emulator.GetActualIPS();
```

La sincronización entre el hilo de emulación y la GUI utiliza:
- `std::atomic<bool>` para `running` y `paused`
- `std::condition_variable` (`pauseCV`) para la espera eficiente en pausa
- `std::mutex emulationMutex` para operaciones atómicas de reset/step

### Modo paso a paso

```cpp
int result = emulator.Step();
// 0  → OK
// 1  → STOP/JAM
// -1 → Opcode inválido
```

### Entrada de teclado

Las teclas se inyectan al búfer de entrada, que la CPU lee a través de la ACIA:

```cpp
emulator.InjectKey('A');
```

### Callbacks de salida

```cpp
emulator.SetOutputCallback([](char c) {
    std::cout << c;          // Salida serial → consola
});
emulator.SetLCDOutputCallback([](char c) {
    // Cada carácter escrito en el LCD
});
```

---

## Clase `Mem` — Sistema de memoria

**Archivo:** `Hardware/Mem.h` / `Hardware/Mem.cpp`

### Mapa de memoria

```
0x0000 – 0x7FFF   RAM (32 KB)
0x2000 – 0x3FFF   VRAM (7.5 KB) — solo con GPU habilitada
0x5000 – 0x5003   ACIA (registros de comunicación serie)
0x6000 – 0x600F   VIA (registros de E/S)
0x8000 – 0xFFFF   ROM (32 KB, solo lectura)
0xFFFC – 0xFFFD   Vector RESET
0xFFFE – 0xFFFF   Vector IRQ/BRK
```

> La ROM ocupa la mitad alta del espacio de direcciones. Los intentos de escritura en `addr >= 0x8000` son ignorados silenciosamente.

### Hooks de lectura/escritura

El sistema de memoria soporta *hooks* por dirección, que permiten a los periféricos (ACIA, VIA, GPU) interceptar accesos:

```cpp
mem.SetWriteHook(0x5000, myWriteHook, contextPtr);
mem.SetReadHook(0x5001, myReadHook, contextPtr);
```

Tipos de hook:
```cpp
using WriteHook = void (*)(void*, Word, Byte);
using ReadHook  = Byte (*)(void*, Word);
```

### Registros de periféricos mapeados en memoria

| Dirección | Registro | Periférico |
|-----------|----------|------------|
| `0x5000` | `ACIA_DATA` | ACIA — dato serial |
| `0x5001` | `ACIA_STATUS` | ACIA — estado |
| `0x5002` | `ACIA_CMD` | ACIA — comando |
| `0x5003` | `ACIA_CTRL` | ACIA — control |
| `0x6000` | `PORTB` | VIA — Puerto B |
| `0x6001` | `PORTA` | VIA — Puerto A |
| `0x6002` | `DDRB` | VIA — Dirección B |
| `0x6003` | `DDRA` | VIA — Dirección A |
| `0x6004`–`0x6009` | `T1C/T2C` | VIA — Timers |
| `0x600A`–`0x600F` | `SR/ACR/PCR/IFR/IER` | VIA — Control |

### API de bajo nivel

```cpp
Byte val = mem.Read(0x0200);
mem.Write(0x0200, 0x42);
mem.WriteWord(0x1234, 0x0300);   // Escribe word en little-endian
mem.WriteROM(0x8000, 0xEA);      // Solo para tests unitarios
```
