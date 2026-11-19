
#include <stdbool.h>

#include "Libs/SID.h"

int main(void) {
    const uint16_t melody[] = {
        NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,
        NOTE_G4, NOTE_E4, NOTE_C4, 0,
    };
    const uint16_t bass[] = {
        NOTE_C3, NOTE_C3, NOTE_G3, NOTE_G3,
        NOTE_A3, NOTE_A3, NOTE_G3, NOTE_G3,
    };
    uint8_t step = 0;

    sid_set_volume(15);

    sid_voice_adsr(1, 0, 9, 5, 0);
    sid_voice_pw(1, 0x800U);

    sid_voice_adsr(2, 2, 9, 8, 9);

    sid_voice_adsr(3, 0, 5, 0, 0);

    while (true) {
        if (melody[step] != 0U) {
            sid_voice_freq(1, melody[step]);
            sid_voice_control(1, (uint8_t)((unsigned int)WAVE_PULSE | (unsigned int)WAVE_GATE));
        } else {
            sid_voice_control(1, WAVE_PULSE);
        }

        sid_voice_freq(2, bass[step]);
        sid_voice_control(2, (uint8_t)((unsigned int)WAVE_SAW | (unsigned int)WAVE_GATE));

        if (step % 2 == 0) {
            if (step % 4 == 0) {
                sid_voice_freq(3, 0x1000U);
                sid_voice_control(3, (uint8_t)((unsigned int)WAVE_NOISE | (unsigned int)WAVE_GATE));
            } else {
                sid_voice_freq(3, 0x2000U);
                sid_voice_control(3, (uint8_t)((unsigned int)WAVE_NOISE | (unsigned int)WAVE_GATE));
            }
        } else {
            sid_voice_control(3, WAVE_NOISE);
        }

        sid_delay(5000);

        sid_voice_control(1, WAVE_PULSE);
        sid_voice_control(2, WAVE_SAW);
        sid_voice_control(3, WAVE_NOISE);

        sid_delay(1000);

        step++;
        if (step >= 8) {
            step = 0;
        }
    }

    return 0;
}
