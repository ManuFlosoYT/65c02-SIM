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
.import __CORE_SIZE__

; Dedicated ZP location to save OS stack pointer (outside app's virtual registers)
os_sp_save = $70 

.segment "HEADER"
    .byte "uDOS"           ; Offset 0-3: Magic Header
    .byte 3                ; Offset 4: Format Version 3
    .byte 1                ; Offset 5: Flags (1 = Paginación activada)
    .word _app_startup     ; Offset 6-7: Entry point 
    .word __CORE_SIZE__    ; Offset 8-9: Tamaño del Resident Core (calculado por linker)
    .word $2000            ; Offset 10-11: Tamaño de página (8KB por defecto)
    .word $2000            ; Offset 12-13: Dirección en RAM de la ventana de paginación
    .res 2, 0              ; Offset 14-15: Reservado

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
