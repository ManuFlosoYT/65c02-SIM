"""
sid_codec.py - Semantic v2 encoding and SIDLZ compression for SID bytecode.
Provides lossless compression with formal round-trip verification.
"""

import collections
from sid_constants import freq_for_note, CLOCK_NTSC

def get_note_maps(clock=CLOCK_NTSC):
    note_map = {}
    freq_to_note = {}
    for _n in range(128):
        _f = freq_for_note(_n, clock)
        if _f <= 0xFFFF:
            note_map[_n] = _f
            freq_to_note[(_f & 0xFF, _f >> 8)] = _n
    return note_map, freq_to_note

NOTE_MAP, FREQ_TO_NOTE = get_note_maps(CLOCK_NTSC)

OP_REG_MAX   = 0x18
OP_NOTE      = 0x19  # 0x19..0x1B: NOTE v (note, ctrl)
OP_NOTE_SAME = 0x1C  # 0x1C..0x1E: NOTE_SAME v (note)
OP_NOTE_RAW  = 0x1F  # 0x1F..0x21: NOTE_RAW v (flo, fhi, ctrl)
OP_GATE_OFF  = 0x22  # 0x22..0x24: GATE_OFF v
OP_PW_UP     = 0x25  # 0x25..0x27: PW_UP v
OP_PW_DN     = 0x28  # 0x28..0x2A: PW_DN v
OP_PW_SET    = 0x2B  # 0x2B..0x2D: PW_SET v (lo, hi)
OP_FREQ_SET  = 0x2E  # 0x2E..0x30: FREQ_SET v (lo, hi)
OP_FREQ_NOTE = 0x31  # 0x31..0x33: FREQ_NOTE v (note)
OP_DELAY_B   = 0x34  # DELAY_B (loops_8)
OP_DELAY_W   = 0x35  # DELAY_W (lo, hi)
OP_DELAY_D   = 0x80  # 0x80..0xFE: DELAY_D (dict_idx)
OP_END       = 0xFF

DEFAULT_PW_STEP = 128


def parse_ir(ir_bytes):
    """Parses IR v1 bytecode into typed operations."""
    ops = []
    i = 0
    n = len(ir_bytes)
    while i < n:
        cmd = ir_bytes[i]
        if cmd == 0xFF:
            ops.append(('end',))
            break
        elif cmd < 0x20:
            if i + 1 >= n: break
            ops.append(('reg', cmd, ir_bytes[i + 1]))
            i += 2
        elif 0x90 <= cmd <= 0x92:
            if i + 3 >= n: break
            voice = cmd - 0x90
            ops.append(('note_on', voice, ir_bytes[i + 1], ir_bytes[i + 2], ir_bytes[i + 3]))
            i += 4
        elif 0x93 <= cmd <= 0x95:
            voice = cmd - 0x93
            ops.append(('note_off', voice))
            i += 1
        elif cmd == 0x81:
            if i + 2 >= n: break
            loops = ir_bytes[i + 1] | (ir_bytes[i + 2] << 8)
            ops.append(('delay', loops))
            i += 3
        elif cmd == 0x82:
            if i + 1 >= n: break
            ops.append(('delay', ir_bytes[i + 1]))
            i += 2
        else:
            ops.append(('raw', cmd))
            i += 1
    return ops


