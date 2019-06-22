	; status bits
	; 0x20 = accum, cleared 16, set = 8
	; 0x10 = index, cleared 16, set = 8

.macro acc8i16
	sep #$20
	.a8
	.i16
.endmacro

.macro acc16i16
	rep #$30
	.a16
	.i16
.endmacro

.macro acc8i8
	sep #$30
	.a8
	.i8
.endmacro

.macro acc16i8
	sep #$10
	.a16
	.i8
.endmacro

; Macro to set one of our zp lut address fields (hi/lo)
.macro SetLUTAddress LUTRegAddr, LUTHiField, LUTLoField
        acc16i16
        ldx #LUTRegAddr
        stz LUTLoField
        stx LUTLoField + 1
        acc8i16
        lda LUTLoField
        clc
        adc #4
        sta LUTHiField
        lda LUTLoField + 1
        sta LUTHiField + 1
        lda LUTLoField + 2
        sta LUTHiField + 2
.endmacro

.macro SetTileXScroll BallField, TileReg
        ; set tile x scroll offset, this is lower 4 bits of X
        acc16i16
        lda BallField
        and #$0f
        acc8i16
        sta TileReg+Tile::x_offset
.endmacro

.macro SetTileYScroll BallField, TileReg
        ; set tile y scroll offset, this is lower 4 bits of Y
        acc16i16
        lda BallField
        and #$0f
        acc8i16
        sta TileReg+Tile::y_offset
        acc16i16
.endmacro

.macro ClearBall TileMapAddr, BallXField, BallYField
        ; draw the ball(s)
        acc16i16
        lda #TileMapAddr
        stz Param1Addr
        sta Param1Addr+1
        lda BallXField
        sta Param1Word
        lda BallYField
        sta Param2Word
        jsr clear_tile_map
.endmacro

.macro DrawBall TileMapAddr, BallXField, BallYField
        ; draw the ball(s)
        acc16i16
        lda #TileMapAddr
        stz Param1Addr
        sta Param1Addr+1
        lda BallXField
        sta Param1Word
        lda BallYField
        sta Param2Word
        jsr place_tile_map
.endmacro

; LUTHiAddr is LUTLowAddr + 4
.macro CycleLut LUTHiAddr, LUTLoAddr
	acc16i16

	; save color at index 1 which is about
	; to be clobbered
	ldy #0
	lda [LUTHiAddr],y
	sta RC
	iny
	lda [LUTHiAddr],y
	sta GC
	iny
	lda [LUTHiAddr],y
	sta BC

	; move colors 2-59 down to 1-58
	lda #239 ; 1 less than 60*4
	ldx LUTHiAddr
	ldy LUTLoAddr
	mvn #$af,#$af

	; put saved color 1 into pos 59
	ldy #240 ; 60 * 4
	lda RC
	sta [LUTLoAddr],y
	iny
	lda GC
	sta [LUTLoAddr],y
	iny
	lda BC
	sta [LUTLoAddr],y
.endmacro

.macro BallPhysics BallNum
	acc16i16
        ldx #BallNum*2
        jsr move_ball_x
        jsr move_ball_y

        ; accelerate ball in y direction to simulate gravity
        jsr accel_ball_y

        ; check bounds and reverse dirs if necessary
        jsr bounds_ball_x
        jsr bounds_ball_y
.endmacro

.macro SetColorLut LutAddr, R, G, B
        ; Param1Addr = lut addr
        acc16i16
        ldx #LutAddr
        stz Param1Addr
        stx Param1Addr+1
        acc8i16
        lda #R
        sta RC
        lda #G
        sta GC
        lda #B
        sta BC
        jsr set_lut_colors
.endmacro

.macro SetBallParameters BallNum, XPos, YPos, XVelocity, YVelocity, YAccel
        ldx #BallNum*2
        ldy #XPos
        sty Ball0_x,x
        ldy #YPos
        sty Ball0_y,x

        ; Ball accel ay 
        ldy #YAccel
        sty Ball0_ay,x

        ; Ball dir dx,dy
        ldy #XVelocity
        sty Ball0_dx,x
        ldy #YVelocity
        sty Ball0_dy,x
.endmacro

.macro EnableTiles TileRegBase, LutNum
        ; enable tile layer, turn on scroll x & y, lut 
        lda #$31+(LutNum<<1)
        sta TileRegBase+Tile::ctrl_reg

        ; point tile 0 to video memory $000000
        ; (load tile bitmap into mapped $b00000)
        lda #$00
        sta TileRegBase+Tile::start_addr
        sta TileRegBase+Tile::start_addr_page
.endmacro
