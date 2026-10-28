		.segment "HEADER"
        .byte "uDOS"
        .byte 3
        .byte 1
        .word _start
        .word 0
        .word 0
        .word 0
        .word 0

_start:
        pla
        sta microdos_ret
        pla
        sta microdos_ret+1
        tsx
        stx microdos_sp
        ldx #0
@save_zp:
        lda $00,x
        sta saved_zp,x
        inx
        cpx #32
        bne @save_zp
        jmp COLD_START
