        ; Param1Addr points to LUT startaddr to populate
        ; Param2Addr used as tmp
	; RC,GC,BC color to use

.proc set_lut_colors
	acc8i16
	; set Param2Addr to point to LUT + 4
        lda Param1Addr
	clc
	adc #4
        sta Param2Addr
        lda Param1Addr + 1
        sta Param2Addr
        lda Param2Addr + 2
        sta Param2Addr

        ; set first 30 colors in LUT to red
        ; index 0 is always transparent
        ldx #30
        ldy #0

first_color:
        lda GC
        sta [Param1Addr],y
        iny

        lda BC
        sta [Param1Addr],y
        iny

        lda RC
        sta [Param1Addr],y
        iny
        iny

        dex
        bne first_color

        ; set next 30 colors in LUT to white
        ldx #30
second_color:
        lda #127
        sta [Param1Addr],y
        iny

        lda #127
        sta [Param1Addr],y
        iny

        lda #127
        sta [Param1Addr],y
        iny
        iny

        dex
        bne second_color
	rts
.endproc
