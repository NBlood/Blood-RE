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
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "typedefs.h"
#include "build.h"
#include "db.h"
#include "debug4g.h"
#include "error.h"
#include "globals.h"
#include "gui.h"
#include "helix.h"
#include "key.h"
#include "misc.h"
#include "resource.h"
#include "screen.h"
#include "tile.h"

#if APPVER_BLOODREV >= AV_BR_BL120
#define LDIFF1 0
#else
#define LDIFF1 -2
#endif

#ifndef _3DFX
extern "C" void loadvoxel(int nVoxel)
{
    static int nLastVoxel = 0;
    dassert(nVoxel >= 0 && nVoxel < kMaxVoxels, 117);
    DICTNODE *hVox = gSysRes.Lookup(nVoxel, "KVX");
    if (!hVox) {
        ThrowError(121)("Missing voxel #%d", nVoxel);
    }

    if (!hVox->lockCount)
        voxoff[nLastVoxel][0] = 0;
    nLastVoxel = nVoxel;
    byte *pVox = (byte*)gSysRes.Lock(hVox);
    for (int i = 0; i < kMaxVoxMips; i++)
    {
        int nSize = *((int*)pVox);
        pVox += 4;
        voxoff[nVoxel][i] = pVox;
        pVox += nSize;
    }
}
#endif

CACHENODE tileNode[kMaxTiles];

int tileStart[256];
int tileEnd[256];
int hTileFile[256];

BOOL tileFileDirty[256];
int nTileFiles;

BOOL artLoaded;

int tileHist[kMaxTiles];

BOOL bSurfChanged;
BOOL bShadeChanged;
BOOL bVoxelChanged;

short tileIndex[kMaxTiles];
int tileIndexCount;

char surfType[kMaxTiles];

signed char tileShade[kMaxTiles];

short voxelIndex[kMaxTiles];

int pickSize[] = { 32, 40, 64, 80, 128, 160 };

char *pzBaseFileName = "TILES%03i.ART";

void tileTerm(void)
{
    for (int i = 0; i < 256; i++)
    {
        if (hTileFile[i] != -1)
        {
            close(hTileFile[i]);
            hTileFile[i] = -1;
        }
    }
}

void CalcPicsiz(int a1, int a2, int a3)
{
    byte nP = 0;
    for (int i = 2; i <= a2; i<<= 1)
        nP++;
    for (i = 2; i <= a3; i<<= 1)
        nP+=1<<4;
    picsiz[a1] = nP;
}

int tileInit(BOOL a1, char *a2)
{
    int i;
    int hFile;
    char filename[20];
    if (artLoaded)
        return nTileFiles;

    memset(tilesizx, 0, sizeof(tilesizx));
    memset(tilesizy, 0, sizeof(tilesizy));
    memset(picanm, 0, sizeof(picanm));
    memset(gotpic, 0, sizeof(gotpic));
    memset(hTileFile, -1, sizeof(hTileFile));
#ifndef _3DFX
    memset(voxoff, 0, sizeof(voxoff));
#endif

    int v8 = 0;

    while (1)
    {
        if (a2)
            sprintf(filename, a2, v8);
        else
            sprintf(filename, pzBaseFileName, v8);

        hFile = open(filename, O_BINARY | O_RDWR);
        if (hFile == -1)
            break;

        hTileFile[v8] = hFile;

        read(hFile, &artversion, 4);
        if (a1)
        {
            if (artversion != 69)
                ThrowError(213+LDIFF1)("Invalid .ART files\n");
            dassert(artversion == 69, 214+LDIFF1);
        }
        read(hFile, &numtiles, 4);
        read(hFile, &tileStart[v8], 4);
        read(hFile, &tileEnd[v8], 4);
        int num = tileEnd[v8] - tileStart[v8] + 1;
        read(hFile, &tilesizx[tileStart[v8]], num * 2);
        read(hFile, &tilesizy[tileStart[v8]], num * 2);
        read(hFile, &picanm[tileStart[v8]], num * 4);
        int pos = lseek(hFile, 0, SEEK_CUR);
        for (i = tileStart[v8]; i <= tileEnd[v8]; i++)
        {
            tilefilenum[i] = v8;
            tilefileoffs[i] = pos;
            pos += tilesizx[i] * tilesizy[i];
        }
        v8++;
    }
    nTileFiles = v8;

    for (i = 0; i < kMaxTiles; i++)
    {
        CalcPicsiz(i, tilesizx[i], tilesizy[i]);
        waloff[i] = NULL;
        voxelIndex[i] = -1;
        tileNode[i].ptr = NULL;
    }

    hFile = open("SURFACE.DAT", O_BINARY | O_RDWR);
    if (hFile != -1)
    {
        read(hFile, surfType, kMaxTiles);
        close(hFile);
    }

    hFile = open("VOXEL.DAT", O_BINARY | O_RDWR);
    if (hFile != -1)
    {
        read(hFile, voxelIndex, kMaxTiles * 2);
        close(hFile);
    }

    hFile = open("SHADE.DAT", O_BINARY | O_RDWR);
    if (hFile != -1)
    {
        read(hFile, tileShade, kMaxTiles);
        close(hFile);
    }

    artLoaded = TRUE;
    return nTileFiles;
}

