
	include	header.s
	initalien	7,data,dataf,pergame,perpattern,perlife
	;
	dc.l	yl_frag1,yl_anim2,yl_name
	dc.l	0,0,0
	dc.l	0,0,0
	dc.l	0,0,0
	;
yl_name	dc.b	'SPACE DART',0
	even

	rsreset	;
	rs.b	syssize2	;my own rs's here...
yl_xs	rs.l	1
yl_xs2	rs.l	1

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
	moveq	#4,d0
	moveq	#5,d1
	moveq	#4,d2
	moveq	#20,d3
	moveq	#40,d4
	lea	message(pc),a0
	jsr	calcnum(a5)
	sne	bonus-data(a2)
	move	d0,yllabs-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
	rts
.skip	move	bonus(pc),d0
	beq.s	.done
	move	#$40,d0
	moveq	#10,d1
	lea	message2(pc),a0
	lea	yl_anim2(pc),a1
	lea	yl_frag1(pc),a2
	jmp	dobonus(a5)
.done	rts

message	dc.b	15,"SKILL WAVE - SPACE DART ONSLAUGHT",0
	even
message2	dc.b	"DART DESTRUCTION",0
	even

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makeyllabs
	rts

makeyllabs	move	yllabs(pc),d7
	beq	.done
	subq	#1,d7
	jsr	rndoffsc(a5)
.loop	jsr	beginadd(a5)
	jsr	getoffsc(a5)
	move	#256,d0
	jsr	rnd2(a5)
	add	#128,d0
	move	d0,y(a0)
	moveq	#1,d0
	moveq	#2,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	move.l	d0,yl_xs(a0)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	move	xs(a0),facing(a0)
	;
	moveq	#2,d0
	moveq	#5,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	move.l	d0,yl_xs2(a0)
	;
	moveq	#1,d0
	moveq	#2,d1
	moveq	#40,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	;
	jsr	sysinitbull(a5)
	lsr	bullr(a0)
	;
	move.b	#$61,id(a0)
	move.b	#$46,col(a0)
	lea	yl_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	yl_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	yl_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	yl_anim2(pc),a2
	jsr	newanim(a5)
	lea	yl_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

yl_loop1	;how does our yllabian move then?...
	jsr	getydiff(a5)
	move	d0,d7
	cmp	#48,d7
	bcs.s	.skip
	move.l	ys(a0),d0
	jsr	homeiny(a5)
	;
.skip	cmp.b	#$61,id(a0)
	bne.s	.aggro
	;I'm peaceful...should I go aggro?
	cmp	#64,d7
	bcc.s	.done
	move.b	#$62,id(a0)
	lea	yl_anim1(pc),a2
	jsr	newanim(a5)
	move.l	yl_xs2(a0),d0
	jsr	homeinx(a5)
	move	xs(a0),facing(a0)
	rts
.aggro	;should I go peacful?
	moveq	#5,d7
	jsr	sysfiremiss(a5)
	cmp	#64,d7
	bcs.s	.no
	move.b	#$61,id(a0)
	move.l	yl_xs(a0),d0
	tst	xs(a0)
	bpl.s	.skip2
	neg.l	d0
.skip2	move.l	d0,xs(a0)
	lea	yl_anim2(pc),a2
	jmp	newanim(a5)
.no	jsr	getxdiff(a5)
	cmp	#32,d0
	bcs.s	.done
	jsr	chkinfront(a5)
	bne.s	.done
	move.l	yl_xs2(a0),d0
	jsr	homeinx(a5)
	move	xs(a0),facing(a0)
.done	rts

yl_coll1	jsr	killplayer(a5)
	;
yl_shot1	lea	data(pc),a2
	subq	#1,yllabs-data(a2)
	subq	#1,tokill(a5)
	move	#$15,d0
	jsr	addscore(a5)
	lea	yl_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

yl_anim1	incbin	yllab1.anm
yl_frag1	incbin	yllab.frags
yl_anim2	incbin	yllab2.anm

data
yllabs	dc	0
bonus	dc	0
dataf

