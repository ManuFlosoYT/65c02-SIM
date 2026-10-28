// App-Runtime.s — microDOS app header and runtime for LLVM-MOS

.section .header, "a"
.globl app_header
app_header:
    .ascii "uDOS"
    .byte 3
    .byte 1
    .short _app_startup
    .short __CORE_SIZE__
    .short 0x2000
    .short 0x2800
    .short 0

.section .text, "ax"
.globl _app_startup
_app_startup:
    lda 0x1E
    sta 0x70
    lda 0x1F
    sta 0x71

    lda #<__stack
    sta 0x1E
    lda #>__stack
    sta 0x1F

    jsr _start

    sta 0x72
    stx 0x73

    lda 0x70
    sta 0x1E
    lda 0x71
    sta 0x1F

    lda 0x72
    ldx 0x73
    rts


// Overlay runtime: state lives in .data/.bss so copy-data/zero-bss initialize it on every launch.
.section .data, "aw"
.globl ov_current_page
ov_current_page: .byte 0xFF

.section .bss, "aw", @nobits
ov_depth:   .fill 1
ov_stack:   .fill 32
ov_a:       .fill 1
ov_x:       .fill 1
ov_y:       .fill 1
ov_page:    .fill 1
ov_target:  .fill 2
ov_tmp:     .fill 1
ov_zp_save: .fill 32

// Emits a resident stub named \name that runs \target (linked in the window) from overlay \page.
.macro OVERLAY_TRAMPOLINE name, page, target
.section .text, "ax"
.globl \name
\name:
    jsr overlay_dispatcher
    .byte \page
    .byte <\target
    .byte >\target
.endm

.section .text, "ax"
.globl overlay_dispatcher
overlay_dispatcher:
    sta ov_a
    stx ov_x
    sty ov_y

    // Inline stub data lives right after the JSR operand; $63/$64 is only used before any kernel call.
    pla
    sta 0x63
    pla
    sta 0x64
    ldy #1
    lda (0x63), y
    sta ov_page
    iny
    lda (0x63), y
    sta ov_target
    iny
    lda (0x63), y
    sta ov_target+1

    // The callee's RTS lands on overlay_return_hook; the caller's address stays below it.
    lda #>(overlay_return_hook - 1)
    pha
    lda #<(overlay_return_hook - 1)
    pha

    // Depth 0 means the caller is resident code, so its page never needs restoring.
    ldx ov_depth
    cpx #32
    bcs .Lov_overflow
    lda #0xFF
    cpx #0
    beq .Lov_push
    lda ov_current_page
.Lov_push:
    sta ov_stack, x
    inc ov_depth

    lda ov_page
    cmp ov_current_page
    beq .Lov_ready
    jsr ov_load_page

.Lov_ready:
    lda ov_a
    ldx ov_x
    ldy ov_y
    jmp (ov_target)

.Lov_overflow:
    stp

.globl overlay_return_hook
overlay_return_hook:
    sta ov_a
    stx ov_x
    sty ov_y

    ldx ov_depth
    beq .Lret_done
    dex
    stx ov_depth
    lda ov_stack, x
    cmp #0xFF
    beq .Lret_done
    cmp ov_current_page
    beq .Lret_done
    sta ov_page
    jsr ov_load_page

.Lret_done:
    lda ov_a
    ldx ov_x
    ldy ov_y
    rts

// Loads ov_page into the window, preserving __rc0-__rc31 (kernel code clobbers them; __rc2/3 hold 32-bit returns).
ov_load_page:
    ldx #31
.Lov_save:
    lda 0, x
    sta ov_zp_save, x
    dex
    bpl .Lov_save

    lda ov_page
    sta ov_current_page
    jsr os_load_app_page

    ldx #31
.Lov_restore:
    lda ov_zp_save, x
    sta 0, x
    dex
    bpl .Lov_restore
    rts
