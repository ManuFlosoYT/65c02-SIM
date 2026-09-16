# SID bytecode generation from processed MIDI events

from sid_constants import (
    CLOCK_NTSC, CLOCK_PAL, MODE_LEVEL_1, MODE_LEVEL_2, MODE_LEVEL_3,
    MODE_LEVEL_7, MODE_LEVEL_8,
    FREQ_LO_1, FREQ_HI_1, PW_LO_1, PW_HI_1, CTRL_1, AD_1, SR_1,
    WAVE_GATE, WAVE_PULSE, WAVE_TEST, WAVE_SYNC, WAVE_RING,
    CUTOFF_LO, CUTOFF_HI, RESON_FILT, MODE_VOL,
    FILT_LP, FILT_BP, FILT_HP, FILT_OFF3,
    get_voice_offset, freq_for_note,
)
from instruments import get_instrument_params
import math


class Voice:
    def __init__(self, index):
        self.index = index
        self.note = None
        self.channel = None
        self.start_time = 0
        self.active = False
        self.released = False # Track release phase
        self.release_timer = 0.0
        self.priority_score = 0
        self.wave = WAVE_PULSE # Default
        self.arpeggio_notes = []
        self.arp_index = 0
        self.arp_counter = 0
        self.pwm_counter = 0
        self.pwm_val = 0x800
        self.pwm_dir = 1
        self.features = []
        self.is_modulator = False


