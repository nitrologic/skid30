
	include	header.s
	initalien	12,data,dataf,pergame,perpattern,perlife
	;
	dc.l	pl_frag1,pl_anim1,pl_name
	dc.l	sp_frag1,sp_anim1,sp_name
	dc.l	0,0,0
	dc.l	0,0,0
	;
pl_name	dc.b	'PLASMOID',0
sp_name	dc.b	'SPARK',0
	even

	rsreset	
	rs.b	syssize
pl_sparks	rs.w	1	;number of sparks left
pl_tokill	rs.w	1	;number left to kill - <0 then none
			;launched.
pl_killed	rs.w	1	;self destruct flag...
pl_acnt	rs.w	1	;counter to new anim...

	rsreset	;
	rs.b	syssize	;my own rs's here...
sp_plasma	rs.l	1	;pointer to my plasma ball...

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	bne.s	.skip
	moveq	#1,d0
	moveq	#4,d1
	moveq	#16,d2
	moveq	#-1,d3
	jsr	calcnum(a5)
	move	d0,plasmas-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
.skip	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makeplasmas
	rts

makeplasmas	
	move	plasmas(pc),d7
	beq.s	.done
	subq	#1,d7
.loop	jsr	beginadd(a5)
	;
	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	;
	move	#32,pl_sparks(a0)
	clr	pl_tokill(a0)
	clr	pl_killed(a0)
	;
	move.b	#$22,col(a0)
	lea	pl_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	pl_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	pl_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	pl_anim1(pc),a2
	jsr	newanim(a5)
	;
	lea	pl_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

makesparks	;
	subq	#1,d7
	;
.loop	jsr	beginadd(a5)
	;
	move	sparkx(pc),d0
	bmi.s	.rnd
	move	d0,x(a0)
	move	sparky(pc),y(a0)
	bra.s	.nornd
	;
.rnd	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
.nornd	;
	moveq	#1,d0
	moveq	#3,d1
	moveq	#30,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	;
	moveq	#1,d0
	moveq	#3,d1
	moveq	#30,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	;
	move.l	(a1),sp_plasma(a0)
	;
	move.b	#$50,col(a0)
	lea	sp_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	sp_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	sp_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	sp_anim1(pc),a2
	jsr	newanim(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

pl_coll1	jsr	killplayer(a5)
	;
pl_shot1	;sparks...
	;
	lea	data(pc),a2
	move	x(a0),sparkx-data(a2)
	move	y(a0),sparky-data(a2)
	jsr	rnd1(a5)
	and	#3,d0
	addq	#2,d0
	move	d0,d7
	cmp	pl_sparks(a0),d7
	bcc.s	.skip
	;
	sub	d7,pl_sparks(a0)
	add	d7,pl_tokill(a0)
	bsr	makesparks
	move	#32,pl_acnt(a0)
	lea	pl_anim2(pc),a2
	jsr	newanim(a5)
	move	#$10,d0
	jmp	addscore(a5)
	;
.skip	;all sparks gone...
	;
	move	pl_sparks(a0),d7
	bsr	makesparks
	;
plasmashot	move	#$75,d0
	jsr	addscore(a5)
	lea	data(pc),a2
	subq	#1,plasmas-data(a2)
	subq	#1,tokill(a5)
	lea	pl_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

pl_loop1	tst	pl_killed(a0)
	bne.s	plasmashot
	subq	#1,pl_acnt(a0)
	bne.s	.skip
	lea	pl_anim1(pc),a2
	jmp	newanim(a5)
	;
.skip	rts

sp_loop1	move.l	xs(a0),d0
	jsr	homeinx(a5)
	move.l	ys(a0),d0
	jmp	homeiny(a5)

sp_coll1	jsr	killplayer(a5)
	;
sp_shot1	move.l	sp_plasma(a0),a2
	subq	#1,pl_tokill(a2)
	bne.s	.skip
	st	pl_killed(a2)
.skip	move	#$1,d0
	jsr	addscore(a5)
	lea	sp_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

pl_anim1	incbin	plasma1.anm
pl_frag1	incbin	plasma1.frags
pl_anim2	incbin	plasma2.anm
	;
sp_anim1	incbin	spark.anm
sp_frag1	incbin	spark.frags

data
plasmas	dc	0
sparkx	dc	0
sparky	dc	0
dataf

