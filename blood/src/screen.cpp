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
#include "typedefs.h"
#include "globals.h"
#include "build.h"
#include "debug4g.h"
#include "error.h"
#include "gfx.h"
#include "globals.h"
#include "helix.h"
#include "misc.h"
#include "resource.h"
#include "screen.h"
#include "textio.h"

#if APPVER_BLOODREV >= AV_BR_BL120
#define LDIFF1 0
#define LDIFF2 0
#else
#define LDIFF1 -4
#define LDIFF2 -24
#endif

BOOL DacInvalid = TRUE;

struct LOADITEM {
    int id;
    char* name;
};

RGB StdPal[32] = {
    { 0, 0, 0 },
    { 0, 0, 170 },
    { 0, 170, 0 },
    { 0, 170, 170 },
    { 170, 0, 0 },
    { 170, 0, 170 },
    { 170, 85, 0 },
    { 170, 170, 170 },
    { 85, 85, 85 },
    { 85, 85, 255 },
    { 85, 255, 85 },
    { 85, 255, 255 },
    { 255, 85, 85 },
    { 255, 85, 255 },
    { 255, 255, 85 },
    { 255, 255, 255 },
    { 241, 241, 241 },
    { 226, 226, 226 },
    { 211, 211, 211 },
    { 196, 196, 196 },
    { 181, 181, 181 },
    { 166, 166, 166 },
    { 151, 151, 151 },
    { 136, 136, 136 },
    { 120, 120, 120 },
    { 105, 105, 105 },
    { 90, 90, 90 },
    { 75, 75, 75 },
    { 60, 60, 60 },
    { 45, 45, 45 },
    { 30, 30, 30 },
    { 15, 15, 15 }
};

LOADITEM PLU[15] = {
    { 0, "NORMAL" },
    { 1, "SATURATE" },
    { 2, "BEAST" },
    { 3, "TOMMY" },
    { 4, "SPIDER3" },
    { 5, "GRAY" },
    { 6, "GRAYISH" },
    { 7, "SPIDER1" },
    { 8, "SPIDER2" },
    { 9, "FLAME" },
    { 10, "COLD" },
    { 11, "P1" },
    { 12, "P2" },
    { 13, "P3" },
    { 14, "P4" }
};

LOADITEM PAL[5] = {
    { 0, "BLOOD" },
    { 1, "WATER" },
    { 2, "BEAST" },
    { 3, "SEWER" },
    { 4, "INVULN1" }
};

static byte (*gammaTable)[256];
RGB curDAC[256];
RGB baseDAC[256];
static RGB fromDAC[256];
static RGB toRGB;
static RGB* palTable[5];
static long messageTime;
static char message[256];
static int curPalette;
static int curGamma;
int gGammaLevels;
BOOL gFogMode;
byte gStdColor[32];

BYTE scrFindClosestColor(int red, int green, int blue)
{
    int i, r, g, b, sum, dist, best;
    dist = 0x7fffffff;
    for (i = 0; i < 256; i++)
    {
        sum = 0;
        g = palette[i*3+1]-green;
        sum += g * g;
        if (sum >= dist) continue;
        r = palette[i*3+0]-red;
        sum += r * r;
        if (sum >= dist) continue;
        b = palette[i*3+2]-blue;
        sum += b * b;
        if (sum >= dist) continue;
        dist = sum;
        best = i;
        if (sum == 0)
            break;
    }
    return best;
}

void scrCreateStdColors(void)
{
    unsigned int i;
    for (i = 0; i < 32; i++)
        gStdColor[i] = scrFindClosestColor(StdPal[i].red, StdPal[i].green, StdPal[i].blue);
}

void scrLoadPLUs(void)
{
    unsigned int i;
    if (gFogMode)
    {
        DICTNODE *pFog = gSysRes.Lookup("FOG", "FLU");
        if (!pFog)
            ThrowError(182+LDIFF1)("FOG.FLU not found");
        palookup[0] = (byte*)gSysRes.Lock(pFog);
        for (i = 0; i < 15; i++)
            palookup[PLU[i].id] = palookup[0];
        parallaxvisibility = 3072;
        return;
    }
    for (i = 0; i < 15; i++)
    {
        DICTNODE *pPlu = gSysRes.Lookup(PLU[i].name,"PLU");
        if (!pPlu)
            ThrowError(200+LDIFF1)("%s.PLU not found", PLU[i].name);
        if (Resource::Size(pPlu) / 256 != 64)
            ThrowError(203+LDIFF1)("Incorrect PLU size");
        palookup[PLU[i].id] = (byte*)gSysRes.Lock(pPlu);
    }
}

