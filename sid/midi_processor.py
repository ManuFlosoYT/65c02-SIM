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
    MODE_MAX_QUALITY, MODE_QUALITY, MODE_BALANCED,
    MODE_COMPRESSED, MODE_MAX_COMPRESSION
)
from instruments import get_percussion_config


class MidiProcessor:
    def __init__(self, midi_path, mode=MODE_MAX_QUALITY, main_track=None, melody_channel=None, bass_channel=None):
        self.midi_path = midi_path
        self.mode = mode
        self.forced_main_track = main_track
        self.forced_melody_channel = melody_channel
        self.forced_bass_channel = bass_channel
        self.detected_meta_melody_tracks = set()
        self.detected_meta_bass_tracks = set()
        self.custom_wave_overrides = {}
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
            MODE_MAX_QUALITY: (0.001, 0.001, False, 0.0, 3),      # old l1
            MODE_QUALITY: (0.006, 0.008, False, 0.0, 3),          # old l3
            MODE_BALANCED: (0.010, 0.012, True, 0.015, 3),        # old l5
            MODE_COMPRESSED: (0.020, 0.020, True, 0.030, 3),      # old l7
            MODE_MAX_COMPRESSION: (0.030, 0.035, True, 0.050, 2), # old l9
        }
        
        cfg = modes_config.get(self.mode, modes_config[MODE_MAX_QUALITY])
        self.quant_grid = cfg[0]
        self.min_note_duration = cfg[1]
        self.chord_reduce = cfg[2]
        self.chord_window = cfg[3]
        self.max_polyphony = cfg[4]

        modes_simplify_window = {
            MODE_MAX_QUALITY: 0.005,
            MODE_QUALITY: 0.008,
            MODE_BALANCED: 0.012,
            MODE_COMPRESSED: 0.015,
            MODE_MAX_COMPRESSION: 0.020,
        }
        self.chord_window_simplify = modes_simplify_window.get(self.mode, 0.005)

        print(f">> Mode: {self.mode.upper()}")
        print(f"   Grid: {self.quant_grid*1000:.1f}ms, MinDur: {self.min_note_duration*1000:.0f}ms, MaxPoly: {self.max_polyphony}")

    def process(self):
        """Parse, filter, and quantize MIDI data. Returns the event list."""
        print(f"Parsing '{self.midi_path}'...")
        self._flatten_events()
        self._pair_notes()
        self._analyze_channels()
        self._analyze_roles_segmented()
        self._analyze_key()
        self._analyze_density()

        self._process_percussion()
        self._filter_short_notes()
        self._simplify_chords()

        if self.chord_reduce:
            self._reduce_chords()

        self._deduplicate_interchannel()
        self._cull_polyphony()

        if hasattr(self, 'density_map') and self.density_map:
            self._quantize_dynamic()
        else:
            for ev in self.events:
                ev['time'] = round(ev['time'] / self.quant_grid) * self.quant_grid

        self.events.sort(key=lambda x: (x['time'], 0 if x['type'] == 'note_off' else 1))

        self._deduplicate()

        return self.events

    def _pair_notes(self):
        active_notes = {}
        note_id_counter = 0
        for ev in self.events:
            if ev['type'] == 'note_on':
                note_id_counter += 1
                ev['id'] = note_id_counter
                active_notes.setdefault((ev['channel'], ev['note']), []).append(ev)
            elif ev['type'] == 'note_off':
                key = (ev['channel'], ev['note'])
                if active_notes.get(key):
                    matching_on = active_notes[key].pop(0)
                    ev['id'] = matching_on['id']
                    matching_on['duration'] = max(0.0, ev['time'] - matching_on['time'])
                else:
                    note_id_counter += 1
                    ev['id'] = note_id_counter

    def _analyze_roles_segmented(self):
        duration = self.events[-1]['time'] if self.events else 0.0
        if duration <= 0:
            return

        window_size = 8.0
        num_windows = max(1, int(math.ceil(duration / window_size)))

        channel_programs = {}
        for ev in self.events:
            if ev['type'] == 'program_change':
                channel_programs[ev['channel']] = ev['program']

        prev_melody = self.melody_channel
        prev_bass = self.bass_channel
        self.window_roles = []

        for w in range(num_windows):
            t_start = w * window_size
            t_end = (w + 1) * window_size

            chan_events = {}
            for ev in self.events:
                if ev['type'] == 'note_on' and t_start <= ev['time'] < t_end:
                    ch = ev['channel']
                    if ch == 9:
                        continue
                    chan_events.setdefault(ch, []).append(ev)

            roles = {9: 'percussion'}

            if chan_events:
                scores = {}
                for ch, notes in chan_events.items():
                    count = len(notes)
                    avg_pitch = sum(n['note'] for n in notes) / count

                    overlaps = 0
                    for k in range(len(notes) - 1):
                        if notes[k].get('duration', 0.2) + notes[k]['time'] > notes[k + 1]['time']:
                            overlaps += 1
                    overlap_ratio = overlaps / count if count > 0 else 0.0

                    prog = channel_programs.get(ch, 0)
                    prog_bass_bonus = 30.0 if 32 <= prog <= 39 else 0.0
                    prog_mel_bonus = 25.0 if (80 <= prog <= 87 or 72 <= prog <= 79 or 40 <= prog <= 43) else 0.0

                    mel_score = (avg_pitch - 55.0) * 1.5 + count * 1.2 + (1.0 - overlap_ratio) * 25.0 + prog_mel_bonus
                    bass_score = (65.0 - avg_pitch) * 2.0 + count * 1.0 + (1.0 - overlap_ratio) * 20.0 + prog_bass_bonus

                    if ch == prev_melody:
                        mel_score *= 1.25
                    if ch == prev_bass:
                        bass_score *= 1.25

                    scores[ch] = {
                        'mel': mel_score,
                        'bass': bass_score,
                        'avg_pitch': avg_pitch
                    }

                if self.forced_melody_channel is not None and self.forced_melody_channel in scores:
                    best_mel = self.forced_melody_channel
                else:
                    best_mel = max(scores.keys(), key=lambda c: scores[c]['mel'])
                roles[best_mel] = 'melody'
                prev_melody = best_mel

                remaining = [c for c in scores.keys() if c != best_mel]
                if remaining:
                    if self.forced_bass_channel is not None and self.forced_bass_channel in remaining:
                        best_bass = self.forced_bass_channel
                        roles[best_bass] = 'bass'
                        prev_bass = best_bass
                    else:
                        best_bass = max(remaining, key=lambda c: scores[c]['bass'])
                        if scores[best_bass]['avg_pitch'] < 65:
                            roles[best_bass] = 'bass'
                            prev_bass = best_bass
                        else:
                            roles[best_bass] = 'harmony'
                    for c in remaining:
                        if c != best_bass:
                            roles[c] = 'harmony'

            self.window_roles.append({
                'start': t_start,
                'end': t_end,
                'roles': roles
            })

        for ev in self.events:
            ch = ev.get('channel', 0)
            if ch == 9:
                ev['role'] = 'percussion'
                continue
            if ev.get('forced_role'):
                ev['role'] = ev['forced_role']
                continue
            t = ev['time']
            w_idx = min(len(self.window_roles) - 1, max(0, int(t / window_size)))
            ev['role'] = self.window_roles[w_idx]['roles'].get(ch, 'harmony')

    def _process_percussion(self):
        PERCUSSION_RANK = {
            'kick': 60,
            'snare': 50,
            'crash': 40,
            'tom': 30,
            'open_hat': 20,
            'closed_hat': 10,
            'default': 0
        }

        on_events = {}
        for ev in self.events:
            if ev['channel'] == 9 and ev['type'] == 'note_on':
                on_events[ev['id']] = ev

        for ev in self.events:
            if ev['channel'] == 9 and ev['type'] == 'note_off':
                on_ev = on_events.get(ev.get('id'))
                if on_ev:
                    cfg = get_percussion_config(on_ev['note'])
                    max_len = cfg.get('max_len', 0.15)
                    if ev['time'] - on_ev['time'] > max_len:
                        ev['time'] = on_ev['time'] + max_len

        dropped_ids = set()
        i = 0
        perc_notes = [ev for ev in self.events if ev['channel'] == 9 and ev['type'] == 'note_on']

        while i < len(perc_notes):
            base_ev = perc_notes[i]
            if base_ev['id'] in dropped_ids:
                i += 1
                continue

            group = [base_ev]
            j = i + 1
            while j < len(perc_notes) and (perc_notes[j]['time'] - base_ev['time'] <= 0.015):
                if perc_notes[j]['id'] not in dropped_ids:
                    group.append(perc_notes[j])
                j += 1

            if len(group) > 1:
                def get_rank(ev):
                    cfg = get_percussion_config(ev['note'])
                    dclass = cfg.get('drum_class', 'default')
                    return (PERCUSSION_RANK.get(dclass, 0), ev.get('velocity', 0))

                group.sort(key=get_rank, reverse=True)
                for loser in group[1:]:
                    dropped_ids.add(loser['id'])

            i = j

        if dropped_ids:
            self.events = [ev for ev in self.events if ev.get('id') not in dropped_ids]
            print(f"   Percussion Optimization: Collapsed {len(dropped_ids)} simultaneous drum hits.")

    def _find_harmonic_root(self, notes, active_melody_note=None):
        pitches = [n['note'] for n in notes]
        scores = {}
        for p in pitches:
            score = 0
            for other in pitches:
                if other == p:
                    continue
                interval = (other - p) % 12
                if interval in [0, 7]:
                    score += 4
                elif interval in [3, 4]:
                    score += 3
                elif interval in [9, 10]:
                    score += 2
                elif interval in [2, 5]:
                    score += 1
            scores[p] = score

        sorted_pitches = sorted(pitches, key=lambda p: (scores[p], -p), reverse=True)
        best_root = sorted_pitches[0]

        if active_melody_note is not None and len(sorted_pitches) > 1:
            mel_pc = active_melody_note % 12
            root_pc = best_root % 12
            diff = min((mel_pc - root_pc) % 12, (root_pc - mel_pc) % 12)
            if diff <= 2:
                for alt in sorted_pitches[1:]:
                    alt_pc = alt % 12
                    alt_diff = min((mel_pc - alt_pc) % 12, (alt_pc - mel_pc) % 12)
                    if alt_diff > 2:
                        best_root = alt
                        break

        for n in notes:
            if n['note'] == best_root:
                return n
        return notes[0]

    def _simplify_chords(self):
        window_size = self.chord_window_simplify
        dropped_ids = set()
        i = 0

        melody_events = [e for e in self.events if e['type'] == 'note_on' and e.get('role') == 'melody']

        def get_active_melody(t):
            for m in reversed(melody_events):
                if m['time'] <= t and t < m['time'] + m.get('duration', 0.5):
                    return m['note']
            return None

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
                    chan_groups.setdefault(ch, []).append(wev)

                for ch, notes in chan_groups.items():
                    if len(notes) > 1:
                        role = notes[0].get('role', 'harmony')
                        if role == 'melody' or ch == self.melody_channel:
                            notes.sort(key=lambda x: x['note'])
                            kept = notes[-1]
                        elif role == 'bass' or (ch == self.bass_channel and ch != self.melody_channel):
                            notes.sort(key=lambda x: x['note'])
                            kept = notes[0]
                        elif role == 'percussion' or ch == 9:
                            kept = notes[0]
                        else:
                            act_mel = get_active_melody(ev['time'])
                            kept = self._find_harmonic_root(notes, act_mel)

                        for n in notes:
                            if n['id'] != kept['id']:
                                dropped_ids.add(n['id'])

                i = j
            else:
                i += 1

        if dropped_ids:
            self.events = [ev for ev in self.events if ev.get('id') not in dropped_ids]
            print(f"   Chord Simplification: Collapsed {len(dropped_ids)} inner chord notes.")

    def _deduplicate_interchannel(self):
        role_prio = {'melody': 4, 'bass': 3, 'percussion': 2, 'harmony': 1}
        dropped_ids = set()
        window_size = 0.010

        note_ons = [ev for ev in self.events if ev['type'] == 'note_on']
        i = 0
        while i < len(note_ons):
            ev1 = note_ons[i]
            if ev1['id'] in dropped_ids:
                i += 1
                continue

            j = i + 1
            while j < len(note_ons) and (note_ons[j]['time'] - ev1['time'] <= window_size):
                ev2 = note_ons[j]
                if ev2['id'] not in dropped_ids and ev1['channel'] != ev2['channel']:
                    is_unison = (ev1['note'] == ev2['note'])
                    is_octave = (abs(ev1['note'] - ev2['note']) % 12 == 0)

                    if is_unison or (is_octave and ev1.get('role') == 'harmony' and ev2.get('role') == 'harmony'):
                        p1 = role_prio.get(ev1.get('role', 'harmony'), 1)
                        p2 = role_prio.get(ev2.get('role', 'harmony'), 1)

                        if p1 >= p2:
                            dropped_ids.add(ev2['id'])
                        else:
                            dropped_ids.add(ev1['id'])
                            break
                j += 1
            i += 1

        if dropped_ids:
            self.events = [ev for ev in self.events if ev.get('id') not in dropped_ids]
            print(f"   Inter-channel Deduplication: Dropped {len(dropped_ids)} redundant notes.")

    def _cull_polyphony(self):
        role_scores = {'melody': 8000, 'bass': 5000, 'percussion': 3000, 'harmony': 1000}

        def get_prio(ev):
            role = ev.get('role', 'harmony')
            score = role_scores.get(role, 1000)
            note = ev.get('note', 60)
            if note > 80:
                score += 500
            if note < 40:
                score += 500
            return score + note

        dropped_ids = set()
        i = 0
        note_ons = [ev for ev in self.events if ev['type'] == 'note_on']

        while i < len(note_ons):
            ev = note_ons[i]
            if ev['id'] in dropped_ids:
                i += 1
                continue

            window_events = [ev]
            j = i + 1
            while j < len(note_ons) and (note_ons[j]['time'] - ev['time'] < 0.002):
                if note_ons[j]['id'] not in dropped_ids:
                    window_events.append(note_ons[j])
                j += 1

            if len(window_events) > self.max_polyphony:
                window_events.sort(key=get_prio, reverse=True)
                for drop_ev in window_events[self.max_polyphony:]:
                    dropped_ids.add(drop_ev['id'])

            i = j

        if dropped_ids:
            self.events = [ev for ev in self.events if ev.get('id') not in dropped_ids]
            print(f"   Polyphony Culling: Dropped {len(dropped_ids)} overlapping Note Ons.")

    def _flatten_events(self):
        track_events = []
        for track_idx, track in enumerate(self.mid.tracks):
            track_forced_role = None
            track_name = getattr(track, 'name', '') or ''
            parsed_wave_override = None

            for msg in track:
                if msg.type in ['text', 'track_name', 'marker']:
                    text = getattr(msg, 'text', '') or getattr(msg, 'name', '') or ''
                    if msg.type == 'track_name' and text:
                        track_name = text
                    if '65C02_ROLE:MELODY' in text or '[MAIN]' in text:
                        track_forced_role = 'melody'
                        self.detected_meta_melody_tracks.add(track_idx)
                    elif '65C02_ROLE:BASS' in text:
                        track_forced_role = 'bass'
                        self.detected_meta_bass_tracks.add(track_idx)
                    elif '65C02_ROLE:HARMONY' in text:
                        track_forced_role = 'harmony'
                    if '65C02_WAVE:' in text:
                        wave_str = text.split('65C02_WAVE:', 1)[1].strip().split()[0]
                        try:
                            parsed_wave_override = int(wave_str, 16)
                        except ValueError:
                            pass

            if self.forced_main_track is not None:
                if str(self.forced_main_track).isdigit():
                    if int(self.forced_main_track) == track_idx:
                        track_forced_role = 'melody'
                elif isinstance(self.forced_main_track, str) and track_name and self.forced_main_track.lower() in track_name.lower():
                    track_forced_role = 'melody'

            abs_ticks = 0
            for msg in track:
                abs_ticks += msg.time
                if msg.type in ['note_on', 'note_off', 'set_tempo', 'program_change', 'control_change', 'pitchwheel']:
                    ch = getattr(msg, 'channel', None)
                    if parsed_wave_override is not None and ch is not None:
                        self.custom_wave_overrides[ch] = parsed_wave_override
                    if msg.type == 'note_on' and msg.velocity > 0 and ch is not None and ch != 9:
                        if track_forced_role == 'melody' and self.forced_melody_channel is None:
                            self.forced_melody_channel = ch
                        elif track_forced_role == 'bass' and self.forced_bass_channel is None:
                            self.forced_bass_channel = ch
                    track_events.append((abs_ticks, msg, track_idx, track_forced_role))

        track_events.sort(key=lambda x: x[0])

        current_tempo = 500000
        seconds_per_tick = current_tempo / (1000000.0 * self.mid.ticks_per_beat)

        last_tick = 0
        current_time = 0.0

        for ticks, msg, track_idx, track_forced_role in track_events:
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
                    'channel': getattr(msg, 'channel', 0),
                    'track_idx': track_idx,
                    'forced_role': track_forced_role
                })
            elif msg.type == 'control_change':
                if msg.control in [1, 7, 6, 38, 71, 74, 98, 99, 100, 101]: # Modulation, Volume, Data Entry, Filter Res/Cutoff, NRPN/RPN
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
                if ch not in stats:
                    stats[ch] = {'count': 0, 'total': 0}
                stats[ch]['count'] += 1
                stats[ch]['total'] += ev['note']

        valid = [c for c, s in stats.items() if s['count'] > 10 and c != 9]
        if valid:
            self.bass_channel = min(valid, key=lambda c: stats[c]['total'] / stats[c]['count'])
            self.melody_channel = max(valid, key=lambda c: stats[c]['total'] / stats[c]['count'])
        else:
            self.bass_channel = -1
            self.melody_channel = -1

        if self.forced_melody_channel is not None:
            self.melody_channel = int(self.forced_melody_channel)
            print(f"   [Main Track Override] Locked Melody Channel -> Ch{self.melody_channel}")
        if self.forced_bass_channel is not None:
            self.bass_channel = int(self.forced_bass_channel)
            print(f"   [Bass Track Override] Locked Bass Channel -> Ch{self.bass_channel}")

        print(f"   Detected Roles -> Bass: Ch{self.bass_channel}, Melody: Ch{self.melody_channel}")

    def _filter_short_notes(self):
        note_starts = {}
        dropped_ids = set()

        for ev in self.events:
            if ev['type'] not in ['note_on', 'note_off']:
                continue

            note_id = ev.get('id')
            if ev['type'] == 'note_on':
                note_starts[note_id] = ev
            elif ev['type'] == 'note_off':
                if note_id in note_starts:
                    start_ev = note_starts[note_id]
                    duration = ev['time'] - start_ev['time']
                    if duration < self.min_note_duration:
                        dropped_ids.add(note_id)
                    del note_starts[note_id]

        if dropped_ids:
            self.events = [ev for ev in self.events if ev.get('id') not in dropped_ids]
            print(f"   Filter Short Notes: Removed {len(dropped_ids)} events.")

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
