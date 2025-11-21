SRC_DIR = src
OBJ_DIR = obj

!ifeq appver_exedef BR120FX
!define _3DFX
!endif
!ifeq appver_exedef BP120FX
!define _3DFX
!endif


#CPPFLAGS = /5r /d3 /i=..\qtools\include;..\helix32;..\audiolib;..\mact;..\smacker
!ifdef _3DFX
EXENAME = 3dfx
CPPFLAGS = /5r /d1 /s /orti /oe=3 /d_3DFX=1 /dAPPVER_EXEDEF=$(appver_exedef)
!else
EXENAME = blood
CPPFLAGS = /5r /d1 /s /orti /oe=3 /dAPPVER_EXEDEF=$(appver_exedef)
!endif
TENFLAGS = /4r /d1 /s /orti /oe=3
CFLAGS = /5r /d2 /orti
AFLAGS = -ml

.h: $(SRC_DIR)
.cpp: $(SRC_DIR)
.c: $(SRC_DIR)
.asm: $(SRC_DIR)
.obj: $(appver_exedef)\$(OBJ_DIR)

OBJS = &
globals.obj &
blood.obj &
screen.obj &
view.obj &
db.obj &
gameutil.obj &
fire.obj &
fframe.obj &
sectorfx.obj &
map2d.obj &
satimer.obj &
usrhooks.obj &
endgame.obj &
levels.obj &
sound.obj &
sfx.obj &
controls.obj &
actor.obj &
player.obj &
replace.obj &
tile.obj &
triggers.obj &
eventq.obj &
qav.obj &
weapon.obj &
gui.obj &
seq.obj &
mirrors.obj &
dude.obj &
warp.obj &
weather.obj &
credits.obj &
menu.obj &
messages.obj &
ai.obj &
aibat.obj &
aibeast.obj &
aiboneel.obj &
aiburn.obj &
aicaleb.obj &
aicerber.obj &
aicult.obj &
aigarg.obj &
aigilbst.obj &
aighost.obj &
aihand.obj &
aihound.obj &
aiinnoc.obj &
aipod.obj &
airat.obj &
aispid.obj &
aitchern.obj &
aizomba.obj &
aizombf.obj &
fx.obj &
callback.obj &
loadsave.obj &
config.obj &
network.obj &
demo.obj &
gib.obj &
asound.obj &
gamemenu.obj &
cdrom.obj &
smkplay.obj &
mpath1.obj &
mpath2.obj &
mpath3.obj &
heat1.obj &
heat2.obj &
heat3.obj &
engage.obj &
choke.obj

all : $(appver_exedef)\$(EXENAME).exe .symbolic
	@%null

$(appver_exedef)\$(EXENAME).exe: $(OBJS) tenblood.obj
	%make $(appver_exedef)\blood.lnk
	cd $(appver_exedef)
	wlink @blood.lnk
	cd ..

$(appver_exedef)\blood.lnk: makefile
	%create $^@
	%append $^@ opt map
	%append $^@ opt stack=8192
	%append $^@ opt static
	%append $^@ debug all
	%append $^@ name $(EXENAME)
	%append $^@ library ..\..\qtools\$(appver_qtools)\qtools.lib
	%append $^@ library ..\..\helix32\helix.lib
!ifndef _3DFX
	%append $^@ library ..\..\build\engine.lib
!endif
	%append $^@ library ..\..\mact\mact386.lib
	%append $^@ library ..\..\audiolib\audio_wf.lib
	%append $^@ library ..\..\smacker\smack.lib
	%append $^@ library ..\..\ten\lib\sdkboxr.lib
    for %i in ($(OBJS)) do %append $^@ file $(OBJ_DIR)\%i
	%append $^@ file ..\..\helix32\hgd13.obj
	%append $^@ file ..\..\helix32\hgdx.obj
	%append $^@ file ..\..\build\mmulti.obj
	%append $^@ file $(OBJ_DIR)\tenblood.obj
!ifdef _3DFX
	%append $^@ library ..\..\3dfx\glide2x.lib
	%append $^@ library ..\..\3dfx\texus\texus.lib
	%append $^@ file ..\..\3dfx\acc3dfx.obj
	%append $^@ file ..\..\3dfx\texcache.obj
	%append $^@ file ..\..\3dfx\engine.obj
	%append $^@ file ..\..\3dfx\a.obj
	%append $^@ file ..\..\3dfx\cache1d.obj
	%append $^@ file ..\..\3dfx\grab.obj
!endif


.cpp.obj:
    wpp386 $(CPPFLAGS) $[* /fo=$(appver_exedef)\$(OBJ_DIR)\$^&

.c.obj:
    wcc386 $(CFLAGS) $[* /fo=$(appver_exedef)\$(OBJ_DIR)\$^&

.asm.obj:
    tasm $(AFLAGS) $[*.asm $(appver_exedef)\$(OBJ_DIR)\$^&

tenblood.obj: $(SRC_DIR)\tenblood.cpp
    wpp386 $(TENFLAGS) $(SRC_DIR)\tenblood /fo=$(appver_exedef)\$(OBJ_DIR)\tenblood.obj
