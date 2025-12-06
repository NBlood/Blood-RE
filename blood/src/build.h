/*
 * Copyright (C) 2018, 2022 nukeykt
 *
 * This file is part of Blood-RE.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _BUILD_H_
#define _BUILD_H_

#include "typedefs.h"
#include "globals.h"

#define kMaxVoxels 512
#define kMaxVoxMips 5
#define kMaxSectors 1024
#define kMaxWalls 8192
#define kMaxSprites 4096
#if APPVER_BLOODREV >= AV_BR_BL120
#define kMaxTiles 6144
#else
#define kMaxTiles 4096
#endif
#define kMaxStatus 1024
#define kMaxSkyTiles 256
#define kMaxPlayers 8
#define kMaxViewSprites 1024

struct SECTOR {
    short wallptr, wallnum;
    long ceilingz, floorz;
    ushort ceilingstat, floorstat;
    short ceilingpicnum, ceilingheinum;
    signed char ceilingshade;
    byte ceilingpal, ceilingxpanning, ceilingypanning;
    short floorpicnum, floorheinum;
    signed char floorshade;
    byte floorpal, floorxpanning, floorypanning;
    byte visibility, align;
    ushort type;
    short hitag, extra;
};

enum {
    kSectorStat0 = 1,
    kSectorStat1 = 2,
    kSectorStat2 = 4,
    kSectorStat3 = 8,
    kSectorStat4 = 16,
    kSectorStat5 = 32,
    kSectorStat6 = 64,
    kSectorStat1_4_5 = 0x34,
    kSectorStat15 = 32768
};

struct WALL
{
    long x, y;
    short point2, nextwall, nextsector;
    ushort cstat;
    short picnum, overpicnum;
    signed char shade;
    byte pal, xrepeat, yrepeat, xpanning, ypanning;
    ushort type;
    short hitag, extra;
};

enum {
    kWallStat0 = 1,
    kWallStat1 = 2,
    kWallStat2 = 4,
    kWallStat3 = 8,
    kWallStat3_8 = 0x108,
    kWallStat4 = 16,
    kWallStat5 = 32,
    kWallStat6 = 64,
    kWallStat7 = 128,
    kWallStat8 = 256,
    kWallStat9 = 512,
    kWallStat7_9 = 0x280,

    kWallStat14 = 16384,
    kWallStat15 = 32768,

    kWallStat14_15 = 49152,
};

struct SPRITE
{
    long x, y, z;
    ushort cstat;
    short picnum;
    signed char shade;
    byte pal, clipdist, detail;
    byte xrepeat, yrepeat;
    signed char xoffset, yoffset;
    short sectnum, statnum;
    short ang, owner, index, yvel, inittype;
    short type;
    ushort flags;
    short extra;
};

enum {
    kSpriteStat0 = 1,
    kSpriteStat1 = 2,
    kSpriteStat2 = 4,
    kSpriteStat3 = 8,
    kSpriteStat5 = 32,
    kSpriteStat6 = 64,
    kSpriteStat7 = 128,
    kSpriteStat8 = 256,
    kSpriteStat9 = 512,
    kSpriteStat1_9 = 0x202,
    kSpriteStat13 = 8192,
    kSpriteStat14 = 16384,
    kSpriteStat15 = 32768,
    kSpriteStat31 = 32768, // doh
    kSpriteStat13_14 = 24576,
};

enum {
    kSpriteFace = 0,
    kSpriteWall = 0x10,
    kSpriteFloor = 0x20,
    kSpriteVoxel = 0x30,
    kSpriteMask = 0x30
};

struct PICANM {
    unsigned int animframes : 5;
    unsigned int at0_5 : 1;
    unsigned int animtype : 2;
    signed int xoffset : 8;
    signed int yoffset : 8;
    unsigned int animspeed : 4;
    unsigned int at3_4 : 3; // type
    unsigned int at3_7 : 1; // filler
};

extern "C" {

extern volatile int totalclock;
extern byte palette[768];
extern byte *palookup[256];
extern int parallaxvisibility;
extern ushort numpalookups;
extern BOOL paletteloaded;
extern byte *transluc;
extern byte vidoption;

extern int windowx1, windowy1, windowx2, windowy2;

extern int xdim, ydim;
extern byte *frameplace;
extern int ylookup[1201];

extern SECTOR sector[kMaxSectors];
extern WALL wall[kMaxWalls];
extern SPRITE sprite[kMaxSprites];

extern short headspritesect[kMaxSectors+1], headspritestat[kMaxStatus+1];
extern short prevspritesect[kMaxSprites], prevspritestat[kMaxSprites];
extern short nextspritesect[kMaxSprites], nextspritestat[kMaxSprites];

extern short numsectors, numwalls;

extern byte show2dsector[(kMaxSectors+7)>>3];
extern byte show2dwall[(kMaxWalls+7)>>3];
extern byte show2dsprite[(kMaxSprites+7)>>3];

extern short pskyoff[kMaxSkyTiles], pskybits;

extern int visibility;

extern byte parallaxtype;
extern byte picsiz[kMaxTiles];

extern byte *voxoff[kMaxVoxels][kMaxVoxMips];

extern short tilesizx[kMaxTiles];
extern short tilesizy[kMaxTiles];
extern PICANM picanm[kMaxTiles];

extern int artversion;
extern int numtiles;

extern byte tilefilenum[kMaxTiles];
extern int tilefileoffs[kMaxTiles];
extern byte gotpic[(kMaxTiles+7)>>3];

extern byte *waloff[kMaxTiles];

void fixtransluscence(void);
void initengine(void);
void uninitengine(void);
int setgamemode(byte, int, int);
void clearview(int);
void nextpage(void);
void setview(int, int, int, int);
int getpalookup(int, int);

void initspritelists(void);
int insertsprite(short, short);
void deletesprite(short);
int changespritesect(short, short);
int changespritestat(short, short);

void printext256(int, int, short, short, char *, char);

int animateoffs(short a1, ushort a2);

void rotatesprite(int, int, int, int, int, signed char, char, char, int, int, int, int);

void setviewtotile(int, int, int);

void setviewback();

void setaspect(int, int);

void drawrooms(int, int, int, int, int, int);

void drawmasks(void);

extern byte gotsector[(kMaxSectors+7)>>3];

enum {
    kRSStat0 = 1,
    kRSStat2 = 4,
    kRSStat8 = 256,
};

int ksqrt(int);

int getangle(int, int);

int inside(int, int, int);
void getzsofslope(int, int, int, int *, int *);
int getflorzofslope(int, int, int);
int getceilzofslope(int, int, int);

BOOL cansee(int, int, int, int, int, int, int, int);

#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)

extern int hitscangoalx, hitscangoaly;

void hitscan(int, int, int, int, int, int, int, short *, short *, short*, int *, int *, int *, ulong);

void getzrange(int, int, int, int, long *, long *, long *, long *, int, ulong);

uint clipmove(long *, long *, long *, short *, long, long, int, int, int, ulong);

int pushmove(long *, long *, long *, short *, long, long, long, ulong);

void updatesector(int, int, short *);

void setsprite(int, int, int, int);

void getvalidvesamodes(void);

extern short validmode[];
extern long validmodexdim[];
extern long validmodeydim[];
extern long validmodecnt;

extern int numpages;
extern int qsetmode;
extern int pageoffset;
extern int ydim16;

extern int yxaspect;

extern int spritesortcnt;
extern SPRITE tsprite[];

extern short sintable[];

extern int clipmoveboxtracenum;

extern byte automapping;

extern int randomseed;

extern byte showinvisibility;

extern int parallaxyoffs;
extern int parallaxyscale;

void drawline256(int, int, int, int, int);
void drawmapview(long cposx, long cposy, long czoom, short cang);

short lastwall(int);
void alignflorslope(int, int, int, int);
void alignceilslope(int, int, int, int);
int sectorofwall(int);

void preparemirror(int, int, int, int, int, int, int, int *, int *, short *);
void completemirror(void);

extern long startposx;
extern long startposy;
extern long startposz;
extern short startsectnum;
extern short startang;

void screencapture(char *, char);

void faketimerhandler(void);

int krand(void);

void printmessage16(char*);
void setbrightness(char, char*);
int loadboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum);
int saveboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum);
void printext16(int, int, short, short, char*, char);
void drawline16(int, int, int, int, int);

// mmulti.c

extern short numplayers;
extern short myconnectindex;
extern short connecthead;
extern short connectpoint2[];
extern byte syncstate;

void sendpacket(int, char *, int);
short getpacket(short *, char *);
void initmultiplayers(int, int, int);
void setpackettimeout(int, int);
void sendlogoff(void);
void uninitmultiplayers(void);

// a.asm

extern byte *palookupoffse[4];
extern byte *bufplce[4];
extern int vince[4];
extern int vplce[4];

void setupvlineasm(int);
#pragma aux setupvlineasm parm [eax]
int vlineasm4(int, byte*);
#pragma aux vlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi]
int vlineasm1(int, byte*, int, int, byte*, byte*);
#pragma aux vlineasm1 parm [eax][ebx][ecx][edx][esi][edi]
int mvlineasm1(int, byte*, int, int, byte*, byte*);
#pragma aux mvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
int mvlineasm4(int, byte*);
#pragma aux mvlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi];
void setupmvlineasm(long);
#pragma aux setupmvlineasm parm [eax];


// editor stuff

extern short asksave;
extern int vel, svel, angvel;
extern long posx, posy, posz;
extern int horiz;
extern short ang, cursectnum;
extern char buildkeys[];
extern int kensplayerheight;
extern int zmode;
extern short defaultspritecstat;
extern short linehighlight;
extern short highlightcnt;
extern short pointhighlight;
extern short highlightsectorcnt;
extern short highlightsector[];
extern short searchwall, searchsector, searchstat;
extern int searchx, searchy;
extern char somethingintab;
extern short highlight[];
extern short tempextra, templotag, tempcstat, temphitag, temppicnum;
extern char tempvis, tempxrepeat, tempyrepeat, tempshade, temppal;
extern int zlock;
extern short searchit;
extern short gridlock;

void ExtInit(void);
void ExtUnInit(void);
void ExtPreCheckKeys(void);
void ExtAnalyzeSprites(void);
void ExtCheckKeys(void);
void ExtLoadMap(const char* mapname);
void ExtSaveMap(const char* mapname);
const char* ExtGetSectorCaption(short sectnum);
const char* ExtGetWallCaption(short wallnum);
const char* ExtGetSpriteCaption(short spritenum);
void ExtShowSectorData(short sectnum);
void ExtShowWallData(short wallnum);
void ExtShowSpriteData(short spritenum);
void ExtEditSectorData(short sectnum);
void ExtEditWallData(short wallnum);
void ExtEditSpriteData(short spritenum);

void keytimerstuff(void);
void initkeys(void);
void uninitkeys(void);
void inittimer(void);
void uninittimer(void);
int initmouse(void);
void clearmidstatbar16(void);
int getpointhighlight(int, int);
int getlinehighlight(int, int);
void draw2dscreen(int, int, short, int, short);
void getpoint(int, int, int*, int*);
short getnumber16(char*, short, int);
void updatenumsprites(void);
void overheadeditor(void);
int nextsectorneighborz(int, int, int, int);
void fixrepeats(int);
char changechar(char, long, char, char);

}

#endif // !_BUILD_H_
