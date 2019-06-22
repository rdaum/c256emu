        ; ZPStruct::Param1Addr points to destination tile map start addr
        ; ZPStruct::Param1Word = 16 bit X location
        ; ZPStruct::Param2Word = 16 bit Y location

        ; zeros used for blanking tile rows via mvp
zero:
        .res 16, $00

.proc clear_tile_map
	acc16i16

	; translate X,Y params to a tile map start index
	; Y / 16
	lda ZP+ZPStruct::Param2Word
	lsr
	lsr
	lsr
	lsr
	tay

	; X / 16
	lda ZP+ZPStruct::Param1Word
	lsr
	lsr
	lsr
	lsr
	tax

        ; set Y = X + Y * 64 using math co-proc
        tya
        sta f:$000100
        lda #64
        sta f:$000102
        stx ZP+ZPStruct::Param1Word
        lda f:$000104
        clc
        adc ZP+ZPStruct::Param1Word
        tay
	
        acc8i16

	lda #16
next_row:
	sta ZP+ZPStruct::Tmp
	ldx #16
	lda #0
row_loop:
	sta [ZP+ZPStruct::Param1Addr],y
	iny
	dex
	bne row_loop

	; add 48 to y index
	acc16i16
	clc
	tya
	adc #48
	tay
	acc8i16

	lda ZP+ZPStruct::Tmp
	dec
	cmp #0
	bne next_row
	rts
.endproc
