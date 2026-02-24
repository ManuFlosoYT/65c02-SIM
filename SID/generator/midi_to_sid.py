import sys
import argparse

from sid_constants import ALL_MODES, MODE_LEVEL_1
from midi_processor import MidiProcessor
from bytecode_generator import BytecodeGenerator
from asm_writer import write_asm


def main():
    parser = argparse.ArgumentParser(description="MIDI to SID Converter")
    parser.add_argument("input", help="Input MIDI file")
    parser.add_argument("--mode", choices=ALL_MODES, default=MODE_LEVEL_1,
                        help="Optimization Level (l1-l8)")

    args = parser.parse_args()

    processor = MidiProcessor(args.input, args.mode)
    events = processor.process()

    generator = BytecodeGenerator(events, args.mode, processor.bass_channel, processor.melody_channel)
    bytecode = generator.generate()

    write_asm(bytecode, args.input, args.mode)


if __name__ == "__main__":
    main()
