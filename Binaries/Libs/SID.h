
#ifndef SID_LIB_H
#define SID_LIB_H

#include <stdint.h>

// SID Base Address
#define SID_BASE 0x4800

// Register Offsets
#define FREQ_LO_1 0x00
#define FREQ_HI_1 0x01
#define PW_LO_1 0x02
#define PW_HI_1 0x03
#define CTRL_1 0x04
#define AD_1 0x05
#define SR_1 0x06

#define FREQ_LO_2 0x07
#define FREQ_HI_2 0x08
#define PW_LO_2 0x09
#define PW_HI_2 0x0A
#define CTRL_2 0x0B
#define AD_2 0x0C
#define SR_2 0x0D

#define FREQ_LO_3 0x0E
#define FREQ_HI_3 0x0F
#define PW_LO_3 0x10
#define PW_HI_3 0x11
#define CTRL_3 0x12
#define AD_3 0x13
#define SR_3 0x14

#define FC_LO 0x15
#define FC_HI 0x16
#define RES_FILT 0x17
#define MODE_VOL 0x18

// Waveforms
#define WAVE_GATE 0x01
#define WAVE_SYNC 0x02
#define WAVE_RING 0x04
#define WAVE_TEST 0x08
#define WAVE_TRI 0x10
#define WAVE_SAW 0x20
#define WAVE_PULSE 0x40
#define WAVE_NOISE 0x80

// Note Frequencies (Approx)
#define NOTE_C0 0x0112
#define NOTE_Cs0 0x0122
#define NOTE_D0 0x0133
#define NOTE_Ds0 0x0146
#define NOTE_E0 0x0159
#define NOTE_F0 0x016E
#define NOTE_Fs0 0x0183
#define NOTE_G0 0x019B
#define NOTE_Gs0 0x01B3
#define NOTE_A0 0x01CD
#define NOTE_As0 0x01E8
#define NOTE_B0 0x0205
#define NOTE_C1 0x0224
#define NOTE_Cs1 0x0245
#define NOTE_D1 0x0267
#define NOTE_Ds1 0x028C
#define NOTE_E1 0x02B3
#define NOTE_F1 0x02DC
#define NOTE_Fs1 0x0307
#define NOTE_G1 0x0336
#define NOTE_Gs1 0x0367
#define NOTE_A1 0x039A
#define NOTE_As1 0x03D1
#define NOTE_B1 0x040B
#define NOTE_C2 0x0449
#define NOTE_Cs2 0x048A
#define NOTE_D2 0x04CF
#define NOTE_Ds2 0x0519
#define NOTE_E2 0x0566
#define NOTE_F2 0x05B8
#define NOTE_Fs2 0x060F
#define NOTE_G2 0x066C
#define NOTE_Gs2 0x06CE
#define NOTE_A2 0x0735
#define NOTE_As2 0x07A3
#define NOTE_B2 0x0817
#define NOTE_C3 0x0892
#define NOTE_Cs3 0x0914
#define NOTE_D3 0x099E
#define NOTE_Ds3 0x0A32
#define NOTE_E3 0x0ACC
#define NOTE_F3 0x0B71
#define NOTE_Fs3 0x0C1F
#define NOTE_G3 0x0CD8
#define NOTE_Gs3 0x0D9C
#define NOTE_A3 0x0E6A
#define NOTE_As3 0x0F47
#define NOTE_B3 0x102F
#define NOTE_C4 0x1124
#define NOTE_Cs4 0x1229
#define NOTE_D4 0x133D
#define NOTE_Ds4 0x1465
#define NOTE_E4 0x1599
#define NOTE_F4 0x16E3
#define NOTE_Fs4 0x183E
#define NOTE_G4 0x19B0
#define NOTE_Gs4 0x1B38
#define NOTE_A4 0x1CD5
#define NOTE_As4 0x1E8E
#define NOTE_B4 0x205E
#define NOTE_C5 0x2249
#define NOTE_Cs5 0x2452
#define NOTE_D5 0x267B
#define NOTE_Ds5 0x28CA
#define NOTE_E5 0x2B33
#define NOTE_F5 0x2DC7
#define NOTE_Fs5 0x307C
#define NOTE_G5 0x3361
#define NOTE_Gs5 0x3671
#define NOTE_A5 0x39AB
#define NOTE_As5 0x3D1C
#define NOTE_B5 0x40BD
#define NOTE_C6 0x4493
#define NOTE_Cs6 0x48A5
#define NOTE_D6 0x4CF7
#define NOTE_Ds6 0x5194
#define NOTE_E6 0x5667
#define NOTE_F6 0x5B8F
#define NOTE_Fs6 0x60F8
#define NOTE_G6 0x66C2
#define NOTE_Gs6 0x6CE2
#define NOTE_A6 0x7357
#define NOTE_As6 0x7A38
#define NOTE_B6 0x817A
#define NOTE_C7 0x8927
#define NOTE_Cs7 0x914A
#define NOTE_D7 0x99EE
#define NOTE_Ds7 0xA328
#define NOTE_E7 0xACCE
#define NOTE_F7 0xB71F
#define NOTE_Fs7 0xC1F1
#define NOTE_G7 0xCD85
#define NOTE_Gs7 0xD9C4
#define NOTE_A7 0xE6AF
#define NOTE_As7 0xF471
#define NOTE_B7 0xFFFF // Max Frequency

// Common ADSR Presets
#define ADSR_PIANO       0x00, 0x09, 0x00, 0x00
#define ADSR_FLUTE       0x94, 0x00, 0xF0, 0x00
#define ADSR_GUITAR      0x00, 0x09, 0x20, 0x05
#define ADSR_ORGAN       0x11, 0x00, 0xF0, 0x00
#define ADSR_DRUM_KICK   0x01, 0x01, 0x00, 0x00
#define ADSR_DRUM_SNARE  0x00, 0x80, 0x00, 0x00

// Helpers to write to SID
#define SID_WRITE(offset, val) (*(volatile uint8_t*)(SID_BASE + offset) = (val))

void sid_set_volume(uint8_t vol) { SID_WRITE(MODE_VOL, vol & 0x0F); }

void sid_voice_freq(uint8_t voice, uint16_t freq) {
    uint8_t offset = (voice - 1) * 7;
    SID_WRITE(offset + FREQ_LO_1, freq & 0xFF);
    SID_WRITE(offset + FREQ_HI_1, (freq >> 8) & 0xFF);
}

void sid_voice_adsr(uint8_t voice, uint8_t attack, uint8_t decay,
                    uint8_t sustain, uint8_t release) {
    uint8_t offset = (voice - 1) * 7;
    SID_WRITE(offset + AD_1, (attack << 4) | (decay & 0x0F));
    SID_WRITE(offset + SR_1, (sustain << 4) | (release & 0x0F));
}

void sid_voice_control(uint8_t voice, uint8_t ctrl) {
    uint8_t offset = (voice - 1) * 7;
    SID_WRITE(offset + CTRL_1, ctrl);
}

void sid_voice_pw(uint8_t voice, uint16_t pw) {
    uint8_t offset = (voice - 1) * 7;
    SID_WRITE(offset + PW_LO_1, pw & 0xFF);
    SID_WRITE(offset + PW_HI_1, (pw >> 8) & 0x0F);
}

void sid_delay(uint16_t loop_count) {
    volatile uint16_t i;
    for (i = 0; i < loop_count; i++);
}
#endif
