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
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include "typedefs.h"
#include "globals.h"
#include "build.h"
#include "crc32.h"
#include "db.h"
#include "debug4g.h"
#include "error.h"
#include "globals.h"
#include "iob.h"
#include "misc.h"
#include "resource.h"

#if APPVER_BLOODREV >= AV_BR_BL120
#define LDIFF1 0
#else
#define LDIFF1 -5
#endif

XSPRITE xsprite[kMaxXSprites];
XWALL xwall[kMaxXWalls];
XSECTOR xsector[kMaxXSectors];

ushort nextXSprite[kMaxXSprites];
ushort nextXWall[kMaxXWalls];
ushort nextXSector[kMaxXSectors];

int gMapRev;
int gSongId;
int gSkyCount;
long gVisibility;

#ifdef REGISTERED
MAPHEADER2 char_19AE44;
#endif

long xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];

ushort gStatCount[kMaxStatus + 1];

char *gItemText[] = {
    "Skull Key",
    "Eye Key",
    "Fire Key",
    "Dagger Key",
    "Spider Key",
    "Moon Key",
    "Key 7",
    "Doctor's Bag",
    "Medicine Pouch",
    "Life Essence",
    "Life Seed",
    "Red Potion",
    "Feather Fall",
    "Limited Invisibility",
    "INVULNERABILITY",
    "Boots of Jumping",
    "Raven Flight",
    "Guns Akimbo",
    "Diving Suit",
    "Gas mask",
    "Clone",
    "Crystal Ball",
    "Decoy",
    "Doppleganger",
    "Reflective shots",
    "Beast Vision",
    "ShadowCloak",
    "Rage shroom",
    "Delirium Shroom",
    "Grow shroom",
    "Shrink shroom",
    "Death mask",
    "Wine Goblet",
    "Wine Bottle",
    "Skull Grail",
    "Silver Grail",
    "Tome",
    "Black Chest",
    "Wooden Chest",
    "Asbestos Armor",
    "Basic Armor",
    "Body Armor",
    "Fire Armor",
    "Spirit Armor",
    "Super Armor",
    "Blue Team Base",
    "Red Team Base",
    "Blue Flag",
    "Red Flag",
};

char *gAmmoText[] = {
    "Spray can",
    "Bundle of TNT*",
    "Bundle of TNT",
    "Case of TNT",
    "Proximity Detonator",
    "Remote Detonator",
    "Trapped Soul",
    "4 shotgun shells",
    "Box of shotgun shells",
    "A few bullets",
    "Voodoo Doll",
    "OBSOLETE",
    "Full drum of bullets",
    "Tesla Charge",
    "OBSOLETE",
    "OBSOLETE",
    "Flares",
    "OBSOLETE",
    "OBSOLETE",
    "Gasoline Can",
    NULL,
};

char *gWeaponText[] = {
    "RANDOM",
    "Sawed-off",
    "Tommy Gun",
    "Flare Pistol",
    "Voodoo Doll",
    "Tesla Cannon",
    "Napalm Launcher",
    "Pitchfork",
    "Spray Can",
    "Dynamite",
    "Life Leech",
};

void dbCrypt(byte *pPtr, int nLength, int nKey)
{
    int pos = nKey;
    for (int i = 0; i < nLength; i++)
    {
        pPtr[i] ^= pos;
        pos++;
    }
}

void InsertSpriteSect(int nSprite, int nSector)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 498);
    dassert(nSector >= 0 && nSector < kMaxSectors, 499);
    int nOther = headspritesect[nSector];
    if (nOther >= 0)
    {
        prevspritesect[nSprite] = prevspritesect[nOther];
        nextspritesect[nSprite] = -1;
        nextspritesect[prevspritesect[nOther]] = nSprite;
        prevspritesect[nOther] = nSprite;
    }
    else
    {
        prevspritesect[nSprite] = nSprite;
        nextspritesect[nSprite] = -1;
        headspritesect[nSector] = nSprite;
    }
    sprite[nSprite].sectnum = nSector;
}

void RemoveSpriteSect(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 523);
    int nSector = sprite[nSprite].sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors, 526);
    if (nextspritesect[nSprite] < 0)
    {
        prevspritesect[headspritesect[nSector]] = prevspritesect[nSprite];
    }
    else
    {
        prevspritesect[nextspritesect[nSprite]] = prevspritesect[nSprite];
    }
    if (headspritesect[nSector] != nSprite)
    {
        nextspritesect[prevspritesect[nSprite]] = nextspritesect[nSprite];
    }
    else
    {
        headspritesect[nSector] = nextspritesect[nSprite];
    }
    sprite[nSprite].sectnum = -1;
}

