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
        """Parse, filter, and quantize MIDI data. Returns the event list."""
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

        return self.events

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
