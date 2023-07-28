
	
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
other	rs.l	1	;hume going for!
xs2	rs.l	1	;old x speed
ys2	rs.l	1	;max for climbing with hume.
ya2	rs.l	1	;for homing in.
timer	rs.w	1	;for above
	
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
rnd3	rs.l	1
chkinfront	rs.l	1
shipx	rs.l	1	;*
shipy	rs.l	1	;*
sxspeed	rs.l	1	;*
syspeed	rs.l	1	;*
shipdir	rs.l	1	;*
homeawayy	rs.l	1
homeiny	rs.l	1
homeawayya	rs.l	1
homeinya	rs.l	1
limitys	rs.l	1
screenx	rs.l	1
screeny	rs.l	1
getxdiff	rs.l	1
findid	rs.l	1
homeinxa2	rs.l	1

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

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	move	#10,numlanders-data(a2)
	sub.l	a0,a0
	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	move	numlanders-data(a2),d7	;number of landers...
	beq.s	.done
	subq	#1,d7
	;
.loop	jsr	beginadd(a5)
	;
	move	#352,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#256,d0
	jsr	rnd2(a5)
	add	#128,d0
	move	d0,y(a0)
	;
	jsr	rnd1(a5)
	ext.l	d0
	lsl.l	#4,d0
	move.l	d0,xs(a0)
	move.l	d0,xs2(a0)
	;
	jsr	rnd1(a5)
	and	#$fff,d0
	move.l	d0,ya2(a0)
	clr.l	other(a0)
	;
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

loop3	;lander going up with hume!
	;
	rts

loop2	;lander going down for a hume.
	;
	move.l	other(a0),a2
	cmp	#$102,flags(a2)	;hume still cool?
	beq.s	.ok
	lea	loop1(pc),a2
	move.l	a2,loop(a0)
	bra	findhume
.ok	move	y(a0),d1
	add	#12,d1
	sub	y(a2),d1
	bpl.s	.skip
	neg	d1
.skip	cmp	#4,d1
	bcc.s	.skip2
	;
	lea	loop3(pc),a2
	move.l	a2,loop(a0)
	clr.l	xs(a0)
	clr.l	ya(a0)
	clr.l	ys(a0)
	rts
	;
.skip2	move.l	#2<<16,d0
	jsr	limitys(a5)
	move.l	ya2(a0),d0
	move	y(a0),d1
	cmp	y(a2),d1
	bcs.s	.skip3
	neg.l	d0
.skip3	move.l	d0,ya(a0)
	;
	move	x(a0),d0
	sub	x(a2),d0
	and	#4095,d0
	beq.s	.skip4
	subq	#1,x(a0)
	cmp	#2048,d0
	bcs.s	.skip4
	addq	#2,x(a0)
.skip4	rts

loop1	;lander scanning for a hume
	;
	move.l	other(a0),d0
	bne.s	chkhume
findhume	move	#$101,d1
	jsr	findid(a5)
	move.l	d0,other(a0)
	beq.s	lmove
	move.l	d0,a2
	move	#$102,flags(a2)
	bra.s	hdone
chkhume	move.l	d0,a2
	cmp	#$102,flags(a2)
	bne.s	findhume
	;
hdone	move	x(a0),d0
	sub	x(a2),d0
	bpl.s	.skip
	neg	d0
.skip	cmp	#8,d0
	bcc.s	lmove
	;
	;above human! go in for the kill!
	;
	move.l	a0,other(a2)
	clr.l	xs(a2)
	clr.l	ys(a2)
	;
	clr.l	xs(a0)
	lea	loop2(pc),a2
	move.l	a2,loop(a0)
	bra	loop2
	;
lmove	tst.l	ys(a0)
	bne.s	.skip
	;
	;not moving up/down, check if we should...
	;
.skip3	jsr	getxdiff(a5)
	cmp	#352,d0
	bcc.s	.skip2
	;
	;OK, do it!
	;
	move	#8,timer(a0)
	move.l	ya2(a0),d0
	jmp	homeinya(a5)
	;
.skip2	clr.l	ya(a0)
	clr.l	ys(a0)
	rts
.skip	move.l	#2<<16,d0
	jsr	limitys(a5)
	subq	#1,timer(a0)
	beq.s	.skip3
	rts

shot1	lea	frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

data
numlanders	dc	0
dataf

anim1	incbin	lander1.anm

frag1	incbin	lander1.frags

	cnop	0,4
	dc.l	'BYE!'
finish	

