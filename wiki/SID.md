# SID — Sound Interface Device & Synthesis Subsystem

**Source Files:**
- Core SID: [`src/Hardware/Audio/SID.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/SID.h) / [`src/Hardware/Audio/SID.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/SID.cpp)
- Oscillator: [`src/Hardware/Audio/Oscillator.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/Oscillator.h) / [`src/Hardware/Audio/Oscillator.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/Oscillator.cpp)
- Envelope: [`src/Hardware/Audio/ADSREnvelope.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/ADSREnvelope.h) / [`src/Hardware/Audio/ADSREnvelope.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/ADSREnvelope.cpp)
- ADSR Bug Monitor: [`src/Hardware/Audio/ADSRBugMonitor.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/ADSRBugMonitor.h)
- Resonant Filter: [`src/Hardware/Audio/SIDFilter.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/SIDFilter.h)
- Band-Limited Resampler: [`src/Hardware/Audio/SIDResampler.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/SIDResampler.h)
- Audio Output Sink: [`src/Hardware/Audio/SIDAudioSink.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/SIDAudioSink.h)
- Waveform Look-Up Tables: [`src/Hardware/Audio/SIDWaveTables.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/SIDWaveTables.h)
- NSF Runner: [`src/Hardware/Audio/NSFRunner.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/NSFRunner.h) / [`src/Hardware/Audio/NSFRunner.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/NSFRunner.cpp)

---

## 1. Overview

The 65c02-SIM platform integrates an authentic hardware emulation of the **MOS Technology 6581 and 8580 Sound Interface Devices (SID)** originally popularized by the Commodore 64. The chip provides **three independent, fully programmable audio voices**, an analog state-variable resonant filter, customizable ADSR envelope generators, oscillator synchronization, ring modulation, and cycle-accurate 16-bit band-limited polyphase resampling.

The SID subsystem runs internally at a native **1.0 MHz clock rate** (`SID_NATIVE_RATE = 1000000.0`), decoupled from the host sample rate (48 kHz) through an anti-aliasing polyphase FIR filter.

```
       Voice 1: Osc 1 + ADSR 1 ──┐
                                 │
       Voice 2: Osc 2 + ADSR 2 ──┼──> [ Matrix Routing ] ──> Multi-Mode Filter ──> Master Mixer ──> Resampler ──> SDL3 Sink
                                 │            │                      │               (16-bit)      (1 MHz->48k)
       Voice 3: Osc 3 + ADSR 3 ──┘            └──────────────────────┘
            ^              │                     (Direct Path)
            └──────────────┘ (Ring Mod & Hard Sync Loop: V1 <- V3, V2 <- V1, V3 <- V2)
```

---

## 2. Chip Architecture & Models: MOS 6581 vs. MOS 8580

The emulator supports both major hardware revisions (`SIDModel::MOS6581` and `SIDModel::MOS8580`), accurately capturing their distinct analog behaviors and non-linearities:

| Characteristic | MOS 6581 | MOS 8580 |
|:---|:---|:---|
| **Filter Cutoff Response** | Non-linear cubic curve: $f_c = 30 + 12000 \times \left(\frac{fc}{2047}\right)^3\text{ Hz}$ | Linear response: $f_c = 30 + 12000 \times \left(\frac{fc}{2047}\right)\text{ Hz}$ |
| **Filter Saturation** | Analog saturation modeled with `FastTanh()` soft clipping | Clean linear response without clipping distortion |
| **Combined Waveforms** | Non-linear analog resistor ladder interactions using real hardware sampling tables (`SIDWaveTables.h`) | Digital bitwise-AND between active waveform bits |
| **Analog DC Offset / Popping** | Significant DC offset bias on volume writes ($(\text{vol} - 0.5) \times 0.45$) used for digitized sample playback (4-bit PCM) | High-pass DC filter removes volume pop; PCM requires virtual bias trick |
| **Signal Leakage** | 5% unfiltered audio signal bleeds directly into the filter stage | Clean voice isolation |
| **ADSR Delay Bug** | Full hardware bug modeled via `ADSRBugMonitor` | Bug present in hardware, monitored in emulator |

---

## 3. Register Reference ($00 – $18)

In the standard system configuration, the SID is mapped at base address **`0x4800` – `0x481F`** (mirrored across 32 bytes). In microDOS and specialized cartridge layouts, the SID is mapped at the top of peripheral high memory at **`0x7FE0` – `0x7FFF`** (offsets `$00`–`$1F` relative to base).

### 3.1 Register Memory Map

| Address | Name | Description |
|:---:|:---|:---|
| **`$00`** | `FREQ_LO_1` | Voice 1 Frequency Register: Low Byte (bits 0–7) |
| **`$01`** | `FREQ_HI_1` | Voice 1 Frequency Register: High Byte (bits 8–15) |
| **`$02`** | `PW_LO_1` | Voice 1 Pulse Width: Low Byte (bits 0–7) |
| **`$03`** | `PW_HI_1` | Voice 1 Pulse Width: High 4 bits (bits 8–11) |
| **`$04`** | `CTRL_1` | Voice 1 Control: Noise · Pulse · Saw · Tri · Test · Ring · Sync · Gate |
| **`$05`** | `AD_1` | Voice 1 Attack Rate (bits 4–7) / Decay Rate (bits 0–3) |
| **`$06`** | `SR_1` | Voice 1 Sustain Level (bits 4–7) / Release Rate (bits 0–3) |
| **`$07`–`$0D`** | *Voice 2* | Identical layout for Voice 2 |
| **`$0E`–`$14`** | *Voice 3* | Identical layout for Voice 3 |
| **`$15`** | `FC_LO` | Filter Cutoff Frequency: Low 3 bits (bits 0–2) |
| **`$16`** | `FC_HI` | Filter Cutoff Frequency: High 8 bits (bits 3–10) |
| **`$17`** | `RES_FILT` | Filter Resonance (bits 4–7) / Voice Routing (bits 0–3) |
| **`$18`** | `MODE_VOL` | Filter Mode (bits 4–7) / Master Volume (bits 0–3) |
| **`$19`** | `POTX` | Game Paddle X (Read-only, returns 0) |
| **`$1A`** | `POTY` | Game Paddle Y (Read-only, returns 0) |
| **`$1B`** | `OSC3` | Voice 3 Oscillator Output (Read-only, upper 8 bits) |
| **`$1C`** | `ENV3` | Voice 3 Envelope Output (Read-only, 8-bit level) |

---

## 4. Voice Synthesis & Waveform Generation

### 4.1 Frequency Calculation & 24-bit Accumulator

Each voice maintains a 24-bit phase accumulator clocked once every 1 MHz cycle:

$$\text{accumulator} = (\text{accumulator} + \text{frequency}) \ \& \ \text{0xFFFFFF}$$

The resulting output audio frequency in Hertz is calculated as:

$$f_{\text{out}} = \frac{f_{\text{clk}} \times \text{FREQ}}{2^{24}} = \frac{1\,000\,000 \times \text{FREQ}}{16\,777\,216} \approx \text{FREQ} \times 0.0596046\text{ Hz}$$

$$\text{FREQ} = \text{round}\left(f_{\text{out}} \times \frac{16\,777\,216}{1\,000\,000}\right) = \text{round}(f_{\text{out}} \times 16.777216)$$

### 4.2 Control Register Bitfield (`CTRL`)

```
Bit:    7       6       5       4       3       2      1      0
      +-------+-------+-------+-------+-------+------+------+------+
      | NOISE | PULSE |  SAW  |  TRI  | TEST  | RING | SYNC | GATE |
      +-------+-------+-------+-------+-------+------+------+------+
```

| Bit | Name | Function / Description |
|:---:|:---:|:---|
| **0** | `GATE` | `1` = Trigger Attack/Decay/Sustain cycle; `0` = Enter Release stage |
| **1** | `SYNC` | Hard Sync: resets this voice's accumulator when the predecessor accumulator wraps |
| **2** | `RING` | Ring Mod: replaces the MSB of this voice with an XOR of the predecessor voice's MSB |
| **3** | `TEST` | Test bit: resets phase accumulator to 0 and re-seeds noise LFSR shift register |
| **4** | `TRI` | Selects Triangle waveform output |
| **5** | `SAW` | Selects Sawtooth waveform output |
| **6** | `PULSE`| Selects Variable Pulse / Square wave output |
| **7** | `NOISE`| Selects 23-bit Pseudo-Random White Noise generator |

### 4.3 Waveform Details

1. **Triangle:** Generated by taking the upper 12 bits of the accumulator and reflecting them using the MSB (bit 23):
   $$\text{Output} = (\text{accumulator} \oplus (\text{MSB} \ ?\ \text{0xFFFFFF} : 0)) \gg 11$$
2. **Sawtooth:** High 12 bits of the phase accumulator:
   $$\text{Output} = \text{accumulator} \gg 12$$
3. **Pulse:** The high 12 bits are compared against the 12-bit `pulseWidth` register:
   $$\text{Output} = ((\text{accumulator} \gg 12) \ge \text{pulseWidth}) \ ?\ \text{0xFFF} : 0$$
4. **Noise:** Driven by a 23-bit Galois Linear Feedback Shift Register (LFSR) seeded with `0x7FFFF8`. The register advances when accumulator bit 19 undergoes a $0 \to 1$ transition:
   $$\text{bit} = \left((\text{noiseShift} \gg 22) \oplus (\text{noiseShift} \gg 17)\right) \ \& \ 1$$
   $$\text{noiseShift} = ((\text{noiseShift} \ll 1) \ \& \ \text{0x7FFFFF}) \mid \text{bit}$$
5. **Combined Waveforms:** Combining Triangle, Sawtooth, and Pulse produces complex timbres. Under MOS 6581, the emulator samples dedicated hardware look-up tables (`wave6581__ST`, `wave6581_P_T`, `wave6581_PS_`, `wave6581_PST`), while MOS 8580 performs bitwise-AND operations.

---

## 5. ADSR Envelope Generator & 6581 Bug Emulation

### 5.1 Envelope State Machine

Each voice features an independent 8-bit digital envelope generator ($0 \dots 255$ level):

```
       Level
        255 +               /\
            |              /  \
            |  ATTACK     /    \  DECAY
            |            /      \______ SUSTAIN
            |           /              \
            |          /                \  RELEASE
          0 +---------+------------------+--------> Time
                    GATE=1             GATE=0
```

1. **ATTACK:** Starts upon `GATE = 1`. Rate counter decrements based on `RATE_COUNTER_PERIODS[attackRate]`. Level increments linearly until reaching 255.
2. **DECAY:** Level decreases towards `sustainLevel` ($0 \dots 15 \times 17$). Decay uses an exponential step divider (stepping every 1, 2, 4, 8, 16, or 30 rate periods).
3. **SUSTAIN:** Constant output level while `GATE = 1`.
4. **RELEASE:** Initiated when `GATE = 0`. Follows the exponential rate specified by `releaseRate`.

### 5.2 Rate Counter Period Table

The rate counter periods (in 1 MHz clock cycles) exactly match MOS specifications:

```cpp
const std::array<uint16_t, 16> RATE_COUNTER_PERIODS = {
      9,     32,     63,     95,    149,    220,    267,    313,
    392,    977,   1954,   3126,   3907,  11720,  19532,  31251
};
```

### 5.3 Hardware ADSR Delay Bug (`ADSRBugMonitor`)

In the physical MOS 6581, when an envelope transitions to Release and is subsequently re-gated (`GATE = 1`) before the rate counter finishes its full countdown, or when the Attack rate is modified on the fly, the internal rate comparator can fail to match. The rate counter then wraps through its entire 15-bit range (`32,768` cycles), causing a noticeable attack delay or stuck note.

The emulator reproduces this behavior faithfully via [`ADSRBugMonitor.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/ADSRBugMonitor.h), ensuring accurate reproduction of demoscene soundtracks that exploit this glitch for percussive timing.

