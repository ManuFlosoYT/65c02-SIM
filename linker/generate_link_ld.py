#!/usr/bin/env python3
import sys
import argparse

def generate_link_ld(flags):
    ram_start = 0x0400
    ram_end = 0x8000
    
    if '--microDOS' in flags:
        reserved_blocks = [
            (0x0200, 0x53FF, "App Area (Reserved)"),
            (0x5FC0, 0x7FFF, "Hardware (microDOS-GPU + MMIO)")
        ]
    else:
        reserved_blocks = [
            (0x5000, 0x5003, "ACIA"),
            (0x5008, 0x500B, "SD Card"),
            (0x6000, 0x600F, "VIA")
        ]
        
        if '--sid' in flags:
            reserved_blocks.append((0x4800, 0x481F, "SID"))
            
        if '--gpu' in flags or '--double-buffer' in flags:
            reserved_blocks.append((0x2000, 0x3FFF, "GPU VRAM Front Buffer"))
            
        if '--double-buffer' in flags:
            reserved_blocks.append((0x6010, 0x790F, "GPU VRAM Back Buffer"))
            
        if '--net' in flags:
            reserved_blocks.append((0x5004, 0x5007, "ESP8266"))
        
    reserved_blocks.sort()
    
    merged_blocks = []
    for block in reserved_blocks:
        if not merged_blocks:
            merged_blocks.append(block)
        else:
            prev_start, prev_end, prev_name = merged_blocks[-1]
            curr_start, curr_end, curr_name = block
            if curr_start <= prev_end + 1:
                merged_blocks[-1] = (prev_start, max(prev_end, curr_end), f"{prev_name} + {curr_name}")
            else:
                merged_blocks.append(block)
                
    ram_blocks = []
    current = ram_start
    for r_start, r_end, r_name in merged_blocks:
        if current < r_start:
            ram_blocks.append((current, r_start - 1))
        current = max(current, r_end + 1)
        
    if current < ram_end:
        ram_blocks.append((current, ram_end - 1))
        
    ld = []
    ld.append("/* Auto-generated link.ld for 65c02-SIM with LLVM-MOS */")
    ld.append("")
    ld.append("__rc0 = 0x00;")
    ld.append("INCLUDE imag-regs.ld")
    ld.append("ASSERT(__rc0 == 0x00, \"Error en mapeo de Zero Page.\")")
    ld.append("ASSERT(__rc31 == 0x1f, \"Error en mapeo de Zero Page.\")")
    ld.append("")
    ld.append("MEMORY {")
    ld.append("    zp           (rw) : ORIGIN = __rc31 + 1, LENGTH = 0xFC - (__rc31 + 1)")
    if '--microDOS' not in flags:
        ld.append("    input_buffer (rw) : ORIGIN = 0x0300,     LENGTH = 0x0100")
    
    ram_names = []
    for i, (start, end) in enumerate(ram_blocks):
        size = end - start + 1
        name = f"ram_{i+1}"
        ram_names.append((name, size))
        ld.append(f"    {name} (rw) : ORIGIN = 0x{start:04X}, LENGTH = 0x{size:04X}")
        for rs, re, rn in merged_blocks:
            if re >= end and rs == end + 1:
                ld.append(f"    /* 0x{rs:04X} - 0x{re:04X} reserved for {rn} */")
    
    ld.append("    rom          (rx) : ORIGIN = 0x8000,     LENGTH = 0x8000")
    ld.append("}")
    ld.append("")
    
    main_ram = ram_names[0][0]
    ld.append("REGION_ALIAS(\"c_readonly\", rom)")
    ld.append(f"REGION_ALIAS(\"c_writeable\", {main_ram})")
    ld.append("")
    ld.append("SECTIONS {")
    if '--microDOS' in flags:
        ld.append("    .input_buffer (NOLOAD) : {")
        ld.append("        KEEP(*(SORT_BY_NAME(.input_buffer*)))")
        ld.append("        *(.input_buffer)")
        ld.append(f"    }} > {main_ram}")
    else:
        ld.append("    .input_buffer 0x0300 (NOLOAD) : {")
        ld.append("        KEEP(*(SORT_BY_NAME(.input_buffer*)))")
        ld.append("        *(.input_buffer)")
        ld.append("    } > input_buffer")
    ld.append("")
    ld.append("    INCLUDE c.ld")
    ld.append("")

    if '--microDOS' in flags:
        ld.append("    .jumptable 0xFF90 : {")
        ld.append("        KEEP(*(SORT_BY_NAME(.jumptable*)))")
        ld.append("        KEEP(*(.jumptable))")
        ld.append("    } > rom")
        ld.append("")

    ld.append("    .vectors 0xFFFA : {")
    ld.append("        SHORT(DEFINED(_nmi) ? _nmi : 0x0F00)")
    ld.append("        SHORT(DEFINED(_start) ? _start : (DEFINED(_reset) ? _reset : 0x8000))")
    ld.append("        SHORT(DEFINED(IRQ_HANDLER) ? IRQ_HANDLER : (DEFINED(_irq) ? _irq : 0))")
    ld.append("    } > rom")
    ld.append("}")
    ld.append("")
    
    stack_start = 0x5FBF if '--microDOS' in flags else ram_blocks[-1][1]
    ld.append(f"__stack = 0x{stack_start:04X};")
    ld.append("")
    ld.append("OUTPUT_FORMAT {")
    ld.append("    FULL(rom)")
    ld.append("}")
    ld.append("")
    
    return "\n".join(ld)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate LLVM-MOS linker script (link.ld).')
    parser.add_argument('--gpu', action='store_true', help='Reserve GPU VRAM')
    parser.add_argument('--double-buffer', action='store_true', help='Reserve GPU Double Buffer')
    parser.add_argument('--net', action='store_true', help='Reserve ESP8266')
    parser.add_argument('--sid', action='store_true', help='Reserve SID')
    parser.add_argument('--microDOS', action='store_true', help='Reserve App memory for microDOS')
    args = parser.parse_args()
    
    flags = []
    if args.gpu: flags.append('--gpu')
    if args.double_buffer: flags.append('--double-buffer')
    if args.net: flags.append('--net')
    if args.sid: flags.append('--sid')
    if args.microDOS: flags.append('--microDOS')
    
    print(generate_link_ld(flags))
