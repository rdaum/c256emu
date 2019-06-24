.p816
.feature c_comments
.include "macros.inc"
	
.rodata
ball_tile_set:  .incbin "tiles.bin"
bmp_1:  .incbin "grid-00000.bin"
bmp_2:  .incbin "grid-10000.bin"
bmp_3:  .incbin "grid-20000.bin"
bmp_4:  .incbin "grid-30000.bin"
bmp_5:  .incbin "grid-40000.bin"
lut_4:  .incbin "grid.col"

.bss
ROW_CLEAR_BLOCK:	.res 16
	
.zeropage

Ball0_x: .res 2	; x position
Ball1_x: .res 2	; x position
Ball2_x: .res 2	; x position
Ball3_x: .res 2	; x position

Ball0_y: .res 2	; y position
Ball1_y: .res 2	; y position
Ball2_y: .res 2	; y position
Ball3_y: .res 2	; y position

Ball0_ay: .res 2	; y acceleration
Ball1_ay: .res 2	; y acceleration
Ball2_ay: .res 2	; y acceleration
Ball3_ay: .res 2	; y acceleration

Ball0_dx: .res 2	; x delta
Ball1_dx: .res 2	; x delta
Ball2_dx: .res 2	; x delta
Ball3_dx: .res 2	; x delta

Ball0_dy: .res 2	; y delta
Ball1_dy: .res 2	; y delta
Ball2_dy: .res 2	; y delta
Ball3_dy: .res 2	; y delta

Ball0_sx: .res 2	; x delta sign
Ball1_sx: .res 2	; x delta sign
Ball2_sx: .res 2	; x delta sign
Ball3_sx: .res 2	; x delta sign

Ball0_sy: .res 2	; y delta sign
Ball1_sy: .res 2	; y delta sign
Ball2_sy: .res 2	; y delta sign
Ball3_sy: .res 2	; y delta sign

Param1Addr: .res 3
Param2Addr: .res 3
Param1Word: .res 2
Param2Word: .res 2

Lut0CycleLo: .res 3
Lut1CycleLo: .res 3
Lut2CycleLo: .res 3
Lut3CycleLo: .res 3

Lut0CycleHi: .res 3
Lut1CycleHi: .res 3
Lut2CycleHi: .res 3
Lut3CycleHi: .res 3

COLOR_SAVE_RG: .res 2	; tmp storage for color values
COLOR_SAVE_BA: .res 2	; tmp storage for color values
RC: .res 1
GC: .res 1
BC: .res 1

.code

INT_PENDING_REG0 = $000140
FNX0_INT00_SOF = $01
BORDER_CTRL_REG = $af0004
VICKY_CTRL_REG = $af0000
BITMAP_CTRL_REG = $af0140

BG_R = $af0008
BG_G = $af0009
BG_B = $af000a

TILE_0_REG = $af0100
TILE_1_REG = $af0108
TILE_2_REG = $af0110
TILE_3_REG = $af0118

TILE_0_MAP = $af50
TILE_1_MAP = $af58
TILE_2_MAP = $af60
TILE_3_MAP = $af68

LUT_0 = $af20
LUT_1 = $af24
LUT_2 = $af28
LUT_3 = $af2c


	
; The structure of a tile
.struct Tile
	ctrl_reg .byte
	start_addr .word
	start_addr_page .byte

	x_offset .byte		; x offset
	y_offset .byte		; y offset
.endstruct

.import __ZEROPAGE_LOAD__ 	; symbol defining the start of the ZP
	
begin:
	jmp init

	.include "place_tiles.asm"
	.include "clear_tiles.asm"
	.include "set_lut.asm"
	.include "ball.asm"

	
init:
	; set zero page to $fe00
	acc16i16
	lda #__ZEROPAGE_LOAD__
	tcd

	; stop IRQ handling
	sei

	;; copy the tile set data into $b0;
	;; can't just use one MVP as this spans across the bank into the next
	acc8i16
	ldx #0
copy_tile_loop:	
	lda f:ball_tile_set,x
	sta $b00000,x
	inx
	cpx #$ffff
	bne copy_tile_loop

	ldx #0
copy_bmp_1:
	lda f:bmp_1,x
	sta $b10000,x
	inx
	cpx #$0
	bne copy_bmp_1

	ldx #0
copy_bmp_2:
	lda f:bmp_2,x
	sta $b20000,x
	inx
	cpx #$0
	bne copy_bmp_2

	ldx #0
