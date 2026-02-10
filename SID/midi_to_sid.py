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

def get_voice_offset(voice_idx):
    return (voice_idx - 1) * 7

def freq_for_note(note_num):
    freq_hz = 440.0 * (2.0 ** ((note_num - 69.0) / 12.0))
    sid_freq = int(freq_hz * 16.777216)
    return sid_freq

# --- Configuration Enums ---
MODE_LEVEL_1 = "l1" # Raw / High Fidelity
MODE_LEVEL_2 = "l2" # Fine
MODE_LEVEL_3 = "l3" # Light
MODE_LEVEL_4 = "l4" # Standard
MODE_LEVEL_5 = "l5" # Medium (Chords)
MODE_LEVEL_6 = "l6" # High
MODE_LEVEL_7 = "l7" # Extreme
MODE_LEVEL_8 = "l8" # Insane

class Voice:
    def __init__(self, index):
        self.index = index
        self.note = None
        self.channel = None
        self.start_time = 0
        self.active = False 
        self.priority_score = 0
        self.wave = WAVE_PULSE # Default

class MidiProcessor:
    def __init__(self, midi_path, mode=MODE_LEVEL_1):
        self.midi_path = midi_path
        self.mode = mode
        self.mid = mido.MidiFile(midi_path)
        self.events = []
        self.voices = [Voice(i) for i in range(1, 4)]
        self.bass_channel = -1
        self.melody_channel = -1
        
        # Mode Parameters
        self.quant_grid = 0.001 
        self.min_note_duration = 0.0
        self.chord_reduce = False
        
        self._configure_mode()

    def _configure_mode(self):
        # Default Baseline (L1)
        self.quant_grid = 0.002 
        self.min_note_duration = 0.01
        self.chord_reduce = False
        self.chord_window = 0.01 
        self.max_polyphony = 4 
        
        # Smoother Gradient Strategy (Size vs Quality):
        # L1: Raw (No dedup, Max Detail)
        # L2-L3: Dedup + Cap at 3 Voices (Hardware Limit) -> Saves space, same audio
        # L4: Cap Chords at 3 notes (Light reduction)
        # L5: Cap Chords at 2 notes (Medium reduction) 
        # L6: Cap Chords at 1 note (Melody focus)
        # L7-L8: Aggressive Grid + Mono

        if self.mode == MODE_LEVEL_1: # Raw
            self.quant_grid = 0.002
            self.min_note_duration = 0.005
            self.chord_reduce = False
            self.max_polyphony = 4 # Keep as is (allow arpeggio overlaps)
            
        elif self.mode == MODE_LEVEL_2: # Fine (Hardware Limit)
            self.quant_grid = 0.005
            self.min_note_duration = 0.01
            self.chord_reduce = True 
            self.chord_window = 0.01 # 10ms window to catch simultaneous chords
            self.max_polyphony = 3 # Hardware limit
            
        elif self.mode == MODE_LEVEL_3: # Light (Hardware Limit + Grid)
            self.quant_grid = 0.010
            self.min_note_duration = 0.02
            self.chord_reduce = True
            self.chord_window = 0.01
            self.max_polyphony = 3
            
        elif self.mode == MODE_LEVEL_4: # Standard (Poly 3 + Loose Window)
            self.quant_grid = 0.020 
            self.min_note_duration = 0.03
            self.chord_reduce = True
            self.chord_window = 0.02 # 20ms window
            self.max_polyphony = 3 
            
        elif self.mode == MODE_LEVEL_5: # Medium (Poly 2)
            self.quant_grid = 0.030 
            self.min_note_duration = 0.04
            self.chord_reduce = True
            self.chord_window = 0.02
            self.max_polyphony = 2 # Stronger trim
            
        elif self.mode == MODE_LEVEL_6: # High (Poly 1 - Melody/Bass)
            self.quant_grid = 0.040 
            self.min_note_duration = 0.05
            self.chord_reduce = True
            self.chord_window = 0.03
            self.max_polyphony = 2 
            
        elif self.mode == MODE_LEVEL_7: # Extreme (Poly 1 Strict)
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
        print(f"   Grid: {self.quant_grid*1000:.1f}ms, MinDur: {self.min_note_duration*1000:.0f}ms, Reduce: {self.chord_reduce} (Win: {self.chord_window*1000:.0f}ms, MaxPoly: {self.max_polyphony})")

    def process(self):
        print(f"Parsing '{self.midi_path}'...")
        self._flatten_events()
        self._analyze_channels()
        self._analyze_key()
        self._analyze_density()
        
        # Pre-calculation Phases
        if self.mode != MODE_LEVEL_1:
            self._filter_short_notes()
            
        if self.chord_reduce:
            self._reduce_chords()
            
        # Quantize Time (Dynamic Grid Support)
        # If we computed a density map, use it. Otherwise use static grid.
        if hasattr(self, 'density_map') and self.density_map:
             self._quantize_dynamic()
        else:
            for ev in self.events:
                ev['time'] = round(ev['time'] / self.quant_grid) * self.quant_grid
            
        # Re-sort after quantizing
        self.events.sort(key=lambda x: (x['time'], 0 if x['type'] == 'note_off' else 1))
        
        # Remove duplicates (Unison) after quantization
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
        # Scan for channel roles
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
        # Map note_on to index to calculate duration
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
        # Weighted Histogram of 12 semitones
        pitch_counts = [0] * 12
        major_profile = [6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88]
        minor_profile = [6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17]

        for ev in self.events:
            if ev['type'] == 'note_on':
                pitch_class = ev['note'] % 12
                pitch_counts[pitch_class] += 1
                
        total_notes = sum(pitch_counts)
        if total_notes == 0: return

        # Normalize
        norm_counts = [x / total_notes * 100 for x in pitch_counts]
        
        best_r = -1.0
        best_key = ""
        keys = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
        
        for i in range(12):
            # Test Major
            r_maj = 0
            for j in range(12):
                r_maj += norm_counts[(i + j) % 12] * major_profile[j]
            
            if r_maj > best_r:
                best_r = r_maj
                best_key = f"{keys[i]} Major"
                
            # Test Minor
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
                
                # Check if we need to reduce
                if len(chord_candidates) > self.max_polyphony:
                    # Sort by pitch
                    chord_candidates.sort(key=lambda x: x['note'])
                    
                    # Logic: 
                    # If max=1: Keep Top (Melody)
                    # If max=2: Keep Top & Bottom
                    # If max=3: Keep Top, Bottom, and one from middle
                    
                    kept = []
                    # 1. Always Keep Top (Melody)
                    kept.append(chord_candidates[-1]) 
                    
                    if self.max_polyphony >= 2:
                        # 2. Keep Bottom (Bass) if distinct
                        if chord_candidates[0] != chord_candidates[-1]:
                            kept.append(chord_candidates[0])
                            
                    if self.max_polyphony >= 3:
                        # 3. Keep a middle note if available
                        if len(chord_candidates) > 2:
                             # Pick closest to center index?
                            mid_idx = len(chord_candidates) // 2
                            if chord_candidates[mid_idx] not in kept:
                                kept.append(chord_candidates[mid_idx])
                                
                    # Fill up if we have slots (e.g. if Top==Bottom)
                    # (logic covers distinct checks, but if candidates=[C4, C4, C4] and max=2, we keep C4)
                    
                    # Add kept notes to new_events
                    for k in kept:
                        k['time'] = ev['time'] # Snap to leader
                        new_events.append(k)
                    
                    new_events.extend(others)
                    i = j - 1
                else:
                    # No reduction needed (count <= max)
                    # Just append all found events in order?
                    # Since we scanned ahead, we must handle them correctly.
                    # Best to just append them all and advance i.
                    # BUT we split them into 'chord_candidates' and 'others'.
                    # Re-merge sorted by time/type? They are already roughly sorted.
                    # But 'others' might be interleaved.
                    
                    # To allow 'no-op' passthrough without reordering issues:
                    # Since we only gathered them, we can just dump them.
                    # But they are split.
                    # Let's re-add them sorted by original index?
                    # Actually, we can just NOT touch loop 'i' if no reduction.
                    # Wait, if we DON'T touch 'i', next iteration processes i+1...
                    # which is inside the window we just scanned.
                    # Is that safe? Yes, because next iter will see a smaller window.
                    # BUT efficient? No. O(N^2).
                    # Better: Emit current 'ev' and continue i+1.
                    # BUT we pulled 'others' out. Logic must be consistent.
                    
                    new_events.append(ev)
                    # We did NOT consume others or lookaheads.
                    # So just continue main loop.
                    # i increment at end handles it.
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
                
            # key = time + type + note + channel
            k = (ev['time'], ev['type'], ev['note'], ev.get('channel', 0))
            if k not in seen:
                seen.add(k)
                unique.append(ev)
        self.events = unique
        print(f"   Deduplication: Reduced to {len(self.events)} events.")

    def _get_priority(self, note, channel, now):
        # Increased separation (Reference style)
        score = 0
        if channel == self.melody_channel: score += 8000
        elif channel == self.bass_channel: score += 5000
        elif channel == 9: score += 3000 # Drums still high but below Melody/Bass
        
        # Pitch bias still useful for tie-breaking
        if note > 80: score += 500
        if note < 40: score += 500
        
        return score

    def _allocate_voice(self, note, channel, time):
        # 1. Check for empty voice
        for v in self.voices:
            if not v.active:
                return v

        # 2. Steal
        new_priority = self._get_priority(note, channel, time)
        
        best_victim = None
        min_score = 999999
        
        for v in self.voices:
            age = time - v.start_time
            age_bonus = 0
            if age < 0.05: age_bonus = 10000 
            
            # Check if this voice is playing a high priority role?
            # Priority is calculated on allocation.
            # We should probably re-eval priority?
            # Start with stored priority.
            
            current_score = v.priority_score + age_bonus
            
            if current_score < min_score:
                min_score = current_score
                best_victim = v

        if best_victim:
            # Only steal if new note is reasonably important?
            # Reference doesn't check new vs old score, just picks victim.
            # But implicitly, if we steal, we are replacing.
            return best_victim
            
        return self.voices[0]

    def _map_program_to_wave(self, program):
        # GM Mapping
        # 0-7: Piano -> Pulse
        # 8-15: Chrom Perc -> Tri
        # 16-23: Organ -> Pulse
        # 24-31: Guitar -> Saw (Distortion) or Pulse
        # 32-39: Bass -> Saw
        # 40-47: Strings -> Saw/Tri
        # 48-55: Ensemble -> Saw
        # 56-63: Brass -> Saw
        # 64-71: Reed -> Square
        # 72-79: Pipe -> Tri
        # 80-95: Synth Lead -> Pulse
        # 96-103: Synth Pad -> Saw
        # 113-119: Perc -> Noise
        
        if 24 <= program <= 31: return WAVE_SAW # Guitar
        if 32 <= program <= 39: return WAVE_SAW # Bass
        if 40 <= program <= 47: return WAVE_TRI # Strings
        if 56 <= program <= 63: return WAVE_SAW # Brass
        if 80 <= program <= 95: return WAVE_PULSE # Lead
        if 113 <= program <= 119: return WAVE_NOISE # Tinklebell/Agogo etc
        
        return WAVE_PULSE # Default

    def _generate_bytecode(self):
        print(f"Generating Bytecode from {len(self.events)} events...")
        bytecode = []
        
        # Init
        bytecode.extend([0x18, 0x0F]) 
        for i in range(1, 4):
            base = get_voice_offset(i)
            bytecode.extend([base + AD_1, 0x09, base + SR_1, 0xF5])

        last_time = 0.0
        MAX_LOOPS = 65535 * 10
        
        # Track current program per channel
        channel_patches = {} 
        
        for ev in self.events:
            dt = ev['time'] - last_time
            if dt > 0.0:
                cycles = dt * SID_CLOCK
                loops = int(cycles / 15.0)
                if loops > MAX_LOOPS: loops = MAX_LOOPS 
                while loops > 0xFFFF:
                     bytecode.extend([0x81, 0xFF, 0xFF])
                     loops -= 0xFFFF
                if loops > 0:
                    lo = loops & 0xFF
                    hi = (loops >> 8) & 0xFF
                    bytecode.extend([0x81, lo, hi])
                last_time = ev['time']
            
            if ev['type'] == 'program_change':
                channel_patches[ev['channel']] = ev['program']
                
            elif ev['type'] == 'note_on':
                v = self._allocate_voice(ev['note'], ev['channel'], ev['time'])
                if v:
                    v.note = ev['note']
                    v.channel = ev['channel']
                    v.active = True
                    v.start_time = ev['time']
                    v.priority_score = self._get_priority(ev['note'], ev['channel'], ev['time'])
                    
                    base = get_voice_offset(v.index)
                    freq = freq_for_note(ev['note'])
                    if freq > 0xFFFF: freq = 0xFFFF 
                    
                    # Dynamic Waveform Selection
                    wave = WAVE_PULSE
                    prog = channel_patches.get(ev['channel'], 0)
                    
                    if ev.get('channel') == 9: 
                        wave = WAVE_NOISE
                    else:
                        wave = self._map_program_to_wave(prog)
                        # Fallback heuristic if program is 0 (Piano) but pitch is super low/high?
                        if prog == 0:
                            if ev['note'] < 48: wave = WAVE_SAW
                            elif ev['note'] > 85: wave = WAVE_TRI
                    
                    v.wave = wave 
                    
                    bytecode.extend([base + CTRL_1, wave]) 
                    bytecode.extend([base + FREQ_LO_1, freq & 0xFF])
                    bytecode.extend([base + FREQ_HI_1, (freq >> 8) & 0xFF])
                    bytecode.extend([base + CTRL_1, wave | WAVE_GATE]) 
            
            elif ev['type'] == 'note_off':
                for v in self.voices:
                    if v.active and v.note == ev['note'] and v.channel == ev.get('channel', 0):
                        v.active = False
                        base = get_voice_offset(v.index)
                        release_wave = v.wave & (~WAVE_GATE) 
                        if release_wave == 0: release_wave = WAVE_PULSE 
                        bytecode.extend([base + CTRL_1, release_wave])

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
