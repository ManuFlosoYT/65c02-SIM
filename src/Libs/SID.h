#pragma once

#include <stdint.h>

#define SID_BASE 0x4800U

enum SidRegister {
    FREQ_LO_1 = 0x00U,
    FREQ_HI_1 = 0x01U,
    PW_LO_1   = 0x02U,
    PW_HI_1   = 0x03U,
    CTRL_1    = 0x04U,
    AD_1      = 0x05U,
    SR_1      = 0x06U,

    FREQ_LO_2 = 0x07U,
    FREQ_HI_2 = 0x08U,
    PW_LO_2   = 0x09U,
    PW_HI_2   = 0x0AU,
    CTRL_2    = 0x0BU,
    AD_2      = 0x0CU,
    SR_2      = 0x0DU,

    FREQ_LO_3 = 0x0EU,
    FREQ_HI_3 = 0x0FU,
    PW_LO_3   = 0x10U,
    PW_HI_3   = 0x11U,
    CTRL_3    = 0x12U,
    AD_3      = 0x13U,
    SR_3      = 0x14U,

    FC_LO     = 0x15U,
    FC_HI     = 0x16U,
    RES_FILT  = 0x17U,
    MODE_VOL  = 0x18U,
};

enum SidWaveform {
    WAVE_GATE  = 0x01U,
    WAVE_SYNC  = 0x02U,
    WAVE_RING  = 0x04U,
    WAVE_TEST  = 0x08U,
    WAVE_TRI   = 0x10U,
    WAVE_SAW   = 0x20U,
    WAVE_PULSE = 0x40U,
    WAVE_NOISE = 0x80U,
};

enum SidNoteFreq {
    NOTE_C0  = 0x0112U,
    NOTE_Cs0 = 0x0122U,
    NOTE_D0  = 0x0133U,
    NOTE_Ds0 = 0x0146U,
    NOTE_E0  = 0x0159U,
    NOTE_F0  = 0x016EU,
    NOTE_Fs0 = 0x0183U,
    NOTE_G0  = 0x019BU,
    NOTE_Gs0 = 0x01B3U,
    NOTE_A0  = 0x01CDU,
    NOTE_As0 = 0x01E8U,
    NOTE_B0  = 0x0205U,
    NOTE_C1  = 0x0224U,
    NOTE_Cs1 = 0x0245U,
    NOTE_D1  = 0x0267U,
    NOTE_Ds1 = 0x028CU,
    NOTE_E1  = 0x02B3U,
    NOTE_F1  = 0x02DCU,
    NOTE_Fs1 = 0x0307U,
    NOTE_G1  = 0x0336U,
    NOTE_Gs1 = 0x0367U,
    NOTE_A1  = 0x039AU,
    NOTE_As1 = 0x03D1U,
    NOTE_B1  = 0x040BU,
    NOTE_C2  = 0x0449U,
    NOTE_Cs2 = 0x048AU,
    NOTE_D2  = 0x04CFU,
    NOTE_Ds2 = 0x0519U,
    NOTE_E2  = 0x0566U,
    NOTE_F2  = 0x05B8U,
    NOTE_Fs2 = 0x060FU,
    NOTE_G2  = 0x066CU,
    NOTE_Gs2 = 0x06CEU,
    NOTE_A2  = 0x0735U,
    NOTE_As2 = 0x07A3U,
    NOTE_B2  = 0x0817U,
    NOTE_C3  = 0x0892U,
    NOTE_Cs3 = 0x0914U,
    NOTE_D3  = 0x099EU,
    NOTE_Ds3 = 0x0A32U,
    NOTE_E3  = 0x0ACCU,
    NOTE_F3  = 0x0B71U,
    NOTE_Fs3 = 0x0C1FU,
    NOTE_G3  = 0x0CD8U,
    NOTE_Gs3 = 0x0D9CU,
    NOTE_A3  = 0x0E6AU,
    NOTE_As3 = 0x0F47U,
    NOTE_B3  = 0x102FU,
    NOTE_C4  = 0x1124U,
    NOTE_Cs4 = 0x1229U,
    NOTE_D4  = 0x133DU,
    NOTE_Ds4 = 0x1465U,
    NOTE_E4  = 0x1599U,
    NOTE_F4  = 0x16E3U,
    NOTE_Fs4 = 0x183EU,
    NOTE_G4  = 0x19B0U,
    NOTE_Gs4 = 0x1B38U,
    NOTE_A4  = 0x1CD5U,
    NOTE_As4 = 0x1E8EU,
    NOTE_B4  = 0x205EU,
    NOTE_C5  = 0x2249U,
    NOTE_Cs5 = 0x2452U,
    NOTE_D5  = 0x267BU,
    NOTE_Ds5 = 0x28CAU,
    NOTE_E5  = 0x2B33U,
    NOTE_F5  = 0x2DC7U,
    NOTE_Fs5 = 0x307CU,
    NOTE_G5  = 0x3361U,
    NOTE_Gs5 = 0x3671U,
    NOTE_A5  = 0x39ABU,
    NOTE_As5 = 0x3D1CU,
    NOTE_B5  = 0x40BDU,
    NOTE_C6  = 0x4493U,
    NOTE_Cs6 = 0x48A5U,
    NOTE_D6  = 0x4CF7U,
    NOTE_Ds6 = 0x5194U,
    NOTE_E6  = 0x5667U,
    NOTE_F6  = 0x5B8FU,
    NOTE_Fs6 = 0x60F8U,
    NOTE_G6  = 0x66C2U,
    NOTE_Gs6 = 0x6CE2U,
    NOTE_A6  = 0x7357U,
    NOTE_As6 = 0x7A38U,
    NOTE_B6  = 0x817AU,
    NOTE_C7  = 0x8927U,
    NOTE_Cs7 = 0x914AU,
    NOTE_D7  = 0x99EEU,
    NOTE_Ds7 = 0xA328U,
    NOTE_E7  = 0xACCEU,
    NOTE_F7  = 0xB71FU,
    NOTE_Fs7 = 0xC1F1U,
    NOTE_G7  = 0xCD85U,
    NOTE_Gs7 = 0xD9C4U,
    NOTE_A7  = 0xE6AFU,
    NOTE_As7 = 0xF471U,
    NOTE_B7  = 0xFFFFU,
};

