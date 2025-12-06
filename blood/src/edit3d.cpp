#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "typedefs.h"
#include "globals.h"
#include "build.h"
#include "bstub.h"
#include "config.h"
#include "db.h"
#include "debug4g.h"
#include "error.h"
#include "gameutil.h"
#include "gfx.h"
#include "gui.h"
#include "key.h"
#include "misc.h"
#include "mouse.h"
#include "screen.h"
#include "sectorfx.h"
#include "sound.h"
#include "tile.h"
#include "trig.h"


short tempang, temptype;
int hvel;

static char buffer[256] = "";
static const char* pzZMode[] = {
    "Gravity",
    "Locked/Step",
    "Locked/Free"
};

static void SetSectorCeilZ(int nSector, int z)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 81);
    z = ClipHigh(z, sector[nSector].floorz);
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        SPRITE* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (getceilzofslope(nSector, pSprite->x, pSprite->y) >= top)
            pSprite->z += z - sector[nSector].ceilingz;
    }
    sector[nSector].ceilingz = z;
}

static void SetSectorFloorZ(int nSector, int z)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 105);
    z = ClipLow(z, sector[nSector].ceilingz);
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        SPRITE* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (getflorzofslope(nSector, pSprite->x, pSprite->y) <= bottom)
            pSprite->z += z - sector[nSector].floorz;
    }
    sector[nSector].floorz = z;
}

static void SetSectorCeilSlope(int nSector, int slope)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 129);
    sector[nSector].ceilingheinum = slope;
    if (sector[nSector].ceilingheinum == 0)
        sector[nSector].ceilingstat &= ~0x02;
    else
        sector[nSector].ceilingstat |= 0x02;
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        SPRITE* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int z = getceilzofslope(nSector, pSprite->x, pSprite->y);
        if (z > top)
            sprite[nSprite].z += z - top;
    }
}

static void SetSectorFloorSlope(int nSector, int slope)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 157);
    sector[nSector].floorheinum = slope;
    if (sector[nSector].floorheinum == 0)
        sector[nSector].floorstat &= ~0x02;
    else
        sector[nSector].floorstat |= 0x02;
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        SPRITE* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int z = getflorzofslope(nSector, pSprite->x, pSprite->y);
        if (z < bottom)
            sprite[nSprite].z += z - bottom;
    }
}

static void SetSectorLightingPhase(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 185);
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        XSECTOR* pXSector = &xsector[nXSector];
        pXSector->at10_0 = a2;
    }
}

static void SetSectorMotionTheta(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 204);
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        XSECTOR* pXSector = &xsector[nXSector];
        pXSector->at38_0 = a2;
    }
}

static char IsSectorHighlight(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 223);
    for (int i = 0; i < highlightsectorcnt; i++)
    {
        if (highlightsector[i] == nSector)
            return 1;
    }
    return 0;
}

inline int GetWallZPeg(int nWall)
{
    int z;
    dassert(nWall >= 0 && nWall < kMaxWalls, 248);
    int nSector = sectorofwall(nWall);
    dassert(nSector >= 0 && nSector < kMaxSectors, 252);
    int nNextSector = wall[nWall].nextsector;
    if (nNextSector == -1)
    {
        if (wall[nWall].cstat & kWallStat2)
            z = sector[nSector].floorz;
        else
            z = sector[nSector].ceilingz;
    }
    else
    {
        if (wall[nWall].cstat & kWallStat2)
            z = sector[nSector].ceilingz;
        else
        {
            if (sector[nNextSector].ceilingz > sector[nSector].ceilingz)
                z = sector[nNextSector].ceilingz;
            if (sector[nNextSector].floorz < sector[nSector].floorz)
                z = sector[nNextSector].floorz;
        }
    }
    return z;
}

static void AlignWalls(int nWall0, int z0, int nWall1, int z1, int nTile)
{
    dassert(nWall0 >= 0 && nWall0 < kMaxWalls, 285);
    dassert(nWall1 >= 0 && nWall1 < kMaxWalls, 286);
    wall[nWall1].cstat &= ~0x108;
    wall[nWall1].xpanning = (wall[nWall0].xpanning+(wall[nWall0].xrepeat<<3))%tilesizx[nTile];
    z1 = GetWallZPeg(nWall1);

    int n = picsiz[nTile]>>4;
    if ((1<<n) != tilesizy[nTile])
        n++;

    wall[nWall1].yrepeat = wall[nWall0].yrepeat;
    wall[nWall1].ypanning = wall[nWall0].ypanning + (((z1-z0)*wall[nWall0].yrepeat) >> (n+3));
}

char visited[kMaxWalls];

static void AutoAlignWalls(int nWall0, int ply)
{
    int z0, z1, nTile, nWall1;
    dassert(nWall0 >= 0 && nWall0 < kMaxWalls, 310);
    nTile = wall[nWall0].picnum;

    if (ply == 64)
        return;

    if (ply == 0)
    {
        memset(visited, 0, sizeof(visited));
        visited[nWall0] = 1;
    }

    z0 = GetWallZPeg(nWall0);

    nWall1 = wall[nWall0].point2;

    dassert(nWall1 >= 0 && nWall1 < kMaxWalls, 338);

    while (1)
    {
        if (visited[nWall1])
            break;

        visited[nWall1] = 1;

        if (wall[nWall1].nextwall == nWall0)
            break;

        if (wall[nWall1].picnum == nTile)
        {
            z1 = GetWallZPeg(nWall1);

            char visible = 0;

            int nNextSector = wall[nWall1].nextsector;
            if (nNextSector < 0)
                visible = 1;
            else
            {
                int nSector = wall[wall[nWall1].nextwall].nextsector;
                if (getceilzofslope(nSector, wall[nWall1].x, wall[nWall1].y) < getceilzofslope(nNextSector, wall[nWall1].x, wall[nWall1].y))
                    visible = 1;
                if (getflorzofslope(nSector, wall[nWall1].x, wall[nWall1].y) > getflorzofslope(nNextSector, wall[nWall1].x, wall[nWall1].y))
                    visible = 1;
            }
            if (visible)
            {
                AlignWalls(nWall0, z0, nWall1, z1, nTile);

                int nNextWall = wall[nWall1].nextwall;
                if (nNextWall < 0)
                {
                    nWall0 = nWall1;
                    z0 = GetWallZPeg(nWall0);
                    nWall1 = wall[nWall0].point2;
                    continue;
                }
                if ((wall[nWall1].cstat & kWallStat1) && wall[nNextWall].picnum == nTile)
                    AlignWalls(nWall0, z0, nNextWall, z1, nTile);
                AutoAlignWalls(nWall1, ply + 1);
            }
        }

        if (wall[nWall1].nextwall < 0)
            break;

        nWall1 = wall[wall[nWall1].nextwall].point2;
    }
}

static void func_216F8(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 421);
    visited[nSector] = 1;
    for (int i = 0; i < sector[nSector].wallnum; i++)
    {
        int nNextSector = wall[sector[nSector].wallptr+i].nextsector;
        if (nNextSector == -1)
            continue;
        if (IsSectorHighlight(nNextSector) && !visited[nNextSector])
        {
            SetSectorFloorZ(nNextSector, sector[nSector].floorz - (a2<<8));
            func_216F8(nNextSector, a2);
        }
    }
}

static void func_21798(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 445);
    visited[nSector] = 1;
    for (int i = 0; i < sector[nSector].wallnum; i++)
    {
        int nNextSector = wall[sector[nSector].wallptr+i].nextsector;
        if (nNextSector == -1)
            continue;
        if (IsSectorHighlight(nNextSector) && !visited[nNextSector])
        {
            SetSectorCeilZ(nNextSector, sector[nSector].ceilingz - (a2<<8));
            func_21798(nNextSector, a2);
        }
    }
}

unsigned short WallShadeFrac[kMaxWalls];
unsigned short FloorShadeFrac[kMaxSectors];
unsigned short CeilShadeFrac[kMaxSectors];
int WallArea[kMaxWalls];
int int_149DE8[kMaxSectors];
int int_14ADE8[kMaxWalls];
int int_152DE8[kMaxWalls];
int int_15ADE8[kMaxSectors];
int int_15BDE8[kMaxSectors];
int int_15CDE8[kMaxSectors];
int int_15DDE8[kMaxSectors];
int int_15EDE8[kMaxSectors];
int int_15FDE8[kMaxSectors];


static void CalcLightBomb(int x, int y, int z, short nSector, int dx, int dy, int dz, int a8, int a9, int a10)
{
    short nHitSect, nHitWall, nHitSprite;
    int hx, hy, hz;
    int dx_;
    int dy_;
    int dz_;
    int dot;
    int t2;
    nHitSect = -1;
    nHitWall = -1;
    nHitSprite = -1;
    hitscan(x, y, z, nSector, dx, dy, dz << 4, &nHitSect, &nHitWall, &nHitSprite, &hx, &hy, &hz, 0);
    dx_ = klabs(x - hx)>>4;
    dy_ = klabs(y - hy)>>4;
    dz_ = klabs(z - hz)>>8;
    a10 += ksqrt(dx_*dx_+dy_*dy_+dz_*dz_);
    if (nHitWall >= 0)
    {
        t2 = divscale16(a8, ClipLow(a10 + gLightBombRampDist, 1));
        if (t2 <= 0)
            return;
        t2 = divscale16(t2, ClipLow(WallArea[nHitWall], 1));
        int shade = (wall[nHitWall].shade<<16)|WallShadeFrac[nHitWall];
        shade -= t2;
        wall[nHitWall].shade = ClipLow(shade>>16, gLightBombMaxBright);
        WallShadeFrac[nHitWall] = shade & 0xffff;
        if (a9 < gLightBombReflections)
        {
            int wx = int_14ADE8[nHitWall];
            int wy = int_152DE8[nHitWall];
            dot = dmulscale16(dx, wx, dy, wy);
            if (dot < 0)
                return;
            dx -= mulscale16(dot*2, wx);
            dy -= mulscale16(dot*2, wy);
            hx += dx >> 12;
            hy += dy >> 12;
            hz += dz >> 8;
            a8 -= mulscale16(a8, gLightBombAttenuation);
            CalcLightBomb(hx, hy, hz, nHitSect, dx, dy, dz, a8, a9+1, a10);
        }
    }
    else if (nHitSprite >= 0)
    {

    }
    else if (dz > 0)
    {
        t2 = divscale16(a8, ClipLow(a10 + gLightBombRampDist, 1));
        if (t2 <= 0)
            return;
        t2 = divscale16(t2, ClipLow(int_149DE8[nHitSect], 1));
        int shade = (sector[nHitSect].floorshade<<16)|FloorShadeFrac[nHitSect];
        shade -= t2;
        sector[nHitSect].floorshade = ClipLow(shade>>16, gLightBombMaxBright);
        FloorShadeFrac[nHitSect] = shade & 0xffff;
        if (a9 < gLightBombReflections)
        {
            if (sector[nHitSect].floorstat & kSectorStat1)
            {
                int wx = int_15ADE8[nHitSect];
                int wy = int_15BDE8[nHitSect];
                int wz = int_15CDE8[nHitSect];
                dot = tmulscale16(dx, wx, dy, wy, dz, wz);
                if (dot < 0)
                    return;

                dx -= mulscale16(dot*2, wx);
                dy -= mulscale16(dot*2, wy);
                dz -= mulscale16(dot*2, wz);
            }
            else
                dz = -dz;
            a8 -= mulscale16(a8, gLightBombAttenuation);
            hx += dx >> 12;
            hy += dy >> 12;
            hz += dz >> 8;
            CalcLightBomb(hx, hy, hz, nHitSect, dx, dy, dz, a8, a9+1, a10);
        }
    }
    else
    {
        t2 = divscale16(a8, ClipLow(a10 + gLightBombRampDist, 1));
        if (t2 <= 0)
            return;
        t2 = divscale16(t2, ClipLow(int_149DE8[nHitSect], 1));
        int shade = (sector[nHitSect].ceilingshade <<16)|CeilShadeFrac[nHitSect];
        shade -= t2;
        sector[nHitSect].ceilingshade = ClipLow(shade>>16, gLightBombMaxBright);
        CeilShadeFrac[nHitSect] = shade & 0xffff;
        if (a9 < gLightBombReflections)
        {
            if (sector[nHitSect].ceilingstat & kSectorStat1)
            {
                int wx = int_15DDE8[nHitSect];
                int wy = int_15EDE8[nHitSect];
                int wz = int_15FDE8[nHitSect];
                dot = tmulscale16(dx, wx, dy, wy, dz, wz);
                if (dot < 0)
                    return;

                dx -= mulscale16(dot*2, wx);
                dy -= mulscale16(dot*2, wy);
                dz -= mulscale16(dot*2, wz);
            }
            else
                dz = -dz;
            a8 -= mulscale16(a8, gLightBombAttenuation);
            hx += dx >> 12;
            hy += dy >> 12;
            hz += dz >> 8;
            CalcLightBomb(hx, hy, hz, nHitSect, dx, dy, dz, a8, a9+1, a10);
        }
    }
}

