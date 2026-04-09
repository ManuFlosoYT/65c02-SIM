import sys
import argparse

def generate_cfg(flags):
    # Free space available for RAM
    ram_start = 0x0400
    ram_end = 0x8000
    
    # Base reservations
    reserved_blocks = [
        (0x5000, 0x5003, "ACIA"),
        (0x6000, 0x600F, "VIA")
    ]
    
    if '--sid' in flags:
        reserved_blocks.append((0x4800, 0x481F, "SID"))

    if '--microDOS' in flags:
        reserved_blocks.append((0x1000, 0x47FF, "App Load Area (Reserved)"))
        reserved_blocks.append((0x6010, 0x7BFF, "App RAM / BSS (Reserved)"))
        
    if '--gpu' in flags or '--double-buffer' in flags:
        reserved_blocks.append((0x2000, 0x3FFF, "GPU VRAM Front Buffer"))
        
    if '--double-buffer' in flags:
        reserved_blocks.append((0x6010, 0x790F, "GPU VRAM Back Buffer"))
        
    if '--net' in flags:
        reserved_blocks.append((0x5004, 0x5007, "ESP8266"))
        
    # Sort blocks
    reserved_blocks.sort()
    
    # Merge overlapping/adjacent blocks
    merged_blocks = []
    for block in reserved_blocks:
        if not merged_blocks:
            merged_blocks.append(block)
        else:
            prev_start, prev_end, prev_name = merged_blocks[-1]
            curr_start, curr_end, curr_name = block
            if curr_start <= prev_end + 1:
                # Merge
                merged_blocks[-1] = (prev_start, max(prev_end, curr_end), f"{prev_name} + {curr_name}")
            else:
                merged_blocks.append(block)
                
    # Calculate free RAM blocks
    ram_blocks = []
    current = ram_start
    for r_start, r_end, r_name in merged_blocks:
        if current < r_start:
            ram_blocks.append((current, r_start - 1))
        current = max(current, r_end + 1)
        
    if current < ram_end:
        ram_blocks.append((current, ram_end - 1))
        
    # Generate CFG
    cfg = []
    cfg.append("MEMORY {")
    cfg.append("    # Zero Page and Stack (Standard)")
    cfg.append("    ZP:           start = $0000, size = $0100, type = rw, define = yes, file = \"\";")
    cfg.append("")
    cfg.append("    # 0x100 - 0x1FF reserved for stack")
    cfg.append("")
    cfg.append("    INPUT_BUFFER: start = $0300, size = $0100, type = rw, file = \"\";")
    cfg.append("")
    
    # Print RAM blocks
    ram_names = []
    for i, (start, end) in enumerate(ram_blocks):
        size = end - start + 1
        name = f"RAM_{i+1}"
        ram_names.append((name, size))
        cfg.append(f"    {name}:" + " "*(12-len(name)) + f"start = ${start:04X}, size = ${size:04X}, type = rw, define = yes, file = \"\";")
        # Find next reserved block to document
        for rs, re, rn in merged_blocks:
            if re >= end and rs == end + 1:
                cfg.append(f"    # ${rs:04X} - ${re:04X} reserved for {rn}")
    
    cfg.append("")
    cfg.append("    # Main ROM (C code + BASIC)")
    cfg.append("    ROM:          start = $8000, size = $7F90, type = ro, define = yes, file = %O, fill = yes, fillval = $FF;")

    if '--microDOS' in flags:
        cfg.append("    # Fixed-address jump table for app ABI (17 slots * 3 bytes = 51 bytes)")
        cfg.append("    JUMPTABLE:    start = $FF90, size = $006A, type = ro, define = yes, file = %O, fill = yes, fillval = $FF;")
    else:
        cfg.append("    # Padding to reach vectors")
        cfg.append("    ROM_PAD:      start = $FF90, size = $006A, type = ro, file = %O, fill = yes, fillval = $FF;")
    cfg.append("    # Hardware Vectors (6502 Reset/IRQ/NMI)")
    cfg.append("    RESETVEC:     start = $FFFA, size = $0006, type = ro, define = yes, file = %O, fill = yes, fillval = $FF;")
    cfg.append("}")
    cfg.append("")
    
    cfg.append("SEGMENTS {")
    cfg.append("    # --- Standard C segments ---")
    cfg.append("    ZEROPAGE:     load = ZP,  type = zp;")
    cfg.append("    INPUT_BUFFER: load = INPUT_BUFFER, type = rw;")
    cfg.append("")
    cfg.append("    # Header at the start of ROM")
    cfg.append("    HEADER:       load = ROM, type = ro;")
    cfg.append("    ")
    
    main_ram = ram_names[0][0]
    
    cfg.append(f"    # Code copied from ROM to RAM at startup")
    cfg.append(f"    DATA:         load = ROM, run = {main_ram}, type = rw, define = yes;")
    cfg.append(f"    ")
    cfg.append(f"    # Uninitialized variables")
    cfg.append(f"    BSS:          load = {main_ram}, type = bss, define = yes;")
    cfg.append(f"    ")
    cfg.append("    # Main C code and constants")
    cfg.append("    CODE:         load = ROM, type = ro;")
    cfg.append("    RODATA:       load = ROM, type = ro;")
    cfg.append("    ")
    cfg.append("    # BIOS (I/O routines)")
    cfg.append("    BIOS:         load = ROM, type = ro;")
    cfg.append("    ")
    if '--microDOS' in flags:
        cfg.append("    # App ABI jump table at fixed address $FF90")
        cfg.append("    JUMPTABLE:    load = JUMPTABLE, type = ro;")
        cfg.append("    ")
    cfg.append("    # C constructors / initialization")
    cfg.append("    STARTUP:      load = ROM, type = ro, optional = yes;")
    cfg.append("    ONCE:         load = ROM, type = ro, optional = yes;")
    cfg.append("")
    cfg.append("    # --- Monitor and Vectors ---")
    cfg.append("    ")
    cfg.append("    # Hardware vectors.")
    cfg.append("    RESETVEC:     load = RESETVEC, type = ro, optional = yes;")
    cfg.append("}")
    cfg.append("")
    cfg.append("FEATURES {")
    cfg.append("    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;")
    cfg.append("    CONDES: type = destructor,  label = __DESTRUCTOR_TABLE__,  count = __DESTRUCTOR_COUNT__,  segment = RODATA;")
    cfg.append("}")
    cfg.append("")
    cfg.append("SYMBOLS {")
    
    # Calculate C stack start (end of the last RAM block)
    stack_start = ram_blocks[-1][1]
    
    cfg.append(f"    # C stack definition")
    cfg.append(f"    __STACKSTART__: type = weak, value = ${stack_start:04X};")
    cfg.append("}")
    cfg.append("")
    
    return "\n".join(cfg)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate C-Runtime linker config.')
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
    
    print(generate_cfg(flags))
