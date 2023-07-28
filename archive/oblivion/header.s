	section	name,CODE_C

	;system structure
	;	
	rsreset
	rs.l	1
xa	rs.l	1
xs	rs.l	1
x	rs.l	1	*
ya	rs.l	1
ys	rs.l	1
y	rs.l	1	*
frame	rs.l	1
flags	rs.b	1
id	rs.b	1
anims	rs.l	1
animf	rs.l	1
anim	rs.l	1	*
facing	rs.w	1
loop	rs.l	1
collide	rs.l	1
shot	rs.l	1
	;
	;scanner stuff
	;
old	rs.w	1	;pos of old plot
olds	rs.b	1	;old plot shift
col	rs.b	1	;col1 (top half of blip=high 4 bits)
	;
syssize	rs.b	0

	rsreset	;for bullet firing funcs!
	rs.b	syssize
bullt	rs.w	1	;timer for firing!
bullr	rs.w	1	;random number range
	;
syssize2	rs.b	0

	;library routines
	;
	rsreset
	;
tokill	rs.w	1
inspace	rs.w	1
playerdie	rs.w	1
pattern	rs.w	1
scoremore	rs.l	1
score	rs.l	1
ships	rs.w	1
smarts	rs.w	1
inviso	rs.l	1
cmode	rs.w	1
	;
beginadd	rs.l	1
endadd	rs.l	1
implode	rs.l	1
explode	rs.l	1
rnd1	rs.l	1
rnd2	rs.l	1
getmounty	rs.l	1
newanim	rs.l	1
killme	rs.l	1
rnd3	rs.l	1
chkinfront	rs.l	1
shipx	rs.l	1	;*
shipy	rs.l	1	;*
sxspeed	rs.l	1	;*
syspeed	rs.l	1	;*
shipdir	rs.l	1	;*
homeawayy	rs.l	1
homeiny	rs.l	1
homeawayya	rs.l	1
homeinya	rs.l	1
limitys	rs.l	1
screenx	rs.l	1
screeny	rs.l	1
getxdiff	rs.l	1
findid	rs.l	1
homeinxa2	rs.l	1
scanprint	rs.l	1
firebull1	rs.l	1
addscore	rs.l	1
killplayer	rs.l	1
warpplayer	rs.l	1
homeawayx	rs.l	1
homeinx	rs.l	1
homeawayxa	rs.l	1
homeinxa	rs.l	1
limitxs	rs.l	1
rndrange	rs.l	1
getydiff	rs.l	1
getrealx	rs.l	1
smarton	rs.b	1
invisoon	rs.b	1
lastsmart	rs.b	1
lastrev	rs.b	1
lastfire	rs.b	1
	rs.b	1
sysfirebull	rs.l	1
dropbomb	rs.l	1
sysinitbull	rs.l	1
sysdropbomb	rs.l	1
onscreen	rs.w	1
firemiss	rs.l	1
sysfiremiss	rs.l	1
homecalc	rs.l	1
sysfirekill	rs.l	1
calcnum	rs.l	1
rndoffsc	rs.l	1
getoffsc	rs.l	1
killme2	rs.l	1
playsfx	rs.l	1
gospace	rs.l	1
dobonus	rs.l	1
killme3	rs.l	1
	;

initalien	macro
	dc.l	\1,\3-\2,\2,\4,\5,\6
	endm


