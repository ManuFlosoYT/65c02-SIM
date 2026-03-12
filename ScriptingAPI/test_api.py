import emu
import sys

def run_test():
    print("[Scripting API] Starting Functionality Test...")
    
    # Pause the emulator to ensure it doesn't execute instructions while we prepare the environment
    print("[Scripting API] Pausing emulator...")
    emu.pause()

    # Inject data into Memory
    # We are going to write a small sequence of bytes in the zero page (0x0000)
    # For example: LDA #$42 (A9 42) and STA $0200 (8D 00 02)
    address = 0x0000
    program = [0xA9, 0x42, 0x8D, 0x00, 0x02, 0x00] # 0x00 = BRK (Stop)
    
    print(f"[Scripting API] Writing {len(program)} bytes at {hex(address)}...")
    for i, byte_val in enumerate(program):
        emu.write_mem(address + i, byte_val)

    # Read from Memory to verify
    print(f"[Scripting API] Verifying written memory...")
    for i, byte_val in enumerate(program):
        read_val = emu.read_mem(address + i)
        if read_val != byte_val:
            print(f"[Error] Memory discrepancy at {hex(address+i)}: Expected {hex(byte_val)}, Read {hex(read_val)}")
        else:
            print(f"  -> {hex(address+i)}: {hex(read_val)} OK")

    print("[Scripting API] Resuming emulator for a few moments...")
    emu.resume()

    # We instruct the Python Virtual Machine to wait until the 6502 emulator
    # advances 100 cycles internally.
    wait_cycles_count = 100
    print(f"[Scripting API] Waiting {wait_cycles_count} cycles...")
    emu.wait_cycles(wait_cycles_count)

    print("[Scripting API] Test successfully completed. Script finished.")
    emu.pause()

run_test()

# You can launch this test from the CLI in interactive mode (or in headless mode) by running:
# ./SIM_65C02 --headless --script ../ScriptingAPI/test_api.py --run-cycles 200
