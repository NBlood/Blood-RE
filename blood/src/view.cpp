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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "typedefs.h"
#include "build.h"
#include "gameutil.h"
#include "misc.h"
#include "trig.h"
#include "gfx.h"
#include "player.h"
#include "aihand.h"
#include "choke.h"
#include "config.h"
#include "db.h"
#include "debug4g.h"
#include "error.h"
#include "gamemenu.h"
#include "globals.h"
#include "helix.h"
#include "levels.h"
#include "loadsave.h"
#include "map2d.h"
#include "messages.h"
#include "mirrors.h"
#include "network.h"
#include "screen.h"
#include "sectorfx.h"
#include "tile.h"
#include "textio.h"
#include "view.h"
#include "warp.h"
#include "weapon.h"
#include "weather.h"

#include "actor.h"

#if APPVER_BLOODREV >= AV_BR_BL120
#define LDIFF1 0
#define LDIFF8 0
#define LDIFF2 0
#define LDIFF3 0
#define LDIFF4 0
#define LDIFF5 0
#define LDIFF6 0
#define LDIFF7 0
#elif APPVER_BLOODREV >= AV_BR_BL111A
#define LDIFF1 -101
#define LDIFF2 -127
#define LDIFF3 -149
#define LDIFF4 -153
#define LDIFF5 -158
#define LDIFF6 -163
#define LDIFF7 -172
#else
#define LDIFF1 -106
#define LDIFF2 -132
#define LDIFF3 -162
#define LDIFF4 -180
#define LDIFF5 -185
#define LDIFF6 -190
#define LDIFF7 -199
#endif

int gViewMode = 3;
int gZoom = 1024;

BOOL gPrediction = TRUE;

int gViewPosAngle[] = {
    0, 0, -256, -512, -768, 1024, 768, 512, 256
};

CGameMessageMgr gGameMessageMgr;

int gViewIndex;

VIEWPOS gViewPos;

int gViewX0, gViewY0, gViewX1, gViewY1;
#if APPVER_BLOODREV >= AV_BR_BL111A
int gViewX0S, gViewY0S, gViewX1S, gViewY1S;
#endif
int xscale, xstep, yscale, ystep;
int gViewXCenter, gViewYCenter;

static long messageTime;
static char message[256];
static char errMsg[256];

int *lensTable;

byte otherMirrorGotpic[2];
byte bakMirrorGotpic[2];

static int pcBackground;

int gShowFrameRate;

long gScreenTilt;

int deliriumTilt, deliriumTurn, deliriumPitch;

struct INTERPOLATE{
    void* pointer;
    int value;
    int value2;
    INTERPOLATE_TYPE type;
};

INTERPOLATE gInterpolation[4096];
int nInterpolations;

LOCATION gPrevSpriteLoc[kMaxSprites];
byte gInterpolateSprite[(kMaxSprites+7)>>3];
byte gInterpolateWall[(kMaxWalls+7)>>3];
byte gInterpolateSector[(kMaxSectors+7)>>3];

struct VIEW {
    int at0;
    int at4;
    int at8; // bob height
    int atc; // bob width
    int at10;
    int at14;
    int at18; // bob sway y
    int at1c; // bob sway x
    int at20;
    int at24; // horiz
    int at28; // horizoff
    int at2c;
    int at30; // angle
    int at34; // weapon z
    int at38; // view z
    int at3c;
    int at40;
    int at44;
    int at48;
    int at4c;
    long at50; // x
    long at54; // y
    long at58; // z
    long at5c; //xvel
    long at60; //yvel
    long at64; //zvel
    short at68;
    uint at6a;
    BOOL at6e;
    BOOL at6f;
    BOOL at70;
    BOOL at71;
    BOOL at72;
    ushort at73;
    SPRITEHIT at75;
};

VIEW gPrevView[kMaxPlayers];

VIEW predict, predictOld;

VIEW predictFifo[256];

int gInterpolate;

#if APPVER_BLOODREV >= AV_BR_BL120
FONT gFont[5];
#endif

int int_172CE0[16][3];

static char buffer[128];

void RotateYZ(long *pX, long *pY, long *pZ, int ang)
{
    int oY = *pY;
    int oZ = *pZ;
    int angSin = Sin(ang);
    int angCos = Cos(ang);
    *pY = dmulscale30r(oY,angCos,oZ,-angSin);
    *pZ = dmulscale30r(oY,angSin,oZ,angCos);
}

void RotateXZ(long *pX, long *pY, long *pZ, int ang)
{
    int oX = *pX;
    int oZ = *pZ;
    int angSin = Sin(ang);
    int angCos = Cos(ang);
    *pX = dmulscale30r(oX,angCos,oZ,-angSin);
    *pZ = dmulscale30r(oX,angSin,oZ,angCos);
}

void RotateXY(long *pX, long *pY, long *pZ, int ang)
{
    int oX = *pX;
    int oY = *pY;
    int angSin = Sin(ang);
    int angCos = Cos(ang);
    *pX = dmulscale30r(oX,angCos,oY,-angSin);
    *pY = dmulscale30r(oX,angSin,oY,angCos);
}

#if APPVER_BLOODREV >= AV_BR_BL120
void viewSetFont(int id, int tile, int space)
{
    if (id < 0 || id >= 5 || tile < 0 || tile >= kMaxTiles)
        return;

    FONT *pFont = &gFont[id];
    int xSize = 0;
    int ySize = 0;
    pFont->tile = tile;
    for (int i = 0; i < 96; i++)
    {
        if (tilesizx[tile+i] > xSize)
            xSize = tilesizx[tile+i];
        if (tilesizy[tile+i] > ySize)
            ySize = tilesizy[tile+i];
    }
    pFont->xSize = xSize;
    pFont->ySize = ySize;
    pFont->space = space;
}

void viewGetFontInfo(int nFont, char* pString, int* pXSize, int* pYSize)
{
    if (nFont < 0 || nFont >= 5)
        return;
    FONT *pFont = &gFont[nFont];
    if (!pString)
    {
        if (pXSize)
            *pXSize = pFont->xSize;
        if (pYSize)
            *pYSize = pFont->ySize;
    }
    else
    {
        int width = -pFont->space;
        for (char *pBuf = pString; *pBuf; pBuf++)
        {
            int tile = ((*pBuf-32)&127)+pFont->tile;
            if (tilesizx[tile] != 0 && tilesizy[tile] != 0)
                width += tilesizx[tile]+pFont->space;
        }
        if (pXSize)
            *pXSize = width;
        if (pYSize)
            *pYSize = pFont->ySize;
    }
}
#endif

#if APPVER_BLOODREV >= AV_BR_BL111A
void viewUpdatePages(void)
{
    pcBackground = numpages;
}
#endif

void viewToggle(int viewMode)
{
    if (viewMode == 3)
        gViewMode = 4;
    else
    {
        gViewMode = 3;
        viewResizeView(gViewSize);
    }
}

void viewInitializePrediction(void)
{
    predict.at30 = gMe->pSprite->ang;
    predict.at20 = gMe->at77;
    predict.at24 = gMe->at7b;
    predict.at28 = gMe->at7f;
    predict.at2c = gMe->at83;
    predict.at6f = gMe->at31c;
    predict.at70 = gMe->at2e;
    predict.at72 = gMe->at87;
    predict.at71 = gMe->atc.buttonFlags.jump;
    predict.at50 = gMe->pSprite->x;
    predict.at54 = gMe->pSprite->y;
    predict.at58 = gMe->pSprite->z;
    predict.at68 = gMe->pSprite->sectnum;
    predict.at73 = gMe->pSprite->flags;
    predict.at5c = xvel[gMe->pSprite->index];
    predict.at60 = yvel[gMe->pSprite->index];
    predict.at64 = zvel[gMe->pSprite->index];
    predict.at6a = gMe->pXSprite->at30_0;
    predict.at48 = gMe->at2f;
    predict.at4c = gMe->at316;
    predict.at6e = gMe->atc.keyFlags.lookCenter;
    predict.at75 = gSpriteHit[gMe->pSprite->extra];
    predict.at0 = gMe->at37;
    predict.at4 = gMe->at3b;
    predict.at8 = gMe->at3f;
    predict.atc = gMe->at43;
    predict.at10 = gMe->at47;
    predict.at14 = gMe->at4b;
    predict.at18 = gMe->at4f;
    predict.at1c = gMe->at53;
    predict.at34 = gMe->at6f-gMe->at67-(12<<8);
    predict.at38 = gMe->at67;
    predict.at3c = gMe->at6b;
    predict.at40 = gMe->at6f;
    predict.at44 = gMe->at73;
    memcpy(&predictOld, &predict, sizeof(VIEW));
}

static void fakePlayerProcess(PLAYER *pPlayer, INPUT *pInput);
static void fakeActProcessSprites(void);

void viewUpdatePrediction(INPUT *pInput)
{
    memcpy(&predictOld, &predict, sizeof(VIEW));
    short bakCStat = gMe->pSprite->cstat;
    gMe->pSprite->cstat = 0;
    fakePlayerProcess(gMe, pInput);
    fakeActProcessSprites();
    gMe->pSprite->cstat = bakCStat;
    predictFifo[gPredictTail&255] = predict;
    gPredictTail++;
}

void func_158B4(PLAYER *pPlayer)
{
    POSTURE *pPosture = &gPosture[pPlayer->at5f][predict.at48];
    predict.at38 = predict.at58 - pPosture->at24;
    predict.at40 = predict.at58 - pPosture->at28;
}

static void fakeProcessInput(PLAYER *pPlayer, INPUT *pInput)
{
    POSTURE *pPosture = &gPosture[pPlayer->at5f][predict.at48];
    predict.at70 = pInput->syncFlags.run;
    predict.at71 = pInput->buttonFlags.jump;
    switch (predict.at48)
    {
        case 1:
        {
            int vel = 0;
            int sinVal = Sin(predict.at30);
            int cosVal = Cos(predict.at30);
            if (pInput->forward)
            {
                if (pInput->forward > 0)
                    vel = pInput->forward * pPosture->at0;
                else
                    vel = pInput->forward * pPosture->at8;
                predict.at5c += mulscale30(vel, cosVal);
                predict.at60 += mulscale30(vel, sinVal);
            }
            if (pInput->strafe)
            {
                vel = pInput->strafe * pPosture->at4;
                predict.at5c += mulscale30(vel, sinVal);
                predict.at60 -= mulscale30(vel, cosVal);
            }
            break;
        }
        default:
            if (predict.at6a < 256)
            {
                int drag = 0x10000;
                int vel = 0;
                if (predict.at6a > 0)
                    drag -= divscale16(predict.at6a, 256);
                int sinVal = Sin(predict.at30);
                int cosVal = Cos(predict.at30);
                if (pInput->forward)
                {
                    if (pInput->forward > 0)
                        vel = pInput->forward * pPosture->at0;
                    else
                        vel = pInput->forward * pPosture->at8;
                    if (predict.at6a)
                        vel = mulscale16(vel, drag);
                    predict.at5c += mulscale30(vel, cosVal);
                    predict.at60 += mulscale30(vel, sinVal);
                }
                if (pInput->strafe)
                {
                    vel = pInput->strafe * pPosture->at4;
                    if (predict.at6a)
                        vel = mulscale16(vel, drag);
                    predict.at5c += mulscale30(vel, sinVal);
                    predict.at60 -= mulscale30(vel, cosVal);
                }
            }
            break;
    }
    if (pInput->turn != 0)
        predict.at30 = (short)((predict.at30 + ((pInput->turn * 4) >> 4)) & 2047);

    if (pInput->keyFlags.spin180 && predict.at4c == 0)
        predict.at4c = -1024;
    if (predict.at4c < 0)
    {
        int speed = predict.at48 == 1 ? 64 : 128;
        predict.at4c = ClipHigh(predict.at4c + speed, 0);
        predict.at30 += (short)speed;
    }
    if (!predict.at71)
        predict.at6f = 0;
    switch (predict.at48)
    {
        case 1:
            if (predict.at71)
                predict.at64 -= 23301;
            if (pInput->buttonFlags.crouch)
                predict.at64 += 23301;
            break;
        case 2:
            if (!pInput->buttonFlags.crouch)
                predict.at48 = 0;
            break;
        default:
            if (!predict.at6f && predict.at71 && predict.at6a == 0)
            {
                if (packItemActive(pPlayer, 4))
                    predict.at64 = -1529173;
                else
                    predict.at64 = -764586;
                predict.at6f = 1;
            }
            if (pInput->buttonFlags.crouch)
                predict.at48 = 2;
            break;
    }
    if (predict.at6e && !pInput->buttonFlags.lookUp && !pInput->buttonFlags.lookDown)
    {
        if (predict.at20 < 0)
        {
            predict.at20 = ClipHigh(predict.at20 + 4, 0);
        }
        if (predict.at20 > 0)
        {
            predict.at20 = ClipLow(predict.at20 - 4, 0);
        }
        if (predict.at20 == 0)
            predict.at6e = 0;
    }
    else
    {
        if (pInput->buttonFlags.lookUp)
        {
            predict.at20 = ClipHigh(predict.at20 + 4, 60);
        }
        if (pInput->buttonFlags.lookDown)
        {
            predict.at20 = ClipLow(predict.at20 - 4, -60);
        }
    }
    if (pInput->mlook < 0)
    {
        predict.at20 = (schar)ClipRange(predict.at20 + ((pInput->mlook+3)>>2), -60, 60);
    }
    else
    {
        predict.at20 = (schar)ClipRange(predict.at20 + (pInput->mlook>>2), -60, 60);
    }

    if (predict.at20 > 0)
    {
        predict.at24 = mulscale30(120, Sin(predict.at20 * 8));
    }
    else if (predict.at20 < 0)
    {
        predict.at24 = mulscale30(180, Sin(predict.at20 * 8));
    }
    else
        predict.at24 = 0;

    int nSector = predict.at68;
    int hit = predict.at75.florhit & 0xe000;
    BOOL onFloor = predict.at6a < 16 && (hit == 0x4000 || hit == 0);
    if (onFloor && (sector[nSector].floorstat&kSectorStat1) != 0)
    {
        int floorZ = getflorzofslope(nSector, predict.at50, predict.at54);
        int newX = predict.at50 + mulscale30(64, Cos(predict.at30));
        int newY = predict.at54 + mulscale30(64, Sin(predict.at30));
        short newSector = nSector;
        updatesector(newX, newY, &newSector);
        if (newSector == nSector)
        {
            int newFloorZ = getflorzofslope(newSector, newX, newY);
            predict.at28 = interpolate16(predict.at28, (floorZ-newFloorZ)>>3, 0x4000);
        }
    }
    else
    {
        predict.at28 = interpolate16(predict.at28, 0, 0x4000);
        if (klabs(predict.at28) < 4)
            predict.at28 = 0;
    }
    predict.at2c = (-predict.at24)<<7;
}

