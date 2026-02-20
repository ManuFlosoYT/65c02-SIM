# VIA y ACIA — Chips de entrada/salida

## VIA — MOS 6522 Versatile Interface Adapter

**Archivo:** `Hardware/VIA.h` / `Hardware/VIA.cpp`  
**Namespace:** `Hardware::VIA`  
**Dirección base:** `0x6000`

### Descripción general

El **VIA 6522** es el chip de interfaz de E/S principal del sistema. Proporciona puertos de E/S paralelos bidireccionales, dos timers de 16 bits y un registro de desplazamiento. En el esquema de Ben Eater se usa principalmente para controlar la **pantalla LCD**.

### Registros

| Dirección | Nombre | Descripción |
|-----------|--------|-------------|
| `0x6000` | `PORTB` | Puerto de datos B |
| `0x6001` | `PORTA` | Puerto de datos A |
| `0x6002` | `DDRB` | Data Direction Register B (1=salida, 0=entrada) |
| `0x6003` | `DDRA` | Data Direction Register A |
| `0x6004` | `T1C_L` | Timer 1 Counter — byte bajo |
| `0x6005` | `T1C_H` | Timer 1 Counter — byte alto |
| `0x6006` | `T1L_L` | Timer 1 Latch — byte bajo |
| `0x6007` | `T1L_H` | Timer 1 Latch — byte alto |
| `0x6008` | `T2C_L` | Timer 2 Counter — byte bajo |
| `0x6009` | `T2C_H` | Timer 2 Counter — byte alto |
| `0x600A` | `SR` | Shift Register |
| `0x600B` | `ACR` | Auxiliary Control Register |
| `0x600C` | `PCR` | Peripheral Control Register |
| `0x600D` | `IFR` | Interrupt Flag Register |
| `0x600E` | `IER` | Interrupt Enable Register |
| `0x600F` | `ORA_NH` | Output Register A (sin handshake) |

### Timers

**Timer 1 (T1)** — 16 bits, dos modos:
- **One-shot:** genera una IRQ cuando expira y sigue contando.
- **Continuo (ACR bit 6=1):** se recarga desde el latch y genera IRQs periódicas.
- Opcionalmente controla el estado de **PB7** (ACR bit 7).

**Timer 2 (T2)** — 16 bits, dos modos:
- **One-shot:** genera una IRQ al expirar y se detiene.
- **Conteo de pulsos (ACR bit 5=1):** cuenta flancos en la entrada PB6.

### Registro de desplazamiento (SR)

El SR permite transferir 8 bits en serie. El modo se selecciona con los bits `ACR[4:2]`:

| Valor | Modo |
|-------|------|
| `000` | Deshabilitado |
| `010` | Entrada por PHI2 |
| `011` | Entrada por CB1 externo |
| `100` | Salida libre (T2) |
| `101` | Salida por T2 |
| `110` | Salida por PHI2 |
| `111` | Salida por CB1 externo |

### Interrupciones

El VIA genera IRQs controladas por `IER` e `IFR`. Cuando una condición de interrupción ocurre, se activa el bit correspondiente en `IFR`. Si el bit está habilitado en `IER`, se señala la línea IRQ de la CPU.

### Clock

En cada ciclo del emulador se llama a `via.Clock()`, que decrementa los timers y actualiza el registro de desplazamiento según el modo activo.

### Callback de Puerto B

El VIA notifica cambios en el Puerto B mediante un callback:

```cpp
via.SetPortBCallback([](Byte val) {
    lcd.Update(val);    // Actualizar el LCD con el nuevo valor
});
```

---

## ACIA — MOS 6551 Asynchronous Communications Interface

**Archivo:** `Hardware/ACIA.h` / `Hardware/ACIA.cpp`  
**Namespace:** `Hardware::ACIA`  
**Dirección base:** `0x5000`

### Descripción general

La **ACIA 6551** proporciona una interfaz de comunicaciones serie asíncrona (UART). En el emulador se usa para la **consola de texto**: la CPU escribe caracteres en el registro de datos y el emulador los envía a la consola de la GUI.

### Registros

| Dirección | Nombre | Descripción |
|-----------|--------|-------------|
| `0x5000` | `ACIA_DATA` | Dato — escritura: enviar carácter; lectura: recibir carácter |
| `0x5001` | `ACIA_STATUS` | Estado — bit 7: dato recibido disponible |
| `0x5002` | `ACIA_CMD` | Comando — configuración de la interfaz |
| `0x5003` | `ACIA_CTRL` | Control — baud rate, bits de datos, bits de parada |

### Funcionamiento

**Envío de caracteres (CPU → consola):**
1. La CPU escribe un byte en `0x5000`.
2. El hook de escritura invoca el `outputCallback`.
3. La GUI muestra el carácter en la **Console Window**.

**Recepción de caracteres (teclado → CPU):**
1. La GUI llama a `emulator.InjectKey(c)`.
2. El byte se añade al `inputBuffer` del emulador.
3. El emulador pone el byte en `0x5000` y activa el bit 7 de `0x5001`.
4. Si la CPU estaba en modo `WAI`, despierta y lee el dato.

### Callback de salida

```cpp
acia.SetOutputCallback([](char c) {
    console.Append(c);    // Añadir carácter a la consola
});
```

### Retardo de baud rate

El emulador implementa un `baudDelay` proporcional a la velocidad de transmisión configurada en `ACIA_CTRL`, para simular fielmente el tiempo de transmisión de cada byte.
