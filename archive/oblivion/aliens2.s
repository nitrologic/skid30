
	include	header.s
	initalien	5,data,dataf,pergame,perpattern,perlife
	;	
	dc.l	po_frag1,po_anim1,po_name
	dc.l	sw_frag1,sw_anim1,sw_name
	dc.l	0,0,0
	dc.l	0,0,0
	;
po_name	dc.b	'SPOID',0
sw_name	dc.b	'SPAWN',0
	even

	rsreset	;
	rs.b	syssize2	;my own rs's here...
	;
sw_xs	rs.l	1	;cruising x speed
sw_xs2	rs.l	1	;agro xspeed
sw_maxys	rs.l	1	;maximum y speed!
sw_frags	rs.l	1	;frags for swarmer
sw_count	rs.l	1

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	bne.s	.skip
	clr	rswarmers-data(a2)
	clr	yswarmers-data(a2)
	moveq	#1,d0
	moveq	#3,d1
	moveq	#3,d2
	moveq	#10,d3
	moveq	#6,d4
	sub.l	a0,a0
	jsr	calcnum(a5)
	move	d0,pods-data(a2)
	add	d0,tokill(a5)
	;
	sub.l	a0,a0
.skip	rts

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	move	#-1,swax-data(a2)
	bsr	makepods
	moveq	#0,d6
	move	rswarmers(pc),d7
	bsr	makeswarmers
	moveq	#-1,d6
	move	yswarmers(pc),d7
	bsr	makeswarmers
	rts

podtext	dc.b	96,0,'SMART BOMB RED SPOIDS...',0
	even

makepods	move	pods(pc),d7
	beq.s	.done
	lea	podtext(pc),a2
	jsr	scanprint(a5)
	move	pods(pc),d7
	subq	#1,d7
.loop	jsr	beginadd(a5)
	;
	move	#176,x(a0)
	move	#128,y(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	jsr	rnd1(a5)
	move.l	d0,ys(a0)
	;
	move.b	#$22,col(a0)
	lea	po_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	po_coll1(pc),a2
	move.l	a2,collide(a0)
	clr.l	animf(a0)
	lea	po_anim1(pc),a2
	jsr	newanim(a5)
	;
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

makeswarmers;
	;d6=type. 0=red, else yellow
	;d7=number (loaded last!)
	;
	subq	#1,d7
	bmi	.done
	;
.loop	jsr	beginadd(a5)
	;
	move	swax(pc),d0
	bmi.s	.rnd
	move	d0,x(a0)
	move	sway(pc),y(a0)
	bra.s	.nornd
	;
.rnd	move	#4096,d0
	jsr	rnd2(a5)
	move	d0,x(a0)
	move	#512,d0
	jsr	rnd2(a5)
	move	d0,y(a0)
	;
.nornd	;xspeed...
	;
	moveq	#1,d0
	moveq	#3,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	move.l	d0,sw_xs(a0)
	moveq	#2,d0
	moveq	#5,d1
	moveq	#40,d2
	jsr	rndrange(a5)
	tst	xs(a0)
	bpl.s	.skip
	neg.l	d0
.skip	move.l	d0,sw_xs2(a0)
	jsr	rnd1(a5)
	jsr	rnd3(a5)
	lsl.l	#2,d0
	move.l	d0,ys(a0)
	;
	;y acc...
	;
	move.l	#$2000,ya(a0)
	moveq	#2,d0
	moveq	#6,d1
	moveq	#20,d2
	jsr	rndrange(a5)
	move.l	d0,sw_maxys(a0)
	;
	jsr	sysinitbull(a5)
	lsr	bullr(a0)
	;
	move.b	#$21,id(a0)	;peaceful!
	move.b	#$36,col(a0)
	lea	sw_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	sw_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	sw_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	sw_anim1(pc),a2
	lea	sw_frag1(pc),a3
	lea	rswarmers(pc),a4
	tst	d6
	beq.s	.yes
	lea	sw_anim2(pc),a2
	lea	sw_frag2(pc),a3
	lea	yswarmers(pc),a4
.yes	move.l	a3,sw_frags(a0)
	move.l	a4,sw_count(a0)
	jsr	newanim(a5)
	move	swax(pc),d0
	bpl.s	.noimp
	move.l	sw_frags(a0),a2
	jsr	implode(a5)
.noimp	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

po_coll1	jsr	killplayer(a5)
	;
po_shot1	;swarmers to happen?
	;
	lea	data(pc),a2
	subq	#1,pods-data(a2)
	subq	#1,tokill(a5)
	move.b	smarton(a5),d0
	beq.s	.dosw
	move	animf(a0),d0
	bne.s	.dontsw
	;
.dosw	jsr	rnd1(a5)
	move	d0,d7
	and	#3,d7
	addq	#2,d7
	move	x(a0),swax-data(a2)
	move	y(a0),sway-data(a2)
	moveq	#0,d6
	add	d7,rswarmers-data(a2)
	move	animf(a0),d0
	bne.s	.skip
	moveq	#-1,d6
	sub	d7,rswarmers-data(a2)
	add	d7,yswarmers-data(a2)
.skip	add	d7,tokill(a5)
	bsr	makeswarmers
	;
.dontsw	move	#$100,d0
	jsr	addscore(a5)
	lea	po_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

sw_loop1	move.l	sw_maxys(a0),d0
	jsr	limitys(a5)
	move.l	ya(a0),d0
	jsr	homeinya(a5)
	;
	jsr	getxdiff(a5)
	cmp	#128,d0
	bcs.s	.aggro
	;
.peaceful	cmp.b	#$22,id(a0)
	bne.s	.done
	subq.b	#1,id(a0)
	move.l	sw_xs(a0),d0
	bpl.s	.skip
	neg.l	d0
.skip	tst.l	xs(a0)
	bpl.s	.skip2
	neg.l	d0
.skip2	move.l	d0,xs(a0)
.done	rts	
	;
.aggro	cmp.b	#$21,id(a0)
	bne.s	.done2
	lea	sw_sfx(pc),a2
	moveq	#76,d0
	moveq	#48,d1
	jsr	playsfx(a5)
	addq.b	#1,id(a0)
	move.l	sw_xs2(a0),d0
	jsr	homeinx(a5)
.done2	jmp	sysfirebull(a5)

sw_coll1	jsr	killplayer(a5)
	;
sw_shot1	move.l	sw_count(a0),a2
	subq	#1,(a2)
	subq	#1,tokill(a5)
	move	#$10,d0
	jsr	addscore(a5)
	move.l	sw_frags(a0),a2
	jsr	explode(a5)
	jmp	killme(a5)

po_anim1	incbin	pod1.anm
po_frag1	incbin	pod1.frags
sw_anim1	incbin	swarmer1.anm
sw_frag1	incbin	swarmer1.frags
sw_anim2	incbin	swarmer2.anm
sw_frag2	incbin	swarmer2.frags
sw_sfx	incbin	swarmer.bsx

data
pods	dc	0
swax	dc	0
sway	dc	0
rswarmers	dc	0	;red swarmers - get smarted
yswarmers	dc	0	;yellow swarmers - smart bomb invincible!
dataf

