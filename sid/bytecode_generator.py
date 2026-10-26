# SID bytecode generation from processed MIDI events

from sid_constants import (
    CLOCK_NTSC, CLOCK_PAL, MODE_MAX_QUALITY, MODE_QUALITY, MODE_BALANCED,
    MODE_COMPRESSED, MODE_MAX_COMPRESSION,
    FREQ_LO_1, FREQ_HI_1, PW_LO_1, PW_HI_1, CTRL_1, AD_1, SR_1,
    WAVE_GATE, WAVE_PULSE, WAVE_TEST, WAVE_SYNC, WAVE_RING,
    CUTOFF_LO, CUTOFF_HI, RESON_FILT, MODE_VOL,
    FILT_LP, FILT_BP, FILT_HP, FILT_OFF3,
    get_voice_offset, freq_for_note,
)
from instruments import get_instrument_params
import math


SID_RELEASE_TIMES = [
    0.006,  # 0: 6 ms
    0.024,  # 1: 24 ms
    0.048,  # 2: 48 ms
    0.072,  # 3: 72 ms
    0.114,  # 4: 114 ms
    0.168,  # 5: 168 ms
    0.204,  # 6: 204 ms
    0.240,  # 7: 240 ms
    0.300,  # 8: 300 ms
    0.750,  # 9: 750 ms
    1.500,  # 10: 1.5 s
    2.400,  # 11: 2.4 s
    3.000,  # 12: 3.0 s
    9.000,  # 13: 9.0 s
    15.000, # 14: 15.0 s
    24.000, # 15: 24.0 s
]


class Voice:
    def __init__(self, index):
        self.index = index
        self.note = None
        self.channel = None
        self.role = 'harmony'
        self.start_time = 0.0
        self.active = False
        self.released = False
        self.release_timer = 0.0
        self.max_release_time = 0.05
        self.priority_score = 0
        self.wave = WAVE_PULSE
        self.arpeggio_notes = []
        self.arp_index = 0
        self.arp_counter = 0
        self.pwm_counter = 0
        self.pwm_val = 0x800
        self.pwm_dir = 1
        self.features = []
        self.is_modulator = False
        self.filter_enabled = False
        self.filter_mode = FILT_LP
        self.filter_res = 0x40
        self.cutoff_base_hz = 1000.0
        self.filter_env_type = None
        self.filter_env_timer = 0.0
        self.filter_env_dur = 0.15
        self.filter_sweep_amount = 0.0

    def reset_filter(self):
        self.filter_enabled = False
        self.filter_mode = FILT_LP
        self.filter_res = 0x40
        self.cutoff_base_hz = 1000.0
        self.filter_env_type = None
        self.filter_env_timer = 0.0
        self.filter_env_dur = 0.15
        self.filter_sweep_amount = 0.0


