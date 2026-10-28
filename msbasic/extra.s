.segment "EXTRA"

microdos_sp:    .res 1
microdos_ret:   .res 2
saved_zp:       .res 32

BASIC_EXIT:
        jsr     $FFC9
        ldx     #0
@rest_zp:
        lda     saved_zp,x
        sta     $00,x
        inx
        cpx     #32
        bne     @rest_zp
        ldx     microdos_sp
        txs
        lda     microdos_ret+1
        pha
        lda     microdos_ret
        pha
        lda     #0
        ldx     #0
        rts

ISCNTC:
        jsr     $FFED
        bcc     @not_cntc
        cmp     #3
        bne     @not_cntc
        jmp     STOP
@not_cntc:
        rts