void tileSaveArt(void)
{
    char v44[20];
    char v30[20];
    char v1c[20];
    int i;
    int j;
    int hFile;
    for (i = 0; i < nTileFiles; i++)
    {
        if (!tileFileDirty[i])
            continue;
        tmpnam(v30);
        hFile = open(v30, O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE);
        write(hFile, &artversion, 4);
        write(hFile, &numtiles, 4);
        write(hFile, &tileStart[i], 4);
        write(hFile, &tileEnd[i], 4);
        int num = tileEnd[i] - tileStart[i] + 1;
        write(hFile, &tilesizx[tileStart[i]], num * 2);
        write(hFile, &tilesizy[tileStart[i]], num * 2);
        write(hFile, &picanm[tileStart[i]], num * 4);
        for (j = tileStart[i]; j <= tileEnd[i]; j++)
        {
            write(hFile, tileLoadTile(j), tilesizx[j] * tilesizy[j]);
        }
        close(hFile);
        sprintf(v1c, "TILES%03i.BAK", i);
        remove(v1c);
        sprintf(v44, "TILES%03i.ART", i);
        rename(v44, v1c);
        rename(v30, v44);
        tileFileDirty[i] = FALSE;
    }
    for (i = 0; i < nTileFiles; i++)
    {
        close(hTileFile[i]);
        hTileFile[i] = -1;
    }

    if (bShadeChanged)
    {
        hFile = open("SHADE.DAT", O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE);
        write(hFile, tileShade, kMaxTiles);
        close(hFile);
        bShadeChanged = FALSE;
    }

    if (bSurfChanged)
    {
        hFile = open("SURFACE.DAT", O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE);
        write(hFile, surfType, kMaxTiles);
        close(hFile);
        bSurfChanged = FALSE;
    }

    if (bVoxelChanged)
    {
        hFile = open("VOXEL.DAT", O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE);
        write(hFile, voxelIndex, kMaxTiles * 2);
        close(hFile);
        bVoxelChanged = FALSE;
    }
    artLoaded = FALSE;
    tileInit(TRUE, NULL);
}

void tileSaveArtInfo(void)
{
    int i;
    int hFile;
    for (i = 0; i < nTileFiles; i++)
    {
        if (!tileFileDirty[i])
            continue;
        hFile = hTileFile[i];
        dassert(hFile != -1, 360+LDIFF1);
        lseek(hFile, 0, SEEK_SET);
        write(hFile, &artversion, 4);
        write(hFile, &numtiles, 4);
        write(hFile, &tileStart[i], 4);
        write(hFile, &tileEnd[i], 4);
        int num = tileEnd[i] - tileStart[i] + 1;
        write(hFile, &tilesizx[tileStart[i]], num * 2);
        write(hFile, &tilesizy[tileStart[i]], num * 2);
        write(hFile, &picanm[tileStart[i]], num * 4);
        tileFileDirty[i] = FALSE;
    }

    if (bShadeChanged)
    {
        hFile = open("SHADE.DAT", O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE);
        write(hFile, tileShade, kMaxTiles);
        close(hFile);
        bShadeChanged = FALSE;
    }

    if (bSurfChanged)
    {
        hFile = open("SURFACE.DAT", O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE);
        write(hFile, surfType, kMaxTiles);
        close(hFile);
        bSurfChanged = FALSE;
    }

    if (bVoxelChanged)
    {
        hFile = open("VOXEL.DAT", O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE);
        write(hFile, voxelIndex, kMaxTiles * 2);
        close(hFile);
        bVoxelChanged = FALSE;
    }
}

