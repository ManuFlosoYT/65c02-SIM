; VRAM fill test - use absolute addressing directly
; VRAM: $2000-$3FFF, addressing A0-A6=X, A7-A13=Y
; So row Y starts at: $2000 + (Y * $80)

  .org $8000

reset:
  ldx #$00        ; Color counter
  
  ; Write to all rows manually
  ; Row 0: $2000
  lda #$11
  jsr fill_row_at_2000
  
  ; Row 1: $2080
  lda #$22
  jsr fill_row_at_2080
  
  ; Row 2: $2100
  lda #$33
  jsr fill_row_at_2100
  
  ; For full test, let's use a loop with proper 16-bit math
  jmp full_fill

fill_row_at_2000:
  ldx #100
:   dex
    sta $2000,x
    cpx #$00
    bne :-
    rts

fill_row_at_2080:
  ldx #100
:   dex
    sta $2080,x
    cpx #$00
    bne :-
    rts

fill_row_at_2100:
  ldx #100
:   dex
    sta $2100,x
    cpx #$00
    bne :-
    rts

full_fill:
  ; Fill using a different approach - write sequentially
  ; but skip the "blanking" bytes (X=100 to X=127)
  
  lda #$00
  sta $10         ; Color
  
  ldx #$00        ; Row counter
row_loop:
  ; Calculate row base address: $2000 + (X * 128)
  ; Store in $11-$12
  txa
  sta $11         ; Save row number
  
  lda #$00        
  sta $12         ; Will build address here (low)
  lda #$20
  sta $13         ; High byte starts at $20
  
  ; Add (row * 128) to $2000
  ; row * 128 = row << 7
  lda $11
  lsr             ; Divide by 2 to get high byte contribution
  clc
  adc $13
  sta $13         ; High byte
  
  lda $11
  asl
  asl
  asl
  asl
  asl
  asl
  asl
  sta $12         ; Low byte = (row & 1) * 128
  
  ; Now write 100 pixels
  ldy #0
pixel_loop:
  lda $10
  sta ($12),y
  iny
  cpy #100
  bne pixel_loop
  
  inc $10         ; Next color
  inx
  cpx #75
  bne row_loop
  
done:
  jmp done

; Reset/IRQ/NMI vectors
  .segment "VECTORS"
  .word reset
  .word reset
  .word reset
