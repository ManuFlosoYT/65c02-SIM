import sys
import os
import math
import argparse

# Check for mido
try:
    import mido
except ImportError:
    print("Error: 'mido' library not found.")
    print("Please install it using: pip install mido")
    sys.exit(1)

# --- Configuration ---
SID_CHANNELS = 3
SID_CLOCK = 1000000.0  # 1 MHz

# --- SID Constants ---
FREQ_LO_1 = 0x00
FREQ_HI_1 = 0x01
PW_LO_1   = 0x02
PW_HI_1   = 0x03
CTRL_1    = 0x04
AD_1      = 0x05
SR_1      = 0x06

# Waveforms
WAVE_GATE  = 0x01
WAVE_SYNC  = 0x02
WAVE_RING  = 0x04
WAVE_TEST  = 0x08
WAVE_TRI   = 0x10
WAVE_SAW   = 0x20
WAVE_PULSE = 0x40
WAVE_NOISE = 0x80

# --- Additional SID Constants ---
ENV_1     = 0x1C 
CUTOFF_LO = 0x15
CUTOFF_HI = 0x16
RESON_FILT= 0x17
MODE_VOL  = 0x18

def get_voice_offset(voice_idx):
    return (voice_idx - 1) * 7

def freq_for_note(note_num):
    freq_hz = 440.0 * (2.0 ** ((note_num - 69.0) / 12.0))
    sid_freq = int(freq_hz * 16.777216)
    return sid_freq

# --- Instrument Configuration ---
# --- Configuration Enums ---
MODE_LEVEL_1 = "l1" # Raw / High Fidelity
MODE_LEVEL_2 = "l2" # Fine
MODE_LEVEL_3 = "l3" # Light
MODE_LEVEL_4 = "l4" # Standard
MODE_LEVEL_5 = "l5" # Medium (Chords)
MODE_LEVEL_6 = "l6" # High
MODE_LEVEL_7 = "l7" # Extreme
MODE_LEVEL_8 = "l8" # Insane

def _get_instrument_params(program, channel, note, velocity):
    # Default: Pulse with generic envelope
    wave = WAVE_PULSE
    attack_decay = 0x09  # Short Attack, Long Decay
    sustain_release = 0xF5 # High Sustain, Medium Release
    pulse_width = 0x0800 # 50%
    
    # Flags
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

