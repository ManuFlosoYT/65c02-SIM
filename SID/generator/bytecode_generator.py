# SID bytecode generation from processed MIDI events

from sid_constants import (
    SID_CLOCK, MODE_LEVEL_1, MODE_LEVEL_2, MODE_LEVEL_3,
    MODE_LEVEL_7, MODE_LEVEL_8,
    FREQ_LO_1, FREQ_HI_1, PW_LO_1, PW_HI_1, CTRL_1, AD_1, SR_1,
    WAVE_GATE, WAVE_PULSE, WAVE_TEST,
    CUTOFF_LO, CUTOFF_HI, RESON_FILT, MODE_VOL,
    get_voice_offset, freq_for_note,
)
from instruments import get_instrument_params


class Voice:
    def __init__(self, index):
        self.index = index
        self.note = None
        self.channel = None
        self.start_time = 0
        self.active = False
        self.released = False # Track release phase
        self.priority_score = 0
        self.wave = WAVE_PULSE # Default
        self.arpeggio_notes = []
        self.arp_index = 0
        self.arp_counter = 0
        self.pwm_counter = 0
        self.pwm_val = 0x800
        self.pwm_dir = 1
        self.features = []


class BytecodeGenerator:
    def __init__(self, events, mode, bass_channel, melody_channel):
        self.events = events
        self.mode = mode
        self.bass_channel = bass_channel
        self.melody_channel = melody_channel
        self.voices = [Voice(i) for i in range(1, 4)]

    def generate(self):
        """Generate and return the SID bytecode array from the event list."""
        print(f"Generating Bytecode from {len(self.events)} events...")
        bytecode = []

        # Init SID
        bytecode.extend([MODE_VOL, 0x0F])

        # Reset Voices
        for i in range(1, 4):
            base = get_voice_offset(i)
            bytecode.extend([base + CTRL_1, 0x00]) # Gate Off
            bytecode.extend([base + AD_1, 0x00, base + SR_1, 0x00])

            # Reset Python Voice State
            self.voices[i-1].arpeggio_notes = []
            self.voices[i-1].arp_index = 0
            self.voices[i-1].arp_counter = 0
            self.voices[i-1].pwm_val = 0x0800
            self.voices[i-1].pwm_dir = 1
            self.voices[i-1].features = []

        # L1-L3: High Quality (0.03s), L7-L8: Low Quality (0.10s), else Mid (0.06s)
        if self.mode in [MODE_LEVEL_1, MODE_LEVEL_2, MODE_LEVEL_3]:
            EFFECT_STEP = 0.03
        elif self.mode in [MODE_LEVEL_7, MODE_LEVEL_8]:
            EFFECT_STEP = 0.10
        else:
            EFFECT_STEP = 0.06

        self.next_arp_time = 0.0
        self.next_pwm_time = 0.0

        def emit_delay_and_effects(dt):
            # Fast check: active effects?
            has_effects = False
            for v in self.voices:
                if v.active and (len(v.arpeggio_notes) > 1 or "pwm_sweep" in v.features):
                    has_effects = True
                    break

            if not has_effects:
                # Just wait
                cycles = dt * SID_CLOCK
                loops = int(cycles / 15.0)
                if loops > 0:
                    if loops > 0xFFFF: loops = 0xFFFF
                    lo = loops & 0xFF
                    hi = (loops >> 8) & 0xFF
                    bytecode.extend([0x81, lo, hi])
                return

            # If we have effects, slice the time into steps
            remaining = dt
            while remaining > 0:
                step = remaining
                if step > EFFECT_STEP: step = EFFECT_STEP

                cycles = step * SID_CLOCK
                loops = int(cycles / 15.0)

                if loops > 0:
                    if loops > 0xFFFF:
                        loops = 0xFFFF
                    lo = loops & 0xFF
                    hi = (loops >> 8) & 0xFF
                    bytecode.extend([0x81, lo, hi])

                # Update Effects
                for v in self.voices:
                    if not v.active: continue
                    base = get_voice_offset(v.index)

                    # 1. Arpeggios (Every step)
                    if len(getattr(v, 'arpeggio_notes', [])) > 1:
                        v.arp_index = (v.arp_index + 1) % len(v.arpeggio_notes)
                        note = v.arpeggio_notes[v.arp_index]
                        freq = freq_for_note(note)
                        bytecode.extend([base + FREQ_LO_1, freq & 0xFF])
                        bytecode.extend([base + FREQ_HI_1, (freq >> 8) & 0xFF])

                    # 2. PWM Sweep (Every step)
                    if "pwm_sweep" in getattr(v, 'features', []):
                        cur = getattr(v, 'pwm_val', 0x800)
                        d = getattr(v, 'pwm_dir', 1)
                        cur += (64 * d) # Faster sweep since step is larger
                        if cur > 0x0E00:
                            cur = 0x0E00
                            v.pwm_dir = -1
                        elif cur < 0x0200:
                            cur = 0x0200
                            v.pwm_dir = 1
                        v.pwm_val = cur
                        bytecode.extend([base + PW_LO_1, cur & 0xFF, base + PW_HI_1, (cur >> 8) & 0xFF])

                remaining -= step

        last_time = 0.0
        channel_patches = {}

        for ev in self.events:
            dt = ev['time'] - last_time
            if dt > 0.0:
                emit_delay_and_effects(dt)
                last_time = ev['time']

            if ev['type'] == 'program_change':
                channel_patches[ev['channel']] = ev['program']

            elif ev['type'] == 'note_on' or ev['type'] == 'arpeggio_on':
                notes = []
                note_val = 0
                if ev['type'] == 'arpeggio_on':
                    notes = ev['notes']
                    note_val = notes[0]
                else:
                    notes = [ev['note']]
                    note_val = ev['note']

                v = self._allocate_voice(note_val, ev['channel'], ev['time'])
                if v:
                    v.note = note_val
                    v.channel = ev['channel']
                    v.active = True
                    v.start_time = ev['time']
                    v.priority_score = self._get_priority(note_val, ev['channel'], ev['time'])
                    v.arpeggio_notes = notes
                    v.arp_index = 0
                    v.arp_counter = 0
                    v.pwm_counter = 0

                    base = get_voice_offset(v.index)
                    freq = freq_for_note(notes[0])
                    if freq > 0xFFFF: freq = 0xFFFF

                    prog = channel_patches.get(ev['channel'], 0)
                    vel = ev.get('velocity', 100)

                    wave, ad, sr, pw, features = get_instrument_params(prog, ev['channel'], note_val, vel)
                    v.wave = wave
                    v.features = features
                    v.pwm_val = pw
                    v.pwm_dir = 1

                    # 1. Gate Off (Hard Restart)
                    bytecode.extend([base + CTRL_1, wave & ~WAVE_GATE])

                    # 2. Setup Registers
                    bytecode.extend([base + AD_1, ad, base + SR_1, sr])

                    # Pulse Width
                    if wave & (WAVE_PULSE | WAVE_TEST):
                        bytecode.extend([base + PW_LO_1, pw & 0xFF, base + PW_HI_1, (pw >> 8) & 0xFF])

                    # Filter Setup (Bass LPF)
                    if "bass_filter" in features:
                        filt_route = 1 << (v.index - 1)
                        bytecode.extend([CUTOFF_LO, 0x00, CUTOFF_HI, 0x20])
                        bytecode.extend([RESON_FILT, 0x40 | filt_route])
                        bytecode.extend([MODE_VOL, 0x1F])

                    # 3. Trigger
                    if "kick_slide" in features:
                        start_freq = freq_for_note(note_val + 12)
                        bytecode.extend([base + FREQ_LO_1, start_freq & 0xFF])
                        bytecode.extend([base + FREQ_HI_1, (start_freq >> 8) & 0xFF])
                        bytecode.extend([base + CTRL_1, wave | WAVE_GATE])
                        bytecode.extend([base + FREQ_HI_1, ((start_freq >> 8) - 2) & 0xFF])
                        bytecode.extend([base + FREQ_HI_1, ((start_freq >> 8) - 4) & 0xFF])
                        bytecode.extend([base + FREQ_HI_1, ((start_freq >> 8) - 8) & 0xFF])
                    else:
                        bytecode.extend([base + FREQ_LO_1, freq & 0xFF])
                        bytecode.extend([base + FREQ_HI_1, (freq >> 8) & 0xFF])
                        bytecode.extend([base + CTRL_1, wave | WAVE_GATE])

            elif ev['type'] == 'note_off':
                for v in self.voices:
                    if v.active and v.channel == ev.get('channel', 0):
                        # Check if this note is part of the voice (Arpeggio or Single)
                        if ev['note'] in getattr(v, 'arpeggio_notes', []):
                            # Remove it
                            if ev['note'] in v.arpeggio_notes:
                                v.arpeggio_notes.remove(ev['note'])

                            # If empty, enter Release phase
                            if not v.arpeggio_notes:
                                v.released = True
                                # v.active stays True so tail plays and effects run!
                                base = get_voice_offset(v.index)
                                release_cmd = v.wave & (~WAVE_GATE)
                                bytecode.extend([base + CTRL_1, release_cmd])

                        elif v.note == ev['note']: # Fallback for single note
                            v.released = True
                            # v.active = False <-- REMOVED! Keep active for tail.
                            base = get_voice_offset(v.index)
                            release_cmd = v.wave & (~WAVE_GATE)
                            bytecode.extend([base + CTRL_1, release_cmd])

        bytecode.append(0xFF)
        return bytecode

    def _get_priority(self, note, channel, now):
        score = 0
        if channel == self.melody_channel: score += 8000
        elif channel == self.bass_channel: score += 5000
        elif channel == 9: score += 3000

        if note > 80: score += 500
        if note < 40: score += 500

        return score

    def _allocate_voice(self, note, channel, time):
        # 1. Try to find a completely inactive voice
        for v in self.voices:
            if not v.active:
                return v

        # 2. Find best victim
        # Criteria: Released > Quietest > Oldest
        min_score = 999999
        best_victim = None

        for v in self.voices:
            age = time - v.start_time
            age_bonus = 0
            if age < 0.05: age_bonus = 10000

            # Base priority
            current_score = v.priority_score + age_bonus

            # If voice is in Release phase, it is a PRIME CANDIDATE for stealing
            if getattr(v, 'released', False):
                current_score -= 50000

            if current_score < min_score:
                min_score = current_score
                best_victim = v

        if best_victim:
            return best_victim

        return self.voices[0]