static void fakePlayerProcess(PLAYER *pPlayer, INPUT *pInput)
{
    SPRITE *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    POSTURE *pPosture = &gPosture[pPlayer->at5f][predict.at48];

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);

    top += predict.at58-pSprite->z;
    bottom += predict.at58-pSprite->z;

    int floordist = (bottom-predict.at58)/4;
    int ceildist = (predict.at58-top)/4;

    int clipdist = pSprite->clipdist<<2;

    short nSector = predict.at68;
    if (!gNoClip)
    {
        pushmove(&predict.at50, &predict.at54, &predict.at58, &predict.at68, clipdist, ceildist, floordist, 0x10001);
        if (predict.at68 == -1)
            predict.at68 = nSector;
    }
    fakeProcessInput(pPlayer, pInput);

    int vel = approxDist(predict.at5c, predict.at60);
    
    int tmp;
    predict.at3c = interpolate16(predict.at3c, predict.at64, 0x7000);
    tmp = predict.at58 - pPosture->at24 - predict.at38;
    if (tmp > 0)
        predict.at3c += mulscale16(tmp<<8, 40960);
    else
        predict.at3c += mulscale16(tmp<<8, 6144);
    predict.at38 += predict.at3c >> 8;

    predict.at44 = interpolate16(predict.at44, predict.at64, 0x5000);
    tmp = predict.at58 - pPosture->at28 - predict.at40;
    if (tmp > 0)
        predict.at44 += mulscale16(tmp<<8, 32768);
    else
        predict.at44 += mulscale16(tmp<<8, 3072);
    predict.at40 += predict.at44 >> 8;

    predict.at0 = ClipLow(predict.at0 - 4, 0);

    vel >>= 16;

    switch (predict.at48)
    {
        case 1:
            predict.at4 = (predict.at4+17)&2047;
            predict.at14 = (predict.at14+17)&2047;
            predict.at8 = mulscale30(pPosture->at14*10,Sin(predict.at4*2));
            predict.atc = mulscale30(pPosture->at18*predict.at0,Sin(predict.at4-256));
            predict.at18 = mulscale30(pPosture->at1c*predict.at0,Sin(predict.at14*2));
            predict.at1c = mulscale30(pPosture->at20*predict.at0,Sin(predict.at14-341));
            break;
        default:
            if (pXSprite->at30_0 < 256)
            {
                predict.at4 = (predict.at4+(pPosture->atc[predict.at70]*4))&2047;
                predict.at14 = (predict.at14+(pPosture->atc[predict.at70]*4)/2)&2047;
                if (predict.at70)
                {
                    if (predict.at0 < 60) predict.at0 = ClipHigh(predict.at0 + vel, 60);
                }
                else
                {
                    if (predict.at0 < 30) predict.at0 = ClipHigh(predict.at0 + vel, 30);
                }
            }
            predict.at8 = mulscale30(pPosture->at14*predict.at0,Sin(predict.at4*2));
            predict.atc = mulscale30(pPosture->at18*predict.at0,Sin(predict.at4-256));
            predict.at18 = mulscale30(pPosture->at1c*predict.at0,Sin(predict.at14*2));
            predict.at1c = mulscale30(pPosture->at20*predict.at0,Sin(predict.at14-341));
            break;
    }
    if (pXSprite->health == 0)
        return;

    predict.at72 = 0;
    if (predict.at48 == 1)
    {
        predict.at72 = 1;
        int nSector = predict.at68;
        int nSprite = gLowerLink[nSector];
        if (nSprite > 0)
        {
            if (sprite[nSprite].type == kMarker14 || sprite[nSprite].type == kMarker10)
            {
                if (predict.at38 < getceilzofslope(nSector, predict.at50, predict.at54))
                    predict.at72 = 0;
            }
        }
    }
}

static void fakeMoveDude(SPRITE *pSprite)
{
    PLAYER *pPlayer = NULL;
    if (IsPlayerSprite(pSprite))
        pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax, 816+LDIFF1);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    top += predict.at58 - pSprite->z;
    bottom += predict.at58 - pSprite->z;
    int var28 = (bottom-predict.at58)/4;
    int var5c = (predict.at58-top)/4;
    int clipdist = pSprite->clipdist<<2;
    int nSector = predict.at68;
    dassert(nSector >= 0 && nSector < kMaxSectors, 828+LDIFF1);
    if (predict.at5c || predict.at60)
    {
        if (pPlayer && gNoClip)
        {
            int x = predict.at50;
            int y = predict.at54;
            predict.at50 += predict.at5c >> 12;
            predict.at54 += predict.at60 >> 12;
            if (!FindSector(predict.at50, predict.at54, &nSector))
                nSector = predict.at68;
        }
        else
        {
            short cstatbak = pSprite->cstat;
            pSprite->cstat &= ~0x101;
            predict.at75.hit = ClipMove(&predict.at50, &predict.at54, &predict.at58, &nSector, predict.at5c >> 12, predict.at60 >> 12, clipdist, var5c, var28, 0x13001);
            if (nSector == -1)
                nSector = predict.at68;
                    
            if (sector[nSector].type >= kSectorType612 && sector[nSector].type <= kSectorType617)
            {
                short nSector2 = nSector;
                pushmove(&predict.at50, &predict.at54, &predict.at58, &nSector2, clipdist, var5c, var28, 0x10001);
                if (nSector2 != -1)
                    nSector = nSector2;
            }

            dassert(nSector >= 0, 866+LDIFF1);

            pSprite->cstat = cstatbak;
        }
        switch (predict.at75.hit & 0xe000)
        {
            case 0x8000:
            {
                int nWall = predict.at75.hit & 0x1fff;
                WALL *pWall = &wall[nWall];
                if (pWall->nextsector != -1)
                {
                    SECTOR *pSector = &sector[pWall->nextsector];
                    if (pSector->floorz > top || pSector->ceilingz < bottom)
                    {
                        actWallBounceVector(&predict.at5c, &predict.at60, nWall, 0);
                        break;
                    }
                }
                actWallBounceVector(&predict.at5c, &predict.at60, nWall, 0);
                break;
            }
        }
    }
    if (nSector != predict.at68)
    {
        dassert(nSector >= 0 && nSector < kMaxSectors, 910+LDIFF1);
        predict.at68 = nSector;
    }
    BOOL var8 = 0;
    BOOL var4 = 0;
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->at13_4)
            var8 = 1;
        if (pXSector->at13_5)
            var4 = 1;
    }
    int link1 = gUpperLink[nSector];
    int link2 = gLowerLink[nSector];
    if (link1 >= 0)
    {
        if (sprite[link1].type == kMarker9 || sprite[link1].type == kMarker13)
            var4 = 1;
    }
    if (link2 >= 0)
    {
        if (sprite[link2].type == kMarker10 || sprite[link2].type == kMarker14)
            var4 = 1;
    }
    if (pPlayer)
        clipdist += 16;

    if (predict.at64)
        predict.at58 += predict.at64 >> 8;

    long var54, var50, var4c, var48;
    SPRITE tSprite = *pSprite;
    tSprite.x = predict.at50;
    tSprite.y = predict.at54;
    tSprite.z = predict.at58;
    tSprite.sectnum = predict.at68;
    GetZRange(&tSprite, &var54, &var50, &var4c, &var48, clipdist, 0x10001);
    GetSpriteExtents(&tSprite, &top, &bottom);
    if (predict.at73 & kSpriteFlag1)
    {
        int tmp = 58254;
        if (var4)
        {
            if (var8)
            {
                int z = getceilzofslope(nSector, predict.at50, predict.at54);
                if (top < z)
                    tmp += ((bottom-z)*-80099)/(bottom-top);
                else
                    tmp=0;
            }
            else
            {
                int z = getflorzofslope(nSector, predict.at50, predict.at54);
                if (bottom > z)
                    tmp += ((bottom-z)*-80099)/(bottom-top);
            }
        }
        else
        {
            if (var8)
                tmp = 0;
            else
            {
                if (bottom >= var4c)
                    tmp = 0;
            }
        }
        if (tmp)
        {
            predict.at58 += ((tmp*4)/2)>>8;
            predict.at64 += tmp;
        }
    }
    GetSpriteExtents(&tSprite, &top, &bottom);
    if (bottom >= var4c)
    {
        long var4c_bak = var4c, var48_bak = var48;
        GetZRange(&tSprite, &var54, &var50, &var4c, &var48, pSprite->clipdist<<2, 0x13001);
        if (bottom <= var4c && predict.at58-var4c_bak < var28)
        {
            var4c = var4c_bak;
            var48 = var48_bak;
        }
    }
    if (bottom >= var4c)
    {
        predict.at75.florhit = var48;
        predict.at58 += var4c-bottom;
        long var44 = predict.at64-velFloor[predict.at68];
        if (var44 > 0)
        {
            actFloorBounceVector(&predict.at5c, &predict.at60, &var44, predict.at68, 0);
            predict.at64 = var44;
            if (klabs(var44) < 0x10000)
            {
                predict.at64 = velFloor[predict.at68];
                predict.at73 &= ~kSpriteFlag2;
            }
            else
                predict.at73 |= kSpriteFlag2;
        }
        else if (predict.at64 == 0)
        {
            predict.at73 &= ~kSpriteFlag2;
        }
    }
    else
    {
        predict.at75.florhit = 0;
        if (predict.at73 & kSpriteFlag1)
            predict.at73 |= kSpriteFlag2;
    }
    if (top <= var54)
    {
        predict.at75.ceilhit = var50;
        predict.at58 += ClipLow(var54-top, 0);
        if (predict.at64 <= 0 && (predict.at73 & kSpriteFlag2))
            predict.at64 = mulscale16(-predict.at64,0x2000);
    }
    else
        predict.at75.ceilhit = 0;

    GetSpriteExtents(&tSprite, &top, &bottom);

    predict.at6a = ClipLow(var4c - bottom, 0) >> 8;
    if (predict.at5c || predict.at60)
    {
        if ((var48 & 0xe000) == 0xc000)
        {
            int nSprite = var48 & 0x1fff;
            if ((sprite[nSprite].cstat&kSpriteMask) == kSpriteFace)
            {
                predict.at5c += mulscale(4,predict.at50-sprite[nSprite].x,2);
                predict.at60 += mulscale(4,predict.at54-sprite[nSprite].y,2);
                return;
            }
        }
        int nSector = pSprite->sectnum;
        int nXSector = sector[nSector].extra;
        if (nXSector > 0)
        {
            XSECTOR *pXSector = &xsector[nXSector];
            if (pXSector->at13_4)
                return;
        }
        if (predict.at6a < 0x100)
        {
            int drag = gDudeDrag;
            if (predict.at6a > 0)
                drag -= kscale(drag, predict.at6a, 0x100);
            predict.at5c -= mulscale16r(predict.at5c, drag);
            predict.at60 -= mulscale16r(predict.at60, drag);
            if (approxDist(predict.at5c, predict.at60) < 0x1000)
                predict.at5c = predict.at60 = 0;
        }
    }
}

void fakeActAirDrag(SPRITE *pSprite, int num)
{
    int vbp = 0;
    int v4 = 0;
    int nSector = predict.at68;
    dassert(nSector >= 0 && nSector < kMaxSectors, 1175+LDIFF1);
    SECTOR *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    if (nXSector > 0)
    {
        dassert(nXSector < kMaxXSectors, 1180+LDIFF1);
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->at35_1 && (pXSector->at37_6 || pXSector->at1_7))
        {
            int vcx = pXSector->at35_1 <<12;
            if (!pXSector->at37_6 && pXSector->at1_7)
                vcx = mulscale16(vcx, pXSector->at1_7);
            vbp = mulscale30(vcx, Cos(pXSector->at36_3));
            v4 = mulscale30(vcx, Sin(pXSector->at36_3));
        }
    }
    predict.at5c += mulscale16(vbp - predict.at5c, num);
    predict.at60 += mulscale16(v4 - predict.at60, num);
    predict.at64 -= mulscale16(predict.at64, num);
}

static void fakeActProcessSprites(void)
{
    SPRITE *pSprite = gMe->pSprite;
    if (pSprite->statnum != 6)
        return;
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites, 1209+LDIFF1);
    int nSector = predict.at68;
    int nXSector = sector[nSector].extra;
    XSECTOR *pXSector = NULL;
    if (nXSector > 0)
    {
        dassert(nXSector > 0 && nXSector < kMaxXSectors, 1217+LDIFF1);
        dassert(xsector[nXSector].reference == nSector, 1218+LDIFF1);
        pXSector = &xsector[nXSector];
    }
    if (pXSector)
    {
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        top += predict.at58 - pSprite->z;
        bottom += predict.at58 - pSprite->z;
        int fz = getflorzofslope(nSector, predict.at50, predict.at54);
        if (bottom > fz)
        {
            int uv = 0;
            int ua = pXSector->at15_0;
            if (pXSector->at13_0 || pXSector->at1_6 || pXSector->at1_7)
            {
                uv = pXSector->at14_0 << 9;
                if (!pXSector->at13_0 && pXSector->at1_7)
                    uv = mulscale16(uv, pXSector->at1_7);
            }
                    
            if (sector[nSector].floorstat&kSectorStat6)
            {
                ua += GetWallAngle(sector[nSector].wallptr)+512;
                ua &= 2047;
            }
            int dx = mulscale30(uv, Cos(ua));
            int dy = mulscale30(uv, Sin(ua));
            predict.at5c += dx;
            predict.at60 += dy;
        }
    }
    if (pXSector && pXSector->at13_4)
        fakeActAirDrag(pSprite, 5376);
    else
        fakeActAirDrag(pSprite, 128);

    if ((predict.at73 & kSpriteFlag2) || predict.at5c || predict.at60 || predict.at64 || velFloor[predict.at68] || velCeil[predict.at68])
    {
        fakeMoveDude(pSprite);
    }
}