void tileShadeDirty(void)
{
    bShadeChanged = TRUE;
}

void tileSurfDirty(void)
{
    bSurfChanged = TRUE;
}

void tileVoxelDirty(void)
{
    bVoxelChanged = TRUE;
}

void tileMarkDirty(int nTile)
{
    tileFileDirty[tilefilenum[nTile]] = TRUE;
}

void tileMarkDirtyAll(void)
{
    int i;
    for (i = 0; i < nTileFiles; i++)
        tileFileDirty[i] = TRUE;
}

void tilePurgeTile(int nTile)
{
    CACHENODE *node = &tileNode[nTile];
    waloff[nTile] = NULL;
    if (node->ptr)
    {
        Resource::Flush(node);
        dassert(node->ptr == NULL, 442+LDIFF1);
    }
}

void tilePurgeAll(void)
{
    int i;
    for (i = 0; i < kMaxTiles; i++)
    {
        CACHENODE *node = &tileNode[i];
        waloff[i] = NULL;
        if (node->ptr)
            Resource::Flush(node);
    }
}

byte *tileLoadTile(int nTile)
{
    static int nLastTile;
    if (tileNode[nLastTile].lockCount == 0)
        waloff[nLastTile] = 0;
    nLastTile = nTile;
    dassert(nTile >= 0 && nTile < kMaxTiles, 472+LDIFF1);
    CACHENODE *node = &tileNode[nTile];
    if (node->ptr)
    {
        waloff[nTile] = (byte*)node->ptr;
        if (node->lockCount == 0)
        {
            Resource::RemoveMRU(node);
            Resource::AddMRU(node);
            nLastTile = nTile;
        }
        return waloff[nTile];
    }
    int nSize = tilesizx[nTile] * tilesizy[nTile];
    if (nSize <= 0)
        return NULL;
    gCacheMiss = gGameClock + 30;
    dassert(node->lockCount == 0, 508+LDIFF1);
    node->ptr = Resource::Alloc(nSize);
    waloff[nTile] = (byte*)node->ptr;
    Resource::AddMRU(node);
    int hFile = hTileFile[tilefilenum[nTile]];
    lseek(hFile, tilefileoffs[nTile], SEEK_SET);
    read(hFile, node->ptr, nSize);
    return waloff[nTile];
}

byte *tileLockTile(int nTile)
{
    dassert(nTile >= 0 && nTile < kMaxTiles, 524+LDIFF1);
    if (!tileLoadTile(nTile))
        return NULL;
    CACHENODE *node = &tileNode[nTile];
    if (node->lockCount++ == 0)
    {
        Resource::RemoveMRU(node);
    }
    return waloff[nTile];
}

void tileUnlockTile(int nTile)
{
    dassert(nTile >= 0 && nTile < kMaxTiles, 540+LDIFF1);
    waloff[nTile] = 0;
    CACHENODE *node = &tileNode[nTile];
    if (node->lockCount > 0)
    {
        if (--node->lockCount == 0)
        {
            Resource::AddMRU(node);
        }
    }
}

byte *tileAllocTile(int nTile, int x, int y, int ox, int oy)
{
    dassert(nTile >= 0 && nTile < kMaxTiles, 559+LDIFF1);
    if (x <= 0 || y <= 0 || nTile >= kMaxTiles)
        return NULL;
    int nSize = x * y;
    byte *p = (byte*)Resource::Alloc(nSize);
    dassert(p != NULL, 567+LDIFF1);
    tileNode[nTile].lockCount++;
    tileNode[nTile].ptr = p;
    waloff[nTile] = p;
    tilesizx[nTile] = x;
    tilesizy[nTile] = y;

    picanm[nTile].animframes = 0;
    picanm[nTile].animtype = 0;
    picanm[nTile].xoffset = ClipRange(ox, -127, 127);
    picanm[nTile].yoffset = ClipRange(oy, -127, 127);
    picanm[nTile].animspeed = 0;

    CalcPicsiz(nTile, x, y);

    return waloff[nTile];
}

void tileFreeTile(int nTile)
{
    dassert(nTile >= 0 && nTile < kMaxTiles, 591+LDIFF1);
    tilePurgeTile(nTile);

    waloff[nTile] = NULL;

    tilesizx[nTile] = 0;
    tilesizy[nTile] = 0;

    picanm[nTile].animframes = 0;
    picanm[nTile].animtype = 0;
    picanm[nTile].xoffset = 0;
    picanm[nTile].yoffset = 0;
    picanm[nTile].animspeed = 0;

    picsiz[nTile] = 0;
}

