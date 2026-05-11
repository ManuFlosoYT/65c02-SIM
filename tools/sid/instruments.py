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

        if 0 <= program <= 7: # Piano (0-7)
            wave = WAVE_PULSE
            attack_decay = 0x09
            sustain_release = (sus_nibble << 4) | 0x5
            pulse_width = 0x0600
        elif 8 <= program <= 15: # Chromatic Perc (8-15)
            wave = WAVE_TRI
            attack_decay = 0x05
            sustain_release = 0x05
        elif 16 <= program <= 23: # Organ (16-23)
            wave = WAVE_PULSE
            attack_decay = 0x11 # Fast
            sustain_release = 0xF2 # Full Sus
            pulse_width = 0x0400
        elif 24 <= program <= 31: # Guitar (24-31)
            wave = WAVE_SAW # Nylon/Steel
            if program >= 29: wave = WAVE_PULSE # Overdrive -> Square
            attack_decay = 0x09
            sustain_release = (sus_nibble << 4) | 0x4
        elif 32 <= program <= 39: # Bass (32-39)
            wave = WAVE_SAW
            if program in [33, 34]: wave = WAVE_PULSE # Finger/Pick
            elif program >= 38: wave = WAVE_TRI # Synth bass
            attack_decay = 0x05
            sustain_release = (sus_nibble << 4) | 0x4
            features.append("bass_filter")
            features.append("velocity_filter")
        elif 40 <= program <= 47: # Strings (40-47)
            wave = WAVE_SAW
            attack_decay = 0x68 # Slow attack
            sustain_release = 0x89 # Med sus, long release
            features.append("pwm_sweep") # Chorus effect
            features.append("vibrato")
        elif 48 <= program <= 55: # Ensemble (48-55)
            wave = WAVE_SAW
            attack_decay = 0x88 # Very slow attack
            sustain_release = 0xAA
            features.append("pwm_sweep")
        elif 56 <= program <= 63: # Brass (56-63)
            wave = WAVE_SAW
            attack_decay = 0x25
            sustain_release = (sus_nibble << 4) | 0x5
            features.append("velocity_filter")
        elif 64 <= program <= 71: # Reed (64-71)
            wave = WAVE_TRI
            attack_decay = 0x45
            sustain_release = (sus_nibble << 4) | 0x6
            features.append("vibrato")
        elif 72 <= program <= 79: # Pipe (72-79)
            wave = WAVE_TRI
            attack_decay = 0x56
            sustain_release = (sus_nibble << 4) | 0x6
            features.append("vibrato")
        elif 80 <= program <= 87: # Synth Lead (80-87)
            wave = WAVE_PULSE
            if program in [81, 87]: wave = WAVE_SAW
            attack_decay = 0x05
            sustain_release = (sus_nibble << 4) | 0x9
            features.append("pwm_sweep")
            features.append("vibrato")
        elif 88 <= program <= 95: # Synth Pad (88-95)
            wave = WAVE_SAW
            attack_decay = 0xAA # Slowest attack
            sustain_release = 0xAA # Slowest release
            features.append("pwm_sweep")
        elif 96 <= program <= 103: # Synth Effects (96-103)
            wave = WAVE_PULSE
            attack_decay = 0x2A
            sustain_release = 0x8A
            features.append("pwm_sweep")
        elif 104 <= program <= 111: # Ethnic (104-111)
            wave = WAVE_TRI
            attack_decay = 0x05
            sustain_release = (sus_nibble << 4) | 0x6
        elif 112 <= program <= 119: # Percussive (112-119)
            wave = WAVE_PULSE
            attack_decay = 0x01
            sustain_release = 0x04
        elif 120 <= program <= 127: # Sound Effects (120-127)
            wave = WAVE_NOISE
            attack_decay = 0x00
            sustain_release = 0x00
        elif program == 128: # APU Pulse (Raw)
            wave = WAVE_PULSE
            attack_decay = 0x00
            sustain_release = 0xF0 # Instant response, full sustain, instant release
        elif program == 129: # APU Triangle (Raw)
            wave = WAVE_TRI
            attack_decay = 0x00
            sustain_release = 0xF0
        elif program == 130: # APU Noise (Raw)
            wave = WAVE_NOISE
            attack_decay = 0x00
            sustain_release = 0xF0

    return wave, attack_decay, sustain_release, pulse_width, features
