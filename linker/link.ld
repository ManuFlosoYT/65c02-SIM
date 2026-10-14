/* Linker script base para 65c02-SIM con LLVM-MOS */

__rc0 = 0x00;
INCLUDE imag-regs.ld
ASSERT(__rc0 == 0x00, "Error en mapeo de Zero Page.")
ASSERT(__rc31 == 0x1f, "Error en mapeo de Zero Page.")

MEMORY {
    zp           (rw) : ORIGIN = __rc31 + 1, LENGTH = 0x100 - (__rc31 + 1)
    input_buffer (rw) : ORIGIN = 0x0300,     LENGTH = 0x0100
    ram          (rw) : ORIGIN = 0x0400,     LENGTH = 0x7C00
    rom          (rx) : ORIGIN = 0x8000,     LENGTH = 0x8000
}

REGION_ALIAS("c_readonly", rom)
REGION_ALIAS("c_writeable", ram)

SECTIONS {
    .input_buffer 0x0300 (NOLOAD) : {
        KEEP(*(SORT_BY_NAME(.input_buffer*)))
        *(.input_buffer)
    } > input_buffer

    INCLUDE c.ld

    .vectors 0xFFFA : {
        SHORT(DEFINED(_nmi) ? _nmi : 0x0F00)
        SHORT(DEFINED(_start) ? _start : (DEFINED(_reset) ? _reset : 0x8000))
        SHORT(DEFINED(IRQ_HANDLER) ? IRQ_HANDLER : (DEFINED(_irq) ? _irq : 0))
    } > rom
}

__stack = 0x7FFF;

OUTPUT_FORMAT {
    FULL(rom)
}