void InsertSpriteStat(int nSprite, int nStat)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 544);
    dassert(nStat >= 0 && nStat <= kMaxStatus, 545);
    int nOther = headspritestat[nStat];
    if (nOther >= 0)
    {
        prevspritestat[nSprite] = prevspritestat[nOther];
        nextspritestat[nSprite] = -1;
        nextspritestat[prevspritestat[nOther]] = nSprite;
        prevspritestat[nOther] = nSprite;
    }
    else
    {
        prevspritestat[nSprite] = nSprite;
        nextspritestat[nSprite] = -1;
        headspritestat[nStat] = nSprite;
    }
    sprite[nSprite].statnum = nStat;
    gStatCount[nStat]++;
}

void RemoveSpriteStat(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 569);
    int nStat = sprite[nSprite].statnum;
    dassert(nStat >= 0 && nStat <= kMaxStatus, 571);
    if (nextspritestat[nSprite] < 0)
    {
        prevspritestat[headspritestat[nStat]] = prevspritestat[nSprite];
    }
    else
    {
        prevspritestat[nextspritestat[nSprite]] = prevspritestat[nSprite];
    }
    if (headspritestat[nStat] != nSprite)
    {
        nextspritestat[prevspritestat[nSprite]] = nextspritestat[nSprite];
    }
    else
    {
        headspritestat[nStat] = nextspritestat[nSprite];
    }
    sprite[nSprite].statnum = -1;
    gStatCount[nStat]--;
}

void initspritelists(void)
{
    for (short i = 0; i <= kMaxSectors; i++)
    {
        headspritesect[i] = -1;
    }
    for (i = 0; i <= kMaxStatus; i++)
    {
        headspritestat[i] = -1;
    }
    for (i = 0; i < kMaxSprites; i++)
    {
        sprite[i].sectnum = -1;
        sprite[i].index = -1;
        InsertSpriteStat(i, kMaxStatus);
    }
    memset(gStatCount, 0, sizeof(gStatCount));
}

int insertsprite(short a1, short a2)
{
    return InsertSprite(a1, a2);
}

int InsertSprite(int nSector, int nStat)
{
    int nSprite = headspritestat[kMaxStatus];
    dassert(nSprite < kMaxSprites, 620);
    if (nSprite < 0)
    {
        return nSprite;
    }
    RemoveSpriteStat(nSprite);
    SPRITE *pSprite = &sprite[nSprite];
    memset(&sprite[nSprite], 0, sizeof(SPRITE));
    InsertSpriteStat(nSprite, nStat);
    InsertSpriteSect(nSprite, nSector);
    pSprite->cstat = 128;
    pSprite->clipdist = 32;
    pSprite->xrepeat = pSprite->yrepeat = 64;
    pSprite->owner = -1;
    pSprite->extra = -1;
    pSprite->index = nSprite;
    xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;

    return nSprite;
}

void deletesprite(short a1)
{
    DeleteSprite(a1);
}

void DeleteSprite(int nSprite)
{
    if (sprite[nSprite].extra > 0)
    {
        dbDeleteXSprite(sprite[nSprite].extra);
    }
    dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus, 667+LDIFF1);
    RemoveSpriteStat(nSprite);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors, 670+LDIFF1);
    RemoveSpriteSect(nSprite);
    InsertSpriteStat(nSprite, kMaxStatus);
}

int changespritesect(short nSprite, short nSector)
{
    return ChangeSpriteSect(nSprite, nSector);
}

int ChangeSpriteSect(int nSprite, int nSector)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites,679+LDIFF1);
    dassert(nSector >= 0 && nSector < kMaxSectors,680+LDIFF1);
    dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus, 682+LDIFF1);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors, 683+LDIFF1);
    RemoveSpriteSect(nSprite);
    InsertSpriteSect(nSprite, nSector);
    return 0;
}

int changespritestat(short nSprite, short nStatus)
{
    return ChangeSpriteStat(nSprite, nStatus);
}

int ChangeSpriteStat(int nSprite, int nStatus)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 694+LDIFF1);
    dassert(nStatus >= 0 && nStatus < kMaxStatus, 695+LDIFF1);
    dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus, 696+LDIFF1);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors, 697+LDIFF1);
    RemoveSpriteStat(nSprite);
    InsertSpriteStat(nSprite, nStatus);
    return 0;
}