void tilePreloadTile(int nTile)
{
    int n = 0;
    switch (picanm[nTile].at3_4)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    case 6:
    case 7:
        if (voxelIndex[nTile] < 0 || voxelIndex[nTile] >= kMaxVoxels)
        {
            voxelIndex[nTile] = -1;
            picanm[nTile].at3_4 = 0;
        }
#ifndef _3DFX
        else
            loadvoxel(voxelIndex[nTile]);
#endif
        break;
    }
    for(; n > 0; n--, nTile += 1 + picanm[nTile].animframes)
    {
        if (picanm[nTile].animtype)
        {
            for (int frame = picanm[nTile].animframes; frame >= 0; frame--)
            {
                if (picanm[nTile].animtype == 3)
                    tileLoadTile(nTile-frame);
                else
                    tileLoadTile(nTile+frame);
            }
        }
        else
            tileLoadTile(nTile);
    }
}

void tilePrecacheTile(int nTile)
{
    int n = 0;
    switch (picanm[nTile].at3_4)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    }
    for(; n > 0; n--, nTile += 1 + picanm[nTile].animframes)
    {
        if (picanm[nTile].animtype)
        {
            for (int frame = picanm[nTile].animframes; frame >= 0; frame--)
            {
                if (picanm[nTile].animtype == 3)
                    SetBitString(gotpic, nTile-frame);
                else
                    SetBitString(gotpic, nTile+frame);
            }
        }
        else
            SetBitString(gotpic, nTile);
    }
}

int CompareTileFreqs(const void *a1, const void *a2)
{
    return tileHist[*(short*)a2] - tileHist[*(short*)a1];
}

int tileBuildHistogram(int a1)
{
    int i;
    memset(tileHist, 0, sizeof(tileHist));
    switch (a1)
    {
    case 0:
        for (i = 0; i < numwalls; i++)
        {
            tileHist[wall[i].picnum]++;
        }
        break;
    case 1:
        for (i = 0; i < numsectors; i++)
        {
            tileHist[sector[i].ceilingpicnum]++;
        }
        break;
    case 2:
        for (i = 0; i < numsectors; i++)
        {
            tileHist[sector[i].floorpicnum]++;
        }
        break;
    case 4:
        for (i = 0; i < numwalls; i++)
        {
            tileHist[wall[i].overpicnum]++;
        }
        break;
    case 3:
        for (i = 0; i < kMaxSprites; i++)
        {
            if (sprite[i].statnum != kStatFree && sprite[i].statnum != kStatMarker)
            {
                if ((sprite[i].cstat & kSpriteStat31) == 0)
                {
                    if ((sprite[i].cstat & kSpriteMask) == kSpriteFace)
                    {
                        tileHist[sprite[i].picnum]++;
                    }
                }
            }
        }
        break;
    case 5:
        for (i = 0; i < kMaxSprites; i++)
        {
            if (sprite[i].statnum != kStatFree && sprite[i].statnum != kStatMarker)
            {
                if ((sprite[i].cstat & kSpriteStat31) == 0)
                {
                    if ((sprite[i].cstat & kSpriteMask) != kSpriteFace)
                    {
                        tileHist[sprite[i].picnum]++;
                    }
                }
            }
        }
        break;
    }
    for (i = 0; i < kMaxTiles; i++)
    {
        tileIndex[i] = i;
    }
    qsort(tileIndex, kMaxTiles, sizeof(short), CompareTileFreqs);
    tileIndexCount = 0;
    while (tileHist[tileIndex[tileIndexCount]] > 0 && tileIndexCount < kMaxTiles)
    {
        tileIndexCount++;
    }
    return tileIndex[0];
}