class MidiProcessor:
    def __init__(self, midi_path, mode=MODE_LEVEL_1):
        self.midi_path = midi_path
        self.mode = mode
        self.mid = mido.MidiFile(midi_path)
        self.events = []
        self.voices = [Voice(i) for i in range(1, 4)]
        self.bass_channel = -1
        self.melody_channel = -1
        
        self.quant_grid = 0.001 
        self.min_note_duration = 0.0
        self.chord_reduce = False
        
        self._configure_mode()

    def _configure_mode(self):
        self.quant_grid = 0.002 
        self.min_note_duration = 0.01
        self.chord_reduce = False
        self.chord_window = 0.01 
        self.max_polyphony = 4 
        
        if self.mode == MODE_LEVEL_1: # Raw
            self.quant_grid = 0.002
            self.min_note_duration = 0.005
            self.chord_reduce = False
            self.max_polyphony = 4 
        elif self.mode == MODE_LEVEL_2: # Fine
            self.quant_grid = 0.005
            self.min_note_duration = 0.01
            self.chord_reduce = True 
            self.chord_window = 0.01
            self.max_polyphony = 3 
        elif self.mode == MODE_LEVEL_3: # Light
            self.quant_grid = 0.010
            self.min_note_duration = 0.02
            self.chord_reduce = True
            self.chord_window = 0.01
            self.max_polyphony = 3
        elif self.mode == MODE_LEVEL_4: # Standard
            self.quant_grid = 0.020 
            self.min_note_duration = 0.03
            self.chord_reduce = True
            self.chord_window = 0.02 
            self.max_polyphony = 3 
        elif self.mode == MODE_LEVEL_5: # Medium
            self.quant_grid = 0.030 
            self.min_note_duration = 0.04
            self.chord_reduce = True
            self.chord_window = 0.02
            self.max_polyphony = 2 
        elif self.mode == MODE_LEVEL_6: # High
            self.quant_grid = 0.040 
            self.min_note_duration = 0.05
            self.chord_reduce = True
            self.chord_window = 0.03
            self.max_polyphony = 2 
        elif self.mode == MODE_LEVEL_7: # Extreme
            self.quant_grid = 0.060 
            self.min_note_duration = 0.08
            self.chord_reduce = True
            self.chord_window = 0.05
            self.max_polyphony = 1 
        elif self.mode == MODE_LEVEL_8: # Insane
            self.quant_grid = 0.100 
            self.min_note_duration = 0.10
            self.chord_reduce = True
            self.chord_window = 0.08
            self.max_polyphony = 1
            
        print(f">> Mode: {self.mode.upper()}")
        print(f"   Grid: {self.quant_grid*1000:.1f}ms, MinDur: {self.min_note_duration*1000:.0f}ms, MaxPoly: {self.max_polyphony}")

    def process(self):
        print(f"Parsing '{self.midi_path}'...")
        self._flatten_events()
        self._analyze_channels()
        self._analyze_key()
        self._analyze_density()
        
        if self.mode != MODE_LEVEL_1:
            self._filter_short_notes()
            
        if self.chord_reduce:
            self._reduce_chords()
            
        if hasattr(self, 'density_map') and self.density_map:
             self._quantize_dynamic()
        else:
            for ev in self.events:
                ev['time'] = round(ev['time'] / self.quant_grid) * self.quant_grid
            
        self.events.sort(key=lambda x: (x['time'], 0 if x['type'] == 'note_off' else 1))
        
        if self.mode != MODE_LEVEL_1:
             self._deduplicate()

        return self._generate_bytecode()

    def _flatten_events(self):
        track_events = []
        for track in self.mid.tracks:
            abs_ticks = 0
            for msg in track:
                abs_ticks += msg.time
                if msg.type in ['note_on', 'note_off', 'set_tempo', 'program_change']:
                    track_events.append((abs_ticks, msg))
        
        track_events.sort(key=lambda x: x[0])
        
        current_tempo = 500000 
        seconds_per_tick = current_tempo / (1000000.0 * self.mid.ticks_per_beat)
        
        last_tick = 0
        current_time = 0.0
        
        for ticks, msg in track_events:
            delta_ticks = ticks - last_tick
            current_time += delta_ticks * seconds_per_tick
            last_tick = ticks
            
            if msg.type == 'set_tempo':
                current_tempo = msg.tempo
                seconds_per_tick = current_tempo / (1000000.0 * self.mid.ticks_per_beat)
            elif msg.type == 'program_change':
                self.events.append({
                    'time': current_time,
                    'type': 'program_change',
                    'program': msg.program,
                    'channel': getattr(msg, 'channel', 0)
                })
            elif msg.type == 'note_on' or msg.type == 'note_off':
                m_type = 'note_on'
                vel = 0
                if msg.type == 'note_on':
                    vel = msg.velocity
                    if vel == 0: m_type = 'note_off'
                else:
                    m_type = 'note_off'
                
                self.events.append({
                    'time': current_time,
                    'type': m_type,
                    'note': msg.note,
                    'velocity': vel,
                    'channel': getattr(msg, 'channel', 0)
                })

    def _analyze_channels(self):
        stats = {}
        for ev in self.events:
            if ev['type'] == 'note_on':
                ch = ev['channel']
                if ch not in stats: stats[ch] = {'count':0, 'total':0}
                stats[ch]['count'] += 1
                stats[ch]['total'] += ev['note']
        
        valid = [c for c, s in stats.items() if s['count'] > 10]
        if valid:
            self.bass_channel = min(valid, key=lambda c: stats[c]['total']/stats[c]['count'])
            self.melody_channel = max(valid, key=lambda c: stats[c]['total']/stats[c]['count'])
        else:
            self.bass_channel = -1
            self.melody_channel = -1
        
        print(f"   Detected Roles -> Bass: Ch{self.bass_channel}, Melody: Ch{self.melody_channel}")

    def _filter_short_notes(self):
        note_starts = {} 
        drop_indices = set()
        
        for i, ev in enumerate(self.events):
            if ev['type'] not in ['note_on', 'note_off']: continue
            
            key = (ev['channel'], ev['note'])
            if ev['type'] == 'note_on':
                note_starts[key] = i
            elif ev['type'] == 'note_off':
                if key in note_starts:
                    start_idx = note_starts[key]
                    duration = ev['time'] - self.events[start_idx]['time']
                    if duration < self.min_note_duration:
                        drop_indices.add(start_idx)
                        drop_indices.add(i)
                    del note_starts[key]
        
        self.events = [ev for i, ev in enumerate(self.events) if i not in drop_indices]
        print(f"   Filter Short Notes: Removed {len(drop_indices)} events.")

    def _analyze_key(self):
        pitch_counts = [0] * 12
        major_profile = [6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88]
        minor_profile = [6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17]

        for ev in self.events:
            if ev['type'] == 'note_on':
                pitch_class = ev['note'] % 12
                pitch_counts[pitch_class] += 1
                
        total_notes = sum(pitch_counts)
        if total_notes == 0: return

        norm_counts = [x / total_notes * 100 for x in pitch_counts]
        
        best_r = -1.0
        best_key = ""
        keys = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
        
        for i in range(12):
            r_maj = 0
            for j in range(12):
                r_maj += norm_counts[(i + j) % 12] * major_profile[j]
            
            if r_maj > best_r:
                best_r = r_maj
                best_key = f"{keys[i]} Major"
                
            r_min = 0
            for j in range(12):
                r_min += norm_counts[(i + j) % 12] * minor_profile[j]
                
            if r_min > best_r:
                best_r = r_min
                best_key = f"{keys[i]} Minor"
                
        self.detected_key = best_key
        print(f"   Detected Key: {best_key}")

    def _analyze_density(self):
        duration = self.events[-1]['time'] if self.events else 0
        if duration == 0: return
        
        block_size = 2.0 
        num_blocks = int(math.ceil(duration / block_size))
        
        self.density_map = [] 
        
        total_notes = sum(1 for e in self.events if e['type'] == 'note_on')
        global_density = total_notes / duration
        print(f"   Global Density: {global_density:.1f} notes/sec")
        
        for b in range(num_blocks):
            start_t = b * block_size
            end_t = (b + 1) * block_size
            
            block_events = [e for e in self.events if start_t <= e['time'] < end_t and e['type'] == 'note_on']
            count = len(block_events)
            
            rate = count / block_size
            local_grid = self.quant_grid 
            status = "Normal"
            
            if rate > global_density * 1.5:
                local_grid = max(0.01, self.quant_grid * 0.5)
                status = "Busy"
            elif rate < global_density * 0.5:
                local_grid = min(0.1, self.quant_grid * 2.0)
                status = "Sparse"
                
            self.density_map.append({
                'start': start_t,
                'end': end_t,
                'grid': local_grid,
                'status': status
            })
            
    def _quantize_dynamic(self):
        print("   Applying Dynamic Quantization...")
        for ev in self.events:
            t = ev['time']
            b_idx = int(t / 2.0) 
            if b_idx < len(self.density_map):
                grid = self.density_map[b_idx]['grid']
                ev['time'] = round(t / grid) * grid
            else:
                ev['time'] = round(t / self.quant_grid) * self.quant_grid

    def _reduce_chords(self):
        window = self.chord_window 
        new_events = []
        i = 0
        while i < len(self.events):
            ev = self.events[i]
            if ev['type'] == 'note_on':
                chord_candidates = [ev]
                others = [] 
                
                j = i + 1
                while j < len(self.events) and (self.events[j]['time'] - ev['time'] < window):
                    scan_ev = self.events[j]
                    if scan_ev['type'] == 'note_on':
                        chord_candidates.append(scan_ev)
                    else:
                        others.append(scan_ev)
                    j += 1
                
                if len(chord_candidates) > self.max_polyphony:
                    chord_candidates.sort(key=lambda x: x['note'])
                    arp_notes = [c['note'] for c in chord_candidates]
                    
                    arp_ev = {
                        'time': ev['time'],
                        'type': 'arpeggio_on',
                        'channel': ev['channel'],
                        'notes': arp_notes,
                        'velocity': max([c.get('velocity',0) for c in chord_candidates]),
                        'note': chord_candidates[0]['note'] 
                    }
                    
                    new_events.append(arp_ev)
                    new_events.extend(others) 
                    i = j - 1
                else:
                    new_events.append(ev) 
                    # Did not consume others, so we proceed normally. 
                    # Others will be picked up by main loop.
            else:
                new_events.append(ev)
            i += 1
            
        self.events = new_events
        print(f"   Chord Reduction: Reduced to {len(self.events)} events.")

    def _deduplicate(self):
        unique = []
        seen = set()
        for ev in self.events:
            if ev['type'] == 'program_change':
                unique.append(ev)
                continue
                
            n = ev.get('note', -1)
            if ev['type'] == 'arpeggio_on':
                n = tuple(ev['notes'])
                
            k = (ev['time'], ev['type'], n, ev.get('channel', 0))
            if k not in seen:
                seen.add(k)
                unique.append(ev)
        self.events = unique
        print(f"   Deduplication: Reduced to {len(self.events)} events.")

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

    def _generate_bytecode(self):
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
        # Time constants for effects (Optimized for size)
        # 0.02s (50Hz) was too heavy (~399KB). 
        # Reducing to ~16Hz (0.06s) for standard effects.
        FRAME_TIME = 0.02 
        
        # Rate at which we actually process/emit effects
        # L1-L3: High Quality (0.03s)
        # L4-L6: Mid Quality (0.06s)
        # L7-L8: Low Quality (0.10s)
        
        if self.mode in [MODE_LEVEL_1, MODE_LEVEL_2, MODE_LEVEL_3]:
            EFFECT_STEP = 0.03
        elif self.mode in [MODE_LEVEL_7, MODE_LEVEL_8]:
            EFFECT_STEP = 0.10
        else:
            EFFECT_STEP = 0.06

        
        # We need a unified time tracker for effects
        self.next_arp_time = 0.0
        self.next_pwm_time = 0.0
        
        def emit_delay_and_effects(dt, current_abs_time):
             # We want to traverse 'dt' in chunks of 'EFFECT_STEP'
             # BUT only if we actually HAVE active effects.
             
             # fast check: active effects?
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

             # If we have effects, we must slice the time
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
                         # Optimization: Only write HI byte? No, rough.
                         bytecode.extend([base + PW_LO_1, cur & 0xFF, base + PW_HI_1, (cur >> 8) & 0xFF])
                             
                 remaining -= step

        
        last_time = 0.0
        MAX_LOOPS = 0xFFFF 
        channel_patches = {}
        for ev in self.events:
            dt = ev['time'] - last_time
            if dt > 0.0:
                emit_delay_and_effects(dt, last_time)
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
                    
                    wave, ad, sr, pw, features = _get_instrument_params(prog, ev['channel'], note_val, vel)
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

