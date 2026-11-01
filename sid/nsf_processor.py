# NSF (Nintendo Sound Format) processor using headless SIM_65C02 APU trace

import json
import math
import os
import shutil
import struct
import subprocess
import tempfile
from typing import Any, Dict, List, Optional, Union

try:
    import mido
except ImportError:
    mido = None

from midi_processor import MidiProcessor
from sid_constants import MODE_MAX_QUALITY

NOISE_PERIODS = [4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068]

_EVENT_PRIORITY = {
    'note_off': 0,
    'program_change': 1,
    'control_change': 2,
    'pitchwheel': 3,
    'note_on': 4,
    'arpeggio_on': 4,
}


class NsfProcessor(MidiProcessor):
    def __init__(
        self,
        nsf_path: Optional[Union[str, Dict[str, Any]]] = None,
        mode: Union[str, int] = MODE_MAX_QUALITY,
        duration: float = 0.0,
        song_idx: int = 0,
        trace_data: Optional[Dict[str, Any]] = None,
        target_polyphony: Optional[int] = None,
    ):
        if isinstance(nsf_path, dict):
            trace_data = nsf_path
            nsf_path = "<trace_dict>"
        if isinstance(mode, int):
            target_polyphony = mode
            mode = MODE_MAX_QUALITY

        self.midi_path = nsf_path or ""
        self.nsf_path = nsf_path or ""
        self.mode = mode
        self.duration = float(duration) if duration is not None else 0.0
        self.song_idx = int(song_idx)
        self.trace_data = trace_data
        self.events: List[Dict[str, Any]] = []
        self.bass_channel = 2
        self.melody_channel = 0
        self.noise_channel = 3
        self.custom_wave_overrides: Dict[int, int] = {}

        self.fps = 60.098814
        self.total_frames = 0
        self.detected_duration = 0.0
        self.stop_reason = "unknown"
        self.loop_start_frame = -1
        self.loop_end_frame = -1
        self.song_name = ""
        self.artist = ""
        self.copyright = ""

        self.quant_grid = 0.001
        self.min_note_duration = 0.0
        self.chord_reduce = False

        self._configure_mode()
        if target_polyphony is not None:
            self.max_polyphony = int(target_polyphony)

    @staticmethod
    def _find_sim_binary() -> str:
        env_bin = os.environ.get("SIM_65C02_BIN")
        if env_bin and os.path.isfile(env_bin):
            return env_bin

        repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        exe_suffix = ".exe" if os.name == "nt" else ""
        candidates = [
            os.path.join(repo_root, "output", f"SIM_65C02{exe_suffix}"),
            os.path.join(repo_root, "output", "SIM_65C02"),
            os.path.join(repo_root, "build", "release", f"SIM_65C02{exe_suffix}"),
            os.path.join(repo_root, "build", "release", "SIM_65C02"),
            os.path.join(repo_root, "build", "debug", f"SIM_65C02{exe_suffix}"),
            os.path.join(repo_root, "build", "debug", "SIM_65C02"),
        ]
        for cand in candidates:
            if os.path.isfile(cand) and os.access(cand, os.X_OK):
                return cand

        which_bin = shutil.which("SIM_65C02")
        if which_bin:
            return which_bin

        raise FileNotFoundError(
            "SIM_65C02 binary not found. Set SIM_65C02_BIN or build SIM_65C02 in output/ or build/release/."
        )

    def _load_trace(self, duration: float) -> Dict[str, Any]:
        if self.trace_data is not None:
            return self.trace_data

        if self.nsf_path.lower().endswith(".json"):
            with open(self.nsf_path, "r", encoding="utf-8") as f:
                return json.load(f)

        sim_bin = self._find_sim_binary()
        fd, tmp_json_path = tempfile.mkstemp(suffix=".json", prefix="nsf_trace_")
        os.close(fd)
        try:
            cmd = [sim_bin, "--headless", "--rom", self.nsf_path, "--nsf-trace", tmp_json_path]
            if duration and duration > 0:
                cmd.extend(["--nsf-max-duration", str(duration)])
            if self.song_idx > 0:
                cmd.extend(["--nsf-song", str(self.song_idx)])
            subprocess.run(cmd, check=True)
            with open(tmp_json_path, "r", encoding="utf-8") as f:
                return json.load(f)
        finally:
            if os.path.exists(tmp_json_path):
                os.remove(tmp_json_path)

    @staticmethod
    def noise_period_to_note(period_idx: int) -> int:
        period = NOISE_PERIODS[int(period_idx) & 0x0F]
        freq = 1789773.0 / (32.0 * period)
        exact_note = 69.0 + 12.0 * math.log2(freq / 440.0)
        return max(0, min(127, int(round(exact_note))))

    def parse(self, duration: Optional[float] = None) -> List[Dict[str, Any]]:
        eff_duration = self.duration if duration is None else float(duration)
        trace = self._load_trace(eff_duration)

        self.fps = float(trace.get("fps", 60.098814)) or 60.098814
        frames = trace.get("frames", [])
        self.total_frames = int(trace.get("total_frames", len(frames)))
        self.detected_duration = float(
            trace.get("duration_sec", self.total_frames / self.fps if self.fps > 0 else 0.0)
        )
        self.stop_reason = str(trace.get("stop_reason", "unknown"))
        self.loop_start_frame = int(trace.get("loop_start_frame", -1))
        self.loop_end_frame = int(trace.get("loop_end_frame", -1))
        self.song_name = str(trace.get("song_name", ""))
        self.artist = str(trace.get("artist", ""))
        self.copyright = str(trace.get("copyright", ""))

        print(f"[NSF] Auto-detected duration: {self.detected_duration:.2f}s (reason: {self.stop_reason})")

        self.events = []
        channel_programs = {0: 128, 1: 128, 2: 129, 3: 130}
        for ch in range(4):
            self.events.append({'time': 0.0, 'type': 'program_change', 'program': channel_programs[ch], 'channel': ch})
            self.events.append({'time': 0.0, 'type': 'control_change', 'control': 101, 'value': 0, 'channel': ch})
            self.events.append({'time': 0.0, 'type': 'control_change', 'control': 100, 'value': 0, 'channel': ch})
            self.events.append({'time': 0.0, 'type': 'control_change', 'control': 6, 'value': 24, 'channel': ch})

        ch_states = [
            {
                'active_note': None,
                'active_note_id': None,
                'active_note_ev': None,
                'last_pitchbend': 0,
                'last_vol': 0,
                'last_duty': -1,
            }
            for _ in range(4)
        ]
        note_id_counter = 0
        last_time = 0.0

        def close_active_note(ch: int, st: Dict[str, Any], t: float) -> None:
            if st['active_note'] is not None:
                on_ev = st['active_note_ev']
                if on_ev is not None:
                    on_ev['duration'] = max(0.0, t - on_ev['time'])
                self.events.append({
                    'time': t,
                    'type': 'note_off',
                    'note': st['active_note'],
                    'channel': ch,
                    'id': st['active_note_id'],
                })
                st['active_note'] = None
                st['active_note_id'] = None
                st['active_note_ev'] = None
                st['last_vol'] = 0

        for frame in frames:
            frame_idx = int(frame[0])
            current_time = frame_idx / self.fps
            last_time = current_time

            for ch in range(4):
                ch_data = frame[ch + 1]
                vol = int(ch_data[0])
                p1 = int(ch_data[1])
                p2 = int(ch_data[2])
                retrigger = int(ch_data[3])
                st = ch_states[ch]

                exact_note = -1.0
                duty = 0

                if vol > 0:
                    if ch in (0, 1):
                        duty = p2 & 0x03
                        if p1 < 8:
                            vol = 0
                        else:
                            freq = 1789773.0 / (16.0 * (p1 + 1))
                            exact_note = 69.0 + 12.0 * math.log2(freq / 440.0)
                    elif ch == 2:
                        if p1 < 2:
                            vol = 0
                        else:
                            freq = 1789773.0 / (32.0 * (p1 + 1))
                            if freq < 20.0:
                                vol = 0
                            else:
                                exact_note = 69.0 + 12.0 * math.log2(freq / 440.0)
                    else:
                        period = NOISE_PERIODS[p1 & 0x0F]
                        freq = 1789773.0 / (32.0 * period)
                        exact_note = 69.0 + 12.0 * math.log2(freq / 440.0)

                if vol <= 0 or (ch < 3 and (exact_note < 0.0 or exact_note > 135.0)):
                    close_active_note(ch, st, current_time)
                    continue

                target_note = max(0, min(127, int(round(exact_note))))
                vel = max(1, min(127, int(round((vol / 15.0) * 127.0))))

                if ch in (0, 1) and duty != st['last_duty']:
                    self.events.append({
                        'time': current_time,
                        'type': 'control_change',
                        'control': 14,
                        'value': duty,
                        'channel': ch,
                    })
                    st['last_duty'] = duty

                need_new_note = (
                    st['active_note'] is None
                    or target_note != st['active_note']
                    or retrigger == 1
                )

                if need_new_note:
                    close_active_note(ch, st, current_time)
                    note_id_counter += 1
                    st['active_note'] = target_note
                    st['active_note_id'] = note_id_counter

                    if ch < 3:
                        frac_diff = exact_note - target_note
                        pb = 0 if abs(frac_diff) < 0.05 else max(-8192, min(8191, int(round(frac_diff * (8192.0 / 24.0)))))
                    else:
                        pb = 0

                    if pb != st['last_pitchbend']:
                        self.events.append({
                            'time': current_time,
                            'type': 'pitchwheel',
                            'pitch': pb,
                            'channel': ch,
                        })
                        st['last_pitchbend'] = pb

                    on_ev = {
                        'time': current_time,
                        'type': 'note_on',
                        'note': target_note,
                        'velocity': vel,
                        'channel': ch,
                        'id': note_id_counter,
                    }
                    st['active_note_ev'] = on_ev
                    self.events.append(on_ev)
                    st['last_vol'] = vol
                else:
                    if ch < 3:
                        frac_diff = exact_note - st['active_note']
                        pb = max(-8192, min(8191, int(round(frac_diff * (8192.0 / 24.0)))))
                        if abs(pb - st['last_pitchbend']) >= 68:
                            self.events.append({
                                'time': current_time,
                                'type': 'pitchwheel',
                                'pitch': pb,
                                'channel': ch,
                            })
                            st['last_pitchbend'] = pb

                    if vol != st['last_vol']:
                        self.events.append({
                            'time': current_time,
                            'type': 'control_change',
                            'control': 7,
                            'value': vel,
                            'channel': ch,
                        })
                        st['last_vol'] = vol

        end_time = max(last_time, self.total_frames / self.fps if self.fps > 0 else last_time)
        for ch in range(4):
            close_active_note(ch, ch_states[ch], end_time)

        self._trim_leading_silence()
        self._analyze_channels()
        self._analyze_roles_segmented()
        self._filter_short_notes()
        self._simplify_chords()

        if self.chord_reduce:
            self._reduce_chords()

        self._cull_polyphony()
        self._trim_leading_silence()

        if hasattr(self, 'density_map') and self.density_map:
            self._quantize_dynamic()
        else:
            for ev in self.events:
                ev['time'] = round(ev['time'] / self.quant_grid) * self.quant_grid

        self.events.sort(key=lambda x: (x['time'], _EVENT_PRIORITY.get(x['type'], 5), x.get('channel', 0)))
        self._deduplicate()
        return self.events

    def process(self, duration: Optional[float] = None) -> List[Dict[str, Any]]:
        return self.parse(duration=duration)

    def _simplify_chords(self):
        pass

    def _analyze_channels(self) -> None:
        avg_note: Dict[int, List[int]] = {0: [], 1: [], 2: [], 3: []}
        for ev in self.events:
            if ev['type'] == 'note_on':
                avg_note.setdefault(ev.get('channel', 0), []).append(ev['note'])

        avg_0 = sum(avg_note[0]) / len(avg_note[0]) if avg_note[0] else 0.0
        avg_1 = sum(avg_note[1]) / len(avg_note[1]) if avg_note[1] else 0.0

        self.melody_channel = 0 if avg_0 >= avg_1 else 1
        self.bass_channel = 2
        self.noise_channel = 3

    def _analyze_roles_segmented(self, window_size: float = 2.0) -> None:
        duration = max((ev['time'] for ev in self.events), default=0.0)
        if duration <= 0.0:
            for ev in self.events:
                ch = ev.get('channel', 0)
                ev['role'] = 'percussion' if ch == 3 else ('bass' if ch == 2 else ('melody' if ch == self.melody_channel else 'harmony'))
            return

        num_windows = max(1, int(math.ceil(duration / window_size)))
        prev_melody = self.melody_channel
        self.window_roles = []

        for w in range(num_windows):
            t_start = w * window_size
            t_end = (w + 1) * window_size

            pulse_notes: Dict[int, List[Dict[str, Any]]] = {0: [], 1: []}
            for ev in self.events:
                if ev['type'] == 'note_on' and t_start <= ev['time'] < t_end:
                    ch = ev.get('channel', 0)
                    if ch in (0, 1):
                        pulse_notes[ch].append(ev)

            roles: Dict[int, str] = {2: 'bass', 3: 'percussion'}
            if pulse_notes[0] or pulse_notes[1]:
                scores = {}
                for ch in (0, 1):
                    notes = pulse_notes[ch]
                    if not notes:
                        scores[ch] = -1e9
                        continue
                    count = len(notes)
                    avg_pitch = sum(n['note'] for n in notes) / count
                    score = (avg_pitch - 55.0) * 1.5 + count * 1.2
                    if ch == prev_melody:
                        score *= 1.15
                    scores[ch] = score

                best_mel = 0 if scores[0] >= scores[1] else 1
                roles[best_mel] = 'melody'
                roles[1 - best_mel] = 'harmony'
                prev_melody = best_mel
            else:
                roles[prev_melody] = 'melody'
                roles[1 - prev_melody] = 'harmony'

            self.window_roles.append({'start': t_start, 'end': t_end, 'roles': roles})

        for ev in self.events:
            ch = ev.get('channel', 0)
            if ch == 3:
                ev['role'] = 'percussion'
            elif ch == 2:
                ev['role'] = 'bass'
            else:
                w_idx = min(len(self.window_roles) - 1, max(0, int(ev['time'] / window_size)))
                ev['role'] = self.window_roles[w_idx]['roles'].get(ch, 'harmony')

    def _cull_polyphony(self) -> None:
        role_scores = {'melody': 8000, 'bass': 5000, 'harmony': 1000, 'percussion': -5000}

        def get_prio(ev: Dict[str, Any]) -> int:
            ch = ev.get('channel', 0)
            if ch == self.noise_channel:
                score = -5000
            else:
                role = ev.get('role')
                score = role_scores.get(role, 8000 if ch == self.melody_channel else (5000 if ch == self.bass_channel else 1000))
            note = ev.get('note', 60)
            if note > 80:
                score += 500
            if note < 40:
                score += 500
            return score + note

        dropped_note_ids = set()
        note_ons = [ev for ev in self.events if ev['type'] == 'note_on']
        i = 0
        while i < len(note_ons):
            ev = note_ons[i]
            if ev.get('id') in dropped_note_ids:
                i += 1
                continue

            window_events = [ev]
            j = i + 1
            while j < len(note_ons) and (note_ons[j]['time'] - ev['time'] < 0.002):
                if note_ons[j].get('id') not in dropped_note_ids:
                    window_events.append(note_ons[j])
                j += 1

            if len(window_events) > self.max_polyphony:
                window_events.sort(key=get_prio, reverse=True)
                for drop_ev in window_events[self.max_polyphony:]:
                    if drop_ev.get('id') is not None:
                        dropped_note_ids.add(drop_ev['id'])

            i = j

        if dropped_note_ids:
            self.events = [ev for ev in self.events if ev.get('id') not in dropped_note_ids]

    def export_midi(self, output_mid_path: str, events: Optional[List[Dict[str, Any]]] = None) -> str:
        ev_list = events if events is not None else (self.events if self.events else self.parse())
        tempo = 500000
        gm_program_map = {128: 80, 129: 38, 130: 127}
        track_names = {0: 'APU Pulse 1', 1: 'APU Pulse 2', 2: 'APU Triangle', 3: 'APU Noise'}

        if mido is not None:
            mid = mido.MidiFile(ticks_per_beat=480)
            meta_track = mido.MidiTrack()
            mid.tracks.append(meta_track)
            meta_track.append(mido.MetaMessage('track_name', name=self.song_name or 'NSF Track', time=0))
            meta_track.append(mido.MetaMessage('set_tempo', tempo=tempo, time=0))
            meta_track.append(mido.MetaMessage('end_of_track', time=0))

            for ch in range(4):
                ch_events = [e for e in ev_list if e.get('channel', 0) == ch]
                if not ch_events:
                    continue
                ch_events.sort(key=lambda x: (x['time'], _EVENT_PRIORITY.get(x['type'], 5)))

                track = mido.MidiTrack()
                mid.tracks.append(track)
                track.append(mido.MetaMessage('track_name', name=track_names.get(ch, f'Channel {ch}'), time=0))

                last_tick = 0
                for ev in ch_events:
                    abs_tick = max(last_tick, int(round(ev['time'] * (480.0 * 1000000.0 / tempo))))
                    delta = abs_tick - last_tick
                    last_tick = abs_tick

                    etype = ev['type']
                    if etype == 'program_change':
                        raw_prog = int(ev.get('program', 0))
                        prog = gm_program_map.get(raw_prog, max(0, min(127, raw_prog)))
                        track.append(mido.Message('program_change', channel=ch, program=prog, time=delta))
                    elif etype == 'control_change':
                        ctrl = max(0, min(127, int(ev.get('control', 0))))
                        val = max(0, min(127, int(ev.get('value', 0))))
                        track.append(mido.Message('control_change', channel=ch, control=ctrl, value=val, time=delta))
                    elif etype == 'pitchwheel':
                        pitch = max(-8192, min(8191, int(ev.get('pitch', 0))))
                        track.append(mido.Message('pitchwheel', channel=ch, pitch=pitch, time=delta))
                    elif etype == 'note_on':
                        note = max(0, min(127, int(ev.get('note', 60))))
                        vel = max(1, min(127, int(ev.get('velocity', 100))))
                        track.append(mido.Message('note_on', channel=ch, note=note, velocity=vel, time=delta))
                    elif etype == 'note_off':
                        note = max(0, min(127, int(ev.get('note', 60))))
                        track.append(mido.Message('note_off', channel=ch, note=note, velocity=0, time=delta))

                track.append(mido.MetaMessage('end_of_track', time=0))

            mid.save(output_mid_path)
            return output_mid_path

        def encode_vlq(val: int) -> bytes:
            val = max(0, int(val))
            buf = [val & 0x7F]
            val >>= 7
            while val > 0:
                buf.append((val & 0x7F) | 0x80)
                val >>= 7
            return bytes(reversed(buf))

        tracks_bin: List[bytes] = []
        t0 = bytearray()
        title_bytes = (self.song_name or 'NSF Track').encode('ascii', errors='ignore')
        t0.extend(b'\x00\xFF\x03' + encode_vlq(len(title_bytes)) + title_bytes)
        t0.extend(b'\x00\xFF\x51\x03' + tempo.to_bytes(3, 'big'))
        t0.extend(b'\x00\xFF\x2F\x00')
        tracks_bin.append(bytes(t0))

        for ch in range(4):
            ch_events = [e for e in ev_list if e.get('channel', 0) == ch]
            if not ch_events:
                continue
            ch_events.sort(key=lambda x: (x['time'], _EVENT_PRIORITY.get(x['type'], 5)))
            tb = bytearray()
            tname = track_names.get(ch, f'Channel {ch}').encode('ascii', errors='ignore')
            tb.extend(b'\x00\xFF\x03' + encode_vlq(len(tname)) + tname)
            last_tick = 0
            for ev in ch_events:
                abs_tick = max(last_tick, int(round(ev['time'] * (480.0 * 1000000.0 / tempo))))
                delta = abs_tick - last_tick
                last_tick = abs_tick
                etype = ev['type']
                if etype == 'program_change':
                    raw_prog = int(ev.get('program', 0))
                    prog = gm_program_map.get(raw_prog, max(0, min(127, raw_prog)))
                    tb.extend(encode_vlq(delta) + bytes([0xC0 | (ch & 0x0F), prog]))
                elif etype == 'control_change':
                    ctrl = max(0, min(127, int(ev.get('control', 0))))
                    val = max(0, min(127, int(ev.get('value', 0))))
                    tb.extend(encode_vlq(delta) + bytes([0xB0 | (ch & 0x0F), ctrl, val]))
                elif etype == 'pitchwheel':
                    pitch = max(-8192, min(8191, int(ev.get('pitch', 0)))) + 8192
                    tb.extend(encode_vlq(delta) + bytes([0xE0 | (ch & 0x0F), pitch & 0x7F, (pitch >> 7) & 0x7F]))
                elif etype == 'note_on':
                    note = max(0, min(127, int(ev.get('note', 60))))
                    vel = max(1, min(127, int(ev.get('velocity', 100))))
                    tb.extend(encode_vlq(delta) + bytes([0x90 | (ch & 0x0F), note, vel]))
                elif etype == 'note_off':
                    note = max(0, min(127, int(ev.get('note', 60))))
                    tb.extend(encode_vlq(delta) + bytes([0x80 | (ch & 0x0F), note, 0]))
            tb.extend(b'\x00\xFF\x2F\x00')
            tracks_bin.append(bytes(tb))

        with open(output_mid_path, 'wb') as f:
            f.write(b'MThd' + struct.pack('>IHHH', 6, 1, len(tracks_bin), 480))
            for trk in tracks_bin:
                f.write(b'MTrk' + struct.pack('>I', len(trk)) + trk)
        return output_mid_path
