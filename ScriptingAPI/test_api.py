import emu
import sys

def test_registers():
    print("[Test] Registers Getters/Setters...")
    emu.set_a(0x42)
    emu.set_x(0x13)
    emu.set_y(0x37)
    emu.set_pc(0x8000)
    emu.set_sp(0xFF)
    
    assert emu.get_a() == 0x42
    assert emu.get_x() == 0x13
    assert emu.get_y() == 0x37
    assert emu.get_pc() == 0x8000
    assert emu.get_sp() == 0xFF
    print("  -> Registers OK")

def test_execution():
    print("[Test] Execution Control...")
    emu.reset()
    emu.set_pc(0x0000)
    emu.write_mem(0x0000, 0xEA) # NOP
    emu.write_mem(0x0001, 0xEA) # NOP
    
    initial_pc = emu.get_pc()
    emu.step_instruction()
    assert emu.get_pc() > initial_pc
    
    emu.wait_instructions(5)
    print("  -> Execution OK")

def test_breakpoints():
    print("[Test] Breakpoints...")
    emu.clear_breakpoints()
    
    # Complex breakpoint: Break if A == 0x99 AND PC == 0x1000
    bp_config = {
        "label": "Test BP",
        "enabled": True,
        "compoundOp": emu.LOGIC_AND,
        "conditions": [
            {
                "type": emu.BP_TYPE_REG,
                "reg": emu.REG_A,
                "op": emu.OP_EQUAL,
                "value": 0x99
            },
            {
                "type": emu.BP_TYPE_PC,
                "address": 0x1000
            }
        ]
    }
    
    id = emu.add_breakpoint(bp_config)
    print(f"  -> Added BP with ID: {id}")
    
    bps = emu.list_breakpoints()
    assert len(bps) > 0
    assert bps[0]["label"] == "Test BP"
    assert len(bps[0]["conditions"]) == 2
    
    emu.remove_breakpoint(id)
    assert len(emu.list_breakpoints()) == 0
    print("  -> Breakpoints OK")

def test_hardware():
    print("[Test] Hardware Toggles...")
    emu.set_gpu_enabled(True)
    emu.set_cycle_accurate(True)
    emu.set_target_ips(2000000)
    
    # Simple check if current total cycles works
    cycles = emu.get_cycles()
    print(f"  -> Total Cycles: {cycles}")
    print("  -> Hardware OK")

def run_all_tests():
    print("========================================")
    print("   65c02-SIM Expanded API Test Suite    ")
    print("========================================")
    emu.pause()
    test_registers()
    test_execution()
    test_breakpoints()
    test_hardware()
    print("========================================")
    print("          All Tests Passed!             ")
    print("========================================")

run_all_tests()

# You can launch this test from the CLI in interactive mode (or in headless mode) by running:
# ./SIM_65C02 --headless --script ../ScriptingAPI/test_api.py --run-cycles 200