def encode_semantic_v2(ir_bytes, pw_step=DEFAULT_PW_STEP, clock=CLOCK_NTSC, sid_clock=None):
    """
    Encodes IR v1 bytecode into Semantic v2 format.
    Returns:
        header_dict: {
            'pw_step': int,
            'ntbl_first': int,
            'ntbl_count': int,
            'ntbl': [(lo, hi), ...],
            'dict': [delay_loops, ...]
        }
        semantic_bytes: bytearray
    """
    if sid_clock is not None:
        clock = sid_clock
    note_map, freq_to_note = get_note_maps(clock)
    ops = parse_ir(ir_bytes)

    # 1. Identify notes and delays used
    notes_used = set()
    delay_counts = collections.Counter()

    for o in ops:
        if o[0] == 'note_on':
            flo, fhi = o[2], o[3]
            note = freq_to_note.get((flo, fhi))
            if note is not None:
                notes_used.add(note)
        elif o[0] == 'delay':
            delay_counts[o[1]] += 1

    if notes_used:
        ntbl_first = min(notes_used)
        ntbl_last = max(notes_used)
        ntbl_count = ntbl_last - ntbl_first + 1
        ntbl = [note_map.get(n, 0) for n in range(ntbl_first, ntbl_last + 1)]
    else:
        ntbl_first = 0
        ntbl_count = 0
        ntbl = []

    # Select top delays for dictionary (max 126 entries so OP_DELAY_D + idx <= 0xFE)
    dict_delays = [d for d, cnt in delay_counts.most_common(126) if cnt >= 2]
    delay_to_dict = {d: idx for idx, d in enumerate(dict_delays)}

    header = {
        'pw_step': pw_step,
        'ntbl_first': ntbl_first,
        'ntbl_count': ntbl_count,
        'ntbl': ntbl,
        'dict': dict_delays
    }

    # 2. Encode stream
    out = bytearray()
    shadow_regs = [0] * 25
    last_ctrl = [0, 0, 0]
    shadow_pw = [0x0800, 0x0800, 0x0800]

    i = 0
    num_ops = len(ops)
    while i < num_ops:
        o = ops[i]
        op_type = o[0]

        if op_type == 'delay':
            loops = o[1]
            if loops in delay_to_dict:
                out.append(OP_DELAY_D + delay_to_dict[loops])
            elif loops <= 0xFF:
                out.append(OP_DELAY_B)
                out.append(loops)
            else:
                out.append(OP_DELAY_W)
                out.append(loops & 0xFF)
                out.append((loops >> 8) & 0xFF)
            i += 1

        elif op_type == 'note_off':
            v = o[1]
            out.append(OP_GATE_OFF + v)
            base = v * 7
            shadow_regs[base + 4] &= ~0x01
            i += 1

        elif op_type == 'note_on':
            v, flo, fhi, ctrl = o[1], o[2], o[3], o[4]
            base = v * 7
            note = freq_to_note.get((flo, fhi))

            if note is not None and ntbl_first <= note < ntbl_first + ntbl_count:
                if last_ctrl[v] == ctrl:
                    out.append(OP_NOTE_SAME + v)
                    out.append(note)
                else:
                    out.append(OP_NOTE + v)
                    out.append(note)
                    out.append(ctrl)
            else:
                out.append(OP_NOTE_RAW + v)
                out.append(flo)
                out.append(fhi)
                out.append(ctrl)

            last_ctrl[v] = ctrl
            shadow_regs[base + 0] = flo
            shadow_regs[base + 1] = fhi
            shadow_regs[base + 4] = ctrl
            i += 1

        elif op_type == 'reg':
            r, val = o[1], o[2]
            v = r // 7
            off = r % 7
            nxt = ops[i + 1] if i + 1 < num_ops else None

            # Check for PW lo + hi pair
            if v < 3 and off == 2 and nxt and nxt[0] == 'reg' and nxt[1] == r + 1:
                pw_lo, pw_hi = val, nxt[2]
                new_pw = pw_lo | (pw_hi << 8)
                old_pw = shadow_pw[v]

                if new_pw == (old_pw + pw_step) & 0xFFFF:
                    out.append(OP_PW_UP + v)
                elif new_pw == (old_pw - pw_step) & 0xFFFF:
                    out.append(OP_PW_DN + v)
                else:
                    out.append(OP_PW_SET + v)
                    out.append(pw_lo)
                    out.append(pw_hi)

                shadow_pw[v] = new_pw
                shadow_regs[r] = pw_lo
                shadow_regs[r + 1] = pw_hi
                i += 2
                continue

            # Check for Frequency lo + hi pair (pitchbend / vibrato)
            if v < 3 and off == 0 and nxt and nxt[0] == 'reg' and nxt[1] == r + 1:
                flo, fhi = val, nxt[2]
                note = freq_to_note.get((flo, fhi))

                if note is not None and ntbl_first <= note < ntbl_first + ntbl_count:
                    out.append(OP_FREQ_NOTE + v)
                    out.append(note)
                else:
                    out.append(OP_FREQ_SET + v)
                    out.append(flo)
                    out.append(fhi)

                shadow_regs[r] = flo
                shadow_regs[r + 1] = fhi
                i += 2
                continue

            # Single register write
            out.append(r)
            out.append(val)
            shadow_regs[r] = val
            if v < 3 and off in (2, 3):
                base = v * 7
                shadow_pw[v] = shadow_regs[base + 2] | (shadow_regs[base + 3] << 8)
            i += 1

        elif op_type == 'end':
            out.append(OP_END)
            i += 1

        else:
            i += 1

    header['uncompressed_len'] = len(out)
    return header, out


