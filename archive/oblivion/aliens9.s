
	include	header.s
	initalien	11,data,dataf,pergame,perpattern,perlife
	;
	dc.l	hi_frag1,hi_anim1,hi_name
	dc.l	be_frag1,be_anim1,be_name
	dc.l	0,0,0
	dc.l	0,0,0
	;
hi_name	dc.b	'HIVE',0
be_name	dc.b	'BEE',0
	even

	rsreset	;
	rs.b	syssize	;my own rs's here...
	;
be_maxxs	rs.l	1
be_maxys	rs.l	1

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	bne.s	.skip
	moveq	#1,d0
	moveq	#3,d1
	moveq	#11,d2
	moveq	#15,d3
	moveq	#5,d4
	lea	message(pc),a0
	jsr	calcnum(a5)
	sne	bonus-data(a2)
	move	d0,beehives-data(a2)
	add	d0,tokill(a5)
	clr	bees-data(a2)
	sub.l	a0,a0
	rts
.skip	move	bonus(pc),d0
	beq.s	.done
	move	#$30,d0
	moveq	#10,d1
	lea	message2(pc),a0
	lea	hi_anim1(pc),a1
	lea	hi_frag1(pc),a2
	jmp	dobonus(a5)
.done	rts

message	dc.b	15,"SKILL WAVE - BEEWARE OF HIVES",0
	even
message2	dc.b	"HONEY HARVEST",0
	even

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	move	#-1,beex-data(a2)
	bsr	makebeehives
	move	bees(pc),d7
	beq.s	.skip
	bsr	makebees
.skip	rts

makebeehives	
	move	beehives(pc),d7
	beq.s	.done
	subq	#1,d7
.loop	jsr	beginadd(a5)
	;
	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	move.l	d0,ys(a0)
	;
	move.b	#$22,col(a0)
	lea	hi_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	hi_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	hi_anim1(pc),a2
	jsr	newanim(a5)
	;
	lea	hi_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

makebees	subq	#1,d7
	;
.loop	jsr	beginadd(a5)
	;
	move	beex(pc),d0
	bmi.s	.rnd
	move	d0,x(a0)
	move	beey(pc),y(a0)
	bra.s	.nornd
	;
.rnd	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	;
.nornd	;
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	lsl.l	#1,d0
	move.l	d0,xs(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	lsl.l	#1,d0
	move.l	d0,ys(a0)
	move.l	#$2000,xa(a0)
	move.l	#$2000,ya(a0)
	;
	;rnd x speed limit, y speed limit
	;
	moveq	#1,d0
	moveq	#6,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	move.l	d0,be_maxxs(a0)
	moveq	#1,d0
	moveq	#6,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	move.l	d0,be_maxys(a0)
	;
	move.b	#$60,col(a0)
	lea	be_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	be_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	be_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	be_anim1(pc),a2
	jsr	newanim(a5)
	move	beex(pc),d0
	bpl.s	.jon
	lea	be_frag1(pc),a2
	jsr	implode(a5)
.jon	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

hi_coll1	jsr	killplayer(a5)
	;
hi_shot1	;bees...
	;
	lea	data(pc),a2
	subq	#1,beehives-data(a2)
	subq	#1,tokill(a5)
	move	x(a0),beex-data(a2)
	move	y(a0),beey-data(a2)
	jsr	rnd1(a5)
	and	#3,d0
	addq	#3,d0
	move	d0,d7
	add	d7,bees-data(a2)
	add	d7,tokill(a5)
	bsr	makebees
	move	#$50,d0
	jsr	addscore(a5)
	lea	hi_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

be_loop1	move	xs(a0),facing(a0)
	lea	be_anim1(pc),a2
	tst	ys(a0)
	bmi.s	.skip
	lea	be_anim2(pc),a2
.skip	jsr	newanim(a5)
	move.l	be_maxxs(a0),d0
	jsr	limitxs(a5)
	move.l	be_maxys(a0),d0
	jsr	limitys(a5)
	move.l	xa(a0),d0
	jsr	homeinxa(a5)
	move.l	ya(a0),d0
	jmp	homeinya(a5)

be_coll1	jsr	killplayer(a5)
	;
be_shot1	lea	data(pc),a2
	subq	#1,bees-data(a2)
	subq	#1,tokill(a5)
	move	#$10,d0
	jsr	addscore(a5)
	lea	be_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

hi_anim1	incbin	beehive.anm
hi_frag1	incbin	beehive.frags
	;
be_anim1	incbin	bee1.anm
be_frag1	incbin	bee1.frags
be_anim2	incbin	bee2.anm

data
beehives	dc	0
bees	dc	0
beex	dc	0
beey	dc	0
bonus	dc	0
dataf

