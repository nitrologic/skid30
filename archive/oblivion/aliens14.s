
	include	header.s
	initalien	13,data,dataf,pergame,perpattern,perlife
	;
	dc.l	di_frag1,di_anim1,di_name
	dc.l	0,0,0
	dc.l	0,0,0
	dc.l	0,0,0
	;
di_name	dc.b	'PHANTASM',0
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
	moveq	#21,d2
	moveq	#25,d3
	moveq	#5,d4
	sub.l	a0,a0
	jsr	calcnum(a5)
	move	d0,diamonds-data(a2)
	add	d0,tokill(a5)
	lea	perloop(pc),a0
.skip	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makediamonds
	rts

perloop	;a5=libbase, a6=chipbase
	;
	rts

makediamonds	move	diamonds(pc),d7
	beq.s	.done
	subq	#1,d7
	jsr	rndoffsc(a5)
.loop	jsr	beginadd(a5)
	jsr	getoffsc(a5)
	move	#256,d0
	jsr	rnd2(a5)
	add	#64,d0
	move	d0,y(a0)
	;
	jsr	sysinitbull(a5)
	;
	move.b	#$42,col(a0)
	lea	di_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	di_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	di_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	di_anim1(pc),a2
	jsr	newanim(a5)
	lea	di_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

di_loop1	jsr	sysfirebull(a5)
	jsr	chkinfront(a5)
	beq.s	.done
	jsr	getydiff(a5)
	cmp	#24,d0
	bcc.s	.done
	jsr	getxdiff(a5)
	cmp	#128,d0
	bcs.s	.done
	;	
	;warp out, then in!
	;
	lea	di_frag1(pc),a2
	jsr	explode(a5)
	move	#352,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#256,d0
	jsr	rnd2(a5)
	add	#64,d0
	add	d0,y(a0)
	lea	di_frag1(pc),a2
	jmp	implode(a5)
	;
.done	rts

di_coll1	jsr	killplayer(a5)
	;
di_shot1	lea	data(pc),a2
	subq	#1,diamonds-data(a2)
	subq	#1,tokill(a5)
	move	#$50,d0
	jsr	addscore(a5)
	lea	di_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

di_anim1	incbin	diamond.anm
di_frag1	incbin	diamond.frags

data
diamonds	dc	0
dataf