def decode_semantic_v2(header, semantic_bytes, clock=CLOCK_NTSC, sid_clock=None):
    """
    Decodes Semantic v2 bytes back into equivalent IR v1 bytecode.
    Used for 100% lossless formal verification.
    """
    if sid_clock is not None:
        clock = sid_clock
    pw_step = header['pw_step']
    ntbl_first = header['ntbl_first']
    ntbl_count = header['ntbl_count']
    ntbl = header['ntbl']
    dict_delays = header['dict']
    note_map, _ = get_note_maps(clock)

    def get_freq(note):
        idx = note - ntbl_first
        if 0 <= idx < ntbl_count:
            return ntbl[idx]
        return note_map.get(note, 0)

    ir = bytearray()
    shadow_regs = [0] * 25
    shadow_pw = [0x0800, 0x0800, 0x0800]
    last_ctrl = [0, 0, 0]

    def emit_delay(loops):
        while loops > 0:
            c = min(loops, 0xFFFF)
            lo = c & 0xFF
            hi = (c >> 8) & 0xFF
            if c <= 0xFF:
                ir.extend([0x82, lo])
            else:
                ir.extend([0x81, lo, hi])
            loops -= c

    i = 0
    n = len(semantic_bytes)
    while i < n:
        cmd = semantic_bytes[i]
        i += 1

        if cmd == OP_END:
            ir.append(0xFF)
            break

        elif cmd <= OP_REG_MAX:
            val = semantic_bytes[i]
            i += 1
            ir.extend([cmd, val])
            shadow_regs[cmd] = val
            v = cmd // 7
            if v < 3 and cmd % 7 in (2, 3):
                b = v * 7
                shadow_pw[v] = shadow_regs[b + 2] | (shadow_regs[b + 3] << 8)

        elif 0x19 <= cmd <= 0x1B:
            v = cmd - 0x19
            note = semantic_bytes[i]
            ctrl = semantic_bytes[i + 1]
            i += 2
            f = get_freq(note)
            flo, fhi = f & 0xFF, (f >> 8) & 0xFF
            ir.extend([0x90 + v, flo, fhi, ctrl])
            last_ctrl[v] = ctrl
            b = v * 7
            shadow_regs[b + 0] = flo
            shadow_regs[b + 1] = fhi
            shadow_regs[b + 4] = ctrl

        elif 0x1C <= cmd <= 0x1E:
            v = cmd - 0x1C
            note = semantic_bytes[i]
            i += 1
            ctrl = last_ctrl[v] | 0x01
            f = get_freq(note)
            flo, fhi = f & 0xFF, (f >> 8) & 0xFF
            ir.extend([0x90 + v, flo, fhi, ctrl])
            last_ctrl[v] = ctrl
            b = v * 7
            shadow_regs[b + 0] = flo
            shadow_regs[b + 1] = fhi
            shadow_regs[b + 4] = ctrl

        elif 0x1F <= cmd <= 0x21:
            v = cmd - 0x1F
            flo = semantic_bytes[i]
            fhi = semantic_bytes[i + 1]
            ctrl = semantic_bytes[i + 2]
            i += 3
            ir.extend([0x90 + v, flo, fhi, ctrl])
            last_ctrl[v] = ctrl
            b = v * 7
            shadow_regs[b + 0] = flo
            shadow_regs[b + 1] = fhi
            shadow_regs[b + 4] = ctrl

        elif 0x22 <= cmd <= 0x24:
            v = cmd - 0x22
            ir.append(0x93 + v)
            b = v * 7
            shadow_regs[b + 4] &= ~0x01

        elif 0x25 <= cmd <= 0x27:
            v = cmd - 0x25
            new_pw = (shadow_pw[v] + pw_step) & 0xFFFF
            shadow_pw[v] = new_pw
            b = v * 7
            ir.extend([b + 2, new_pw & 0xFF, b + 3, (new_pw >> 8) & 0xFF])
            shadow_regs[b + 2] = new_pw & 0xFF
            shadow_regs[b + 3] = (new_pw >> 8) & 0xFF

        elif 0x28 <= cmd <= 0x2A:
            v = cmd - 0x28
            new_pw = (shadow_pw[v] - pw_step) & 0xFFFF
            shadow_pw[v] = new_pw
            b = v * 7
            ir.extend([b + 2, new_pw & 0xFF, b + 3, (new_pw >> 8) & 0xFF])
            shadow_regs[b + 2] = new_pw & 0xFF
            shadow_regs[b + 3] = (new_pw >> 8) & 0xFF

        elif 0x2B <= cmd <= 0x2D:
            v = cmd - 0x2B
            plo = semantic_bytes[i]
            phi = semantic_bytes[i + 1]
            i += 2
            shadow_pw[v] = plo | (phi << 8)
            b = v * 7
            ir.extend([b + 2, plo, b + 3, phi])
            shadow_regs[b + 2] = plo
            shadow_regs[b + 3] = phi

        elif 0x2E <= cmd <= 0x30:
            v = cmd - 0x2E
            flo = semantic_bytes[i]
            fhi = semantic_bytes[i + 1]
            i += 2
            b = v * 7
            ir.extend([b + 0, flo, b + 1, fhi])
            shadow_regs[b + 0] = flo
            shadow_regs[b + 1] = fhi

        elif 0x31 <= cmd <= 0x33:
            v = cmd - 0x31
            note = semantic_bytes[i]
            i += 1
            f = get_freq(note)
            flo, fhi = f & 0xFF, (f >> 8) & 0xFF
            b = v * 7
            ir.extend([b + 0, flo, b + 1, fhi])
            shadow_regs[b + 0] = flo
            shadow_regs[b + 1] = fhi

        elif cmd == OP_DELAY_B:
            loops = semantic_bytes[i]
            i += 1
            emit_delay(loops)

        elif cmd == OP_DELAY_W:
            loops = semantic_bytes[i] | (semantic_bytes[i + 1] << 8)
            i += 2
            emit_delay(loops)

        elif 0x80 <= cmd <= 0xFE:
            idx = cmd - 0x80
            if idx < len(dict_delays):
                emit_delay(dict_delays[idx])

    return list(ir)


