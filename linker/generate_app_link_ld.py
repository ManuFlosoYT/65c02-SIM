#!/usr/bin/env python3
"""
generate_app_link_ld.py — Linker script generator for microDOS .app binaries (LLVM-MOS).
"""
import sys
import argparse

APP_BASE    = 0x0200
APP_END     = 0x47FF
BSS_START   = 0x6010
BSS_END     = 0x7BFF
ZP_APP_BASE = 0x00
ZP_APP_END  = 0x5F

JUMPTABLE = 0xFF90

def generate_app_link_ld():
    ld = []
    ld.append("/* Auto-generated link.ld for microDOS apps with LLVM-MOS */")
    ld.append("")
    ld.append("__rc0 = 0x00;")
    ld.append("INCLUDE imag-regs.ld")
    ld.append("ASSERT(__rc0 == 0x00, \"Error en mapeo de Zero Page.\")")
    ld.append(f"ASSERT(__rc31 <= 0x{ZP_APP_END:02X}, \"Registros virtuales exceden Zero Page de app.\")")
    ld.append("")
    ld.append("MEMORY {")
    ld.append(f"    zp   (rw) : ORIGIN = __rc31 + 1, LENGTH = 0x{ZP_APP_END + 1:02X} - (__rc31 + 1)")
    ld.append(f"    core (rx) : ORIGIN = 0x{APP_BASE:04X},   LENGTH = 0x{APP_END - APP_BASE + 1:04X}")
    ld.append(f"    bss  (rw) : ORIGIN = 0x{BSS_START:04X},  LENGTH = 0x{BSS_END - BSS_START + 1:04X}")
    ld.append("}")
    ld.append("")
    ld.append("REGION_ALIAS(\"c_readonly\", core)")
    ld.append("REGION_ALIAS(\"c_writeable\", bss)")
    ld.append("")
    ld.append("SECTIONS {")
    ld.append(f"    .header 0x{APP_BASE:04X} : {{")
    ld.append("        KEEP(*(SORT_BY_NAME(.header*)))")
    ld.append("        KEEP(*(.header))")
    ld.append("    } > core")
    ld.append("")
    ld.append("    INCLUDE c.ld")
    ld.append(f"    PROVIDE(__CORE_SIZE__ = LOADADDR(.data) + SIZEOF(.data) - 0x{APP_BASE:04X});")
    ld.append("    PROVIDE(_CORE_SIZE__ = __CORE_SIZE__);")
    ld.append("}")
    ld.append("")
    ld.append(f"__stack = 0x{BSS_END:04X};")
    ld.append("")
    ld.append("/* OS ABI Jump Table Symbols */")
    
    symbols = [
        ("print_str", 0x00),
        ("println", 0x03),
        ("print_num", 0x06),
        ("print_hex_byte", 0x09),
        ("bios_getchar", 0x0C),
        ("bios_putchar", 0x0F),
        ("sd_open", 0x12),
        ("sd_read", 0x15),
        ("sd_write", 0x18),
        ("sd_close", 0x1B),
        ("sd_puts", 0x1E),
        ("sd_getc", 0x21),
        ("sd_getcwd", 0x24),
        ("sd_chdir", 0x27),
        ("net_send", 0x2A),
        ("net_cmd", 0x2D),
        ("net_send_num", 0x30),
        ("os_load_app_page", 0x33),
        ("sid_write", 0x36),
        ("sid_reset", 0x39),
        ("sd_opendir", 0x3C),
        ("sd_readdir", 0x3F),
        ("sd_closedir", 0x42),
        ("sd_mkdir", 0x45),
        ("sd_remove", 0x48),
        ("sd_exists", 0x4B),
        ("net_wifi", 0x4E),
        ("os_swap_save", 0x51),
        ("os_swap_load", 0x54),
        ("os_alloc", 0x57),
        ("os_free", 0x5A),
        ("bios_getchar_nb", 0x5D)
    ]
    
    for sym, offset in symbols:
        addr = JUMPTABLE + offset
        ld.append(f"PROVIDE({sym} = 0x{addr:04X});")
        ld.append(f"PROVIDE(_{sym} = 0x{addr:04X});")
        
    ld.append("")
    ld.append("OUTPUT_FORMAT {")
    ld.append("    TRIM(core)")
    ld.append("}")
    ld.append("")
    
    return "\n".join(ld)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate linker script for microDOS apps with LLVM-MOS.")
    args = parser.parse_args()
    print(generate_app_link_ld())