def main():
    parser = argparse.ArgumentParser(description="MIDI to SID Converter")
    parser.add_argument("input", help="Input MIDI file")
    parser.add_argument("--mode", choices=[MODE_LEVEL_1, MODE_LEVEL_2, MODE_LEVEL_3, MODE_LEVEL_4, MODE_LEVEL_5, MODE_LEVEL_6, MODE_LEVEL_7, MODE_LEVEL_8], default=MODE_LEVEL_1, help="Optimization Level (l1-l8)")
    
    args = parser.parse_args()
    mode = args.mode
    
    processor = MidiProcessor(args.input, mode)
    bytecode = processor.process()
    
    # Output
    filename_only = os.path.splitext(os.path.basename(args.input))[0].replace(" ", "") + ".s"
    output_file = os.path.join("SID", filename_only)
    
    print(f"Writing ASM to {output_file}...")
    
    with open(output_file, "w") as f:
        f.write(f"; Generated by midi_to_sid.py (Mode: {mode})\n")
        f.write(".segment \"CODE\"\n")
        f.write("reset:\n    ldx #$FF\n    txs\n    cli\n")
        f.write("    lda #<music_data\n    sta $00\n    lda #>music_data\n    sta $01\n")
        f.write("    ldy #0\n")
        f.write("play_loop:\n    lda ($00), y\n    bmi special_cmd\n    tax\n    iny\n    bne :+\n    inc $01\n:   lda ($00), y\n    sta $4800, x\n    iny\n    bne play_loop\n    inc $01\n    jmp play_loop\n")
        f.write("special_cmd:\n    cmp #$FF\n    beq stop\n    cmp #$81\n    beq do_delay\n    iny\n    bne play_loop\n    inc $01\n    jmp play_loop\n")
        f.write("do_delay:\n    iny\n    bne :+\n    inc $01\n:   lda ($00), y\n    tax\n    iny\n    bne :+\n    inc $01\n:   lda ($00), y\n    sty $02\n    tay\n")
        f.write("delay_loop:\n    cpx #0\n    bne :+\n    cpy #0\n    beq delay_done\n    dey\n:   dex\n    nop\n    nop\n    jmp delay_loop\n")
        f.write("delay_done:\n    ldy $02\n    iny\n    bne :+\n    inc $01\n:   jmp play_loop\n")
        f.write("stop:\n    lda #0\n    sta $4804\n    sta $480B\n    sta $4812\nstop_loop: jmp stop_loop\n")
        f.write("music_data:\n")
        
        # Chunked Write to avoid massive string construction
        chunk_size = 16
        for i in range(0, len(bytecode), chunk_size):
            chunk = bytecode[i:i+chunk_size]
            hex_str = ", ".join([f"${b:02X}" for b in chunk])
            f.write(f"    .byte {hex_str}\n")
            
        f.write(f".segment \"VECTORS\"\n    .word reset\n    .word reset\n    .word reset\n")

    # Size Report
    size_bytes = len(bytecode)
    size_kb = size_bytes / 1024.0
    limit_32k = 32768
    percent = (size_bytes / limit_32k) * 100.0
    
    print(f"Generated {output_file}")
    print(f"Size: {size_bytes} bytes ({size_kb:.1f} KB)")
    print(f"Usage: {percent:.1f}% of 32KB Limit")
    if size_bytes > limit_32k:
        print(">> WARNING: FILE EXCEEDS 32KB LIMIT!")
    else:
        print(">> OK: Fits in 32KB.")
if __name__ == "__main__":
    main()
