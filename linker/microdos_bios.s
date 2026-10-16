// microdos_bios.s — BIOS routines and ABI Jump Table for microDOS (LLVM-MOS / GNU Assembler)

.globl MONRDKEY, MONCOUT, LOAD, SAVE, INIT_BUFFER, IRQ_HANDLER, MONPEEK, MONGETCHAR_NB
.globl _MONRDKEY, _MONCOUT, _LOAD, _SAVE, _INIT_BUFFER, _MONGETCHAR_NB, _IRQ_HANDLER
.globl bios_putchar, bios_getchar, bios_getchar_nb, BIOS_JUMPTABLE

MONRDKEY = MONRDKEY_IMPL
_MONRDKEY = MONRDKEY_IMPL
bios_getchar = MONRDKEY_IMPL

MONGETCHAR_NB = MONGETCHAR_NB_IMPL
_MONGETCHAR_NB = MONGETCHAR_NB_IMPL
bios_getchar_nb = MONGETCHAR_NB_IMPL

MONCOUT = MONCOUT_IMPL
_MONCOUT = MONCOUT_IMPL
bios_putchar = MONCOUT_IMPL

LOAD = LOAD_IMPL
_LOAD = LOAD_IMPL

SAVE = SAVE_IMPL
_SAVE = SAVE_IMPL

INIT_BUFFER = INIT_BUFFER_IMPL
_INIT_BUFFER = INIT_BUFFER_IMPL

IRQ_HANDLER = IRQ_HANDLER_IMPL
_IRQ_HANDLER = IRQ_HANDLER_IMPL

// Constants
CONFIG_2A = 1
CONFIG_SCRTCH_ORDER = 2
STACK_TOP = 0xFA
WIDTH = 40
WIDTH2 = 30
RAMSTART2 = 0x0400

READ_PTR  = 0xFC
WRITE_PTR = 0xFD

ACIA_DATA   = 0x5000
ACIA_STATUS = 0x5001
ACIA_CMD    = 0x5002
ACIA_CTRL   = 0x5003
PORTA       = 0x6001
DDRA        = 0x6003

.section .input_buffer, "aw", @nobits
.globl INPUT_BUFFER
INPUT_BUFFER: .fill 0x100

.section .text, "ax"

LOAD_IMPL:
    rts

SAVE_IMPL:
    rts

MONRDKEY_IMPL:
.globl CHRIN
CHRIN:
    phx
.Lwait_key:
    jsr BUFFER_SIZE
    beq .Lwait_key
    jsr READ_BUFFER
    pha
    cmp #8
    beq .Lskip_echo
    cmp #127
    beq .Lskip_echo
    jsr CHROUT
.Lskip_echo:
    jsr BUFFER_SIZE
    cmp #0xB0
    bcs .Lmostly_full
    lda #0xFE
    and PORTA
    sta PORTA
.Lmostly_full:
    pla
    plx
    sec
    rts

MONGETCHAR_NB_IMPL:
    jsr BUFFER_SIZE
    beq .Lempty
    jmp MONRDKEY_IMPL
.Lempty:
    lda #0
    ldx #0
    clc
    rts

MONPEEK:
    phx
    jsr BUFFER_SIZE
    beq .Lno_keyp
    ldx READ_PTR
    lda INPUT_BUFFER,x
    plx
    sec
    rts
.Lno_keyp:
    plx
    clc
    rts

MONCOUT_IMPL:
.globl CHROUT
CHROUT:
    pha
    sta ACIA_DATA
    lda #0xFF
.Ltxdelay:
    dec
    bne .Ltxdelay
    pla
    rts

INIT_BUFFER_IMPL:
    lda READ_PTR
    sta WRITE_PTR
    lda #0x01
    sta DDRA
    lda #0xFE
    and PORTA
    sta PORTA
    lda #0x1F
    sta ACIA_CTRL
    lda #0x01
    sta ACIA_CMD
    rts

WRITE_BUFFER:
    ldx WRITE_PTR
    sta INPUT_BUFFER,x
    inc WRITE_PTR
    rts

READ_BUFFER:
    ldx READ_PTR
    lda INPUT_BUFFER,x
    inc READ_PTR
    rts

BUFFER_SIZE:
    lda WRITE_PTR
    sec
    sbc READ_PTR
    rts

IRQ_HANDLER_IMPL:
    pha
    phx
    lda ACIA_STATUS
    bpl .Lnot_acia
    and #0x08
    beq .Lnot_acia
    lda ACIA_DATA
    jsr WRITE_BUFFER
    jsr BUFFER_SIZE
    cmp #0xF0
    bcc .Lnot_full
    lda #0x01
    ora PORTA
    sta PORTA
.Lnot_full:
.Lnot_acia:
    plx
    pla
    rti

.section .init.50, "ax", @progbits
    cld
    ldx #0xff
    txs
    cli

.section .after_main, "ax", @progbits
.Lhang:
    stp
    jmp .Lhang

// ABI Jump Table for microDOS apps (at $FF90)
.section .jumptable, "ax"
BIOS_JUMPTABLE:
    jmp print_str       // $FF90
    jmp println         // $FF93
    jmp print_num       // $FF96
    jmp print_hex_byte  // $FF99
    jmp MONRDKEY        // $FF9C
    jmp MONCOUT         // $FF9F
    jmp sd_open         // $FFA2
    jmp sd_read         // $FFA5
    jmp sd_write        // $FFA8
    jmp sd_close        // $FFAB
    jmp sd_puts         // $FFAE
    jmp sd_getc         // $FFB1
    jmp sd_getcwd       // $FFB4
    jmp sd_chdir        // $FFB7
    jmp net_send        // $FFBA
    jmp net_cmd         // $FFBD
    jmp net_send_num    // $FFC0
    jmp os_load_app_page // $FFC3
    jmp sid_write       // $FFC6
    jmp sid_reset       // $FFC9
    jmp sd_opendir      // $FFCC
    jmp sd_readdir      // $FFCF
    jmp sd_closedir     // $FFD2
    jmp sd_mkdir        // $FFD5
    jmp sd_remove       // $FFD8
    jmp sd_exists       // $FFDB
    jmp net_wifi        // $FFDE
    jmp os_swap_save    // $FFE1
    jmp os_swap_load    // $FFE4
    jmp os_alloc        // $FFE7
    jmp os_free         // $FFEA
    jmp MONGETCHAR_NB   // $FFED
