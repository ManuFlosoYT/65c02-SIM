.setcpu "65C02"
.debuginfo
.export MONRDKEY, MONCOUT, LOAD, SAVE, INIT_BUFFER, IRQ_HANDLER, MONPEEK
.export _MONRDKEY, _MONCOUT, _LOAD, _SAVE, _INIT_BUFFER

_MONRDKEY = MONRDKEY
_MONCOUT = MONCOUT
_LOAD = LOAD
_SAVE = SAVE
_INIT_BUFFER = INIT_BUFFER

; configuration
CONFIG_2A := 1

CONFIG_SCRTCH_ORDER := 2

; zero page
ZP_START0 = $00
ZP_START1 = $02
ZP_START2 = $0C
ZP_START3 = $62
ZP_START4 = $6D

; extra/override ZP variables
USR := GORESTART

; constants
SPACE_FOR_GOSUB := $3E
STACK_TOP := $FA
WIDTH := 40
WIDTH2 := 30
RAMSTART2 := $0400

; symbol aliases
COLD_START = RESET
GORESTART = GETLINE

.segment "HEADER"
jmp COLD_START

.segment "CODE"
ISCNTC:
        jsr MONRDKEY
        bcc not_cntc
        cmp #3
        bne not_cntc
        jmp is_cntc

not_cntc:
        rts

is_cntc:
        ; Fall through
;!!! runs into "STOP"

.zeropage

READ_PTR:       .res 1
WRITE_PTR:      .res 1

.segment "INPUT_BUFFER"
INPUT_BUFFER:   .res $100

.segment "BIOS"

ACIA_DATA       = $5000
ACIA_STATUS     = $5001
ACIA_CMD        = $5002
ACIA_CTRL       = $5003
PORTA           = $6001
DDRA            = $6003

LOAD:
                rts

SAVE:
                rts


; Input a character from the serial interface.
; On return, carry flag indicates whether a key was pressed
; If a key was pressed, the key value will be in the A register
;
MONRDKEY:
CHRIN:
                phx
@wait_key:
                jsr     BUFFER_SIZE
                beq     @wait_key
                
                jsr     READ_BUFFER
                jsr     CHROUT                  ; echo
                pha
                jsr     BUFFER_SIZE
                cmp     #$B0
                bcs     @mostly_full
                lda     #$fe
                and     PORTA
                sta     PORTA
@mostly_full:
                pla
                plx
                sec
                rts

; Peek a character from input buffer without removing it
; Modifies: flags, A, X
MONPEEK:
                phx
                jsr BUFFER_SIZE
                beq @no_keyp
                ldx READ_PTR
                lda INPUT_BUFFER,x
                plx
                sec
                rts
@no_keyp:
                plx
                clc
                rts


; Output a character (from the A register) to the serial interface.
;
; Modifies: flags
MONCOUT:
CHROUT:
                pha
                sta     ACIA_DATA
                lda     #$FF
@txdelay:       dec
                bne     @txdelay
                pla
                rts

; Initialize the circular input buffer
; Modifies: flags, A
INIT_BUFFER:
                lda READ_PTR
                sta WRITE_PTR
                lda #$01
                sta DDRA
                lda #$fe
                and PORTA
                sta PORTA
                rts

; Write a character (from the A register) to the circular input buffer
; Modifies: flags, X
WRITE_BUFFER:
                ldx WRITE_PTR
                sta INPUT_BUFFER,x
                inc WRITE_PTR
                rts

; Read a character from the circular input buffer and put it in the A register
; Modifies: flags, A, X
READ_BUFFER:
                ldx READ_PTR
                lda INPUT_BUFFER,x
                inc READ_PTR
                rts

; Return (in A) the number of unread bytes in the circular input buffer
; Modifies: flags, A
BUFFER_SIZE:
                lda WRITE_PTR
                sec
                sbc READ_PTR
                rts


; Interrupt request handler
.export IRQ_HANDLER
IRQ_HANDLER:
                pha
                phx
                lda     ACIA_STATUS
                ; For now, assume the only source of interrupts is incoming data
                lda     ACIA_DATA
                jsr     WRITE_BUFFER
                jsr     BUFFER_SIZE
                cmp     #$F0
                bcc     @not_full
                lda     #$01
                ora     PORTA
                sta     PORTA
@not_full:
                plx
                pla
                rti

.export _wozmon_entry
_wozmon_entry:
                jmp GORESTART

.export WOZMON_RESET
WOZMON_RESET = RESET

.include "wozmon.s"



