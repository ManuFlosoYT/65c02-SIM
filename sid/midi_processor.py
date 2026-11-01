# MIDI file parsing and preprocessing

import math
import struct

try:
    import mido
except ImportError:
    mido = None

from sid_constants import (
    MODE_MAX_QUALITY, MODE_QUALITY, MODE_BALANCED,
    MODE_COMPRESSED, MODE_MAX_COMPRESSION
)
from instruments import get_percussion_config


class _FallbackMsg:
    def __init__(self, msg_type, time=0, **kwargs):
        self.type = msg_type
        self.time = time
        for k, v in kwargs.items():
            setattr(self, k, v)


class _FallbackTrack(list):
    def __init__(self):
        super().__init__()
        self.name = ''


class _FallbackMidiFile:
    def __init__(self, midi_path):
        self.ticks_per_beat = 480
        self.tracks = []
        with open(midi_path, 'rb') as f:
            data = f.read()
        pos = 0
        n = len(data)
        if n >= 14 and data[:4] == b'MThd':
            hdr_len = struct.unpack('>I', data[4:8])[0]
            _, _, division = struct.unpack('>HHH', data[8:14])
            if not (division & 0x8000) and division > 0:
                self.ticks_per_beat = division
            pos = 8 + hdr_len
        while pos + 8 <= n:
            chunk_id = data[pos:pos + 4]
            chunk_len = struct.unpack('>I', data[pos + 4:pos + 8])[0]
            pos += 8
            chunk_end = min(n, pos + chunk_len)
            if chunk_id == b'MTrk':
                self.tracks.append(self._parse_track(data, pos, chunk_end))
            pos = chunk_end

    @staticmethod
    def _read_vlq(data, idx, end):
        val = 0
        while idx < end:
            b = data[idx]
            idx += 1
            val = (val << 7) | (b & 0x7F)
            if not (b & 0x80):
                break
        return val, idx

    def _parse_track(self, data, idx, end):
        track = _FallbackTrack()
        running_status = 0
        while idx < end:
            delta, idx = self._read_vlq(data, idx, end)
            if idx >= end:
                break
            status = data[idx]
            if status < 0x80:
                if running_status == 0:
                    idx += 1
                    continue
                status = running_status
            else:
                idx += 1
                if status < 0xF0:
                    running_status = status
                elif status < 0xF8:
                    running_status = 0

            if status == 0xFF:
                if idx >= end:
                    break
                meta_type = data[idx]
                idx += 1
                meta_len, idx = self._read_vlq(data, idx, end)
                payload = data[idx:min(end, idx + meta_len)]
                idx = min(end, idx + meta_len)
                if meta_type == 0x51 and len(payload) >= 3:
                    tempo = (payload[0] << 16) | (payload[1] << 8) | payload[2]
                    track.append(_FallbackMsg('set_tempo', time=delta, tempo=tempo))
                elif meta_type in (0x01, 0x03, 0x06):
                    txt = payload.decode('latin1', errors='replace')
                    mtype = {0x01: 'text', 0x03: 'track_name', 0x06: 'marker'}[meta_type]
                    if meta_type == 0x03 and not track.name:
                        track.name = txt
                    track.append(_FallbackMsg(mtype, time=delta, text=txt, name=txt))
                elif meta_type == 0x2F:
                    track.append(_FallbackMsg('end_of_track', time=delta))
                    break
                else:
                    track.append(_FallbackMsg('meta', time=delta))
            elif status in (0xF0, 0xF7):
                syx_len, idx = self._read_vlq(data, idx, end)
                idx = min(end, idx + syx_len)
                track.append(_FallbackMsg('sysex', time=delta))
            elif status >= 0xF1:
                if status == 0xF2:
                    idx = min(end, idx + 2)
                elif status in (0xF1, 0xF3):
                    idx = min(end, idx + 1)
                track.append(_FallbackMsg('system', time=delta))
            else:
                high = status & 0xF0
                ch = status & 0x0F
                if high in (0xC0, 0xD0):
                    d1 = min(127, data[idx]) if idx < end else 0
                    idx = min(end, idx + 1)
                    if high == 0xC0:
                        track.append(_FallbackMsg('program_change', time=delta, channel=ch, program=d1))
                    else:
                        track.append(_FallbackMsg('aftertouch', time=delta, channel=ch, value=d1))
                else:
                    d1 = min(127, data[idx]) if idx < end else 0
                    d2 = min(127, data[idx + 1]) if idx + 1 < end else 0
                    idx = min(end, idx + 2)
                    if high == 0x80:
                        track.append(_FallbackMsg('note_off', time=delta, channel=ch, note=d1, velocity=d2))
                    elif high == 0x90:
                        track.append(_FallbackMsg('note_on', time=delta, channel=ch, note=d1, velocity=d2))
                    elif high == 0xA0:
                        track.append(_FallbackMsg('polytouch', time=delta, channel=ch, note=d1, value=d2))
                    elif high == 0xB0:
                        track.append(_FallbackMsg('control_change', time=delta, channel=ch, control=d1, value=d2))
                    elif high == 0xE0:
                        pitch = ((d2 << 7) | d1) - 8192
                        track.append(_FallbackMsg('pitchwheel', time=delta, channel=ch, pitch=pitch))
        return track


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
        self.mid = mido.MidiFile(midi_path, clip=True) if mido is not None else _FallbackMidiFile(midi_path)
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
        self._trim_leading_silence()
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
        self._trim_leading_silence()

        if hasattr(self, 'density_map') and self.density_map:
            self._quantize_dynamic()
        else:
            for ev in self.events:
                ev['time'] = round(ev['time'] / self.quant_grid) * self.quant_grid

        self.events.sort(key=lambda x: (x['time'], 0 if x['type'] == 'note_off' else 1))

        self._deduplicate()

        return self.events

    def _trim_leading_silence(self):
        first_note_time = min(
            (ev['time'] for ev in self.events if ev['type'] in ('note_on', 'arpeggio_on')),
            default=0.0
        )
        if first_note_time > 0.0:
            for ev in self.events:
                ev['time'] = max(0.0, ev['time'] - first_note_time)

        last_zero_setup = {}
        for idx, ev in enumerate(self.events):
            if ev['time'] == 0.0 and ev['type'] in ('program_change', 'control_change', 'pitchwheel'):
                key = (ev.get('channel', 0), ev['type'], ev.get('control') if ev['type'] == 'control_change' else None)
                last_zero_setup[key] = idx
        if last_zero_setup:
            keep_indices = set(last_zero_setup.values())
            self.events = [
                ev for idx, ev in enumerate(self.events)
                if not (ev['time'] == 0.0 and ev['type'] in ('program_change', 'control_change', 'pitchwheel')) or idx in keep_indices
            ]

    def _pair_notes(self):
        active_notes = {}
        off_by_id = {}
        chan_note_ons = {}
        note_id_counter = 0
        for ev in self.events:
            if ev['type'] == 'note_on':
                note_id_counter += 1
                ev['id'] = note_id_counter
                active_notes.setdefault((ev['channel'], ev['note']), []).append(ev)
                chan_note_ons.setdefault(ev['channel'], []).append(ev)
            elif ev['type'] == 'note_off':
                key = (ev['channel'], ev['note'])
                if active_notes.get(key):
                    matching_on = active_notes[key].pop(0)
                    ev['id'] = matching_on['id']
                    matching_on['duration'] = max(0.0, ev['time'] - matching_on['time'])
                    off_by_id[matching_on['id']] = ev
                else:
                    note_id_counter += 1
                    ev['id'] = note_id_counter

        for ch, ons in chan_note_ons.items():
            if ch == 9:
                continue
            for idx, on_ev in enumerate(ons):
                off_ev = off_by_id.get(on_ev['id'])
                if not off_ev or on_ev.get('duration', 0.0) <= 0.025:
                    continue
                next_on_ev = None
                for k in range(idx + 1, len(ons)):
                    if ons[k]['time'] > on_ev['time'] + 0.025:
                        next_on_ev = ons[k]
                        break
                if next_on_ev and (next_on_ev['time'] - 0.002 <= off_ev['time'] <= next_on_ev['time'] + 0.15):
                    off_ev['time'] = max(on_ev['time'] + 0.015, next_on_ev['time'] - 0.004)
                    on_ev['duration'] = max(0.0, off_ev['time'] - on_ev['time'])

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

                    eff_count = min(count, 24)
                    pitch_variety = min(12, len(set(n['note'] for n in notes))) * 1.5
                    mel_score = (avg_pitch - 55.0) * 1.5 + eff_count * 1.2 + pitch_variety + (1.0 - overlap_ratio) * 25.0 + prog_mel_bonus
                    bass_score = (65.0 - avg_pitch) * 2.0 + eff_count * 1.0 + (1.0 - overlap_ratio) * 20.0 + prog_bass_bonus

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

    def _find_harmonic_root(self, notes, active_melody_note=None, active_bass_note=None):
        pitches = [n['note'] for n in notes]
        scores = {}
        mel_pc = active_melody_note % 12 if active_melody_note is not None else None
        bass_pc = active_bass_note % 12 if active_bass_note is not None else None

        for p in pitches:
            score = 0
            pc = p % 12
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

            if bass_pc is not None:
                b_int = (pc - bass_pc) % 12
                if b_int == 0:
                    score -= 6
                elif b_int in (3, 4, 9, 10, 11):
                    score += 5
                elif b_int == 7:
                    score += 2

            if mel_pc is not None:
                m_diff = min((pc - mel_pc) % 12, (mel_pc - pc) % 12)
                if m_diff == 0:
                    score -= 6
                elif m_diff <= 2:
                    score -= 3

            scores[p] = score

        sorted_pitches = sorted(pitches, key=lambda p: (scores[p], -p), reverse=True)
        best_root = sorted_pitches[0]

        for n in notes:
            if n['note'] == best_root:
                return n
        return notes[0]

    def _simplify_chords(self):
        window_size = self.chord_window_simplify
        dropped_ids = set()
        i = 0

        melody_events = [e for e in self.events if e['type'] == 'note_on' and e.get('role') == 'melody']
        bass_events = [e for e in self.events if e['type'] == 'note_on' and e.get('role') == 'bass']

        def get_active_melody(t):
            for m in reversed(melody_events):
                if m['time'] <= t and t < m['time'] + m.get('duration', 0.5):
                    return m['note']
            return None

        def get_active_bass(t):
            for b in reversed(bass_events):
                if b['time'] <= t and t < b['time'] + b.get('duration', 0.5):
                    return b['note']
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
                            act_bass = get_active_bass(ev['time'])
                            kept = self._find_harmonic_root(notes, act_mel, act_bass)

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
                if msg.control in [1, 7, 11, 6, 38, 71, 74, 98, 99, 100, 101]: # Modulation, Volume, Data Entry, Filter Res/Cutoff, NRPN/RPN
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
        chan_notes = {}
        channel_programs = {}
        for ev in self.events:
            if ev['type'] == 'program_change':
                channel_programs[ev['channel']] = ev['program']
            elif ev['type'] == 'note_on':
                ch = ev['channel']
                if ch != 9:
                    chan_notes.setdefault(ch, []).append(ev)

        valid = [c for c, notes in chan_notes.items() if len(notes) > 10]
        if not valid and chan_notes:
            valid = list(chan_notes.keys())

        if valid:
            song_span = max((e['time'] for e in self.events), default=1.0)
            mel_scores = {}
            bass_scores = {}
            avg_pitches = {}

            for ch in valid:
                notes = chan_notes[ch]
                count = len(notes)
                avg_pitch = sum(n['note'] for n in notes) / count
                avg_pitches[ch] = avg_pitch
                span = max(0.0, notes[-1]['time'] - notes[0]['time'])
                coverage = min(1.0, span / max(1.0, song_span))

                overlaps = 0
                for k in range(count - 1):
                    if notes[k]['time'] + notes[k].get('duration', 0.2) > notes[k + 1]['time'] + 0.005:
                        overlaps += 1
                overlap_ratio = overlaps / count

                pitch_variety = len(set(n['note'] for n in notes))
                prog = channel_programs.get(ch, 0)
                prog_bass_bonus = 25.0 if 32 <= prog <= 39 else 0.0
                prog_mel_bonus = 20.0 if (80 <= prog <= 87 or 72 <= prog <= 79 or 40 <= prog <= 43 or 56 <= prog <= 71) else 0.0

                activity = min(40.0, math.log2(count + 1) * 6.0) + coverage * 20.0 + min(15.0, pitch_variety * 0.8)
                mel_scores[ch] = (avg_pitch - 55.0) * 1.2 + activity + (1.0 - overlap_ratio) * 30.0 + prog_mel_bonus
                bass_scores[ch] = (62.0 - avg_pitch) * 1.8 + activity * 0.8 + (1.0 - overlap_ratio) * 25.0 + prog_bass_bonus

            self.melody_channel = max(valid, key=lambda c: mel_scores[c])
            remaining = [c for c in valid if c != self.melody_channel]
            if remaining:
                self.bass_channel = max(remaining, key=lambda c: bass_scores[c])
            else:
                self.bass_channel = self.melody_channel
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
            if ev['type'] == 'note_on' and ev.get('role') == 'harmony' and ev.get('duration', 0.0) >= 0.18:
                chord_candidates = [ev]
                others = []

                j = i + 1
                while j < len(self.events) and (self.events[j]['time'] - ev['time'] < window):
                    scan_ev = self.events[j]
                    if (
                        scan_ev['type'] == 'note_on'
                        and scan_ev.get('channel') == ev.get('channel')
                        and scan_ev.get('role') == 'harmony'
                        and scan_ev.get('duration', 0.0) >= 0.18
                    ):
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
                        'note': chord_candidates[0]['note'],
                        'role': 'harmony',
                        'id': ev.get('id')
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
