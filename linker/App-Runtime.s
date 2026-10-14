// App-Runtime.s — microDOS app header and runtime for LLVM-MOS

.section .header, "a"
.globl app_header
app_header:
    .ascii "uDOS"
    .byte 3
    .byte 1
    .short _start
    .short 0x4600
    .short 0x2000
    .short 0x2800
    .short 0
