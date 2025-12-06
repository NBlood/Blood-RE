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
#ifndef _TILE_H_
#define _TILE_H_

#include "typedefs.h"
#include "build.h"

enum {
    kSurf0 = 0,
    kSurfMax = 15
};

extern signed char tileShade[];
extern short voxelIndex[];
extern char surfType[kMaxTiles];
extern int tileIndexCount;
extern short tileIndex[kMaxTiles];


byte *tileLoadTile(int);
void tilePreloadTile(int nTile);
void tilePrecacheTile(int nTile);
int tileInit(BOOL a1, char *a2);
void tileTerm(void);
void scrLoadPalette(void);
byte* tileAllocTile(int nTile, int x, int y, int ox = 0, int oy = 0);
byte tileGetSurfType(int hit);
int tilePick(int nTile, int a2, int a3);

#endif // !_TILE_H_
