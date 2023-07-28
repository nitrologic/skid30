
	include	header.s
	initalien	9,data,dataf,pergame,perpattern,perlife
	;
	dc.l	sp_frag1,sp_anim1,sp_name
	dc.l	0,0,0
	dc.l	0,0,0
	dc.l	0,0,0
	;
sp_name	dc.b	'ROTOX',0
	even

	rsreset	;
	rs.b	syssize	;my own rs's here...
sp_ys	rs.l	1	;peacful y speed
sp_ys2	rs.l	1	;aggro y speed
sp_xs	rs.l	1	;x speed
sp_xs2	rs.l	1	;aggro xspeed

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
	moveq	#3,d2
	moveq	#5,d3
	moveq	#7,d4
	sub.l	a0,a0
	jsr	calcnum(a5)
	move	d0,spinners-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
.skip	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makespinners
	rts

makespinners
	move	spinners(pc),d7
	beq	.done
	subq	#1,d7
.loop	jsr	beginadd(a5)
	move	#4096-(352*3),d0
	jsr	rnd2(a5)
	add	#352*2,d0
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	move.l	#$8000,d0
	move.l	d0,sp_xs(a0)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	moveq	#1,d0
	moveq	#3,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	move.l	d0,sp_xs2(a0)
	moveq	#1,d0
	moveq	#3,d1
	moveq	#10,d2
	jsr	rndrange(a5)
	sub.l	#$8000,d0
	move.l	d0,sp_ys(a0)
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	moveq	#2,d0
	moveq	#4,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	sub.l	#$8000,d0
	move.l	d0,sp_ys2(a0)
	;
	move.b	#$81,id(a0)
	move.b	#$12,col(a0)
	;
	lea	sp_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	sp_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	sp_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	sp_anim1(pc),a2
	jsr	newanim(a5)
	;
	lea	sp_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

sp_loop1	cmp.b	#$81,id(a0)
	beq.s	.peaceful
	;	
	;I'm aggro!
	;
	jsr	getydiff(a5)
	cmp	#32,d0
	bcs.s	.done
	;go peacful
	move.b	#$81,id(a0)
	move.l	sp_xs(a0),d0
	tst	xs(a0)
	bpl.s	.skip0
	neg.l	d0
.skip0	move.l	d0,xs(a0)
	;
	move.l	sp_ys(a0),d0
	tst	ys(a0)
	bpl.s	.skip
	neg.l	d0
.skip	move.l	d0,ys(a0)
.done	rts
	;
.peaceful	;
	jsr	getydiff(a5)
	cmp	#8,d0
	bcc.s	.done2
	;
	;go aggro!
	;
	move.b	#$82,id(a0)
	move.l	sp_xs2(a0),d0
	jsr	homeinx(a5)
	move.l	sp_ys2(a0),d0
	tst	ys(a0)
	bmi.s	.skip2
	neg.l	d0
.skip2	move.l	d0,ys(a0)
.done2	rts

sp_coll1	jsr	killplayer(a5)
	;
sp_shot1	lea	data(pc),a2
	subq	#1,spinners-data(a2)
	subq	#1,tokill(a5)
	move	#$15,d0
	jsr	addscore(a5)
	lea	sp_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

sp_anim1	incbin	spinner.anm
sp_frag1	incbin	spinner.frags

data
spinners	dc	0
dataf

