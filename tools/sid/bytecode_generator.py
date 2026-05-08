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
        self.sid_state = [-1] * 25

    def _emit_reg(self, bytecode, reg, val):
        if self.sid_state[reg] != val:
            bytecode.extend([reg, val])
            self.sid_state[reg] = val

    def generate(self):
        """Generate and return the SID bytecode array from the event list."""
        print(f"Generating Bytecode from {len(self.events)} events...")
        bytecode = []

        # Init SID
        self._emit_reg(bytecode, MODE_VOL, 0x0F)

        # Reset Voices
        for i in range(1, 4):
            base = get_voice_offset(i)
            self._emit_reg(bytecode, base + CTRL_1, 0x00) # Gate Off
            self._emit_reg(bytecode, base + AD_1, 0x00)
            self._emit_reg(bytecode, base + SR_1, 0x00)

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

        import math

        channel_patches = {}
        channel_mod = {i: 0 for i in range(16)}
        channel_pitchbend = {i: 0 for i in range(16)}
        channel_pitchbend_range = {i: 2 for i in range(16)}
        channel_rpn_msb = {i: 127 for i in range(16)}
        channel_rpn_lsb = {i: 127 for i in range(16)}

        def emit_delay_and_effects(dt):
            # Fast check: active effects?
            has_effects = False
            for v in self.voices:
                if v.active:
                    ch = getattr(v, 'channel', 0)
                    if (len(getattr(v, 'arpeggio_notes', [])) > 1 or "pwm_sweep" in getattr(v, 'features', []) or
                        "vibrato" in getattr(v, 'features', []) or channel_pitchbend.get(ch, 0) != 0 or
                        channel_mod.get(ch, 0) > 0):
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
                    if loops <= 0xFF:
                        bytecode.extend([0x82, lo])
                    else:
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
                    if loops <= 0xFF:
                        bytecode.extend([0x82, lo])
                    else:
                        bytecode.extend([0x81, lo, hi])

                # Update Effects
                for v in self.voices:
                    if not v.active: continue
                    base = get_voice_offset(v.index)
                    ch = getattr(v, 'channel', 0)

                    # 1. Pitch Update (Arp, Bend, Vibrato)
                    note = getattr(v, 'note', 60)
                    is_arp = len(getattr(v, 'arpeggio_notes', [])) > 1
                    if is_arp:
                        v.arp_index = (v.arp_index + 1) % len(v.arpeggio_notes)
                        note = v.arpeggio_notes[v.arp_index]

                    pb = channel_pitchbend.get(ch, 0)
                    pb_range = channel_pitchbend_range.get(ch, 2)
                    semitones_shift = (pb / 8192.0) * pb_range

                    mod_val = channel_mod.get(ch, 0) / 127.0
                    if mod_val == 0 and "vibrato" in getattr(v, 'features', []):
                        mod_val = 0.3 # Intrinsic vibrato
                    
                    vib_semitones = 0.0
                    if mod_val > 0:
                        v.arp_counter += 1
                        lfo_phase = v.arp_counter * 0.5
                        vib_semitones = math.sin(lfo_phase) * 0.5 * mod_val

                    if semitones_shift != 0.0 or vib_semitones != 0.0 or is_arp:
                        freq = freq_for_note(note + semitones_shift + vib_semitones)
                        if freq < 0: freq = 0
                        if freq > 0xFFFF: freq = 0xFFFF
                        self._emit_reg(bytecode, base + FREQ_LO_1, int(freq) & 0xFF)
                        self._emit_reg(bytecode, base + FREQ_HI_1, (int(freq) >> 8) & 0xFF)

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
                        self._emit_reg(bytecode, base + PW_LO_1, cur & 0xFF)
                        self._emit_reg(bytecode, base + PW_HI_1, (cur >> 8) & 0xFF)

                remaining -= step

        last_time = 0.0

        for ev in self.events:
            dt = ev['time'] - last_time
            if dt > 0.0:
                emit_delay_and_effects(dt)
                last_time = ev['time']

            if ev['type'] == 'program_change':
                channel_patches[ev['channel']] = ev['program']

            elif ev['type'] == 'control_change':
                ch = ev['channel']
                ctrl = ev['control']
                val = ev['value']
                if ctrl == 7: # Volume
                    pass # Volume control disabled, kept at max
                elif ctrl == 1: # Modulation
                    channel_mod[ch] = val
                elif ctrl == 101: # RPN MSB
                    channel_rpn_msb[ch] = val
                elif ctrl == 100: # RPN LSB
                    channel_rpn_lsb[ch] = val
                elif ctrl == 6: # Data Entry MSB
                    if channel_rpn_msb.get(ch, 127) == 0 and channel_rpn_lsb.get(ch, 127) == 0:
                        channel_pitchbend_range[ch] = val

            elif ev['type'] == 'pitchwheel':
                ch = ev['channel']
                channel_pitchbend[ch] = ev['pitch']

            elif ev['type'] == 'note_on' or ev['type'] == 'arpeggio_on':
                notes = []
                note_val = 0
                if ev['type'] == 'arpeggio_on':
                    notes = ev['notes']
                    note_val = notes[-1] # Use highest note for priority
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
                    v.released = False
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
                    curr_ctrl = self.sid_state[base + CTRL_1]
                    if curr_ctrl != -1 and (curr_ctrl & WAVE_GATE):
                        self.sid_state[base + CTRL_1] = curr_ctrl & ~WAVE_GATE
                        bytecode.extend([0x93 + (v.index - 1)])

                    # 2. Setup Registers
                    self._emit_reg(bytecode, base + AD_1, ad)
                    self._emit_reg(bytecode, base + SR_1, sr)

                    # Pulse Width
                    if wave & (WAVE_PULSE | WAVE_TEST):
                        self._emit_reg(bytecode, base + PW_LO_1, pw & 0xFF)
                        self._emit_reg(bytecode, base + PW_HI_1, (pw >> 8) & 0xFF)

                    # Filter Setup (Bass LPF or Velocity Filter)
                    if "bass_filter" in features or "velocity_filter" in features:
                        is_melody = (ev['channel'] == self.melody_channel)
                        melody_active = any(vo.active and getattr(vo, 'channel', -1) == self.melody_channel for vo in self.voices)
                        
                        if is_melody or not melody_active:
                            filt_route = 1 << (v.index - 1)
                            
                            hi_val = 0x20
                            if "velocity_filter" in features:
                                hi_val = int((vel / 127.0) * 0xFF)
                                if hi_val < 0x10: hi_val = 0x10
                                
                            self._emit_reg(bytecode, CUTOFF_LO, 0x00)
                            self._emit_reg(bytecode, CUTOFF_HI, hi_val)
                            self._emit_reg(bytecode, RESON_FILT, 0x40 | filt_route)
                            
                            curr = self.sid_state[MODE_VOL]
                            if curr == -1: curr = 0x0F
                            self._emit_reg(bytecode, MODE_VOL, curr | 0x10) # Set LP filter

                    # 3. Trigger
                    if "kick_slide" in features:
                        start_freq = freq_for_note(note_val + 12)
                        self._emit_reg(bytecode, base + FREQ_LO_1, start_freq & 0xFF)
                        self._emit_reg(bytecode, base + FREQ_HI_1, (start_freq >> 8) & 0xFF)
                        self._emit_reg(bytecode, base + CTRL_1, wave | WAVE_GATE)
                        self._emit_reg(bytecode, base + FREQ_HI_1, ((start_freq >> 8) - 2) & 0xFF)
                        self._emit_reg(bytecode, base + FREQ_HI_1, ((start_freq >> 8) - 4) & 0xFF)
                        self._emit_reg(bytecode, base + FREQ_HI_1, ((start_freq >> 8) - 8) & 0xFF)
                    else:
                        cmd = 0x90 + (v.index - 1)
                        f_lo = freq & 0xFF
                        f_hi = (freq >> 8) & 0xFF
                        ctrl_val = wave | WAVE_GATE
                        
                        self.sid_state[base + FREQ_LO_1] = f_lo
                        self.sid_state[base + FREQ_HI_1] = f_hi
                        self.sid_state[base + CTRL_1] = ctrl_val
                        
                        bytecode.extend([cmd, f_lo, f_hi, ctrl_val])

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
                                base = get_voice_offset(v.index)
                                release_cmd = v.wave & (~WAVE_GATE)
                                self.sid_state[base + CTRL_1] = release_cmd
                                bytecode.extend([0x93 + (v.index - 1)])

                        elif v.note == ev['note']: # Fallback for single note
                            v.released = True
                            base = get_voice_offset(v.index)
                            release_cmd = v.wave & (~WAVE_GATE)
                            self.sid_state[base + CTRL_1] = release_cmd
                            bytecode.extend([0x93 + (v.index - 1)])

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
        # 0. Fast path: Inactive voice that already has this channel's affinity
        best_inactive = None
        for v in self.voices:
            if not v.active:
                if getattr(v, 'channel', None) == channel:
                    return v
                elif best_inactive is None:
                    best_inactive = v
        if best_inactive:
            return best_inactive

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

            # --- CHANNEL AFFINITY OPTIMIZATION ---
            # Affinity MUST outrank Release to ensure instruments stick to their voices!
            if getattr(v, 'channel', None) == channel:
                current_score -= 100000

            if current_score < min_score:
                min_score = current_score
                best_victim = v

        if best_victim:
            return best_victim

        return self.voices[0]
