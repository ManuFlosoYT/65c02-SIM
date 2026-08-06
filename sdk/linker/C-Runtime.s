.export _startup
.import _main
.importzp sp
.import copydata
.import zerobss
.import IRQ_HANDLER

; Import symbols created by the linker to initialize the C stack
.import __STACKSTART__

.segment "CODE"

_startup:
    ; Hardware Stack
    ldx #$FF
    txs

    ; Configure the C stack
    lda #<__STACKSTART__
    sta sp
    lda #>__STACKSTART__
    sta sp+1

    ; Init C environment
    jsr copydata
    jsr zerobss

    jsr _main       ; Run C code

    ; Return, intentional crash
    .byte $02

.segment "RESETVEC"
    .word $0F00        ; NMI
    .word _startup     ; RESET (C startup)
    .word IRQ_HANDLER  ; IRQ (BIOS handler)
