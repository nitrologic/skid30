
	include	header.s

	;create my own custom stuff...
	;
	rsreset
	rs.b	syssize
	;
co_t	rs.w	1	;timer before release 1

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	move	#1024,t1-data(a2)
	lea	perloop(pc),a0
	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

addcomets	moveq	#9,d7
	;
.loop	jsr	beginadd(a5)
	;
	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	clr	y(a0)
	jsr	rnd1(a5)
	or	#$8000,d0
	lsl.l	#3,d0
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	jsr	rnd1(a5)
	add.l	d0,d0
	move.l	d0,ys(a0)
	move.l	#$1000,ya(a0)
	;
	bsr	maket
	;
	move.b	#$40,col(a0)
	lea	co_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	co_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	co_anim1(pc),a2
	jsr	newanim(a5)
	;
	jsr	endadd(a5)
	;
	dbf	d7,.loop
	;
	rts

maket	move	#8,d0
	jsr	rnd2(a5)
	add	#4,d0
	move	d0,co_t(a0)
	rts

perloop	lea	t1(pc),a2
	subq	#1,(a2)
	beq.s	.doit
	cmp	#96,(a2)
	bne.s	.done
	lea	co_text1(pc),a2
	jmp	scanprint(a5)
.done	rts
.doit	move	#1024,(a2)
	bra	addcomets

co_shot1	lea	co_frags1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

co_shot2	lea	co_frags2(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

co_loop2	cmp	#512-32,y(a0)
	bgt.s	.skip
	subq	#1,co_t(a0)
	beq.s	.skip
	rts
.skip	jmp	killme(a5)

co_loop1	cmp	#512-32,y(a0)
	bgt	.skip
	subq	#1,co_t(a0)
	bne	.done
	;
	;throw out new fragment...
	;
	bsr	maket
	;
	move.l	a0,a2
	jsr	beginadd(a5)
	;
	move	x(a2),x(a0)
	move	y(a2),y(a0)
	move.l	xs(a2),d0
	asr.l	#3,d0
	move.l	d0,xs(a0)
	move.l	ys(a2),d0
	asr.l	#3,d0
	move.l	d0,ys(a0)
	move.l	#$1000,ya(a0)
	;
	move.b	#$40,col(a0)
	move	#64,co_t(a0)
	;
	lea	co_loop2(pc),a2
	move.l	a2,loop(a0)
	lea	co_shot2(pc),a2
	move.l	a2,shot(a0)
	lea	co_anim2(pc),a2
	jsr	newanim(a5)
	;
	jmp	endadd(a5)
.skip	jmp	killme(a5)
.done	rts

co_text1	dc.b	96,1,"ALERT! METEOR STORM!",0
	even

co_anim1	incbin	comet1.anm
co_anim2	incbin	comet2.anm

co_frags1	incbin	comet1.frags
co_frags2	incbin	comet2.frags

data
t1	dc	0
dataf

	cnop	0,4
	dc.l	'BYE!'
finish	

