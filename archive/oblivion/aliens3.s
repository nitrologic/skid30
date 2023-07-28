
	include	header.s
	initalien	6,data,dataf,pergame,perpattern,perlife
	;
	dc.l	bo_frag1,bo_anim1,bo_name
	dc.l	0,0,0
	dc.l	0,0,0
	dc.l	0,0,0
	;
bo_name	dc.b	'BOMBER',0
	even

	rsreset	;
	rs.b	syssize2	;my own rs's here...
bo_t	rs.w	1	;timer to negate ya
bo_maxys	rs.l	1

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
	moveq	#2,d2
	moveq	#-1,d3
	jsr	calcnum(a5)
	move	d0,bombers-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
.skip	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makebombers
	rts

makebombers	move	bombers(pc),d7
	beq	.done
	subq	#1,d7
	jsr	rndoffsc(a5)
.loop	jsr	beginadd(a5)
	jsr	getoffsc(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	;
	moveq	#1,d0
	moveq	#4,d1
	moveq	#12,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	swap	d0
	move	d0,facing(a0)
	move.l	#$0800,ya(a0)
	;
	moveq	#2,d0
	moveq	#3,d1
	moveq	#10,d2
	jsr	rndrange(a5)
	move.l	d0,bo_maxys(a0)
	;
	clr	bo_t(a0)
	jsr	sysinitbull(a5)
	;
	move.b	#$33,col(a0)
	lea	bo_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	bo_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	bo_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	bo_anim1(pc),a2
	jsr	newanim(a5)
	lea	bo_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

bo_loop1	move.l	bo_maxys(a0),d0
	jsr	limitys(a5)
	jsr	sysdropbomb(a5)
	subq	#1,bo_t(a0)
	bpl.s	.skip
	neg.l	ya(a0)
	move	#128,d0
	jsr	rnd2(a5)
	move	d0,bo_t(a0)
.skip	rts

bo_coll1	jsr	killplayer(a5)
	;
bo_shot1	lea	data(pc),a2
	subq	#1,bombers-data(a2)
	subq	#1,tokill(a5)
	move	#$20,d0
	jsr	addscore(a5)
	lea	bo_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

bo_anim1	incbin	bomber.anm
bo_frag1	incbin	bomber.frags

data
bombers	dc	0
dataf

