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