void InitFreeList(ushort *pList, int nCount)
{
    for (int i = 1; i < nCount; i++)
    {
        pList[i] = i-1;
    }
    pList[0] = nCount - 1;
}

static inline void InsertFree(ushort *pList, int nIndex)
{
    pList[nIndex] = pList[0];
    pList[0] = nIndex;
}

static inline ushort GetFree(ushort *pList)
{
    ushort ret = pList[0];
    pList[0] = pList[ret];
    return ret;
}

ushort dbInsertXSprite(int nSprite)
{
    ushort nXSprite = GetFree(nextXSprite);
    if (nXSprite == 0)
    {
        ThrowError(756+LDIFF1)("Out of free XSprites");
    }
    memset(&xsprite[nXSprite], 0, sizeof(XSPRITE));
    sprite[nSprite].extra = nXSprite;
    xsprite[nXSprite].reference = nSprite;
    return nXSprite;
}

void dbDeleteXSprite(int nXSprite)
{
    dassert(xsprite[nXSprite].reference >= 0, 768+LDIFF1);
    dassert(sprite[xsprite[nXSprite].reference].extra == nXSprite, 769+LDIFF1);
    InsertFree(nextXSprite, nXSprite);
    sprite[xsprite[nXSprite].reference].extra = -1;
    xsprite[nXSprite].reference = -1;
}

ushort dbInsertXWall(int nWall)
{
    ushort nXWall = GetFree(nextXWall);
    if (nXWall == 0)
    {
        ThrowError(782+LDIFF1)("Out of free XWalls");
    }
    memset(&xwall[nXWall], 0, sizeof(XWALL));
    wall[nWall].extra = nXWall;
    xwall[nXWall].reference = nWall;
    return nXWall;
}

void dbDeleteXWall(int nXWall)
{
    dassert(xwall[nXWall].reference >= 0, 794+LDIFF1);
    InsertFree(nextXWall, nXWall);
    wall[xwall[nXWall].reference].extra = -1;
    xwall[nXWall].reference = -1;
}

ushort dbInsertXSector(int nSector)
{
    ushort nXSector = GetFree(nextXSector);
    if (nXSector == 0)
    {
        ThrowError(807+LDIFF1)("Out of free XSectors");
    }
    memset(&xsector[nXSector], 0, sizeof(XSECTOR));
    sector[nSector].extra = nXSector;
    xsector[nXSector].reference = nSector;
    return nXSector;
}

void dbDeleteXSector(int nXSector)
{
    dassert(xsector[nXSector].reference >= 0, 819+LDIFF1);
    InsertFree(nextXSector, nXSector);
    sector[xsector[nXSector].reference].extra = -1;
    xsector[nXSector].reference = -1;
}

void dbXSpriteClean(void)
{
    int nSprite, nXSprite;
    for (nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        nXSprite = sprite[nSprite].extra;
        if (nXSprite == 0)
        {
            sprite[nSprite].extra = -1;
        }
        if (sprite[nSprite].statnum < kMaxStatus && nXSprite > 0)
        {
            dassert(nXSprite < kMaxXSprites, 846+LDIFF1);
            if (xsprite[nXSprite].reference != nSprite)
            {
                int nXSprite2 = dbInsertXSprite(nSprite);
                xsprite[nXSprite2] = xsprite[nXSprite];
                xsprite[nXSprite2].reference = nSprite;
            }
        }
    }
    for (nXSprite = 1; nXSprite < kMaxXSprites; nXSprite++)
    {
        nSprite = xsprite[nXSprite].reference;
        if (nSprite >= 0)
        {
            dassert(nSprite < kMaxSprites, 864+LDIFF1);
            if (sprite[nSprite].statnum >= kMaxStatus || sprite[nSprite].extra != nXSprite)
            {
                InsertFree(nextXSprite, nXSprite);
                xsprite[nXSprite].reference = -1;
            }
        }
    }
}

void dbXWallClean(void)
{
    int nWall, nXWall;
    for (nWall = 0; nWall < numwalls; nWall++)
    {
        nXWall = wall[nWall].extra;
        if (nXWall == 0)
        {
            wall[nWall].extra = -1;
        }
        if (nXWall > 0)
        {
            dassert(nXWall < kMaxXWalls, 894+LDIFF1);
            if (xwall[nXWall].reference == -1)
            {
                wall[nWall].extra = -1;
            }
            else
            {
                xwall[nXWall].reference = nWall;
            }
        }
    }
    for (nWall = 0; nWall < numwalls; nWall++)
    {
        if (wall[nWall].extra > 0)
        {
            nXWall = wall[nWall].extra;
            dassert(nXWall < kMaxXWalls, 914+LDIFF1);
            if (xwall[nXWall].reference != nWall)
            {
                int nXWall2 = dbInsertXWall(nWall);
                xwall[nXWall2] = xwall[nXWall];
                xwall[nXWall2].reference = nWall;
            }
        }
    }
    for (nXWall = 1; nXWall < kMaxXWalls; nXWall++)
    {
        nWall = xwall[nXWall].reference;
        if (nWall >= 0)
        {
            dassert(nWall < kMaxWalls, 933+LDIFF1);
            if (nWall >= numwalls || wall[nWall].extra != nXWall)
            {
                InsertFree(nextXWall, nXWall);
                xwall[nXWall].reference = -1;
            }
        }
    }
}