static int CalcSectorArea(SECTOR* pSector)
{
    int nArea = 0;
    int nStartWall = pSector->wallptr;
    int nEndWall = nStartWall + pSector->wallnum;
    for (int i = nStartWall; i < nEndWall; i++)
    {
        int x0 = wall[i].x >> 4;
        int y0 = wall[i].y >> 4;
        int x1 = wall[wall[i].point2].x >> 4;
        int y1 = wall[wall[i].point2].y >> 4;

        nArea += (x0+x1)*(y1-y0);
    }

    nArea >>= 1;
    return nArea;
}

static void ResetLightBomb()
{
    SECTOR* pSector;
    WALL* pWall;
    int i, j;
    pSector = sector;
    for (i = 0; i < numsectors; i++, pSector++)
    {
        FloorShadeFrac[i] = 0;
        CeilShadeFrac[i] = 0;
        int_149DE8[i] = CalcSectorArea(pSector);
        pWall = &wall[pSector->wallptr];
        for (j = pSector->wallptr; j < pSector->wallptr+pSector->wallnum; j++, pWall++)
        {
            WallShadeFrac[j] = 0;
            
            int nx = (wall[pWall->point2].y-pWall->y)>>4;
            int ny = (-(wall[pWall->point2].x-pWall->x))>>4;
            int nLength = ksqrt(nx*nx+ny*ny);
            int_14ADE8[j] = divscale16(nx, nLength);
            int_152DE8[j] = divscale16(ny, nLength);
            int ceil,ceilz0, ceilz1, flor, florz0, florz1;
            getzsofslope(i, pWall->x, pWall->y, &ceilz0, &florz0);
            getzsofslope(i, wall[pWall->point2].x, wall[pWall->point2].y, &ceilz1, &florz1);
            ceil = (ceilz0+ceilz1)>>1;
            flor = (florz0+florz1)>>1;
            int height = flor - ceil;
            if (pWall->nextsector >= 0)
            {
                height = 0;
                int ceiln, v28, v24, florn, v20, v1c;
                getzsofslope(pWall->nextsector, pWall->x, pWall->y, &v28, &v20);
                getzsofslope(pWall->nextsector, wall[pWall->point2].x, wall[pWall->point2].y, &v24, &v1c);
                ceiln = (v28+v24)>>1;
                florn = (v20+v1c)>>1;
                if (florn < flor)
                    height += flor-florn;
                if (ceiln > ceil)
                    height += ceiln-ceil;
            }
            WallArea[j] = (nLength*height)>>8;
        }

        int_15ADE8[i] = 0;
        int_15BDE8[i] = 0;
        int_15CDE8[i] = -0x10000;
        int_15DDE8[i] = 0;
        int_15EDE8[i] = 0;
        int_15FDE8[i] = 0x10000;
    }
}

static void DoLightBomb(int x, int y, int z, short nSector)
{
    int dx, dy, dz;
    for (int i = 171; i <= 853; i += 85)
    {
        for (int j = 0; j < 2048; j += 8)
        {
            dx = mulscale30(Cos(j), Sin(i)) >> 16;
            dy = mulscale30(Sin(j), Sin(i)) >> 16;
            dz = Cos(i) >> 16;

            CalcLightBomb(x, y, z, nSector, dx, dy, dz, gLightBombIntensity, 0, 0);
        }
    }
}

static void SetFirstWall(int nSector, int nWall)
{
    int start;
    int length;
    int n;
    int k;
    int t;
    int j;
    WALL twall;
    start = sector[nSector].wallptr;
    length = sector[nSector].wallnum;
    dassert(nWall >= start && nWall < start + length, 726);
    n = nWall - start;
    if (!n)
        return;
    k = j = start;
    for (int i = length; i > 0; i--)
    {
        if (k == j)
            twall = wall[k];

        t = k+n;
        while (t >= start + length)
            t -= length;
        if (t == j)
        {
            wall[k] = twall;
            k = ++j;
        }
        else
        {
            wall[k] = wall[t];
            k = t;
        }
    }
    for (k = start; k < start+length; k++)
    {
        wall[k].point2 -= n;
        if (wall[k].point2 < start)
            wall[k].point2 += length;
        if (wall[k].nextwall >= 0)
            wall[wall[k].nextwall].nextwall = k;
    }
    func_1058C();
}

static void func_22350(void)
{
    short nHitSect = -1;
    short nHitWall = -1;
    short nHitSprite = -1;
    int hx, hy, hz;
    long x = 0x4000;
    long y = divscale(searchx-xdim/2, xdim/2, 14);
    RotateVector(&x, &y, ang);
    hitscan(posx, posy, posz, cursectnum, x, y, (searchy-horiz)*2000, &nHitSect, &nHitWall, &nHitSprite, &hx, &hy, &hz, 0);
    if (nHitWall == searchwall)
        searchsector = wall[searchwall].nextsector;
    else if (nHitWall == wall[searchwall].nextwall)
        searchsector = wall[nHitWall].nextsector;
    else
        return;
    if (getflorzofslope(searchsector, hx, hy) < hz)
        searchstat = 2;
    else
        searchstat = 1;
}