copy_bmp_3:
	lda f:bmp_3,x
	sta $b30000,x
	inx
	cpx #$0
	bne copy_bmp_3

	ldx #0
copy_bmp_4:
	lda f:bmp_4,x
	sta $b40000,x
	inx
	cpx #$0
	bne copy_bmp_4

	ldx #0
copy_bmp_5:
	lda f:bmp_5,x
	sta $b50000,x
	inx
	cpx #$b000
	bne copy_bmp_5

	ldx #0
copy_lut_4:
	lda f:lut_4,x
	sta $af3000,x
	inx
	cpx #$400
	bne copy_lut_4

	; populate $020000 with 16 zeroes
	ldx #16
	lda #00
zero:
	sta f:ROW_CLEAR_BLOCK,x
	dex
	bne zero

	; turn off border
	lda #0
	sta BORDER_CTRL_REG

        ; black background
        lda #$30
        sta BG_R
        sta BG_G
        sta BG_B

	; set tile and bitmap mode
	lda #$18
	sta VICKY_CTRL_REG

	; enable bitmap
	lda #1 | (4 << 4)
	sta BITMAP_CTRL_REG

	; set bitmap addr to 0xb10000
	acc16i16
	lda #$0000
	sta $af0141
	acc8i16
	lda #$01
	sta $af0143

	; Enable tiles + options + desired LUT for each
        EnableTiles TILE_0_REG, 0
        EnableTiles TILE_1_REG, 1
        EnableTiles TILE_2_REG, 2
        EnableTiles TILE_3_REG, 3

	; BallNum, XPos, YPos, XVelocity, YVelocity, YAccel
	SetBallParameters 0, 32, 48, 1, 0, 1
	SetBallParameters 1, 64, 64, 2, 0, 1
	SetBallParameters 2, 128, 16, 2, 0, 1
	SetBallParameters 3, 192, 86, 3, 0, 1

	; Init LUT tables: LUTAddr, R, G, B
	SetColorLut LUT_0, 127, 0, 0    ; Red
	SetColorLut LUT_1, 0, 127, 0    ; Green
	SetColorLut LUT_2, 127, 0, 127  ; Yellow
	SetColorLut LUT_3, 0, 0, 127    ; Blue

	; Pre-compute lut hi/lo addresses for lut cycle macros
	SetLUTAddress LUT_0, Lut0CycleHi, Lut0CycleLo
	SetLUTAddress LUT_1, Lut1CycleHi, Lut1CycleLo
	SetLUTAddress LUT_2, Lut2CycleHi, Lut2CycleLo
	SetLUTAddress LUT_3, Lut3CycleHi, Lut3CycleLo

loop:
	ClearBall TILE_0_MAP, Ball0_x, Ball0_y
	BallPhysics 0
	CycleLut Lut0CycleHi, Lut0CycleLo
	DrawBall TILE_0_MAP, Ball0_x, Ball0_y
	SetTileXScroll Ball0_x, TILE_0_REG
	SetTileYScroll Ball0_y, TILE_0_REG

	ClearBall TILE_1_MAP, Ball1_x, Ball1_y
	BallPhysics 1
	CycleLut Lut1CycleHi, Lut1CycleLo
	DrawBall TILE_1_MAP, Ball1_x, Ball1_y
	SetTileXScroll Ball1_x, TILE_1_REG
	SetTileYScroll Ball1_y, TILE_1_REG

	ClearBall TILE_2_MAP, Ball2_x, Ball2_y
	BallPhysics 2
	CycleLut Lut2CycleHi, Lut2CycleLo
	DrawBall TILE_2_MAP, Ball2_x, Ball2_y
	SetTileXScroll Ball2_x, TILE_2_REG
	SetTileYScroll Ball2_y, TILE_2_REG

	ClearBall TILE_3_MAP, Ball3_x, Ball3_y
	BallPhysics 3
	CycleLut Lut3CycleHi, Lut3CycleLo
	DrawBall TILE_3_MAP, Ball3_x, Ball3_y
	SetTileXScroll Ball3_x, TILE_3_REG
	SetTileYScroll Ball3_y, TILE_3_REG

delay:
	lda f:INT_PENDING_REG0	; have to use far because DBR keeps
				; getting clobbered
	and #FNX0_INT00_SOF
	cmp #FNX0_INT00_SOF	; check for SOF interrupt
	bne delay		; not there, spin
	sta f:INT_PENDING_REG0	; if so, clear the interrupt
	jmp loop	        ; and then cycle loop