void dbXSectorClean(void)
{
    int nSector, nXSector;
    for (nSector = 0; nSector < numsectors; nSector++)
    {
        int nXSector = sector[nSector].extra;
        if (nXSector == 0)
        {
            sector[nSector].extra = -1;
        }
        if (nXSector > 0)
        {
            dassert(nXSector < kMaxXSectors, 963+LDIFF1);
            if (xsector[nXSector].reference == -1)
            {
                sector[nSector].extra = -1;
            }
            else
            {
                xsector[nXSector].reference = nSector;
            }
        }
    }
    for (nSector = 0; nSector < numsectors; nSector++)
    {
        if (sector[nSector].extra > 0)
        {
            nXSector = sector[nSector].extra;
            dassert(nXSector < kMaxXSectors, 983+LDIFF1);
            if (xsector[nXSector].reference != nSector)
            {
                int nXSector2 = dbInsertXSector(nSector);
                xsector[nXSector2] = xsector[nXSector];
                xsector[nXSector2].reference = nSector;
            }
        }
    }
    for (nXSector = 1; nXSector < kMaxXSectors; nXSector++)
    {
        int nSector = xsector[nXSector].reference;
        if (nSector >= 0)
        {
            dassert(nSector < kMaxSectors, 1002+LDIFF1);
            if (nSector >= numsectors || sector[nSector].extra != nXSector)
            {
                InsertFree(nextXSector, nXSector);
                xsector[nXSector].reference = -1;
            }
        }
    }
}

void dbInit(void)
{
    InitFreeList(nextXSprite, kMaxXSprites);
    for (int i = 1; i < kMaxXSprites; i++)
    {
        xsprite[i].reference = -1;
    }
    InitFreeList(nextXWall, kMaxXWalls);
    for (i = 1; i < kMaxXWalls; i++)
    {
        xwall[i].reference = -1;
    }
    InitFreeList(nextXSector, kMaxXSectors);
    for (i = 1; i < kMaxXSectors; i++)
    {
        xsector[i].reference = -1;
    }
    initspritelists();
    for (i = 0; i < kMaxSprites; i++)
    {
        sprite[i].cstat = 128;
    }
}

void PropagateMarkerReferences(void)
{
    int nNextSprite;
    int nOwner;
    int nXSector;
    for (int nSprite = headspritestat[10]; nSprite != -1; nSprite = nNextSprite)
    {
        nNextSprite = nextspritestat[nSprite];
        switch (sprite[nSprite].type)
        {
        case 8:
        {
            nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2c_0 = nSprite;
                    continue;
                }
            }
            break;
        }
        case 3:
        {
            nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2c_0 = nSprite;
                    continue;
                }
            }
            break;
        }
        case 4:
        {
            nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2e_0 = nSprite;
                    continue;
                }
            }
            break;
        }
        case 5:
        {
            nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2c_0 = nSprite;
                    continue;
                }
            }
            break;
        }
        }
        DeleteSprite(nSprite);
    }
}

BOOL char_1A76C6, char_1A76C7, char_1A76C8;

struct MAPSIGNATURE {
    char signature[4];
    short version;
};

struct MAPHEADER{
    int at0; // x
    int at4; // y
    int at8; // z
    short atc; // ang
    short ate; // sect
    short at10; // pskybits
    int at12; // visibility
    int at16; // song id, Matt
    byte at1a; // parallaxtype
    int at1b; // map revision
    ushort at1f; // numsectors
    ushort at21; // numwalls
    ushort at23; // numsprites
};

