
#include "sid.h"

// Note Frequencies (Approx)
// Using defines from sid.h

int main() {
    uint16_t melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,
                         NOTE_G4, NOTE_E4, NOTE_C4, 0};
    uint16_t bass[] = {NOTE_C3, NOTE_C3, NOTE_G3, NOTE_G3,
                       NOTE_A3, NOTE_A3, NOTE_G3, NOTE_G3};
    uint8_t step = 0;

    // init SID
    sid_set_volume(15);

    // Voice 1: Melody (Pulse)
    sid_voice_adsr(1, 0, 9, 5, 0);  // A=2ms, D=750ms, S=Mid, R=6ms
    sid_voice_pw(1, 0x800);         // 50% Pulse

    // Voice 2: Bass (Triangle)
    sid_voice_adsr(2, 2, 9, 8, 9);  // Softer attack

    // Voice 3: Drums (Noise)
    sid_voice_adsr(3, 0, 5, 0, 0);  // Quick sharp AD

    while (1) {
        // --- 1. MELODY ---
        if (melody[step] != 0) {
            sid_voice_freq(1, melody[step]);
            sid_voice_control(1, WAVE_PULSE | WAVE_GATE);
        } else {
            sid_voice_control(1, WAVE_PULSE);  // Release
        }

        // --- 2. BASS ---
        sid_voice_freq(2, bass[step]);
        sid_voice_control(2, WAVE_SAW | WAVE_GATE);

        // --- 3. DRUMS ---
        // Simple beat: Kick (Tri) on 0,4. Snare (Noise) on 2,6
        if (step % 2 == 0) {
            // Kick-ish (Low Pulse/Tri)
            // Actually, let's use Noise for everything for simplicity in
            // "Bateria" context, or switch waveforms. Switching waveforms might
            // be glitchy if not careful. Let's use Voice 3 for Hi-Hat/Snare
            // (Noise)

            if (step % 4 == 0) {
                // Strong beat
                sid_voice_freq(3, 0x1000);
                sid_voice_control(3, WAVE_NOISE | WAVE_GATE);
            } else {
                // Off beat
                sid_voice_freq(3, 0x2000);
                sid_voice_control(3, WAVE_NOISE | WAVE_GATE);
            }
        } else {
            // Silence drum
            sid_voice_control(3, WAVE_NOISE);
        }

        // --- HOLD ---
        sid_delay(5000);

        // --- RELEASE ---
        sid_voice_control(1, WAVE_PULSE);
        sid_voice_control(2, WAVE_SAW);
        sid_voice_control(3, WAVE_NOISE);

        // --- GAP ---
        sid_delay(1000);

        step++;
        if (step >= 8) step = 0;
    }

    return 0;
}
