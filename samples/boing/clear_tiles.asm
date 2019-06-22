        ; ZPStruct::Param1Addr points to destination tile map start addr
        ; ZPStruct::Param1Word = 16 bit X location
        ; ZPStruct::Param2Word = 16 bit Y location

.macro ClearRow
	.a16
	.i16
        lda #15			; copy 16 zeroes
	ldx #0000		; src address of zeroes
	sty ZP+ZPStruct::Param1Word	; save current dest ptr
        mvn #$02, #$af		; from page $02 into $af
	ldy ZP+ZPStruct::Param1Word	; restore dest ptr
	tya
	clc
	adc #64			; advance to next row
	tay			; put back into dest arg
.endmacro

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

	; Param1Word used as tmp space from here on
        ; set Y = TileStartAddr + X + Y * 64 using math co-proc
        tya
        sta f:$000100			; op A
        lda #64				;
        sta f:$000102			; op B
        stx ZP+ZPStruct::Param1Word	; save x
        lda f:$000104			; = y * 64
        clc
        adc ZP+ZPStruct::Param1Word	; add X
	adc ZP+ZPStruct::Param1Addr	; add start addr
        tay				; dest for our zeroes

	; clear 16 rows each 16 tiles
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow
	ClearRow

	rts
.endproc
