import sys
import argparse

from sid_constants import ALL_MODES, MODE_LEVEL_1
from midi_processor import MidiProcessor
from nsf_processor import NsfProcessor
from bytecode_generator import BytecodeGenerator
from asm_writer import write_asm
from bin_writer import write_bin

def main():
    parser = argparse.ArgumentParser(description="Audio to SID Converter (MIDI & NSF)")
    parser.add_argument("input", help="Input MIDI or NSF file")
    parser.add_argument("--mode", choices=ALL_MODES, default=MODE_LEVEL_1,
                        help="Optimization Level (l1-l8)")
    parser.add_argument("--microDOS", action='store_true', help="Output raw execution bytes instead of ASM")
    parser.add_argument("--duration", type=int, default=180, help="Max duration in seconds for infinite NSF loops")

    args = parser.parse_args()

    ext = args.input.lower().split('.')[-1]
    if ext == 'nsf':
        processor = NsfProcessor(args.input, args.mode, duration=args.duration)
    else:
        processor = MidiProcessor(args.input, args.mode)
    events = processor.process()

    noise_channel = getattr(processor, 'noise_channel', -1)
    generator = BytecodeGenerator(events, args.mode, processor.bass_channel, processor.melody_channel, noise_channel)
    bytecode = generator.generate()

    if args.microDOS:
        write_bin(bytecode, args.input)
    else:
        write_asm(bytecode, args.input, args.mode)


if __name__ == "__main__":
    main()