---

## 6. Multi-Mode Analog Filter

The SID includes a second-order state-variable multi-mode analog filter with independent Low-Pass, Band-Pass, and High-Pass outputs:

### 6.1 Cutoff & Resonance Configuration

- **Cutoff (`FC_LO`, `FC_HI`):** 11-bit unsigned integer ($0 \dots 2047$):
  $$\text{Cutoff} = (\text{registers}[0x15] \ \& \ 7) \mid (\text{registers}[0x16] \ll 3)$$
- **Resonance (`RES_FILT` bits [7:4]):** 4-bit unsigned integer ($0 \dots 15$). Controls filter $Q$:
  - MOS 6581: $Q = 1.5 - \text{effectiveRes}$
  - MOS 8580: $Q = 2.0 - 1.8 \times \left(\frac{\text{res}}{15}\right)$

### 6.2 Filter Routing & Mode Selection

**Register `$17` (Routing):**
- Bit 0: Filter Voice 1
- Bit 1: Filter Voice 2
- Bit 2: Filter Voice 3
- Bit 3: Filter External Audio In
- Bits [7:4]: Resonance ($0 \dots 15$)

**Register `$18` (Mode & Volume):**
- Bit 4: **Low-Pass (LP)** Enable (12 dB/octave attenuation above cutoff)
- Bit 5: **Band-Pass (BP)** Enable (6 dB/octave attenuation on both sides)
- Bit 6: **High-Pass (HP)** Enable (12 dB/octave attenuation below cutoff)
- Bit 7: **Voice 3 Off** (Disconnects Voice 3 from direct audio path; Voice 3 still sounds if routed through filter)
- Bits [3:0]: **Master Volume** ($0 \dots 15$)