# --- SIDLZ Compressor & Decompressor ---

class SeqBitEncoder:
    def __init__(self):
        self.stream = bytearray()
        self.cur_ctrl_idx = None
        self.bit_mask = 0

    def write_bit(self, b):
        if self.bit_mask == 0:
            self.cur_ctrl_idx = len(self.stream)
            self.stream.append(0)
            self.bit_mask = 0x80
        if b:
            self.stream[self.cur_ctrl_idx] |= self.bit_mask
        self.bit_mask >>= 1

    def write_gamma(self, val):
        bits = []
        v = val
        while v > 0:
            bits.append(v & 1)
            v >>= 1
        for _ in range(len(bits) - 1):
            self.write_bit(0)
        for b in reversed(bits):
            self.write_bit(b)

    def write_byte(self, val):
        self.stream.append(val & 0xFF)

    def finish(self):
        return bytes(self.stream)


def compress_sidlz(data, window_size=2048):
    """
    Compresses data using unified sequential SIDLZ format:
    Control bytes and payload bytes (literals, offset low) are interleaved sequentially.
    The decoder only needs a single sequential next_byte() source.
    """
    n = len(data)
    if n == 0:
        return b''

    enc = SeqBitEncoder()
    idx = collections.defaultdict(list)
    pos = 0
    lit_run = []
    last_offset = 0

    def flush_literals():
        nonlocal lit_run
        if lit_run:
            enc.write_bit(0)
            enc.write_gamma(len(lit_run))
            for b in lit_run:
                enc.write_byte(b)
            lit_run = []

    def add_hash(p):
        if p + 2 <= n:
            key = (data[p], data[p + 1])
            idx[key].append(p)

    while pos < n:
        rep_len = 0
        if last_offset > 0 and pos >= last_offset:
            ref = pos - last_offset
            while pos + rep_len < n and rep_len < 255 and data[ref + rep_len] == data[pos + rep_len]:
                rep_len += 1

        best_len, best_off = 0, 0
        if pos + 2 <= n:
            key = (data[pos], data[pos + 1])
            matches = idx.get(key)
            if matches:
                for p in reversed(matches[-48:]):
                    off = pos - p
                    if off > window_size:
                        break
                    l = 0
                    while pos + l < n and l < 255 and data[p + l] == data[pos + l]:
                        l += 1
                    if l > best_len:
                        best_len, best_off = l, off

        if rep_len >= 2 and (rep_len >= best_len or best_len - rep_len <= 1):
            flush_literals()
            enc.write_bit(1)
            enc.write_bit(1)
            enc.write_gamma(rep_len - 1)
            for q in range(pos, pos + rep_len):
                add_hash(q)
            pos += rep_len
        elif best_len >= 2:
            flush_literals()
            enc.write_bit(1)
            enc.write_bit(0)
            off_val = best_off - 1
            enc.write_gamma((off_val >> 8) + 1)
            enc.write_byte(off_val & 0xFF)
            enc.write_gamma(best_len - 1)
            last_offset = best_off
            for q in range(pos, pos + best_len):
                add_hash(q)
            pos += best_len
        else:
            lit_run.append(data[pos])
            add_hash(pos)
            pos += 1
            if len(lit_run) >= 255:
                flush_literals()

    flush_literals()
    return enc.finish()


