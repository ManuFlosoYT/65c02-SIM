// vga.s — VGA screen test for 65c02-SIM (LLVM-MOS / GNU Assembler syntax)

vidpage = 0x0000
start_color = 0x0002

.section .text, "ax"
.globl reset, _reset, _start
_start = reset
_reset = reset
reset:
    lda #0
    sta start_color

.Lloop:
    // initialize vidpage to beginning of video ram $2000
    lda #0x20
    sta vidpage + 1
    lda #0x00
    sta vidpage

    ldx #0x20 // X will count down how many pages of video RAM to go
    ldy #0    // populate a page starting at 0
    inc start_color
    lda start_color // color of pixel

.Lpage:
    sta (vidpage), y // write A register to address vidpage + y

    and #0x7F // if we cycled through 127 colors
    bne .Linc_color
    clc
    adc #1 // increment twice

.Linc_color:
    clc
    adc #1 // otherwise, increment pixel color value just once

    iny
    bne .Lpage

    inc vidpage + 1 // skip to the next page
    dex
    bne .Lpage      // keep going through $20 pages

    jmp .Lloop

.section .vectors, "a"
    .short reset
    .short reset
    .short reset
