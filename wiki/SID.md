# SID — Sound Synthesis

**File:** `Hardware/SID.h` / `Hardware/SID.cpp`  
**Namespace:** `Hardware::SID`

## Overview

The emulator includes a sound synthesis chip emulation inspired by the **SID** chip (like the MOS 6581 used in the Commodore 64). It has **3 independent oscillators**, each with its own **waveform generator** and **ADSR envelope**. Audio is played back in real time via SDL3 AudioStream.

## Internal architecture

```
SID
├── voices[0]  →  Oscillator + ADSREnvelope
├── voices[1]  →  Oscillator + ADSREnvelope
└── voices[2]  →  Oscillator + ADSREnvelope
         ↓
    GenerateAudio()
         ↓
    SDL3 AudioStream  →  Speaker
```

## `Oscillator` struct

Each voice has an oscillator with the following fields:

| Field | Type | Description |
|-------|------|-------------|
| `accumulator` | `uint32_t` | Phase accumulator |
| `frequency` | `uint32_t` | Programmed frequency |
| `pulseWidth` | `uint16_t` | Pulse width (for square wave) |
| `control` | `uint8_t` | Control register: Gate · Sync · Ring · Test · Tri · Saw · Pulse · Noise |
| `noiseShift` | `uint32_t` | LFSR shift register for noise |

### Waveforms (`control` register bits)

| Bit | Waveform | Description |
|-----|----------|-------------|
| 0 | Gate | Enables/disables the envelope |
| 1 | Sync | Synchronises with the previous oscillator |
| 2 | Ring | Ring modulation with the previous oscillator |
| 3 | Test | Test mode (halts the oscillator) |
| 4 | Triangle | Triangle wave |
| 5 | Sawtooth | Sawtooth wave |
| 6 | Pulse | Square/pulse wave |
| 7 | Noise | White noise (LFSR) |

## `ADSREnvelope` struct

The ADSR envelope controls the volume of each note:

| State | Description |
|-------|-------------|
| `IDLE` | No activity |
| `ATTACK` | Attack: level rises from 0 |
| `DECAY` | Decay: level falls toward the sustain level |
| `SUSTAIN` | Sustain: constant level while Gate=1 |
| `RELEASE` | Release: level falls to 0 when Gate=0 |

Envelope parameters:

| Field | Description |
|-------|-------------|
| `attackRate` | Attack speed |
| `decayRate` | Decay speed |
| `sustainLevel` | Sustain level (0.0–1.0) |
| `releaseRate` | Release speed |

## Memory-mapped registers

SID registers are accessible from 65c02 code through Write/Read functions (integrated into the emulator via hooks):

| Offset | Name | Description |
|--------|------|-------------|
| `0x00`–`0x06` | Voice 1 | Frequency, pulse width, control, ADSR |
| `0x07`–`0x0D` | Voice 2 | Same as Voice 1 |
| `0x0E`–`0x14` | Voice 3 | Same as Voice 1 |
| `0x18` | Volume/Filter | Master volume byte and filter |

## Real-time audio

The SID uses **SDL3**'s audio system (`SDL_AudioStream`) at 44100 Hz, 16-bit samples, mono. Audio generation happens in an asynchronous callback:

```cpp
SID::AudioCallback(void* userdata, SDL_AudioStream* stream,
                   int additional_amount, int total_amount);
```

### Enable control

```cpp
sid.EnableSound(true);            // Enable audio
sid.SetEmulationPaused(true);     // Pause generation (mute)
bool enabled = sid.IsSoundEnabled();
```

## Frontend visualisation

The **SID Viewer** window in the GUI shows in real time:
- The waveform and state of each oscillator
- The envelope state (ATTACK/DECAY/SUSTAIN/RELEASE)
- Frequency and pulse-width values

## MIDI-to-SID tool

The `SID/generator/` directory includes a Python script that converts MIDI files to 65c02 assembly code playable on the emulator.

```bash
./midi-to-bin.sh <midi_file>
# Output: output/midi/<song>.bin
```

The script has 8 compression optimisation modes (granularities from 1 ms to 100+ ms) and automatically selects the best one.

### Included sample songs

Several demonstration MIDI files are provided in `SID/`