class SeqBitDecoder:
    def __init__(self, stream_bytes):
        self.stream = stream_bytes
        self.ptr = 0
        self.bit_buf = 0
        self.bit_mask = 0

    def next_byte(self):
        if self.ptr < len(self.stream):
            b = self.stream[self.ptr]
            self.ptr += 1
            return b
        return 0

    def read_bit(self):
        if self.bit_mask == 0:
            self.bit_buf = self.next_byte()
            self.bit_mask = 0x80
        b = 1 if (self.bit_buf & self.bit_mask) else 0
        self.bit_mask >>= 1
        return b

    def read_gamma(self):
        zeros = 0
        while self.read_bit() == 0:
            zeros += 1
            if zeros > 16:
                return 1
        val = 1
        for _ in range(zeros):
            val = (val << 1) | self.read_bit()
        return val


def decompress_sidlz(stream_bytes, expected_uncompressed_len=None):
    """Decompresses unified sequential SIDLZ stream."""
    if not stream_bytes:
        return b''

    dec = SeqBitDecoder(stream_bytes)
    out = bytearray()
    last_offset = 0

    def op_operand_count(cmd):
        if cmd <= OP_REG_MAX: return 1
        if 0x19 <= cmd <= 0x1B: return 2
        if 0x1C <= cmd <= 0x1E: return 1
        if 0x1F <= cmd <= 0x21: return 3
        if 0x22 <= cmd <= 0x2A: return 0
        if 0x2B <= cmd <= 0x30: return 2
        if 0x31 <= cmd <= 0x33: return 1
        if cmd == OP_DELAY_B: return 1
        if cmd == OP_DELAY_W: return 2
        if 0x80 <= cmd <= 0xFE: return 0
        return 0

    bytes_left_in_op = 0
    end_reached = False

    while dec.ptr < len(stream_bytes) or dec.bit_mask != 0:
        if expected_uncompressed_len is not None and len(out) >= expected_uncompressed_len:
            break
        if end_reached:
            break

        tag = dec.read_bit()
        if tag == 0:
            lit_len = dec.read_gamma()
            for _ in range(lit_len):
                b = dec.next_byte()
                out.append(b)
                if expected_uncompressed_len is None:
                    if bytes_left_in_op == 0:
                        if b == OP_END:
                            end_reached = True
                            break
                        bytes_left_in_op = op_operand_count(b)
                    else:
                        bytes_left_in_op -= 1
                elif len(out) >= expected_uncompressed_len:
                    break
        else:
            sub = dec.read_bit()
            if sub == 0:
                off_hi = dec.read_gamma() - 1
                off_lo = dec.next_byte()
                offset = ((off_hi << 8) | off_lo) + 1
                match_len = dec.read_gamma() + 1
                last_offset = offset
            else:
                offset = last_offset
                match_len = dec.read_gamma() + 1

            for _ in range(match_len):
                b = out[-offset] if len(out) >= offset else 0
                out.append(b)
                if expected_uncompressed_len is None:
                    if bytes_left_in_op == 0:
                        if b == OP_END:
                            end_reached = True
                            break
                        bytes_left_in_op = op_operand_count(b)
                    else:
                        bytes_left_in_op -= 1
                elif len(out) >= expected_uncompressed_len:
                    break

    return bytes(out)


