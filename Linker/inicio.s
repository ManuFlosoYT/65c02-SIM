.export _startup
.import _main
.importzp sp
.import copydata
.import zerobss

; Importamos símbolos creados por el linker para inicializar la pila C (opcional en este test, pero correcto)
.import __STACKSTART__

.segment "CODE"

_startup:
    ; 1. Hardware Stack (Tu emulador lo pone en 01FF, esto lo asegura)
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

.segment "VECTORS"
    .word _startup
    .word _startup
    .word _startup