class BytecodeGenerator:
    def __init__(self, events, mode, chip="8580", system="NTSC", bass_channel=-1, melody_channel=-1, noise_channel=-1, clock=None, cpu_clock=None, sid_clock=None, custom_wave_overrides=None):
        self.events = events
        self.mode = mode
        self.chip = chip
        self.system = system
        self.bass_channel = bass_channel
        self.melody_channel = melody_channel
        self.noise_channel = noise_channel
        self.custom_wave_overrides = custom_wave_overrides or {}
        self.voices = [Voice(i) for i in range(1, 4)]
        self.sid_state = [-1] * 25
        if sid_clock is not None:
            self.sid_clock = float(sid_clock)
        else:
            self.sid_clock = CLOCK_PAL if system == "PAL" else CLOCK_NTSC

        if cpu_clock is not None:
            self.cpu_clock = float(cpu_clock)
        elif clock is not None:
            self.cpu_clock = float(clock)
        else:
            self.cpu_clock = 1000000.0

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
            self.voices[i-1].reset_filter()

        # Max Quality/Quality: High (0.04s), Compressed/Max: Low (0.10s), else Mid (0.08s)
        if self.mode in [MODE_MAX_QUALITY, MODE_QUALITY]:
            EFFECT_STEP = 0.04
        elif self.mode in [MODE_COMPRESSED, MODE_MAX_COMPRESSION]:
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
        channel_cc74 = {i: 64 for i in range(16)}
        channel_cc71 = {i: 64 for i in range(16)}

        def update_filter_routing():
            filt_route = sum(1 << (v.index - 1) for v in self.voices if v.active and v.filter_enabled)
            curr_vol_mode = self.sid_state[MODE_VOL]
            if curr_vol_mode == -1:
                curr_vol_mode = 0x0F
            vol = curr_vol_mode & 0x0F

            if filt_route == 0:
                self._emit_reg(bytecode, RESON_FILT, 0x00)
                off3 = FILT_OFF3 if self.voices[2].is_modulator else 0
                self._emit_reg(bytecode, MODE_VOL, off3 | vol)
                return

            filt_mode = 0
            res_nibbles = []
            for v in self.voices:
                if v.active and v.filter_enabled:
                    filt_mode |= v.filter_mode
                    ch = getattr(v, 'channel', 0)
                    cc71 = channel_cc71.get(ch, 64)
                    base_nibble = (v.filter_res >> 4) & 0x0F
                    scaled = int(round(base_nibble * (cc71 / 64.0)))
                    res_nibbles.append(max(0, min(15, scaled)))

            if not filt_mode:
                filt_mode = FILT_LP

            if self.voices[2].is_modulator or (self.voices[2].active and (
                "filter_off3" in getattr(self.voices[2], 'features', []) or
                self.voices[2].role == 'percussion' or
                getattr(self.voices[2], 'channel', -1) == self.noise_channel
            )):
                filt_mode |= FILT_OFF3

            composite_res = (max(res_nibbles) << 4) if res_nibbles else 0x40
            self._emit_reg(bytecode, RESON_FILT, (composite_res & 0xF0) | filt_route)
            self._emit_reg(bytecode, MODE_VOL, (filt_mode & 0xF0) | vol)

        current_time = 0.0
        emitted_total_cycles = 0.0

        def advance_time(target_time):
            nonlocal current_time, emitted_total_cycles
            if target_time <= current_time:
                return

            dt = target_time - current_time
            target_cycles = round(target_time * self.cpu_clock)

            has_effects = False
            for v in self.voices:
                if v.active:
                    ch = getattr(v, 'channel', 0)
                    if (len(getattr(v, 'arpeggio_notes', [])) > 1 or "pwm_sweep" in getattr(v, 'features', []) or
                        "vibrato" in getattr(v, 'features', []) or channel_pitchbend.get(ch, 0) != 0 or
                        channel_mod.get(ch, 0) > 0 or getattr(v, 'released', False) or
                        (v.filter_enabled and (v.filter_env_type is not None or "filter_lfo" in getattr(v, 'features', [])))):
                        has_effects = True
                        break

            if not has_effects:
                needed_cycles = target_cycles - emitted_total_cycles
                if needed_cycles > 0:
                    loops = int(needed_cycles / 15.0)
                    emitted_total_cycles += loops * 15.0
                    while loops > 0:
                        cur = min(loops, 0xFFFF)
                        lo = cur & 0xFF
                        hi = (cur >> 8) & 0xFF
                        if cur <= 0xFF:
                            bytecode.extend([0x82, lo])
                        else:
                            bytecode.extend([0x81, lo, hi])
                        loops -= cur
                current_time = target_time
                return

            remaining_dt = dt
            step_dt = EFFECT_STEP
            accumulated_step_cycles = 0.0

            while remaining_dt > 0:
                step = min(remaining_dt, step_dt)
                remaining_dt -= step
                step_cycles = step * self.cpu_clock
                accumulated_step_cycles += step_cycles

                prev_len = len(bytecode)

                for v in self.voices:
                    if not v.active:
                        continue
                    if getattr(v, 'released', False):
                        v.release_timer += step
                        if v.release_timer >= getattr(v, 'max_release_time', 0.1):
                            v.active = False
                            v.released = False
                            v.reset_filter()
                            update_filter_routing()
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
                                mod_val = 0.3

                    vib_semitones = 0.0
                    if mod_val > 0:
                        v.arp_counter += 1
                        lfo_phase = v.arp_counter * 0.5
                        vib_semitones = math.sin(lfo_phase) * 0.5 * mod_val
                        vib_semitones = round(vib_semitones * 16.0) / 16.0

                    if semitones_shift != 0.0 or vib_semitones != 0.0 or is_arp:
                        freq = freq_for_note(note + semitones_shift + vib_semitones, self.sid_clock)
                        if freq < 0: freq = 0
                        if freq > 0xFFFF: freq = 0xFFFF
                        self._emit_reg(bytecode, base + FREQ_LO_1, int(freq) & 0xFF)
                        self._emit_reg(bytecode, base + FREQ_HI_1, (int(freq) >> 8) & 0xFF)

                    # 2. PWM Sweep
                    if "pwm_sweep" in getattr(v, 'features', []) and not getattr(v, 'released', False):
                        cur = getattr(v, 'pwm_val', 0x800)
                        d = getattr(v, 'pwm_dir', 1)
                        cur += (128 * d)
                        if cur > 0x0E00:
                            cur = 0x0E00
                            v.pwm_dir = -1
                        elif cur < 0x0200:
                            cur = 0x0200
                            v.pwm_dir = 1
                        v.pwm_val = cur
                        self._emit_reg(bytecode, base + PW_LO_1, cur & 0xFF)
                        self._emit_reg(bytecode, base + PW_HI_1, (cur >> 8) & 0xFF)

                    # 3. Filter Envelope Timer & LFO Advance
                    if v.filter_enabled:
                        v.filter_env_timer += step
                        if mod_val == 0 and (v.filter_env_type == 'lfo' or "filter_lfo" in getattr(v, 'features', [])):
                            v.arp_counter += 1

                # 4. Filter Cutoff Envelope & Sweeps
                filtered_voices = [v for v in self.voices if v.active and v.filter_enabled]
                if filtered_voices:
                    active_envs = [v for v in filtered_voices if v.filter_env_type is not None or "filter_lfo" in getattr(v, 'features', [])]
                    lead_v = max(active_envs if active_envs else filtered_voices, key=lambda v: v.priority_score)
                    hz = lead_v.cutoff_base_hz
                    if lead_v.filter_env_type == 'pluck':
                        decay = (1.0 - min(1.0, lead_v.filter_env_timer / lead_v.filter_env_dur)) ** 2
                        hz = lead_v.cutoff_base_hz + lead_v.filter_sweep_amount * decay
                        if lead_v.filter_env_timer >= lead_v.filter_env_dur:
                            lead_v.filter_env_type = None
                    elif lead_v.filter_env_type == 'swell':
                        t = min(1.0, lead_v.filter_env_timer / lead_v.filter_env_dur)
                        hz = lead_v.cutoff_base_hz + lead_v.filter_sweep_amount * t
                        if lead_v.filter_env_timer >= lead_v.filter_env_dur:
                            lead_v.filter_env_type = None

                    if lead_v.filter_env_type == 'lfo' or "filter_lfo" in getattr(lead_v, 'features', []):
                        hz *= (1.0 + 0.25 * math.sin(lead_v.arp_counter * 0.4))
                    elif channel_mod.get(getattr(lead_v, 'channel', 0), 0) > 0:
                        mod_depth = (channel_mod.get(getattr(lead_v, 'channel', 0), 0) / 127.0) * 0.25
                        hz *= (1.0 + mod_depth * math.sin(lead_v.arp_counter * 0.4))

                    ch = getattr(lead_v, 'channel', 0)
                    cc74 = channel_cc74.get(ch, 64)
                    hz *= (cc74 / 64.0)
                    target_hz = max(30.0, min(12000.0, hz))

                    dac_val = self._freq_to_cutoff_dac(target_hz)
                    self._emit_reg(bytecode, CUTOFF_LO, dac_val & 0x07)
                    self._emit_reg(bytecode, CUTOFF_HI, (dac_val >> 3) & 0xFF)

                if len(bytecode) > prev_len:
                    new_writes = bytecode[prev_len:]
                    del bytecode[prev_len:]

                    loops = int(accumulated_step_cycles / 15.0)
                    emitted_total_cycles += loops * 15.0
                    accumulated_step_cycles -= loops * 15.0
                    while loops > 0:
                        c = min(loops, 0xFFFF)
                        lo = c & 0xFF
                        hi = (c >> 8) & 0xFF
                        if c <= 0xFF:
                            bytecode.extend([0x82, lo])
                        else:
                            bytecode.extend([0x81, lo, hi])
                        loops -= c
                    bytecode.extend(new_writes)

            rem_cycles = target_cycles - emitted_total_cycles
            if rem_cycles > 0:
                loops = int(rem_cycles / 15.0)
                emitted_total_cycles += loops * 15.0
                while loops > 0:
                    c = min(loops, 0xFFFF)
                    lo = c & 0xFF
                    hi = (c >> 8) & 0xFF
                    if c <= 0xFF:
                        bytecode.extend([0x82, lo])
                    else:
                        bytecode.extend([0x81, lo, hi])
                    loops -= c

            current_time = target_time

        for ev in self.events:
            advance_time(ev['time'])

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
                elif ctrl == 71: # Resonance
                    channel_cc71[ch] = val
                    update_filter_routing()
                elif ctrl == 74: # Cutoff / Brightness
                    channel_cc74[ch] = val
                    for v in self.voices:
                        if v.active and v.filter_enabled and getattr(v, 'channel', -1) == ch:
                            hz = v.cutoff_base_hz * (val / 64.0)
                            target_hz = max(30.0, min(12000.0, hz))
                            dac_val = self._freq_to_cutoff_dac(target_hz)
                            self._emit_reg(bytecode, CUTOFF_LO, dac_val & 0x07)
                            self._emit_reg(bytecode, CUTOFF_HI, (dac_val >> 3) & 0xFF)
                            break
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
                role = ev.get('role')
                wave, ad, sr, pw, features = get_instrument_params(prog, ev['channel'], note_val, vel, self.chip)
                if ev['channel'] in self.custom_wave_overrides and self.custom_wave_overrides[ev['channel']] != 0:
                    wave = self.custom_wave_overrides[ev['channel']]
                    if self.chip == "6581" and (wave & 0x80) and (wave != 0x80):
                        wave = 0x80

                v = self._allocate_voice(note_val, ev['channel'], ev['time'], features, role)
                if v:
                    v.note = note_val
                    v.channel = ev['channel']
                    v.role = role if role else ('melody' if ev['channel'] == self.melody_channel else ('bass' if ev['channel'] == self.bass_channel else ('percussion' if ev['channel'] == 9 else 'harmony')))
                    v.active = True
                    v.start_time = ev['time']
                    v.priority_score = self._get_priority(note_val, ev['channel'], ev['time'], v.role)
                    v.released = False
                    v.release_timer = 0.0
                    v.max_release_time = SID_RELEASE_TIMES[sr & 0x0F]
                    v.arpeggio_notes = notes
                    v.arp_index = 0
                    v.arp_counter = 0
                    v.pwm_counter = 0

                    base = get_voice_offset(v.index)
                    freq = freq_for_note(notes[0], self.sid_clock)
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
                    has_filt = any(f in features for f in ["bass_filter", "velocity_filter", "filter_pluck", "filter_brass", "filter_lfo", "lead_filter", "pad_filter", "filter_hp", "filter_bp"])
                    if has_filt:
                        v.filter_enabled = True
                        if "filter_hp" in features:
                            v.filter_mode = FILT_HP
                        elif "filter_bp" in features:
                            v.filter_mode = FILT_BP
                        else:
                            v.filter_mode = FILT_LP

                        if "bass_filter" in features or "lead_filter" in features:
                            v.filter_res = 0x80
                        elif "filter_bp" in features:
                            v.filter_res = 0x60
                        else:
                            v.filter_res = 0x40

                        if "filter_pluck" in features:
                            v.cutoff_base_hz = 800.0
                            v.filter_sweep_amount = 2500.0 + (vel / 127.0) * 1000.0
                            v.filter_env_dur = 0.15
                            v.filter_env_type = 'pluck'
                        elif "filter_brass" in features:
                            v.cutoff_base_hz = 800.0
                            v.filter_sweep_amount = 2000.0
                            v.filter_env_dur = 0.25
                            v.filter_env_type = 'swell'
                        elif "filter_bp" in features:
                            v.cutoff_base_hz = 1200.0
                            v.filter_sweep_amount = 2000.0
                            v.filter_env_dur = 0.10
                            v.filter_env_type = 'pluck'
                        elif "filter_hp" in features:
                            v.cutoff_base_hz = 5000.0
                            v.filter_sweep_amount = 0.0
                            v.filter_env_dur = 0.10
                            v.filter_env_type = None
                        elif "bass_filter" in features:
                            v.cutoff_base_hz = 800.0
                            v.filter_sweep_amount = 2500.0 + (vel / 127.0) * 1000.0
                            v.filter_env_dur = 0.15
                            v.filter_env_type = 'pluck'
                        elif "lead_filter" in features or "pad_filter" in features:
                            v.cutoff_base_hz = 2000.0 if "lead_filter" in features else 1500.0
                            if "velocity_filter" in features:
                                v.cutoff_base_hz += (vel / 127.0) * 1500.0
                            v.filter_sweep_amount = 0.0
                            v.filter_env_dur = 0.15
                            v.filter_env_type = 'lfo' if "filter_lfo" in features else None
                        elif "velocity_filter" in features:
                            v.cutoff_base_hz = 300.0 + (vel / 127.0) * 6000.0
                            v.filter_sweep_amount = 0.0
                            v.filter_env_dur = 0.15
                            v.filter_env_type = 'lfo' if "filter_lfo" in features else None
                        else:
                            v.cutoff_base_hz = 1000.0
                            v.filter_sweep_amount = 0.0
                            v.filter_env_dur = 0.15
                            v.filter_env_type = 'lfo' if "filter_lfo" in features else None

                        v.filter_env_timer = 0.0

                        initial_hz = v.cutoff_base_hz
                        if v.filter_env_type == 'pluck':
                            initial_hz += v.filter_sweep_amount
                        cc74 = channel_cc74.get(ev['channel'], 64)
                        initial_hz *= (cc74 / 64.0)
                        initial_hz = max(30.0, min(12000.0, initial_hz))
                        dac_val = self._freq_to_cutoff_dac(initial_hz)
                        self._emit_reg(bytecode, CUTOFF_LO, dac_val & 0x07)
                        self._emit_reg(bytecode, CUTOFF_HI, (dac_val >> 3) & 0xFF)

                        update_filter_routing()
                    else:
                        v.reset_filter()
                        update_filter_routing()

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
                        start_freq = freq_for_note(note_val + 12, self.sid_clock)
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
                target_note = ev['note']
                target_channel = ev.get('channel', 0)
                for v in self.voices:
                    if v.active and getattr(v, 'channel', -1) == target_channel and not getattr(v, 'released', False):
                        if target_note in getattr(v, 'arpeggio_notes', []) or v.note == target_note:
                            if target_note in getattr(v, 'arpeggio_notes', []):
                                v.arpeggio_notes.remove(target_note)
                                if not v.arpeggio_notes:
                                    v.released = True
                                    v.release_timer = 0.0
                                    base = get_voice_offset(v.index)
                                    release_cmd = v.wave & (~WAVE_GATE)
                                    self.sid_state[base + CTRL_1] = release_cmd
                                    bytecode.extend([0x93 + (v.index - 1)])
                            else:
                                v.released = True
                                v.release_timer = 0.0
                                base = get_voice_offset(v.index)
                                release_cmd = v.wave & (~WAVE_GATE)
                                self.sid_state[base + CTRL_1] = release_cmd
                                bytecode.extend([0x93 + (v.index - 1)])
                            break

        bytecode.append(0xFF)
        return bytecode

    def _get_priority(self, note, channel, now=0, role=None):
        if channel == getattr(self, 'noise_channel', -1):
            return -1000
        score = 0
        if role == 'melody' or channel == self.melody_channel:
            score = 8000
        elif role == 'bass' or (channel == self.bass_channel and channel != self.melody_channel):
            score = 5000
        elif role == 'percussion' or channel == 9:
            score = 3000
        else:
            score = 1000

        if note > 80:
            score += 100
        elif note < 40:
            score += 100

        return score + note

    def _allocate_voice(self, note, channel, time, features=[], role=None):
        is_perc = (channel == 9 or role == 'percussion' or channel == getattr(self, 'noise_channel', -1))
        if is_perc:
            active_perc = sum(1 for v in self.voices if v.active and (v.channel == 9 or v.role == 'percussion' or v.channel == getattr(self, 'noise_channel', -1)))
            if active_perc >= 1:
                mel_or_bass_active = any(v.active and v.role in ['melody', 'bass'] for v in self.voices)
                if mel_or_bass_active:
                    return None

        best_inactive = None
        for v in self.voices:
            if not v.active:
                if getattr(v, 'channel', None) == channel:
                    best_inactive = v
                    break
                elif best_inactive is None:
                    best_inactive = v

        if best_inactive:
            chosen = best_inactive
        else:
            incoming_prio = self._get_priority(note, channel, time, role)
            candidates = []

            for v in self.voices:
                if getattr(v, 'released', False):
                    if v.role in ['melody', 'bass'] and v.release_timer < 0.120:
                        if incoming_prio < 5000:
                            continue
                    eff_score = v.priority_score - 30000
                else:
                    eff_score = v.priority_score
                    if (time - v.start_time) < 0.05:
                        eff_score += 5000
                    if getattr(v, 'channel', None) == channel:
                        eff_score -= 2000

                if getattr(v, 'released', False) or incoming_prio > v.priority_score:
                    candidates.append((eff_score, v))

            if not candidates:
                return None

            candidates.sort(key=lambda x: x[0])
            chosen = candidates[0][1]

        if "hard_sync" in features or "ring_mod" in features:
            mod_idx = (chosen.index - 2) % 3
            self.voices[mod_idx].is_modulator = True

        return chosen

