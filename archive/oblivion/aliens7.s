
	include	header.s
	initalien	14,data,dataf,pergame,perpattern,perlife
	;
	dc.l	ba_frag1,ba_anim1,ba_name
	dc.l	ph_frag1,ph_anim1,ph_name
	dc.l	mu_frag1,mu_anim1,mu_name
	dc.l	0,0,0
	;
ba_name	dc.b	'BAITER',0
ph_name	dc.b	'PHRANK',0
mu_name	dc.b	'SNAK',0
	even

	rsreset	;
	rs.b	syssize2	;my own rs's here...
ba_t	rs.w	1

	rsreset	;
	rs.b	syssize2
ph_t	rs.w	1

	rsreset
	rs.b	syssize
mu_t	rs.w	1
mu_frag	rs.l	1

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	bne.s	.done
	clr	nextph-data(a2)
	subq	#1,d0
	cmp	#13,d0
	bcs.s	.skip
	moveq	#12,d0
.skip	move	d0,maxas-data(a2)
	move	#4608,next-data(a2)
	lea	perloop(pc),a0
.done	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perloop	;a5=libbase, a6=chipbase
	;
	lea	data(pc),a2
	subq	#1,next-data(a2)
	bne.s	.done
	;
	move	#64,d0
	move	#768,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	swap	d0
	move	#768+64,d1
	sub	d0,d1
	move	d1,next-data(a2)
	bsr	makebaiter
	lea	data(pc),a2
	;
.done	move	tokill(a5),d0	
	cmp	maxas(pc),d0
	bcc.s	.done2
	;
	;launch a phred?
	;
	subq	#1,nextph-data(a2)
	bpl.s	.done2
	move	#64,d0
	move	#768,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	swap	d0
	move	#768+64,d1
	sub	d0,d1
	move	d1,nextph-data(a2)
	bra	makephred
.done2	;
	rts
	
makebaiter	jsr	beginadd(a5)
	move	#352,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	;
	clr	ba_t(a0)
	jsr	sysinitbull(a5)
	asr	bullr(a0)
	;
	or.b	#$40,flags(a0)
	move.b	#$20,col(a0)
	lea	ba_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	ba_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	ba_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	ba_anim1(pc),a2
	jsr	newanim(a5)
	lea	ba_frag1(pc),a2
	jsr	implode(a5)
	jmp	endadd(a5)

makephred	jsr	beginadd(a5)
	move	#352,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	;
	clr	ph_t(a0)
	jsr	sysinitbull(a5)
	;
	or.b	#$40,flags(a0)
	move.b	#$20,col(a0)
	lea	ph_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	ph_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	ph_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	ph_anim1(pc),a2
	jsr	newanim(a5)
	lea	ph_frag1(pc),a2
	jsr	implode(a5)
	jmp	endadd(a5)

makemunch	move.l	a0,a2
	jsr	beginadd(a5)
	move	x(a2),x(a0)
	move	y(a2),y(a0)
	;
	clr	mu_t(a0)
	;
	or.b	#$40,flags(a0)
	move.b	#$20,col(a0)
	lea	mu_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	mu_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	mu_coll1(pc),a2
	move.l	a2,collide(a0)
	jsr	rnd1(a5)
	and	#3,d0
	mulu	#mu_anim2-mu_anim1,d0
	lea	mu_anim1(pc),a2
	add	d0,a2
	lea	mu_frag1-mu_anim1(a2),a3
	move.l	a3,mu_frag(a0)
	jsr	newanim(a5)
	jmp	endadd(a5)

ba_loop1	jsr	sysfirebull(a5)
	subq	#1,ba_t(a0)
	bpl.s	.done
	moveq	#16,d0
	moveq	#64,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	swap	d0
	moveq	#64+16,d1
	sub	d0,d1
	move	d1,ba_t(a0)
	move	x(a0),d0
	move	y(a0),d1
	move	screenx(a5),d2
	move	shipy(a5),d3
	jsr	homecalc(a5)
	add.l	d4,d4
	add.l	d5,d5
	add.l	sxspeed(a5),d4
	clr.b	d4
	clr.b	d5
	move.l	d4,xs(a0)
	move.l	d5,ys(a0)
.done	rts

ba_coll1	jsr	killplayer(a5)
	;
ba_shot1	move	#$20,d0
	jsr	addscore(a5)
	lea	ba_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

ph_loop1	subq	#1,bullt(a0)
	bpl.s	.skip
	move	bullr(a0),d0
	jsr	rnd2(a5)
	move	d0,bullt(a0)
	bsr	makemunch
.skip	subq	#1,ph_t(a0)
	bpl.s	.done
	moveq	#16,d0
	moveq	#64,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	swap	d0
	moveq	#64+16,d1
	sub	d0,d1
	move	d1,ph_t(a0)
	move	x(a0),d0
	move	y(a0),d1
	move	screenx(a5),d2
	move	shipy(a5),d3
	jsr	homecalc(a5)
	add.l	d4,d4
	add.l	d5,d5
	add.l	sxspeed(a5),d4
	and.l	#$fffff000,d4
	and.l	#$fffff000,d5
	move.l	d4,xs(a0)
	move	xs(a0),facing(a0)
	move.l	d5,ys(a0)
.done	rts

ph_coll1	jsr	killplayer(a5)
	;
ph_shot1	move	#$20,d0
	jsr	addscore(a5)
	lea	ph_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

mu_loop1	subq	#1,mu_t(a0)
	bpl.s	.done
	moveq	#32,d0
	move	#128,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	swap	d0
	move	#32+128,d1
	sub	d0,d1
	move	d1,mu_t(a0)
	move	x(a0),d0
	move	y(a0),d1
	move	screenx(a5),d2
	move	shipy(a5),d3
	jsr	homecalc(a5)
	add.l	d4,d4
	add.l	d5,d5
	add.l	sxspeed(a5),d4
	and.l	#$ffffc000,d4
	and.l	#$ffffc000,d5
	move.l	d4,xs(a0)
	move	xs(a0),facing(a0)
	move.l	d5,ys(a0)
.done	rts

mu_coll1	jsr	killplayer(a5)
	;
mu_shot1	move	#$5,d0
	jsr	addscore(a5)
	move.l	mu_frag(a0),a2
	jsr	explode(a5)
	jmp	killme(a5)

ba_anim1	incbin	baiter.anm
ba_frag1	incbin	baiter.frags

ph_anim1	incbin	phred.anm
ph_frag1	incbin	phred.frags

mu_anim1	incbin	munch1.anm
mu_frag1	incbin	munch1.frags
mu_anim2	incbin	munch2.anm
mu_frag2	incbin	munch2.frags
mu_anim3	incbin	munch3.anm
mu_frag3	incbin	munch3.frags
mu_anim4	incbin	munch4.anm
mu_frag4	incbin	munch4.frags

data
next	dc	0	;delay to next!
nextph	dc	0
maxas	dc	0	;when tokill gets below this, launch
			;phreds...
dataf

