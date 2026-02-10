  .org $8000

reset:
  ; Write some test patterns directly to VRAM
  ; VRAM is at $2000-$3FFF
  
  ldx #$00
loop:
  txa
  sta $2000,x  ; Write X value to VRAM $2000+X
  sta $2100,x  ; Write X value to VRAM $2100+X
  sta $2200,x  ; Write X value to VRAM $2200+X
  inx
  bne loop
  
infinite:
  jmp infinite

; Reset/IRQ/NMI vectors
  .segment "VECTORS"
  .word reset  ; NMI
  .word reset  ; RESET
  .word reset  ; IRQ
