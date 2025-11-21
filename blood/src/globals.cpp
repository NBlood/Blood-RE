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
#include "typedefs.h"
#include "types.h"
#include "build.h"
#include "debug4g.h"
#include "resource.h"
#include "globals.h"

Resource gSysRes;
int gOldDisplayMode;
long gFrameClock;
int gFrameTicks;
int gFrame;
long volatile gGameClock;
int gCacheMiss;
int gFrameRate;
int32 gGamma;
char *int_148E0C;
char *int_148E10;
int int_148E14;
INPUT_MODE gInputMode;
BOOL gQuitGame;
BOOL gQuitRequest;
BOOL gPaused;
int gNetPlayers;
BOOL gSaveGameActive;
BOOL gSavingGame;
int gSaveGameNum;
BOOL gTenQuit;
BOOL char_148E29;
BOOL gInWindows;
BOOL gUse8250;
char *gVersionString;
char gVersionStringBuf[16];

//char gBuildDate[] = __DATE__;
//char gBuildTime[] = __TIME__;
#if APPVER_BLOODREV >= AV_BR_BL121
char gBuildDate[] = "Jul 15 1998";
char gBuildTime[] = "14:37:44";
const BLOODVERSION gGameVersion = { 21, 1 };
#elif APPVER_BLOODREV >= AV_BR_BL120
char gBuildDate[] = "Mar 18 1998";
# ifdef PLASMAPAK
#  ifdef _3DFX
char gBuildTime[] = "12:21:32";
#  else
char gBuildTime[] = "12:16:41";
#  endif
# else
#  ifdef _3DFX
char gBuildTime[] = "12:11:08";
#  else
char gBuildTime[] = "12:07:19";
#  endif
# endif
const BLOODVERSION gGameVersion = { 20, 1 };
#elif APPVER_BLOODREV >= AV_BR_BL111A
char gBuildDate[] = "Sep 23 1997";
# ifdef REGISTERED
#  ifdef PLASMAPAK
char gBuildTime[] = "13:23:56";
#  else
char gBuildTime[] = "13:20:08";
#  endif
# else
#  ifdef SWRETAIL
char gBuildTime[] = "13:17:24";
#  else
char gBuildTime[] = "13:14:36";
#  endif
# endif
const BLOODVERSION gGameVersion = { 11, 1 };
#elif APPVER_BLOODREV >= AV_BR_BL111
char gBuildDate[] = "Aug 19 1997";
# ifdef REGISTERED
#  ifdef PLASMAPAK
char gBuildTime[] = "11:42:52";
#  else
char gBuildTime[] = "11:39:05";
#  endif
# else
#  ifdef SWRETAIL
char gBuildTime[] = "11:36:21";
#  else
char gBuildTime[] = "11:33:31";
#  endif
# endif
const BLOODVERSION gGameVersion = { 11, 1 };
#else
char gBuildDate[] = "Aug 12 1997";
# ifdef REGISTERED
#  ifdef PLASMAPAK
char gBuildTime[] = "16:01:44";
#  else
char gBuildTime[] = "15:57:55";
#  endif
# else
#  ifdef SWRETAIL
char gBuildTime[] = "15:55:11";
#  else
char gBuildTime[] = "15:52:20";
#  endif
# endif
const BLOODVERSION gGameVersion = { 10, 1 };
#endif

BOOL gAdultContent = 1;

void ClockStrobe(void)
{
    ++gGameClock;
    totalclock = gGameClock;
}

void CLOCK_STROBE_END(void) {}

void LockClockStrobe(void) {}

void UnlockClockStrobe(void) {}

char *GetVersionString(void)
{
    if (!gVersionString)
    {
        gVersionString = gVersionStringBuf;
        if (!gVersionString)
            return NULL;
        sprintf(gVersionString, "%d.%02d", gGameVersion.b.major, gGameVersion.b.minor);
    }
    return gVersionString;
}