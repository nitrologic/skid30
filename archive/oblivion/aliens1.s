
	include	header.s
	;
	initalien	1,data,dataf,pergame,perpattern,perlife
	;
	dc.l	hu_frag1,hu_anim3,hu_name
	dc.l	la_frag1,la_anim1,la_name
	dc.l	mu_frag1,mu_anim1,mu_name
	dc.l	0,0,0
	;
	opt	p+
	;
hu_name	dc.b	'HUMANOID',0
la_name	dc.b	'SCOUT',0
mu_name	dc.b	'MUTOID',0
	even
	;
	;create my own custom stuff...
	;
	;first, humes...
	;
	;ids:   1 = hume on ground
	;       2 = hume being descended on by lander
	;
	;
	rsreset	;humanoid data
	rs.b	syssize2
	;
hu_lander	rs.l	1
hu_xs	rs.l	1
hu_ys	rs.l	1
hu_hitechk	rs.w	1
hu_xo	rs.w	1
hu_yo	rs.w	1

	rsreset	;lander data
	rs.b	syssize2
	;
la_hume	rs.l	1
la_xs	rs.l	1
la_ya	rs.l	1
la_ys	rs.l	1
la_destx	rs.w	1
la_desty	rs.w	1	;actually, an offset from top of mounts
la_xs2	rs.l	1

	rsreset	;mutant data
	rs.b	syssize2

	rsreset	;bonus data
	rs.b	syssize
bo_t	rs.w	1	;timer

hite1	equ	6
hite2	equ	1

catch.sfx	incbin	catch.bsx
putdown.sfx	incbin	putdown.bsx
mutate.sfx	incbin	mutate.bsx

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	clr	inspace(a5)
	move	#-1,lastpat-data(a2)
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	;if mi=1 then this is a bonus check...
	;
	beq.s	.skip2		;not a bonus
	;
	bpl.s	.skip9
	move	#$20,d0
	move	humes(pc),d1
	beq.s	.skip3
	bra.s	.skip0
	;
.skip9	move	humes(pc),d1	;no bonus!
	beq.s	.skip3
	cmp	#6,d0		;pattern number
	bcs.s	.skip0
	moveq	#5,d0
.skip0	lsl	#4,d0		;bonus value
	lea	hu_name(pc),a0	;name of bonus
	lea	hu_anim3(pc),a1	;anim
	lea	hu_frag1(pc),a2	;frags
	jmp	dobonus(a5)		;hmmm...
.skip3	rts
	;
.skip2	;
	moveq	#0,d1
	move	d0,d1
	subq	#1,d1
	divu	#10,d1	;0,1,2,3....
	cmp	#4,d1
	bcs.s	.wok
	moveq	#3,d1
.wok	addq	#4,d1
	move	d1,warpnum-data(a2)
	;
	swap	d1
	tst	d1
	bne.s	.wokskip
	swap	d1
	;
	add	#48,d1
	lea	woktext(pc),a2
	move.b	d1,2(a2)
	move	d0,-(a7)
	jsr	scanprint(a5)
	move	(a7)+,d0
	lea	data(pc),a2
	;
.wokskip	move	#2,gates-data(a2)
	moveq	#0,d1
	move	d0,d1
	divu	#5,d1
	swap	d1
	tst	d1
	bne.s	.skipm
	;
	;skill patterns...
	;
	clr	inspace(a5)
	clr	humes-data(a2)
	clr	gates-data(a2)
	moveq	#0,d1
	move	d0,d1
	subq	#1,d1
	divu	#5,d1
	bra.s	.skipq
	;
.skipm	moveq	#0,d1
	move	d0,d1
	subq	#1,d1
	divu	#5,d1
	cmp	lastpat(pc),d1
	beq.s	.skipq
	;
.skipy	clr	inspace(a5)
	move	#10,humes-data(a2)
	;
.skipq	move	d1,lastpat-data(a2)
	;
	addq	#3,d0
	neg	d0
	move	d0,warpto-data(a2)
	;
	moveq	#3,d0
	moveq	#7,d1
	moveq	#1,d2
	moveq	#-1,d3
	jsr	calcnum(a5)
	lsl	#2,d0
	add	d0,tokill(a5)
	;
	clr	landers-data(a2)
	move	#1,warpt-data(a2)
	;
	move	d0,landersw-data(a2)
	clr	mutants-data(a2)
	tst	inspace(a5)
	beq.s	.skip
	clr	landersw-data(a2)
	move	d0,mutants-data(a2)
