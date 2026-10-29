#!/usr/bin/env python3
"""
generate_app_link_ld.py — Linker script generator for microDOS .app binaries (LLVM-MOS).
"""
import sys
import argparse

ZP_APP_BASE = 0x00
ZP_APP_END  = 0x5F

APP_BASE = 0x0200
APP_END  = 0x53FF
OVERLAY_WINDOW_ADDR = 0x2800
OVERLAY_WINDOW_SIZE = 0x2000
CORE_SIZE_MAX = OVERLAY_WINDOW_ADDR - APP_BASE

EXTRA_RAM_START = 0x5FC0
EXTRA_RAM_SIZE  = 0x2000
EXTRA_RAM_END   = EXTRA_RAM_START + EXTRA_RAM_SIZE - 1

JUMPTABLE = 0xFF90

def generate_app_link_ld(has_gpu=False, num_overlays=0):
    main_region = "core_ram" if num_overlays > 0 else "ram"

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
    if num_overlays > 0:
        ld.append(f"    core_ram  (rw) : ORIGIN = 0x{APP_BASE:04X},   LENGTH = 0x{CORE_SIZE_MAX:04X}")
        for i in range(1, num_overlays + 1):
            bank_origin = (i << 16) | OVERLAY_WINDOW_ADDR
            ld.append(f"    overlay_{i} (rx) : ORIGIN = 0x{bank_origin:06X}, LENGTH = 0x{OVERLAY_WINDOW_SIZE:04X}")
    else:
        ld.append(f"    ram  (rw) : ORIGIN = 0x{APP_BASE:04X},   LENGTH = 0x{APP_END - APP_BASE + 1:04X}")
    if not has_gpu:
        ld.append(f"    extra_ram (rw) : ORIGIN = 0x{EXTRA_RAM_START:04X},   LENGTH = 0x{EXTRA_RAM_SIZE:04X}")
    ld.append("}")
    ld.append("")
    ld.append(f"REGION_ALIAS(\"c_readonly\", {main_region})")
    writeable_region = "extra_ram" if num_overlays > 0 and not has_gpu else main_region
    ld.append(f"REGION_ALIAS(\"c_writeable\", {writeable_region})")
    ld.append("EXTERN(__do_init_stack)")
    ld.append("EXTERN(__do_copy_data)")
    ld.append("EXTERN(__do_copy_zp_data)")
    ld.append("EXTERN(__do_zero_bss)")
    ld.append("EXTERN(__do_zero_zp_bss)")
    ld.append("")
    ld.append("SECTIONS {")
    ld.append(f"    .header 0x{APP_BASE:04X} : {{")
    ld.append("        KEEP(*(SORT_BY_NAME(.header*)))")
    ld.append("        KEEP(*(.header))")
    ld.append(f"    }} > {main_region}")
    ld.append("")
    ld.append("    INCLUDE c.ld")
    ld.append(f"    PROVIDE(__CORE_SIZE__ = LOADADDR(.data) + SIZEOF(.data) - 0x{APP_BASE:04X});")
    ld.append("    PROVIDE(_CORE_SIZE__ = __CORE_SIZE__);")
    ld.append("")
    if num_overlays > 0:
        for i in range(1, num_overlays + 1):
            bank_addr = (i << 16) | OVERLAY_WINDOW_ADDR
            ld.append(f"    .overlay_{i} 0x{bank_addr:06X} : {{")
            ld.append(f"        KEEP(*(SORT_BY_NAME(.overlay_{i}*)))")
            ld.append(f"        *(.overlay_{i} .overlay_{i}.*)")
            ld.append(f"        KEEP(*(SORT_BY_NAME(.overlay{i}*)))")
            ld.append(f"        *(.overlay{i} .overlay{i}.*)")
            ld.append(f"    }} > overlay_{i}")
            ld.append("")
    if not has_gpu:
        ld.append("    .extra_ram (NOLOAD) : {")
        ld.append("        KEEP(*(SORT_BY_NAME(.extra_ram*)))")
        ld.append("        *(.extra_ram)")
        ld.append("    } > extra_ram")
        ld.append("")
        ld.append(f"    PROVIDE(__extra_ram_start = 0x{EXTRA_RAM_START:04X});")
        ld.append(f"    PROVIDE(__extra_ram_end = 0x{EXTRA_RAM_END:04X});")
        ld.append(f"    PROVIDE(__extra_ram_size = 0x{EXTRA_RAM_SIZE:04X});")
        ld.append(f"    PROVIDE(__app_heap_start = DEFINED(__heap_start) ? __heap_start : 0x{EXTRA_RAM_START:04X});")
        ld.append("    PROVIDE(_app_heap_start = __app_heap_start);")
    else:
        ld.append(f"    PROVIDE(__app_heap_start = DEFINED(__heap_start) ? __heap_start : 0x{APP_BASE:04X});")
        ld.append("    PROVIDE(_app_heap_start = __app_heap_start);")
    ld.append("}")
    ld.append("")
    ld.append(f"__stack = 0x{APP_END:04X};")
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
        ("bios_getchar_nb", 0x5D),
        ("sd_seek", 0x60)
    ]

    for sym, offset in symbols:
        addr = JUMPTABLE + offset
        ld.append(f"PROVIDE({sym} = 0x{addr:04X});")
        ld.append(f"PROVIDE(_{sym} = 0x{addr:04X});")

    ld.append("")
    ld.append("OUTPUT_FORMAT {")
    ld.append(f"    TRIM({main_region})")
    if num_overlays > 0:
        for i in range(1, num_overlays + 1):
            ld.append(f"    FULL(overlay_{i})")
    ld.append("}")

    return "\n".join(ld) + "\n"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate linker script for microDOS apps with LLVM-MOS.")
    parser.add_argument('--gpu', action='store_true', help='Reserve GPU VRAM for app')
    parser.add_argument('--overlays', type=int, default=0, help='Number of 8KB overlay pages')
    args = parser.parse_args()
    print(generate_app_link_ld(args.gpu, args.overlays), end="")