Combinations enable Notch (LP + HP) and other hybrid responses!

---

## 7. Band-Limited Polyphase Resampler (`SIDResampler`)

Directly downsampling the 1.0 MHz SID output to 48 kHz produces severe aliasing harmonics that destroy high-frequency synthesis. 

65c02-SIM employs a specialized **256-phase Kaiser-windowed polyphase FIR filter** (`SIDResampler`):
- **Native Input Rate:** $1\,000\,000.0\text{ Hz}$
- **Host Output Rate:** $48\,000.0\text{ Hz}$
- **Filter Length:** 32 taps (16 half-taps) with Kaiser window parameter $\beta = 8.0$.
- **Real-Time Performance:** Pre-calculated polyphase coefficient lookup tables ensure pristine anti-aliased output with near-zero host CPU overhead.

### 7.1 Dormant State Optimization

When all 3 voices have zero envelope output, zero frequency, and the filter is quiet, `SID::UpdateDormantState()` switches the chip into dormant mode (`isDormant = true`). In dormant mode, clock cycles are counted without computing synthesis arithmetic, restoring envelope counters transparently when a register is written.

---

## 8. NSF Runner Subsystem (`NSFRunner`)

[`NSFRunner`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Audio/NSFRunner.h) provides automated playback, tracing, and analysis of NES Sound Format (NSF) chiptune music files:

- **Frame Tracing:** Records per-frame volume, period, duty cycle, and retrigger events for 4 NES channels (Pulse 1, Pulse 2, Triangle, Noise).
- **JSON Export:** `NSFRunner::WriteTraceJson()` exports execution traces to disk for analysis in external visualizers and audio benchmark tools.

---

## 9. Programming Examples

### 9.1 65c02 Assembly: Playing a Musical Note

```assembly
; =====================================================================
; PlaySIDNote: Plays an 'A4' (440 Hz) Sawtooth note on Voice 1
; Base address = $4800
; =====================================================================
SID_BASE    = $4800

FREQ_LO_1   = SID_BASE + $00
FREQ_HI_1   = SID_BASE + $01
PW_LO_1     = SID_BASE + $02
PW_HI_1     = SID_BASE + $03
CTRL_1      = SID_BASE + $04
AD_1        = SID_BASE + $05
SR_1        = SID_BASE + $06
MODE_VOL    = SID_BASE + $18

PlayNote:
    ; 1. Set Master Volume to Maximum (15) without filters
    LDA #$0F
    STA MODE_VOL

    ; 2. Configure ADSR Envelope
    ; Attack = 2 (fast), Decay = 4
    LDA #$24
    STA AD_1
    ; Sustain = 12 (high), Release = 8 (medium)
    LDA #$C8
    STA SR_1

    ; 3. Program Frequency for A4 (440 Hz)
    ; FREQ = 440 * 16.777216 = 7382 = $1CD6
    LDA #$D6
    STA FREQ_LO_1
    LDA #$1C
    STA FREQ_HI_1

    ; 4. Start Note: Sawtooth ($20) + Gate ($01) = $21
    LDA #$21
    STA CTRL_1
    RTS

StopNote:
    ; Clear Gate bit to initiate Release stage
    LDA #$20            ; Sawtooth, Gate = 0
    STA CTRL_1
    RTS
```

### 9.2 C Language: BIOS Macros & Filter Configuration

```c
#include <stdint.h>
#include "Libs/app-bios.h"

void play_filtered_chord(void) {
    // 1. Reset chip
    sid_reset();

    // 2. Configure Voice 1: Fast attack, high sustain
    sid_set_envelope(1, 0x01, 0x04, 0x0C, 0x04);

    // 3. Set Low-Pass Filter with medium resonance on Voice 1
    // Cutoff frequency = 1200 (out of 2047)
    sid_set_filter_cutoff(1200);
    // Resonance: 8, Route Voice 1 (bit 0 = 1), Mode: Low-Pass (0x10), Vol: 15
    sid_set_filter_config(8, 0x01, 0x10, 15);

    // 4. Trigger Note C-4 (Freq 0x1124) with Sawtooth + Gate
    sid_trigger_note(1, NOTE_C4, WAVE_SAW | WAVE_GATE);
}
```