.skip	;
	moveq	#1,d0
	moveq	#3,d1
	moveq	#30,d2
	jsr	rndrange(a5)
	move.l	d0,maxys-data(a2)
	lea	perloop(pc),a0
	rts

woktext	dc.b	97,1,"4 HUMANOIDS FOR WARP",0
	even

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	clr	onship-data(a2)
	move	#-1,mutx-data(a2)
	bsr	makestargates
	bsr	makehumes
	move	landers(pc),d7
	bsr	makelanders
	move	mutants(pc),d7
	bsr	makemutants
	rts

perloop	;see if more landers need to be added
	;
	move	landersw(pc),d0
	beq.s	.skip2
	lea	data(pc),a2
	move	landers(pc),d0
	beq.s	.skip
	subq	#1,warpt-data(a2)
	bne.s	.skip2
.skip	;
	;add landers!
	;
	moveq	#6,d7	;how many to warp in...
	cmp	landersw(pc),d7
	bls.s	.skip3
	move	landersw(pc),d7
.skip3	sub	d7,landersw-data(a2)
	add	d7,landers-data(a2)
	move	#768,warpt-data(a2)
	tst	inspace(a5)
	beq.s	.skip4
	move	#1,warpt-data(a2)
.skip4	bsr	makelanders
.skip2	lea	data(pc),a2
	move	screenx(a5),d0
	addq	#8,d0
	move	d0,oldx-data(a2)
	move	#9,yoff-data(a2)
	rts

makelanders	subq	#1,d7
	bmi	.none
	;
.loop	jsr	beginadd(a5)
	;
	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	;
	move	#1024,d0
	jsr	rnd2(a5)
	add	#3072,d0
	move	d0,la_destx(a0)
	;
	move	#256,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	;
	moveq	#1,d0	;min x speed
	moveq	#4,d1	;max x speed...
	moveq	#30,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	move.l	d0,la_xs(a0)
	;
	moveq	#1,d0	;min x speed
	moveq	#5,d1	;max x speed...
	moveq	#30,d2
	jsr	rndrange(a5)
	tst	xs(a0)
	bmi.s	.skip
	neg.l	d0
.skip	move.l	d0,la_xs2(a0)
	;
	move	#$1000,d0
	move	#$4000,d1
	moveq	#10,d2
	jsr	rndrange(a5)
	clr	d0
	swap	d0
	move.l	d0,la_ya(a0)
	;
	jsr	sysinitbull(a5)
	;
	move	#192,d0
	jsr	rnd2(a5)
	add	#64,d0
	move	d0,la_desty(a0)
	clr.l	la_hume(a0)
	;
	move.b	#$44,col(a0)	;scanner colour
	lea	la_anim1(pc),a2
	jsr	newanim(a5)
	lea	la_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	la_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	la_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	la_frag1(pc),a2
	jsr	implode(a5)
	;
	jsr	endadd(a5)
	dbf	d7,.loop
.none	rts

makemutants	subq	#1,d7
	bmi	.none
	;
.loop	jsr	beginadd(a5)
	;
	move	mutx(pc),d0
	bmi.s	.makernd
	;
	move	d0,x(a0)
	move	muty(pc),y(a0)
	bra.s	.nornd
	;
.makernd	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	;
	move	#256,d0
	jsr	rnd2(a5)
	add	#128,d0
	move	d0,y(a0)
	;
.nornd	moveq	#2,d0	;min x speed
	moveq	#6,d1	;max x speed...
	moveq	#40,d2
	jsr	rndrange(a5)
	move.l	d0,xs(a0)
	;
	moveq	#1,d0	;min y speed
	moveq	#5,d1	;max y speed...
	moveq	#40,d2
	jsr	rndrange(a5)
	move.l	d0,ys(a0)
	;
	jsr	sysinitbull(a5)
	lsr	bullr(a0)
	;
	move.b	#$15,col(a0)	;scanner colour
	lea	mu_anim1(pc),a2
	jsr	newanim(a5)
	lea	mu_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	mu_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	mu_shot1(pc),a2
	move.l	a2,shot(a0)
	move	mutx(pc),d0
	bpl.s	.noimp
	;
	lea	mu_frag1(pc),a2
	jsr	implode(a5)
	;
