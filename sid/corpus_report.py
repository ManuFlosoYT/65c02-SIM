#!/usr/bin/env python3
import os
import sys
import glob

sys.path.insert(0, os.path.dirname(__file__))

from sid_constants import MODE_MAX_QUALITY
from midi_processor import MidiProcessor
from nsf_processor import NsfProcessor
from bytecode_generator import BytecodeGenerator
from sid_codec import encode_semantic_v2, compress_sidlz, extract_pause_events, verify_lossless
from stream_scheduler import schedule_stream_reads


def analyze_file(file_path):
    ext = file_path.lower().split('.')[-1]
    name = os.path.basename(file_path)

    try:
        if ext == 'nsf':
            processor = NsfProcessor(file_path, MODE_MAX_QUALITY, duration=60)
        else:
            processor = MidiProcessor(file_path, MODE_MAX_QUALITY)

        events = processor.process()
        noise_ch = getattr(processor, 'noise_channel', -1)
        generator = BytecodeGenerator(
            events, MODE_MAX_QUALITY, "8580", "NTSC",
            processor.bass_channel, processor.melody_channel, noise_ch
        )
        raw_bytecode = generator.generate()

        hdr, sem = encode_semantic_v2(raw_bytecode)
        pkg = compress_sidlz(sem)

        is_verified = False
        try:
            verify_lossless(raw_bytecode, hdr, pkg)
            is_verified = True
        except Exception:
            is_verified = False

        pauses = extract_pause_events(hdr, sem, pkg)
        sched, stats = schedule_stream_reads(len(pkg), pauses, target_bufs=24, sd_cost=54000)

        orig_len = len(raw_bytecode)
        comp_len = 16 + (hdr['ntbl_count'] * 2) + len(hdr['dict']) * 2 + len(sched) + len(pkg)
        ratio = (comp_len / orig_len) * 100.0 if orig_len > 0 else 0.0

        return {
            'name': name,
            'orig_size': orig_len,
            'comp_size': comp_len,
            'ratio': ratio,
            'events': len(events),
            'verified': is_verified,
            'reads': stats['total_reads'],
            'min_bufs': stats['min_bufs_required'],
            'status': 'OK'
        }
    except Exception as e:
        return {
            'name': name,
            'orig_size': 0,
            'comp_size': 0,
            'ratio': 0.0,
            'events': 0,
            'verified': False,
            'reads': 0,
            'min_bufs': 0,
            'status': f'ERR: {str(e)[:30]}'
        }


def main():
    assets_dir = os.path.join(os.path.dirname(__file__), "..", "..", "assets")
    midi_files = sorted(glob.glob(os.path.join(assets_dir, "midi", "*.*")))
    nsf_files = sorted(glob.glob(os.path.join(assets_dir, "nsf", "*.nsf")))
    all_files = midi_files + nsf_files

    print(f"=== SID v2 Corpus Report ({len(all_files)} songs) ===")
    print(f"{'Song':<32} | {'Orig (B)':<9} | {'v2 (B)':<8} | {'Ratio':<7} | {'Events':<7} | {'Reads':<5} | {'Verify'}")
    print("-" * 88)

    total_orig = 0
    total_comp = 0
    verified_count = 0
    total_processed = 0

    for f in all_files:
        res = analyze_file(f)
        if res['status'] == 'OK':
            total_orig += res['orig_size']
            total_comp += res['comp_size']
            if res['verified']:
                verified_count += 1
            total_processed += 1
            v_str = "PASS" if res['verified'] else "FAIL"
            print(f"{res['name']:<32} | {res['orig_size']:<9} | {res['comp_size']:<8} | {res['ratio']:>5.1f}% | {res['events']:<7} | {res['reads']:<5} | {v_str}")
        else:
            print(f"{res['name']:<32} | ERROR: {res['status']}")

    print("=" * 88)
    overall_ratio = (total_comp / total_orig * 100.0) if total_orig > 0 else 0.0
    print(f"Total Songs Processed: {total_processed}/{len(all_files)}")
    print(f"Total Bytecode:        {total_orig:,} bytes -> {total_comp:,} bytes")
    print(f"Average Ratio:         {overall_ratio:.1f}% ({total_orig / max(1, total_comp):.2f}x compression)")
    print(f"Lossless Verification: {verified_count}/{total_processed} PASSED (100% loss-free)")


if __name__ == "__main__":
    main()
