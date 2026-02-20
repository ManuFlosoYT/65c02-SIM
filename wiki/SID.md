# SID — Síntesis de sonido

**Archivo:** `Hardware/SID.h` / `Hardware/SID.cpp`  
**Namespace:** `Hardware::SID`

## Descripción general

El emulador incluye una emulación del chip de síntesis de sonido estilo **SID** (como el MOS 6581 de la Commodore 64). Cuenta con **3 osciladores independientes**, cada uno con su propio **generador de forma de onda** y **envolvente ADSR**. El audio se reproduce en tiempo real a través de SDL3 AudioStream.

## Arquitectura interna

```
SID
├── voices[0]  →  Oscillator + ADSREnvelope
├── voices[1]  →  Oscillator + ADSREnvelope
└── voices[2]  →  Oscillator + ADSREnvelope
         ↓
    GenerateAudio()
         ↓
    SDL3 AudioStream  →  Altavoz
```

## Struct `Oscillator`

Cada voz tiene un oscilador con los siguientes campos:

| Campo | Tipo | Descripción |
|-------|------|-------------|
| `accumulator` | `uint32_t` | Acumulador de fase |
| `frequency` | `uint32_t` | Frecuencia programada |
| `pulseWidth` | `uint16_t` | Ancho de pulso (para onda cuadrada) |
| `control` | `uint8_t` | Registro de control: Gate · Sync · Ring · Test · Tri · Saw · Pulse · Noise |
| `noiseShift` | `uint32_t` | Registro de desplazamiento LFSR para ruido |

### Formas de onda (bits del registro `control`)

| Bit | Forma | Descripción |
|-----|-------|-------------|
| 0 | Gate | Activa/desactiva la envolvente |
| 1 | Sync | Sincronización con el oscilador anterior |
| 2 | Ring | Modulación en anillo con el oscilador anterior |
| 3 | Test | Modo test (detiene el oscilador) |
| 4 | Triangle | Onda triangular |
| 5 | Sawtooth | Onda de sierra |
| 6 | Pulse | Onda cuadrada/pulso |
| 7 | Noise | Ruido blanco (LFSR) |

## Struct `ADSREnvelope`

La envolvente ADSR controla el volumen de cada nota:

| Estado | Descripción |
|--------|-------------|
| `IDLE` | Sin actividad |
| `ATTACK` | Ataque: subida del nivel desde 0 |
| `DECAY` | Decaimiento: bajada hacia el nivel de sustain |
| `SUSTAIN` | Sostenimiento: nivel constante mientras Gate=1 |
| `RELEASE` | Liberación: bajada hasta 0 cuando Gate=0 |

Parámetros de la envolvente:

| Campo | Descripción |
|-------|-------------|
| `attackRate` | Velocidad de ataque |
| `decayRate` | Velocidad de decaimiento |
| `sustainLevel` | Nivel de sustain (0.0–1.0) |
| `releaseRate` | Velocidad de liberación |

## Registros mapeados en memoria

Los registros del SID están accesibles desde el código 65c02 a través de funciones Write/Read (integradas en el emulador vía hooks):

| Offset | Nombre | Descripción |
|--------|--------|-------------|
| `0x00`–`0x06` | Voz 1 | Frecuencia, ancho de pulso, control, ADSR |
| `0x07`–`0x0D` | Voz 2 | Igual que Voz 1 |
| `0x0E`–`0x14` | Voz 3 | Igual que Voz 1 |
| `0x18` | Volumen/Filtro | Byte de volumen maestro y filtro |

## Audio en tiempo real

El SID usa el sistema de audio de **SDL3** (`SDL_AudioStream`) a 44100 Hz, 16 bits por muestra, mono. La generación de audio se realiza en un callback asíncrono:

```cpp
SID::AudioCallback(void* userdata, SDL_AudioStream* stream,
                   int additional_amount, int total_amount);
```

### Control de habilitación

```cpp
sid.EnableSound(true);            // Activa el audio
sid.SetEmulationPaused(true);     // Pausa la generación (silencia)
bool enabled = sid.IsSoundEnabled();
```

## Visualización en el Frontend

La ventana **SID Viewer** de la GUI muestra en tiempo real:
- La forma de onda y estado de cada oscilador
- El estado de la envolvente (ATTACK/DECAY/SUSTAIN/RELEASE)
- Los valores de frecuencia y ancho de pulso

## Herramienta MIDI-to-SID

El directorio `SID/generator/` incluye un script Python que convierte archivos MIDI a código ensamblador 65c02 para reproducir en el emulador.

```bash
./midi-to-bin.sh <archivo_midi>
# Salida: output/midi/<cancion>.bin
```

El script tiene 8 modos de optimización de compresión (de 1ms a 100ms+ de granularidad) y selecciona automáticamente el mejor.

### Canciones de ejemplo incluidas

En `SID/` se encuentran varios archivos MIDI de demostración:
- `fortnite.mid`
- `miku.mid`
- `overworld.mid`
- y otros (6 más)
