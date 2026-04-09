# Instrument patch definitions (General MIDI mapping for SID)

from sid_constants import (
    WAVE_TRI, WAVE_SAW, WAVE_PULSE, WAVE_NOISE,
)


def get_instrument_params(program, channel, note, velocity):
    """
    Returns SID voice parameters for a given GM program, channel, note and velocity.

    Returns:
        tuple: (wave, attack_decay, sustain_release, pulse_width, features)
    """
    # Default: Pulse with generic envelope
    wave = WAVE_PULSE
    attack_decay = 0x09  # Short Attack, Long Decay
    sustain_release = 0xF5 # High Sustain, Medium Release
    pulse_width = 0x0800 # 50%

    is_drum = (channel == 9)
    features = []

    if is_drum:
        # Drum Map (GM)
        if note in [35, 36]: # Kick
            wave = WAVE_TRI
            attack_decay = 0x05 # Fast attack, med decay
            sustain_release = 0x00 # No sustain, fast release
            features.append("kick_slide")
        elif note in [38, 40]: # Snare
            wave = WAVE_NOISE
            attack_decay = 0x05
            sustain_release = 0x00
            features.append("snare_macro") # Use macro for noise+tone
        elif note in [42, 44, 46]: # HiHats
            wave = WAVE_NOISE
            attack_decay = 0x00 # Instant
            sustain_release = 0x04 # Short release
        elif note in [49, 57]: # Crash
            wave = WAVE_NOISE
            attack_decay = 0x0F # Soft
            sustain_release = 0x08 # Long release
        else: # Generic Perc
            wave = WAVE_NOISE
            attack_decay = 0x05
            sustain_release = 0x40

    else:
        # Melodic Instruments
        # Velocity -> Sustain Level mapping (High nibble of SR)
        sus_nibble = int((velocity / 127.0) * 15)
        if sus_nibble < 4: sus_nibble = 4 # Min sustain

        # Base patches
        if 0 <= program <= 7: # Piano
            wave = WAVE_PULSE
            attack_decay = 0x09
            sustain_release = (0x8 << 4) | 0x5 # Sus 8, Rel 5
            pulse_width = 0x0600
        elif 8 <= program <= 15: # Chromatic Perc (Glock etc)
            wave = WAVE_TRI
            attack_decay = 0x05
            sustain_release = 0x05 # Low sustain
        elif 16 <= program <= 23: # Organ
            wave = WAVE_PULSE
            attack_decay = 0x11 # Fast
            sustain_release = 0xF2 # Full Sus
            pulse_width = 0x0400
        elif 24 <= program <= 31: # Guitar
            wave = WAVE_SAW # Nylon/Steel
            if program >= 29: wave = WAVE_PULSE # Overdrive -> Square
            attack_decay = 0x09
            sustain_release = 0xA4
        elif 32 <= program <= 39: # Bass
            wave = WAVE_SAW # Or Triangle? Saw is punchier
            if program in [33, 34]: wave = WAVE_PULSE # Finger/Pick
            attack_decay = 0x05
            sustain_release = 0xA4
            features.append("bass_filter")
        elif 40 <= program <= 47: # Strings
            wave = WAVE_SAW
            attack_decay = 0x68 # Slow attack
            sustain_release = 0x89 # Med sus, long release
            features.append("pwm_sweep") # Chorus effect
        elif 56 <= program <= 63: # Brass
            wave = WAVE_SAW
            attack_decay = 0x25
            sustain_release = 0x85
        elif 80 <= program <= 95: # Lead
            wave = WAVE_PULSE # Square leads
            if program in [81, 87]: wave = WAVE_SAW
            attack_decay = 0x05
            sustain_release = 0xC9 # Long release for smoother transitions
            features.append("pwm_sweep")
        else:
            # Default fallback
            wave = WAVE_PULSE
            attack_decay = 0x09
            sustain_release = (sus_nibble << 4) | 0x9 # 750ms release

    return wave, attack_decay, sustain_release, pulse_width, features
