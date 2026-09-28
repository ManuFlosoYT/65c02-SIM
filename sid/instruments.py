# Instrument patch definitions (General MIDI mapping for SID)

import json
import os
from sid_constants import (
    WAVE_TRI, WAVE_SAW, WAVE_PULSE, WAVE_NOISE,
)

_instruments_data = None

def _load_instruments():
    global _instruments_data
    if _instruments_data is None:
        json_path = os.path.join(os.path.dirname(__file__), "instruments.json")
        with open(json_path, "r") as f:
            _instruments_data = json.load(f)

def _parse_wave_array(waves_array):
    wave_map = {
        "TRI": WAVE_TRI,
        "SAW": WAVE_SAW,
        "PULSE": WAVE_PULSE,
        "NOISE": WAVE_NOISE
    }
    result = 0
    for w in waves_array:
        result |= wave_map.get(w.upper(), 0)
    return result

def get_instrument_params(program, channel, note, velocity, chip="8580"):
    """
    Returns SID voice parameters for a given GM program, channel, note and velocity.
    """
    _load_instruments()
    
    is_drum = (channel == 9)
    config = None
    
    if is_drum:
        # Check percussion mapping
        perc_map = _instruments_data.get("percussion", {})
        for key, val in perc_map.items():
            if key == "default": continue
            notes = [int(n) for n in key.split(",")]
            if note in notes:
                config = val
                break
        if not config:
            config = perc_map.get("default", {})
    else:
        # Check melodic mapping
        melodic_map = _instruments_data.get("melodic", {})
        for key, val in melodic_map.items():
            if "-" in key:
                parts = key.split("-")
                if int(parts[0]) <= program <= int(parts[1]):
                    config = val
                    break
            else:
                if program == int(key):
                    config = val
                    break
                    
    # Fallback default
    if not config:
        config = {
            "waves": ["PULSE"],
            "attack_decay": "0x09",
            "sustain_release_base": "0x05",
            "pulse_width": "0x0800",
            "features": []
        }
        
    wave = _parse_wave_array(config.get("waves", ["PULSE"]))
    attack_decay = int(config.get("attack_decay", "0x09"), 16)
    sr_base = int(config.get("sustain_release_base", "0x05"), 16)
    pulse_width = int(config.get("pulse_width", "0x0800"), 16)
    features = list(config.get("features", []))
    
    # Velocity -> Sustain Level mapping (High nibble of SR)
    if not is_drum:
        sus_nibble = int((velocity / 127.0) * 15)
        if sus_nibble < 4: sus_nibble = 4 # Min sustain
        # Some configs might hardcode a 0 sustain base (like APU waves)
        if sr_base > 0x0F: 
            sustain_release = sr_base # It provided the full byte (e.g. 0xF0)
        else:
            sustain_release = (sus_nibble << 4) | (sr_base & 0x0F)
    else:
        sustain_release = sr_base
        
    # Remove chip specific features if not supported
    if chip != "6581" and "pcm_sample" in features:
        features.remove("pcm_sample")

    # 6581 Noise Mixing Bug Fix
    if chip == "6581":
        if (wave & WAVE_NOISE) and (wave != WAVE_NOISE):
            wave = WAVE_NOISE # Fallback to pure noise to avoid 0V short

    return wave, attack_decay, sustain_release, pulse_width, features