void viewCorrectPrediction(void)
{
    if (gGameOptions.nGameType == GAMETYPE_0) return;
    VIEW *pView = &predictFifo[(gNetFifoTail-1)&255];
    if (pView->at30 == gMe->pSprite->ang && pView->at24 == gMe->at7b
        && pView->at50 == gMe->pSprite->x && pView->at54 == gMe->pSprite->y && pView->at58 == gMe->pSprite->z)
        return;
    viewInitializePrediction();
    predictOld = gPrevView[myconnectindex];
    gPredictTail = gNetFifoTail;
    while (gPredictTail < gNetFifoHead[myconnectindex])
    {
        viewUpdatePrediction(&gFifoInput[gPredictTail&255][myconnectindex]);
    }
}

void viewBackupView(int nPlayer)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->at30 = pPlayer->pSprite->ang;
    pView->at50 = pPlayer->pSprite->x;
    pView->at54 = pPlayer->pSprite->y;
    pView->at38 = pPlayer->at67;
    pView->at34 = pPlayer->at6f-pPlayer->at67-0xc00;
    pView->at24 = pPlayer->at7b;
    pView->at28 = pPlayer->at7f;
    pView->at2c = pPlayer->at83;
    pView->at8 = pPlayer->at3f;
    pView->atc = pPlayer->at43;
    pView->at18 = pPlayer->at4f;
    pView->at1c = pPlayer->at53;
}

void viewClearInterpolations(void)
{
    nInterpolations = 0;
    memset(gInterpolateSprite, 0, sizeof(gInterpolateSprite));
    memset(gInterpolateWall, 0, sizeof(gInterpolateWall));
    memset(gInterpolateSector, 0, sizeof(gInterpolateSector));
}

void viewAddInterpolation(void *data, INTERPOLATE_TYPE type)
{
    if (nInterpolations == 4096)
        ThrowError(1328+LDIFF1)("Too many interpolations");
    INTERPOLATE *pInterpolate = &gInterpolation[nInterpolations++];
    pInterpolate->pointer = data;
    pInterpolate->type = type;
    switch (type)
    {
    case INTERPOLATE_TYPE_INT:
        pInterpolate->value = *((int*)data);
        break;
    case INTERPOLATE_TYPE_SHORT:
        pInterpolate->value = *((short*)data);
        break;
    }
}

void CalcInterpolations(void)
{
    int i;
    INTERPOLATE *pInterpolate = gInterpolation;
    for (i = 0; i < nInterpolations; i++, pInterpolate++)
    {
        switch (pInterpolate->type)
        {
        case INTERPOLATE_TYPE_INT:
            pInterpolate->value2 = *((int*)pInterpolate->pointer);
            *((int*)pInterpolate->pointer) = interpolate16(pInterpolate->value, *((int*)pInterpolate->pointer), gInterpolate);
            break;
        case INTERPOLATE_TYPE_SHORT:
            pInterpolate->value2 = *((short*)pInterpolate->pointer);
            *((short*)pInterpolate->pointer) = interpolate16(pInterpolate->value, *((short*)pInterpolate->pointer), gInterpolate);
            break;
        }
    }
}

void RestoreInterpolations(void)
{
    int i;
    INTERPOLATE *pInterpolate = gInterpolation;
    for (i = 0; i < nInterpolations; i++, pInterpolate++)
    {
        switch (pInterpolate->type)
        {
        case INTERPOLATE_TYPE_INT:
            *((int*)pInterpolate->pointer) = pInterpolate->value2;
            break;
        case INTERPOLATE_TYPE_SHORT:
            *((short*)pInterpolate->pointer) = pInterpolate->value2;
            break;
        }
    }
}

void viewDrawChar(QFONT *pFont, char chr, int x, int y, byte *pLookup)
{
    dassert(pFont != NULL, 1400+LDIFF1);
    QFONTCHAR *pCharInfo = &pFont->at20[chr];
    if (pCharInfo->w == 0 || pCharInfo->h == 0)
        return;
    y += pFont->atf + pCharInfo->oy;
    x = mulscale16(x, xscale);
    y = mulscale16(y, yscale);
    int wScaled = mulscale16(pCharInfo->w, xscale);
    int hScaled = mulscale16(pCharInfo->h, yscale);

    Rect rect1(x, y, x + wScaled, y + hScaled);
    Rect rect2(0, 0, xdim, ydim);

    rect1 &= rect2;

    if (!rect1)
        return;

    byte *data = &pFont->at820[pCharInfo->offset];
    for (int i = 0; i < 4; i++)
    {
        palookupoffse[i] = pLookup;
        vince[i] = ystep;
    }

    byte *fbuf = frameplace+ylookup[rect1.f_4] + rect1.f_0;
    x = rect1.f_0;
    int xfrac = 0;
    while (x < rect1.f_8 && (x & 3))
    {
        byte *s = data+(xfrac>>16)*pCharInfo->h;
        mvlineasm1(ystep, pLookup, hScaled-1, 0, s, fbuf);
        fbuf++; x++; xfrac += xstep;
    }
    while (x+3 < rect1.f_8)
    {
        for (i = 0; i < 4; i++)
        {
            vplce[i] = 0;
            bufplce[i] = data+(xfrac>>16)*pCharInfo->h;
            xfrac += xstep;
        }
        mvlineasm4(hScaled, fbuf);
        fbuf += 4; x += 4;
    }
    while (x < rect1.f_8)
    {
        byte *s = data+(xfrac>>16)*pCharInfo->h;
        mvlineasm1(ystep, pLookup, hScaled-1, 0, s, fbuf);
        fbuf++; x++; xfrac += xstep;
    }
}

void viewDrawText(int nFont, char * string, int x, int y, int nShade, int nPalette, int position, BOOL shadow)
{
#if APPVER_BLOODREV >= AV_BR_BL120
    char *s;

    if (nFont < 0 || nFont >= 5 || !string) return;
    FONT *pFont = &gFont[nFont];

    if (position != 0)
    {
        int width = -pFont->space;
        for (s = string; *s; s++)
        {
            int nTile = ((*s-32)&127)+pFont->tile;
            if (tilesizx[nTile] == 0 || tilesizy[nTile] == 0)
                continue;
            width += tilesizx[nTile]+pFont->space;
        }
        if (position == 1)
            width >>= 1;
        x -= width;
    }
    for (s = string; *s; s++)
    {
        int nTile = ((*s-32)&127)+pFont->tile;
        if (tilesizx[nTile] == 0 || tilesizy[nTile] == 0)
            continue;
        int tx = x;
        int ty = y;
        if (shadow)
        {
            viewDrawSprite((tx+1)<<16, (ty+1)<<16, 65536, 0, nTile, 127, nPalette, 26, 0, 0, xdim-1, ydim-1);
        }
        viewDrawSprite(tx<<16, ty<<16, 65536, 0, nTile, nShade, nPalette, 26, 0, 0, xdim-1, ydim-1);
        x += tilesizx[nTile]+pFont->space;
    }
#else
    char *s;

    dassert(string != NULL, 1369+LDIFF1+101);
    DICTNODE *hFont = gSysRes.Lookup(nFont, "QFN");
    dassert(hFont != NULL, 1372+LDIFF1+101);

    QFONT *pFont = (QFONT*)gSysRes.Lock(hFont);
    byte* pPalookup = palookup[nPalette] + (getpalookup(0, nShade) << 8);
    byte* pShadowPal = palookup[nPalette] + (getpalookup(0, 127) << 8);

    setupmvlineasm(16);

    if (position != 0)
    {
        int width = -pFont->at11;
        for (s = string; *s; s++)
        {
            width += pFont->at20[*s].ox + pFont->at11;
        }
        if (position == 1)
            width >>= 1;
        x -= width;
    }
    for (s = string; *s; s++)
    {
        if (shadow)
        {
            viewDrawChar(pFont, *s, x + 1, y + 1, pShadowPal);
        }
        viewDrawChar(pFont, *s, x, y, pPalookup);
        x += pFont->at20[*s].ox + pFont->at11;
    }
    gSysRes.Unlock(hFont);
#endif
}

void viewTileSprite(int nTile, int nShade, int nPalette, int x1, int y1, int x2, int y2)
{
    int x, y, i;

    Rect rect1(x1, y1, x2, y2);
    Rect rect2(0, 0, xdim, ydim);
    rect1 &= rect2;

    if (!rect1)
        return;

    dassert(nTile >= 0 && nTile < kMaxTiles, 1544+LDIFF2);
    int width = tilesizx[nTile];
    int height = tilesizy[nTile];
    int pixels = width * height;

    if (pixels == 0)
        return;

    byte *pPalookup = palookup[nPalette] + (getpalookup(0, nShade)<<8);
    byte *pTile = tileLoadTile(nTile);
    byte *pEnd = pTile+pixels;

    setupvlineasm(16);

    for (i = 0; i < 4; i++)
    {
        palookupoffse[i] = pPalookup;
        vince[i] = 65536;
    }

    int yend;
    for (y = rect1.f_4; y < rect1.f_c; y = yend)
    {
        yend = ClipHigh(IncBy(y, height), rect1.f_c);
        x = rect1.f_0;
        byte *pSource = pTile + (x % width)*height+(y % height);
        byte *pDest = frameplace+ylookup[y]+x;
        while (x < rect1.f_8 && (x & 3))
        {
            vlineasm1(65536, pPalookup, yend-y-1, 0, pSource, pDest);
            pSource += height;
            if (pSource >= pEnd)
                pSource -= pixels;
            pDest++;
            x++;
        }
        while (x+3 < rect1.f_8)
        {
            for (i = 0; i < 4; i++)
            {
                bufplce[i] = pSource;
                pSource += height;
                if (pSource >= pEnd)
                    pSource -= pixels;
                vplce[i] = 0;
            }
            vlineasm4(yend-y, pDest);
            pDest += 4;
            x += 4;
        }
        while (x < rect1.f_8)
        {
            vlineasm1(65536, pPalookup, yend-y-1, 0, pSource, pDest);
            pSource += height;
            if (pSource >= pEnd)
                pSource -= pixels;
            pDest++;
            x++;
        }
    }
}

void InitStatusBar(void)
{
    tileLoadTile(2200);
}

void DrawStatSprite(int nTile, int x, int y, int nShade = 0, int nPalette = 0, uint nStat = 0)
{
    rotatesprite(x<<16, y<<16, 65536, 0, nTile, nShade, nPalette, nStat | 74, 0, 0, xdim-1, ydim-1);
}

void DrawStatMaskedSprite(int nTile, int x, int y, int nShade = 0, int nPalette = 0, uint nStat = 0)
{
    rotatesprite(x<<16, y<<16, 65536, 0, nTile, nShade, nPalette, nStat | 10, 0, 0, xdim-1, ydim-1);
}

void DrawStatNumber(char *pFormat, int nNumber, int nTile, int x, int y, int nShade = 0, int nPalette = 0)
{
    char tempbuf[80];
    int width = tilesizx[nTile] + 1;
    sprintf(tempbuf, pFormat, nNumber);
    for (uint i = 0; i < strlen(tempbuf); i++, x += width)
    {
        if (tempbuf[i] == ' ') continue;
        DrawStatMaskedSprite(nTile+tempbuf[i]-'0', x, y, nShade, nPalette);
    }
}

void TileHGauge(int nTile, int x, int y, int nMult, int nDiv)
{
    int bx = tilesizx[nTile]*nMult/nDiv;
    rotatesprite(x<<16, y<<16, 65536, 0, nTile, 0, 0, 90, 0, 0, mulscale16(x+bx,xscale)-1, ydim-1);
}

int gPackIcons[6] = {
    2569, 2564, 2566, 2568, 2560, 829
};

void tenPlayerDebugInfo(char *a1, int pid);

void viewDrawPack(PLAYER *pPlayer, int x, int y)
{
#if APPVER_BLOODREV >= AV_BR_BL111A
    static int int_14C508;
#endif
    if (pPlayer->at31d)
    {
        int width = 0;
        int nPacks = 0;
        int packs[5];
        for (int i = 0; i < 5; i++)
        {
            if (pPlayer->packInfo[i].at1)
            {
                packs[nPacks++] = i;
                width += tilesizx[gPackIcons[i]] + 1;
            }
        }
        x -= width / 2;
        for (i = 0; i < nPacks; i++)
        {
            int nPack = packs[i];
            DrawStatSprite(2568, x+1, y-8);
            DrawStatSprite(2568, x+1, y-6);
            DrawStatSprite(gPackIcons[nPack], x+1, y+1);
            if (nPack == pPlayer->at321)
                DrawStatMaskedSprite(2559, x+1, y+1);
            int nShade = pPlayer->packInfo[nPack].at0 ? 4 : 24;
            DrawStatNumber("%3d", pPlayer->packInfo[nPack].at1, 2250, x-4, y-13, nShade);
            x += tilesizx[gPackIcons[nPack]] + 1;
        }
    }
#if APPVER_BLOODREV >= AV_BR_BL111A
    if (pPlayer->at31d != int_14C508)
    {
        viewUpdatePages();
    }
    int_14C508 = pPlayer->at31d;
#endif
}

void DrawPackItemInStatusBar(PLAYER *pPlayer, int x, int y, int x2, int y2)
{
    if (pPlayer->at321 < 0) return;

    DrawStatSprite(gPackIcons[pPlayer->at321], x, y);
    DrawStatNumber("%3d", pPlayer->packInfo[pPlayer->at321].at1, 2250, x2, y2, 4);
}

