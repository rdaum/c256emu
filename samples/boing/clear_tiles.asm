	; Y ptr to top left corner byte of square to clear
.macro ClearRow RowNum
	.a16
	.i16

        lda #15					; copy 16 zeroes
	ldx #.loword(ROW_CLEAR_BLOCK)		; src address of zeroes
        mvn #.bankbyte(ROW_CLEAR_BLOCK), #$af	; from src page into $af
	tya
	clc
	adc #48			; advance to next row
	tay			; put back into dest arg
.endmacro

        ; Param1Addr points to destination tile map start addr
        ; Param1Word = 16 bit X location
        ; Param2Word = 16 bit Y location
.proc clear_tile_map
	acc16i16

	; translate X,Y params to a tile map start index
	; Y / 16
	lda Param2Word
	lsr
	lsr
	lsr
	lsr
	tay

	; X / 16
	lda Param1Word
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
        stx Param1Word	; save x
        lda f:$000104			; = y * 64
        clc
        adc Param1Word	; add X
	adc Param1Addr	; add start addr
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