ulong dbReadMapCRC(char *pPath)
{
    char path1[_MAX_PATH2], path2[_MAX_PATH];
    char *ext, *dir, *node, *fname;
    MAPSIGNATURE header;
    char_1A76C7 = 0;
    char_1A76C8 = 0;
    _splitpath2(pPath, path1, &node, &dir, &fname, &ext);
    _makepath(path2, NULL, NULL, fname, NULL);
    DICTNODE *pNode = gSysRes.Lookup(path2, "MAP");
    if (!pNode)
    {
        ThrowError(1144+LDIFF1)("Error opening map file %s", path2);
    }
    int nSize = Resource::Size(pNode);
    byte *pData = (byte*)gSysRes.Lock(pNode);
    IOBuffer iob(nSize, pData);
    iob.Read(&header, 6);
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        ThrowError(1154+LDIFF1)("Map file corrupted");
    }
#ifdef REGISTERED
    if ((header.version & 0xff00) == 0x600)
    {
    }
    else if ((header.version & 0xff00) == 0x700)
    {
        char_1A76C8 = 1;
    }
    else
    {
        ThrowError(1164+LDIFF1)("Map file is wrong version");
    }
#else
    if ((header.version & 0xff00) != 0x600)
        ThrowError(1167+LDIFF1)("Map file is wrong version");
#endif
    ulong nCRC = *(ulong*)(pData+nSize-4);
    gSysRes.Unlock(pNode);
    return nCRC;
}

void dbLoadMap(char *pPath, long *pX, long *pY, long *pZ, short *pAngle, short *pSector, ulong *pCRC)
{
    char path2[_MAX_PATH];
    int i;
    int numsprites;
    int nSize;
    MAPSIGNATURE header;
    MAPHEADER mapHeader;
    byte* pData;
    ulong nCRC;
    char* node;
    char* dir;
    char* fname;
    char* ext;
    char path1[_MAX_PATH2];
    memset(show2dsector, 0, sizeof(show2dsector));
    memset(show2dwall, 0, sizeof(show2dwall));
    memset(show2dsprite, 0, sizeof(show2dsprite));
    _splitpath2(pPath, path1, &node, &dir, &fname, &ext);
    _makepath(path2, NULL, NULL, fname, NULL);
    DICTNODE *pNode = gSysRes.Lookup(path2, "MAP");
    if (!pNode)
    {
        ThrowError(1211+LDIFF1)("Error opening map file %s", path2);
    }
    nSize = Resource::Size(pNode);
    pData = (byte*)gSysRes.Lock(pNode);
    IOBuffer IOBuffer1 = IOBuffer(nSize, pData);
    IOBuffer1.Read(&header, 6);
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        ThrowError(1221+LDIFF1)("Map file corrupted");
    }
#ifdef REGISTERED
    char_1A76C8 = 0;
    if ((header.version & 0xff00) != 0x600)
    {
        if ((header.version & 0xff00) == 0x700)
        {
            char_1A76C8 = 1;
        }
        else
        {
            ThrowError(1235+LDIFF1)("Map file is wrong version");
        }
    }
#else
    if ((header.version & 0xff00) != 0x600)
    {
        ThrowError(1238+LDIFF1)("Map file is wrong version");
    }
#endif
    IOBuffer1.Read(&mapHeader,37);
#ifdef REGISTERED
    if (mapHeader.at16 != 0 && mapHeader.at16 != 'ttaM' && mapHeader.at16 != 'Matt')
    {
        dbCrypt((byte*)&mapHeader, 37, (int)'ttaM');
        char_1A76C7 = 1;
    }
#endif
    *pX = mapHeader.at0;
    *pY = mapHeader.at4;
    *pZ = mapHeader.at8;
    *pAngle = mapHeader.atc;
    *pSector = mapHeader.ate;
    pskybits = mapHeader.at10;
    gVisibility = visibility = mapHeader.at12;
    gSongId = mapHeader.at16;
#ifdef REGISTERED
    if (char_1A76C8)
    {
        if (mapHeader.at16 == 'ttaM' || mapHeader.at16 == 'Matt')
        {
            char_1A76C6 = 1;
        }
        else if (!mapHeader.at16)
        {
            char_1A76C6 = 0;
        }
        else
        {
            ThrowError(1267+LDIFF1)("Corrupted Map file");
        }
    }
    else
#endif
    if (mapHeader.at16)
    {
        ThrowError(1272+LDIFF1)("Corrupted Map file");
    }
    parallaxtype = mapHeader.at1a;
    gMapRev = mapHeader.at1b;
    numsectors = mapHeader.at1f;
    numwalls = mapHeader.at21;
    numsprites = mapHeader.at23;
    dbInit();
#ifdef REGISTERED
    if (char_1A76C8)
    {
        IOBuffer1.Read(&char_19AE44, 128);
        dbCrypt((byte*)&char_19AE44, 128, numwalls);
    }
    else
    {
        memset(&char_19AE44, 0, 128);
    }
