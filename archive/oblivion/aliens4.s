
	include	header.s
	initalien	4,data,dataf,pergame,perpattern,perlife
	;
	dc.l	fb_frag1,fb_anim1,fb_name
	dc.l	ff_frag1,ff_anim1,ff_name
	dc.l	0,0,0
	dc.l	0,0,0
	;
fb_name	dc.b	'DIZZMO',0
ff_name	dc.b	'DIZETTE',0
	even

	rsreset	;
	rs.b	syssize	;my own rs's here...
fb_ys	rs.l	1	;peacful y speed
fb_ys2	rs.l	1	;aggro y speed
fb_fc	rs.w	1	;fire chance

	rsreset
	rs.b	syssize
ff_t	rs.w	1

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
	moveq	#3,d0
	moveq	#6,d1
	moveq	#2,d2
	moveq	#10,d3
	moveq	#25,d4
	lea	message(pc),a0
	jsr	calcnum(a5)
	sne	bonus-data(a2)
	move	d0,fbombers-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
	rts
	;
.skip	move	bonus(pc),d0
	beq.s	.done
	move	#$20,d0
	moveq	#10,d1
	lea	message2(pc),a0
	lea	fb_anim1(pc),a1
	lea	fb_frag1(pc),a2
	jmp	dobonus(a5)
.done	rts

message	dc.b	15,"SKILL WAVE - DIZZMO DOGFIGHT",0
	even
message2	dc.b	"FORMIDABLE FIREPOWER",0
	even

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makefbombers
	rts

makefbombers
	move	fbombers(pc),d7
	beq	.done
	subq	#1,d7
	jsr	rndoffsc(a5)
.loop	jsr	beginadd(a5)
	jsr	getoffsc(a5)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	jsr	rnd1(a5)
	or	#$8000,d0
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	moveq	#1,d0
	moveq	#2,d1
	moveq	#10,d2
	jsr	rndrange(a5)
	sub.l	#$8000,d0
	move.l	d0,fb_ys(a0)
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	moveq	#1,d0
	moveq	#4,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	move.l	d0,fb_ys2(a0)
	moveq	#1,d0
	moveq	#8,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	lsr.l	#6,d0
	move	d0,fb_fc(a0)
	;
	move.b	#$41,id(a0)
	move.b	#$12,col(a0)
	;
	lea	fb_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	fb_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	fb_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	fb_anim1(pc),a2
	jsr	newanim(a5)
	;
	lea	fb_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

makefbomb	move	x(a0),d6
	move	y(a0),d7
	addq	#3,d7
	jsr	beginadd(a5)
	move	d6,x(a0)
	move	d7,y(a0)
	move.l	#$60000,d0
	jsr	homeinx(a5)
	move.l	#$2000,ya(a0)
	;
	move	#64,ff_t(a0)
	move.b	#$60,col(a0)
	lea	ff_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	ff_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	ff_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	ff_anim1(pc),a2
	jsr	newanim(a5)
	jmp	endadd(a5)

fb_loop1	cmp.b	#$41,id(a0)
	beq.s	.peaceful
	;	
	;I'm aggro!
	;
	jsr	getydiff(a5)
	cmp	#64,d0
	bcs.s	.done
	;go peacful
	move.b	#$41,id(a0)
	move.l	fb_ys(a0),d0
	tst	ys(a0)
	bpl.s	.skip
	neg.l	d0
.skip	move.l	d0,ys(a0)
	rts
.done	move	onscreen(a5),d0
	beq.s	.done2
	jsr	rnd1(a5)
	cmp	fb_fc(a0),d0
	bcc.s	.done2
	bra	makefbomb
	;
.peaceful	;
	jsr	getydiff(a5)
	cmp	#32,d0
	bcc.s	.done2
	;
	;go aggro!
	;
	move.b	#$42,id(a0)
	move.l	fb_ys2(a0),d0
	tst	ys(a0)
	bmi.s	.skip2
	neg.l	d0
.skip2	move.l	d0,ys(a0)
.done2	rts

fb_coll1	jsr	killplayer(a5)
	;
fb_shot1	lea	data(pc),a2
	subq	#1,fbombers-data(a2)
	subq	#1,tokill(a5)
	move	#$25,d0
	jsr	addscore(a5)
	lea	fb_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme3(a5)

ff_loop1	subq	#1,ff_t(a0)
	bne.s	.done
	jmp	killme2(a5)
.done	rts

ff_coll1	jsr	killplayer(a5)
	;
ff_shot1	move	#$5,d0
	jsr	addscore(a5)
	lea	ff_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme3(a5)

fb_anim1	incbin	fbomber.anm
fb_frag1	incbin	fbomber.frags

ff_anim1	incbin	fbomb.anm
ff_frag1	incbin	fbomb.frags

data
fbombers	dc	0
bonus	dc	0
dataf

