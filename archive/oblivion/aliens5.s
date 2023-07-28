
	include	header.s
	initalien	2,data,dataf,pergame,perpattern,perlife
	;
	dc.l	dy_frag1,dy_anim1,dy_name
	dc.l	hu_frag1,hu_anim1,hu_name
	dc.l	0,0,0
	dc.l	0,0,0
	;
dy_name	dc.b	'DYNAMO',0
hu_name	dc.b	'SPACE HUM',0
	even
	;
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
	moveq	#1,d2
	moveq	#25,d3
	moveq	#30,d4
	lea	message(pc),a0
	jsr	calcnum(a5)
	sne	bonus-data(a2)
	move	d0,dynamos-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
	rts
.skip	move	bonus(pc),d0
	beq.s	.done
	move	#$50,d0
	moveq	#10,d1
	lea	message2(pc),a0
	lea	dy_anim1(pc),a1
	lea	dy_frag1(pc),a2
	jmp	dobonus(a5)
.done	rts

message	dc.b	15,"SKILL WAVE - DYNAMO DEVASTATION",0
	even
message2	dc.b	"DYNAMO DEVASTATION",0
	even

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makedynamos
	rts

makedynamos	move	dynamos(pc),d7
	beq	.done
	subq	#1,d7
	jsr	rndoffsc(a5)
.loop	jsr	beginadd(a5)
	jsr	getoffsc(a5)
	move	#256,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	moveq	#1,d0
	moveq	#3,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	sub.l	#$c000,d0
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	moveq	#1,d0
	moveq	#2,d1
	moveq	#30,d2
	jsr	rndrange(a5)
	sub.l	#$8000,d0
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	;
	jsr	sysinitbull(a5)
	;
	move.b	#$16,col(a0)
	lea	dy_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	dy_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	dy_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	dy_anim1(pc),a2
	jsr	newanim(a5)
	lea	dy_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

makehum	move	x(a0),d6
	move	y(a0),d7
	lea	dy_sfx(pc),a2
	moveq	#72,d0
	moveq	#48,d1
	jsr	playsfx(a5)
	;
	jsr	beginadd(a5)
	move	d6,x(a0)
	move	d7,y(a0)
	moveq	#1,d0
	moveq	#5,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	sub.l	#$8000,d0
	move.l	d0,xs(a0)
	moveq	#1,d0
	moveq	#3,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	sub.l	#$8000,d0
	move.l	d0,ys(a0)
	;
	move.b	#$10,col(a0)
	lea	hu_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	hu_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	hu_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	hu_anim1(pc),a2
	jsr	newanim(a5)
	jmp	endadd(a5)

dy_loop1	subq	#1,bullt(a0)
	bpl.s	.done
	move	bullr(a0),d0
	jsr	rnd2(a5)
	move	d0,bullt(a0)
	move	onscreen(a5),d0
	beq.s	.done
	neg.l	xs(a0)
	jsr	rnd1(a5)
	tst	d0
	bpl.s	.skip
	neg.l	xs(a0)
	neg.l	ys(a0)
.skip	;launch a hum
	bra	makehum
.done	rts

dy_coll1	jsr	killplayer(a5)
	;
dy_shot1	lea	data(pc),a2
	subq	#1,dynamos-data(a2)
	subq	#1,tokill(a5)
	move	#$10,d0
	jsr	addscore(a5)
	lea	dy_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

hu_loop1	jsr	getxdiff(a5)
	cmp	#12,d0
	bcs.s	.skip
	move.l	xs(a0),d0
	jsr	homeinx(a5)
.skip	jsr	getydiff(a5)
	cmp	#12,d0
	bcs.s	.skip2
	move.l	ys(a0),d0
	jmp	homeiny(a5)
.skip2	rts

hu_coll1	jsr	killplayer(a5)
	;
hu_shot1	move	#$1,d0
	jsr	addscore(a5)
	lea	hu_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

dy_anim1	incbin	dynamo.anm
dy_frag1	incbin	dynamo.frags

hu_anim1	incbin	hum.anm
hu_frag1	incbin	hum.frags

dy_sfx	incbin	dynamo.bsx

data
dynamos	dc	0
bonus	dc	0
dataf

