
	include	header.s
	initalien	8,data,dataf,pergame,perpattern,perlife
	;
	dc.l	st_frag1,st_anim1,st_name
	dc.l	0,0,0
	dc.l	0,0,0
	dc.l	0,0,0
	;
st_name	dc.b	'NEBULAX',0
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
	moveq	#1,d0
	moveq	#3,d1
	moveq	#4,d2
	moveq	#15,d3
	moveq	#20,d4
	sub.l	a0,a0
	jsr	calcnum(a5)
	move	d0,stars-data(a2)
	add	d0,tokill(a5)
	lea	perloop(pc),a0
.skip	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makestars
	rts

perloop	;a5=libbase, a6=chipbase
	;
	rts

makestars	move	stars(pc),d7
	beq.s	.done
	subq	#1,d7
.loop	jsr	beginadd(a5)
	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	;
	moveq	#1,d0
	moveq	#4,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	move.l	d0,xs(a0)
	moveq	#1,d0
	moveq	#3,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	sub.l	#$8000,d0
	move.l	d0,ys(a0)
	;
	jsr	sysinitbull(a5)
	;
	move.b	#$15,col(a0)
	lea	st_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	st_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	st_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	st_anim1(pc),a2
	jsr	newanim(a5)
	lea	st_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

st_loop1	jsr	sysfirebull(a5)
	move.l	xs(a0),d0
	jsr	homeinx(a5)
	move.l	ys(a0),d0
	tst	onscreen(a5)
	bne.s	.skip
	jmp	homeiny(a5)
.skip	jmp	homeawayy(a5)

st_coll1	jsr	killplayer(a5)
	;
st_shot1	lea	data(pc),a2
	subq	#1,stars-data(a2)
	subq	#1,tokill(a5)
	move	#$20,d0
	jsr	addscore(a5)
	lea	st_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

st_anim1	incbin	star.anm
st_frag1	incbin	star.frags

data
stars	dc	0
dataf

