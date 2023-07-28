
	include	header.s
	initalien	0,data,dataf,pergame,perpattern,perlife

	rsreset	;
	rs.b	syssize	;my own rs's here...

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	moveq	#5,d0
	move	d0,aliens-data(a2)
	add	d0,tokill(a5)
	lea	perloop(pc),a0
	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makealiens
	rts

perloop	;a5=libbase, a6=chipbase
	;
	rts

makealiens	move	aliens(pc),d7
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
	move.b	#$11,col(a0)
	lea	al_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	al_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	al_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	al_anim1(pc),a2
	jsr	newanim(a5)
	lea	al_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

al_loop1
	rts

al_coll1	jsr	killplayer(a5)
	;
al_shot1	lea	data(pc),a2
	subq	#1,aliens-data(a2)
	subq	#1,tokill(a5)
	move	#$20,d0
	jsr	addscore(a5)
	lea	al_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

al_anim1	incbin	alien.anm
al_frag1	incbin	alien.frags

data
aliens	dc	0
dataf

