.p816
.include "macros.inc"

.segment "TILESET"
ball_tile_set:  .incbin "tiles.bin", $0, $ffff

.code
INT_PENDING_REG0 = $000140
FNX0_INT00_SOF = $01
BORDER_CTRL_REG = $af0004
VICKY_CTRL_REG = $af0000
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

; Struct for our zero page vars
ZP = $00
.struct ZPStruct
        Ball0_x .word	; x position
        Ball1_x .word	; x position
        Ball2_x .word	; x position
        Ball3_x .word	; x position

        Ball0_y .word	; y position
        Ball1_y .word	; y position
        Ball2_y .word	; y position
        Ball3_y .word	; y position

	Ball0_ay .word	; y acceleration
	Ball1_ay .word	; y acceleration
	Ball2_ay .word	; y acceleration
	Ball3_ay .word	; y acceleration

	Ball0_dx .word	; x delta
	Ball1_dx .word	; x delta
	Ball2_dx .word	; x delta
	Ball3_dx .word	; x delta

	Ball0_dy .word	; y delta
	Ball1_dy .word	; y delta
	Ball2_dy .word	; y delta
	Ball3_dy .word	; y delta

	Ball0_sx .word	; x delta sign
	Ball1_sx .word	; x delta sign
	Ball2_sx .word	; x delta sign
	Ball3_sx .word	; x delta sign

	Ball0_sy .word	; y delta sign
	Ball1_sy .word	; y delta sign
	Ball2_sy .word	; y delta sign
	Ball3_sy .word	; y delta sign

	Param1Addr .faraddr
	Param2Addr .faraddr
	Param1Word .word
	Param2Word .word

	Lut0CycleLo .faraddr
	Lut1CycleLo .faraddr
	Lut2CycleLo .faraddr
	Lut3CycleLo .faraddr

	Lut0CycleHi .faraddr
	Lut1CycleHi .faraddr
	Lut2CycleHi .faraddr
	Lut3CycleHi .faraddr

        RC .byte	; tmp storage for color values
        GC .byte	; tmp storage for color values
        BC .byte	; tmp storage for color values
        Tmp .byte
.endstruct

begin:
	jmp init

	.include "place_tiles.asm"
	.include "clear_tiles.asm"
	.include "set_lut.asm"
	.include "ball.asm"

init:
	; set zero page to $fe00
	acc16i16
	lda #$fe00
	tcd

	; stop IRQ handling
	sei

	acc8i16

	; populate $020000 with 16 zeroes
	ldx #16
	lda #00
zero:
	sta f:$020000,x
	dex
	bne zero

	; turn off border
	lda #0
	sta BORDER_CTRL_REG

	; set tile mode
	lda #$10
	sta VICKY_CTRL_REG

	; black background
	lda #$0
	sta BG_R
	sta BG_G
	sta BG_B

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