.noimp	jsr	endadd(a5)
	;
	dbf	d7,.loop
.none	rts

makestargates
	move	gates(pc),d7
	beq.s	.done
	subq	#1,d7
	move	#1024+512,d6
.loop	jsr	beginadd(a5)
	;
	move	d6,x(a0)
	move	#256,d0
	jsr	rnd2(a5)
	add	#128,d0
	move	d0,y(a0)
	;
	move.b	#$55,col(a0)
	lea	sg_anim1(pc),a2
	jsr	newanim(a5)
	lea	sg_coll1(pc),a2
	move.l	a2,collide(a0)
	;
	jsr	endadd(a5)
	;
	add	#2048,d6
	dbf	d7,.loop
.done	rts

makehumes	move	humes(pc),d7	;number of humes...
	beq	.done
	subq	#1,d7
	;
.loop	jsr	beginadd(a5)
	;
	move	#1024,d0
	jsr	rnd2(a5)
	add	#1024,d0
	move	d0,x(a0)
	jsr	getmounty(a5)
	move	d0,y(a0)
	;
	jsr	rnd1(a5)
	and	#$7fff,d0
	or	#$4000,d0
	jsr	rnd3(a5)
	move.l	d0,hu_xs(a0)
	move.l	d0,xs(a0)
	swap	d0
	move	d0,facing(a0)
	;
	jsr	rnd1(a5)
	and	#$1fff,d0
	or	#$1000,d0
	move.l	d0,hu_ys(a0)
	move	#hite1,hu_hitechk(a0)
	clr.l	hu_lander(a0)
	;
	move.b	#$11,col(a0)
	or.b	#$30,flags(a0)
	move.b	#1,id(a0)
	lea	hu_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	hu_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	hu_anim1(pc),a2
	jsr	newanim(a5)
	;
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

