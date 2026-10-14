import os
import sys

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if _SCRIPT_DIR not in sys.path:
    sys.path.insert(0, _SCRIPT_DIR)

from sid_codec import encode_semantic_v2, compress_sidlz, extract_pause_events, verify_lossless
from stream_scheduler import schedule_stream_reads, SD_DEFAULT_COST, DEFAULT_TARGET_BUFS
from asm_writer import calculate_duration


def write_bin(bytecode, input_path, target_bufs=DEFAULT_TARGET_BUFS, sd_cost=SD_DEFAULT_COST, system="NTSC", verify=True):
    """
    Encodes and writes raw SID bytecode into a compressed SDZ v2 .sid container.

    Args:
        bytecode: list of integer byte values (IR v1)
        input_path: original MIDI or NSF file path
        target_bufs: ring buffer size target in play.c (default 24)
        sd_cost: cycles per 512B SD read (default 54000)
        system: "NTSC" or "PAL"
        verify: if True, formally verifies lossless reconstruction
    """
    filename_only = os.path.splitext(os.path.basename(input_path))[0].replace(" ", "") + ".sid"
    project_root = os.path.dirname(os.path.dirname(_SCRIPT_DIR))
    subfolder = "nsf" if input_path.lower().endswith(".nsf") else "midi"
    output_dir = os.path.join(project_root, "output", subfolder)

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    output_file = os.path.join(output_dir, filename_only)

    # 1. Encode to Semantic v2
    hdr, sem = encode_semantic_v2(bytecode)

    # 2. Compress with SIDLZ
    pkg = compress_sidlz(sem)

    # 3. Formal verification
    if verify:
        verify_lossless(bytecode, hdr, pkg)

    # 4. Schedule SD reads
    pauses = extract_pause_events(hdr, sem, pkg)
    sched, stats = schedule_stream_reads(len(pkg), pauses, target_bufs=target_bufs, sd_cost=sd_cost)

    # 5. Pack container
    container = bytearray()
    container.extend(b"SDZ\x02")
    flags = 0x01 if system == "PAL" else 0x00
    container.append(flags)
    container.append(stats['min_bufs_required'])
    container.append(hdr['pw_step'])
    container.append(hdr['ntbl_first'])
    container.append(hdr['ntbl_count'])
    for f in hdr['ntbl']:
        container.append(f & 0xFF)
        container.append((f >> 8) & 0xFF)
    container.append(len(hdr['dict']))
    for d in hdr['dict']:
        container.append(d & 0xFF)
        container.append((d >> 8) & 0xFF)
    container.append(len(sched) & 0xFF)
    container.append((len(sched) >> 8) & 0xFF)
    container.extend(sched)
    container.extend(pkg)

    print(f"Writing compressed SID v2 binary to {output_file}...")
    with open(output_file, "wb") as f:
        f.write(container)

    orig_size = len(bytecode)
    final_size = len(container)
    ratio = (final_size / orig_size * 100.0) if orig_size > 0 else 0
    mins, secs, total_sec = calculate_duration(bytecode, system)
    dur_str = f"{mins}:{secs:02d}"

    print(f"Generated {output_file}")
    print(f"Song Duration:   {dur_str} ({total_sec:.1f}s)")
    print(f"IR bytecode:     {orig_size} bytes ({orig_size / 1024.0:.1f} KB)")
    print(f"Semantic stream: {len(sem)} bytes ({len(sem) / 1024.0:.1f} KB)")
    print(f"Compressed pkg:  {len(pkg)} bytes ({len(pkg) / 1024.0:.1f} KB)")
    print(f"microDOS .sid:   {final_size} bytes ({final_size / 1024.0:.1f} KB) — {ratio:.1f}% of IR ({100.0 - ratio:.1f}% reduction)")
    print(f"SD schedule:     {stats['total_reads']} reads ({stats['pre_roll_blocks']} pre-roll, {stats['planned_read_events']} planned, {stats['forced_reads']} forced). Buffer budget: {stats['min_bufs_required']} bufs")

    return output_file
