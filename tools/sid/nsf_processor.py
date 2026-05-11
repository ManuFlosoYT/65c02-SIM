# NSF (Nintendo Sound Format) processor using py65

import os
import struct
import math

try:
    from py65.devices.mpu6502 import MPU
except ImportError:
    import sys
    print("Error: 'py65' library not found.")
    print("Please install it using: pip install py65")
    sys.exit(1)

from midi_processor import MidiProcessor

class NsfMemory:
    def __init__(self, processor):
        self.mem = bytearray(65536)
        self.processor = processor
    
    def __getitem__(self, addr):
        return self.mem[addr]
        
    def __setitem__(self, addr, val):
        self.mem[addr] = val
        if 0x4000 <= addr <= 0x4017:
            self.processor._apu_write_hook(addr, val)


class NsfProcessor(MidiProcessor):
    def __init__(self, nsf_path, mode="l1", duration=180):
        self.midi_path = nsf_path # For compatibility
        self.nsf_path = nsf_path
        self.mode = mode
        self.duration = duration
        self.events = []
        self.bass_channel = -1
        self.melody_channel = -1
        
        self.quant_grid = 0.001
        self.min_note_duration = 0.0
        self.chord_reduce = False
        
        self._configure_mode()
        
        self.cpu = MPU()
        self.memory = NsfMemory(self)
        self.cpu.memory = self.memory
        
        self.apu_state = {}
        for i in range(0x4000, 0x4018):
            self.apu_state[i] = 0

        self.has_apu_changes = False
        self._load_nsf()

    def _apu_write_hook(self, address, value):
        self.apu_state[address] = value
        self.has_apu_changes = True
        
    def _load_nsf(self):
        with open(self.nsf_path, "rb") as f:
            header = f.read(128)
            
            if header[0:5] != b'NESM\x1a':
                raise ValueError("Invalid NSF file")
                
            self.version = header[0x05]
            self.total_songs = header[0x06]
            self.starting_song = header[0x07]
            
            self.load_addr = struct.unpack("<H", header[0x08:0x0A])[0]
            self.init_addr = struct.unpack("<H", header[0x0A:0x0C])[0]
            self.play_addr = struct.unpack("<H", header[0x0C:0x0E])[0]
            
            self.song_name = header[0x0E:0x2E].split(b'\x00')[0].decode('ascii', errors='ignore')
            self.artist = header[0x2E:0x4E].split(b'\x00')[0].decode('ascii', errors='ignore')
            self.copyright = header[0x4E:0x6E].split(b'\x00')[0].decode('ascii', errors='ignore')
            
            self.play_speed_ntsc = struct.unpack("<H", header[0x6E:0x70])[0]
            self.bank_switch = list(header[0x70:0x78])
            self.play_speed_pal = struct.unpack("<H", header[0x78:0x7A])[0]
            
            self.ntsc_pal_bits = header[0x7A]
            self.extra_chips = header[0x7B]
            
            data = f.read()
            
            # Load into memory
            addr = self.load_addr
            for b in data:
                self.memory.mem[addr] = b
                addr = (addr + 1) & 0xFFFF
                
        print(f">> NSF Loaded: {self.song_name} by {self.artist}")
        print(f"   Load: ${self.load_addr:04X}, Init: ${self.init_addr:04X}, Play: ${self.play_addr:04X}")

    def process(self):
        print(f"Parsing '{self.nsf_path}' using py65...")
        
        # Setup stack
        self.cpu.sp = 0xFF
        
        # Init routine
        self.cpu.a = self.starting_song - 1
        self.cpu.x = 0 # NTSC
        
        # Let's put a JMP at 0x3FFF to itself to catch return
        self.memory.mem[0x3FFF] = 0x4C # JMP
        self.memory.mem[0x4000] = 0xFF
        self.memory.mem[0x4001] = 0x3F
        
        def call_subroutine(addr):
            # Push return address 0x3FFF - 1 = 0x3FFE
            ret = 0x3FFE
            self.memory.mem[0x0100 + self.cpu.sp] = (ret >> 8) & 0xFF
            self.cpu.sp = (self.cpu.sp - 1) & 0xFF
            self.memory.mem[0x0100 + self.cpu.sp] = ret & 0xFF
            self.cpu.sp = (self.cpu.sp - 1) & 0xFF
            
            self.cpu.pc = addr
            
            # Run until PC reaches 0x3FFF
            cycles = 0
            while self.cpu.pc != 0x3FFF:
                self.cpu.step()
                cycles += 1
                if cycles > 100000: # Timeout
                    break
        
        call_subroutine(self.init_addr)
        
        fps = 60.0
        if self.play_speed_ntsc == 0:
            frame_time = 1.0 / 60.0
        else:
            frame_time = self.play_speed_ntsc / 1000000.0
            
        current_time = 0.0
        max_time = self.duration
        
        self.prev_ch_states = [{'vol': 0, 'freq': 0, 'duty': None, 'active_note': None} for _ in range(4)]
        silence_frames = 0
        
        # Setup Pitchbend Range to 24 for all channels
        for ch in range(4):
            self.events.append({'time': 0, 'type': 'control_change', 'control': 101, 'value': 0, 'channel': ch})
            self.events.append({'time': 0, 'type': 'control_change', 'control': 100, 'value': 0, 'channel': ch})
            self.events.append({'time': 0, 'type': 'control_change', 'control': 6, 'value': 24, 'channel': ch})

        while current_time < max_time:
            self.has_apu_changes = False
            call_subroutine(self.play_addr)
            
            if self.has_apu_changes or current_time == 0.0:
                self._capture_frame(current_time)
                
            # Detect silence
            is_silent = True
            for ch in range(4):
                if self.prev_ch_states[ch] and self.prev_ch_states[ch]['vol'] > 0:
                    is_silent = False
                    break
                    
            if is_silent:
                silence_frames += 1
            else:
                silence_frames = 0
                
            if silence_frames > fps * 3: # 3 seconds of silence
                print(f"   Detected 3s silence at {current_time:.1f}s, stopping.")
                break
                
            current_time += frame_time
            
        # Ensure trailing note_offs
        for ch in range(4):
            if self.prev_ch_states[ch]['vol'] > 0:
                active_note = self.prev_ch_states[ch].get('active_note', 60)
                self.events.append({'time': current_time, 'type': 'note_off', 'note': active_note, 'channel': ch})
            
        print(f"   Generated {len(self.events)} raw events.")
        
        # Post-processing pipeline shared with MidiProcessor
        self._analyze_channels()
        
        # We don't analyze density or key unless we want to
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
        print(f"   Optimized to {len(self.events)} events.")
        
        return self.events

    def _capture_frame(self, current_time):
        self._process_pulse(0, 0x4000, current_time)
        self._process_pulse(1, 0x4004, current_time)
        self._process_triangle(2, 0x4008, current_time)
        self._process_noise(3, 0x400C, current_time)

    def _process_pulse(self, ch, base, current_time):
        ctrl = self.apu_state[base]
        timer_lo = self.apu_state[base+2]
        timer_hi = self.apu_state[base+3]
        
        vol = ctrl & 0x0F
        duty = (ctrl >> 6) & 0x03
        timer = timer_lo | ((timer_hi & 0x07) << 8)
        
        if timer < 8:
            vol = 0
            
        freq = 1789773.0 / (16.0 * (timer + 1)) if timer > 0 else 0
        self._emit_channel_state(ch, current_time, vol, freq, duty=duty)

    def _process_triangle(self, ch, base, current_time):
        ctrl = self.apu_state[base]
        timer_lo = self.apu_state[base+2]
        timer_hi = self.apu_state[base+3]
        
        linear_counter = ctrl & 0x7F
        timer = timer_lo | ((timer_hi & 0x07) << 8)
        
        vol = 15 if linear_counter > 0 and timer > 0 else 0
        freq = 1789773.0 / (32.0 * (timer + 1)) if timer > 0 else 0
        
        if freq < 20: vol = 0
        self._emit_channel_state(ch, current_time, vol, freq)

    def _process_noise(self, ch, base, current_time):
        ctrl = self.apu_state[base]
        period_idx = self.apu_state[base+2] & 0x0F
        
        vol = ctrl & 0x0F
        periods = [4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068]
        timer = periods[period_idx]
        freq = 1789773.0 / timer
        
        self._emit_channel_state(ch, current_time, vol, freq)

    def _freq_to_note(self, freq):
        if freq <= 0: return 0
        note = 69.0 + 12.0 * math.log2(freq / 440.0)
        return note

    def _emit_channel_state(self, ch, current_time, vol, freq, duty=None):
        prev = self.prev_ch_states[ch]
        note = self._freq_to_note(freq)
        
        if vol > 0:
            if prev['vol'] == 0:
                prog = 128 # APU Pulse
                if ch == 2: prog = 129 # APU Triangle
                elif ch == 3: prog = 130 # APU Noise
                
                self.events.append({'time': current_time, 'type': 'program_change', 'program': prog, 'channel': ch})
                self.events.append({'time': current_time, 'type': 'note_on', 'note': round(note), 'velocity': int((vol / 15.0) * 127), 'channel': ch})
                prev['active_note'] = round(note)
            else:
                if prev['vol'] != vol:
                    self.events.append({'time': current_time, 'type': 'control_change', 'control': 7, 'value': int((vol / 15.0) * 127), 'channel': ch})
                if abs(prev['freq'] - freq) > 0.05:
                    active_note = prev.get('active_note', round(note))
                    diff = note - active_note
                    
                    if abs(diff) > 24:
                        self.events.append({'time': current_time, 'type': 'note_off', 'note': active_note, 'channel': ch})
                        self.events.append({'time': current_time, 'type': 'note_on', 'note': round(note), 'velocity': int((vol / 15.0) * 127), 'channel': ch})
                        prev['active_note'] = round(note)
                    else:
                        pb_val = int((diff / 24.0) * 8191)
                        pb_val = max(-8192, min(8191, pb_val))
                        self.events.append({'time': current_time, 'type': 'pitchwheel', 'pitch': pb_val, 'channel': ch})
            
            if duty is not None and duty != prev['duty']:
                self.events.append({'time': current_time, 'type': 'control_change', 'control': 14, 'value': duty, 'channel': ch})
                
        else:
            if prev['vol'] > 0:
                active_note = prev.get('active_note', round(note))
                self.events.append({'time': current_time, 'type': 'note_off', 'note': active_note, 'channel': ch})
                prev['active_note'] = None

        prev['vol'] = vol
        prev['freq'] = freq
        prev['duty'] = duty
