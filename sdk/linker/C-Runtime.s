.export _startup
.import _main
.importzp sp
.import copydata
.import zerobss
.import IRQ_HANDLER

; Importamos símbolos creados por el linker para inicializar la pila C
.import __STACKSTART__

.segment "CODE"

_startup:
    ; 1. Hardware Stack
    ldx #$FF
    txs

    ; 2. Configurar el Stack de C
    lda #<__STACKSTART__
    sta sp
    lda #>__STACKSTART__
    sta sp+1

    ; 2.5 Init C environment
    jsr copydata
    jsr zerobss

    jsr _main       ; Ejecuta el código C

    ; 3. Al volver, crash intencional
    .byte $02

.segment "RESETVEC"
    .word $0F00        ; NMI
    .word _startup     ; RESET (C startup)
    .word IRQ_HANDLER  ; IRQ (BIOS handler)