
	
	;An alien file. All PC relative.
	
	rsreset
	rs.l	1
xa	rs.l	1
xs	rs.l	1
x	rs.l	1	*
ya	rs.l	1
ys	rs.l	1
y	rs.l	1	*
frame	rs.l	1
flags	rs.b	1
id	rs.b	1
animf	rs.l	1
anims	rs.l	1
anim	rs.l	1	*
facing	rs.w	1
loop	rs.l	1
collide	rs.l	1
shot	rs.l	1
	;
	;user stuff...
	;
other	rs.l	1
xs2	rs.l	1
ys2	rs.l	1
hitechk	rs.w	1
	;
	
	;library routines...
	;
	rsreset
beginadd	rs.l	1
endadd	rs.l	1
implode	rs.l	1
explode	rs.l	1
rnd1	rs.l	1
rnd2	rs.l	1
getmounty	rs.l	1
newanim	rs.l	1
killme	rs.l	1
rnd3	rs.l	1	;randomly negate d0!
	;
hites	rs.l	1	;pointer to heights of mounts
			;table


start	dc.l	finish-start+4		;length of alien mod
	dc.l	data-start		;player dependant data start
	dc.l	dataf-data		;above's length
	;
	dc.l	pergame-start		;start of game routine
	dc.l	perpattern-start	;new pattern routine,
				;pattern in d0. Return
				;perloop routine in d0,
				;or none.
	dc.l	perlife-start		;new life routine.

hite1	equ	6
hite2	equ	1

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	move	#50,numhumes-data(a2)
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	divu	#5,d0
	swap	d0
	tst	d0
	bne.s	.skip
	move	#10,numhumes-data(a2)
.skip	sub.l	a0,a0
	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	move	numhumes-data(a2),d7	;number of humes...
	beq	.done
	subq	#1,d7
	;
.loop	jsr	beginadd(a5)
	;
	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	jsr	getmounty(a5)
	move	d0,y(a0)
	;
	jsr	rnd1(a5)
	and	#$7fff,d0
	or	#$4000,d0
	jsr	rnd3(a5)
	move.l	d0,xs2(a0)
	move.l	d0,xs(a0)
	swap	d0
	move	d0,facing(a0)
	;
	jsr	rnd1(a5)
	and	#$1fff,d0
	or	#$1000,d0
	move.l	d0,ys2(a0)
	move	#hite1,hitechk(a0)
	clr.l	other(a0)
	;
	move.b	#1,id(a0)
	lea	anim1(pc),a2
	jsr	newanim(a5)
	lea	loop1(pc),a2
	move.l	a2,loop(a0)
	lea	shot1(pc),a2
	move.l	a2,shot(a0)
	lea	frag1(pc),a2
	jsr	implode(a5)
	;
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

loop1	move.l	other(a0),d0
	beq.s	lmove
	move.l	d0,a2
	cmp	#$100,flags(a2)
	beq.s	.skip
	clr.l	other(a0)
	move.b	#1,id(a0)
	bra.s	lmove
.skip	;
	;lander-ized
	;
	rts
	;
lmove	move	x(a0),d0
	jsr	getmounty(a5)
	move	y(a0),d1		;where I am!
	sub	d0,d1
	move	d1,d2
	bpl.s	.skip
	neg	d2
.skip	cmp	hitechk(a0),d2
	bls.s	.golr
	;
.goud	;move.l	xs(a0),d0
	;beq.s	.done
	;
	move.l	ys2(a0),d0
	tst	d1
	bmi.s	.skip2
	neg.l	d0
.skip2	clr.l	xs(a0)
	move.l	d0,ys(a0)
	move	#hite2,hitechk(a0)
	lea	anim2(pc),a2
	jsr	newanim(a5)
.done	rts
	;
.golr	move.l	ys(a0),d0
	beq.s	.done
	move.l	xs2(a0),xs(a0)
	clr.l	ys(a0)
	move	#hite1,hitechk(a0)
	lea	anim1(pc),a2
	jsr	newanim(a5)
	rts

shot1	lea	frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

data
numhumes	dc	0
dataf

anim1	incbin	hume1.anm
anim2	incbin	hume2.anm

frag1	incbin	hume1.frags

	cnop	0,4
	dc.l	'BYE!'
finish
