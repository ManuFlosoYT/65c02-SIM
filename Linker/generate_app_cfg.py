#!/usr/bin/env python3
"""
generate_app_cfg.py — Linker config generator for microDOS .app binaries.

Apps are position-independent flat binaries loaded into RAM by the microDOS
`run` command. They live at $0800-$3FFF (14KB) with BSS in $4000+ and the
C stack growing down from $7FFF.

Usage: python3 Linker/generate_app_cfg.py [--gpu] [--net] > app.cfg
"""
import sys
import argparse

# Fixed layout for apps
APP_BASE    = 0x1000  # Where the app loader puts the binary
APP_END     = 0x5FFF  # Max code+rodata end
BSS_START   = 0x6010  # BSS / static data starts here (safe RAM_5 area)
BSS_END     = 0x79FF  # Large BSS (6.5KB)
STACK_TOP   = 0x7BFF  # C stack area for apps
ZP_APP_BASE = 0x40    # App ZP range ($00-$3F reserved for OS)
ZP_APP_END  = 0x5F    # 32 bytes for app virtual registers

# Fixed addresses of the BIOS jump table entries (matches bios.s JUMPTABLE)
JUMPTABLE = 0xFF90

def generate_cfg(flags):
    cfg = []
    cfg.append("MEMORY {")
    cfg.append(f"    ZP:     start = ${ZP_APP_BASE:02X}, size = ${ZP_APP_END - ZP_APP_BASE + 1:02X}, type = rw, file = \"\";")
    cfg.append("")
    cfg.append(f"    APP:    start = ${APP_BASE:04X}, size = ${APP_END - APP_BASE + 1:04X}, type = rw, file = %O, fill = yes, fillval = $FF;")
    cfg.append(f"    BSS:    start = ${BSS_START:04X}, size = ${BSS_END - BSS_START + 1:04X}, type = rw, file = \"\";")
    cfg.append("}")
    cfg.append("")
    cfg.append("SEGMENTS {")
    cfg.append("    ZEROPAGE: load = ZP,  type = zp;")
    cfg.append("")
    cfg.append("    # App code and read-only data, loaded at $0800")
    cfg.append("    CODE:     load = APP, type = ro;")
    cfg.append("    RODATA:   load = APP, type = ro;")
    cfg.append("")
    cfg.append("    # Initialized data (copied from APP image to BSS area at startup)")
    cfg.append("    DATA:     load = APP, run = BSS, type = rw, define = yes;")
    cfg.append("")
    cfg.append("    # Uninitialized data")
    cfg.append("    BSS:      load = BSS, type = bss, define = yes;")
    cfg.append("")
    cfg.append("    # C constructors / init")
    cfg.append("    STARTUP:  load = APP, type = ro, optional = yes;")
    cfg.append("    ONCE:     load = APP, type = ro, optional = yes;")
    cfg.append("}")
    cfg.append("")
    cfg.append("FEATURES {")
    cfg.append("    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;")
    cfg.append("    CONDES: type = destructor,  label = __DESTRUCTOR_TABLE__,  count = __DESTRUCTOR_COUNT__,  segment = RODATA;")
    cfg.append("}")
    cfg.append("")
    cfg.append("SYMBOLS {")
    stack_start = 0x7BFF
    
    cfg.append(f"    __STACKSTART__: type = weak, value = ${stack_start:04X};")
    cfg.append("")
    cfg.append("    # OS ABI: resolve calls directly to jump table entries in ROM")

    cfg.append(f"    _print_str:      type = weak, value = ${JUMPTABLE + 0x00:04X};")
    cfg.append(f"    _println:        type = weak, value = ${JUMPTABLE + 0x03:04X};")
    cfg.append(f"    _print_num:      type = weak, value = ${JUMPTABLE + 0x06:04X};")
    cfg.append(f"    _print_hex_byte: type = weak, value = ${JUMPTABLE + 0x09:04X};")
    cfg.append(f"    _bios_getchar:   type = weak, value = ${JUMPTABLE + 0x0C:04X};")
    cfg.append(f"    _bios_putchar:   type = weak, value = ${JUMPTABLE + 0x0F:04X};")
    cfg.append(f"    _sd_open:        type = weak, value = ${JUMPTABLE + 0x12:04X};")
    cfg.append(f"    _sd_read:        type = weak, value = ${JUMPTABLE + 0x15:04X};")
    cfg.append(f"    _sd_write:       type = weak, value = ${JUMPTABLE + 0x18:04X};")
    cfg.append(f"    _sd_close:       type = weak, value = ${JUMPTABLE + 0x1B:04X};")
    cfg.append(f"    _sd_puts:        type = weak, value = ${JUMPTABLE + 0x1E:04X};")
    cfg.append(f"    _sd_getc:        type = weak, value = ${JUMPTABLE + 0x21:04X};")
    cfg.append(f"    _sd_getcwd:      type = weak, value = ${JUMPTABLE + 0x24:04X};")
    cfg.append(f"    _sd_chdir:       type = weak, value = ${JUMPTABLE + 0x27:04X};")
    cfg.append(f"    _net_send:       type = weak, value = ${JUMPTABLE + 0x2A:04X};")
    cfg.append(f"    _net_cmd:        type = weak, value = ${JUMPTABLE + 0x2D:04X};")
    cfg.append(f"    _net_send_num:   type = weak, value = ${JUMPTABLE + 0x30:04X};")
    cfg.append("}")
    cfg.append("")
    return "\n".join(cfg)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate linker config for microDOS .app binaries.")
    args = parser.parse_args()
    print(generate_cfg([]))
