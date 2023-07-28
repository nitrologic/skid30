
	include	header.s
	initalien	10,data,dataf,pergame,perpattern,perlife
	;
	dc.l	bu_frag1,bu_anim1,bu_name
	dc.l	du_frag1,du_anim1,du_name
	dc.l	0,0,0
	dc.l	0,0,0
	;
bu_name	dc.b	'BUS',0
du_name	dc.b	'SPACE DUCK',0
	even

	rsreset	;
	rs.b	syssize2	;my own rs's here...

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	bne.s	.skip
	moveq	#2,d0
	moveq	#4,d1
	moveq	#6,d2
	moveq	#20,d3
	moveq	#5,d4
	sub.l	a0,a0
	jsr	calcnum(a5)
	move	d0,buss-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
.skip	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makebuss
	rts

makebuss	move	buss(pc),d7
	beq.s	.done
	subq	#1,d7
.loop	jsr	beginadd(a5)
	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512-48,d0
	jsr	rnd2(a5)
	add	#16,d0
	move	d0,y(a0)
	;
	moveq	#1,d0
	moveq	#8,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	move	xs(a0),facing(a0)
	;
	jsr	sysinitbull(a5)
	;
	move.b	#$22,col(a0)
	lea	bu_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	bu_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	bu_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	bu_anim1(pc),a2
	jsr	newanim(a5)
	lea	bu_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

makeduck	;make a space duck
	;
	move.l	a0,a2
	jsr	beginadd(a5)
	move	x(a2),x(a0)
	move	y(a2),y(a0)
	;
	moveq	#2,d0
	moveq	#4,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	move	xs(a0),facing(a0)
	;
	moveq	#1,d0
	moveq	#2,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	;
	jsr	sysinitbull(a5)
	;
	move.b	#$35,col(a0)
	lea	du_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	du_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	du_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	du_anim1(pc),a2
	jsr	newanim(a5)
	jmp	endadd(a5)

bu_loop1	subq	#1,bullt(a0)
	bpl.s	.skip
	move	bullr(a0),d0
	jsr	rnd2(a5)
	move	d0,bullt(a0)
	move	onscreen(a5),d0
	beq.s	.skip
	bra	makeduck
.skip	rts

bu_coll1	jsr	killplayer(a5)
	;
bu_shot1	lea	data(pc),a2
	subq	#1,buss-data(a2)
	subq	#1,tokill(a5)
	move	#$25,d0
	jsr	addscore(a5)
	lea	bu_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

du_loop1	moveq	#2,d7
	jsr	sysfiremiss(a5)
	move	xs(a0),facing(a0)
	jsr	getxdiff(a5)
	cmp	#64,d0
	bcs.s	.skip
	move.l	xs(a0),d0
	jsr	homeinx(a5)
.skip	jsr	getydiff(a5)
	cmp	#64,d0
	bcs.s	.skip2
	move.l	ys(a0),d0
	jmp	homeiny(a5)
.skip2	rts

du_coll1	jsr	killplayer(a5)
	;
du_shot1	move	#$5,d0
	jsr	addscore(a5)
	lea	du_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

bu_anim1	incbin	bus.anm
bu_frag1	incbin	bus.frags

du_anim1	incbin	duck.anm
du_frag1	incbin	duck.frags

data
buss	dc	0
dataf

