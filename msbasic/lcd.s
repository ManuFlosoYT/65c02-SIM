.segment "CODE"

PORTB   = $6000
DDRB    = $6002
LCD_E   = $40
LCD_RS  = $10

lcd_send_nibble:
        and     #$0F
        cpx     #0
        bne     lcd_nibble_data
lcd_nibble_cmd:
        and     #$0F
        ora     #LCD_E
        sta     PORTB
        eor     #LCD_E
        sta     PORTB
        rts

lcd_nibble_data:
        and     #$0F
        ora     #(LCD_RS | LCD_E)
        sta     PORTB
        eor     #LCD_E
        sta     PORTB
        rts

lcd_instruction:
        pha
        lsr
        lsr
        lsr
        lsr
        jsr     lcd_nibble_cmd
        pla
        jsr     lcd_nibble_cmd
        rts

lcd_print_char:
        pha
        lsr
        lsr
        lsr
        lsr
        jsr     lcd_nibble_data
        pla
        jsr     lcd_nibble_data
        rts

LCDINIT:
        lda     #$FF
        sta     DDRB

        lda     #$03
        jsr     lcd_nibble_cmd
        lda     #$03
        jsr     lcd_nibble_cmd
        lda     #$03
        jsr     lcd_nibble_cmd

        lda     #$02
        jsr     lcd_nibble_cmd

        lda     #$28
        jsr     lcd_instruction
        lda     #$0E
        jsr     lcd_instruction
        lda     #$06
        jsr     lcd_instruction
        lda     #$01
        jsr     lcd_instruction
        rts

LCDCMD:
        jsr     GETBYT
        txa
        jmp     lcd_instruction

LCDPRINT:
        jsr     FRMEVL
        bit     VALTYP
        bmi     @is_str
        jsr     FOUT
        jsr     STRLIT
@is_str:
        jsr     FREFAC
        tax
        beq     @done
        ldy     #0
@loop:
        lda     (INDEX),y
        jsr     lcd_print_char
        iny
        dex
        bne     @loop
@done:
        rts
