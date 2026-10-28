.segment "CODE"

SOUND:
        jsr     GETBYT
        txa
        pha
        jsr     COMBYTE
        pla
        jmp     $FFC6

BEEP:
        jsr     CHRGOT
        beq     @def_beep
        jsr     FRMEVL
        jsr     MKINT
        lda     FAC+4
        ora     FAC+3
        beq     @silent
        lda     FAC+4
        tax
        lda     #$00
        jsr     $FFC6
        lda     FAC+3
        tax
        lda     #$01
        jsr     $FFC6
        jsr     CHKCOM
        jsr     GETBYT
        cpx     #0
        beq     @done
        jmp     @start_voice
@def_beep:
        lda     #$00
        ldx     #$D5
        jsr     $FFC6
        lda     #$01
        ldx     #$1C
        jsr     $FFC6
        ldx     #20
@start_voice:
        lda     #$18
        ldx     #$0F
        jsr     $FFC6
        lda     #$05
        ldx     #$00
        jsr     $FFC6
        lda     #$06
        ldx     #$F0
        jsr     $FFC6
        lda     #$02
        ldx     #$00
        jsr     $FFC6
        lda     #$03
        ldx     #$08
        jsr     $FFC6
        lda     #$04
        ldx     #$41
        jsr     $FFC6
@d1:
        ldy     #$FF
@d2:
        dey
        bne     @d2
        dex
        bne     @d1
        lda     #$04
        ldx     #$40
        jsr     $FFC6
@done:
        rts

@silent:
        jsr     CHKCOM
        jsr     GETBYT
        rts
