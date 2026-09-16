# SID chip constants and utility functions

SID_CHANNELS = 3
CLOCK_NTSC = 1000000.0  # 1 MHz
CLOCK_PAL = 985248.0

# --- SID Voice Register Offsets (relative to voice base) ---
FREQ_LO_1 = 0x00
FREQ_HI_1 = 0x01
PW_LO_1   = 0x02
PW_HI_1   = 0x03
CTRL_1    = 0x04
AD_1      = 0x05
SR_1      = 0x06

# --- Waveform Flags ---
WAVE_GATE  = 0x01
WAVE_SYNC  = 0x02
WAVE_RING  = 0x04
WAVE_TEST  = 0x08
WAVE_TRI   = 0x10
WAVE_SAW   = 0x20
WAVE_PULSE = 0x40
WAVE_NOISE = 0x80

# --- Global SID Registers ---
ENV_1     = 0x1C
CUTOFF_LO = 0x15
CUTOFF_HI = 0x16
RESON_FILT= 0x17
MODE_VOL  = 0x18

# --- Filter Modes ---
FILT_LP = 0x10
FILT_BP = 0x20
FILT_HP = 0x40
FILT_OFF3 = 0x80

# --- Mode Level Constants ---
MODE_MAX_QUALITY = "max_quality"
MODE_QUALITY = "quality"
MODE_BALANCED = "balanced"
MODE_COMPRESSED = "compressed"
MODE_MAX_COMPRESSION = "max_compression"

ALL_MODES = [
    MODE_MAX_QUALITY, MODE_QUALITY, MODE_BALANCED,
    MODE_COMPRESSED, MODE_MAX_COMPRESSION
]


def get_voice_offset(voice_idx):
    """Returns the SID register base offset for a given 1-indexed voice."""
    return (voice_idx - 1) * 7


def freq_for_note(note_num):
    """Converts a MIDI note number to a SID frequency register value."""
    freq_hz = 440.0 * (2.0 ** ((note_num - 69.0) / 12.0))
    sid_freq = int(freq_hz * 16.777216)
    return sid_freq