#endif
    gSkyCount = 1<<pskybits;
    IOBuffer1.Read(pskyoff, gSkyCount*sizeof(pskyoff[0]));
#ifdef REGISTERED
    if (char_1A76C8)
    {
        dbCrypt((byte*)pskyoff, gSkyCount*sizeof(pskyoff[0]), gSkyCount*2);
    }
#endif
    for (i = 0; i < numsectors; i++)
    {
        IOBuffer1.Read(&sector[i], sizeof(SECTOR));
#ifdef REGISTERED
        if (char_1A76C8)
        {
            dbCrypt((byte*)&sector[i], sizeof(SECTOR), gMapRev*sizeof(SECTOR));
        }
#endif
        if (sector[i].extra > 0)
        {
            XSECTOR *pXSector = &xsector[dbInsertXSector(i)];
            memset(pXSector, 0, sizeof(XSECTOR));
            int size;
            if (!char_1A76C8)
                size = sizeof(XSECTOR);
            else
#ifdef REGISTERED
                size = char_19AE44.at48;
#else
                size = sizeof(XSECTOR);
#endif
            IOBuffer1.Read(pXSector, size);
            xsector[sector[i].extra].reference = i;
            xsector[sector[i].extra].at1_7 = xsector[sector[i].extra].at1_6<<16;
        }
    }
    for (i = 0; i < numwalls; i++)
    {
        IOBuffer1.Read(&wall[i], sizeof(WALL));
#ifdef REGISTERED
        if (char_1A76C8)
        {
            dbCrypt((byte*)&wall[i], sizeof(WALL), (gMapRev*sizeof(SECTOR))|(int)'ttaM');
        }
#endif
        if (wall[i].extra > 0)
        {
            XWALL *pXWall = &xwall[dbInsertXWall(i)];
            memset(pXWall, 0, sizeof(XWALL));
            int size;
            if (!char_1A76C8)
                size = sizeof(XWALL);
            else
#ifdef REGISTERED
                size = char_19AE44.at44;
#else
                size = sizeof(XWALL);
#endif
            IOBuffer1.Read(pXWall, size);
            xwall[wall[i].extra].reference = i;
            xwall[wall[i].extra].at1_7 = xwall[wall[i].extra].at1_6 << 16;
        }
    }
    initspritelists();
    for (i = 0; i < numsprites; i++)
    {
        RemoveSpriteStat((short)i);
        IOBuffer1.Read(&sprite[i], sizeof(SPRITE));
#ifdef REGISTERED
        if (char_1A76C8)
        {
            dbCrypt((byte*)&sprite[i], sizeof(SPRITE), (gMapRev*sizeof(SPRITE)) | (int)'ttaM');
        }
#endif
        InsertSpriteSect((short)i, sprite[i].sectnum);
        InsertSpriteStat((short)i, sprite[i].statnum);
        sprite[i].index = i;
        if (sprite[i].extra > 0)
        {
            XSPRITE *pXSprite = &xsprite[dbInsertXSprite(i)];
            memset(pXSprite, 0, sizeof(XSPRITE));
            int size;
            if (!char_1A76C8)
                size = sizeof(XSPRITE);
#ifdef REGISTERED
            else
                size = char_19AE44.at40;
#else
            size = sizeof(XSPRITE);
#endif
            IOBuffer1.Read(pXSprite, size);
            xsprite[sprite[i].extra].reference = i;
            xsprite[sprite[i].extra].at1_7 = xsprite[sprite[i].extra].at1_6 << 16;
            if (!char_1A76C8)
            {
                xsprite[sprite[i].extra].atb_7 = xsprite[sprite[i].extra].atf_5;
            }
        }
        if ((sprite[i].cstat & kSpriteMask) == kSpriteVoxel)
        {
            sprite[i].cstat &= ~kSpriteMask;
        }
    }
    IOBuffer1.Read(&nCRC, 4);
    if (CRC32(pData, nSize-4) != nCRC)
    {
        ThrowError(1439+LDIFF1)("Map File does not match CRC");
    }
    *pCRC = nCRC;
    gSysRes.Unlock(pNode);
    PropagateMarkerReferences();
#ifdef REGISTERED
    if (char_1A76C8)
    {
        if (gSongId == 'ttaM' || gSongId == 'Matt')
        {
            char_1A76C6 = 1;
        }
        else if (!gSongId)
        {
            char_1A76C6 = 0;
        }
        else
        {
            ThrowError(1457+LDIFF1)("Corrupted Map file");
        }
    }
    else
