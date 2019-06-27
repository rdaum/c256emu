.p816
.feature c_comments
.include "macros.inc"

.rodata
ball0_tile_set:  .incbin "tiles-256.bin"
ball1_tile_set:  .incbin "tiles-224.bin"
ball2_tile_set:  .incbin "tiles-192.bin"
ball3_tile_set:  .incbin "tiles-160.bin"
bmp_1:  .incbin "grid-00000.bin"
bmp_2:  .incbin "grid-10000.bin"
bmp_3:  .incbin "grid-20000.bin"
bmp_4:  .incbin "grid-30000.bin"
bmp_5:  .incbin "grid-40000.bin"
lut_4:  .incbin "grid.col"

.bss
ROW_CLEAR_BLOCK:        .res 16

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
BG_R = $af0008
BG_G = $af0009
BG_B = $af000a

BMAP_CTRL = $af0140
BMAP_ADDR = $af0141

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

; for tiles
BALL_0_DST = $b00000
BALL_1_DST = $b10000
BALL_2_DST = $b20000
BALL_3_DST = $b30000

; for background bitmap
; BMAP_HI/MD/LO must point to video ram relative to $000000
BMAP_HI = $04
BMAP_MD = $00
BMAP_LO = $00
BMAP_1 = $b40000
BMAP_2 = $b50000
BMAP_3 = $b60000
BMAP_4 = $b70000
BMAP_5 = $b80000
BMAP_LUT = $af3000

; The structure of a tile
.struct Tile
	ctrl_reg .byte
	start_addr .word
	start_addr_page .byte

	x_offset .byte		; x offset
	y_offset .byte		; y offset
.endstruct

.import __ZEROPAGE_LOAD__       ; symbol defining the start of the ZP

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

	CopyBlock ball0_tile_set, BALL_0_DST, 0 ; 64k
	CopyBlock ball1_tile_set, BALL_1_DST, 0 ; 64k
	CopyBlock ball2_tile_set, BALL_2_DST, 0 ; 64k
	CopyBlock ball3_tile_set, BALL_3_DST, 0 ; 64k

	lda #(1 | 4 << 4) ; turn on bitmap with lut 4
	; Point bitmap to where background data went
	sta BMAP_CTRL
	lda #BMAP_LO
	sta BMAP_ADDR
	lda #BMAP_MD
	sta BMAP_ADDR+1
	lda #BMAP_HI
	sta BMAP_ADDR+2

	CopyBlock bmp_1, BMAP_1, 0 ; 64k
	CopyBlock bmp_2, BMAP_2, 0 ; 64k
	CopyBlock bmp_3, BMAP_3, 0 ; 64k
	CopyBlock bmp_4, BMAP_4, 0 ; 64k
	CopyBlock bmp_5, BMAP_5, $b000

	CopyBlock lut_4, BMAP_LUT, $400

	; turn off border
	lda #0
	sta BORDER_CTRL_REG

	; set tile and bitmap mode
	lda #$18
	sta VICKY_CTRL_REG

	; black background
	lda #$0
	sta BG_R
	sta BG_G
	sta BG_B

	; Enable tiles + options + desired LUT for each
        EnableTiles TILE_0_REG, 0, $0000, $00
        EnableTiles TILE_1_REG, 1, $0000, $01
        EnableTiles TILE_2_REG, 2, $0000, $02
        EnableTiles TILE_3_REG, 3, $0000, $03

	; BallNum, XPos, YPos, XVelocity, YVelocity, YAccel
	SetBallParameters 0, 32, 48, 1, 0, 1
	SetBallParameters 1, 64, 64, 2, 0, 1
	SetBallParameters 2, 128, 16, 2, 0, 1
	SetBallParameters 3, 192, 86, 3, 0, 1

	; Init LUT tables: LUTAddr, R, G, B
	SetColorLut LUT_0, 127, 0, 0    ; Red
	SetColorLut LUT_1, 0, 127, 0    ; Green
	SetColorLut LUT_2, 127, 127, 0  ; Purple
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