makebonus	;a2=anim
	;
	move	x(a0),d6
	move	y(a0),d7
	jsr	beginadd(a5)
	move	d6,x(a0)
	move	d7,y(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	add.l	sxspeed(a5),d0
	move.l	d0,xs(a0)
	jsr	rnd1(a5)
	neg.l	d0
	move.l	d0,ys(a0)
	move	#64,bo_t(a0)
	clr.b	col(a0)
	jsr	newanim(a5)
	lea	bo_loop1(pc),a2
	move.l	a2,loop(a0)
	jmp	endadd(a5)

bo_loop1	subq	#1,bo_t(a0)
	beq.s	.done
	rts
.done	jmp	killme2(a5)

hu_loop1	cmp	#512-224,y(a0)
	bgt.s	.bye
	move	#511,y(a0)
	;
.bye	move	x(a0),d0
	jsr	getmounty(a5)
	move	y(a0),d1		;where I am!
	sub	d0,d1
	move	d1,d2
	bpl.s	.skip
	neg	d2
.skip	cmp	hu_hitechk(a0),d2
	bls.s	.golr
	;
.goud	;move.l	xs(a0),d0
	;beq.s	.done
	;
	move.l	hu_ys(a0),d0
	tst	d1
	bmi.s	.skip2
	neg.l	d0
.skip2	clr.l	xs(a0)
	move.l	d0,ys(a0)
	move	#hite2,hu_hitechk(a0)
	lea	hu_anim2(pc),a2
	jsr	newanim(a5)
.done	rts
	;
.golr	move.l	ys(a0),d0
	beq.s	.chkx
	move.l	hu_xs(a0),xs(a0)
	clr.l	ys(a0)
	move	#hite1,hu_hitechk(a0)
	lea	hu_anim1(pc),a2
	jsr	newanim(a5)
	rts
.chkx	jsr	getrealx(a5)
	cmp	#1024,d0
	ble.s	.gorite
	cmp	#1024+1024,d0
	bge.s	.goleft
	rts
.gorite	move.l	xs(a0),d0
	bpl.s	.done
.neg	neg.l	d0
	move.l	d0,xs(a0)
	move.l	d0,hu_xs(a0)
	swap	d0
	move	d0,facing(a0)
	rts
.goleft	move.l	xs(a0),d0
	bpl.s	.neg
	rts

hu_loop3	;going up with lander
	;
	move.l	hu_lander(a0),a2
	;
	move	x(a2),d0
	add	hu_xo(a0),d0
	move	d0,x(a0)
	;
	move	y(a2),d1
	add	hu_yo(a0),d1
	move	d1,y(a0)
	;
	rts

hu_loop6	;on player...
	;
	lea	oldx(pc),a2
	move	oldx(pc),d0
	move	x(a0),(a2)+
	move	shipy(a5),d1
	add	(a2),d1
	addq	#8,(a2)
	;
	move	d0,x(a0)
	move	d1,y(a0)
	move	#$50,d7
	lea	b500_anim(pc),a4
	bra	hu_loop5_2
	;
hu_loop5	;after parachute!
	;
	move	#$25,d7
	lea	b250_anim(pc),a4
	;
hu_loop5_2	move	x(a0),d0
	jsr	getmounty(a5)
	cmp	y(a0),d0
	bcs.s	.skip
	rts
	;
.skip	;hit ground! - d7 = bonus, a4 = anim
	;
	cmp	#$50,d7
	bne.s	.skipz
	lea	putdown.sfx(pc),a2
	moveq	#102,d0
	moveq	#64,d1
	jsr	playsfx(a5)
	lea	data(pc),a2
	subq	#1,onship-data(a2)
.skipz	jsr	getrealx(a5)
	cmp	#3072,d0
	bcs.s	.humeok
	;
	;mutate routine!
	;
	lea	muttext(pc),a2
	jsr	scanprint(a5)
	lea	data(pc),a2
	moveq	#1,d7
	add	d7,mutants-data(a2)
	addq	#1,tokill(a5)
	move	x(a0),mutx-data(a2)
	move	y(a0),muty-data(a2)
	bsr	makemutants
	lea	mutate.sfx(pc),a2
	moveq	#104,d0
	moveq	#64,d1
	jsr	playsfx(a5)
	bra	hu_shot1
	;
.humeok	move	d7,d0
	jsr	addscore(a5)
	move.l	a4,a2
	bsr	makebonus
	clr.l	collide(a0)
	clr.l	ya(a0)
	clr.l	ys(a0)
	move.b	#1,id(a0)
	move.l	hu_xs(a0),xs(a0)
	move	#hite1,hu_hitechk(a0)
	lea	hu_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	hu_shot1(pc),a2
	move.l	a2,shot(a0)
	clr.l	collide(a0)
	lea	hu_anim1(pc),a2
	jmp	newanim(a5)

muttext	dc.b	96,0,'HUMANOID MUTATED!',0
	even

hu_loop4	;before parachute!
	;
	move	x(a0),d0
	jsr	getmounty(a5)
	cmp	y(a0),d0
	bcc.s	.skip
	;
	;hume hit ground!
	;
	bra	hu_shot1
	;
.skip	cmp.l	#4<<16,ys(a0)
	blt.s	.skip2
	;
	;open chute!
	;
	clr.l	ya(a0)
	move	#2,ys(a0)
	lea	hu_anim4(pc),a2
	jsr	newanim(a5)
	lea	hu_loop5(pc),a2
	move.l	a2,loop(a0)
	;
.skip2	rts

angeltext	dc.b	96,0,'BEWARE - AVENGING ANGEL!',0
	even

hu_shot1	lea	humes(pc),a2
	subq	#1,(a2)
	bne.s	.nosp
	;
	jsr	gospace(a5)
	;
.nosp	cmp.b	#1,id(a0)
	beq.s	.skip
	lea	hu_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme3(a5)
.skip	;
	;make an angel!
	;
	lea	angeltext(pc),a2
	jsr	scanprint(a5)
	clr.b	id(a0)
	clr.l	xs(a0)	
	clr.l	ys(a0)
	move.l	#-$1000,ya(a0)
	lea	an_loop1(pc),a2
	move.l	a2,loop(a0)
	clr.l	shot(a0)
	clr.l	collide(a0)
	;
	move	#6,bullr(a0)
	move	#3,bullt(a0)
	;
	lea	an_anim1(pc),a2
	jsr	newanim(a5)
	lea	hu_frag1(pc),a2
	jmp	explode(a5)

an_loop1	;angel loop!
	;
	cmp	#64,y(a0)
	bcs.s	.skip
	jmp	sysfirekill(a5)
.skip	jmp	killme(a5)

hu_coll1	;collided with player!
	;
	lea	catch.sfx(pc),a2
	moveq	#103,d0
	moveq	#64,d1
	jsr	playsfx(a5)
	;
	move	onship(pc),d7
	cmp	#4,d7
	bcs.s	.skip
	moveq	#3,d7
.skip	lsl	#2,d7
	lea	pickups(pc),a2
	move	0(a2,d7),d0		;score...
	add	2(a2,d7),a2
	move.l	a2,-(a7)
	jsr	addscore(a5)
	move.l	(a7)+,a2
	bsr	makebonus
	;
	clr.l	ya(a0)
	clr.l	ys(a0)
	lea	hu_anim3(pc),a2
	jsr	newanim(a5)
	lea	hu_loop6(pc),a2
	move.l	a2,loop(a0)
	clr.l	shot(a0)
	clr.l	collide(a0)
	lea	data(pc),a2
	addq	#1,onship-data(a2)
	move	warpnum(pc),d0
	sub	onship(pc),d0
	beq.s	.warpok
	bmi.s	.done
	add	#48,d0
	lea	warptext1(pc),a2
	move.b	d0,2(a2)
	jmp	scanprint(a5)
.warpok	lea	warptext2(pc),a2
	jmp	scanprint(a5)
.done	rts

warptext1	dc.b	96,0,'3 HUMANOID)S( TO WARP',0
warptext2	dc.b	96,1,'WARP AVAILABLE!',0
	even

la_loop3	;going up with hume
	;
	bsr	chkspace
	jsr	sysfirebull(a5)
	move.l	maxys(pc),d0
	jsr	limitys(a5)
	;
	move	x(a0),d0
	add	shipx(a5),d0
	and	#4095,d0
	sub	la_destx(a0),d0
	bpl.s	.skip
	neg	d0
.skip	cmp	#8,d0
	bcc.s	.skip2
	lsl	bullr(a0)
	;
	;drop hume off...
	;
	bsr	drophume
	;
	move.l	la_xs(a0),xs(a0)
	move.l	la_ya(a0),ya(a0)
	lea	la_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	la_shot1(pc),a2
	move.l	a2,shot(a0)
	rts
	;
.skip2	move	y(a0),d1
	sub	la_desty(a0),d1
	move	d1,d2
	bpl.s	.skip4
	neg	d2
.skip4	cmp	#8,d2
	bcc.s	.uptoy
	clr.l	ya(a0)
	clr.l	ys(a0)
	rts
	;
.uptoy	move.l	la_ya(a0),ya(a0)
	neg.l	ya(a0)
	rts

la_loop2	;going in for humanoid
	;
	bsr	chkspace
	jsr	sysfirebull(a5)
	move.l	la_hume(a0),a2
	cmp.b	#2,id(a2)
	beq.s	.skip
	;
	;hume gone!
	;
	clr.l	la_hume(a0)
	;
	lsl	bullr(a0)
	move.l	la_xs,xs(a0)
	lea	la_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	la_shot1(pc),a2
	move.l	a2,shot(a0)
	rts
	;
.skip	move	y(a0),d0
	add	#12,d0
	sub	y(a2),d0
	move	d0,d1
	bpl.s	.skip2
	neg	d1
.skip2	cmp	#4,d1
	bcs.s	.skip3
	move.l	la_ya(a0),d1
	tst	d0
	bmi.s	.skip4
	neg.l	d1
.skip4	move.l	d1,ya(a0)
	move.l	maxys(pc),d0
	jsr	limitys(a5)
	;
	move	x(a0),d0
	lsr	#2,d0
	move	x(a2),d1
	lsr	#2,d1
	cmp	d1,d0
	beq.s	.skip5
	subq	#1,x(a0)
	cmp	d1,d0
	bgt.s	.skip5
	addq	#2,x(a0)
.skip5	rts
	;
.skip3	;at hume!
	;
	move.l	la_xs2(a0),xs(a0)
	move.l	la_ya(a0),ya(a0)
	neg.l	ya(a0)
	clr.l	ys(a0)
	move.l	a0,hu_lander(a2)
	move	x(a2),d0
	sub	x(a0),d0
	move	d0,hu_xo(a2)
	move	y(a2),d1
	sub	y(a0),d1
	move	d1,hu_yo(a2)
	lea	hu_loop3(pc),a3
	move.l	a3,loop(a2)
	;
	lea	la_shot2(pc),a2
	move.l	a2,shot(a0)
	lea	la_loop3(pc),a2
	move.l	a2,loop(a0)
	lea	pickup.sfx(pc),a2
	moveq	#120,d0
	moveq	#64,d1
	jmp	playsfx(a5)

chkspace	tst	inspace(a5)
	beq.s	.done
	lea	data(pc),a2
	subq	#1,landers-data(a2)
	addq	#1,mutants-data(a2)
	move	x(a0),mutx-data(a2)
	move	y(a0),muty-data(a2)
	move	#1,warpt-data(a2)
	moveq	#1,d7
	bsr	makemutants
	jmp	killme(a5)
.done	rts

la_loop1	;lander scanning for a hume
	;
	bsr	chkspace
	jsr	sysfirebull(a5)
	;
.nofire	move.l	la_hume(a0),d0
	beq.s	.findhume
	;
.chkhume	move.l	d0,a2
	cmp.b	#1,id(a2)
	bne.s	.findhume
	;
	;hume OK!
	;
	move	x(a0),d0
	sub	x(a2),d0
	and	#4095,d0
	cmp	#16,d0
	bcc.s	la_move1
	;
	;at hume, prepare hume...
	;
	move.l	a2,a0
	;
	clr.l	xs(a0)
	clr.l	ys(a0)
	move.b	#2,id(a0)
	clr.l	loop(a0)
	lea	hu_anim3(pc),a2
	jsr	newanim(a5)
	;
	move.l	(a1),a0
	;
	;prepare lander...
	;
	lsr	bullr(a0)
	clr.l	xs(a0)
	lea	la_shot3(pc),a2
	move.l	a2,shot(a0)
	lea	la_loop2(pc),a2
	move.l	a2,loop(a0)
	rts
	;
.findhume	moveq	#1,d1
	jsr	findid(a5)
	move.l	d0,la_hume(a0)
	bne.s	la_move1
	move	onship(pc),d0
	bne.s	la_move1
	;
	lea	humedang(pc),a2
	jsr	scanprint(a5)
	;
la_move1	move.l	maxys(pc),d0
	jsr	limitys(a5)
	;
	;home in to desty above mountains.
	;
	move	x(a0),d0
	jsr	getmounty(a5)
	sub	la_desty(a0),d0	;home in point...
	move.l	la_ya(a0),d1
	bpl.s	.skip
	neg.l	d1
.skip	cmp	y(a0),d0
	bgt.s	.skip2
	neg.l	d1
.skip2	move.l	d1,ya(a0)
	rts

humedang	dc.b	100,1,"PLANET IN DANGER!",0
	even

drophume	move.l	la_hume(a0),a2
	cmp.b	#2,id(a2)
	bne.s	.done
	;
	clr.l	la_hume(a0)
	;
	move.l	a2,a0
	;
	move.l	#$2000,ya(a0)
	lea	hu_loop4(pc),a2
	move.l	a2,loop(a0)
	lea	hu_coll1(pc),a2
	move.l	a2,collide(a0)
	;
	move.l	(a1),a0
	;
.done	rts

la_shot3	;shot lander going down for hume!
	;
	move.l	la_hume(a0),a2
	cmp.b	#2,id(a2)
	bne	la_shot1
	;
	move.l	a2,a0
	;
	move.b	#1,id(a0)
	move.l	hu_xs(a0),xs(a0)
	move	#hite1,hu_hitechk(a0)
	lea	hu_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	hu_anim1(pc),a2
	jsr	newanim(a5)
	;
	move.l	(a1),a0
	;
	bra	la_shot1

la_shot2	;shot lander picking up hume!
	;
	move.l	la_hume(a0),a2
	cmp.b	#2,id(a2)
	bne.s	.skip
	;
	lea	fall.sfx(pc),a2
	moveq	#112,d0
	moveq	#64,d1
	jsr	playsfx(a5)
	;
.skip	bsr	drophume
	;
la_shot1	move	#$15,d0
	jsr	addscore(a5)
	lea	la_frag1(pc),a2
	jsr	explode(a5)
	lea	data(pc),a2
	subq	#1,landers-data(a2)
	subq	#1,tokill(a5)
	move	landersw(pc),d0
	or	landers(pc),d0
	bne.s	.skip
	lea	landertext(pc),a2
	jsr	scanprint(a5)
.skip	jmp	killme(a5)

landertext	dc.b	97,1,'ALL SCOUTS DESTROYED!',0

la_coll1	jsr	killplayer(a5)
	bra	la_shot1

sg_coll1	move	onship(pc),d0
	cmp	warpnum(pc),d0
	bcs.s	.nowarp
	move	warpto(pc),d0
	jmp	warpplayer(a5)
.nowarp	move	x(a0),d0
	add	#2048+64,d0
	move	shipdir(a5),d1
	beq.s	.skip
	sub	#4096+128,d0
.skip	and	#4095,d0
	jmp	warpplayer(a5)

mu_loop1	;OK, how is our mutant going to work?
	;
	jsr	sysfirebull(a5)
	jsr	getxdiff(a5)
	cmp	#16,d0
	bcs.s	.nohomein
	move.l	xs(a0),d0
	jsr	homeinx(a5)	;cool
.nohomein	;
	jsr	chkinfront(a5)
	bne.s	.infront
	;
	;behind player!
	;
.to	move.l	ys(a0),d0
	jmp	homeiny(a5)
.infront	jsr	getxdiff(a5)
	cmp	#32,d0
	bcs.s	.to
	jsr	getydiff(a5)
	cmp	#32,d0
	bcs.s	.away
	move.l	ys(a0),d0
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	rts
.away	move.l	ys(a0),d0
	jmp	homeawayy(a5)

mu_coll1	jsr	killplayer(a5)
	;
mu_shot1	move	#$25,d0
	jsr	addscore(a5)
	lea	mu_frag1(pc),a2
	jsr	explode(a5)
	lea	data(pc),a2
	subq	#1,mutants-data(a2)
	subq	#1,tokill(a5)
	jmp	killme(a5)

hu_anim1	incbin	hume1.anm
hu_anim2	incbin	hume2.anm
hu_anim3	incbin	hume3.anm
hu_anim4	incbin	hume4.anm
hu_frag1	incbin	hume1.frags
	;
la_anim1	incbin	lander1.anm
la_frag1	incbin	lander1.frags

sg_anim1	incbin	stargate.anm

mu_anim1	incbin	mutant1.anm

mu_frag1	incbin	mutant1.frags

an_anim1	incbin	angel.anm

pickups	dc	$50,b500_anim-pickups
	dc	$100,b1000_anim-pickups
	dc	$150,b1500_anim-pickups
	dc	$200,b2000_anim-pickups

b250_anim	incbin	250.anm
b500_anim	incbin	500.anm
b1000_anim	incbin	1000.anm
b1500_anim	incbin	1500.anm
b2000_anim	incbin	2000.anm


onship	dc	0	;how many humes on the ship!
oldx	dc	0
yoff	dc	0
mutx	dc	0	;-1 = make mutants X random, else use this...
muty	dc	0	;y for mutant
;
data
humes	dc	0
landers	dc	0	;landers warped in.
mutants	dc	0
landersw	dc	0	;landers yet to warp in.
warpt	dc	0	;timer for warp in.
maxys	dc.l	0
warpto	dc	0
lastpat	dc	0	;last pattern at (for #humes after warp)
gates	dc	0	;how many stargates
warpnum	dc	0	;how many humes for warp...
dataf

pickup.sfx	incbin	pickup.bsx
fall.sfx	incbin	fall.bsx

