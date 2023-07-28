
	include	header.s
	initalien	3,data,dataf,pergame,perpattern,perlife
	;
	dc.l	fi_frag1,fi_anim1,fi_name
	dc.l	0,0,0
	dc.l	0,0,0
	dc.l	0,0,0
	;
fi_name	dc.b	'COSMIC FISH',0
	even

	rsreset	;
	rs.b	syssize2	;my own rs's here...
fi_xs	rs.l	0
fi_xs2	rs.l	0
fi_ys	rs.l	0

pergame	;a2=start of data, a5=libbase, a6=chipbase
	;
	rts

perpattern	;d0=pattern number
	;
	;a2=start of data, a5=libbase, a6=chipbase
	;
	;return a0=perloop routine or 0
	;
	bne.s	.skip	;check not end of pattern
	moveq	#2,d0
	moveq	#6,d1
	moveq	#1,d2
	moveq	#5,d3
	moveq	#50,d4
	lea	message(pc),a0
	jsr	calcnum(a5)
	sne	bonus-data(a2)	;set bonus flag
	;
	move	d0,fishs-data(a2)
	add	d0,tokill(a5)
	sub.l	a0,a0
	rts
	;
.skip	;OK, end of pattern...bonus?
	;
	move	bonus(pc),d0
	beq.s	.done
	move	#$10,d0
	moveq	#10,d1
	lea	message2(pc),a0
	lea	fi_anim1(pc),a1
	lea	fi_frag1(pc),a2
	jmp	dobonus(a5)
.done	rts

message	dc.b	15,"SKILL WAVE - COSMIC FEEDING FRENZY",0
	even
message2	dc.b	'AQUATIC EXCELLENCE',0
	even

perlife	;a2=start of data, a5=libbase, a6=chipbase
	;
	bsr	makefishs
	rts

makefishs	move	fishs(pc),d7
	beq	.done
	subq	#1,d7
	jsr	rndoffsc(a5)
.loop	jsr	beginadd(a5)
	jsr	getoffsc(a5)
	move	#256,d0
	jsr	rnd2(a5)
	add	#128,d0
	move	d0,y(a0)
	;
	moveq	#2,d0
	moveq	#3,d1
	moveq	#30,d2
	jsr	rndrange(a5)
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	move	xs(a0),facing(a0)
	move.l	d0,fi_xs(a0)
	moveq	#2,d0
	moveq	#4,d1
	moveq	#40,d2
	jsr	rndrange(a5)
	move.l	d0,fi_xs2(a0)
	moveq	#1,d0
	moveq	#3,d1
	moveq	#50,d2
	jsr	rndrange(a5)
	move.l	d0,fi_ys(a0)
	;
	jsr	sysinitbull(a5)
	;
	move.b	#$b1,id(a0)
	move.b	#$30,col(a0)
	lea	fi_loop1(pc),a2
	move.l	a2,loop(a0)
	lea	fi_shot1(pc),a2
	move.l	a2,shot(a0)
	lea	fi_coll1(pc),a2
	move.l	a2,collide(a0)
	lea	fi_anim1(pc),a2
	jsr	newanim(a5)
	lea	fi_frag1(pc),a2
	jsr	implode(a5)
	jsr	endadd(a5)
	dbf	d7,.loop
.done	rts

fi_loop1	;OK, how does our fish work?
	;
	moveq	#5,d7
	jsr	sysfiremiss(a5)
	cmp.b	#$b2,id(a0)
	bne.s	.skip
	jsr	getydiff(a5)
	cmp	#64,d0
	bcs.s	.done
	move.b	#$b1,id(a0)
	lsl	bullr(a0)
	move.l	fi_xs(a0),d0
	jsr	rnd3(a5)
	move.l	d0,xs(a0)
	move	xs(a0),facing(a0)
	clr.l	ys(a0)
	rts
	;
.skip	cmp	#16,y(a0)
	bcs.s	.goaggro
	cmp	#512-15,y(a0)
	bcc.s	.goaggro
	;
	jsr	getydiff(a5)
	cmp	#48,d0
	bcc.s	.done
	;
	;fish go angry!
	;
.goaggro	tst	onscreen(a5)
	beq.s	.nosnd
	moveq	#70,d0
	moveq	#48,d1
	lea	fi_sfx(pc),a2
	jsr	playsfx(a5)
.nosnd	move.b	#$b2,id(a0)
	lsr	bullr(a0)
	move.l	fi_xs2(a0),d0
	jsr	homeinx(a5)
	move	xs(a0),facing(a0)
	move.l	fi_ys(a0),d0
	jmp	homeiny(a5)
.done	rts

fi_coll1	jsr	killplayer(a5)
	;
fi_shot1	lea	data(pc),a2
	subq	#1,fishs-data(a2)
	subq	#1,tokill(a5)
	move	#$15,d0
	jsr	addscore(a5)
	lea	fi_frag1(pc),a2
	jsr	explode(a5)
	jmp	killme(a5)

fi_anim1	incbin	fish.anm
fi_frag1	incbin	fish.frags
fi_sfx	incbin	fish.bsx

data
fishs	dc	0
bonus	dc	0
dataf