void tileDrawTileScreen(int a1, int a2, int a3, int a4)
{
    // v18 = a1
    // vc = a2
    // v34 = a3
    // v20 = a4

    int v24;

    int v14;
    int v28;

    int v54;

    unsigned int v3c;

    unsigned int v4c;
    unsigned int v60;

    unsigned int vbp;

    unsigned int v50;
    int v38;

    int v30;
    int v2c;

    int v1c = xdim / a3;
    int v10 = ydim / a3;

    int v48;
    int v44;

    byte *v58;
    byte *v5c;

    char v0[8];

    Video.SetColor(gStdColor[0]);
    clearview(0);
    v24 = 0;
    v30 = 0;
    for (v14 = 0; v14 < v10; v14++, v30 += a3)
    {
        v2c = 0;
        for (v28 = 0; v28 < v1c; v28++, v24++, v2c += a3)
        {
            if (a1 + v24 >= a4)
                break;
            v38 = tileIndex[a1 + v24];
            if (tilesizx[v38] > 0 && tilesizy[v38] > 0)
            {
                int v40 = picsiz[v38] >> 4;
                v40 = 30 - v40;
                setupvlineasm(v40);
                palookupoffse[0] = palookupoffse[1] = palookupoffse[2] = palookupoffse[3] = palookup[0];
                v58 = tileLoadTile(v38);
                v4c = tilesizx[v38];
                v60 = tilesizy[v38];
                v5c = frameplace + ylookup[v30] + v2c;
                if (v4c <= a3 && v60 <= a3)
                {
                    vince[0] = vince[1] = vince[2] = vince[3] = 1<<v40;
                    bufplce[3] = v58 - v60;
                    for (vbp = 0; vbp+3 < v4c; vbp += 4)
                    {
                        bufplce[0] = bufplce[3] + v60;
                        bufplce[1] = bufplce[0] + v60;
                        bufplce[2] = bufplce[1] + v60;
                        bufplce[3] = bufplce[2] + v60;
                        vplce[0] = vplce[1] = vplce[2] = vplce[3] = 0;
                        vlineasm4(v60, v5c);
                        v58 += v60 * 4;
                        v5c += 4;
                    }
                    for (; vbp < v4c; vbp++)
                    {
                        vlineasm1(1<<v40, palookupoffse[0], v60-1, 0, v58, v5c);
                        v58 += v60;
                        v5c++;
                    }
                }
                else
                {
                    if (v4c > v60)
                    {
                        v50 = divscale16(v4c, a3);
                        v3c = divscale(v4c, a3, v40);
                        v48 = (a3 * v60) / v4c;
                        v44 = a3;
                    }
                    else
                    {
                        v50 = divscale16(v60, a3);
                        v3c = divscale(v60, a3, v40);
                        v48 = a3;
                        v44 = (a3 * v4c) / v60;
                    }
                    if (v48 == 0)
                        continue;
                    vince[0] = vince[1] = vince[2] = vince[3] = v3c;
                    vbp = 0;
                    for (v54 = 0; v54+3 < v44; v54 += 4)
                    {
                        bufplce[0] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        bufplce[1] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        bufplce[2] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        bufplce[3] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        vplce[0] = vplce[1] = vplce[2] = vplce[3] = 0;
                        vlineasm4(v48, v5c);
                        v5c += 4;
                    }
                    for (; v54 < v44; v54++)
                    {
                        v58 = waloff[v38] + (vbp >> 16) * v60;
                        vlineasm1(v3c, palookupoffse[0], v48 - 1, 0, v58, v5c);
                        v5c++;
                        vbp += v50;
                    }
                }
                if (a4 < kMaxTiles && !keystatus[0x3a])
                {
                    sprintf(v0, "%d", tileHist[v38]);
                    Video.FillBox(0, v2c + a3 - strlen(v0) * 4 - 2, v30, v2c + a3, v30 + 7);
                    printext256(v2c + a3 - strlen(v0) * 4 - 1, v30, gStdColor[11], -1, v0, 1);
                }
            }
            if (!keystatus[0x3a])
            {
                sprintf(v0, "%d", v38);
                Video.FillBox(0, v2c, v30, v2c + strlen(v0) * 4 + 1, v30 + 7);
                printext256(v2c + 1, v30, gStdColor[14], -1, v0, 1);
            }
        }
    }
    if (IsBlinkOn())
    {
        v54 = a2 - a1;
        v2c = (v54 % v1c) * a3;
        v30 = (v54 / v1c) * a3;
        Video.SetColor(gStdColor[15]);
        Video.HLine(0, v30, v2c, v2c+a3-1);
        Video.HLine(0, v30+a3-1, v2c, v2c+a3-1);
        Video.VLine(0, v2c, v30, v30+a3-1);
        Video.VLine(0, v2c+a3-1, v30, v30+a3-1);
    }
}

