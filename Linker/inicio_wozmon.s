.export _startup
.import WOZMON_RESET
.import IRQ_HANDLER

.segment "CODE"

_startup:
    ; 1. Inicializar Stack del Hardware 6502
    ldx #$FF
    txs

    ; 2. Saltar directamente al inicio del monitor
    jmp WOZMON_RESET

; --- Definición de Vectores (Reset, IRQ, NMI) ---
.segment "RESETVEC"
    .word $0F00        ; NMI
    .word _startup     ; RESET
    .word IRQ_HANDLER  ; IRQ
