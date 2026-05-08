# MIDI file parsing and preprocessing

import math

try:
    import mido
except ImportError:
    import sys
    print("Error: 'mido' library not found.")
    print("Please install it using: pip install mido")
    sys.exit(1)

from sid_constants import (
    MODE_LEVEL_1, MODE_LEVEL_2, MODE_LEVEL_3, MODE_LEVEL_4,
    MODE_LEVEL_5, MODE_LEVEL_6, MODE_LEVEL_7, MODE_LEVEL_8,
    MODE_LEVEL_9, MODE_LEVEL_10, MODE_LEVEL_11, MODE_LEVEL_12,
    MODE_LEVEL_13, MODE_LEVEL_14, MODE_LEVEL_15
)


class MidiProcessor:
    def __init__(self, midi_path, mode=MODE_LEVEL_1):
        self.midi_path = midi_path
        self.mode = mode
        self.mid = mido.MidiFile(midi_path, clip=True)
        self.events = []
        self.bass_channel = -1
        self.melody_channel = -1

        self.quant_grid = 0.001
        self.min_note_duration = 0.0
        self.chord_reduce = False

        self._configure_mode()

    def _configure_mode(self):
        # Quantization Grid, Min Note Duration, Chord Reduction, Chord Window, Max Polyphony
        modes_config = {
            "l1": (0.001, 0.001, False, 0.0, 3),
            "l2": (0.004, 0.006, False, 0.0, 3),
            "l3": (0.006, 0.008, False, 0.0, 3),
            "l4": (0.008, 0.010, True, 0.010, 3),
            "l5": (0.010, 0.012, True, 0.015, 3),
            "l6": (0.015, 0.015, True, 0.020, 3),
            "l7": (0.020, 0.020, True, 0.030, 3),
            "l8": (0.025, 0.025, True, 0.040, 3),
            "l9": (0.030, 0.035, True, 0.050, 2),
            "l10": (0.040, 0.045, True, 0.060, 2),
            "l11": (0.050, 0.055, True, 0.070, 2),
            "l12": (0.060, 0.070, True, 0.080, 2),
            "l13": (0.075, 0.085, True, 0.090, 1),
            "l14": (0.090, 0.100, True, 0.100, 1),
            "l15": (0.100, 0.120, True, 0.120, 1),
        }
        
        cfg = modes_config.get(self.mode, modes_config["l1"])
        self.quant_grid = cfg[0]
        self.min_note_duration = cfg[1]
        self.chord_reduce = cfg[2]
        self.chord_window = cfg[3]
        self.max_polyphony = cfg[4]

        print(f">> Mode: {self.mode.upper()}")
        print(f"   Grid: {self.quant_grid*1000:.1f}ms, MinDur: {self.min_note_duration*1000:.0f}ms, MaxPoly: {self.max_polyphony}")

    def process(self):
        """Parse, filter, and quantize MIDI data. Returns the event list."""
        print(f"Parsing '{self.midi_path}'...")
        self._flatten_events()
        self._analyze_channels()
        self._analyze_key()
        self._analyze_density()

        self._filter_short_notes()

        self._simplify_chords()

        if self.chord_reduce:
            self._reduce_chords()

        self._cull_polyphony()

        if hasattr(self, 'density_map') and self.density_map:
            self._quantize_dynamic()
        else:
            for ev in self.events:
                ev['time'] = round(ev['time'] / self.quant_grid) * self.quant_grid

        self.events.sort(key=lambda x: (x['time'], 0 if x['type'] == 'note_off' else 1))

        self._deduplicate()

        return self.events

    def _simplify_chords(self):
        """
        Detecta notas simultáneas en el mismo canal (acordes) y las simplifica a
        una sola frecuencia dominante para evitar disonancias y ahorro de voces.
        """
        new_events = []
        i = 0
        window_size = 0.005 # 5ms window
        
        while i < len(self.events):
            ev = self.events[i]
            if ev['type'] == 'note_on':
                window_events = [ev]
                j = i + 1
                while j < len(self.events) and (self.events[j]['time'] - ev['time'] < window_size):
                    if self.events[j]['type'] == 'note_on':
                        window_events.append(self.events[j])
                    j += 1
                
                chan_groups = {}
                for wev in window_events:
                    ch = wev.get('channel', 0)
                    if ch not in chan_groups:
                        chan_groups[ch] = []
                    chan_groups[ch].append(wev)
                
                simplified_notes = []
                for ch, notes in chan_groups.items():
                    if len(notes) > 1:
                        notes.sort(key=lambda x: x.get('note', 60))
                        if ch == self.bass_channel and ch != self.melody_channel:
                            kept = notes[0] # Bass -> Lowest
                        else:
                            kept = notes[-1] # Melody/Others -> Highest
                        simplified_notes.append(kept)
                    else:
                        simplified_notes.append(notes[0])
                
                for k_ev in simplified_notes:
                    new_events.append(k_ev)
                
                for k in range(i + 1, j):
                    if self.events[k]['type'] != 'note_on':
                        new_events.append(self.events[k])
                
                i = j
            else:
                new_events.append(ev)
                i += 1
                
        print(f"   Chord Simplification: Collapsed {len(self.events) - len(new_events)} inner chord notes.")
        self.events = new_events

    def _cull_polyphony(self):
        def get_prio(ev):
            ch = ev.get('channel', 0)
            score = 0
            if ch == self.melody_channel: score += 8000
            elif ch == self.bass_channel and ch != self.melody_channel: score += 5000
            elif ch == 9: score += 3000
            note = ev.get('note', 60)
            if note > 80: score += 500
            if note < 40: score += 500
            return score + note

        new_events = []
        i = 0
        while i < len(self.events):
            ev = self.events[i]
            if ev['type'] == 'note_on':
                window_events = [ev]
                j = i + 1
                while j < len(self.events) and (self.events[j]['time'] - ev['time'] < 0.002):
                    if self.events[j]['type'] == 'note_on':
                        window_events.append(self.events[j])
                    j += 1
                
                if len(window_events) > self.max_polyphony:
                    window_events.sort(key=get_prio, reverse=True)
                    kept = window_events[:self.max_polyphony]
                    for k_ev in kept:
                        new_events.append(k_ev)
                else:
                    for k_ev in window_events:
                        new_events.append(k_ev)
                
                for k in range(i + 1, j):
                    if self.events[k]['type'] != 'note_on':
                        new_events.append(self.events[k])
                
                i = j
            else:
                new_events.append(ev)
                i += 1
                
        print(f"   Polyphony Culling: Dropped {len(self.events) - len(new_events)} overlapping Note Ons.")
        self.events = new_events

    def _flatten_events(self):
        track_events = []
        for track in self.mid.tracks:
            abs_ticks = 0
            for msg in track:
                abs_ticks += msg.time
                if msg.type in ['note_on', 'note_off', 'set_tempo', 'program_change', 'control_change', 'pitchwheel']:
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
            elif msg.type == 'control_change':
                if msg.control in [1, 7, 6, 38, 98, 99, 100, 101]: # Modulation, Volume, Data Entry, NRPN/RPN
                    self.events.append({
                        'time': current_time,
                        'type': 'control_change',
                        'control': msg.control,
                        'value': msg.value,
                        'channel': getattr(msg, 'channel', 0)
                    })
            elif msg.type == 'pitchwheel':
                self.events.append({
                    'time': current_time,
                    'type': 'pitchwheel',
                    'pitch': msg.pitch,
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
                local_grid = self.quant_grid * 0.5
                status = "Busy"
            elif rate < global_density * 0.5:
                local_grid = self.quant_grid * 2.0
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
                        'velocity': max([c.get('velocity', 0) for c in chord_candidates]),
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
            if ev['type'] in ['program_change', 'control_change', 'pitchwheel']:
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