static void UpdateStatusBar(int arg)
{
    PLAYER *pPlayer = gView;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    int i;

    int nPalette = 0;

    if (gGameOptions.nGameType == GAMETYPE_3)
    {
        nPalette = (pPlayer->at2ea & 1) ? 7 : 10;
    }

    if (gViewSize < 0) return;

    if (gViewSize <= 1)
    {
        if (pPlayer->at1ba)
            TileHGauge(2260, 124, 175, pPlayer->at1ba, 65536);
        else
            viewDrawPack(pPlayer, 166, 200-tilesizy[2201]/2);
    }
    if (gViewSize == 1)
    {
        DrawStatSprite(2201, 34, 187, 16, nPalette);
        if (pXSprite->health >= 16 || (gGameClock&16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health>>4, 2190, 8, 183);
        }
        if (pPlayer->atbd != 0 && pPlayer->atc7 != -1)
        {
            int num = pPlayer->at181[pPlayer->atc7];
            if (pPlayer->atc7 == 6)
                num /= 10;
            DrawStatNumber("%3d", num, 2240, 42, 183);
        }
        DrawStatSprite(2173, 284, 187, 16, nPalette);
        if (pPlayer->at33e[1])
        {
            TileHGauge(2207, 250, 175, pPlayer->at33e[1], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[1]>>4, 2230, 255, 178);
        }
        if (pPlayer->at33e[0])
        {
            TileHGauge(2209, 250, 183, pPlayer->at33e[0], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[0]>>4, 2230, 255, 186);
        }
        if (pPlayer->at33e[2])
        {
            TileHGauge(2208, 250, 191, pPlayer->at33e[2], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[2]>>4, 2230, 255, 194);
        }
        DrawPackItemInStatusBar(pPlayer, 286, 186, 302, 183);
    }
    else if (gViewSize > 1)
    {
        viewDrawPack(pPlayer, 160, 200-tilesizy[2200]);
#if APPVER_BLOODREV < AV_BR_BL111A
        pcBackground = numpages;
#endif
        DrawStatMaskedSprite(2200, 160, 172, 16, nPalette);
        DrawPackItemInStatusBar(pPlayer, 265, 186, 260, 172);
        if (pXSprite->health >= 16 || (gGameClock&16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health>>4, 2190, 86, 183);
        }
        if (pPlayer->atbd != 0 && pPlayer->atc7 != -1)
        {
            int num = pPlayer->at181[pPlayer->atc7];
            if (pPlayer->atc7 == 6)
                num /= 10;
            if (pPlayer->atc7 != -1)
                DrawStatNumber("%3d", num, 2240, 216, 183);
        }
        for (i = 9; i >= 1; i--)
        {
            int x = 135+((i-1)/3)*23;
            int y = 182+((i-1)%3)*6;
            int num = pPlayer->at181[i];
            if (i == 6)
                num /= 10;
            if (i == pPlayer->atc7)
            {
                DrawStatNumber("%3d", num, 2230, x, y, -128, 10);
            }
            else
            {
                DrawStatNumber("%3d", num, 2230, x, y, 32, 10);
            }
        }

        if (pPlayer->atc7 == 10)
        {
            DrawStatNumber("%2d", pPlayer->at181[10], 2230, 291, 194, -128, 10);
        }
        else
        {
            DrawStatNumber("%2d", pPlayer->at181[10], 2230, 291, 194, 32, 10);
        }

        if (pPlayer->atc7 == 11)
        {
            DrawStatNumber("%2d", pPlayer->at181[11], 2230, 309, 194, -128, 10);
        }
        else
        {
            DrawStatNumber("%2d", pPlayer->at181[11], 2230, 309, 194, 32, 10);
        }

        if (pPlayer->at33e[1])
        {
            TileHGauge(2207, 44, 174, pPlayer->at33e[1], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[1]>>4, 2230, 50, 177);
        }
        if (pPlayer->at33e[0])
        {
            TileHGauge(2209, 44, 182, pPlayer->at33e[0], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[0]>>4, 2230, 50, 185);
        }
        if (pPlayer->at33e[2])
        {
            TileHGauge(2208, 44, 190, pPlayer->at33e[2], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[2]>>4, 2230, 50, 193);
        }
        sprintf(buffer, "v%s", GetVersionString());
        viewDrawText(3, buffer, 20, 191, 32, 0, 1, 0);

        for (i = 0; i < 6; i++)
        {
            int x = 73+(i&1)*173;
            int y = 171+(i>>1)*11;
            if (pPlayer->at88[i+1])
                DrawStatSprite(2220+i, x, y);
            else
                DrawStatSprite(2220+i, x, y, 40, 5);
        }
        DrawStatMaskedSprite(2202, 118, 185, pPlayer->at2e ? 16 : 40);
        DrawStatMaskedSprite(2202, 201, 185, pPlayer->at2e ? 16 : 40);
        if (pPlayer->at1ba)
        {
            TileHGauge(2260, 124, 175, pPlayer->at1ba, 65536);
        }
    }
#if APPVER_BLOODREV >= AV_BR_BL111
    if (gGameOptions.nGameType >= GAMETYPE_1 || int_28E3D4 == 4)
#else
    if (gGameOptions.nGameType > GAMETYPE_1)
#endif
    {
        if (gGameOptions.nGameType == GAMETYPE_3)
        {
            int x = 1, y = 1;
            if (int_21EFD0[0] == 0 || (gGameClock & 8))
            {
                viewDrawText(0, "BLUE", x, y, -128, 10, 0, 0);
                int_21EFD0[0] = ClipLow(int_21EFD0[0]-arg, 0);
                sprintf(buffer, "%-3d", int_21EFB0[0]);
                viewDrawText(0, buffer, x, y+10, -128, 10, 0, 0);
            }
            x = 319;
            if (int_21EFD0[1] == 0 || (gGameClock & 8))
            {
                viewDrawText(0, "RED", x, y, -128, 7, 2, 0);
                int_21EFD0[1] = ClipLow(int_21EFD0[1]-arg, 0);
                sprintf(buffer, "%3d", int_21EFB0[1]);
                viewDrawText(0, buffer, x, y+10, -128, 7, 2, 0);
            }
        }
        else
        {
            for (i = (gNetPlayers-1) / 4; i >= 0; i--)
            {
                for (int j = 0; j < 4; j++)
                {
                    DrawStatSprite(2229, 40+j*80, 4+i*9, 16);
                }
            }
            int j = 0;
            for (i = connecthead; i >= 0; i = connectpoint2[i], j++)
            {
                int x = 80*(j&3);
                int y = 9*(j/4);
                int col = gPlayer[i].at2ea&3;
                if (gProfile[i].skill == 2)
                    sprintf(buffer, "%s", gProfile[i].name);
                else
                    sprintf(buffer, "%s [%d]", gProfile[i].name, gProfile[i].skill);
                strupr(buffer);
                viewDrawText(4, buffer, x+4, y+1, -128, 11+col, 0, 0);
                if (int_28E3D4 == 4)
                {
                    tenPlayerDebugInfo(buffer, i);
                }
                else
                {
                    sprintf(buffer, "%2d", gPlayer[i].at2c6);
                }
                viewDrawText(4, buffer, x+76, y+1, -128, 11+col, 2, 0);
            }
        }
    }
}

#define kLensSize 80

#if APPVER_BLOODREV >= AV_BR_BL120
enum FONTTYPE {
    kFont0 = 0,
    kFont1,
    kFont2,
    kFont3,
    kFont4,
};
#else
#define kFont0 0
#define kFont1 1
#define kFont2 2
#define kFont3 3
#define kFont4 4
#endif

void viewInit(void)
{
    tioPrint("Initializing status bar");
    InitStatusBar();
#if APPVER_BLOODREV >= AV_BR_BL120
    viewSetFont(0, 4096, 0);
    viewSetFont(1, 4192, 1);
    viewSetFont(2, 4288, 1);
    viewSetFont(3, 4384, 1);
    viewSetFont(4, 4480, 0);
#endif

    DICTNODE *hLens = gSysRes.Lookup("LENS", "DAT");
    dassert(hLens != NULL, 2143+LDIFF3);
    dassert(gSysRes.Size(hLens) == kLensSize * kLensSize * sizeof(int), 2144+LDIFF3);

    lensTable = (int*)gSysRes.Lock(hLens);
    byte *data = tileAllocTile(4077, kLensSize, kLensSize);
    memset(data, 255, kLensSize*kLensSize);
    gGameMessageMgr.SetState(gMessageState);
    gGameMessageMgr.SetCoordinates(1, 1);
    gGameMessageMgr.SetFont(gMessageFont == 0 ? kFont3 : kFont0);
    gGameMessageMgr.SetMaxMessages(gMessageCount);
    gGameMessageMgr.SetMessageTime(gMessageTime);

    for (int i = 0; i < 16; i++)
    {
        int_172CE0[i][0] = mulscale(rand(), 2048, 15);
        int_172CE0[i][1] = mulscale(rand(), 2048, 15);
        int_172CE0[i][2] = mulscale(rand(), 2048, 15);
    }
    gViewMap.func_25C38(0, 0, gZoom, 0, gFollowMap);
}

void viewResizeView(int size)
{
    gViewXCenter = xdim/2;
    gViewYCenter = ydim/2;
    xscale = divscale16(xdim, 320);
    yscale = divscale16(ydim, 200);
    xstep = divscale16(320, xdim);
    ystep = divscale16(200, ydim);
    gViewSize = ClipRange(size, 0, 6);
#ifndef _3DFX
    if (gViewSize == 0 || gViewSize == 1)
#endif
    {
        gViewX0 = 0;
        gViewY0 = 0;
        gViewX1 = xdim-1;
        gViewY1 = ydim-1;
        if (gGameOptions.nGameType > GAMETYPE_0 && gGameOptions.nGameType < GAMETYPE_3)
        {
            gViewY0 += (tilesizy[2229]*ydim*((gNetPlayers+3)/4))/200;
        }
#if APPVER_BLOODREV >= AV_BR_BL111A
        gViewX0S = divscale16(gViewX0, xscale);
        gViewY0S = divscale16(gViewY0, yscale);
        gViewX1S = divscale16(gViewX1, xscale);
        gViewY1S = divscale16(gViewY1, yscale);
        setview(gViewX0, gViewY0, gViewX1, gViewY1);
        gGameMessageMgr.SetCoordinates(gViewX0S+1, gViewY0S+1);
#else
        setview(gViewX0, gViewY0, gViewX1, gViewY1);
        gGameMessageMgr.SetCoordinates(gViewX0+1, gViewY0+1);
#endif
        return;
    }
    gViewX0 = 0;
    gViewY0 = 0;
    gViewX1 = xdim-1;
    gViewY1 = ydim-1-(25*ydim)/200;
    if (gGameOptions.nGameType > GAMETYPE_0 && gGameOptions.nGameType < GAMETYPE_3)
    {
        gViewY0 += (tilesizy[2229]*ydim*((gNetPlayers+3)/4))/200;
    }

    int height = gViewY1-gViewY0;
    gViewX0 += mulscale16((gViewSize-2)*xdim,4096);
    gViewX1 -= mulscale16((gViewSize-2)*xdim,4096);
    gViewY0 += mulscale16((gViewSize-2)*height,4096);
    gViewY1 -= mulscale16((gViewSize-2)*height,4096);
#if APPVER_BLOODREV >= AV_BR_BL111A
    gViewX0S = divscale16(gViewX0, xscale);
    gViewY0S = divscale16(gViewY0, yscale);
    gViewX1S = divscale16(gViewX1, xscale);
    gViewY1S = divscale16(gViewY1, yscale);
    setview(gViewX0, gViewY0, gViewX1, gViewY1);
    gGameMessageMgr.SetCoordinates(gViewX0S + 1, gViewY0S + 1);
    viewUpdatePages();
#else
    setview(gViewX0, gViewY0, gViewX1, gViewY1);
    gGameMessageMgr.SetCoordinates(gViewX0 + 1, gViewY0 + 1);
    pcBackground = numpages;
#endif
}

void UpdateFrame(void)
{
    int x0, y0, x1, y1;
    
    x0 = gViewX0;
    y0 = gViewY0;
    x1 = gViewX1;
    y1 = gViewY1;
    viewTileSprite(230, 0, 0, 0, 0, xdim, y0-3);
    viewTileSprite(230, 0, 0, 0, y1+4, xdim, ydim);
    viewTileSprite(230, 0, 0, 0, y0-3, x0-3, y1+4);
    viewTileSprite(230, 0, 0, x1+4, y0-3, xdim, y1+4);

    viewTileSprite(230, 20, 0, x0-3, y0-3, x0, y1+1);
    viewTileSprite(230, 20, 0, x0, y0-3, x1+4, y0);
    viewTileSprite(230, 10, 1, x1+1, y0, x1+4, y1+4);
    viewTileSprite(230, 10, 1, x0-3, y1+1, x1+1, y1+4);
}

void viewDrawInterface(int arg)
{
    if (gViewMode == 3 && gViewSize > 2 && pcBackground != 0)
    {
        UpdateFrame();
        pcBackground--;
    }
    UpdateStatusBar(arg);
}

SPRITE *viewInsertTSprite(int nSector, int nStatnum, SPRITE *pSprite)
{
    int nTSprite = spritesortcnt;
    SPRITE *pTSprite = &tsprite[nTSprite];
    memset(pTSprite, 0, sizeof(SPRITE));
    pTSprite->type = -spritesortcnt;
    pTSprite->sectnum = nSector;
    pTSprite->statnum = nStatnum;
    pTSprite->cstat = 128;
    pTSprite->xrepeat = 64;
    pTSprite->yrepeat = 64;
    pTSprite->owner = -1;
    pTSprite->extra = -1;
    spritesortcnt++;
    if (pSprite)
    {
        pTSprite->x = pSprite->x;
        pTSprite->y = pSprite->y;
        pTSprite->z = pSprite->z;
        pTSprite->owner = pSprite->owner;
        pTSprite->ang = pSprite->ang;
    }
    return &tsprite[nTSprite];
}

