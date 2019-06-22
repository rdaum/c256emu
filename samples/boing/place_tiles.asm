        ; ZPStruct::Param1Addr points to destination tile map start addr
        ; ZPStruct::Param1Word = 16 bit X location
        ; ZPStruct::Param2Word = 16 bit Y location

        ; layout tile indices 0-255 in a 16x16 grid
	; into tile map memory determined by X,Y
.proc place_tile_map
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

	; set Y = X + Y * 64 using mathc co-proc
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
	; y is offset from zero page ptr from above
	; a tile index 0-255
	lda #0

next_row:
	ldx #16
row_loop:
	sta [ZP+ZPStruct::Param1Addr],y
	inc
	iny
	dex
	bne row_loop

	; add 48 to y index, preserve a
	acc16i16
	clc
	tax
	tya
	adc #48
	tay
	txa
	acc8i16

	; last tile index?
	cmp #0
	bne next_row
	rts
.endproc
