; App-Runtime.s — Minimal C startup for microDOS .app binaries
; 
; Unlike the main C-Runtime, this one:
;   - Does NOT set RESETVEC (microDOS owns those)
;   - Uses a C stack in the app's own RAM area ($6010-$7FFF)
;   - Returns via RTS when _main() returns (back to cmd_run loader)
;
.export _app_startup
.import _main
.importzp sp
.import copydata
.import zerobss
.import __STACKSTART__

; Dedicated ZP location to save OS stack pointer (outside app's virtual registers)
os_sp_save = $70 

.segment "CODE"

_app_startup:
    ; Save OS stack pointer
    lda sp
    sta os_sp_save
    lda sp+1
    sta os_sp_save+1

    ; Set up app's own C stack
    lda #<__STACKSTART__
    sta sp
    lda #>__STACKSTART__
    sta sp+1

    ; Init app's C environment (data/bss)
    jsr copydata
    jsr zerobss

    jsr _main       ; Run app main()

    ; Restore OS stack pointer
    lda os_sp_save
    sta sp
    lda os_sp_save+1
    sta sp+1

    ; Return to cmd_run() in microDOS
    rts
