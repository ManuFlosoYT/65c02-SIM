import sys
import argparse

from sid_constants import ALL_MODES, MODE_MAX_QUALITY, CLOCK_NTSC, CLOCK_PAL
from bytecode_generator import BytecodeGenerator
from asm_writer import write_asm
from bin_writer import write_bin

def main():
    parser = argparse.ArgumentParser(description="Audio to SID Converter (MIDI & NSF)")
    parser.add_argument("input", help="Input MIDI or NSF file")
    parser.add_argument("--mode", choices=ALL_MODES, default=MODE_MAX_QUALITY,
                        help="Optimization Level (max_quality, quality, balanced, compressed, max_compression)")
    parser.add_argument("--chip", choices=["6581", "8580"], default="8580",
                        help="Target SID Chip model (6581 or 8580)")
    parser.add_argument("--system", choices=["NTSC", "PAL"], default="NTSC",
                        help="Target clock system (NTSC=1MHz, PAL=985kHz)")
    parser.add_argument("--cpu-clock", type=float, default=None,
                        help="Target CPU clock frequency in Hz (e.g. 10000000 for microDOS, 1000000 otherwise)")
    parser.add_argument("--clock", type=float, default=None,
                        help="Alias for --cpu-clock")
    parser.add_argument("--sid-clock", type=float, default=None,
                        help="Target SID clock frequency in Hz (default: 1000000.0 NTSC / 985248.0 PAL)")
    parser.add_argument("--microDOS", action='store_true', help="Output raw execution bytes instead of ASM")
    parser.add_argument("--duration", type=int, default=180, help="Max duration in seconds for infinite NSF loops")
    parser.add_argument("--sd-cost", type=int, default=49731, help="Cycles per 512B SD read")
    parser.add_argument("--stream-bufs", type=int, default=16, help="Target buffer budget in microDOS play.c")
    parser.add_argument("--no-verify", action='store_true', help="Skip formal lossless roundtrip verification")
    parser.add_argument("--main-track", default=None, help="Force main melody track by index (0-based) or name")
    parser.add_argument("--melody-channel", type=int, default=None, help="Force main melody MIDI channel (0-15)")
    parser.add_argument("--bass-channel", type=int, default=None, help="Force bass MIDI channel (0-15)")

    args = parser.parse_args()

    if args.sid_clock is not None:
        sid_clock = args.sid_clock
    elif args.system == "PAL":
        sid_clock = CLOCK_PAL
    else:
        sid_clock = CLOCK_NTSC

    cpu_clock = args.cpu_clock if args.cpu_clock is not None else args.clock
    if cpu_clock is None:
        cpu_clock = 10000000.0 if args.microDOS else 1000000.0

    ext = args.input.lower().split('.')[-1]
    if ext == 'nsf':
        from nsf_processor import NsfProcessor
        processor = NsfProcessor(args.input, args.mode, duration=args.duration)
    else:
        from midi_processor import MidiProcessor
        processor = MidiProcessor(
            args.input,
            args.mode,
            main_track=args.main_track,
            melody_channel=args.melody_channel,
            bass_channel=args.bass_channel
        )
    events = processor.process()

    noise_channel = getattr(processor, 'noise_channel', -1)
    custom_wave_overrides = getattr(processor, 'custom_wave_overrides', None)
    generator = BytecodeGenerator(events, args.mode, args.chip, args.system, processor.bass_channel, processor.melody_channel, noise_channel, cpu_clock=cpu_clock, sid_clock=sid_clock, custom_wave_overrides=custom_wave_overrides)
    bytecode = generator.generate()

    if args.microDOS:
        write_bin(
            bytecode,
            args.input,
            target_bufs=args.stream_bufs,
            sd_cost=args.sd_cost,
            system=args.system,
            cpu_clock=cpu_clock,
            sid_clock=sid_clock,
            verify=not args.no_verify
        )
    else:
        _out, fits = write_asm(
            bytecode,
            args.input,
            args.mode,
            system=args.system,
            cpu_clock=cpu_clock,
            sid_clock=sid_clock,
            verify=not args.no_verify
        )
        if not fits:
            sys.exit(1)


if __name__ == "__main__":
    main()