void scrLoadPalette(void)
{
    unsigned int i;
    tioPrint("Loading palettes");
    for (i = 0; i < 5; i++)
    {
        DICTNODE *pPal = gSysRes.Lookup(PAL[i].name, "PAL");
        if (!pPal)
            ThrowError(220+LDIFF1)("%s.PAL not found (RFF files may be wrong version)", PAL[i].name);
        palTable[PAL[i].id] = (RGB*)gSysRes.Lock(pPal);
    }
    memcpy(palette, palTable[0], sizeof(palette));
    numpalookups = 64;
    paletteloaded = TRUE;
    scrLoadPLUs();
    tioPrint("Loading translucency table");
    DICTNODE *pTrans = gSysRes.Lookup("TRANS", "TLU");
    if (!pTrans)
        ThrowError(238+LDIFF1)("TRANS.TLU not found");
    transluc = (byte*)gSysRes.Lock(pTrans);
    fixtransluscence();
}

void scrSetMessage(char *pMessage)
{
    messageTime = gGameClock;
    strcpy(message, pMessage);
}

void scrDisplayMessage(int unk)
{
    if (messageTime + 360 > gGameClock)
        gfxDrawText(windowx1, windowy1, unk, message, NULL);
}

void scrSetPalette(int palId)
{
    curPalette = palId;
    scrSetGamma(curGamma);
}

void scrSetGamma(int nGamma)
{
    dassert(nGamma < gGammaLevels, 270+LDIFF1);
    curGamma = nGamma;
    for (int i = 0; i < 256; i++)
    {
        baseDAC[i].red = gammaTable[nGamma][palTable[curPalette][i].red];
        baseDAC[i].green = gammaTable[nGamma][palTable[curPalette][i].green];
        baseDAC[i].blue = gammaTable[nGamma][palTable[curPalette][i].blue];
    }
    DacInvalid = TRUE;
}

void scrSetupFade(byte red, byte green, byte blue)
{
    memcpy(fromDAC, curDAC, sizeof(fromDAC));
    toRGB.red = red;
    toRGB.green = green;
    toRGB.blue = blue;
}

void scrSetupUnfade(void)
{
    memcpy(fromDAC, baseDAC, sizeof(fromDAC));
}

extern "C" void convertAndDownloadPalette(void* dlpalette);

void scrFadeAmount(int amount)
{
    for (int i = 0; i < 256; i++)
    {
        curDAC[i].red = interpolate16(fromDAC[i].red, toRGB.red, amount);
        curDAC[i].green = interpolate16(fromDAC[i].green, toRGB.green, amount);
        curDAC[i].blue = interpolate16(fromDAC[i].blue, toRGB.blue, amount);
    }
    gSetDACRange(0, 256, (byte*)curDAC);
#ifdef _3DFX
    convertAndDownloadPalette(baseDAC);
#endif
}

void scrSetDac(void)
{
    if (vidoption == 6)
        return;
    if (DacInvalid)
        gSetDACRange(0, 256, (byte*)baseDAC);
#ifdef _3DFX
    convertAndDownloadPalette(baseDAC);
#endif
    DacInvalid = 0;
}

void scrInit(void)
{
    tioPrint("Initializing engine");
    initengine();
    curPalette = 0;
    curGamma = 0;
    tioPrint("Loading gamma correction table");
    DICTNODE *pGamma = gSysRes.Lookup("gamma", "DAT");
    if (!pGamma)
        ThrowError(374+LDIFF2)("Gamma table not found");
    gGammaLevels = Resource::Size(pGamma) / 256;
    gammaTable = (unsigned char(*)[256])gSysRes.Lock(pGamma);
}

void scrSetGameMode(int vidMode, int XRes, int YRes)
{
    if (setgamemode(vidMode, XRes, YRes) == -1)
    {
        vidMode = 2;
        if (setgamemode(vidMode, XRes, YRes) == -1)
            ThrowError(397+LDIFF2)("Standard video mode failed.");
    }
    if (vidMode == 6 && (XRes != 320 || YRes != 200))
        ThrowError(401+LDIFF2)("Specified video mode doesn't support %dx%d resolution!", XRes, YRes);
    switch (vidMode)
    {
    default:
        ThrowError(406+LDIFF2)("Unsupported video mode (%d) passed to scrSetGameMode.", vidMode);
        break;
    case 1:
    case 2:
    case 6:
        if (gFindMode(320,200,256,3) == 0)
        	ThrowError(413+LDIFF2)("Helix driver not found");
        gPageTable[0].begin = (int)frameplace;
        gPageTable[0].bytesPerRow = xdim;
        gPageTable[0].size = xdim*ydim;
        break;
    }
    gfxSetClip(0, 0, XRes, YRes);

    for (int i = 0; i < 1200; i++)
        gYLookup[i] = ylookup[i];

    if (vidoption == 1 || vidoption == 2)
    {
    	gPageTable[0].bytesPerRow = ylookup[1];
    	gPageTable[0].begin = (int)frameplace;
    }
    clearview(0);
    scrNextPage();
    scrSetPalette(curPalette);
}

void scrNextPage(void)
{
    nextpage();
    if (vidoption == 1)
    	gPageTable[0].begin = (int)frameplace;
}