#endif
    if (gSongId != 0)
    {
        ThrowError(1462+LDIFF1)("Corrupted Shareware Map file");
    }
    if ((header.version & 0xff00) == 0x603) // BUG
    {
        switch (header.version&0xff)
        {
        case 0:
            for (i = 0; i < numsectors; i++)
            {
                SECTOR *pSector = &sector[i];
                int nXSector = pSector->extra;
                if (nXSector > 0)
                {
                    XSECTOR *pXSector = &xsector[nXSector];
                    pXSector->at18_2 = pXSector->ata_4;
                    if (pXSector->atc_0 > 0)
                    {
                        if (pXSector->atd_4 == 0)
                        {
                            pXSector->atf_6 = 1;
                        }
                        else
                        {
                            pXSector->at19_6 = pXSector->atc_0;
                            pXSector->atc_0 = 0;
                            pXSector->atf_7 = 1;
                        }
                    }
                }
            }
        case 1:
            for (i = 0; i < numsectors; i++)
            {
                SECTOR *pSector = &sector[i];
                int nXSector = pSector->extra;
                if (nXSector > 0)
                {
                    XSECTOR *pXSector = &xsector[nXSector];
                    pXSector->ate_6 >>= 1;
                }
            }
        case 2:
            for (i = 0; i < kMaxSprites; i++) // dead code
            {
                if (sprite[i].statnum != 1024)
                {
                    SPRITE *pSprite = &sprite[i];
                    int nXSprite = pSprite->extra;
                    if (nXSprite > 0)
                    {
                        XSPRITE *pXSprite = &xsprite[nXSprite];
                    }
                }
            }
            break;
            
        }
    }
}

void dbSaveMap(char *pPath, long nX, long nY, long nZ, short nAngle, short nSector)
{
    char sMapExt[_MAX_PATH];
    char sBakExt[_MAX_PATH];
    int nHandle;
    MAPSIGNATURE header;
    MAPHEADER mapheader;
    int nSize;
    byte *pData;
#if MAPEDIT
    char_1A76C8 = 1;
#endif
    int nSpriteNum = 0;
    int i;
    ulong nCRC;
    gSkyCount = 1<<pskybits;
    gMapRev++;
    strcpy(sMapExt, pPath);
    strcpy(sBakExt, pPath);
    ChangeExtension(sMapExt, ".MAP");
    ChangeExtension(sBakExt, ".BAK");
    nSize = sizeof(MAPSIGNATURE)+sizeof(MAPHEADER);
#ifdef REGISTERED
    if (char_1A76C8)
    {
        nSize += sizeof(MAPHEADER2);
    }
#endif
    nSize += gSkyCount*sizeof(pskyoff[0]);
    nSize += sizeof(SECTOR)*numsectors;
    for (i = 0; i < numsectors; i++)
    {
        if (sector[i].extra > 0)
        {
            nSize += sizeof(XSECTOR);
        }
    }
    nSize += sizeof(WALL)*numwalls;
    for (i = 0; i < numwalls; i++)
    {
        if (wall[i].extra > 0)
        {
            nSize += sizeof(XWALL);
        }
    }
    for (i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            nSpriteNum++;
            if (sprite[i].extra > 0)
            {
                nSize += sizeof(XSPRITE);
            }
        }
    }
    nSize += sizeof(SPRITE)*nSpriteNum;
    nSize += 4;
    pData = (byte*)Resource::Alloc(nSize);
    IOBuffer IOBuffer1(nSize, pData);
    memcpy(&header, "BLM\x1a", 4);
#if MAPEDIT
    header.version = 0x700;
    char_1A76C7 = 1;
#else
    if (char_1A76C8)
    {
        header.version = 0x700;
        char_1A76C7 = 1;
    }
    else
    {
        header.version = 0x603;
        char_1A76C7 = 0;
    }
#endif
    IOBuffer1.Write(&header, sizeof(header));
    mapheader.at0 = nX;
    mapheader.at4 = nY;
    mapheader.at8 = nZ;
    mapheader.atc = nAngle;
    mapheader.ate = nSector;
    mapheader.at10 = pskybits;
    mapheader.at12 = gVisibility;
#if MAPEDIT
    gSongId = 'ttaM';
#else
    if (char_1A76C6)
    {
        gSongId = 'ttaM';
    }
    else
    {
        gSongId = 0;
    }