def extract_pause_events(header, semantic_bytes, compressed_pkg):
    """
    Extracts pause events with exact duration (cycles) and consumed compressed bytes.
    Used by stream_scheduler.py.
    """
    if not compressed_pkg:
        return []

    dec = SeqBitDecoder(compressed_pkg)
    consumption_map = []
    out = bytearray()
    last_offset = 0

    while (dec.ptr < len(compressed_pkg) or dec.bit_mask != 0) and len(out) < len(semantic_bytes):
        tag = dec.read_bit()
        if tag == 0:
            lit_len = dec.read_gamma()
            for _ in range(lit_len):
                out.append(dec.next_byte())
                consumption_map.append(dec.ptr)
                if len(out) >= len(semantic_bytes):
                    break
        else:
            sub = dec.read_bit()
            if sub == 0:
                off_hi = dec.read_gamma() - 1
                off_lo = dec.next_byte()
                offset = ((off_hi << 8) | off_lo) + 1
                match_len = dec.read_gamma() + 1
                last_offset = offset
            else:
                offset = last_offset
                match_len = dec.read_gamma() + 1

            for _ in range(match_len):
                if len(out) >= offset:
                    out.append(out[-offset])
                else:
                    out.append(0)
                consumption_map.append(dec.ptr)
                if len(out) >= len(semantic_bytes):
                    break

    pauses = []
    i = 0
    n = len(semantic_bytes)
    pause_idx = 0
    dict_delays = header.get('dict', [])

    while i < n:
        cmd = semantic_bytes[i]
        sem_pos = i
        i += 1

        if cmd == OP_END:
            break
        elif cmd <= OP_REG_MAX:
            i += 1
        elif 0x19 <= cmd <= 0x1B:
            i += 2
        elif 0x1C <= cmd <= 0x1E:
            i += 1
        elif 0x1F <= cmd <= 0x21:
            i += 3
        elif 0x22 <= cmd <= 0x24:
            pass
        elif 0x25 <= cmd <= 0x2A:
            pass
        elif 0x2B <= cmd <= 0x30:
            i += 2
        elif 0x31 <= cmd <= 0x33:
            i += 1
        elif cmd == OP_DELAY_B:
            loops = semantic_bytes[i]
            i += 1
            consumed = consumption_map[sem_pos] if sem_pos < len(consumption_map) else len(compressed_pkg)
            pauses.append((pause_idx, loops * 15, consumed))
            pause_idx += 1
        elif cmd == OP_DELAY_W:
            loops = semantic_bytes[i] | (semantic_bytes[i + 1] << 8)
            i += 2
            consumed = consumption_map[sem_pos] if sem_pos < len(consumption_map) else len(compressed_pkg)
            pauses.append((pause_idx, loops * 15, consumed))
            pause_idx += 1
        elif 0x80 <= cmd <= 0xFE:
            idx = cmd - 0x80
            loops = dict_delays[idx] if idx < len(dict_delays) else 0
            consumed = consumption_map[sem_pos] if sem_pos < len(consumption_map) else len(compressed_pkg)
            pauses.append((pause_idx, loops * 15, consumed))
            pause_idx += 1

    return pauses


def verify_lossless(ir_original, header, compressed_pkg, clock=CLOCK_NTSC, sid_clock=None):
    """
    Formal verification of lossless round-trip:
    compressed_pkg -> decompress_sidlz -> decode_semantic_v2 == ir_original
    Returns True on success, raises ValueError with details on discrepancy.
    """
    if sid_clock is not None:
        clock = sid_clock
    decompressed_sem = decompress_sidlz(compressed_pkg, expected_uncompressed_len=header.get('uncompressed_len'))
    reconstructed_ir = decode_semantic_v2(header, decompressed_sem, clock=clock)

    if list(ir_original) != reconstructed_ir:
        diff_idx = -1
        for idx, (b1, b2) in enumerate(zip(ir_original, reconstructed_ir)):
            if b1 != b2:
                diff_idx = idx
                break
        if diff_idx == -1:
            diff_idx = min(len(ir_original), len(reconstructed_ir))
        orig_slice = ir_original[max(0, diff_idx - 8):diff_idx + 8]
        recon_slice = reconstructed_ir[max(0, diff_idx - 8):diff_idx + 8]
        raise ValueError(
            f"Lossless verification FAILED at byte {diff_idx}! "
            f"Lengths: orig={len(ir_original)}, recon={len(reconstructed_ir)}. "
            f"Orig slice: {orig_slice}, Recon slice: {recon_slice}"
        )
    return True
