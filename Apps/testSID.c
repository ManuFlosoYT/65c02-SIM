#include "Libs/app-bios.h"

// Helper to handle voice offsets
static void voice_freq(uint8_t voice, uint16_t freq) {
    uint8_t offset = (voice - 1) * 7;
    sid_write(offset + FREQ_LO_1, freq & 0xFF);
    sid_write(offset + FREQ_HI_1, (freq >> 8) & 0xFF);
}

static void voice_adsr(uint8_t voice, uint8_t attack, uint8_t decay, uint8_t sustain, uint8_t release) {
    uint8_t offset = (voice - 1) * 7;
    sid_write(offset + AD_1, (attack << 4) | (decay & 0x0F));
    sid_write(offset + SR_1, (sustain << 4) | (release & 0x0F));
}

static void voice_control(uint8_t voice, uint8_t ctrl) {
    uint8_t offset = (voice - 1) * 7;
    sid_write(offset + CTRL_1, ctrl);
}

static void voice_pw(uint8_t voice, uint16_t pw) {
    uint8_t offset = (voice - 1) * 7;
    sid_write(offset + PW_LO_1, pw & 0xFF);
    sid_write(offset + PW_HI_1, (pw >> 8) & 0x0F);
}

static void simple_delay(uint16_t count) {
    volatile uint16_t i;
    for (i = 0; i < count; i++);
}

int main() {
    uint16_t melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,
                         NOTE_G4, NOTE_E4, NOTE_C4, 0};
    uint16_t bass[] = {NOTE_C3, NOTE_C3, NOTE_G3, NOTE_G3,
                       NOTE_A3, NOTE_A3, NOTE_G3, NOTE_G3};
    uint8_t step = 0;

    println("microDOS SID Player");
    
    sid_reset();
    sid_write(MODE_VOL, 15);

    // Voice 1: Melody (Pulse)
    voice_adsr(1, 0, 9, 5, 0);
    voice_pw(1, 0x800);

    // Voice 2: Bass (Triangle)
    voice_adsr(2, 2, 9, 8, 9);

    // Voice 3: Drums (Noise)
    voice_adsr(3, 0, 5, 0, 0);

    while (1) {
        if (melody[step] != 0) {
            voice_freq(1, melody[step]);
            voice_control(1, WAVE_PULSE | WAVE_GATE);
        } else {
            voice_control(1, WAVE_PULSE);
        }

        voice_freq(2, bass[step]);
        voice_control(2, WAVE_TRI | WAVE_GATE);

        if (step % 2 == 0) {
            voice_freq(3, (step % 4 == 0) ? 0x1000 : 0x2000);
            voice_control(3, WAVE_NOISE | WAVE_GATE);
        } else {
            voice_control(3, WAVE_NOISE);
        }

        simple_delay(5000);

        // Release
        voice_control(1, WAVE_PULSE);
        voice_control(2, WAVE_TRI);
        voice_control(3, WAVE_NOISE);

        simple_delay(1000);

        step++;
        if (step >= 8) step = 0;
    }

    return 0;
}