enum VIEW_EFFECT {
    VIEW_EFFECT_0 = 0,
    VIEW_EFFECT_1,
    VIEW_EFFECT_2,
    VIEW_EFFECT_3,
    VIEW_EFFECT_4,
    VIEW_EFFECT_5,
    VIEW_EFFECT_6,
    VIEW_EFFECT_7,
    VIEW_EFFECT_8,
    VIEW_EFFECT_9,
    VIEW_EFFECT_10,
    VIEW_EFFECT_11,
    VIEW_EFFECT_12,
    VIEW_EFFECT_13,
    VIEW_EFFECT_14,
    VIEW_EFFECT_15,
    VIEW_EFFECT_16,
    VIEW_EFFECT_17,
    VIEW_EFFECT_18,
    kViewEffectMax
};

int effectDetail[] = {
    4, 4, 4, 4, 0, 0, 0, 0, 0, 1, 4, 4, 0, 0, 0, 1, 0, 0, 0
};

struct WEAPONICON {
    short nTile;
    byte xRepeat;
    byte yRepeat;
};

WEAPONICON gWeaponIcon[] = {
    { -1, 0, 0 },
    { -1, 0, 0 },
    { 524, 32, 32 },
    { 559, 32, 32 },
    { 558, 32, 32 },
    { 526, 32, 32 },
    { 589, 32, 32 },
    { 618, 32, 32 },
    { 539, 32, 32 },
    { 800, 32, 32 },
    { 525, 32, 32 },
};

SPRITE *viewAddEffect(int nTSprite, VIEW_EFFECT nViewEffect)
{
    dassert(nViewEffect >= 0 && nViewEffect < kViewEffectMax, 2375+LDIFF4);
    SPRITE *pTSprite = &tsprite[nTSprite];
    if (gDetail < effectDetail[nViewEffect]) return NULL;

    if (spritesortcnt < kMaxViewSprites)
    {
        switch (nViewEffect)
        {
            case VIEW_EFFECT_18:
            {        
                for (int i = 0; i < 16; i++)
                {
                    SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                    int ang = (gFrameClock*2048)/120;
                    int nRand1 = int_172CE0[i][0];
                    int nRand2 = int_172CE0[i][1];
                    int nRand3 = int_172CE0[i][2];
                    long dx = mulscale30(512, Cos(ang+nRand3));
                    long dy = mulscale30(512, Sin(ang+nRand3));
                    long dz = 0;
                    RotateYZ(&dx, &dy, &dz, nRand1);
                    RotateXZ(&dx, &dy, &dz, nRand2);
                    pNSprite->x = pTSprite->x + dx;
                    pNSprite->y = pTSprite->y + dy;
                    pNSprite->z = pTSprite->z + (dz<<4);
                    pNSprite->picnum = 1720;
                    pNSprite->shade = -128;
                }
                break;
            }
            case VIEW_EFFECT_16:
            case VIEW_EFFECT_17:
            {
                int top, bottom;
                GetSpriteExtents(pTSprite, &top, &bottom);
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->z = top;
                pNSprite->shade = -128;
                pNSprite->pal = 0;
                if (nViewEffect == VIEW_EFFECT_16)
                    pNSprite->xrepeat = pNSprite->yrepeat = 24;
                else
                    pNSprite->xrepeat = pNSprite->yrepeat = 64;
                pNSprite->picnum = 3558;
                return pNSprite;
            }
            case VIEW_EFFECT_15:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->z = pTSprite->z;
                pNSprite->cstat |= 2;
                pNSprite->shade = -128;
                pNSprite->xrepeat = pTSprite->xrepeat;
                pNSprite->yrepeat = pTSprite->yrepeat;
                pNSprite->picnum = 2135;
                break;
            }
            case VIEW_EFFECT_14:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->shade = -128;
                pNSprite->pal = 0;
                pNSprite->xrepeat = pNSprite->yrepeat = 64;
                pNSprite->picnum = 2605;
                return pNSprite;
            }
            case VIEW_EFFECT_13:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->cstat |= 2;
                pNSprite->shade = 26;
                pNSprite->pal = 0;
                pNSprite->xrepeat = pNSprite->yrepeat = 64;
                pNSprite->picnum = 2089;
                break;
            }
            case VIEW_EFFECT_11:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                int top, bottom;
                GetSpriteExtents(pTSprite, &top, &bottom);
                pNSprite->z = top;
                pNSprite->cstat |= 2;
                pNSprite->shade = 26;
                pNSprite->pal = 0;
                pNSprite->xrepeat = pNSprite->yrepeat = 24;
                pNSprite->picnum = 626;
                break;
            }
            case VIEW_EFFECT_10:
            {
                int nAng = pTSprite->ang;
                if (pTSprite->cstat & kSpriteWall)
                {
                    nAng = (nAng+512)&2047;
                }
                else
                {
                    nAng = (nAng+1024)&2047;
                }
                for (int i = 0; i < 5 && spritesortcnt < kMaxViewSprites; i++)
                {
                    int nSector = pTSprite->sectnum;
                    SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, NULL);
                    pNSprite->ang = pTSprite->ang;
                    pNSprite->x = pTSprite->x + mulscale30((i+1)<<7, Cos(nAng));
                    pNSprite->y = pTSprite->y + mulscale30((i+1)<<7, Sin(nAng));
                    pNSprite->z = pTSprite->z;
                    dassert(nSector >= 0 && nSector < kMaxSectors, 2508+LDIFF4);
                    FindSector(pNSprite->x, pNSprite->y, pNSprite->z, &nSector);
                    pNSprite->sectnum = nSector;
                    pNSprite->owner = pTSprite->owner;
                    pNSprite->picnum = pTSprite->picnum;
                    pNSprite->cstat |= 2;
                    if (i < 2)
                        pNSprite->cstat |= 514;
                    pNSprite->shade = ClipLow(pTSprite->shade-16, -128);
                    pNSprite->xrepeat = pTSprite->xrepeat;
                    pNSprite->yrepeat = pTSprite->yrepeat;
                    pNSprite->picnum = pTSprite->picnum;
                }
                break;
            }
            case VIEW_EFFECT_8:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->z = pTSprite->z;
                pNSprite->shade = -128;
                pNSprite->xrepeat = pNSprite->yrepeat = (tilesizx[pTSprite->picnum]*pTSprite->xrepeat)/64;
                pNSprite->picnum = 908;
                pNSprite->statnum = 0;
                break;
            }
            case VIEW_EFFECT_6:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                int top, bottom;
                GetSpriteExtents(pTSprite, &top, &bottom);
                pNSprite->z = top;
                if (IsDudeSprite(pTSprite))
                    pNSprite->picnum = 672;
                else
                    pNSprite->picnum = 754;
                pNSprite->cstat |= 2;
                pNSprite->shade = 8;
                pNSprite->xrepeat = pTSprite->xrepeat;
                pNSprite->yrepeat = pTSprite->yrepeat;
                break;
            }
            case VIEW_EFFECT_7:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                int top, bottom;
                GetSpriteExtents(pTSprite, &top, &bottom);
                pNSprite->z = bottom;
                if (IsDudeSprite(pTSprite))
                    pNSprite->picnum = 672;
                else
                    pNSprite->picnum = 754;
                pNSprite->cstat |= 2;
                pNSprite->shade = 8;
                pNSprite->xrepeat = pTSprite->xrepeat;
                pNSprite->yrepeat = pTSprite->yrepeat;
                break;
            }
            case VIEW_EFFECT_4:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                int top, bottom;
                GetSpriteExtents(pTSprite, &top, &bottom);
                pNSprite->z = top;
                pNSprite->picnum = 2101;
                pNSprite->shade = -128;
                pNSprite->xrepeat = pNSprite->yrepeat = (tilesizx[pTSprite->picnum]*pTSprite->xrepeat)/32;
                break;
            }
            case VIEW_EFFECT_5:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                int top, bottom;
                GetSpriteExtents(pTSprite, &top, &bottom);
                pNSprite->z = bottom;
                pNSprite->picnum = 2101;
                pNSprite->shade = -128;
                pNSprite->xrepeat = pNSprite->yrepeat = (tilesizx[pTSprite->picnum]*pTSprite->xrepeat)/32;
                break;
            }
            case VIEW_EFFECT_0:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->z = getflorzofslope(pTSprite->sectnum, pNSprite->x, pNSprite->y);
                pNSprite->cstat |= 2;
                pNSprite->shade = 127;
                pNSprite->xrepeat = pTSprite->xrepeat;
                pNSprite->yrepeat = pTSprite->yrepeat>>2;
                pNSprite->picnum = pTSprite->picnum;
                pNSprite->pal = 5;
                int nTile = pNSprite->picnum;
                pNSprite->z -= (tilesizy[nTile]-(tilesizy[nTile]/2+picanm[nTile].yoffset)) * (pNSprite->yrepeat<<2);
                break;
            }
            case VIEW_EFFECT_1:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->z = pTSprite->z;
                pNSprite->cstat |= 2;
                pNSprite->shade = -128;
                pNSprite->pal = 2;
                pNSprite->xrepeat = pTSprite->xrepeat;
                pNSprite->yrepeat = pTSprite->yrepeat;
                pNSprite->picnum = 2427;
                break;
            }
            case VIEW_EFFECT_2:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                SECTOR *pSector = &sector[pTSprite->sectnum];
                int nShade = (pTSprite->z-pSector->ceilingz)>>8;
                pNSprite->x = pTSprite->x;
                pNSprite->y = pTSprite->y;
                pNSprite->z = pSector->ceilingz;
                pNSprite->cstat |= 106;
                pNSprite->shade = -64 + nShade;
                pNSprite->pal = 2;
                pNSprite->xrepeat = 64;
                pNSprite->yrepeat = 64;
                pNSprite->picnum = 624;
                pNSprite->ang = pTSprite->ang;
                pNSprite->owner = pTSprite->owner;
                break;
            }
            case VIEW_EFFECT_3:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                SECTOR *pSector = &sector[pTSprite->sectnum];
                int nShade = (pSector->floorz-pTSprite->z)>>8; 
                pNSprite->x = pTSprite->x;
                pNSprite->y = pTSprite->y;
                pNSprite->z = pSector->floorz;
                pNSprite->cstat |= 98;
                pNSprite->shade = -32 + nShade;
                pNSprite->pal = 2;
                pNSprite->xrepeat = nShade;
                pNSprite->yrepeat = nShade;
                pNSprite->picnum = 624;
                pNSprite->ang = pTSprite->ang;
                pNSprite->owner = pTSprite->owner;
                break;
            }
            case VIEW_EFFECT_9:
            {
                SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                pNSprite->z = pTSprite->z;
                if (gDetail > 1)
                    pNSprite->cstat |= 514;
                pNSprite->shade = ClipLow(pTSprite->shade-32, -128);
                pNSprite->xrepeat = pTSprite->xrepeat;
                pNSprite->yrepeat = 64;
                pNSprite->picnum = 775;
                break;
            }
            case VIEW_EFFECT_12:
            {
                dassert(pTSprite->type >= kDudePlayer1 && pTSprite->type <= kDudePlayer8, 2682+LDIFF4);
                PLAYER *pPlayer = &gPlayer[pTSprite->type-kDudePlayer1];
                if (gWeaponIcon[pPlayer->atbd].nTile >= 0)
                {
                    SPRITE *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
                    int top, bottom;
                    GetSpriteExtents(pTSprite, &top, &bottom);
                    pNSprite->x = pTSprite->x;
                    pNSprite->y = pTSprite->y;
                    pNSprite->z = pTSprite->z-(32<<8);
                    pNSprite->picnum = gWeaponIcon[pPlayer->atbd].nTile;
                    pNSprite->shade = pTSprite->shade;
                    pNSprite->xrepeat = gWeaponIcon[pPlayer->atbd].xRepeat;
                    pNSprite->yrepeat = gWeaponIcon[pPlayer->atbd].yRepeat;
                }
                break;
            }
        }
    }
    return NULL;
}

void viewProcessSprites(int cX, int cY, int cZ)
{
    int nTSprite, nOctant;
    int nXSprite;
    long dX, dY;
    dassert(spritesortcnt <= kMaxViewSprites, 2713+LDIFF4);
    int nViewSprites = spritesortcnt;
    for (nTSprite = nViewSprites-1; nTSprite >= 0; nTSprite--)
    {
        SPRITE *pTSprite = &tsprite[nTSprite];
        XSPRITE *pTXSprite = NULL;
        nXSprite = pTSprite->extra;
        if (sprite[pTSprite->owner].detail > gDetail)
        {
            pTSprite->xrepeat = 0;
            continue;
        }
        if (nXSprite > 0)
        {
            pTXSprite = &xsprite[nXSprite];
        }
        int nTile = pTSprite->picnum;
        if (nTile < 0 || nTile >= kMaxTiles)
        {
            continue;
        }

        int nSprite = pTSprite->owner;
        if (gViewInterpolate && TestBitString(gInterpolateSprite, nSprite) && !(pTSprite->flags&kSpriteFlag9))
        {
            LOCATION *pPrevLoc = &gPrevSpriteLoc[nSprite];
            pTSprite->x = interpolate16(pPrevLoc->x, pTSprite->x, gInterpolate);
            pTSprite->y = interpolate16(pPrevLoc->y, pTSprite->y, gInterpolate);
            pTSprite->z = interpolate16(pPrevLoc->z, pTSprite->z, gInterpolate);
            int delta = ((pTSprite->ang-pPrevLoc->ang+1024)&2047)-1024;
            pTSprite->ang = pPrevLoc->ang+mulscale16(delta, gInterpolate);
        }
        int nAnim = 0;
        switch (picanm[nTile].at3_4)
        {
            case 0:
                if (nXSprite > 0)
                {
                    dassert(nXSprite < kMaxXSprites, 2778+LDIFF4);
                    switch (pTSprite->type)
                    {
                    case 20:
                    case 21:
                        if (xsprite[nXSprite].at1_6)
                        {
                            nAnim = 1;
                        }
                        break;
                    case 22:
                        nAnim = xsprite[nXSprite].at10_0;
                        break;
                    }
                }
                break;
            case 1:
                dX = cX - pTSprite->x;
                dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, -pTSprite->ang+128);
                nOctant = GetOctant(dX, dY);
                if (nOctant <= 4)
                {
                    nAnim = nOctant;
                    pTSprite->cstat &= ~4;
                }
                else
                {
                    nAnim = 8 - nOctant;
                    pTSprite->cstat |= 4;
                }
                break;
            case 2:
                dX = cX - pTSprite->x;
                dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, -pTSprite->ang+128);
                nOctant = GetOctant(dX, dY);
                nAnim = nOctant;
                break;
            case 3:
                if (nXSprite > 0)
                {
                    if (gSpriteHit[nXSprite].florhit == 0)
                        nAnim = 1;
                }
                else
                {
                    int top, bottom;
                    GetSpriteExtents(pTSprite, &top, &bottom);
                    if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) > bottom)
                        nAnim = 1;
                }
                break;
            case 6:
            case 7:
#ifndef _3DFX
                if (gDetail >= 4 &&(pTSprite->flags&kSpriteFlag4) == 0)
                {
                    pTSprite->cstat |= 48;
                    pTSprite->picnum = voxelIndex[pTSprite->picnum];
                    if (picanm[nTile].at3_4 == 7)
                    {
                        pTSprite->ang = (gGameClock<<3)&2047;
                    }
                }
#endif
                break;
        }
        for (; nAnim > 0; nAnim--)
        {
            pTSprite->picnum += (short)(1+picanm[pTSprite->picnum].animframes);
        }
        int nShade = pTSprite->shade;
        SECTOR *pSector = &sector[pTSprite->sectnum];
#if 1
        XSECTOR *pXSector = pSector->extra ? &xsector[pSector->extra] : NULL;
#else
        XSECTOR* pXSector;
        if (pSector->extra)
        {
            pXSector = &xsector[pSector->extra];
        }
        else
        {
            pXSector = NULL;
        }
#endif
        if ((pSector->ceilingstat&kSectorStat0) && !(pSector->floorstat&kSectorStat31))
        {
            nShade += pSector->ceilingshade+tileShade[pSector->ceilingpicnum];
        }
        else
        {
            nShade += pSector->floorshade+tileShade[pSector->floorpicnum];
        }
        nShade += tileShade[pTSprite->picnum];
        pTSprite->shade = ClipRange(nShade, -128, 127);
        if ((pTSprite->flags&kSpriteFlag4) && sprite[pTSprite->owner].owner == 3)
        {
            dassert(pTXSprite != NULL, 2892+LDIFF5);
            pTSprite->picnum = 2272 + 2*pTXSprite->atb_4;
            pTSprite->xrepeat = 48;
            pTSprite->yrepeat = 48;
            pTSprite->shade = -128;
            pTSprite->cstat &= ~514;
            if (((IsItemSprite(pTSprite) || IsAmmoSprite(pTSprite)) && gGameOptions.nItemSettings == ITEMSETTINGS_2)
                || (IsWeaponSprite(pTSprite) && gGameOptions.nWeaponSettings == WEAPONSETTINGS_3))
            {
                pTSprite->xrepeat = pTSprite->yrepeat = 48;
            }
            else
            {
                pTSprite->xrepeat = pTSprite->yrepeat = 0;
            }
        }
        if (spritesortcnt < kMaxViewSprites)
        {
            if (pTXSprite && actGetBurnTime(pTXSprite) > 0)
            {
                pTSprite->shade = ClipRange(pTSprite->shade-16-QRandom(8), -128, 127);
            }
            if (pTSprite->flags&kSpriteFlag8)
            {
                viewAddEffect(nTSprite, VIEW_EFFECT_6);
            }
            if (pTSprite->flags&kSpriteFlag10)
            {
                pTSprite->cstat |= 4;
            }
            if (pTSprite->flags&kSpriteFlag11)
            {
                pTSprite->cstat |= 8;
            }
            switch (pTSprite->statnum)
            {
            case 0:
            {
                switch (pTSprite->type)
                {
                case 32:
                    if (pTXSprite == NULL || pTXSprite->at1_6 > 0)
                    {
                        pTSprite->shade = -128;
                        viewAddEffect(nTSprite, VIEW_EFFECT_11);
                    }
                    else
                    {
                        pTSprite->shade = -8;
                    }
                    break;
                case 30:
                    if (pTXSprite)
                    {
                        if (pTXSprite->at1_6 > 0)
                        {
                            pTSprite->picnum++;
                            viewAddEffect(nTSprite, VIEW_EFFECT_4);
                        }
                        else
                        {
                            viewAddEffect(nTSprite, VIEW_EFFECT_6);
                        }
                    }
                    else
                    {
                        pTSprite->picnum++;
                        viewAddEffect(nTSprite, VIEW_EFFECT_4);
                    }
                    break;
                default:
                    if (pXSector && pXSector->at18_0)
                    {
                        pTSprite->pal = pSector->floorpal;
                    }
                    break;
                }
                break;
            }
            case 3:
            {
                switch (pTSprite->type)
                {
                case 145:
                    if (pTXSprite && pTXSprite->at1_6 > 0 && gGameOptions.nGameType == GAMETYPE_3)
                    {
                        SPRITE *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_17);
                        if (pNTSprite)
                            pNTSprite->pal = 10;
                    }
                    break;
                case 146:
                    if (pTXSprite && pTXSprite->at1_6 > 0 && gGameOptions.nGameType == GAMETYPE_3)
                    {
                        SPRITE *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_17);
                        if (pNTSprite)
                            pNTSprite->pal = 7;
                    }
                    break;
                case 147:
                    pTSprite->cstat |= 4;
                    pTSprite->pal = 10;
                    break;
                case 148:
                    pTSprite->cstat |= 4;
                    pTSprite->pal = 7;
                    break;
                default:
                    if (pTSprite->type >= 100 && pTSprite->type <= 106)
                        pTSprite->shade = -128;
                    if (pXSector && pXSector->at18_0)
                    {
                        pTSprite->pal = pSector->floorpal;
                    }
                    break;
                }
                break;
            }
            case 5:
            {
                switch (pTSprite->type)
                {
                case 302:
                    pTSprite->cstat |= 32;
                    pTSprite->yrepeat = 128;
                    break;
                case 306:
                    viewAddEffect(nTSprite, VIEW_EFFECT_15);
                    break;
                case 300:
                    viewAddEffect(nTSprite, VIEW_EFFECT_10);
                    break;
                case 301:
                case 303:
                    if (pTSprite->statnum == 14)
                    {
                        dassert(pTXSprite != NULL, 3034+LDIFF5);
                        if (pTXSprite->target == gView->at5b)
                        {
                            pTSprite->xrepeat = 0;
                            continue;
                        }
                    }
                    viewAddEffect(nTSprite, VIEW_EFFECT_1);
                    if (pTSprite->type == 301)
                    {
                        SECTOR *pSector = &sector[pTSprite->sectnum];
                        int zDiff = (pTSprite->z-pSector->ceilingz)>>8;
                        if (!(pSector->ceilingstat&kSectorStat0) && zDiff < 64)
                        {
                            viewAddEffect(nTSprite, VIEW_EFFECT_2);
                        }
                        zDiff = (pSector->floorz-pTSprite->z)>>8;
                        if (!(pSector->floorstat&kSectorStat0) && zDiff < 64)
                        {
                            viewAddEffect(nTSprite, VIEW_EFFECT_3);
                        }
                    }
                    break;
                }
                break;
            }
            case 6:
            {
                XSPRITE* pXSprite2 = &xsprite[pTSprite->extra];
                if (pTSprite->type == 212 && pXSprite2->at34 == &hand13A3B4)
                {
                    SPRITE *pTTarget = &sprite[pTXSprite->target];
                    dassert(pTXSprite != NULL && pTTarget != NULL, 3065+LDIFF5);
                    if (pTTarget->type >= kDudePlayer1 && pTTarget->type <= kDudePlayer8)
                    {
                        pTSprite->xrepeat = 0;
                        continue;
                    }
                }
                if (pXSector && pXSector->at18_0)
                {
                    pTSprite->pal = pSector->floorpal;
                }
                if (powerupCheck(gView, 25) > 0)
                {
                    pTSprite->shade = -128;
                }
                if (pTSprite->type >= kDudePlayer1 && pTSprite->type <= kDudePlayer8)
                {
                    PLAYER *pPlayer = &gPlayer[pTSprite->type-kDudePlayer1];
                    if (powerupCheck(pPlayer, 13) && !powerupCheck(gView, 25))
                    {
                        pTSprite->cstat |= 2;
                        pTSprite->pal = 5;
                    }
                    else if (powerupCheck(pPlayer, 14))
                    {
                        pTSprite->shade = -128;
                        pTSprite->pal = 5;
                    }
                    else if (powerupCheck(pPlayer, 23))
                    {
                        pTSprite->pal = 11+(gView->at2ea&3);
                    }
                    if (powerupCheck(pPlayer, 24))
                    {
                        viewAddEffect(nTSprite, VIEW_EFFECT_13);
                    }
                    if (gShowWeapon && gGameOptions.nGameType > GAMETYPE_0 && gView != pPlayer)
                    {
                        viewAddEffect(nTSprite, VIEW_EFFECT_12);
                    }
                    if ((pPlayer->at37b & 1) && (gView != pPlayer || gViewPos != VIEWPOS_0))
                    {
                        SPRITE *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_14);
                        if (pNTSprite)
                        {
                            POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
                            pNTSprite->x += mulscale28(pPosture->at30, Cos(pTSprite->ang));
                            pNTSprite->y += mulscale28(pPosture->at30, Sin(pTSprite->ang));
                            pNTSprite->z = pPlayer->pSprite->z-pPosture->at2c;
                        }
                    }
                    if (pPlayer->at90 > 0 && gGameOptions.nGameType == GAMETYPE_3)
                    {
                        if (pPlayer->at90&1)
                        {
                            SPRITE *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_16);
                            if (pNTSprite)
                            {
                                pNTSprite->cstat |= 4;
                                pNTSprite->pal = 10;
                            }
                        }
                        if (pPlayer->at90&2)
                        {
                            SPRITE *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_16);
                            if (pNTSprite)
                            {
                                pNTSprite->cstat |= 4;
                                pNTSprite->pal = 7;
                            }
                        }
                    }
                }
                if (pTSprite->owner != gView->pSprite->index || gViewPos)
                {
                    if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) >= cZ)
                    {
                        viewAddEffect(nTSprite, VIEW_EFFECT_0);
                    }
                }
                break;
            }
            case 11:
            {
                switch (pTSprite->type)
                {
                case 454:
                    if (pTXSprite->at1_6)
                    {
                        if (pTXSprite->at10_0)
                        {
                            pTSprite->picnum = 772;
                            if (pTXSprite->at12_0)
                            {
                                viewAddEffect(nTSprite, VIEW_EFFECT_9);
                            }
                        }
                    }
                    else
                    {
                        if (pTXSprite->at10_0)
                        {
                            pTSprite->picnum = 773;
                        }
                        else
                        {
                            pTSprite->picnum = 656;
                        }
                    }
                    break;
                }
                break;
            }
            case 4:
            {
                if (pXSector && pXSector->at18_0)
                {
                    pTSprite->pal = pSector->floorpal;
                }
                if (pTSprite->flags&kSpriteFlag0)
                {
                    if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) >= cZ)
                    {
                        if (pTSprite->type >= 400 && pTSprite->type < 433)
                        {
                            if (gSpriteHit[nXSprite].florhit)
                                break;
                        }
                        viewAddEffect(nTSprite, VIEW_EFFECT_0);
                    }
                }
                break;
            }
            case 1:
                break;
            case 2:
                break;
            case 7:
                break;
            case 8:
                break;
            case 9:
                break;
            case 10:
                break;
            case 12:
                break;
            case 13:
                break;
            case 14:
                break;
            case 15:
                break;
            }
        }
    }

    for (nTSprite = spritesortcnt-1; nTSprite >= nViewSprites; nTSprite--)
    {
        int nAnim = 0;
        SPRITE *pTSprite = &tsprite[nTSprite];
        switch (picanm[pTSprite->picnum].at3_4)
        {
            case 1:
            {
                long dX = cX - pTSprite->x;
                long dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, -pTSprite->ang+128);
                int nOctant = GetOctant(dX, dY);
                if (nOctant <= 4)
                {
                    nAnim = nOctant;
                    pTSprite->cstat &= ~4;
                }
                else
                {
                    nAnim = 8 - nOctant;
                    pTSprite->cstat |= 4;
                }
                break;
            }
            case 2:
            {
                long dX = cX - pTSprite->x;
                long dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, -pTSprite->ang+128);
                int nOctant = GetOctant(dX, dY);
                nAnim = nOctant;
                break;
            }
        }
        for (;nAnim > 0; nAnim--)
        {
            pTSprite->picnum += (short)(1+picanm[pTSprite->picnum].animframes);
        }
    }
}

long othercameradist = 1280;
long cameradist = -1;
long othercameraclock;
long cameraclock;

static void CalcOtherPosition(SPRITE *pSprite, long *pX, long *pY, long *pZ, int *vsectnum, int nAng, int zm)
{
    int vX, vY, vZ, dX, dY, hX, hY, hZ;
    short nHSector, nHWall, nHSprite;
    vX = mulscale30(-Cos(nAng), 1280);
    vY = mulscale30(-Sin(nAng), 1280);
    vZ = mulscale(zm, 1280, 3);
    vZ -= (16<<8);
    short bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors, 3294+LDIFF5);
    FindSector(*pX, *pY, *pZ, vsectnum);
    hitscan(*pX, *pY, *pZ, *vsectnum, vX, vY, vZ, &nHSector, &nHWall, &nHSprite, &hX, &hY, &hZ, CLIPMASK1);
    dX = hX-*pX;
    dY = hY-*pY;
    if (klabs(vX)+klabs(vY) > klabs(dX)+klabs(dY))
    {
        *vsectnum = nHSector;
        dX -= ksgn(vX)<<6;
        dY -= ksgn(vY)<<6;
        if (klabs(vX) > klabs(vY))
            othercameradist = ClipHigh(othercameradist, divscale16(dX,vX));
        else
            othercameradist = ClipHigh(othercameradist, divscale16(dY,vY));
    }
    *pX += mulscale16(vX, othercameradist);
    *pY += mulscale16(vY, othercameradist);
    *pZ += mulscale16(vZ, othercameradist);
    othercameradist += (gGameClock-othercameraclock)<<10;
    if (othercameradist > 65536)
        othercameradist = 65536;
    othercameraclock = gGameClock;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors, 3322+LDIFF5);
    FindSector(*pX, *pY, *pZ, vsectnum);
    pSprite->cstat = bakCstat;
}