#define ADSR_PIANO       0x00U, 0x09U, 0x00U, 0x00U
#define ADSR_FLUTE       0x94U, 0x00U, 0xF0U, 0x00U
#define ADSR_GUITAR      0x00U, 0x09U, 0x20U, 0x05U
#define ADSR_ORGAN       0x11U, 0x00U, 0xF0U, 0x00U
#define ADSR_DRUM_KICK   0x01U, 0x01U, 0x00U, 0x00U
#define ADSR_DRUM_SNARE  0x00U, 0x80U, 0x00U, 0x00U

#define SID_WRITE(offset, val) (*(volatile uint8_t*)(SID_BASE + (offset)) = (val))

static inline void sid_set_volume(uint8_t vol) {
    SID_WRITE(MODE_VOL, (uint8_t)(vol & 0x0FU));
}

static inline void sid_voice_freq(uint8_t voice, uint16_t freq) {
    uint8_t offset = (uint8_t)((voice - 1U) * 7U);
    SID_WRITE((uint8_t)(offset + FREQ_LO_1), (uint8_t)(freq & 0xFFU));
    SID_WRITE((uint8_t)(offset + FREQ_HI_1), (uint8_t)((freq >> 8U) & 0xFFU));
}

static inline void sid_trigger_note(uint8_t voice, uint16_t freq, uint8_t ctrl) {
    uint8_t offset = (uint8_t)((voice - 1U) * 7U);
    SID_WRITE((uint8_t)(offset + FREQ_LO_1), (uint8_t)(freq & 0xFFU));
    SID_WRITE((uint8_t)(offset + FREQ_HI_1), (uint8_t)((freq >> 8U) & 0xFFU));
    SID_WRITE((uint8_t)(offset + CTRL_1), ctrl);
}

static inline void sid_voice_adsr(uint8_t voice, uint8_t attack, uint8_t decay,
                                  uint8_t sustain, uint8_t release) {
    uint8_t offset = (uint8_t)((voice - 1U) * 7U);
    uint8_t ad = (uint8_t)(((unsigned int)attack << 4U) | ((unsigned int)decay & 0x0FU));
    uint8_t sr = (uint8_t)(((unsigned int)sustain << 4U) | ((unsigned int)release & 0x0FU));
    SID_WRITE((uint8_t)(offset + AD_1), ad);
    SID_WRITE((uint8_t)(offset + SR_1), sr);
}

static inline void sid_voice_control(uint8_t voice, uint8_t ctrl) {
    uint8_t offset = (uint8_t)((voice - 1U) * 7U);
    SID_WRITE((uint8_t)(offset + CTRL_1), ctrl);
}

static inline void sid_voice_pw(uint8_t voice, uint16_t pw) {
    uint8_t offset = (uint8_t)((voice - 1U) * 7U);
    SID_WRITE((uint8_t)(offset + PW_LO_1), (uint8_t)(pw & 0xFFU));
    SID_WRITE((uint8_t)(offset + PW_HI_1), (uint8_t)((pw >> 8U) & 0x0FU));
}

static inline void sid_delay(uint16_t loop_count) {
    for (volatile uint16_t i = 0; i < loop_count; ++i) {
    }
}
