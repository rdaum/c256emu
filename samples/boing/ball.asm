; ball physics

BALL_W = 256
BALL_H = 256
MAX_BALL_X = 640 - BALL_W
; ball should bounce slightly before bottom of screen
MAX_BALL_Y = 450 - BALL_H

.proc move_ball_x
	.a16
	.i16
        ; move ball x, X reg indexes ball (0-3)*2, preserves X
        LDA Ball0_sx,x
        CMP #$1
        beq sub_x
        LDA Ball0_x,x
        ADC Ball0_dx,x
        STA Ball0_x,x
        rts
sub_x:
        LDA Ball0_x,x
        SBC Ball0_dx,x
        STA Ball0_x,x
	rts
.endproc

.proc move_ball_y
	.a16
	.i16
        ; move ball y, X reg indexes ball (0-3)*2, preserves X
        LDA Ball0_sy,x
        CMP #$1
        beq sub_y
        LDA Ball0_y,x
        ADC Ball0_dy,x
        STA Ball0_y,x
        rts
sub_y:
        LDA Ball0_y,x
        SBC Ball0_dy,x
        STA Ball0_y,x
	rts
.endproc

.proc bounds_ball_x
	.a16
	.i16
        ; check bounds x, X reg indexes ball (0-3)*2, preserves X
        LDA Ball0_x,x
        CMP #MAX_BALL_X
        BCC no_reverse_x
reverse_x:
        LDA Ball0_sx,x
        EOR #$1
        STA Ball0_sx,x
        jsr move_ball_x
no_reverse_x:
	rts
.endproc

.proc bounds_ball_y
	.a16
	.i16
        ; check bounds y, X reg indexes ball (0-3)*2, preserves X
        LDA Ball0_y,x
        CMP #MAX_BALL_Y
        BCC no_reverse_y
reverse_y:
        LDA Ball0_sy,x
        EOR #$1
        STA Ball0_sy,x
	; put ball at its max y for the bounce off bottom
	lda #MAX_BALL_Y
	sta Ball0_y,x
	; diminish the force a bit, or else we bounce higher and higher
	jsr accel_ball_y
no_reverse_y:
	rts
.endproc

.proc accel_ball_y
	.a16
	.i16
        ; accel ball y, X reg indexes ball (0-3)*2, preserves X
        LDA Ball0_sy,x
        CMP #$1
        beq sub_accel_y
        LDA Ball0_dy,x
        ADC Ball0_ay,x
        STA Ball0_dy,x
	rts
sub_accel_y:
        LDA Ball0_dy,x
        SBC Ball0_ay,x
        STA Ball0_dy,x
	; did we go negative? this would be the ball's apex
	cmp #8000
	BCC no_rev_accel_y
rev_accel_y:
	; ball is at its apex, reverse dir and set dy to 0
        LDA Ball0_sy,x
        EOR #$1
        STA Ball0_sy,x
	LDA #0
	STA Ball0_dy,x
no_rev_accel_y:
	rts
.endproc