static int InsertMisc(int a1, int a2, int a3, int a4, int a5, int a6)
{
    static short short_CD07C[] = {
        1072, 1074, 1076, 1070, 1078, 1048, 1046, 1127, 796,
        266, 1069, 1142, 462, 739, 642, 2522, 2523, 2524, 2525,
        2526, 2527, 2528, 2529, 650
    };
    short type;
    short v4 = 0;
    tileIndexCount = 24;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = short_CD07C[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    switch (vc)
    {
    case 650:
        type = 32;
        break;
    case 1046:
    case 1048:
    case 1070:
    case 1072:
    case 1074:
    case 1076:
    case 1078:
        type = 20;
        break;
    case 1127:
        type = 408;
        break;
    case 796:
        type = 407;
        break;
    case 266:
        type = 406;
        break;
    case 1069:
        type = 410;
        break;
    case 1142:
        type = 409;
        break;
    case 462:
        type = 405;
        break;
    case 739:
        type = 403;
        break;
    case 642:
        type = 404;
        break;
    // case 642:
    //     type = 404;
    //     break;
    case 2522:
    case 2523:
    case 2524:
    case 2525:
    case 2526:
    case 2527:
    case 2528:
    case 2529:
        type = 1;
        break;
    }
    int nSprite = InsertSprite(a2, v4);
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 909);
    sprite[nSprite].type = type;
    sprite[nSprite].picnum = vc;
    switch (vc)
    {
    case 1046:
    case 1048:
    case 1070:
    case 1072:
    case 1074:
    case 1076:
    case 1078:
        sprite[nSprite].xrepeat = sprite[nSprite].yrepeat = 48;
        break;
    case 2522:
    case 2523:
    case 2524:
    case 2525:
    case 2526:
    case 2527:
    case 2528:
    case 2529:
    {
        int nXSprite = func_10E50(nSprite);
        dassert(nXSprite > 0 && nXSprite < kMaxXSprites, 935);
        xsprite[nXSprite].at10_0 = vc - 2522;
        break;
    }
    }

    func_10EA0();
    SPRITE* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

static int InsertHazard(int a1, int a2, int a3, int a4, int a5, int a6)
{
    static short short_CD0AC[9] = {
        655, 3444, 907, 968, 1080, 835, 1156, 2178, 908
    };
    short type;
    short v4 = 0;
    tileIndexCount = 9;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = short_CD0AC[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    switch (vc)
    {
    case 655:
        type = 454;
        break;
    case 3444:
        type = 401;
        break;
    case 907:
        type = 400;
        break;
    case 968:
        type = 450;
        break;
    case 1080:
        type = 457;
        break;
    case 835:
        type = 458;
        break;
    case 1156:
        type = 456;
        break;
    case 2178:
        type = 413;
        break;
    case 908:
        type = 459;
        break;
    default:
        scrSetMessage("Invalid hazard tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, v4);
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1039);
    sprite[nSprite].type = type;

    func_10EA0();
    SPRITE* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

static int InsertItem(int a1, int a2, int a3, int a4, int a5, int a6)
{
    static short short_CD0BE[] = {
        2552, 2553, 2554, 2555, 2556, 2557, 519, 2169, 2433,
        896, 825, 827, 829, 830, 760, 2428, 839, 840, 841, 842,
        843, 518, 522, 523, 837, 2628, 2586, 2578, 2602, 2594
    };
    short type;
    tileIndexCount = 30;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = short_CD0BE[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    switch (vc)
    {
    case 2552:
        type = 100;
        break;
    case 2553:
        type = 101;
        break;
    case 2554:
        type = 102;
        break;
    case 2555:
        type = 103;
        break;
    case 2556:
        type = 104;
        break;
    case 2557:
        type = 105;
        break;
    case 519:
        type = 107;
        break;
    case 822:
        type = 108;
        break;
    case 2169:
        type = 109;
        break;
    case 2433:
        type = 110;
        break;
    case 517:
        type = 111;
        break;
    case 783:
        type = 112;
        break;
    case 896:
        type = 113;
        break;
    case 825:
        type = 114;
        break;
    case 827:
        type = 115;
        break;
    case 828:
        type = 116;
        break;
    case 829:
        type = 117;
        break;
    case 830:
        type = 118;
        break;
    case 831:
        type = 119;
        break;
    case 863:
        type = 120;
        break;
    //case 863:
    //    type = 120;
    //    break;
    case 760:
        type = 121;
        break;
    case 836:
        type = 122;
        break;
    case 851:
        type = 123;
        break;
    case 2428:
        type = 124;
        break;
    case 839:
        type = 125;
        break;
    case 768:
        type = 126;
        break;
    case 840:
        type = 127;
        break;
    case 841:
        type = 128;
        break;
    case 842:
        type = 129;
        break;
    case 843:
        type = 130;
        break;
    //case 843:
    //    type = 130;
    //    break;
    case 683:
        type = 131;
        break;
    case 521:
        type = 132;
        break;
    case 604:
        type = 133;
        break;
    //case 604:
    //    type = 133;
    //    break;
    case 520:
        type = 134;
        break;
    case 803:
        type = 135;
        break;
    case 518:
        type = 136;
        break;
    case 522:
        type = 137;
        break;
    case 523:
        type = 138;
        break;
    case 837:
        type = 139;
        break;
    case 2628:
        type = 140;
        break;
    case 2586:
        type = 141;
        break;
    case 2578:
        type = 142;
        break;
    case 2602:
        type = 143;
        break;
    case 2594:
        type = 144;
        break;
    default:
        scrSetMessage("Invalid item tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 3);
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1277);
    sprite[nSprite].type = type;
    func_10EA0();
    SPRITE* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

static int InsertAmmo(int a1, int a2, int a3, int a4, int a5, int a6)
{
    static short short_CD0FA[] = {
        548, 820, 589, 618, 619, 809, 810, 811, 812, 813, 817, 816, 801, 525
    };
    short type;
    tileIndexCount = 14;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = short_CD0FA[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    switch (vc)
    {
    case 820:
        type = 66;
        break;
    case 548:
        type = 73;
        break;
    case 589:
        type = 62;
        break;
    case 618:
        type = 60;
        break;
    case 619:
        type = 67;
        break;
    case 809:
        type = 63;
        break;
    //case 809:
    //    type = 63;
    //    break;
    case 810:
        type = 65;
        break;
    case 811:
        type = 64;
        break;
    case 812:
        type = 68;
        break;
    case 813:
        type = 69;
        break;
    case 817:
        type = 72;
        break;
    case 816:
        type = 76;
        break;
    case 801:
        type = 79;
        break;
    case 525:
        type = 70;
        break;
    default:
        scrSetMessage("Invalid ammo tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 3);
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1395);
    sprite[nSprite].type = type;
    func_10EA0();
    SPRITE* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

static int InsertWeapon(int a1, int a2, int a3, int a4, int a5, int a6)
{
    static short short_CD116[9] = {
        524, 559, 558, 526, 589, 618, 539, 800, 525
    };
    short type;
    tileIndexCount = 9;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = short_CD116[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    switch (vc)
    {
    case 524:
        type = 43;
        break;
    case 559:
        type = 41;
        break;
    case 558:
        type = 42;
        break;
    case 526:
        type = 46;
        break;
    case 589:
        type = 62;
        break;
    case 618:
        type = 60;
        break;
    case 539:
        type = 45;
        break;
    case 800:
        type = 50;
        break;
    case 525:
        type = 70;
        break;
    default:
        scrSetMessage("Invalid weapon tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 3);
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1494);
    sprite[nSprite].type = type;
    func_10EA0();
    SPRITE* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

static int InsertEnemy(int a1, int a2, int a3, int a4, int a5, int a6)
{
    static short short_CD128[] = {
        2860, 2825, 3385, 1170, 3054, 1370, 1209, 1470, 1530,
        3060, 1270, 1980, 1920, 1925, 1930, 1935, 1570, 1870,
        1950, 1745,
#ifdef PLASMAPAK
        1792, 1797,
#endif
        2680, 3140, 3798,
#ifdef PLASMAPAK
        3870, 2960
#endif
    };
    short type;
    tileIndexCount = sizeof(short_CD128)/sizeof(short_CD128[0]);
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = short_CD128[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    switch (vc)
    {
    case 2860:
        type = 201;
        break;
    case 3385:
        type = 230;
        break;
    case 2825:
        type = 202;
        break;
    case 1170:
        type = 203;
        break;
    case 1209:
        type = 244;
        break;
    case 3054:
        type = 205;
        break;
    case 1370:
        type = 204;
        break;
    case 1470:
        type = 206;
        break;
    case 1530:
        type = 208;
        break;
    case 3060:
        type = 210;
        break;
    case 1270:
        type = 211;
        break;
    case 1980:
        type = 212;
        break;
    case 1920:
        type = 213;
        break;
    case 1925:
        type = 214;
        break;
#ifdef PLASMAPAK
    case 1930:
        type = 215;
        break;
#endif
    case 1935:
        type = 216;
        break;
    //case 1935:
    //    type = 216;
    //    break;
    case 1570:
        type = 217;
        break;
    case 1870:
        type = 218;
        break;
    case 1950:
        type = 219;
        break;
    case 1745:
        type = 220;
        break;
    case 1792:
        type = 221;
        break;
    case 1797:
        type = 222;
        break;
    case 2680:
        type = 227;
        break;
    case 832:
        type = 200;
        break;
    case 3140:
        type = 229;
        break;
    case 3798:
        type = 245;
        break;
#ifdef PLASMAPAK
    case 3870:
        type = 250;
        break;
    case 2960:
        type = 251;
        break;
#endif
    default:
        scrSetMessage("Invalid dude tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 6);
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1664);
    sprite[nSprite].type = type;
    func_10EA0();
    SPRITE* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    int nXSprite = func_10E50(nSprite);
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites, 1687);
    switch (type)
    {
    case 213:
    case 214:
    case 215:
    case 216:
        if (a1 == 1 && !(sector[a2].ceilingstat & kSectorStat0))
            pSprite->cstat |= 0x08;
        break;
    case 219:
        if (a1 == 1 && !(sector[a2].ceilingstat & kSectorStat0))
            pSprite->picnum = 1948;
        else
            xsprite[nXSprite].at1_6 = 1;
        break;
    }
    ModifyBeep();
    return nSprite;
}

static int InsertObject(int a1, int a2, int a3, int a4, int a5, int a6)
{
    Window window(0, 0, 80, 182, "Insert");

    TextButton* pEnemy = new TextButton(4, 4, 60, 20, "&Enemy", (MODAL_RESULT)8);
    TextButton* pWeapon = new TextButton(4, 26, 60, 20, "&Weapon", (MODAL_RESULT)9);
    TextButton* pAmmo = new TextButton(4, 48, 60, 20, "&Ammo", (MODAL_RESULT)10);
    TextButton* pItem = new TextButton(4, 70, 60, 20, "&Item", (MODAL_RESULT)11);
    TextButton* pHazard = new TextButton(4, 92, 60, 20, "&Hazard", (MODAL_RESULT)12);
    TextButton* pMisc = new TextButton(4, 114, 60, 20, "&Misc", (MODAL_RESULT)13);
    TextButton* pCancel = new TextButton(4, 136, 60, 20, "&Cancel", MODAL_RESULT_2);

    window.at5e->Insert(pEnemy);
    window.at5e->Insert(pWeapon);
    window.at5e->Insert(pAmmo);
    window.at5e->Insert(pItem);
    window.at5e->Insert(pHazard);
    window.at5e->Insert(pMisc);
    window.at5e->Insert(pCancel);

    ShowModal(&window);

    switch (window.at25)
    {
    case 8:
        return InsertEnemy(a1, a2, a3, a4, a5, (a6+1024)&2047);
    case 9:
        return InsertWeapon(a1, a2, a3, a4, a5, 0);
    case 10:
        return InsertAmmo(a1, a2, a3, a4, a5, 0);
    case 11:
        return InsertItem(a1, a2, a3, a4, a5, 0);
    case 12:
        return InsertHazard(a1, a2, a3, a4, a5, a6);
    case 13:
        return InsertMisc(a1, a2, a3, a4, a5, a6);
    case MODAL_RESULT_2:
        return -1;
    }
    return -1;
}

static unsigned char CompareXSectors(XSECTOR *pXSector1, XSECTOR *pXSector2)
{
    return memcmp(pXSector1, pXSector2, sizeof(XSECTOR)) == 0;
}

static unsigned char ConfirmYesNo(char *a1)
{
    Window window(59, 80, 202, 46, a1);

    TextButton* pYes = new TextButton(4, 4, 60, 20, "&Yes", (MODAL_RESULT)6);
    window.at5e->Insert(pYes);

    TextButton* pNo = new TextButton(68, 4, 60, 20, "&No", (MODAL_RESULT)7);
    window.at5e->Insert(pNo);

    ShowModal(&window);

    return window.at25 == (MODAL_RESULT)6 ? 1 : 0;
}

static unsigned char ShowOptions(void)
{
    Window window(0, 0, 80, 182, "Options");

    TextButton* pClean = new TextButton(4, 4, 60, 20, "C&lean", (MODAL_RESULT)8);

    TextButton* pCancel = new TextButton(4, 136, 60, 20, "&Cancel", MODAL_RESULT_2);
    window.at5e->Insert(pClean);
    window.at5e->Insert(pCancel);

    ShowModal(&window);

    switch (window.at25)
    {
    case 8:
    {
        XSECTOR v64;
        if (somethingintab == 2)
        {
            if (sector[searchsector].extra >= 0)
            {
                if (xsector[sector[searchsector].extra].reference == searchsector)
                    memcpy(&v64, &xsector[sector[searchsector].extra], sizeof(XSECTOR));
            }
        }
        int vdi = 1;
        for (int i = 0; i < kMaxXSectors; i++)
        {
            int nSector = xsector[i].reference;
            if (nSector != -1 && sector[nSector].extra == i && nSector != searchsector)
            {
                v64.reference = xsector[i].reference;
                if (CompareXSectors(&v64, &xsector[i]))
                    vdi++;
            }
        }
        char buffer[40];
        sprintf(buffer, "Clean %i common sectors?", vdi);
        if (ConfirmYesNo(buffer))
        {
            for (int i = 0; i < kMaxXSectors; i++)
            {
                int nSector = xsector[i].reference;
                if (nSector != -1 && sector[nSector].extra == i)
                {
                    v64.reference = xsector[i].reference;
                    if (CompareXSectors(&v64, &xsector[i]))
                        dbDeleteXSector(i);
                }
            }
        }
        InitSectorFX();
        break;
    }
    case MODAL_RESULT_2:
        return 0;
    }
    return 1;
}

static void func_258B0(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 1901);
    visited[nSector] = 1;
    for (int i = 0; i < sector[nSector].wallnum; i++)
    {
        int nNextSector = wall[sector[nSector].wallptr + i].nextsector;
        if (nNextSector != -1 && IsSectorHighlight(nNextSector) && !visited[nNextSector])
        {
            int nXSector = sector[nSector].extra;
            if (nXSector > 0)
            {
                dassert(nXSector < kMaxXSectors, 1916);
                XSECTOR* pXSector = &xsector[nXSector];
                SetSectorLightingPhase(nNextSector, (pXSector->at10_0 + a2) & 2047);
                func_258B0(nNextSector, a2);
            }
        }
    }
}

static void func_259F0(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 1938);
    visited[nSector] = 1;
    for (int i = 0; i < sector[nSector].wallnum; i++)
    {
        int nNextSector = wall[sector[nSector].wallptr + i].nextsector;
        if (nNextSector != -1 && IsSectorHighlight(nNextSector) && !visited[nNextSector])
        {
            int nXSector = sector[nSector].extra;
            if (nXSector > 0)
            {
                dassert(nXSector < kMaxXSectors, 1953);
                XSECTOR* pXSector = &xsector[nXSector];
                SetSectorMotionTheta(nNextSector, (pXSector->at38_0 + a2) & 2047);
                func_259F0(nNextSector, a2);
            }
        }
    }
}

static char ShowSectorTricks(void)
{
    char v4 = 0;
    Window window(0, 0, 180, 182, "Sector Tricks");

    Label* pValue = new Label(4, 8, "&Value:");
    window.at5e->Insert(pValue);

    EditNumber* pEdit = new EditNumber(44, 4, 80, 16, 0);
    pEdit->at1f = 'V';
    window.at5e->Insert(pEdit);

    TextButton* pSH = new TextButton(4, 24, 80, 20, "&Step Height", (MODAL_RESULT)8);
    window.at5e->Insert(pSH);

    TextButton* pLH = new TextButton(4, 44, 80, 20, "&Light Phase", (MODAL_RESULT)9);
    window.at5e->Insert(pLH);

    TextButton* pZP = new TextButton(4, 64, 80, 20, "&Z Phase", (MODAL_RESULT)10);
    window.at5e->Insert(pZP);

    ShowModal(&window);

    switch (window.at25)
    {
    case 8:
        switch (searchstat)
        {
        case 2:
            memset(visited, 0, sizeof(visited));
            func_216F8(searchsector, pEdit->at130);
            v4 = 1;
            break;
        case 1:
            memset(visited, 0, sizeof(visited));
            func_21798(searchsector, pEdit->at130);
            v4 = 1;
            break;
        }
        break;
    case 9:
        memset(visited, 0, sizeof(visited));
        func_258B0(searchsector, pEdit->at130);
        v4 = 1;
        break;
    case 10:
        memset(visited, 0, sizeof(visited));
        func_259F0(searchsector, pEdit->at130);
        v4 = 1;
        break;
    }
    return v4;
}

static void SetSkyTile(int nTile)
{
    dassert(nTile >= 0 && nTile < kMaxTiles, 2052);

    pskybits = 10 - (picsiz[nTile] & 15);
    gSkyCount = 1 << pskybits;

    sector[searchsector].ceilingpicnum = nTile;

    for (short i = 0; i < gSkyCount; i++)
        pskyoff[i] = i;

    for (i = 0; i < numsectors; i++)
    {
        if (sector[i].ceilingstat & kSectorStat0)
            sector[i].ceilingpicnum = nTile;
        if (sector[i].floorstat & kSectorStat0)
            sector[i].floorpicnum = nTile;
    }
}

static void func_25E64(SPRITE* pSprite, int a2)
{
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - top;
}

static void func_25EF8(SPRITE* pSprite, int a2)
{
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - bottom;
}

static void func_25F8C(SPRITE* pSprite, int a2)
{
    pSprite->z = DecStep(pSprite->z, a2);
}

static void func_25F9C(SPRITE* pSprite, int a2)
{
    pSprite->z = IncStep(pSprite->z, a2);
}

static void OperateSprites(void (*a1)(SPRITE *, int), int a2)
{
    if (TestBitString(show2dsprite, searchwall))
    {
        for (int i = 0; i < highlightcnt; i++)
        {
            if ((highlight[i] & 0xc000) == 0x4000)
            {
                a1(&sprite[highlight[i]&0x3fff], a2);
            }
        }
    }
    else
        a1(&sprite[searchwall], a2);
}

static void func_26054(int nSector, int a2)
{
    SetSectorCeilZ(nSector, sector[nSector].ceilingz + a2);
}

static void func_26074(int nSector, int a2)
{
    SetSectorFloorZ(nSector, sector[nSector].floorz + a2);
}

static void func_26094(int nSector, int a2)
{
    SetSectorCeilZ(nSector, DecStep(sector[nSector].ceilingz, a2));
}

static void func_260B8(int nSector, int a2)
{
    SetSectorFloorZ(nSector, DecStep(sector[nSector].floorz, a2));
}

static void func_260DC(int nSector, int a2)
{
    SetSectorCeilZ(nSector, IncStep(sector[nSector].ceilingz, a2));
}

static void func_260FC(int nSector, int a2)
{
    SetSectorFloorZ(nSector, IncStep(sector[nSector].floorz, a2));
}

static void func_2611C(int nSector, int a2)
{
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        xsector[nXSector].at11_0 = a2;
    }
}

static void OperateSectors(void (*a1)(int, int), int a2)
{
    if (IsSectorHighlight(searchsector))
    {
        for (short i = 0; i < highlightsectorcnt; i++)
        {
            a1(highlightsector[i], a2);
        }
    }
    else
        a1(searchsector, a2);
}

char tempvis2;

void Check3DKeys(void)
{
    int startwall;
    int endwall;
    int nSector;
    int i;
    int j;
    int doubvel;
    int changedir;
    int goalz;
    int xvect;
    int yvect;
    long hiz, loz;
    short hitsect, hitwall, hitsprite;
    int hitx, hity, hitz;
    long hihit, lohit;
    long x;
    long y;
    int nXSector;
    int nXWall;
    int nXSprite;
    unsigned char key;
    int top, bottom;
    int step;
    char shift;
    char ctrl;
    char alt;
    char kp5;

    shift = keystatus[bsc_LShift] | keystatus[bsc_RShift];
    ctrl = keystatus[bsc_LCtrl] | keystatus[bsc_RCtrl];
    alt = keystatus[bsc_LAlt] | keystatus[bsc_RAlt];
    kp5 = keystatus[bsc_Pad_5];
    if (ctrl && kp5)
        horiz = 100;

    if (angvel)
    {
        doubvel = gFrameTicks;
        if (shift)
            doubvel += gFrameTicks / 2;
        ang = (ang + ((angvel* doubvel) >> 4)) & 2047;
    }

    if (vel | svel)
    {
        doubvel = gFrameTicks;
        if (shift)
            doubvel += gFrameTicks;
        xvect = 0, yvect = 0;
        if (vel)
        {
            xvect += mulscale30((vel * doubvel) >> 2, Cos(ang));
            yvect += mulscale30((vel * doubvel) >> 2, Sin(ang));
        }
        if (svel)
        {
            xvect += mulscale30((svel * doubvel) >> 2, Sin(ang));
            yvect -= mulscale30((svel * doubvel) >> 2, Cos(ang));
        }
        clipmove(&posx, &posy, &posz, &cursectnum, xvect<<14, yvect<<14, 200, 1024, 1024, 0);
    }

    getzrange(posx, posy, posz, cursectnum, &hiz, &hihit, &loz, &lohit, 200, 0x10001);

    if ((hihit & 0xe000) == 0x4000 && (hihit & 0x1fff) == cursectnum && (sector[cursectnum].ceilingstat & kSectorStat0))
        hiz = (int)0x80000000;

    if (zmode == 0)
    {
        goalz = loz - kensplayerheight;
        if (goalz < hiz + 0x1000)
            goalz = (loz + hiz) / 2;
        if (keystatus[bsc_A])
        {
            if (ctrl)
            {
                if (horiz > 0)
                    horiz -= 4;
            }
            else
            {
                goalz -= 0x1000;
                if (shift)
                    goalz -= 0x1800;
            }
        }
        if (keystatus[bsc_Z])
        {
            if (ctrl)
            {
                if (horiz < 200)
                    horiz += 4;
            }
            else
            {
                goalz += 0xc00;
                if (shift)
                    goalz += 0xc00;
            }
        }
        if (goalz != posz)
        {
            if (posz < goalz)
                hvel += 32;
            if (posz > goalz)
                hvel = (goalz-posz)>>3;
            posz += hvel;
            if (posz > loz - 0x400)
            {
                posz = loz - 0x400;
                hvel = 0;
            }
            if (posz < hiz + 0x400)
            {
                posz = hiz + 0x400;
                hvel = 0;
            }
        }
    }
    else
    {
        goalz = posz;
        if (keystatus[bsc_A])
        {
            if (keystatus[bsc_LCtrl])
            {
                if (horiz > 0)
                    horiz -= 4;
            }
            else if (zmode != 1)
            {
                goalz -= 0x800;
            }
            else
            {
                zlock += 0x400;
                keystatus[bsc_A] = 0;
            }
        }
        if (keystatus[bsc_Z])
        {
            if (keystatus[bsc_LCtrl])
            {
                if (horiz < 200)
                    horiz += 4;
            }
            else if (zmode != 1)
            {
                goalz += 0x800;
            }
            else if (zlock > 0)
            {
                zlock -= 0x400;
                keystatus[bsc_Z] = 0;
            }
        }

        if (goalz < hiz + 0x400)
            goalz = hiz + 0x400;
        if (goalz > loz - 0x400)
            goalz = loz - 0x400;
        if (zmode == 1)
            goalz = loz - zlock;
        if (goalz < hiz + 0x400)
            goalz = (loz + hiz) / 2;
        if (zmode == 1)
            posz = goalz;
        if (goalz != posz)
        {
            if (posz < goalz)
                hvel += 32;
            if (posz > goalz)
                hvel -= 32;
            posz += hvel;
            if (posz > loz - 0x400)
            {
                posz = loz - 0x400;
                hvel = 0;
            }
            if (posz < hiz + 0x400)
            {
                posz = hiz + 0x400;
                hvel = 0;
            }
        }
        else
            hvel = 0;
    }
    Mouse::Read(gFrameTicks);

    searchx = ClipRange(Mouse::X, 1, xdim-2);
    searchy = ClipRange(Mouse::Y, 1, ydim-2);

    searchit = 2;

    if (searchstat >= 0 && (Mouse::buttons&1))
        searchit = 0;

    Video.SetColor(gStdColor[23+mulscale30(8, Sin((gFrameClock<<11)/120))]);
    gfxHLine(searchy, searchx - 6, searchx - 2);
    gfxHLine(searchy, searchx + 2, searchx + 6);
    gfxVLine(searchx, searchy - 5, searchy - 2);
    gfxVLine(searchx, searchy + 2, searchy + 5);

    if (searchstat < 0)
        return;

    key = keyGet();
    switch (key)
    {
    case bsc_PgUp:
    {
        step = 0x400;
        if (shift)
            step = 0x100;
        if (searchstat == 0)
        {
            if (wall[searchwall].nextsector != -1)
                func_22350();
        }
        if (searchstat == 0)
            searchstat = 1;
        if (ctrl)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].ceilingz, -1, -1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorCeilZ, sector[vd].ceilingz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].floorz, 1, -1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorFloorZ, sector[vd].floorz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(func_25E64, 0);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off floor", vd, vd) * 256;
                int dz = (sector[searchsector].floorz - vd) - sector[searchsector].ceilingz;
                OperateSectors(func_26054, dz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off ceiling", vd, vd) * 256;
                int dz = (sector[searchsector].ceilingz + vd) - sector[searchsector].floorz;
                OperateSectors(func_26074, dz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 1:
            {
                OperateSectors(func_26094, step);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                OperateSectors(func_260B8, step);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(func_25F8C, step);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        ModifyBeep();
        break;
    }
    case bsc_PgDn:
    {
        step = 0x400;
        if (shift)
            step = 0x100;
        if (searchstat == 0)
        {
            if (wall[searchwall].nextsector != -1)
                func_22350();
        }
        if (searchstat == 0)
            searchstat = 1;
        if (ctrl)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].ceilingz, -1, 1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorCeilZ, sector[vd].ceilingz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].floorz, 1, 1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorFloorZ, sector[vd].floorz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(func_25EF8, 0);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off floor", vd, vd) * 256;
                int dz = (sector[searchsector].floorz - vd) - sector[searchsector].ceilingz;
                OperateSectors(func_26054, dz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off ceiling", vd, vd) * 256;
                int dz = (sector[searchsector].ceilingz + vd) - sector[searchsector].floorz;
                OperateSectors(func_26074, dz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 1:
            {
                OperateSectors(func_260DC, step);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                OperateSectors(func_260FC, step);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(func_25F9C, step);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        ModifyBeep();
        break;
    }
    case bsc_Del:
        if (searchstat == 3)
        {
            DeleteSprite(searchwall);
            updatenumsprites();
            ModifyBeep();
        }
        else
            Beep();
        break;
    case bsc_Tab:
        switch (searchstat)
        {
        case 0:
            temppicnum = wall[searchwall].picnum;
            tempshade = wall[searchwall].shade;
            temppal = wall[searchwall].pal;
            tempxrepeat = wall[searchwall].xrepeat;
            tempyrepeat = wall[searchwall].yrepeat;
            tempcstat = wall[searchwall].cstat;
            tempextra = wall[searchwall].extra;
            temptype = wall[searchwall].type;
            break;
        case 1:
            temppicnum = sector[searchsector].ceilingpicnum;
            tempshade = sector[searchsector].ceilingshade;
            temppal = sector[searchsector].ceilingpal;
            tempxrepeat = sector[searchsector].ceilingxpanning;
            tempyrepeat = sector[searchsector].ceilingypanning;
            tempcstat = sector[searchsector].ceilingstat;
            tempextra = sector[searchsector].extra;
            tempvis2 = sector[searchsector].visibility;
            temptype = sector[searchsector].type;
            break;
        case 2:
            temppicnum = sector[searchsector].floorpicnum;
            tempshade = sector[searchsector].floorshade;
            temppal = sector[searchsector].floorpal;
            tempxrepeat = sector[searchsector].floorxpanning;
            tempyrepeat = sector[searchsector].floorypanning;
            tempcstat = sector[searchsector].floorstat;
            tempextra = sector[searchsector].extra;
            tempvis2 = sector[searchsector].visibility;
            temptype = sector[searchsector].type;
            break;
        case 3:
            temppicnum = sprite[searchwall].picnum;
            tempshade = sprite[searchwall].shade;
            temppal = sprite[searchwall].pal;
            tempxrepeat = sprite[searchwall].xrepeat;
            tempyrepeat = sprite[searchwall].yrepeat;
            tempcstat = sprite[searchwall].cstat;
            tempextra = sprite[searchwall].extra;
            tempang = sprite[searchwall].ang;
            temptype = sprite[searchwall].type;
            break;
        case 4:
            temppicnum = wall[searchwall].overpicnum;
            tempshade = wall[searchwall].shade;
            temppal = wall[searchwall].pal;
            tempxrepeat = wall[searchwall].xrepeat;
            tempyrepeat = wall[searchwall].yrepeat;
            tempcstat = wall[searchwall].cstat;
            tempextra = wall[searchwall].extra;
            temptype = wall[searchwall].type;
            break;
        }
        somethingintab = searchstat;
        break;
    case bsc_CapsLock:
    {
        zmode = IncRotate(zmode, 3);
        if (zmode == 1)
            zlock = (loz-posz)&~0x3ff;
        sprintf(buffer, "ZMode = %s", pzZMode[zmode]);
        scrSetMessage(buffer);
        break;
    }
    case bsc_Enter:
        if (somethingintab == 255)
        {
            Beep();
            break;
        }
        if (ctrl)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                i = searchwall;
                do
                {
                    if (shift)
                    {
                        wall[i].shade = tempshade;
                        wall[i].pal = temppal;
                    }
                    else
                    {
                        wall[i].picnum = temppicnum;
                        if (somethingintab == 0 || somethingintab == 4)
                        {
                            wall[i].xrepeat = tempxrepeat;
                            wall[i].yrepeat = tempyrepeat;
                            wall[i].cstat = tempcstat;
                        }
                        fixrepeats(i);
                    }
                    i = wall[i].point2;
                } while (i != searchwall);
                ModifyBeep();
                break;
            }
            case 1:
            {
                for (i = 0; i < numsectors; i++)
                {
                    if (sector[i].ceilingstat & kSectorStat0)
                    {
                        sector[i].ceilingpicnum = temppicnum;
                        sector[i].ceilingshade = tempshade;
                        sector[i].ceilingpal = temppal;
                        if (somethingintab == 1 || somethingintab == 2)
                        {
                            sector[i].ceilingxpanning = tempxrepeat;
                            sector[i].ceilingypanning = tempyrepeat;
                            sector[i].ceilingstat = (char)(tempcstat|kSectorStat0);
                        }
                    }
                }
                ModifyBeep();
                break;
            }
            case 2:
            {
                for (i = 0; i < numsectors; i++)
                {
                    if (sector[i].floorstat & kSectorStat0)
                    {
                        sector[i].floorpicnum = temppicnum;
                        sector[i].floorshade = tempshade;
                        sector[i].floorpal = temppal;
                        if (somethingintab == 1 || somethingintab == 2)
                        {
                            sector[i].floorxpanning = tempxrepeat;
                            sector[i].floorypanning = tempyrepeat;
                            sector[i].floorstat = (char)(tempcstat|kSectorStat0);
                        }
                    }
                }
                ModifyBeep();
                break;
            }
            default:
                Beep();
                break;
            }
        }
        else if (shift)
        {
            switch (searchstat)
            {
            case 0:
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                break;
            case 1:
                if (IsSectorHighlight(searchsector))
                {
                    for (i = 0; i < highlightsectorcnt; i++)
                    {
                        sector[i].ceilingshade = tempshade;
                        sector[i].ceilingpal = temppal;
                        sector[i].visibility = tempvis2;
                    }
                }
                else
                {
                    sector[searchsector].ceilingshade = tempshade;
                    sector[searchsector].ceilingpal = temppal;
                    sector[searchsector].visibility = tempvis2;
                    if (sector[searchsector].ceilingstat & kSectorStat0)
                    {
                        for (i = 0; i < numsectors; i++)
                        {
                            if (sector[i].ceilingstat & kSectorStat0)
                            {
                                sector[i].ceilingshade = tempshade;
                                sector[i].ceilingpal = temppal;
                                sector[i].visibility = tempvis2;
                            }
                        }
                    }
                }
                break;
            case 2:
                if (IsSectorHighlight(searchsector))
                {
                    for (i = 0; i < highlightsectorcnt; i++)
                    {
                        sector[i].floorshade = tempshade;
                        sector[i].floorpal = temppal;
                        sector[i].visibility = tempvis2;
                    }
                }
                else
                {
                    sector[searchsector].floorshade = tempshade;
                    sector[searchsector].floorpal = temppal;
                    sector[searchsector].visibility = tempvis2;
                }
                break;
            case 3:
                sprite[searchwall].shade = tempshade;
                sprite[searchwall].pal = temppal;
                break;
            case 4:
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                break;
            }
            ModifyBeep();
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                if (somethingintab == 0 || somethingintab == 4)
                {
                    wall[searchwall].extra = tempextra;
                    wall[searchwall].type = temptype;
                    func_1058C();
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            case 1:
            case 2:
                if (somethingintab == 1 || somethingintab == 2)
                {
                    if (IsSectorHighlight(searchsector))
                    {
                        for (i = 0; i < highlightsectorcnt; i++)
                        {
                            sector[i].extra = tempextra;
                            sector[i].type = temptype;
                        }
                    }
                    else
                    {
                        sector[searchsector].extra = tempextra;
                        sector[searchsector].type = temptype;
                    }
                    func_1058C();
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            case 3:
                if (somethingintab == 3)
                {
                    sprite[searchwall].type = temptype;
                    sprite[searchwall].extra = tempextra;
                    func_1058C();
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
                wall[searchwall].picnum = temppicnum;
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                if (somethingintab == 0)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                }
                fixrepeats(searchwall);
                break;
            case 1:
                sector[searchsector].ceilingpicnum = temppicnum;
                sector[searchsector].ceilingshade = tempshade;
                sector[searchsector].ceilingpal = temppal;
                if (somethingintab == 1 || somethingintab == 2)
                {
                    sector[searchsector].ceilingxpanning = tempxrepeat;
                    sector[searchsector].ceilingypanning = tempyrepeat;
                    sector[searchsector].ceilingstat = (char)tempcstat; // FIXME
                    sector[searchsector].visibility = tempvis2;
                }
                break;
            case 2:
                sector[searchsector].floorpicnum = temppicnum;
                sector[searchsector].floorshade = tempshade;
                sector[searchsector].floorpal = temppal;
                if (somethingintab == 1 || somethingintab == 2)
                {
                    sector[searchsector].floorxpanning = tempxrepeat;
                    sector[searchsector].floorypanning = tempyrepeat;
                    sector[searchsector].floorstat = (char)tempcstat; // FIXME
                    sector[searchsector].visibility = tempvis2;
                }
                break;
            case 3:
            {
                sprite[searchwall].picnum = temppicnum;
                sprite[searchwall].shade = tempshade;
                sprite[searchwall].pal = temppal;
                if (somethingintab == 3)
                {
                    sprite[searchwall].xrepeat = tempxrepeat;
                    sprite[searchwall].yrepeat = tempyrepeat;
                    if (sprite[searchwall].xrepeat < 1)
                        sprite[searchwall].xrepeat = 1;
                    if (sprite[searchwall].yrepeat < 1)
                        sprite[searchwall].yrepeat = 1;
                    sprite[searchwall].cstat = tempcstat;
                }
                GetSpriteExtents(&sprite[searchwall], &top, &bottom);
                if (!(sector[sprite[searchwall].sectnum].ceilingstat & kSectorStat0))
                    sprite[searchwall].z += ClipLow(sector[sprite[searchwall].sectnum].ceilingz - top, 0);
                    
                if (!(sector[sprite[searchwall].sectnum].floorstat & kSectorStat0))
                    sprite[searchwall].z += ClipHigh(sector[sprite[searchwall].sectnum].floorz - bottom, 0);
                break;
            }
            case 4:
                wall[searchwall].overpicnum = temppicnum;
                if (wall[searchwall].nextwall >= 0)
                    wall[wall[searchwall].nextwall].overpicnum = temppicnum;
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                if (somethingintab == 4)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                }
                fixrepeats(searchwall);
                break;
            }
            ModifyBeep();
        }
        break;
    case bsc_OpenBracket:
        step = 0x100;
        if (shift)
            step = 0x20;
        switch (searchstat)
        {
        case 1:
            if (IsSectorHighlight(searchsector))
            {
                for (i = 0; i < highlightsectorcnt; i++)
                {
                    SetSectorCeilSlope(highlightsector[i], DecStep(sector[highlightsector[i]].ceilingheinum, step));
                }
                sprintf(buffer, "adjusted %i ceilings by %i", highlightsectorcnt, step);
                scrSetMessage(buffer);
            }
            else
            {
                SetSectorCeilSlope(searchsector, DecStep(sector[searchsector].ceilingheinum, step));
                sprintf(buffer, "sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingheinum);
                scrSetMessage(buffer);
            }
            break;
        case 2:
            if (IsSectorHighlight(searchsector))
            {
                for (i = 0; i < highlightsectorcnt; i++)
                {
                    SetSectorFloorSlope(highlightsector[i], DecStep(sector[highlightsector[i]].floorheinum, step));
                }
                sprintf(buffer, "adjusted %i floors by %i", highlightsectorcnt, step);
                scrSetMessage(buffer);
            }
            else
            {
                SetSectorFloorSlope(searchsector, DecStep(sector[searchsector].floorheinum, step));
                sprintf(buffer, "sector[%i].floorslope: %i", searchsector, sector[searchsector].floorheinum);
                scrSetMessage(buffer);
            }
            break;
        }
        ModifyBeep();
        break;
    case bsc_CloseBracket:
        if (alt)
        {
            short nNextSector = wall[searchwall].nextsector;
            if (nNextSector < 0)
            {
                Beep();
                break;
            }
            x = wall[searchwall].x;
            y = wall[searchwall].y;
            switch (searchstat)
            {
            case 1:
                alignceilslope(searchsector, x, y, getceilzofslope(nNextSector, x, y));
                ModifyBeep();
                break;
            case 2:
                alignflorslope(searchsector, x, y, getflorzofslope(nNextSector, x, y));
                ModifyBeep();
                break;
            default:
                Beep();
                break;
            }
        }
        else
        {
            step = 0x100;
            if (shift)
                step = 0x20;
            switch (searchstat)
            {
            case 1:
                if (IsSectorHighlight(searchsector))
                {
                    for (i = 0; i < highlightsectorcnt; i++)
                    {
                        SetSectorCeilSlope(highlightsector[i], IncStep(sector[highlightsector[i]].ceilingheinum, step));
                    }
                    sprintf(buffer, "adjusted %i ceilings by %i", highlightsectorcnt, step);
                    scrSetMessage(buffer);
                }
                else
                {
                    SetSectorCeilSlope(searchsector, IncStep(sector[searchsector].ceilingheinum, step));
                    sprintf(buffer, "sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingheinum);
                    scrSetMessage(buffer);
                }
                break;
            case 2:
                if (IsSectorHighlight(searchsector))
                {
                    for (i = 0; i < highlightsectorcnt; i++)
                    {
                        SetSectorFloorSlope(highlightsector[i], IncStep(sector[highlightsector[i]].floorheinum, step));
                    }
                    sprintf(buffer, "adjusted %i floors by %i", highlightsectorcnt, step);
                    scrSetMessage(buffer);
                }
                else
                {
                    SetSectorFloorSlope(searchsector, IncStep(sector[searchsector].floorheinum, step));
                    sprintf(buffer, "sector[%i].floorslope: %i", searchsector, sector[searchsector].floorheinum);
                    scrSetMessage(buffer);
                }
                break;
            }
            ModifyBeep();
        }
        break;
    case bsc_Comma:
        switch (searchstat)
        {
        case 3:
        {
            step = shift ? 0x10 : 0x100;
            i = searchwall;
            sprite[i].ang = IncStep(sprite[i].ang, step) & 2047;
            sprintf(buffer, "sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        default:
            Beep();
            break;
        }
        break;
    case bsc_Period:
        switch (searchstat)
        {
        case 3:
        {
            step = shift ? 0x10 : 0x100;
            i = searchwall;
            sprite[i].ang = DecStep(sprite[i].ang, step) & 2047;
            sprintf(buffer, "sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        case 0:
        case 4:
            AutoAlignWalls(searchwall, 0);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_BackSlash:
        switch (searchstat)
        {
        case 1:
            sector[searchsector].ceilingheinum = 0;
            sector[searchsector].ceilingstat &= ~kSectorStat1;
            sprintf(buffer, "sector[%i] ceiling slope reset", searchsector);
            scrSetMessage(buffer);
            break;
        case 2:
            sector[searchsector].floorheinum = 0;
            sector[searchsector].floorstat &= ~kSectorStat1;
            sprintf(buffer, "sector[%i] floor slope reset", searchsector);
            scrSetMessage(buffer);
            break;
        }
        ModifyBeep();
        break;
    case bsc_Slash:
        switch (searchstat)
        {
        case 0:
        case 4:
            if (shift)
            {
                wall[searchwall].xrepeat = wall[searchwall].yrepeat;
            }
            else
            {
                wall[searchwall].xpanning = 0;
                wall[searchwall].ypanning = 0;
                wall[searchwall].xrepeat = 8;
                wall[searchwall].yrepeat = 8;
                wall[searchwall].cstat = 0;
            }
            fixrepeats(searchwall);
            sprintf(buffer, "wall[%i] pan/repeat reset", searchwall);
            scrSetMessage(buffer);
            break;
        case 1:
            sector[searchsector].ceilingxpanning = 0;
            sector[searchsector].ceilingypanning = 0;
            sector[searchsector].ceilingstat &= ~kSectorStat1_4_5;
            sprintf(buffer, "sector[%i] ceiling pan reset", searchsector);
            scrSetMessage(buffer);
            break;
        case 2:
            sector[searchsector].floorxpanning = 0;
            sector[searchsector].floorypanning = 0;
            sector[searchsector].floorstat &= ~kSectorStat1_4_5;
            sprintf(buffer, "sector[%i] floor pan reset", searchsector);
            scrSetMessage(buffer);
            break;
        case 3:
            if (shift)
            {
                sprite[searchwall].xrepeat = sprite[searchwall].yrepeat;
            }
            else
            {
                sprite[searchwall].xrepeat = sprite[searchwall].yrepeat = 64;
            }
            sprintf(buffer, "sprite[%i].xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
            scrSetMessage(buffer);
            break;
        }
        ModifyBeep();
        break;
    case bsc_Minus:
        if (ctrl && alt)
        {
            if (IsSectorHighlight(searchsector))
            {
                for (i = 0; i < highlightsectorcnt; i++)
                {
                    sector[highlightsector[i]].visibility++;
                }
                scrSetMessage("highlighted sectors less visible");
            }
            else
            {
                sector[searchsector].visibility++;
                sprintf(buffer, "Visibility: %i", sector[searchsector].visibility);
                scrSetMessage(buffer);
            }
            ModifyBeep();
        }
        else if (ctrl)
        {
            nXSector = func_10DBC(searchsector);
            xsector[nXSector].atd_6++;
            sprintf(buffer, "Amplitude: %i", xsector[nXSector].atd_6);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (shift)
        {
            nXSector = func_10DBC(searchsector);
            xsector[nXSector].at10_0 -= 6;
            sprintf(buffer, "Phase: %i", xsector[nXSector].at10_0);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (keystatus[bsc_G])
        {
            gGamma = ClipLow(gGamma - 1, 0);
            sprintf(buffer, "Gamma correction level %i", gGamma);
            scrSetMessage(buffer);
            scrSetGamma(gGamma);
            scrSetDac();
        }
        else if (keystatus[bsc_D])
        {
            gVisibility = ClipHigh(gVisibility + 0x10, 0x1000);
            sprintf(buffer, "Depth cueing level %i", gVisibility);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        break;
    case bsc_Plus:
        if (ctrl && alt)
        {
            if (IsSectorHighlight(searchsector))
            {
                for (i = 0; i < highlightsectorcnt; i++)
                {
                    sector[highlightsector[i]].visibility--;
                }
                scrSetMessage("highlighted sectors more visible");
            }
            else
            {
                sector[searchsector].visibility--;
                sprintf(buffer, "Visibility: %i", sector[searchsector].visibility);
                scrSetMessage(buffer);
            }
            ModifyBeep();
        }
        else if (ctrl)
        {
            nXSector = func_10DBC(searchsector);
            xsector[nXSector].atd_6--;
            sprintf(buffer, "Amplitude: %i", xsector[nXSector].atd_6);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (shift)
        {
            nXSector = func_10DBC(searchsector);
            xsector[nXSector].at10_0 += 6;
            sprintf(buffer, "Phase: %i", xsector[nXSector].at10_0);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (keystatus[bsc_G])
        {
            gGamma = ClipHigh(gGamma + 1, gGammaLevels-1);
            sprintf(buffer, "Gamma correction level %i", gGamma);
            scrSetMessage(buffer);
            scrSetGamma(gGamma);
            scrSetDac();
        }
        else if (keystatus[bsc_D])
        {
            gVisibility = ClipLow(gVisibility - 0x10, 0x80);
            sprintf(buffer, "Depth cueing level %i", gVisibility);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        break;
    case bsc_B:
        switch (searchstat)
        {
        case 0:
        case 4:
            if (TestBitString(show2dwall, searchwall))
            {
                for (int i = 0; i < highlightcnt; i++)
                {
                    if ((highlight[i] & 0xc000) == 0)
                    {
                        int nWall = highlight[i];
                        wall[nWall].cstat ^= kWallStat0;
                    }
                }
            }
            else
            {
                wall[searchwall].cstat ^= kWallStat0;
                if (wall[searchwall].nextwall >= 0)
                {
                    wall[wall[searchwall].nextwall].cstat &= ~kWallStat0;
                    wall[wall[searchwall].nextwall].cstat |= wall[searchwall].cstat & kWallStat0;
                }
            }
            sprintf(buffer, "Wall %i blocking flag is %s", searchwall, int_D9A88[(wall[searchwall].cstat & kWallStat0) ? 1 : 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
            sprite[searchwall].cstat ^= kSpriteStat0;
            sprintf(buffer, "sprite[%i] %s blocking", searchwall, (sprite[searchwall].cstat & kSpriteStat0) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_C:
        if (alt)
        {
            if (somethingintab != searchstat)
            {
                Beep();
                break;
            }
            switch (searchstat)
            {
                case 0:
                    j = wall[searchwall].picnum;
                    if (TestBitString(show2dwall, searchwall))
                    {
                        for (int i = 0; i < highlightcnt; i++)
                        {
                            if ((highlight[i] & 0xc000) == 0)
                            {
                                int nWall = highlight[i];
                                if (wall[nWall].picnum == j)
                                {
                                    if (wall[nWall].picnum != temppicnum)
                                        wall[nWall].picnum = temppicnum;
                                    else if (wall[nWall].pal != temppal)
                                        wall[nWall].pal = temppal;
                                }
                            }
                        }
                    }
                    else
                    {
                        for (i = 0; i < numwalls; i++)
                        {
                            if (wall[i].picnum == j)
                            {
                                if (wall[i].picnum != temppicnum)
                                    wall[i].picnum = temppicnum;
                                else if (wall[i].pal != temppal)
                                    wall[i].pal = temppal;
                            }
                        }
                    }
                    break;
                case 1:
                    j = sector[searchsector].ceilingpicnum;
                    for (i = 0; i < numsectors; i++)
                    {
                        if (sector[i].ceilingpicnum == j)
                        {
                            if (sector[i].ceilingpicnum != temppicnum)
                                sector[i].ceilingpicnum = temppicnum;
                            else if (sector[i].ceilingpal != temppal)
                                sector[i].ceilingpal = temppal;
                        }
                    }
                    break;
                case 2:
                    j = sector[searchsector].floorpicnum;
                    for (i = 0; i < numsectors; i++)
                    {
                        if (sector[i].floorpicnum == j)
                        {
                            if (sector[i].floorpicnum != temppicnum)
                                sector[i].floorpicnum = temppicnum;
                            else if (sector[i].floorpal != temppal)
                                sector[i].floorpal = temppal;
                        }
                    }
                    break;
                case 3:
                    j = sprite[searchwall].picnum;
                    for (i = 0; i < kMaxSprites; i++)
                    {
                        if (sprite[i].statnum < kMaxStatus && sprite[i].picnum == j)
                        {
                            sprite[i].picnum = temppicnum;
                            sprite[i].type = temptype;
                        }
                    }
                    break;
                case 4:
                    j = wall[searchwall].overpicnum;
                    for (i = 0; i < numwalls; i++)
                    {
                        if (wall[i].overpicnum == j)
                        {
                            wall[i].overpicnum = temppicnum;
                        }
                    }
                    break;
            }
            ModifyBeep();
            break;
        }
        break;
    case bsc_D:
        if (searchstat == 3)
        {
            if (alt)
            {
                int nClipDist = GetNumberBox("Sprite clipdist", sprite[searchwall].clipdist, sprite[searchwall].clipdist);
                if (nClipDist >= 0 && nClipDist < 256)
                {
                    sprite[searchwall].clipdist = nClipDist;
                    sprintf(buffer, "sprite[%d].clipdist is %d", searchwall, nClipDist);
                    scrSetMessage(buffer);
                    ModifyBeep();
                }
                else
                {
                    sprintf(buffer, "Clipdist must be between %d and %d", 0, 255);
                    scrSetMessage(buffer);
                    Beep();
                }
            }
            else
            {
                int nDetail = GetNumberBox("Sprite detail", sprite[searchwall].detail, sprite[searchwall].detail);
                if (nDetail >= 0 && nDetail <= 4)
                {
                    sprite[searchwall].detail = nDetail;
                    sprintf(buffer, "sprite[%d].detail is %d", searchwall, nDetail);
                    scrSetMessage(buffer);
                    ModifyBeep();
                }
                else
                {
                    sprintf(buffer, "Detail must be between %d and %d", 0, 4);
                    scrSetMessage(buffer);
                    Beep();
                }
            }
        }
        break;
    case bsc_E:
        switch (searchstat)
        {
        case 1:
            sector[searchsector].ceilingstat ^= kSectorStat3;
            sprintf(buffer, "ceiling texture %s expanded", (sector[searchsector].ceilingstat & kSectorStat3) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 2:
            sector[searchsector].floorstat ^= kSectorStat3;
            sprintf(buffer, "floor texture %s expanded", (sector[searchsector].floorstat & kSectorStat3) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_F:
        if (alt)
        {
            switch (searchstat)
            {  
            case 0:
            case 4:
            {
                nSector = sectorofwall(searchwall);
                sector[searchsector].ceilingstat &= ~kSectorStat1_4_5;
                sector[searchsector].ceilingstat |= kSectorStat6;
                SetFirstWall(nSector, searchwall);
                ModifyBeep();
                break;
            }
            case 1:
                sector[searchsector].ceilingstat &= ~kSectorStat1_4_5;
                sector[searchsector].ceilingstat |= kSectorStat6;
                SetFirstWall(searchsector, sector[searchsector].wallptr+1);
                ModifyBeep();
                break;
            case 2:
                sector[searchsector].floorstat &= ~kSectorStat1_4_5;
                sector[searchsector].floorstat |= kSectorStat6;
                SetFirstWall(searchsector, sector[searchsector].wallptr+1);
                ModifyBeep();
                break;
            default:
                Beep();
                break;
            }
        }
        else if (ctrl)
        {
            gFogMode = !gFogMode;
            scrLoadPLUs();
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                i = wall[searchwall].cstat & kWallStat3_8;
                switch (i)
                {
                case 0:
                    i = 8;
                    break;
                case 8:
                    i = 0x108;
                    break;
                case 0x108:
                    i = 0x100;
                    break;
                case 0x100:
                    i = 0;
                    break;
                }

                wall[searchwall].cstat &= ~kWallStat3_8;
                wall[searchwall].cstat |= i;
                sprintf(buffer, "wall[%i]", searchwall);
                if (wall[searchwall].cstat & kWallStat3)
                    strcat(buffer, " x-flipped");
                if (wall[searchwall].cstat & kWallStat8)
                {
                    if (wall[searchwall].cstat & kWallStat3)
                        strcat(buffer, " and");
                    strcat(buffer, " y-flipped");
                }
                scrSetMessage(buffer);
                ModifyBeep();
                break;
            }
            case 1:
            {
                i = sector[searchsector].ceilingstat & kSectorStat1_4_5;
                switch (i)
                {
                case 0x00:
                    i = 0x10;
                    break;
                case 0x10:
                    i = 0x30;
                    break;
                case 0x30:
                    i = 0x20;
                    break;
                case 0x20:
                    i = 0x04;
                    break;
                case 0x04:
                    i = 0x14;
                    break;
                case 0x14:
                    i = 0x34;
                    break;
                case 0x34:
                    i = 0x24;
                    break;
                case 0x24:
                    i = 0x00;
                    break;
                }
                sector[searchsector].ceilingstat &= ~kSectorStat1_4_5;
                sector[searchsector].ceilingstat |= (char)i;
                break;
            }
            case 2:
            {
                i = sector[searchsector].floorstat & kSectorStat1_4_5;
                switch (i)
                {
                case 0x00:
                    i = 0x10;
                    break;
                case 0x10:
                    i = 0x30;
                    break;
                case 0x30:
                    i = 0x20;
                    break;
                case 0x20:
                    i = 0x04;
                    break;
                case 0x04:
                    i = 0x14;
                    break;
                case 0x14:
                    i = 0x34;
                    break;
                case 0x34:
                    i = 0x24;
                    break;
                case 0x24:
                    i = 0x00;
                    break;
                }
                sector[searchsector].floorstat &= ~kSectorStat1_4_5;
                sector[searchsector].floorstat |= (char)i;
                break;
            }
            case 0x03:
            {
                i = sprite[searchwall].cstat;
                if ((i&kSpriteMask) == kSpriteFloor && !(i&kSpriteStat6))
                {
                    sprite[searchwall].cstat &= ~kSpriteStat3;
                    sprite[searchwall].cstat ^= kSpriteStat2;
                }
                else
                {
                    i &= 0x0c;
                    switch (i)
                    {
                    case 0x00:
                        i = 0x04;
                        break;
                    case 0x04:
                        i = 0x0c;
                        break;
                    case 0x0c:
                        i = 0x08;
                        break;
                    case 0x08:
                        i = 0x00;
                        break;
                    }
                    sprite[searchwall].cstat &= ~0x0c;
                    sprite[searchwall].cstat |= i;
                }
                break;
            }
            }
            ModifyBeep();
        }
        break;
    case bsc_G:
        if (alt)
        {
            int_D9A80 = IncRotate(int_D9A80, 5);
            scrSetPalette(int_D9A80);
            scrSetDac();
            switch (int_D9A80)
            {
            case 0:
                scrSetMessage("Normal palette");
                break;
            case 1:
                scrSetMessage("Water palette");
                break;
            case 2:
                scrSetMessage("Beast palette");
                break;
            case 3:
                scrSetMessage("Sewer palette");
                break;
            case 4:
                scrSetMessage("Invulnerability palette");
                break;
            }
        }
        break;
    case bsc_H:
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].cstat ^= kWallStat6;
            if (wall[searchwall].nextwall >= 0)
            {
                wall[wall[searchwall].nextwall].cstat &= ~kWallStat6;
                wall[wall[searchwall].nextwall].cstat |= wall[searchwall].cstat & kWallStat6;
            }
            sprintf(buffer, "wall[%i] %s hitscan sensitive", searchwall, (wall[searchwall].cstat & kWallStat6) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
            sprite[searchwall].cstat ^= kSpriteStat8;
            sprintf(buffer, "sprite[%i] %s hitscan sensitive", searchwall, (sprite[searchwall].cstat & kSpriteStat8) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_I:
        switch (searchstat)
        {
        case 3:
            sprite[searchwall].cstat ^= kSpriteStat15;
            sprintf(buffer, "sprite[%i] is%s invisible", searchwall, (sprite[searchwall].cstat & kSpriteStat15) ? "" : " not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_L:
        switch (searchstat)
        {
        case 3:
        {
            GetSpriteExtents(&sprite[searchwall], &top, &bottom);
            ushort bakcstat = sprite[searchwall].cstat;
            sprite[searchwall].cstat &= ~kSpriteStat8;
            ResetLightBomb();
            DoLightBomb(sprite[searchwall].x, sprite[searchwall].y, top, sprite[searchwall].sectnum);
            sprite[searchwall].cstat = bakcstat;
            ModifyBeep();
            break;
        }
        case 2:
            if (sector[searchsector].ceilingstat & kSectorStat0)
            {
                sector[searchsector].floorstat ^= kSectorStat15;
                sprintf(buffer, "Forced floor shading is %s", int_D9A88[(sector[searchsector].floorstat & kSectorStat15) ? 1 : 0]);
                scrSetMessage(buffer);
            }
            else
                scrSetMessage("Sky must be parallaxed to force floorshade");
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_M:
        switch (searchstat)
        {
        case 0:
        case 4:
        case 1: // ??
        case 2: // ??
        {
            i = wall[searchwall].nextwall;
            if (i < 0)
            {
                Beep();
                break;
            }
            wall[searchwall].cstat ^= kWallStat4;
            if (wall[searchwall].cstat & kWallStat4)
            {
                wall[searchwall].cstat &= ~kWallStat3;
                if (!shift)
                {
                    wall[i].cstat |= (kWallStat3|kWallStat4);
                    if (wall[searchwall].overpicnum < 0)
                        wall[searchwall].overpicnum = 0;
                    wall[i].overpicnum = wall[searchwall].overpicnum;
                }
            }
            else
            {
                wall[searchwall].cstat &= ~kWallStat3;
                if (!shift)
                {
                    wall[i].cstat &= ~(kWallStat3|kWallStat4);
                }
            }
            wall[searchwall].cstat &= ~kWallStat5;
            if (!shift)
                wall[i].cstat &= ~kWallStat5;
            sprintf(buffer, "wall[%i] %s masked", searchwall, (wall[searchwall].cstat & kWallStat4) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        default:
            Beep();
            break;
        }
        break;
    case bsc_O:
        if (alt)
        {
            asksave = ShowOptions();
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                wall[searchwall].cstat ^= kWallStat2;
                if (wall[searchwall].nextwall == -1)
                {
                    sprintf(buffer, "Texture pegged at %s", (wall[searchwall].cstat & kWallStat2) ? "bottom" : "top");
                }
                else
                {
                    sprintf(buffer, "Texture pegged at %s", (wall[searchwall].cstat & kWallStat2) ? "outside" : "inside");
                }
                scrSetMessage(buffer);
                ModifyBeep();
                break;
            case 3:
            {
                if (sprite[searchwall].z > sector[sprite[searchwall].sectnum].floorz)
                {
                    scrSetMessage("Sprite z is below floor");
                    Beep();
                    break;
                }
                if (sprite[searchwall].z < sector[sprite[searchwall].sectnum].ceilingz)
                {
                    scrSetMessage("Sprite z is above ceiling");
                    Beep();
                    break;
                }
                int va = HitScan(&sprite[searchwall], sprite[searchwall].z, Cos(sprite[searchwall].ang+1024)>>16, Sin(sprite[searchwall].ang+1024)>>16, 0, 0, 0);
                if (va == 0 || va == 4)
                {
                    int nx, ny;
                    GetWallNormal(gHitInfo.hitwall, &nx, &ny);
                    sprite[searchwall].x = gHitInfo.hitx + (nx>>14);
                    sprite[searchwall].y = gHitInfo.hity + (ny>>14);
                    sprite[searchwall].z = gHitInfo.hitz;

                    sprite[searchwall].cstat &= ~kSpriteStat0;
                    sprite[searchwall].cstat |= kSpriteStat6;

                    ChangeSpriteSect(searchwall, gHitInfo.hitsect);

                    sprite[searchwall].ang = (GetWallAngle(gHitInfo.hitwall)+512)&2047;
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            }
            default:
                Beep();
                break;
            }
        }
        break;
    case bsc_P:
        if (ctrl)
        {
            parallaxtype = IncRotate(parallaxtype, 3);
            sprintf(buffer, "Parallax type: %i", parallaxtype);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                wall[searchwall].pal = GetNumberBox("Wall palookup", wall[searchwall].pal, wall[searchwall].pal);
                break;
            case 1:
                sector[searchsector].ceilingpal = GetNumberBox("Ceiling palookup", sector[searchsector].ceilingpal, sector[searchsector].ceilingpal);
                break;
            case 2:
                sector[searchsector].floorpal = GetNumberBox("Floor palookup", sector[searchsector].floorpal, sector[searchsector].floorpal);
                break;
            case 3:
                sprite[searchwall].pal = GetNumberBox("Sprite palookup", sprite[searchwall].pal, sprite[searchwall].pal);
                break;
            default:
                break;
            }
            ModifyBeep();
        }
        else
        {
            switch (searchstat)
            {
            case 1:
                sector[searchsector].ceilingstat ^= kSectorStat0;
                sprintf(buffer, "sector[%i] ceiling %s parallaxed", searchsector, (sector[searchsector].ceilingstat & kSectorStat0) ? "is" : "not");
                scrSetMessage(buffer);
                if (sector[searchsector].ceilingstat & kSectorStat0)
                    SetSkyTile(sector[searchsector].ceilingpicnum);
                else
                    sector[searchsector].floorstat &= ~kSectorStat15;
                ModifyBeep();
                break;
            case 2:
                sector[searchsector].floorstat ^= kSectorStat0;
                sprintf(buffer, "sector[%i] floor %s parallaxed", searchsector, (sector[searchsector].floorstat & kSectorStat0) ? "is" : "not");
                scrSetMessage(buffer);
                ModifyBeep();
                break;
            default:
                Beep();
                break;
            }
        }
        break;
    case bsc_R:
        switch (searchstat)
        {
        case 1:
            sector[searchsector].ceilingstat ^= kSectorStat6;
            sprintf(buffer, "sector[%i] ceiling %s relative", searchsector, (sector[searchsector].ceilingstat & kSectorStat6) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 2:
            sector[searchsector].floorstat ^= kSectorStat6;
            sprintf(buffer, "sector[%i] floor %s relative", searchsector, (sector[searchsector].floorstat & kSectorStat6) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
        {
            i = sprite[searchwall].cstat & kSpriteMask;
            switch (i)
            {
            case 0x00:
                i = 0x10;
                sprintf(buffer, "sprite[%i] is wall sprite", searchwall);
                break;
            case 0x10:
                i = 0x20;
                sprintf(buffer, "sprite[%i] is floor sprite", searchwall);
                break;
            case 0x20:
                i = 0x00;
                sprintf(buffer, "sprite[%i] is face sprite", searchwall);
                break;
            default:
                i = 0x00;
                sprintf(buffer, "sprite[%i] is face sprite", searchwall);
                break;
            }
            scrSetMessage(buffer);
            sprite[searchwall].cstat &= ~kSpriteMask;
            sprite[searchwall].cstat |= i;
            ModifyBeep();
            break;
        }
        default:
            Beep();
            break;
        }
        break;
    case bsc_S:
        if (alt)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                x = 0x4000;
                y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (searchy-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitwall != searchwall)
                    break;
                if (hitsect < 0)
                {
                    Beep();
                    break;
                }
                x = hitx;
                y = hity;
                if (gGridLock > 0 && gGrid > 0)
                {
                    x = (x + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                    y = (y + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                }
                i = InsertObject(searchstat, hitsect, x, y, hitz, ang);
                if (i >= 0)
                {
                    int nx, ny;
                    sprite[i].ang = (GetWallAngle(hitwall)+512)&2047;
                    GetWallNormal(hitwall, &nx, &ny);
                    sprite[i].x = hitx + (nx >> 14);
                    sprite[i].y = hity + (ny >> 14);
                    sprite[i].z = hitz;
                    sprite[i].cstat |= kSpriteWall|kSpriteStat6;
                    updatenumsprites();
                    asksave = 1;
                    ModifyBeep();
                }
                break;
            }
            case 1:
            case 2:
            {
                x = 0x4000;
                y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (searchy-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitsect < 0)
                {
                    Beep();
                    break;
                }
                x = hitx;
                y = hity;
                if (gGridLock > 0 && gGrid > 0)
                {
                    x = (x + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                    y = (y + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                }
                i = InsertObject(searchstat, hitsect, x, y, hitz, ang);
                if (i >= 0)
                {
                    asksave = 1;
                    ModifyBeep();
                }
                break;
            }
            default:
                Beep();
                break;
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                x = 0x4000;
                y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (searchy-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitwall != searchwall)
                    break;
                int nx, ny;
                int v64;
                if (somethingintab != 3)
                {
                    v64 = tilePick(-1, -1, 5);
                    if (v64 == -1)
                        break;
                }
                i = InsertSprite(searchsector, 0);
                sprite[i].ang = (GetWallAngle(hitwall)+512)&2047;
                GetWallNormal(hitwall, &nx, &ny);
                sprite[i].x = hitx + (nx >> 14);
                sprite[i].y = hity + (ny >> 14);
                sprite[i].z = hitz;

                if (somethingintab == 3)
                {
                    sprite[i].picnum = temppicnum;
                    sprite[i].shade = tempshade;
                    sprite[i].pal = temppal;
                    sprite[i].xrepeat = tempxrepeat;
                    sprite[i].yrepeat = tempyrepeat;
                    if (sprite[i].xrepeat < 1)
                        sprite[i].xrepeat = 1;
                    if (sprite[i].yrepeat < 1)
                        sprite[i].yrepeat = 1;

                    tempcstat &= ~kSpriteMask;
                    sprite[i].cstat = tempcstat | kSpriteWall|kSpriteStat6;
                }
                else
                {
                    sprite[i].cstat |= kSpriteWall|kSpriteStat6;
                    sprite[i].picnum = v64;
                    sprite[i].shade = -8;
                    sprite[i].pal = 0;
                    if (tilesizy[sprite[i].picnum] >= 32)
                        sprite[i].cstat |= kSpriteStat0;
                }
                updatenumsprites();
                ModifyBeep();
                break;
            }
            case 1:
            case 2:
            {
                x = 0x4000;
                y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (searchy-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitsect >= 0)
                {
                    x = hitx;
                    y = hity;
                    if (gGridLock > 0 && gGrid > 0)
                    {
                        x = (x + (0x400 >> gGrid)) & (0xffffffff << (11 - gGrid));
                        y = (y + (0x400 >> gGrid)) & (0xffffffff << (11 - gGrid));
                    }
                    int v64;
                    if (somethingintab != 3)
                    {
                        v64 = tilePick(-1, -1, 3);
                        if (v64 == -1)
                            break;
                    }
                    i = InsertSprite(hitsect, 0);
                    sprite[i].x = x;
                    sprite[i].y = y;

                    if (somethingintab == 3)
                    {
                        sprite[i].picnum = temppicnum;
                        sprite[i].shade = tempshade;
                        sprite[i].pal = temppal;
                        sprite[i].xrepeat = tempxrepeat;
                        sprite[i].yrepeat = tempyrepeat;
                        sprite[i].ang = tempang;
                        if (sprite[i].xrepeat < 1)
                            sprite[i].xrepeat = 1;
                        if (sprite[i].yrepeat < 1)
                            sprite[i].yrepeat = 1;

                        sprite[i].cstat = tempcstat;
                    }
                    else
                    {
                        sprite[i].picnum = v64;
                        sprite[i].shade = -8;
                        sprite[i].pal = 0;
                        if (tilesizy[sprite[i].picnum] >= 32)
                            sprite[i].cstat |= kSpriteStat0;
                    }
                    GetSpriteExtents(&sprite[i], &top, &bottom);
                    if (searchstat == 2)
                        sprite[i].z += getflorzofslope(hitsect, x, y) - bottom;
                    else
                        sprite[i].z += getceilzofslope(hitsect, x, y) - top;
                    func_10EA0();
                    updatenumsprites();
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            }
            default:
                Beep();
                break;
            }
        }
        break;
    case bsc_T:
        switch (searchstat)
        {
        case 4:
        {
            int vd = 0;
            if (wall[searchwall].cstat & kWallStat7)
                vd = 2;
            if (wall[searchwall].cstat & kWallStat9)
                vd = 1;
            vd = IncRotate(vd, 3);
            switch (vd)
            {
            case 0:
                wall[searchwall].cstat &= ~kWallStat7_9;
                break;
            case 1:
                wall[searchwall].cstat |= kWallStat7_9;
                break;
            case 2:
                wall[searchwall].cstat &= ~kWallStat7_9;
                wall[searchwall].cstat |= kWallStat7;
                break;
            }
            if (wall[searchwall].nextwall >= 0)
            {
                wall[wall[searchwall].nextwall].cstat &= ~kWallStat7_9;
                wall[wall[searchwall].nextwall].cstat |= wall[searchwall].cstat & kWallStat7_9;
            }
            sprintf(buffer, "wall[%i] translucent type %d", searchwall, vd);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        case 3:
        {
            int vd = 0;
            if (sprite[searchwall].cstat & kSpriteStat1)
                vd = 2;
            if (sprite[searchwall].cstat & kSpriteStat9)
                vd = 1;
            vd = IncRotate(vd, 3);
            switch (vd)
            {
            case 0:
                sprite[searchwall].cstat &= ~kSpriteStat1_9;
                break;
            case 1:
                sprite[searchwall].cstat |= kSpriteStat1_9;
                break;
            case 2:
                sprite[searchwall].cstat &= ~kSpriteStat1_9;
                sprite[searchwall].cstat |= kSpriteStat1;
                break;
            }
            sprintf(buffer, "sprite[%i] translucent type %d", searchwall, vd);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        default:
            Beep();
            break;
        }
        break;
    case bsc_U:
    {
        for (i = 0; i < numsectors; i++)
        {
            sector[i].visibility = 0;
        }
        scrSetMessage("All sector visibility values set to 0");
        break;
    }
    case bsc_V:
        switch (searchstat)
        {
        case 0:
        {
            short vc = wall[searchwall].picnum;
            wall[searchwall].picnum = tilePick(wall[searchwall].picnum, wall[searchwall].picnum, 0);
            vel = svel = angvel = 0;
            if (wall[searchwall].picnum != vc)
                ModifyBeep();
            break;
        }
        case 1:
        {
            short vc = sector[searchsector].ceilingpicnum;
            sector[searchsector].ceilingpicnum = tilePick(sector[searchsector].ceilingpicnum, sector[searchsector].ceilingpicnum, 1);
            vel = svel = angvel = 0;
            if (sector[searchsector].ceilingstat & kSectorStat0)
                SetSkyTile(sector[searchsector].ceilingpicnum);
            else
                sector[searchsector].floorstat &= ~kSectorStat15;
            if (sector[searchsector].ceilingpicnum != vc)
                ModifyBeep();
            break;
        }
        case 2:
        {
            short vc = sector[searchsector].floorpicnum;
            sector[searchsector].floorpicnum = tilePick(sector[searchsector].floorpicnum, sector[searchsector].floorpicnum, 2);
            vel = svel = angvel = 0;
            if (sector[searchsector].floorpicnum != vc)
                ModifyBeep();
            break;
        }
        case 3:
        {
            if (sprite[searchwall].cstat & kSpriteMask)
                searchstat = 5;
            int vc = sprite[searchwall].picnum;
            sprite[searchwall].picnum = tilePick(sprite[searchwall].picnum, sprite[searchwall].picnum, searchstat);
            GetSpriteExtents(&sprite[searchwall], &top, &bottom);
            if (!(sector[sprite[searchwall].sectnum].ceilingstat & kSectorStat0))
                sprite[searchwall].z += ClipLow(sector[sprite[searchwall].sectnum].ceilingz - top, 0);
            if (!(sector[sprite[searchwall].sectnum].floorstat & kSectorStat0))
                sprite[searchwall].z += ClipHigh(sector[sprite[searchwall].sectnum].floorz - bottom, 0);
            vel = svel = angvel = 0;
            if (sprite[searchwall].picnum != vc)
                ModifyBeep();
            break;
        }
        case 4:
        {
            short vc = wall[searchwall].overpicnum;
            wall[searchwall].overpicnum = tilePick(wall[searchwall].overpicnum, wall[searchwall].overpicnum, 4);
            vel = svel = angvel = 0;
            if (wall[searchwall].nextwall >= 0)
                wall[wall[searchwall].nextwall].overpicnum = wall[searchwall].overpicnum;
            if (wall[searchwall].overpicnum != vc)
                ModifyBeep();
            break;
        }
        }
        break;
    case bsc_W:
    {
        nXSector = func_10DBC(searchsector);
        int wf = xsector[nXSector].at11_0;
        do
        {
            wf = IncRotate(wf, 15);
        } while (!WaveForm[wf]);
        OperateSectors(func_2611C, wf);
        scrSetMessage(WaveForm[wf]);
        break;
    }
    case bsc_Pad_Minus:
    {
        step = ctrl ? 0x100 : 0x01;
        if (IsSectorHighlight(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                nSector = highlightsector[i];
                sector[nSector].ceilingshade = ClipHigh(sector[nSector].ceilingshade+step, 63);
                sector[nSector].floorshade = ClipHigh(sector[nSector].floorshade+step, 63);
                startwall = sector[nSector].wallptr;
                endwall = startwall + sector[nSector].wallnum - 1;
                for (j = startwall; j <= endwall; j++)
                {
                    wall[j].shade = ClipHigh(wall[j].shade+step, 63);
                }
            }
        }
        else
        {
            signed char v14;
            switch (searchstat)
            {
            case 0:
                v14 = wall[searchwall].shade = ClipHigh(wall[searchwall].shade+step, 63);
                break;
            case 1:
                v14 = sector[searchsector].ceilingshade = ClipHigh(sector[searchsector].ceilingshade+step, 63);
                break;
            case 2:
                v14 = sector[searchsector].floorshade = ClipHigh(sector[searchsector].floorshade+step, 63);
                break;
            case 3:
                v14 = sprite[searchwall].shade = ClipHigh(sprite[searchwall].shade+step, 63);
                break;
            case 4:
                v14 = wall[searchwall].shade = ClipHigh(wall[searchwall].shade+step, 63);
                break;
            }
            sprintf(buffer, "shade: %i", v14);
            scrSetMessage(buffer);
        }
        ModifyBeep();
        break;
    }
    case bsc_Pad_Plus:
    {
        step = ctrl ? 0x100 : 0x01;
        if (IsSectorHighlight(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                nSector = highlightsector[i];
                sector[nSector].ceilingshade = ClipLow(sector[nSector].ceilingshade-step, -128);
                sector[nSector].floorshade = ClipLow(sector[nSector].floorshade-step, -128);
                startwall = sector[nSector].wallptr;
                endwall = startwall + sector[nSector].wallnum - 1;
                for (j = startwall; j <= endwall; j++)
                {
                    wall[j].shade = ClipLow(wall[j].shade-1, -128);
                }
            }
        }
        else
        {
            signed char v14;
            switch (searchstat)
            {
            case 0:
                v14 = wall[searchwall].shade = ClipLow(wall[searchwall].shade-step, -128);
                break;
            case 1:
                v14 = sector[searchsector].ceilingshade = ClipLow(sector[searchsector].ceilingshade-step, -128);
                break;
            case 2:
                v14 = sector[searchsector].floorshade = ClipLow(sector[searchsector].floorshade-step, -128);
                break;
            case 3:
                v14 = sprite[searchwall].shade = ClipLow(sprite[searchwall].shade-step, -128);
                break;
            case 4:
                v14 = wall[searchwall].shade = ClipLow(wall[searchwall].shade-step, -128);
                break;
            }
            sprintf(buffer, "shade: %i", v14);
            scrSetMessage(buffer);
        }
        ModifyBeep();
        break;
    }
    case bsc_Pad_0:
        if (IsSectorHighlight(searchsector))
        {
            for (i = 0; i < highlightsectorcnt; i++)
            {
                nSector = highlightsector[i];
                sector[nSector].ceilingshade = 0;
                sector[nSector].floorshade = 0;
                startwall = sector[nSector].wallptr;
                endwall = startwall + sector[nSector].wallnum - 1;
                for (j = startwall; j <= endwall; j++)
                {
                    wall[j].shade = 0;
                }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
                wall[searchwall].shade = 0;
                break;
            case 1:
                sector[searchsector].ceilingshade = 0;
                break;
            case 2:
                sector[searchsector].floorshade = 0;
                break;
            case 3:
                sprite[searchwall].shade = 0;
                break;
            case 4:
                wall[searchwall].shade = 0;
                break;
            }
        }
        ModifyBeep();
        break;
    case bsc_Pad_4:
    case bsc_Pad_6:
    {
        char vbl = gOldKeyMapping ? kp5 : (char)!shift;
        char val = gOldKeyMapping ? shift : ctrl;
        changedir = key == bsc_Pad_4 ? 1 : -1;
        switch (searchstat)
        {
        case 0:
        case 4:
            if (val)
            {
                wall[searchwall].xpanning = changechar(wall[searchwall].xpanning, changedir, vbl, 0);
                sprintf(buffer, "wall %i xpanning: %i ypanning: %i", searchwall, wall[searchwall].xpanning, wall[searchwall].ypanning);
            }
            else
            {
                wall[searchwall].xrepeat = changechar(wall[searchwall].xrepeat, changedir, vbl, 1);
                sprintf(buffer, "wall %i xrepeat: %i yrepeat: %i", searchwall, wall[searchwall].xrepeat, wall[searchwall].yrepeat);
            }
            scrSetMessage(buffer);
            break;
        case 1:
            sector[searchsector].ceilingxpanning = changechar(sector[searchsector].ceilingxpanning, changedir, vbl, 0);
            break;
        case 2:
            sector[searchsector].floorxpanning = changechar(sector[searchsector].floorxpanning, changedir, vbl, 0);
            break;
        case 3:
            if (val)
            {
                sprite[searchwall].xoffset = changechar(sprite[searchwall].xoffset, changedir, vbl, 0);
                sprintf(buffer, "sprite %i xoffset: %i yoffset: %i", searchwall, sprite[searchwall].xoffset, sprite[searchwall].yoffset);
            }
            else
            {
                sprite[searchwall].xrepeat = ClipLow(changechar(sprite[searchwall].xrepeat, -changedir, vbl, 1), 4);
                sprintf(buffer, "sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
            }
            scrSetMessage(buffer);
            break;
        }
        ModifyBeep();
        break;
    }
    case bsc_Pad_2:
    case bsc_Pad_8:
    {
        char vbl = gOldKeyMapping ? kp5 : (char)!shift;
        char val = gOldKeyMapping ? shift : ctrl;
        changedir = key == bsc_Pad_8 ? -1 : 1;
        switch (searchstat)
        {
        case 0:
        case 4:
            if (val)
            {
                wall[searchwall].ypanning = changechar(wall[searchwall].ypanning, changedir, vbl, 0);
                sprintf(buffer, "wall %i xpanning: %i ypanning: %i", searchwall, wall[searchwall].xpanning, wall[searchwall].ypanning);
            }
            else
            {
                wall[searchwall].yrepeat = changechar(wall[searchwall].yrepeat, changedir, vbl, 1);
                sprintf(buffer, "wall %i xrepeat: %i yrepeat: %i", searchwall, wall[searchwall].xrepeat, wall[searchwall].yrepeat);
            }
            scrSetMessage(buffer);
            break;
        case 1:
            sector[searchsector].ceilingypanning = changechar(sector[searchsector].ceilingypanning, changedir, vbl, 0);
            break;
        case 2:
            sector[searchsector].floorypanning = changechar(sector[searchsector].floorypanning, changedir, vbl, 0);
            break;
        case 3:
            if (val)
            {
                sprite[searchwall].yoffset = changechar(sprite[searchwall].yoffset, changedir, vbl, 0);
                sprintf(buffer, "sprite %i xoffset: %i yoffset: %i", searchwall, sprite[searchwall].xoffset, sprite[searchwall].yoffset);
            }
            else
            {
                sprite[searchwall].yrepeat = ClipLow(changechar(sprite[searchwall].yrepeat, -changedir, vbl, 1), 4);
                sprintf(buffer, "sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
            }
            scrSetMessage(buffer);
            break;
        }
        ModifyBeep();
        break;
    }
    case bsc_Pad_Enter:
        overheadeditor();
        clearview(0);
        scrNextPage();
        keyFlushStream();
        func_1058C();
        break;
    case bsc_F2:
        switch (searchstat)
        {
        case 0:
        case 4:
        {
            nXWall = wall[searchwall].extra;
            if (nXWall > 0)
            {
                xwall[nXWall].at1_6 ^= 1;
                xwall[nXWall].at1_7 = xwall[nXWall].at1_6 << 16;
                scrSetMessage(int_D9A88[xwall[nXWall].at1_6]);
                ModifyBeep();
            }
            else
                Beep();
            break;
        }
        case 1:
        case 2:
        {
            nXSector = sector[searchsector].extra;
            if (nXSector > 0)
            {
                xsector[nXSector].at1_6 ^= 1;
                xsector[nXSector].at1_7 = xsector[nXSector].at1_6 << 16;
                scrSetMessage(int_D9A88[xsector[nXSector].at1_6]);
                ModifyBeep();
            }
            else
                Beep();
            break;
        }
        case 3:
        {
            nXSprite = sprite[searchwall].extra;
            if (nXSprite > 0)
            {
                xsprite[nXSprite].at1_6 ^= 1;
                xsprite[nXSprite].at1_7 = xsprite[nXSprite].at1_6 << 16;
                scrSetMessage(int_D9A88[xsprite[nXSprite].at1_6]);
                ModifyBeep();
            }
            else
                Beep();
            break;
        }
        }
        break;
    case bsc_F3:
    {
        nSector = searchsector;
        if (alt)
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    nSector = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                nXSector = sector[nSector].extra;
                if (nXSector > 0)
                {
                    xsector[nXSector].at24_0 = sector[nSector].floorz;
                    xsector[nXSector].at1c_0 = sector[nSector].ceilingz;
                    sprintf(buffer, "SET offFloorZ= %i  offCeilZ= %i", xsector[nXSector].at24_0, xsector[nXSector].at1c_0);
                    scrSetMessage(buffer);
                    ModifyBeep();
                    asksave = 1;
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    nSector = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                nXSector = sector[nSector].extra;
                if (nXSector > 0)
                {
                    switch (sector[nSector].type)
                    {
                    case 600:
                    case 617:
                    case 615:
                    case 616:
                    case 614:
                        sector[nSector].floorz = xsector[nXSector].at24_0;
                        sector[nSector].ceilingz = xsector[nXSector].at1c_0;
                        sprintf(buffer, "SET floorz= %i  ceilingz= %i", sector[nSector].floorz, sector[nSector].ceilingz);
                        scrSetMessage(buffer);
                        ModifyBeep();
                        asksave = 1;
                        break;
                    }
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        break;
    }
    case bsc_F4:
    {
        nSector = searchsector;
        if (alt)
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    nSector = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                nXSector = sector[nSector].extra;
                if (nXSector > 0)
                {
                    xsector[nXSector].at28_0 = sector[nSector].floorz;
                    xsector[nXSector].at20_0 = sector[nSector].ceilingz;
                    sprintf(buffer, "SET onFloorZ= %i  onCeilZ= %i", xsector[nXSector].at28_0, xsector[nXSector].at20_0);
                    scrSetMessage(buffer);
                    ModifyBeep();
                    asksave = 1;
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    nSector = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                nXSector = sector[nSector].extra;
                if (nXSector > 0)
                {
                    switch (sector[nSector].type)
                    {
                    case 600:
                    case 617:
                    case 615:
                    case 616:
                    case 614:
                        sector[nSector].floorz = xsector[nXSector].at28_0;
                        sector[nSector].ceilingz = xsector[nXSector].at20_0;
                        sprintf(buffer, "SET floorz= %i  ceilingz= %i", sector[nSector].floorz, sector[nSector].ceilingz);
                        scrSetMessage(buffer);
                        ModifyBeep();
                        asksave = 1;
                        break;
                    }
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        break;
    }
    case bsc_F9:
        asksave = ShowSectorTricks();
        break;
    case bsc_F11:
        char_CA89C = !char_CA89C;
        sprintf(buffer, "Global panning is %s", int_D9A88[char_CA89C]);
        scrSetMessage(buffer);
        ModifyBeep();
        break;
    case bsc_F12:
        gBeep = !gBeep;
        sprintf(buffer, "Beeps are %s", int_D9A88[gBeep]);
        scrSetMessage(buffer);
        ModifyBeep();
        break;
    case bsc_1:
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].cstat ^= kWallStat5;
            sprintf(buffer, "Wall %i one-way flag is %s", searchwall, int_D9A88[(wall[searchwall].cstat & kWallStat5) ? 1 : 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
            sprite[searchwall].cstat ^= kSpriteStat6;
            i = sprite[searchwall].cstat;
            if ((i & kSpriteMask) == kSpriteFloor)
            {
                sprite[searchwall].cstat &= ~kSpriteStat3;
                if (i & kSpriteStat6)
                {
                    if (posz > sprite[searchwall].z)
                    {
                        sprite[searchwall].cstat |= kSpriteStat3;
                    }
                }
            }
            sprintf(buffer, "Sprite %i one-sided flag is %s", searchwall, int_D9A88[(sprite[searchwall].cstat & kSpriteStat6) ? 1 : 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_2:
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].cstat ^= kWallStat1;
            sprintf(buffer, "Wall %i bottom swap flag is %s", searchwall, int_D9A88[(wall[searchwall].cstat & kWallStat1) ? 1 : 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case bsc_PrntScrn:
        screencapture("captxxxx.pcx",0);
        ModifyBeep();
        break;
    }
    if (key != 0)
        keyFlushStream();
}