#endif
    mapheader.at16 = gSongId;
    mapheader.at1a = parallaxtype;
    mapheader.at1b = gMapRev;
    mapheader.at1f = numsectors;
    mapheader.at21 = numwalls;
    mapheader.at23 = nSpriteNum;
    if (char_1A76C7)
    {
        dbCrypt((byte*)&mapheader, sizeof(MAPHEADER), (int)'ttaM');
    }
    IOBuffer1.Write(&mapheader, sizeof(MAPHEADER));
#ifdef REGISTERED
    if (char_1A76C8)
    {
#if MAPEDIT
        memset(&char_19AE44, 0, sizeof(MAPHEADER2));
#endif
        strcpy(char_19AE44.at0, "Copyright 1997 Monolith Productions.  All Rights Reserved");
        char_19AE44.at40 = sizeof(XSPRITE);
        char_19AE44.at44 = sizeof(XWALL);
        char_19AE44.at48 = sizeof(XSECTOR);
        dbCrypt((byte*)&char_19AE44, sizeof(MAPHEADER2), numwalls);
        IOBuffer1.Write(&char_19AE44, sizeof(MAPHEADER2));
        dbCrypt((byte*)&char_19AE44, sizeof(MAPHEADER2), numwalls);
    }
#endif
    if (char_1A76C8)
    {
        dbCrypt((byte*)pskyoff, gSkyCount*sizeof(pskyoff[0]), gSkyCount*sizeof(pskyoff[0]));
    }
    IOBuffer1.Write(pskyoff, gSkyCount*sizeof(pskyoff[0]));
    if (char_1A76C8)
    {
        dbCrypt((byte*)pskyoff, gSkyCount*sizeof(pskyoff[0]), gSkyCount*sizeof(pskyoff[0]));
    }
    for (i = 0; i < numsectors; i++)
    {
        if (char_1A76C8)
        {
            dbCrypt((byte*)&sector[i], sizeof(SECTOR), gMapRev*sizeof(SECTOR));
        }
        IOBuffer1.Write(&sector[i], sizeof(SECTOR));
        if (char_1A76C8)
        {
            dbCrypt((byte*)&sector[i], sizeof(SECTOR), gMapRev*sizeof(SECTOR));
        }
        if (sector[i].extra > 0)
        {
            IOBuffer1.Write(&xsector[sector[i].extra], sizeof(XSECTOR));
        }
    }
    for (i = 0; i < numwalls; i++)
    {
        if (char_1A76C8)
        {
            dbCrypt((byte*)&wall[i], sizeof(WALL), gMapRev*sizeof(SECTOR) | (int)'ttaM');
        }
        IOBuffer1.Write(&wall[i], sizeof(WALL));
        if (char_1A76C8)
        {
            dbCrypt((byte*)&wall[i], sizeof(WALL), gMapRev*sizeof(SECTOR) | (int)'ttaM');
        }
        if (wall[i].extra > 0)
        {
            IOBuffer1.Write(&xwall[wall[i].extra], sizeof(XWALL));
        }
    }
    for (i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            if (char_1A76C8)
            {
                dbCrypt((byte*)&sprite[i], sizeof(SPRITE), gMapRev*sizeof(SPRITE) | (int)'ttaM');
            }
            IOBuffer1.Write(&sprite[i], sizeof(SPRITE));
            if (char_1A76C8)
            {
                dbCrypt((byte*)&sprite[i], sizeof(SPRITE), gMapRev*sizeof(SPRITE) | (int)'ttaM');
            }
            if (sprite[i].extra > 0)
            {
                IOBuffer1.Write(&xsprite[sprite[i].extra], sizeof(XSPRITE));
            }
        }
    }
    nCRC = CRC32(pData, nSize-4);
    IOBuffer1.Write(&nCRC, 4);
    unlink(sBakExt);
    rename(sMapExt, sBakExt);
    nHandle = open(sMapExt, O_BINARY|O_TRUNC|O_CREAT|O_WRONLY, S_IWRITE);
    if (nHandle == -1)
    {
        ThrowError(1747+LDIFF1)("Error opening MAP file");
    }
    if (write(nHandle, pData, nSize) != nSize)
    {
        ThrowError(1750+LDIFF1)("Error writing MAP file");
    }
    close(nHandle);
    Resource::Free(pData);
    char *pExt = strchr(sMapExt, '.');
    if (pExt)
    {
        *pExt = 0;
    }
    gSysRes.AddExternalResource(sMapExt, "MAP", nSize);
    DICTNODE *hMap = gSysRes.Lookup(sMapExt, "MAP");
    dassert(hMap != NULL, 1761+LDIFF1);
}