static void CalcPosition(SPRITE *pSprite, long *pX, long *pY, long *pZ, int *vsectnum, int nAng, int zm)
{
    int vX, vY, vZ, dX, dY, hX, hY, hZ;
    short nHSector, nHWall, nHSprite;
    vX = mulscale30(-Cos(nAng), 1280);
    vY = mulscale30(-Sin(nAng), 1280);
    vZ = mulscale(zm, 1280, 3);
    vZ -= (16<<8);
    short bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors, 3344+LDIFF5);
    FindSector(*pX, *pY, *pZ, vsectnum);
    hitscangoalx = hitscangoaly = 0x1fffffff;
    hitscan(*pX, *pY, *pZ, *vsectnum, vX, vY, vZ, &nHSector, &nHWall, &nHSprite, &hX, &hY, &hZ, CLIPMASK1);
    dX = hX-*pX;
    dY = hY-*pY;
    if (klabs(vX)+klabs(vY) > klabs(dX)+klabs(dY))
    {
        *vsectnum = nHSector;
        dX -= ksgn(vX)<<6;
        dY -= ksgn(vY)<<6;
        if (klabs(vX) > klabs(vY))
            cameradist = ClipHigh(cameradist, divscale16(dX,vX));
        else
            cameradist = ClipHigh(cameradist, divscale16(dY,vY));
    }
    *pX += mulscale16(vX, cameradist);
    *pY += mulscale16(vY, cameradist);
    *pZ += mulscale16(vZ, cameradist);
    cameradist += (gGameClock-cameraclock)<<10;
    if (cameradist > 65536)
        cameradist = 65536;
    cameraclock = gGameClock;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors, 3373+LDIFF5);
    FindSector(*pX, *pY, *pZ, vsectnum);
    pSprite->cstat = bakCstat;
}

void viewDrawSprite(long x, long y, long z, int a, int pn, schar shade, byte pal, ushort stat, long xd1, long yd1, long xd2, long yd2)
{
    if (stat & kRSStat8)
    {
        a = (a+1024)&2047;
        stat ^= kRSStat2;
    }
    rotatesprite(x, y, z, a, pn, shade, pal, stat, xd1, yd1, xd2, yd2);
}

struct {
    short nTile;
    byte nStat;
    char nPal;
    long nScale;
    short nX, nY;
} burnTable[9] = {
     { 2101, 2, 0, 118784, 10, 220 },
     { 2101, 2, 0, 110592, 40, 220 },
     { 2101, 2, 0, 81920, 85, 220 },
     { 2101, 2, 0, 69632, 120, 220 },
     { 2101, 2, 0, 61440, 160, 220 },
     { 2101, 2, 0, 73728, 200, 220 },
     { 2101, 2, 0, 77824, 235, 220 },
     { 2101, 2, 0, 110592, 275, 220 },
     { 2101, 2, 0, 122880, 310, 220 }
};

void viewBurnTime(int gScale)
{
    if (!gScale) return;

    for (int i = 0; i < 9; i++)
    {
        int nTile = burnTable[i].nTile;
        nTile += animateoffs(nTile, 32768 + i);
        int nScale = burnTable[i].nScale;
        if (gScale < 600)
        {
            nScale = kscale(nScale, gScale, 600);
        }
        viewDrawSprite(burnTable[i].nX<<16, burnTable[i].nY<<16, nScale, 0, nTile, 0,
            burnTable[i].nPal, burnTable[i].nStat, windowx1, windowy1, windowx2, windowy2);
    }
}

void viewSetMessage(char *pMessage)
{
    gGameMessageMgr.Add(pMessage, 15);
}

void viewDisplayMessage(void)
{
    gGameMessageMgr.Display();
}

void viewSetErrorMessage(char *pMessage)
{
    if (pMessage == NULL)
    {
        strcpy(errMsg, "");
        return;
    }
    strcpy(errMsg, pMessage);
}

static void DoLensEffect(void)
{
    byte *d = (byte*)waloff[4077];
    dassert(d != NULL, 3480+LDIFF6);
    byte *s = (byte*)waloff[4079];
    dassert(s != NULL, 3482+LDIFF6);
    for (int i = 0; i < kLensSize*kLensSize; i++, d++)
    {
        if (lensTable[i] >= 0)
        {
            *d = *(s+lensTable[i]);
        }
    }
}

extern "C" void convertAndDownloadPalette(void* dlpalette);

static void UpdateDacs(int nPalette)
{
    static RGB newDAC[256];
    static int oldPalette;
    if (nPalette != oldPalette)
    {
        scrSetPalette(nPalette);
        oldPalette = nPalette;
    }

    for (int i = 0; i < 256; i++)
    {
        int nRed = baseDAC[i].red;
        int nGreen = baseDAC[i].green;
        int nBlue = baseDAC[i].blue;
        nRed += gView->at377;
        nGreen += gView->at377;
        nBlue -= gView->at377;

        nRed += ClipHigh(gView->at366, 85)*2;
        nGreen -= ClipHigh(gView->at366, 85)*3;
        nBlue -= ClipHigh(gView->at366, 85)*3;

        nRed -= gView->at36a;
        nGreen -= gView->at36a;
        nBlue -= gView->at36a;

        nRed -= gView->at36e>>6;
        nGreen -= gView->at36e>>5;
        nBlue -= gView->at36e>>6;

        newDAC[i].red = ClipRange(nRed, 0, 255);
        newDAC[i].green = ClipRange(nGreen, 0, 255);
        newDAC[i].blue = ClipRange(nBlue, 0, 255);
    }
    if (memcmp(newDAC, curDAC, 768) != 0)
    {
        memcpy(curDAC, newDAC, 768);
        gSetDACRange(0, 256, (byte*)curDAC);
    }
#ifdef _3DFX
    convertAndDownloadPalette(baseDAC);
#endif
}

#define TILTBUFFER 4078

