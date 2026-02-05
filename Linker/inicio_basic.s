.export _startup
.import _COLD_START
.import IRQ_HANDLER

.segment "CODE"

_startup:
    ; 1. Inicializar Stack del Hardware 6502
    ldx #$FF
    txs

    ; 2. Saltar directamente al inicio frío de BASIC
    jmp _COLD_START

; --- Definición de Vectores (Reset, IRQ, NMI) ---
.segment "RESETVEC"
    .word $0F00        ; NMI (Vector dummy o tu manejador NMI)
    .word _startup     ; RESET (Apunta a este inicio)
    .word IRQ_HANDLER  ; IRQ (Apunta a la BIOS)