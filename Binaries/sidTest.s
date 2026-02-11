
.segment "CODE"

reset:
  ldx #$FF
  txs
  cli

  ; Initialize SID
  lda #$0F
  sta $4818 ; Volume Max (15)

  ; Voice 1 ADSR Setup
  lda #$09  ; Attack 2ms, Decay 750ms
  sta $4805
  lda #$00  ; Sustain 0, Release 6ms
  sta $4806
  
  ldx #0    ; Index for notes

play_loop:
  lda notes_lo, x
  sta $4800 ; Freq Lo
  lda notes_hi, x
  sta $4801 ; Freq Hi
  
  ; Gate On + Triangle
  lda #$11
  sta $4804 

  ; Delay
  jsr delay

  ; Gate Off + Triangle (Start Release)
  lda #$10
  sta $4804
  
  ; Short delay between notes
  jsr short_delay

  inx
  cpx #8    ; Number of notes
  bne play_loop
  
  ldx #0    ; Restart loop
  jmp play_loop

delay:
  ldy #$FF
d1:
  lda #$FF
d2:
  sec
  sbc #1
  bne d2
  dey
  bne d1
  rts

short_delay:
  ldy #$40
sd1:
  lda #$FF
sd2:
  sec
  sbc #1
  bne sd2
  dey
  bne sd1
  rts

notes_lo:
  .byte $25, $4B, $75, $A0, $CD, $FB, $2C, $5F ; C, D, E, F, G, A, B, C (approx)
notes_hi:
  .byte $11, $13, $15, $16, $19, $1C, $20, $22


; Reset/IRQ/NMI vectors
  .segment "VECTORS"
  .word reset
  .word reset
  .word reset
