.export _startup
.import _main

; Importamos símbolos creados por el linker para inicializar la pila C (opcional en este test, pero correcto)
.import __STACKSTART__

.segment "CODE"

_startup:
    ; 1. Hardware Stack (Tu emulador lo pone en 01FF, esto lo asegura)
    ldx #$FF
    txs

    ; 2. (Opcional) Aquí se configuraría el Stack de C, pero para este test simple
    ;    saltaremos directo a main.

    jsr _main       ; Ejecuta el código C

    ; 3. Al volver, crash intencional
    .byte $02

.segment "VECTORS"
    .word _startup
    .word _startup
    .word _startup