int tilePick(int nTile, int a2, int a3)
{
    static int nZoom = 3;
    int i;
    int vbp;
    int vsi;
    int vb;
    byte key;
    vb = pickSize[nZoom];
    int vdi = xdim / vb;
    int v8 = ydim / vb;
    if (a3 != -2)
        tileBuildHistogram(a3);
    if (tileIndexCount == 0)
    {
        tileIndexCount = kMaxTiles;
        for (i = 0; i < kMaxTiles; i++)
        {
            tileIndex[i] = i;
        }
    }
    vbp = 0;
    for (i = 0; i < tileIndexCount; i++)
    {
        if (tileIndex[i] == nTile)
        {
            vbp = i;
            break;
        }
    }
    vsi = ClipLow(((vbp - v8 * vdi + vdi) / vdi) * vdi, 0);
    while (1)
    {
        tileDrawTileScreen(vsi, vbp, pickSize[nZoom], tileIndexCount);
        if (vidoption != 1)
        {
            WaitVBL();
        }
        scrNextPage();
        gFrameTicks = gGameClock - gFrameClock;
        gFrameClock += gFrameTicks;
        UpdateBlinkClock(gFrameTicks);
        key = keyGet();
        switch (key)
        {
        case 0x37:
            if (nZoom > 0)
            {
                nZoom--;
                vb = pickSize[nZoom];
                vdi = xdim / vb;
                v8 = ydim / vb;
                vsi = ClipLow(((vbp - v8 * vdi + vdi) / vdi) * vdi, 0);
            }
            break;
        case 0xb5:
            if ((unsigned int)(nZoom+1) < 6)
            {
                nZoom++;
                vb = pickSize[nZoom];
                vdi = xdim / vb;
                v8 = ydim / vb;
                vsi = ClipLow(((vbp - v8 * vdi + vdi) / vdi) * vdi, 0);
            }
            break;
        case 0xcb:
            if (vbp - 1 >= 0)
                vbp--;
            SetBlinkOn();
            break;
        case 0xcd:
            if (vbp + 1 < tileIndexCount)
                vbp++;
            SetBlinkOn();
            break;
        case 0xc8:
            if (vbp - vdi >= 0)
                vbp -= vdi;
            SetBlinkOn();
            break;
        case 0xd0:
            if (vbp + vdi < tileIndexCount)
                vbp += vdi;
            SetBlinkOn();
            break;
        case 0xc9:
            if (vbp - v8 * vdi >= 0)
            {
                vbp -= v8 * vdi;
                vsi -= v8 * vdi;
                if (vsi < 0)
                    vsi = 0;
            }
            SetBlinkOn();
            break;
        case 0xd1:
            if (vbp + v8 * vdi < tileIndexCount)
            {
                vbp += v8 * vdi;
                vsi += v8 * vdi;
            }
            SetBlinkOn();
            break;
        case 0xc7:
            SetBlinkOn();
            vbp = 0;
            break;
        case 0x2f:
            if (a3 != -2 && tileIndexCount < kMaxTiles)
            {
                vbp = tileIndex[vbp];
                tileIndexCount = kMaxTiles;
                for (i = 0; i < kMaxTiles; i++)
                {
                    tileIndex[i] = i;
                }
            }
            break;
        case 0x22:
            nTile = GetNumberBox("Goto tile", 0, tileIndex[vbp]);
            if (nTile < kMaxTiles && nTile != tileIndex[vbp])
            {
                vbp = nTile;
                tileIndexCount = kMaxTiles;
                for (i = 0; i < kMaxTiles; i++)
                {
                    tileIndex[i] = i;
                }
            }
            break;

        case 0x01:
            clearview(0);
            keystatus[0x01] = 0;
            return a2;
        case 0x1c:
            clearview(0);
            nTile = tileIndex[vbp];
            if (tilesizx[nTile] == 0 || tilesizy[nTile] == 0)
                return a2;
            return nTile;
        }
        while (vbp < vsi)
        {
            vsi -= vdi;
        }
        while (v8 * vdi + vsi <= vbp)
        {
            vsi += vdi;
        }
        if (key != 0)
            keyFlushStream();
    }
}

byte tileGetSurfType(int hit)
{
    int n = hit & 0x1fff;
    switch (hit&0xe000)
    {
    case 0x4000:
        return surfType[sector[n].floorpicnum];
    case 0x6000:
        return surfType[sector[n].ceilingpicnum];
    case 0x8000:
        return surfType[wall[n].picnum];
    case 0xc000:
        return surfType[sprite[n].picnum];
    }
    return 0;
}
