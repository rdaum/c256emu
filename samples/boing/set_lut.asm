        ; ZP+ZPStruct::Param1Addr points to LUT startaddr to populate
        ; ZP+ZPStruct::Param2Addr used as tmp
	; ZP+ZPStruct::RC,ZP+ZPStruct::GC,ZP+ZPStruct::BC color to use

.proc set_lut_colors
	acc8i16
	; set ZP+ZPStruct::Param2Addr to point to LUT + 4
        lda ZP+ZPStruct::Param1Addr
	clc
	adc #4
        sta ZP+ZPStruct::Param2Addr
        lda ZP+ZPStruct::Param1Addr + 1
        sta ZP+ZPStruct::Param2Addr
        lda ZP+ZPStruct::Param2Addr + 2
        sta ZP+ZPStruct::Param2Addr

        ; set first 30 colors in LUT to red
        ; index 0 is always transparent
        ldx #30
        ldy #0

first_color:
        lda ZP+ZPStruct::GC
        sta [ZP+ZPStruct::Param1Addr],y
        iny

        lda ZP+ZPStruct::BC
        sta [ZP+ZPStruct::Param1Addr],y
        iny

        lda ZP+ZPStruct::RC
        sta [ZP+ZPStruct::Param1Addr],y
        iny
        iny

        dex
        bne first_color

        ; set next 30 colors in LUT to white
        ldx #30
second_color:
        lda #127
        sta [ZP+ZPStruct::Param1Addr],y
        iny

        lda #127
        sta [ZP+ZPStruct::Param1Addr],y
        iny

        lda #127
        sta [ZP+ZPStruct::Param1Addr],y
        iny
        iny

        dex
        bne second_color
	rts
.endproc
