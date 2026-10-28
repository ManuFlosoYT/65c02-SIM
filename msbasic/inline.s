.segment "CODE"

INLIN:
        ldx     #$00
INLIN2:
        jsr     GETLN
        cmp     #$0D
        beq     L2453
        cmp     #$0A
        beq     L2453
        cmp     #$08
        beq     @backspace
        cmp     #$7F
        beq     @backspace
        cmp     #$20
        bcc     INLIN2
        cpx     #$47
        bcs     @bell
        sta     INPUTBUFFER,x
        inx
        jmp     INLIN2

@backspace:
        cpx     #$00
        beq     INLIN2
        dex
        lda     #$08
        jsr     OUTDO
        lda     #$20
        jsr     OUTDO
        lda     #$08
        jsr     OUTDO
        jmp     INLIN2

@bell:
        lda     #$07
        jsr     OUTDO
        jmp     INLIN2

L2453:
        jmp     L29B9

GETLN:
        jsr     MONRDKEY
        rts

GET_UPPER:
        lda     INPUTBUFFERX,x
        cmp     #'a'
        bcc     @not_lower
        cmp     #'z'+1
        bcs     @not_lower
        sbc     #$1F
@not_lower:
        rts