void viewDrawScreen(void)
{
    static int lastUpdate;
    int arg = 0;
    int delta = ClipLow(gGameClock - lastUpdate, 0);
    lastUpdate = gGameClock;
    if (!gPaused && (!CGameMenuMgr::Active() || gGameOptions.nGameType != GAMETYPE_0))
    {
        gInterpolate = divscale16(gGameClock-gNetFifoClock+4, 4);
    }
    if (gInterpolate < 0 || gInterpolate > 65536)
    {
        gInterpolate = ClipRange(gInterpolate, 0, 65536);
    }
    if (gViewInterpolate)
    {
        CalcInterpolations();
    }

    if (gViewMode == 3 || gViewMode == 4 || gOverlayMap)
    {
        DoSectorLighting();
    }
    if (gViewMode == 3 || gOverlayMap)
    {
        long cX = gView->pSprite->x;
        long cY = gView->pSprite->y;
        long cZ = gView->at67;
        int zDelta = gView->at6f-gView->at67-(12<<8);
        int cA = gView->pSprite->ang;
        int va0 = gView->at7b;
        int v90 = gView->at7f;
        int v74 = gView->at43;
        int v8c = gView->at3f;
        int v4c = gView->at53;
        int v48 = gView->at4f;
        int nSectnum = gView->pSprite->sectnum;
        if (gViewInterpolate)
        {
            if (numplayers > 1 && gView == gMe && gPrediction && gMe->pXSprite->health > 0)
            {
                nSectnum = predict.at68;
                cX = interpolate16(predictOld.at50, predict.at50, gInterpolate);
                cY = interpolate16(predictOld.at54, predict.at54, gInterpolate);
                cZ = interpolate16(predictOld.at38, predict.at38, gInterpolate);
                zDelta = interpolate16(predictOld.at34, predict.at34, gInterpolate);
                int delta = ((predict.at30-predictOld.at30+1024)&2047)-1024;
                cA = predictOld.at30 + mulscale16(delta, gInterpolate);
                va0 = interpolate16(predictOld.at24, predict.at24, gInterpolate);
                v90 = interpolate16(predictOld.at28, predict.at28, gInterpolate);
                v74 = interpolate16(predictOld.atc, predict.atc, gInterpolate);
                v8c = interpolate16(predictOld.at8, predict.at8, gInterpolate);
                v4c = interpolate16(predictOld.at1c, predict.at1c, gInterpolate);
                v48 = interpolate16(predictOld.at18, predict.at18, gInterpolate);
            }
            else
            {
                VIEW *pView = &gPrevView[gViewIndex];
                cX = interpolate16(pView->at50, cX, gInterpolate);
                cY = interpolate16(pView->at54, cY, gInterpolate);
                cZ = interpolate16(pView->at38, cZ, gInterpolate);
                zDelta = interpolate16(pView->at34, zDelta, gInterpolate);
                int delta = ((cA-pView->at30+1024)&2047)-1024;
                cA =  pView->at30 + mulscale16(delta, gInterpolate);
                va0 = interpolate16(pView->at24, va0, gInterpolate);
                v90 = interpolate16(pView->at28, v90, gInterpolate);
                v74 = interpolate16(pView->atc, v74, gInterpolate);
                v8c = interpolate16(pView->at8, v8c, gInterpolate);
                v4c = interpolate16(pView->at1c, v4c, gInterpolate);
                v48 = interpolate16(pView->at18, v48, gInterpolate);
            }
        }

        if (gView->at35a)
        {
            int nValue = ClipHigh(gView->at35a*8, 2000);
            va0 += QRandom2(nValue>>8);
            cA += QRandom2(nValue>>8);
            cX += QRandom2(nValue>>4);
            cY += QRandom2(nValue>>4);
            cZ += QRandom2(nValue);
            v4c += QRandom2(nValue);
            v48 += QRandom2(nValue);
        }

        if (gView->at37f)
        {
            int nValue = ClipHigh(gView->at37f*8, 2000);
            va0 += QRandom2(nValue>>8);
            cA += QRandom2(nValue>>8);
            cX += QRandom2(nValue>>4);
            cY += QRandom2(nValue>>4);
            cZ += QRandom2(nValue);
            v4c += QRandom2(nValue);
            v48 += QRandom2(nValue);
        }
        va0 += mulscale30(0x40000000-Cos(gView->at35e<<2), 30);
        if (gViewPos == VIEWPOS_0)
        {
            if (gViewHBobbing)
            {
                cX -= mulscale30(v74, Sin(cA))>>4;
                cY += mulscale30(v74, Cos(cA))>>4;
            }
            if (gViewVBobbing)
            {
                cZ += v8c;
            }
            if (gSlopeTilting)
            {
                va0 += v90;
            }
            cZ += va0*10;
            cameradist = -1;
            cameraclock = gGameClock;
        }
        else
        {
            CalcPosition(gView->pSprite, &cX, &cY, &cZ, &nSectnum, cA, va0);
        }
        CheckLink(&cX, &cY, &cZ, &nSectnum);
        int v78 = gScreenTilt;
        BOOL vc = 0;
        BOOL v4 = 0;
        byte v14 = 0;
        schar v10 = 0;
        vc = powerupCheck(gView, 28) > 0;
        v4 = powerupCheck(gView, 21) > 0;
        if (v78 || vc)
        {
            if (!waloff[4078])
            {
                tileAllocTile(4078, 320, 320);
            }
            setviewtotile(4078, 320, 320);
            int nAng = v78&511;
            if (nAng > 256)
            {
                nAng = 512-nAng;
            }
            nAng = dmulscale32(Cos(nAng), 256000, Sin(nAng), 160000);
            setaspect(nAng, yxaspect);
        }
        else
        {
            if (v4 && gNetPlayers > 1)
            {
                int tmp = (gGameClock/240)%(gNetPlayers-1);
                int i = connecthead;
                while (1)
                {
                    if (i == gViewIndex)
                        i = connectpoint2[i];
                    if (tmp == 0)
                        break;
                    i = connectpoint2[i];
                    tmp--;
                }
                PLAYER *pOther = &gPlayer[i];
                //othercameraclock = gGameClock;
                if (!waloff[4079])
                {
                    tileAllocTile(4079, 128, 128);
                }
                setviewtotile(4079, 128, 128);
                setaspect(65536, 78643);
                long vd8 = pOther->pSprite->x;
                long vd4 = pOther->pSprite->y;
                long vd0 = pOther->at67;
                int vcc = pOther->pSprite->sectnum;
                int v50 = pOther->pSprite->ang;
                int v54 = 0;
                if (pOther->at35a)
                {
                    int nValue = ClipHigh(pOther->at35a*8, 2000);
                    v54 += QRandom2(nValue>>8);
                    v50 += QRandom2(nValue>>8);
                    vd8 += QRandom2(nValue>>4);
                    vd4 += QRandom2(nValue>>4);
                    vd0 += QRandom2(nValue);
                }
                if (pOther->at37f)
                {
                    int nValue = ClipHigh(pOther->at37f*8, 2000);
                    v54 += QRandom2(nValue>>8);
                    v50 += QRandom2(nValue>>8);
                    vd8 += QRandom2(nValue>>4);
                    vd4 += QRandom2(nValue>>4);
                    vd0 += QRandom2(nValue);
                }
                CalcOtherPosition(pOther->pSprite, &vd8, &vd4, &vd0, &vcc, v50, 0);
                CheckLink(&vd8, &vd4, &vd0, &vcc);
                if (IsUnderwaterSector(vcc))
                {
                    v14 = 10;
                }
                memcpy(bakMirrorGotpic, gotpic+510, 2);
                memcpy(gotpic+510, otherMirrorGotpic, 2);
                visibility = ClipLow(gVisibility-pOther->at362*32, 0);
#if APPVER_BLOODREV < AV_BR_BL111
                v50 = (v50 + 1024) & 2047;
#endif
                int vc8, vc4;
                getzsofslope(vcc, vd8, vd4, &vc8, &vc4);
                if (vd0 >= vc4)
                {
                    vd0 = vc4-(8<<4);
                }
                if (vd0 <= vc8)
                {
                    vd0 = vc8+(8<<4);
                }
                v54 = ClipRange(v54, -200, 200);
                DrawMirrors(vd8, vd4, vd0, v50, 90 + v54);
                drawrooms(vd8, vd4, vd0, v50, 90 + v54, vcc);
                memcpy(otherMirrorGotpic, gotpic+510, 2);
                memcpy(gotpic+510, bakMirrorGotpic, 2);
                viewProcessSprites(vd8, vd4, vd0);
                drawmasks();
                setviewback();
            }
            else
            {
                othercameraclock = gGameClock;
            }
        }

        if (!vc)
        {
            deliriumTilt = 0;
            deliriumTurn = 0;
            deliriumPitch = 0;
        }
        int unk = 0;

        for (int nSprite = headspritestat[2]; nSprite >= 0; nSprite = nextspritestat[nSprite])
        {
            SPRITE *pSprite = &sprite[nSprite];
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites, 3812+LDIFF7);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (TestBitString(gotsector, pSprite->sectnum))
            {
                unk += pXSprite->at14_0*32;
            }
        }

        for (nSprite = headspritestat[5]; nSprite >= 0; nSprite = nextspritestat[nSprite])
        {
            SPRITE *pSprite = &sprite[nSprite];
            switch (pSprite->type)
            {
            case kMissile303:
            case kMissile306:
            case kMissile302:
            case kMissile301:
                if (TestBitString(gotsector, pSprite->sectnum))
                {
                    unk += 256;
                }
                break;
            }
        }
        visibility = ClipLow(gVisibility - gView->at362 * 32 - unk, 0);
        cA = (cA + deliriumTurn) & 2047;
        int vfc, vf8;
        getzsofslope(nSectnum, cX, cY, &vfc, &vf8);
        if (cZ >= vf8)
        {
            cZ = vf8-(8<<4);
        }
        if (cZ <= vfc)
        {
            cZ = vfc+(8<<4);
        }
        va0 = ClipRange(va0, -200, 200);
        DrawMirrors(cX, cY, cZ, cA, 90 + va0 + deliriumPitch);
        ushort bakCstat = gView->pSprite->cstat;
        if (gViewPos == VIEWPOS_0)
        {
            gView->pSprite->cstat |= 32768;
        }
        else
        {
            gView->pSprite->cstat |= 514;
        }
        drawrooms(cX, cY, cZ, cA, 90 + va0 + deliriumPitch, nSectnum);
        viewProcessSprites(cX, cY, cZ);
        func_5571C(TRUE);
        drawmasks();
        func_5571C(FALSE);
        func_557C4(cX, cY, gInterpolate);
        drawmasks();
        gView->pSprite->cstat = bakCstat;
        if (v78 || vc)
        {
            dassert(waloff[ TILTBUFFER ] != NULL, 3874+LDIFF7);
            setviewback();
            byte vrc = 70;
            if (vc)
            {
                vrc |= 33;
            }
            int nAng = v78 & 511;
            if (nAng > 256)
            {
                nAng = 512 - nAng;
            }
            nAng = dmulscale32(Cos(nAng), 256000, Sin(nAng), 160000);
            rotatesprite(160<<16, 100<<16, nAng, v78+512, TILTBUFFER, 0, 0, vrc, gViewX0, gViewY0, gViewX1, gViewY1);
        }
        long vf4, vf0, vec, ve8;
        GetZRange(gView->pSprite, &vf4, &vf0, &vec, &ve8, gView->pSprite->clipdist<<2, 0);
        int tmpSect = nSectnum;
        if ((vf0 & 0xe000) == 0x4000)
        {
            tmpSect = vf0 & 0x1fff;
        }
        BOOL v8 = (gWeatherType > 0 && (sector[tmpSect].ceilingstat&kSectorStat0)) ? TRUE : FALSE;
        if (gWeather.GetCount() > 0 || v8)
        {
            short vsi = gWeather.GetCount();
            gWeather.Draw(cX, cY, cZ, cA, 90 + va0 + deliriumPitch, vsi);
            if (v8)
            {
                gWeather.SetCount(vsi+delta*8);
            }
            else
            {
                gWeather.SetCount(vsi-delta*64);
            }
        }
        if (gViewPos == VIEWPOS_0)
        {
            if (gAimReticle)
            {
                cX = 160;
                cY = 90;
                rotatesprite(cX<<16, cY<<16, 65536, 0, 2319, 0, 0, 2, gViewX0, gViewY0, gViewX1, gViewY1);
            }
            cX = 160+(v4c>>8);
            cY = 220+(v48>>8)+(zDelta>>7);
            int nShade = sector[nSectnum].floorshade;
            int nPalette = 0;
            if (sector[gView->pSprite->sectnum].extra > 0)
            {
                SECTOR *pSector = &sector[gView->pSprite->sectnum];
                XSECTOR *pXSector = &xsector[pSector->extra];
                if (pXSector->at18_0)
                {
                    nPalette = pSector->floorpal;
                }
            }
            WeaponDraw(gView, nShade, cX, cY, nPalette);
        }
        if (gViewPos == VIEWPOS_0 && actGetBurnTime(gView->pXSprite) > 60)
        {
            viewBurnTime(actGetBurnTime(gView->pXSprite));
        }
        if (packItemActive(gView, 1))
        {
            byte flags = 18;
            rotatesprite(0, 0, 65536, 0, 2344, 0, 0, flags, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(320<<16, 0, 65536, 1024, 2344, 0, 0, flags|4, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(0, 200<<16, 65536, 0, 2344, 0, 0, flags|4, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(320<<16, 200<<16, 65536, 1024, 2344, 0, 0, flags, gViewX0, gViewY0, gViewX1, gViewY1);
            if (gDetail >= 4)
            {
                rotatesprite(15<<16, 3<<16, 65536, 0, 2346, 32, 0, flags|1, gViewX0, gViewY0, gViewX1, gViewY1);
                rotatesprite(212<<16, 77<<16, 65536, 0, 2347, 32, 0, flags|1, gViewX0, gViewY0, gViewX1, gViewY1);
            }
        }
        if (powerupCheck(gView, 39) > 0)
        {
            byte flags = 18;
            rotatesprite(0, 200<<16, 65536, 0, 2358, 0, 0, flags|4, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(320<<16, 200<<16, 65536, 1024, 2358, 0, 0, flags, gViewX0, gViewY0, gViewX1, gViewY1);
        }
        if (v4 && gNetPlayers > 1)
        {
            DoLensEffect();
            setaspect(65536, 54613);
            rotatesprite(280<<16, 35<<16, 53248, 512, 4077, v10, v14, 6, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(280<<16, 35<<16, 53248, 0, 1683, v10, 0, 35, gViewX0, gViewY0, gViewX1, gViewY1);
            setaspect(65536, divscale16(ydim*320, xdim*200));
        }
        if (powerupCheck(gView, 14) > 0)
        {
            arg = 4;
        }
        else if(powerupCheck(gView, 24) > 0)
        {
            arg = 1;
        }
        else
        {
            if (gView->at87)
            {
                if (gView->pXSprite->at17_6 == 1)
                {
                    arg = 1;
                }
                else if (gView->pXSprite->at17_6 == 2)
                {
                    arg = 3;
                }
            }
            else if (gView->at5f == 1)
                arg = 2;
        }
    }
#if APPVER_BLOODREV < AV_BR_BL111A
    else
        clearview(0);
#endif

    if (gViewMode == 4)
    {
        gViewMap.func_25DB0(gView->pSprite);
#if APPVER_BLOODREV < AV_BR_BL111A
        char *pTitle = levelGetTitle();
        if (pTitle)
        {
            sprintf(buffer, "%s: %s",
                levelGetFilename(gGameOptions.nEpisode, gGameOptions.nLevel), pTitle);
        }
        else
        {
            sprintf(buffer, "%s", levelGetFilename(gGameOptions.nEpisode, gGameOptions.nLevel));
        }
        viewDrawText(3, buffer, gViewX1, gViewY0+1, -128, 0, 2, 0);

        char *pMsg = gViewMap.Mode() ? "MAP FOLLOW MODE" : "MAP SCROLL MODE";
        viewDrawText(3, pMsg, gViewX1, gViewY0+10, -128, 0, 2, 0);
#endif
    }
    viewDrawInterface(delta);
    int zDelta = gView->at6f-gView->at67-(12<<8);
    int cx = 160;
    int cy = 220+(zDelta>>7);
    PLAYER *pPSprite = &gPlayer[gMe->pSprite->type-kDudePlayer1];
    if (pPSprite->at376 == 1)
    {
        gChoke.func_84110(cx, cy);
        if ((gGameClock % 5) == 0)
        {
            gChoke.f_1c(&gChoke, pPSprite);
        }
    }
    if (char_1A76C6)
    {
        DrawStatSprite(2048, xdim-15, 20);
    }
    viewDisplayMessage();
    CalcFrameRate();
    if (gShowFrameRate)
    {
#if APPVER_BLOODREV >= AV_BR_BL111A
        int fX = gViewMode == 3 ? gViewX1 : xdim;
        int fY = gViewMode == 3 ? gViewY0 : 0;
        if (gViewMode == 4)
        {
            fY += mulscale16(20, yscale);
        }
        sprintf(buffer, "%3i", gFrameRate);
        printext256(fX-12, fY, 31, -1, buffer, 1);
        fY += 8;
        sprintf(buffer, "pos=%d,%d,%d", gView->pSprite->x, gView->pSprite->y, gView->pSprite->z);
        printext256(fX-strlen(buffer)*4, fY, 31, -1, buffer, 1);
#else
        char buffer[16];
        sprintf(buffer, "%3i", gFrameRate);
        printext256(gViewX1-12, gViewY0, 31, -1, buffer, 1);
        sprintf(buffer, "pos=%d,%d,%d", gView->pSprite->x, gView->pSprite->y, gView->pSprite->z);
        printext256(gViewX1-100, gViewY0+10, 31, -1, buffer, 1);
        if (gGameOptions.nGameType > GAMETYPE_0)
        {
            extern int gNetSentSizeThreshold;
            sprintf(buffer, "%d", gNetSentSizeThreshold);
            printext256(gViewX1 - 64, gViewY0, 31, -1, buffer, 1);
            if (gPacketMode == PACKETMODE_2)
                printext256(gViewX1 - 24, gViewY0, 31, -1, "B", 1);
            else if (myconnectindex == connecthead)
                printext256(gViewX1 - 24, gViewY0, 31, -1, "M", 1);
            else
                printext256(gViewX1 - 24, gViewY0, 31, -1, "S", 1);
        }
#endif
    }
    if (gPaused)
    {
        viewDrawText(1, "PAUSED", 160, 10, 0, 0, 1, 0);
    }
    else if (gView != gMe)
    {
        sprintf(buffer, "] %s [", gProfile[gView->at57].name);
        viewDrawText(0, buffer, 160, 10, 0, 0, 1, 0);
    }
    if (errMsg[0])
    {
        viewDrawText(0, errMsg, 160, 20, 0, 0, 1, 0);
    }
    if (gViewInterpolate)
    {
        RestoreInterpolations();
    }
    UpdateDacs(arg);
}

void func_1EC78(int nTile, char *pText, char *pText2, char *pText3)
{
    int vc;
    int v8 = -128;
    gMenuTextMgr.GetFontInfo(1, NULL, NULL, &vc);
    if (nTile)
    {
        clearview(0);
        rotatesprite(160<<16, 100<<16, 65536, 0, nTile, 0, 0, 74, 0, 0, xdim-1, ydim-1);
    }
    if (pText)
    {
        rotatesprite(160<<16, 20<<16, 65536, 0, 2038, -128, 0,
#if APPVER_BLOODREV >= AV_BR_BL111A
            78,
#else
            6,
#endif
            0, 0, xdim-1, ydim-1);
        viewDrawText(1, pText, 160, 20-vc/2, -128, 0, 1, 1);
    }
    if (pText2)
    {
        viewDrawText(1, pText2, 160, 50, v8, 0, 1, 1);
    }
    if (pText3)
    {
        viewDrawText(1, pText3, 160, 70, v8, 0, 1, 1);
    }
    viewDrawText(3, "Please Wait", 160, 134, -128, 0, 1, 1);
    scrNextPage();
}

class ViewLoadSave : public LoadSave {
public:
    void Load(void);
    void Save(void);
};

void ViewLoadSave::Load(void)
{
    Read(&messageTime, sizeof(messageTime));
    Read(message, sizeof(message));
    Read(otherMirrorGotpic, sizeof(otherMirrorGotpic));
    Read(bakMirrorGotpic, sizeof(bakMirrorGotpic));
    Read(&gScreenTilt, sizeof(gScreenTilt));
    Read(&deliriumTilt, sizeof(deliriumTilt));
    Read(&deliriumTurn, sizeof(deliriumTurn));
    Read(&deliriumPitch, sizeof(deliriumPitch));
}

void ViewLoadSave::Save(void)
{
    Write(&messageTime, sizeof(messageTime));
    Write(message, sizeof(message));
    Write(otherMirrorGotpic, sizeof(otherMirrorGotpic));
    Write(bakMirrorGotpic, sizeof(bakMirrorGotpic));
    Write(&gScreenTilt, sizeof(gScreenTilt));
    Write(&deliriumTilt, sizeof(deliriumTilt));
    Write(&deliriumTurn, sizeof(deliriumTurn));
    Write(&deliriumPitch, sizeof(deliriumPitch));
}

static ViewLoadSave myLoadSave;