class BytecodeGenerator:
    def __init__(self, events, mode, chip="8580", system="NTSC", bass_channel=-1, melody_channel=-1, noise_channel=-1):
        self.events = events
        self.mode = mode
        self.chip = chip
        self.system = system
        self.bass_channel = bass_channel
        self.melody_channel = melody_channel
        self.noise_channel = noise_channel
        self.voices = [Voice(i) for i in range(1, 4)]
        self.sid_state = [-1] * 25
        self.sid_clock = CLOCK_NTSC if system == "NTSC" else CLOCK_PAL

    def _freq_to_cutoff_dac(self, hz):
        if self.chip == "6581":
            dac = int(2047.0 * math.pow(hz / 12500.0, 0.4))
            return max(0, min(2047, dac))
        else:
            dac = int((hz / 12500.0) * 2047.0)
            return max(0, min(2047, dac))

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
            self.voices[i-1].is_modulator = False

        # L1-L3: High Quality (0.04s), L7-L8: Low Quality (0.10s), else Mid (0.08s)
        if self.mode in [MODE_LEVEL_1, MODE_LEVEL_2, MODE_LEVEL_3]:
            EFFECT_STEP = 0.04
        elif self.mode in [MODE_LEVEL_7, MODE_LEVEL_8]:
            EFFECT_STEP = 0.10
        else:
            EFFECT_STEP = 0.08

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

        def emit_delay_bytes(dt):
            cycles = dt * self.sid_clock
            loops = int(cycles / 15.0)
            while loops > 0:
                cur = min(loops, 0xFFFF)
                lo = cur & 0xFF
                hi = (cur >> 8) & 0xFF
                if cur <= 0xFF:
                    bytecode.extend([0x82, lo])
                else:
                    bytecode.extend([0x81, lo, hi])
                loops -= cur

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
                emit_delay_bytes(dt)
                return

            # If we have effects, slice the time into steps and accumulate delays
            remaining = dt
            accumulated_dt = 0.0

            while remaining > 0:
                step = min(remaining, EFFECT_STEP)
                accumulated_dt += step
                remaining -= step

                # Record length of bytecode before updating effects
                prev_len = len(bytecode)

                # Update Effects
                for v in self.voices:
                    if not v.active: continue
                    if getattr(v, 'released', False):
                        v.release_timer += step
                        if v.release_timer > 0.8:
                            v.active = False
                            continue
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
                        if ch == self.melody_channel:
                            is_highest = True
                            for other_v in self.voices:
                                if other_v.active and other_v.index != v.index and getattr(other_v, 'channel', -1) == ch:
                                    if getattr(other_v, 'note', 0) > getattr(v, 'note', 0):
                                        is_highest = False
                                        break
                            if is_highest:
                                mod_val = 0.3 # Intrinsic vibrato ONLY for highest melody note!
                    
                    vib_semitones = 0.0
                    if mod_val > 0:
                        v.arp_counter += 1
                        lfo_phase = v.arp_counter * 0.5
                        vib_semitones = math.sin(lfo_phase) * 0.5 * mod_val
                        vib_semitones = round(vib_semitones * 16.0) / 16.0

                    if semitones_shift != 0.0 or vib_semitones != 0.0 or is_arp:
                        freq = freq_for_note(note + semitones_shift + vib_semitones)
                        if freq < 0: freq = 0
                        if freq > 0xFFFF: freq = 0xFFFF
                        self._emit_reg(bytecode, base + FREQ_LO_1, int(freq) & 0xFF)
                        self._emit_reg(bytecode, base + FREQ_HI_1, (int(freq) >> 8) & 0xFF)

                    # 2. PWM Sweep (Every step)
                    if "pwm_sweep" in getattr(v, 'features', []) and not getattr(v, 'released', False):
                        cur = getattr(v, 'pwm_val', 0x800)
                        d = getattr(v, 'pwm_dir', 1)
                        cur += (128 * d) # Balanced PWM sweep step
                        if cur > 0x0E00:
                            cur = 0x0E00
                            v.pwm_dir = -1
                        elif cur < 0x0200:
                            cur = 0x0200
                            v.pwm_dir = 1
                        v.pwm_val = cur
                        self._emit_reg(bytecode, base + PW_LO_1, cur & 0xFF)
                        self._emit_reg(bytecode, base + PW_HI_1, (cur >> 8) & 0xFF)

                # If registers were actually written during this step, prepend the accumulated delay
                if len(bytecode) > prev_len:
                    new_writes = bytecode[prev_len:]
                    del bytecode[prev_len:]
                    
                    # Flush accumulated delay first
                    cycles = accumulated_dt * self.sid_clock
                    loops = int(cycles / 15.0)
                    while loops > 0:
                        c = min(loops, 0xFFFF)
                        lo = c & 0xFF
                        hi = (c >> 8) & 0xFF
                        if c <= 0xFF:
                            bytecode.extend([0x82, lo])
                        else:
                            bytecode.extend([0x81, lo, hi])
                        loops -= c
                    
                    # Append new writes after delay
                    bytecode.extend(new_writes)
                    accumulated_dt = 0.0

            # Flush any remaining accumulated delay at the end of dt
            if accumulated_dt > 0.0:
                cycles = accumulated_dt * self.sid_clock
                loops = int(cycles / 15.0)
                while loops > 0:
                    c = min(loops, 0xFFFF)
                    lo = c & 0xFF
                    hi = (c >> 8) & 0xFF
                    if c <= 0xFF:
                        bytecode.extend([0x82, lo])
                    else:
                        bytecode.extend([0x81, lo, hi])
                    loops -= c

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
                    val = ev['value']
                    sustain = val >> 3 # 0-127 -> 0-15
                    for v in self.voices:
                        if getattr(v, 'channel', -1) == ch and v.active:
                            patch = channel_patches.get(ch, 0)
                            if 128 <= patch <= 130:
                                base = get_voice_offset(v.index)
                                sr = (sustain << 4) | 0x00
                                self._emit_reg(bytecode, base + SR_1, sr)
                elif ctrl == 1: # Modulation
                    channel_mod[ch] = val
                elif ctrl == 101: # RPN MSB
                    channel_rpn_msb[ch] = val
                elif ctrl == 100: # RPN LSB
                    channel_rpn_lsb[ch] = val
                elif ctrl == 6: # Data Entry MSB
                    if channel_rpn_msb.get(ch, 127) == 0 and channel_rpn_lsb.get(ch, 127) == 0:
                        channel_pitchbend_range[ch] = val
                elif ctrl == 14: # Custom PW (NSF)
                    # 0: 12.5%, 1: 25%, 2: 50%, 3: 25% negated
                    duty_map = {0: 0x0200, 1: 0x0400, 2: 0x0800, 3: 0x0C00}
                    pw_val = duty_map.get(val, 0x0800)
                    for v in self.voices:
                        if v.active and getattr(v, 'channel', None) == ch:
                            base = get_voice_offset(v.index)
                            self._emit_reg(bytecode, base + PW_LO_1, pw_val & 0xFF)
                            self._emit_reg(bytecode, base + PW_HI_1, (pw_val >> 8) & 0xFF)

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

                prog = channel_patches.get(ev['channel'], 0)
                vel = ev.get('velocity', 100)
                wave, ad, sr, pw, features = get_instrument_params(prog, ev['channel'], note_val, vel, self.chip)

                v = self._allocate_voice(note_val, ev['channel'], ev['time'], features)
                if v:
                    v.note = note_val
                    v.channel = ev['channel']
                    v.active = True
                    v.start_time = ev['time']
                    v.priority_score = self._get_priority(note_val, ev['channel'], ev['time'])
                    v.released = False
                    v.release_timer = 0.0
                    v.arpeggio_notes = notes
                    v.arp_index = 0
                    v.arp_counter = 0
                    v.pwm_counter = 0

                    base = get_voice_offset(v.index)
                    freq = freq_for_note(notes[0])
                    if freq > 0xFFFF: freq = 0xFFFF
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

                    # Filter Setup
                    has_filt = any(f in features for f in ["bass_filter", "velocity_filter", "filter_hp", "filter_bp"])
                    if has_filt:
                        is_melody = (ev['channel'] == self.melody_channel)
                        melody_active = any(vo.active and getattr(vo, 'channel', -1) == self.melody_channel for vo in self.voices)
                        
                        if is_melody or not melody_active:
                            filt_route = 1 << (v.index - 1)
                            
                            target_hz = 1000.0
                            if "velocity_filter" in features:
                                target_hz = 200.0 + (vel / 127.0) * 8000.0
                            elif "bass_filter" in features:
                                target_hz = 1500.0

                            dac_val = self._freq_to_cutoff_dac(target_hz)
                            self._emit_reg(bytecode, CUTOFF_LO, dac_val & 0x07)
                            self._emit_reg(bytecode, CUTOFF_HI, (dac_val >> 3) & 0xFF)
                            
                            res = 0x40 # default resonance
                            if self.chip == "6581" and dac_val > 1500:
                                res = 0x10 # Q-Drop
                            self._emit_reg(bytecode, RESON_FILT, res | filt_route)
                            
                            curr = self.sid_state[MODE_VOL]
                            if curr == -1: curr = 0x0F
                            fmode = FILT_LP
                            if "filter_hp" in features: fmode = FILT_HP
                            elif "filter_bp" in features: fmode = FILT_BP
                            
                            # if voice 3 is modulator, turn off its audio output
                            if self.voices[2].is_modulator:
                                fmode |= FILT_OFF3
                                
                            curr = curr & ~(FILT_LP | FILT_BP | FILT_HP | FILT_OFF3)
                            self._emit_reg(bytecode, MODE_VOL, curr | fmode)

                    # 3. Trigger
                    if "pcm_sample" in features and self.chip == "6581":
                        # Fast Volume DAC manipulation for PCM
                        for vol in [15, 12, 9, 6, 3, 0]:
                            curr = self.sid_state[MODE_VOL]
                            if curr == -1: curr = 0x00
                            self._emit_reg(bytecode, MODE_VOL, (curr & 0xF0) | vol)
                            bytecode.extend([0x82, 0x10]) # micro delay
                        self._emit_reg(bytecode, MODE_VOL, (curr & 0xF0) | 0x0F)
                    elif "kick_slide" in features:
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
                                v.release_timer = 0.0
                                base = get_voice_offset(v.index)
                                release_cmd = v.wave & (~WAVE_GATE)
                                self.sid_state[base + CTRL_1] = release_cmd
                                bytecode.extend([0x93 + (v.index - 1)])

                        elif v.note == ev['note']: # Fallback for single note
                            v.released = True
                            v.release_timer = 0.0
                            base = get_voice_offset(v.index)
                            release_cmd = v.wave & (~WAVE_GATE)
                            self.sid_state[base + CTRL_1] = release_cmd
                            bytecode.extend([0x93 + (v.index - 1)])

        bytecode.append(0xFF)
        return bytecode

    def _get_priority(self, note, channel, now):
        if channel == getattr(self, 'noise_channel', -1): return -1000
        score = 0
        if channel == self.melody_channel: score += 8000
        elif channel == self.bass_channel: score += 5000
        elif channel == 9: score += 3000

        if note > 80: score += 500
        if note < 40: score += 500

        return score + note

    def _allocate_voice(self, note, channel, time, features=[]):
        # 0. Fast path: Inactive voice that already has this channel's affinity
        best_inactive = None
        for v in self.voices:
            if not v.active:
                if getattr(v, 'channel', None) == channel:
                    best_inactive = v
                    break
                elif best_inactive is None:
                    best_inactive = v
                    
        chosen = best_inactive

        if not chosen:
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
                    
            chosen = best_victim if best_victim else self.voices[0]
            
        if "hard_sync" in features or "ring_mod" in features:
            mod_idx = (chosen.index - 2) % 3
            self.voices[mod_idx].is_modulator = True

        return chosen
