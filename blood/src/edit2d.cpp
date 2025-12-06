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
#include "key.h"
#include "misc.h"
#include "sound.h"
#include "trig.h"

#if APPVER_BLOODREV >= AV_BR_BL111A
#define LDIFF1 0
#define LDIFF2 0
#else
#define LDIFF1 -364
#define LDIFF2 -377
#endif

#if APPVER_BLOODREV >= AV_BR_BL111A
CXTracker gXTracker;

CXTracker::CXTracker()
{
    dprintf("CXTracker::CXTracker()\n");
    TrackClear();
}

void DrawVector(int x0, int y0, int x1, int y1, char c, int a5)
{
    drawline16(x0, y0, x1, y1, c);
    int ang = getangle(x0 - x1, y0 - y1);
    x0 = mulscale30(a5/64, Cos(ang+170));
    y0 = mulscale30(a5/64, Sin(ang+170));
    drawline16(x1, y1, x1 + x0, y1 + y0, c);
    x0 = mulscale30(a5/64, Cos(ang-170));
    y0 = mulscale30(a5/64, Sin(ang-170));
    drawline16(x1, y1, x1 + x0, y1 + y0, c);
}

void CXTracker::TrackClear()
{
    dprintf("CXTracker::TrackClear()\n");
    m_type = 0;
    m_nSector = m_nWall = m_nSprite = -1;
    m_pSector = 0;
    m_pWall = 0;
    m_pSprite = 0;
    m_pXSector = 0;
    m_pXWall = 0;
    m_pXSprite = 0;
}

void CXTracker::TrackSector(int nSector, char a2)
{
    dprintf("CXTracker::TrackSector( %d, %s )\n", nSector, a2 ? "TRUE" : "FALSE");
    TrackClear();
    if (nSector < 0 || nSector >= kMaxSectors)
        return;
    dprintf("sector is valid\n");
    SECTOR* pSector = &sector[nSector];
    if (pSector->extra <= 0 || pSector->extra >= kMaxXSprites) // ???
        return;
    dprintf("xsector is valid\n");
    XSECTOR* pXSector = &xsector[pSector->extra];
    if ((a2 && !pXSector->at6_0) || (!a2 && !pXSector->at8_0))
        return;
    dprintf("txID = %d,  rxID = %d\n", pXSector->at6_0, pXSector->at8_0);
    m_nSector = nSector;
    m_pSector = pSector;
    m_pXSector = pXSector;
    m_type = a2;
}

void CXTracker::TrackWall(int nWall, char a2)
{
    dprintf("CXTracker::TrackWall( %d, %s )\n", nWall, a2 ? "TRUE" : "FALSE");
    TrackClear();
    if (nWall < 0 || nWall >= kMaxWalls)
        return;
    dprintf("wall is valid\n");
    WALL* pWall = &wall[nWall];
    if (pWall->extra <= 0 || pWall->extra >= kMaxXSprites) // ???
        return;
    XWALL* pXWall = &xwall[pWall->extra];
    dprintf("xwall is valid\n");
    if ((a2 && !pXWall->at6_0) || (!a2 && !pXWall->at8_0))
        return;
    dprintf("txID = %d,  rxID = %d\n", pXWall->at6_0, pXWall->at8_0);
    m_nWall = nWall;
    m_pWall = pWall;
    m_pXWall = pXWall;
    m_type = a2;
}

void CXTracker::TrackSprite(int nSprite, char a2)
{
    dprintf("CXTracker::TrackSprite( %d, %s )\n", nSprite, a2 ? "TRUE" : "FALSE");
    TrackClear();
    if (nSprite < 0 || nSprite >= kMaxSprites)
        return;
    dprintf("sprite is valid\n");
    SPRITE* pSprite = &sprite[nSprite];
    if (pSprite->extra <= 0 || pSprite->extra >= kMaxXSprites) // ???
        return;
    dprintf("xsprite is valid\n");
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    if ((a2 && !pXSprite->at4_0) || (!a2 && !pXSprite->at5_2))
        return;
    dprintf("txID = %d,  rxID = %d\n", pXSprite->at4_0, pXSprite->at5_2);
    m_nSprite = nSprite;
    m_pSprite = pSprite;
    m_pXSprite = pXSprite;
    m_type = a2;
}

void CXTracker::Draw(int a1, int a2, int a3)
{
    int txId = 0;
    int rxId = 0;
    char v4 = (gFrameClock&16) ? 8 : 0;
    char color;
    int x0, y0;
    int x1, y1;
    if (m_type)
    {
        if (m_pXSector)
        {
            txId = m_pXSector->at6_0;
            WALL* pWall2 = &wall[m_pSector->wallptr];
            x0 = 320+mulscale(pWall2->x - a1, a3, 14);
            y0 = 200+mulscale(pWall2->y - a2, a3, 14);
        }
        else if (m_pXWall)
        {
            txId = m_pXWall->at6_0;
            WALL* pWall2 = &wall[m_pWall->point2];
            int x = m_pWall->x + (pWall2->x - m_pWall->x) / 2;
            int y = m_pWall->y + (pWall2->y - m_pWall->y) / 2;

            x0 = 320+mulscale(x - a1, a3, 14);
            y0 = 200+mulscale(y - a2, a3, 14);
        }
        else if (m_pXSprite)
        {
            txId = m_pXSprite->at4_0;
            x0 = 320+mulscale(m_pSprite->x - a1, a3, 14);
            y0 = 200+mulscale(m_pSprite->y - a2, a3, 14);
        }
        else
        {
            return;
        }
        if (txId == 0)
            return;
        for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
        {
            SPRITE* pSprite = &sprite[nSprite];
            if (pSprite->statnum < 0 || pSprite->statnum >= kMaxStatus || pSprite->extra <= 0)
                continue;
            XSPRITE* pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->at5_2 == txId)
            {
                x1 = 320 + mulscale(pSprite->x - a1, a3, 14);
                y1 = 200 + mulscale(pSprite->y - a2, a3, 14);
                color = kColor3 ^ v4;
                DrawVector(x0, y0, x1, y1, color, a3);
            }
        }
        for (int nWall = 0; nWall < numwalls; nWall++)
        {
            WALL* pWall = &wall[nWall];
            if (pWall->extra <= 0)
                continue;
            XWALL* pXWall = &xwall[pWall->extra];
            if (pXWall->at8_0 == txId)
            {
                WALL* pWall2 = &wall[pWall->point2];
                int x = pWall->x + (pWall2->x - pWall->x) / 2;
                int y = pWall->y + (pWall2->y - pWall->y) / 2;
                x1 = 320 + mulscale(x - a1, a3, 14);
                y1 = 200 + mulscale(y - a2, a3, 14);
                color = kColor4 ^ v4;
                DrawVector(x0, y0, x1, y1, color, a3);
            }
        }
        for (int nSector = 0; nSector < numsectors; nSector++)
        {
            SECTOR* pSector = &sector[nSector];
            if (pSector->extra <= 0)
                continue;
            XSECTOR* pXSector = &xsector[pSector->extra];
            if (pXSector->at8_0 == txId)
            {
                WALL* pWall = &wall[pSector->wallptr];
                x1 = 320 + mulscale(pWall->x - a1, a3, 14);
                y1 = 200 + mulscale(pWall->y - a2, a3, 14);
                color = kColor6 ^ v4;
                DrawVector(x0, y0, x1, y1, color, a3);
            }
        }
    }
    else
    {
        if (m_pXSector)
        {
            rxId = m_pXSector->at8_0;
            WALL* pWall2 = &wall[m_pSector->wallptr];
            x0 = 320+mulscale(pWall2->x - a1, a3, 14);
            y0 = 200+mulscale(pWall2->y - a2, a3, 14);
        }
        else if (m_pXWall)
        {
            rxId = m_pXWall->at8_0;
            WALL* pWall2 = &wall[m_pWall->point2];
            int x = m_pWall->x + (pWall2->x - m_pWall->x) / 2;
            int y = m_pWall->y + (pWall2->y - m_pWall->y) / 2;

            x0 = 320+mulscale(x - a1, a3, 14);
            y0 = 200+mulscale(y - a2, a3, 14);
        }
        else if (m_pXSprite)
        {
            rxId = m_pXSprite->at5_2;
            x0 = 320+mulscale(m_pSprite->x - a1, a3, 14);
            y0 = 200+mulscale(m_pSprite->y - a2, a3, 14);
        }
        else
        {
            return;
        }
        if (rxId == 0)
            return;
        for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
        {
            SPRITE* pSprite = &sprite[nSprite];
            if (pSprite->statnum < 0 || pSprite->statnum >= kMaxStatus || pSprite->extra <= 0)
                continue;
            XSPRITE* pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->at4_0 == rxId)
            {
                x1 = 320 + mulscale(pSprite->x - a1, a3, 14);
                y1 = 200 + mulscale(pSprite->y - a2, a3, 14);
                color = kColor11 ^ v4;
                DrawVector(x1, y1, x0, y0, color, a3);
            }
        }
        for (int nWall = 0; nWall < numwalls; nWall++)
        {
            WALL* pWall = &wall[nWall];
            if (pWall->extra <= 0)
                continue;
            XWALL* pXWall = &xwall[pWall->extra];
            if (pXWall->at6_0 == rxId)
            {
                WALL* pWall2 = &wall[pWall->point2];
                int x = pWall->x + (pWall2->x - pWall->x) / 2;
                int y = pWall->y + (pWall2->y - pWall->y) / 2;
                x1 = 320 + mulscale(x - a1, a3, 14);
                y1 = 200 + mulscale(y - a2, a3, 14);
                color = kColor12 ^ v4;
                DrawVector(x1, y1, x0, y0, color, a3);
            }
        }
        for (int nSector = 0; nSector < numsectors; nSector++)
        {
            SECTOR* pSector = &sector[nSector];
            if (pSector->extra <= 0)
                continue;
            XSECTOR* pXSector = &xsector[pSector->extra];
            if (pXSector->at6_0 == rxId)
            {
                WALL* pWall = &wall[pSector->wallptr];
                x1 = 320 + mulscale(pWall->x - a1, a3, 14);
                y1 = 200 + mulscale(pWall->y - a2, a3, 14);
                color = kColor14 ^ v4;
                DrawVector(x1, y1, x0, y0, color, a3);
            }
        }
    }
}
#endif


short sectorhighlight;

#if APPVER_BLOODREV >= AV_BR_BL111A
char char_CBA0C = 1;
#endif

char gTempBuf[256] = "";

enum CONTROL_TYPE
{
    CONTROL_TYPE_0 = 0, // LABEL
    CONTROL_TYPE_1, // NUMBER
    CONTROL_TYPE_2, // TOGGLEBUTTON
    RADIOBUTTON, // 3 RADIOBUTTON
    CONTROL_TYPE_4, // LIST
    CONTROL_TYPE_5,
    CONTROL_END = 255
};

struct CONTROL {
    int at0;
    int at4;
    int at8; // tag?
    CONTROL_TYPE type; // atc
    char* atd;
    int at11; // minvalue
    int at15; // maxvalue
    char** names; // at19
    unsigned char (*at1d)(CONTROL* control, unsigned char key);
    int at21; // value
};

unsigned char func_1BE80(CONTROL* control, unsigned char a2);
unsigned char func_1BFF4(CONTROL* control, unsigned char a2);
unsigned char func_1BFD8(CONTROL* control, unsigned char a2);

CONTROL controlXSprite[] = {
    { 0, 0, 1, CONTROL_TYPE_4, "Type %4d: %-18.18s", 0, 1023, int_D9A90, NULL, 0 },
    { 0, 1, 2, CONTROL_TYPE_1, "RX ID: %4d", 0, 1023, NULL, func_1BE80, 0 },
    { 0, 2, 3, CONTROL_TYPE_1, "TX ID: %4d", 0, 1023, NULL, func_1BE80, 0 },
    { 0, 3, 4, CONTROL_TYPE_4, "State %1d: %-3.3s", 0, 1, int_D9A88, NULL, 0 },
    { 0, 4, 5, CONTROL_TYPE_4, "Cmd: %3d: %-12.12s", 0, 255, int_DCB00, NULL, 0 },
    { 0, 5, 0, CONTROL_TYPE_0, "Send when:", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "going ON", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "going OFF", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_1, "busyTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 9, 9, CONTROL_TYPE_1, "waitTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 10, 10, CONTROL_TYPE_4, "restState %1d: %-3.3s", 0, 1, int_D9A88, NULL, 0 },
    { 30, 0, 0, CONTROL_TYPE_0, "Trigger On:", 0, 0, NULL, NULL, 0 },
    { 30, 1, 11, CONTROL_TYPE_2, "Push", 0, 0, NULL, NULL, 0 },
    { 30, 2, 12, CONTROL_TYPE_2, "Vector", 0, 0, NULL, NULL, 0 },
    { 30, 3, 13, CONTROL_TYPE_2, "Impact", 0, 0, NULL, NULL, 0 },
    { 30, 4, 14, CONTROL_TYPE_2, "Pickup", 0, 0, NULL, NULL, 0 },
    { 30, 5, 15, CONTROL_TYPE_2, "Touch", 0, 0, NULL, NULL, 0 },
    { 30, 6, 16, CONTROL_TYPE_2, "Sight", 0, 0, NULL, NULL, 0 },
    { 30, 7, 17, CONTROL_TYPE_2, "Proximity", 0, 0, NULL, NULL, 0 },
    { 30, 8, 18, CONTROL_TYPE_2, "DudeLockout", 0, 0, NULL, NULL, 0 },
    { 21, 9, 0, CONTROL_TYPE_0, "Launch 1 2 3 4 5 S B C T", 0, 0, NULL, NULL, 0 },
    { 28, 10, 19, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 30, 10, 20, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 32, 10, 21, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 34, 10, 22, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 36, 10, 23, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 38, 10, 24, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 40, 10, 25, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 42, 10, 26, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 44, 10, 27, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 46, 0, 0, CONTROL_TYPE_0, "Trigger Flags:", 0, 0, NULL, NULL, 0 },
    { 46, 1, 28, CONTROL_TYPE_2, "Decoupled", 0, 0, NULL, NULL, 0 },
    { 46, 2, 29, CONTROL_TYPE_2, "1-shot", 0, 0, NULL, NULL, 0 },
    { 46, 3, 30, CONTROL_TYPE_2, "Locked", 0, 0, NULL, NULL, 0 },
    { 46, 4, 31, CONTROL_TYPE_2, "Interruptable", 0, 0, NULL, NULL, 0 },
    { 46, 5, 32, CONTROL_TYPE_1, "Data1: %5d", 0, 65535, NULL, func_1BFF4, 0 },
    { 46, 6, 33, CONTROL_TYPE_1, "Data2: %5d", 0, 65535, NULL, func_1BFF4, 0 },
    { 46, 7, 34, CONTROL_TYPE_1, "Data3: %5d", 0, 65535, NULL, func_1BFF4, 0 },
    { 46, 8, 35, CONTROL_TYPE_1, "Data4: %5d", 0, 65535, NULL, func_1BFF4, 0 },
    { 46, 9, 36, CONTROL_TYPE_1, "Key: %1d", 0, 7, NULL, NULL, 0 },
    { 46, 10, 37, CONTROL_TYPE_4, "Wave: %1d %-8.8s", 0, 7, WaveForm2, NULL, 0 },
    { 62, 0, 0, CONTROL_TYPE_0, "Respawn:", 0, 0, NULL, NULL, 0 },
    { 62, 1, 38, CONTROL_TYPE_4, "When %1d: %-6.6s", 0, 3, int_DCF00, NULL, 0 },
    { 62, 3, 0, CONTROL_TYPE_0, "Dude Flags:", 0, 0, NULL, NULL, 0 },
    { 62, 4, 39, CONTROL_TYPE_2, "dudeDeaf", 0, 0, NULL, NULL, 0 },
    { 62, 5, 40, CONTROL_TYPE_2, "dudeAmbush", 0, 0, NULL, NULL, 0 },
    { 62, 6, 41, CONTROL_TYPE_2, "dudeGuard", 0, 0, NULL, NULL, 0 },
    { 62, 7, 42, CONTROL_TYPE_2, "reserved", 0, 0, NULL, NULL, 0 },
    { 62, 9, 43, CONTROL_TYPE_1, "Lock msg: %3d", 0, 255, NULL, NULL, 0 },
    { 62, 10, 44, CONTROL_TYPE_1, "Drop item: %3d", 0, 255, NULL, NULL, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

CONTROL controlXWall[] = {
    { 0, 0, 1, CONTROL_TYPE_4, "Type %4d: %-18.18s", 0, 1023, int_DAA90, NULL, 0 },
    { 0, 1, 2, CONTROL_TYPE_1, "RX ID: %4d", 0, 1023, NULL, func_1BE80, 0 },
    { 0, 2, 3, CONTROL_TYPE_1, "TX ID: %4d", 0, 1023, NULL, func_1BE80, 0 },
    { 0, 3, 4, CONTROL_TYPE_4, "State %1d: %-3.3s", 0, 1, int_D9A88, NULL, 0 },
    { 0, 4, 5, CONTROL_TYPE_4, "Cmd: %3d: %-12.12s", 0, 255, int_DCB00, NULL, 0 },
    { 0, 5, 0, CONTROL_TYPE_0, "Send when:", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "going ON", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "going OFF", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_1, "busyTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 9, 9, CONTROL_TYPE_1, "waitTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 10, 10, CONTROL_TYPE_4, "restState %1d: %-3.3s", 0, 1, int_D9A88, NULL, 0 },
    { 30, 0, 0, CONTROL_TYPE_0, "Trigger On:", 0, 0, NULL, NULL, 0 },
    { 30, 1, 11, CONTROL_TYPE_2, "Push", 0, 0, NULL, NULL, 0 },
    { 30, 2, 12, CONTROL_TYPE_2, "Vector", 0, 0, NULL, NULL, 0 },
    { 30, 3, 13, CONTROL_TYPE_2, "Reserved", 0, 0, NULL, NULL, 0 },
    { 30, 4, 14, CONTROL_TYPE_2, "DudeLockout", 0, 0, NULL, NULL, 0 },
    { 46, 0, 0, CONTROL_TYPE_0, "Trigger Flags:", 0, 0, NULL, NULL, 0 },
    { 46, 1, 15, CONTROL_TYPE_2, "Decoupled", 0, 0, NULL, NULL, 0 },
    { 46, 2, 16, CONTROL_TYPE_2, "1-shot", 0, 0, NULL, NULL, 0 },
    { 46, 3, 17, CONTROL_TYPE_2, "Locked", 0, 0, NULL, NULL, 0 },
    { 46, 4, 18, CONTROL_TYPE_2, "Interruptable", 0, 0, NULL, NULL, 0 },
    { 46, 6, 19, CONTROL_TYPE_1, "Data: %5d", 0, 65535, NULL, NULL, 0 },
    { 46, 7, 20, CONTROL_TYPE_1, "Key: %1d", 0, 7, NULL, NULL, 0 },
    { 46, 8, 21, CONTROL_TYPE_1, "panX = %4d", -128, 127, NULL, NULL, 0 },
    { 46, 9, 22, CONTROL_TYPE_1, "panY = %4d", -128, 127, NULL, NULL, 0 },
    { 46, 10, 23, CONTROL_TYPE_2, "panAlways", 0, 0, NULL, NULL, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

CONTROL controlXSector2[] = {
    { 0, 0, 0, CONTROL_TYPE_0, "Lighting:", 0, 0, NULL, NULL, 0 },
    { 0, 1, 1, CONTROL_TYPE_4, "Wave: %1d %-9.9s", 0, 15, WaveForm, NULL, 0 },
    { 0, 2, 2, CONTROL_TYPE_1, "Amplitude: %+4d", -128, 127, NULL, NULL, 0 },
    { 0, 3, 3, CONTROL_TYPE_1, "Freq:    %3d", 0, 255, NULL, NULL, 0 },
    { 0, 4, 4, CONTROL_TYPE_1, "Phase:     %3d", 0, 255, NULL, NULL, 0 },
    { 0, 5, 5, CONTROL_TYPE_2, "floor", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "ceiling", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "walls", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_2, "shadeAlways", 0, 0, NULL, NULL, 0 },
    { 20, 0, 0, CONTROL_TYPE_0, "More Lighting:", 0, 0, NULL, NULL, 0 },
    { 20, 1, 9, CONTROL_TYPE_2, "Color Lights", 0, 0, NULL, NULL, 0 },
    { 20, 2, 10, CONTROL_TYPE_1, "ceil  pal2 = %3d", 0, 15, NULL, NULL, 0 },
    { 20, 3, 11, CONTROL_TYPE_1, "floor pal2 = %3d", 0, 15, NULL, NULL, 0 },
    { 40, 0, 0, CONTROL_TYPE_0, "Motion FX:", 0, 0, NULL, NULL, 0 },
    { 40, 1, 12, CONTROL_TYPE_1, "Speed = %4d", 0, 255, NULL, NULL, 0 },
    { 40, 2, 13, CONTROL_TYPE_1, "Angle = %4d", 0, 2047, NULL, NULL, 0 },
    { 40, 3, 14, CONTROL_TYPE_2, "pan floor", 0, 0, NULL, NULL, 0 },
    { 40, 4, 15, CONTROL_TYPE_2, "pan ceiling", 0, 0, NULL, NULL, 0 },
    { 40, 5, 16, CONTROL_TYPE_2, "panAlways", 0, 0, NULL, NULL, 0 },
    { 40, 6, 17, CONTROL_TYPE_2, "drag", 0, 0, NULL, NULL, 0 },
    { 40, 8, 18, CONTROL_TYPE_1, "Wind vel: %4d", 0, 1023, NULL, NULL, 0 },
    { 40, 9, 19, CONTROL_TYPE_1, "Wind ang: %4d", 0, 2047, NULL, NULL, 0 },
    { 40, 10, 20, CONTROL_TYPE_2, "Wind always", 0, 0, NULL, NULL, 0 },
    { 60, 0, 0, CONTROL_TYPE_0, "Continuous motion:", 0, 0, NULL, NULL, 0 },
    { 60, 1, 21, CONTROL_TYPE_1, "Z range: %3d", 0, 31, NULL, NULL, 0 },
    { 60, 2, 22, CONTROL_TYPE_1, "Theta: %-4d", 0, 2047, NULL, NULL, 0 },
    { 60, 3, 23, CONTROL_TYPE_1, "Speed: %-4d", -2048, 2047, NULL, NULL, 0 },
    { 60, 4, 24, CONTROL_TYPE_2, "always", 0, 0, NULL, NULL, 0 },
    { 60, 5, 25, CONTROL_TYPE_2, "bob floor", 0, 0, NULL, NULL, 0 },
    { 60, 6, 26, CONTROL_TYPE_2, "bob ceiling", 0, 0, NULL, NULL, 0 },
    { 60, 7, 27, CONTROL_TYPE_2, "rotate", 0, 0, NULL, NULL, 0 },
    { 60, 9, 28, CONTROL_TYPE_1, "DamageType: %1d", 0, 5, NULL, NULL, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

CONTROL controlXSector[] = {
    { 0, 0, 1, CONTROL_TYPE_4, "Type %4d: %-16.16s", 0, 1023, int_DBA90, NULL, 0 },
    { 0, 1, 2, CONTROL_TYPE_1, "RX ID: %4d", 0, 1023, NULL, func_1BE80, 0 },
    { 0, 2, 3, CONTROL_TYPE_1, "TX ID: %4d", 0, 1023, NULL, func_1BE80, 0 },
    { 0, 3, 4, CONTROL_TYPE_4, "State %1d: %-3.3s", 0, 1, int_D9A88, NULL, 0 },
    { 0, 4, 5, CONTROL_TYPE_4, "Cmd: %3d: %-12.12s", 0, 255, int_DCB00, NULL, 0 },
    { 0, 5, 0, CONTROL_TYPE_0, "Trigger Flags:", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "Decoupled", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "1-shot", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_2, "Locked", 0, 0, NULL, NULL, 0 },
    { 0, 9, 9, CONTROL_TYPE_2, "Interruptable", 0, 0, NULL, NULL, 0 },
    { 0, 10, 10, CONTROL_TYPE_2, "DudeLockout", 0, 0, NULL, NULL, 0 },
    { 30, 0, 0, CONTROL_TYPE_0, "OFF->ON:", 0, 0, NULL, NULL, 0 },
    { 30, 1, 11, CONTROL_TYPE_2, "send at ON", 0, 0, NULL, NULL, 0 },
    { 30, 2, 12, CONTROL_TYPE_1, "busyTime = %3d", 0, 255, NULL, NULL, 0 },
    { 30, 3, 13, CONTROL_TYPE_4, "wave: %1d %-8.8s", 0, 7, WaveForm2, NULL, 0 },
    { 30, 4, 14, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 32, 4, 15, CONTROL_TYPE_1, "waitTime = %3d", 0, 255, NULL, NULL, 0 },
    { 30, 6, 0, CONTROL_TYPE_0, "ON->OFF:", 0, 0, NULL, NULL, 0 },
    { 30, 7, 16, CONTROL_TYPE_2, "send at OFF", 0, 0, NULL, NULL, 0 },
    { 30, 8, 17, CONTROL_TYPE_1, "busyTime = %3d", 0, 255, NULL, NULL, 0 },
    { 30, 9, 18, CONTROL_TYPE_4, "wave: %1d %-8.8s", 0, 7, WaveForm2, NULL, 0 },
    { 30, 10, 19, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 32, 10, 20, CONTROL_TYPE_1, "waitTime = %3d", 0, 255, NULL, NULL, 0 },
    { 48, 0, 0, CONTROL_TYPE_0, "Trigger On:", 0, 0, NULL, NULL, 0 },
    { 48, 1, 21, CONTROL_TYPE_2, "Push", 0, 0, NULL, NULL, 0 },
    { 48, 2, 22, CONTROL_TYPE_2, "Vector", 0, 0, NULL, NULL, 0 },
    { 48, 3, 23, CONTROL_TYPE_2, "Reserved", 0, 0, NULL, NULL, 0 },
    { 48, 4, 24, CONTROL_TYPE_2, "Enter", 0, 0, NULL, NULL, 0 },
    { 48, 5, 25, CONTROL_TYPE_2, "Exit", 0, 0, NULL, NULL, 0 },
    { 48, 6, 26, CONTROL_TYPE_2, "WallPush", 0, 0, NULL, NULL, 0 },
    { 60, 0, 27, CONTROL_TYPE_1, "Data: %5d", 0, 65535, NULL, NULL, 0 },
    { 60, 1, 28, CONTROL_TYPE_1, "Key: %1d", 0, 7, NULL, NULL, 0 },
    { 60, 2, 29, CONTROL_TYPE_1, "Depth = %1d", 0, 7, NULL, NULL, 0 },
    { 60, 3, 30, CONTROL_TYPE_2, "Underwater", 0, 0, NULL, NULL, 0 },
    { 60, 4, 31, CONTROL_TYPE_2, "Crush", 0, 0, NULL, NULL, 0 },
    { 60 ,10, 32, CONTROL_TYPE_5, "FX...", 0, 0, NULL, func_1BFD8, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

void PrintText(int x, int y, short c, short bc, char *pString)
{
    printext16(x*8+4, y*8+28, c, bc, pString, 0);
}

void ControlPrint(CONTROL* control, int a2)
{
    short vsi, vdi;
    vsi = 11;
    vdi = 8;
    if (a2)
    {
        vsi = 14;
        vdi = 0;
    }
    // Label?
    if (control->at8 == 0)
    {
        vsi = 15;
        vdi = 2;
    }
    switch (control->type)
    {
    case CONTROL_TYPE_0:
    case CONTROL_TYPE_5:
        PrintText(control->at0, control->at4, vsi, vdi, control->atd);
        break;
    case CONTROL_TYPE_1:
        sprintf(gTempBuf, control->atd, control->at21);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    case CONTROL_TYPE_4:
        dassert(control->names != NULL, 667+LDIFF1);
        sprintf(gTempBuf, control->atd, control->at21, control->names[control->at21]);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    case CONTROL_TYPE_2:
        sprintf(gTempBuf, "%c %s", control->at21 ? 3 : 2, control->atd);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    case RADIOBUTTON:
        sprintf(gTempBuf, "%c %s", control->at21 ? 5 : 4, control->atd);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    }
}

void ControlPrintList(CONTROL* control)
{
    clearmidstatbar16();
    for (CONTROL* ctrl = control; ctrl->type != CONTROL_END; ctrl++)
    {
        ControlPrint(ctrl, 0);
    }
}

CONTROL *ControlFindTag(CONTROL* control, int tag)
{
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END, 709+LDIFF1);

    if (control->type == RADIOBUTTON)
    {
        while (control->at21 == 0)
        {
            control++;
            dassert(control->type != CONTROL_END, 716+LDIFF1);
            dassert(control->type == RADIOBUTTON, 717+LDIFF1);
        }
    }
    return control;
}

void ControlSet(CONTROL* control, int tag, int value)
{
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END, 736+LDIFF1);

    control->at21 = value;
}

int ControlRead(CONTROL* control, int tag)
{
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END, 753+LDIFF1);

    return control->at21;
}

void func_1B988(CONTROL* _control, int tag, int value)
{
    for (CONTROL* control = _control; control->type != CONTROL_END; control++)
    {
        if (control->at8 == tag)
            break;
    }

    dassert(control->type != CONTROL_END, 770+LDIFF1);

    // ???
    while (control->type != CONTROL_END && control->at8 == tag)
    {
        control->at21 = value == 0;
        value--;
    }
    dassert(value < 0, 777+LDIFF1);
}

int func_1B9F4(CONTROL* control, int tag)
{
    int value = 0;
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END, 794+LDIFF1);

    // ???
    while (control->type != CONTROL_END)
    {
        dassert(control->type == RADIOBUTTON, 798+LDIFF1);
        if (control->at21 != 0)
            break;
        value++;
    }

    dassert(control->type != CONTROL_END, 803+LDIFF1);

    return value;
}

int ControlKeys(CONTROL* list)
{
    int tags = 0;
    int tag = 1;
    CONTROL* control;
    int t;
    unsigned char key;
    keyFlushStream();
    ControlPrintList(list);
    for (control = list; control->type != CONTROL_END; control++)
    {
        if (control->at8 > tags)
            tags = control->at8;
    }
    control = ControlFindTag(list, tag);
    while (1)
    {
        ControlPrint(control, 1);
        do
        {
            key = keyGet();
        } while (key == 0);
        if (control->at1d)
        {
            key = control->at1d(control, key);
            ControlPrintList(list);
        }
        switch (key)
        {
        case bsc_Enter:
        case bsc_Pad_Enter:
            keystatus[key] = 0;
            ModifyBeep();
            return 1;
        case bsc_Esc:
            keystatus[key] = 0;
            return 0;
        case bsc_Left:
            tag--;
            if (tag == 0)
                tag = tags;
            ControlPrint(control, 0);
            control = ControlFindTag(list, tag);
            break;
        case bsc_Right:
            tag++;
            if (tag > tags)
                tag = 1;
            ControlPrint(control, 0);
            control = ControlFindTag(list, tag);
            break;
        case bsc_Tab:
            if (keystatus[bsc_LShift] || keystatus[bsc_RShift])
            {
                tag--;
                if (tag == 0)
                    tag = tags;
                ControlPrint(control, 0);
                control = ControlFindTag(list, tag);
            }
            else
            {
                tag++;
                if (tag > tags)
                    tag = 1;
                ControlPrint(control, 0);
                control = ControlFindTag(list, tag);
            }
            break;
        default:
            switch (control->type)
            {
            case CONTROL_TYPE_1:
                switch (key)
                {
                case bsc_Backspace:
                    control->at21 /= 10;
                    break;
                case bsc_Up:
                case bsc_Pad_Plus:
                    control->at21++;
                    break;
                case bsc_Down:
                case bsc_Pad_Minus:
                    control->at21--;
                    break;
                case bsc_PgUp:
                    control->at21 = IncBy(control->at21, 10);
                    break;
                case bsc_PgDn:
                    control->at21 = DecBy(control->at21, 10);
                    break;
                default:
                    key = ScanToAscii[key];
                    if (key >= '0' && key <= '9')
                        control->at21 = control->at21 * 10 + key - '0';
                }
                if (control->at21 > control->at15)
                    control->at21 = control->at15;

                if (control->at21 < control->at11)
                    control->at21 = control->at11;

                break;
            case CONTROL_TYPE_2:
                switch (key)
                {
                case bsc_SpaceBar:
                    control->at21 = !control->at21;
                    break;
                }
                break;
            case RADIOBUTTON:
                ControlPrint(control, 0);
                switch (key)
                {
                case bsc_Up:
                    control->at21 = 0;
                    ControlPrint(control, 0);
                    control--;
                    if (control < list || control->at8 != tag)
                        control--;
                    control->at21 = 1;
                    break;
                case bsc_Down:
                    control->at21 = 0;
                    ControlPrint(control, 0);
                    control++;
                    if (control->at8 != tag)
                        control--;
                    control->at21 = 1;
                    break;
                }
                break;
            case CONTROL_TYPE_4:
            {
                t = control->at21;
                switch (key)
                {
                case bsc_Up:
                case bsc_Pad_Plus:
                    do
                    {
                        t++;
                    } while (t <= control->at15 && control->names[t] == NULL);
                    break;
                case bsc_Down:
                case bsc_Pad_Minus:
                    do
                    {
                        t--;
                    } while (t >= control->at11 && control->names[t] == NULL);
                    break;
                case bsc_PgUp:
                    do
                    {
                        t = IncBy(t, 10);
                    } while (t <= control->at15 && control->names[t] == NULL);
                    break;
                case bsc_PgDn:
                    do
                    {
                        t = DecBy(t, 10);
                    } while (t >= control->at11 && control->names[t] == NULL);
                    break;
                }

                if (t >= control->at11 && t <= control->at15 && control->names[t])
                    control->at21 = t;

                break;
            }

            }
        }
    }
}

unsigned char func_1BE80(CONTROL* control, unsigned char a2) // Next tx/rx id
{
    int i;
    char t[1024];
    switch (a2)
    {
        case bsc_F10:
            memset(t, 0, sizeof(t));
            for (i = 0; i < numsectors; i++)
            {
                int nXSector = sector[i].extra;
                if (nXSector <= 0)
                    continue;
                t[xsector[nXSector].at6_0] = 1;
                t[xsector[nXSector].at8_0] = 1;
            }
            for (i = 0; i < numwalls; i++)
            {
                int nXWall = wall[i].extra;
                if (nXWall <= 0)
                    continue;
                t[xwall[nXWall].at6_0] = 1;
                t[xwall[nXWall].at8_0] = 1;
            }
            for (i = 0; i < kMaxSprites; i++)
            {
                if (sprite[i].statnum >= kMaxStatus)
                    continue;
                int nXSprite = sprite[i].extra;
                if (nXSprite <= 0)
                    continue;
                t[xsprite[nXSprite].at4_0] = 1;
                t[xsprite[nXSprite].at5_2] = 1;
            }
            for (i = 100; i < 1024; i++)
            {
                if (!t[i])
                    break;
            }
            control->at21 = i;
            return 0;
    }
    return a2;
}

unsigned char func_1BFD8(CONTROL* control, unsigned char a2)
{
    switch (a2)
    {
        case bsc_Enter:
            if (ControlKeys(controlXSector2))
                return bsc_Enter;
            return 0;
    }
    return a2;
}

unsigned char func_1BFF4(CONTROL* control, unsigned char a2) // sound
{
    int i;
    DICTNODE* hSnd;
    switch (a2)
    {
        case bsc_Up:
            for (i = control->at21+1; i < 65535; i++)
            {
                if (gSoundRes.Lookup(i, "SFX") != NULL)
                {
                    control->at21 = i;
                    break;
                }
            }
            if (i == 65535)
                Beep();
            return 0;
        case bsc_Down:
            for (i = control->at21-1; i > 0; i--)
            {
                if (gSoundRes.Lookup(i, "SFX") != NULL)
                {
                    control->at21 = i;
                    break;
                }
            }
            return 0;
        case bsc_F10:
            hSnd = gSoundRes.Lookup(control->at21, "SFX");
            if (hSnd != NULL)
            {
                SFX* pSfx = (SFX*)gSoundRes.Load(hSnd);
                printmessage16(pSfx->rawName);
                sndStartSample(control->at21, FXVolume, 0, 0);
            }
            return 0;
    }
    return a2;
}

void XWallControlSet(int nWall)
{
    int nXWall = wall[nWall].extra;
    dassert(nXWall > 0 && nXWall < kMaxXWalls, 1158+LDIFF1);
    XWALL* pXWall = &xwall[nXWall];
    ControlSet(controlXWall, 1, wall[nWall].type); // type
    ControlSet(controlXWall, 2, pXWall->at8_0); // rx id
    ControlSet(controlXWall, 3, pXWall->at6_0); // tx id
    ControlSet(controlXWall, 4, pXWall->at1_6); // state
    ControlSet(controlXWall, 5, pXWall->at9_2); // cmd
    ControlSet(controlXWall, 6, pXWall->ata_2); // going on
    ControlSet(controlXWall, 7, pXWall->ata_3); // going off
    ControlSet(controlXWall, 8, pXWall->ata_4); // busyTime
    ControlSet(controlXWall, 9, pXWall->atc_0); // waitTime
    ControlSet(controlXWall, 10, pXWall->atd_4); // restState
    ControlSet(controlXWall, 11, pXWall->at10_5); // Push
    ControlSet(controlXWall, 12, pXWall->at10_6); // Vector
    ControlSet(controlXWall, 13, pXWall->at10_7); // Reserved
    ControlSet(controlXWall, 14, pXWall->at13_3); // DudeLockout
    ControlSet(controlXWall, 15, pXWall->atf_7); // Decoupled
    ControlSet(controlXWall, 16, pXWall->at10_0); // 1-Shot
    ControlSet(controlXWall, 17, pXWall->at13_2); // Locked
    ControlSet(controlXWall, 18, pXWall->atd_5); // Interruptable
    ControlSet(controlXWall, 19, pXWall->at4_0); // Data
    ControlSet(controlXWall, 20, pXWall->at10_2); // Key
    ControlSet(controlXWall, 21, pXWall->atd_7); // panX
    ControlSet(controlXWall, 22, pXWall->ate_7); // panY
    ControlSet(controlXWall, 23, pXWall->atd_6); // panAlways
}

void XWallControlRead(int nWall)
{
    int nXWall = wall[nWall].extra;
    dassert(nXWall > 0 && nXWall < kMaxXWalls, 1198+LDIFF1);
    XWALL* pXWall = &xwall[nXWall];
    wall[nWall].type = ControlRead(controlXWall, 1); // type
    pXWall->at8_0 = ControlRead(controlXWall, 2); // rx id
    pXWall->at6_0 = ControlRead(controlXWall, 3); // tx id
    pXWall->at1_6 = ControlRead(controlXWall, 4); // state
    pXWall->at9_2 = ControlRead(controlXWall, 5); // cmd
    pXWall->ata_2 = ControlRead(controlXWall, 6); // going on
    pXWall->ata_3 = ControlRead(controlXWall, 7); // going off
    pXWall->ata_4 = ControlRead(controlXWall, 8); // busyTime
    pXWall->atc_0 = ControlRead(controlXWall, 9); // waitTime
    pXWall->atd_4 = ControlRead(controlXWall, 10); // restState
    pXWall->at10_5 = ControlRead(controlXWall, 11); // Push
    pXWall->at10_6 = ControlRead(controlXWall, 12); // Vector
    pXWall->at10_7 = ControlRead(controlXWall, 13); // Reserved
    pXWall->at13_3 = ControlRead(controlXWall, 14); // DudeLockout
    pXWall->atf_7 = ControlRead(controlXWall, 15); // Decoupled
    pXWall->at10_0 = ControlRead(controlXWall, 16); // 1-Shot
    pXWall->at13_2 = ControlRead(controlXWall, 17); // Locked
    pXWall->atd_5 = ControlRead(controlXWall, 18); // Interruptable
    pXWall->at4_0 = ControlRead(controlXWall, 19); // Data
    pXWall->at10_2 = ControlRead(controlXWall, 20); // Key
    pXWall->atd_7 = ControlRead(controlXWall, 21); // panX
    pXWall->ate_7 = ControlRead(controlXWall, 22); // panY
    pXWall->atd_6 = ControlRead(controlXWall, 23); // panAlways
}

void XSpriteControlSet(int nSprite)
{
    int nXSprite = sprite[nSprite].extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites, 1238+LDIFF1);
    XSPRITE* pXSprite = &xsprite[nXSprite];
    ControlSet(controlXSprite, 1, sprite[nSprite].type); // type
    ControlSet(controlXSprite, 2, pXSprite->at5_2); // rx id
    ControlSet(controlXSprite, 3, pXSprite->at4_0); // tx id
    ControlSet(controlXSprite, 4, pXSprite->at1_6); // state
    ControlSet(controlXSprite, 5, pXSprite->at6_4); // cmd
    ControlSet(controlXSprite, 6, pXSprite->at7_4); // going on
    ControlSet(controlXSprite, 7, pXSprite->at7_5); // going off
    ControlSet(controlXSprite, 8, pXSprite->at8_0); // busyTime
    ControlSet(controlXSprite, 9, pXSprite->at9_4); // waitTime
    ControlSet(controlXSprite, 10, pXSprite->atb_0); // restState
    ControlSet(controlXSprite, 11, pXSprite->atd_6); // Push
    ControlSet(controlXSprite, 12, pXSprite->atd_7); // Vector
    ControlSet(controlXSprite, 13, pXSprite->ate_0); // Impact
    ControlSet(controlXSprite, 14, pXSprite->ate_1); // Pickup
    ControlSet(controlXSprite, 15, pXSprite->ate_2); // Touch
    ControlSet(controlXSprite, 16, pXSprite->ate_3); // Sight
    ControlSet(controlXSprite, 17, pXSprite->ate_4); // Proximity
    ControlSet(controlXSprite, 18, pXSprite->atf_7); // DudeLockout
    ControlSet(controlXSprite, 19, !((pXSprite->ate_7>>0)&1)); // Launch 1
    ControlSet(controlXSprite, 20, !((pXSprite->ate_7>>1)&1)); // Launch 2
    ControlSet(controlXSprite, 21, !((pXSprite->ate_7>>2)&1)); // Launch 3
    ControlSet(controlXSprite, 22, !((pXSprite->ate_7>>3)&1)); // Launch 4
    ControlSet(controlXSprite, 23, !((pXSprite->ate_7>>4)&1)); // Launch 5
    ControlSet(controlXSprite, 24, !pXSprite->atf_4); // Launch S
    ControlSet(controlXSprite, 25, !pXSprite->atf_5); // Launch B
    ControlSet(controlXSprite, 26, !pXSprite->atf_6); // Launch C
    ControlSet(controlXSprite, 27, !pXSprite->atb_7); // Launch T
    ControlSet(controlXSprite, 28, pXSprite->atd_0); // Decoupled
    ControlSet(controlXSprite, 29, pXSprite->atd_1); // 1-shot
    ControlSet(controlXSprite, 30, pXSprite->at17_5); // Locked
    ControlSet(controlXSprite, 31, pXSprite->atb_1); // Interruptable
    ControlSet(controlXSprite, 32, pXSprite->at10_0); // Data 1
    ControlSet(controlXSprite, 33, pXSprite->at12_0); // Data 2
    ControlSet(controlXSprite, 34, pXSprite->at14_0); // Data 3
    ControlSet(controlXSprite, 35, pXSprite->at18_2); // Data 4
    ControlSet(controlXSprite, 36, pXSprite->atd_3); // Key
    ControlSet(controlXSprite, 37, pXSprite->at7_6); // Wave
    ControlSet(controlXSprite, 38, pXSprite->at18_0); // Respawn option
    ControlSet(controlXSprite, 39, pXSprite->at1d_4); // dudeDeaf
    ControlSet(controlXSprite, 40, pXSprite->at1d_5); // dudeAmbush
    ControlSet(controlXSprite, 41, pXSprite->at1d_6); // dudeGuard
    ControlSet(controlXSprite, 42, pXSprite->at1d_7); // reserved
    ControlSet(controlXSprite, 43, pXSprite->at1b_0); // Lock msg
    ControlSet(controlXSprite, 44, pXSprite->atc_0); // Drop item
}

void XSpriteControlRead(int nSprite)
{
    int nXSprite = sprite[nSprite].extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites, 1303+LDIFF1);
    XSPRITE* pXSprite = &xsprite[nXSprite];
    sprite[nSprite].type = ControlRead(controlXSprite, 1); // type
    pXSprite->at5_2 = ControlRead(controlXSprite, 2); // rx id
    pXSprite->at4_0 = ControlRead(controlXSprite, 3); // tx id
    pXSprite->at1_6 = ControlRead(controlXSprite, 4); // state
    pXSprite->at6_4 = ControlRead(controlXSprite, 5); // cmd
    pXSprite->at7_4 = ControlRead(controlXSprite, 6); // going on
    pXSprite->at7_5 = ControlRead(controlXSprite, 7); // going off
    pXSprite->at8_0 = ControlRead(controlXSprite, 8); // busyTime
    pXSprite->at9_4 = ControlRead(controlXSprite, 9); // waitTime
    pXSprite->atb_0 = ControlRead(controlXSprite, 10); // restState
    pXSprite->atd_6 = ControlRead(controlXSprite, 11); // Push
    pXSprite->atd_7 = ControlRead(controlXSprite, 12); // Vector
    pXSprite->ate_0 = ControlRead(controlXSprite, 13); // Impact
    pXSprite->ate_1 = ControlRead(controlXSprite, 14); // Pickup
    pXSprite->ate_2 = ControlRead(controlXSprite, 15); // Touch
    pXSprite->ate_3 = ControlRead(controlXSprite, 16); // Sight
    pXSprite->ate_4 = ControlRead(controlXSprite, 17); // Proximity
    pXSprite->atf_7 = ControlRead(controlXSprite, 18); // DudeLockout
    pXSprite->ate_7 = (!ControlRead(controlXSprite, 19) << 0) // Launch 12345
                     | (!ControlRead(controlXSprite, 20) << 1)
                     | (!ControlRead(controlXSprite, 21) << 2)
                     | (!ControlRead(controlXSprite, 22) << 3)
                     | (!ControlRead(controlXSprite, 23) << 4);
    pXSprite->atf_4 = !ControlRead(controlXSprite, 24); // Launch S
    pXSprite->atf_5 = !ControlRead(controlXSprite, 25); // Launch B
    pXSprite->atf_6 = !ControlRead(controlXSprite, 26); // Launch C
    pXSprite->atb_7 = !ControlRead(controlXSprite, 27); // Launch T
    pXSprite->atd_0 = ControlRead(controlXSprite, 28); // Decoupled
    pXSprite->atd_1 = ControlRead(controlXSprite, 29); // 1-shot
    pXSprite->at17_5 = ControlRead(controlXSprite, 30); // Locked
    pXSprite->atb_1 = ControlRead(controlXSprite, 31); // Interruptable
    pXSprite->at10_0 = ControlRead(controlXSprite, 32); // Data 1
    pXSprite->at12_0 = ControlRead(controlXSprite, 33); // Data 2
    pXSprite->at14_0 = ControlRead(controlXSprite, 34); // Data 3
    pXSprite->at18_2 = ControlRead(controlXSprite, 35); // Data 4
    pXSprite->atd_3 = ControlRead(controlXSprite, 36); // Key
    pXSprite->at7_6 = ControlRead(controlXSprite, 37); // Wave
    pXSprite->at18_0 = ControlRead(controlXSprite, 38); // Respawn option
    pXSprite->at1d_4 = ControlRead(controlXSprite, 39); // dudeDeaf
    pXSprite->at1d_5 = ControlRead(controlXSprite, 40); // dudeAmbush
    pXSprite->at1d_6 = ControlRead(controlXSprite, 41); // dudeGuard
    pXSprite->at1d_7 = ControlRead(controlXSprite, 42); // reserved
    pXSprite->at1b_0 = ControlRead(controlXSprite, 43); // Lock msg
    pXSprite->atc_0 = ControlRead(controlXSprite, 44); // Drop item
}

void XSectorControlSet(int nSector)
{
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors, 1369+LDIFF1);
    XSECTOR* pXSector = &xsector[nXSector];
    ControlSet(controlXSector, 1, sector[nSector].type); // type
    ControlSet(controlXSector, 2, pXSector->at8_0); // rx id
    ControlSet(controlXSector, 3, pXSector->at6_0); // tx id
    ControlSet(controlXSector, 4, pXSector->at1_6); // state
    ControlSet(controlXSector, 5, pXSector->at9_2); // cmd
    ControlSet(controlXSector, 6, pXSector->at16_4); // Decoupled
    ControlSet(controlXSector, 7, pXSector->at16_5); // 1-shot
    ControlSet(controlXSector, 8, pXSector->at35_0); // Locked
    ControlSet(controlXSector, 9, pXSector->atd_5); // Interruptable
    ControlSet(controlXSector, 10, pXSector->at37_7); // DudeLockout
    ControlSet(controlXSector, 11, pXSector->ata_2); // Send at ON
    ControlSet(controlXSector, 12, pXSector->ata_4); // OFF->ON busyTime
    ControlSet(controlXSector, 13, pXSector->at7_2); // OFF->ON wave
    ControlSet(controlXSector, 14, pXSector->atf_6); // OFF->ON wait
    ControlSet(controlXSector, 15, pXSector->atc_0); // OFF->ON waitTime
    ControlSet(controlXSector, 16, pXSector->ata_3); // Send at OFF
    ControlSet(controlXSector, 17, pXSector->at18_2); // ON->OFF busyTime
    ControlSet(controlXSector, 18, pXSector->at7_5); // ON->OFF wave
    ControlSet(controlXSector, 19, pXSector->atf_7); // ON->OFF wait
    ControlSet(controlXSector, 20, pXSector->at19_6); // ON->OFF waitTime
    ControlSet(controlXSector, 21, pXSector->at17_2); // Push
    ControlSet(controlXSector, 22, pXSector->at17_3); // Vector
    ControlSet(controlXSector, 23, pXSector->at17_4); // Reserved
    ControlSet(controlXSector, 24, pXSector->at17_5); // Enter
    ControlSet(controlXSector, 25, pXSector->at17_6); // Exit
    ControlSet(controlXSector, 26, pXSector->at17_7); // WallPush
    ControlSet(controlXSector, 27, pXSector->at4_0); // Data
    ControlSet(controlXSector, 28, pXSector->at16_7); // Key
    ControlSet(controlXSector, 29, pXSector->at13_5); // Depth
    ControlSet(controlXSector, 30, pXSector->at13_4); // Underwater
    ControlSet(controlXSector, 31, pXSector->at30_0); // Crush
    ControlSet(controlXSector2, 1, pXSector->at11_0); // Lighting wave
    ControlSet(controlXSector2, 2, pXSector->atd_6); // Lighting amplitude
    ControlSet(controlXSector2, 3, pXSector->ate_6); // Lighting freq
    ControlSet(controlXSector2, 4, pXSector->at10_0); // Lighting phase
    ControlSet(controlXSector2, 5, pXSector->at11_5); // Lighting floor
    ControlSet(controlXSector2, 6, pXSector->at11_6); // Lighting ceiling
    ControlSet(controlXSector2, 7, pXSector->at11_7); // Lighting walls
    ControlSet(controlXSector2, 8, pXSector->at11_4); // Lighting shadeAlways
    ControlSet(controlXSector2, 9, pXSector->at18_0); // Color Lights
    ControlSet(controlXSector2, 10, pXSector->at1b_4); // Ceil pal2
    ControlSet(controlXSector2, 11, pXSector->at33_4); // floor pal2
    ControlSet(controlXSector2, 12, pXSector->at14_0); // Motion speed
    ControlSet(controlXSector2, 13, pXSector->at15_0); // Motion angle
    ControlSet(controlXSector2, 14, pXSector->at13_1); // Pan floor
    ControlSet(controlXSector2, 15, pXSector->at13_2); // Pan ceiling
    ControlSet(controlXSector2, 16, pXSector->at13_0); // Pan always
    ControlSet(controlXSector2, 17, pXSector->at13_3); // Pan drag
    ControlSet(controlXSector2, 18, pXSector->at35_1); // Wind vel
    ControlSet(controlXSector2, 19, pXSector->at36_3); // Wind ang
    ControlSet(controlXSector2, 20, pXSector->at37_6); // Wind always
    ControlSet(controlXSector2, 21, pXSector->at39_3); // Motion Z range
    ControlSet(controlXSector2, 22, pXSector->at38_0); // Motion Theta
    ControlSet(controlXSector2, 23, pXSector->at3a_0); // Motion speed
    ControlSet(controlXSector2, 24, pXSector->at3b_4); // Motion always
    ControlSet(controlXSector2, 25, pXSector->at3b_5); // Motion bob floor
    ControlSet(controlXSector2, 26, pXSector->at3b_6); // Motion bob ceiling
    ControlSet(controlXSector2, 27, pXSector->at3b_7); // Motion rotate
    ControlSet(controlXSector2, 28, pXSector->at33_1); // DamageType
}

void XSectorControlRead(int nSector)
{
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors, 1451+LDIFF1);
    XSECTOR* pXSector = &xsector[nXSector];
    sector[nSector].type = ControlRead(controlXSector, 1); // type
    pXSector->at8_0 = ControlRead(controlXSector, 2); // rx id
    pXSector->at6_0 = ControlRead(controlXSector, 3); // tx id
    pXSector->at1_6 = ControlRead(controlXSector, 4); // state
    pXSector->at9_2 = ControlRead(controlXSector, 5); // cmd
    pXSector->at16_4 = ControlRead(controlXSector, 6); // Decoupled
    pXSector->at16_5 = ControlRead(controlXSector, 7); // 1-shot
    pXSector->at35_0 = ControlRead(controlXSector, 8); // Locked
    pXSector->atd_5 = ControlRead(controlXSector, 9); // Interruptable
    pXSector->at37_7 = ControlRead(controlXSector, 10); // DudeLockout
    pXSector->ata_2 = ControlRead(controlXSector, 11); // Send at ON
    pXSector->ata_4 = ControlRead(controlXSector, 12); // OFF->ON busyTime
    pXSector->at7_2 = ControlRead(controlXSector, 13); // OFF->ON wave
    pXSector->atf_6 = ControlRead(controlXSector, 14); // OFF->ON wait
    pXSector->atc_0 = ControlRead(controlXSector, 15); // OFF->ON waitTime
    pXSector->ata_3 = ControlRead(controlXSector, 16); // Send at OFF
    pXSector->at18_2 = ControlRead(controlXSector, 17); // ON->OFF busyTime
    pXSector->at7_5 = ControlRead(controlXSector, 18); // ON->OFF wave
    pXSector->atf_7 = ControlRead(controlXSector, 19); // ON->OFF wait
    pXSector->at19_6 = ControlRead(controlXSector, 20); // ON->OFF waitTime
    pXSector->at17_2 = ControlRead(controlXSector, 21); // Push
    pXSector->at17_3 = ControlRead(controlXSector, 22); // Vector
    pXSector->at17_4 = ControlRead(controlXSector, 23); // Reserved
    pXSector->at17_5 = ControlRead(controlXSector, 24); // Enter
    pXSector->at17_6 = ControlRead(controlXSector, 25); // Exit
    pXSector->at17_7 = ControlRead(controlXSector, 26); // WallPush
    pXSector->at4_0 = ControlRead(controlXSector, 27); // Data
    pXSector->at16_7 = ControlRead(controlXSector, 28); // Key
    pXSector->at13_5 = ControlRead(controlXSector, 29); // Depth
    pXSector->at13_4 = ControlRead(controlXSector, 30); // Underwater
    pXSector->at30_0 = ControlRead(controlXSector, 31); // Crush
    pXSector->at11_0 = ControlRead(controlXSector2, 1); // Lighting wave
    pXSector->atd_6 = ControlRead(controlXSector2, 2); // Lighting amplitude
    pXSector->ate_6 = ControlRead(controlXSector2, 3); // Lighting freq
    pXSector->at10_0 = ControlRead(controlXSector2, 4); // Lighting phase
    pXSector->at11_5 = ControlRead(controlXSector2, 5); // Lighting floor
    pXSector->at11_6 = ControlRead(controlXSector2, 6); // Lighting ceiling
    pXSector->at11_7 = ControlRead(controlXSector2, 7); // Lighting walls
    pXSector->at11_4 = ControlRead(controlXSector2, 8); // Lighting shadeAlways
    pXSector->at18_0 = ControlRead(controlXSector2, 9); // Color Lights
    pXSector->at1b_4 = ControlRead(controlXSector2, 10); // Ceil pal2
    pXSector->at33_4 = ControlRead(controlXSector2, 11); // floor pal2
    pXSector->at14_0 = ControlRead(controlXSector2, 12); // Motion speed
    pXSector->at15_0 = ControlRead(controlXSector2, 13); // Motion angle
    pXSector->at13_1 = ControlRead(controlXSector2, 14); // Pan floor
    pXSector->at13_2 = ControlRead(controlXSector2, 15); // Pan ceiling
    pXSector->at13_0 = ControlRead(controlXSector2, 16); // Pan always
    pXSector->at13_3 = ControlRead(controlXSector2, 17); // Pan drag
    pXSector->at35_1 = ControlRead(controlXSector2, 18); // Wind vel
    pXSector->at36_3 = ControlRead(controlXSector2, 19); // Wind ang
    pXSector->at37_6 = ControlRead(controlXSector2, 20); // Wind always
    pXSector->at39_3 = ControlRead(controlXSector2, 21); // Motion Z range
    pXSector->at38_0 = ControlRead(controlXSector2, 22); // Motion Theta
    pXSector->at3a_0 = ControlRead(controlXSector2, 23); // Motion speed
    pXSector->at3b_4 = ControlRead(controlXSector2, 24); // Motion always
    pXSector->at3b_5 = ControlRead(controlXSector2, 25); // Motion bob floor
    pXSector->at3b_6 = ControlRead(controlXSector2, 26); // Motion bob ceiling
    pXSector->at3b_7 = ControlRead(controlXSector2, 27); // Motion rotate
    pXSector->at33_1 = ControlRead(controlXSector2, 28); // DamageType
}

int getpointhighlight(int x, int y) // Replace
{
    int i;
    int nMinDist = divscale(gHighlightThreshold, gZoom, 14);
    int nStat = -1;
    for (i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            int nDist = approxDist(x-sprite[i].x, y-sprite[i].y);
            if (nDist < nMinDist)
            {
                nMinDist = nDist;
                nStat = i | 0x4000;
            }
        }
    }

    for (i = 0; i < numwalls; i++)
    {
        int nDist = approxDist(x-wall[i].x, y-wall[i].y);
        if (nDist < nMinDist)
        {
            nMinDist = nDist;
            nStat = i;
        }
    }

    return nStat;
}

void ShowSectorData(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 1571+LDIFF1);
    sprintf(gTempBuf, "Sector %d", nSector);
    printmessage16(gTempBuf);
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        dassert(nXSector < kMaxXSectors, 1580+LDIFF1);
        XSectorControlSet(nSector);
        ControlPrintList(controlXSector);
    }
    else
        clearmidstatbar16();
}

void EditSectorData(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 1599+LDIFF1);
    func_1058C();
    sprintf(gTempBuf, "Sector %d", nSector);
    printmessage16(gTempBuf);
    func_10DBC(nSector);
    XSectorControlSet(nSector);
    if (ControlKeys(controlXSector))
        XSectorControlRead(nSector);
    ShowSectorData(nSector);
    angvel = 0;
    svel = 0;
    vel = 0;
    func_1058C();
}

void ShowWallData(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls, 1627+LDIFF1);
    int nLen = approxDist(wall[wall[nWall].point2].x-wall[nWall].x, wall[wall[nWall].point2].y-wall[nWall].y);
    sprintf(gTempBuf, "Wall %d:  Length = %d", nWall, nLen);
    printmessage16(gTempBuf);
    int nXWall = wall[nWall].extra;
    if (nXWall > 0)
    {
        dassert(nXWall < kMaxXWalls, 1638+LDIFF1);
        XWallControlSet(nWall);
        ControlPrintList(controlXWall);
    }
    else
        clearmidstatbar16();
}

void EditWallData(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls, 1659+LDIFF1);
    func_1058C();
    sprintf(gTempBuf, "Wall %d", nWall);
    printmessage16(gTempBuf);
    func_10E08(nWall);
    XWallControlSet(nWall);
    if (ControlKeys(controlXWall))
        XWallControlRead(nWall);
    ShowWallData(nWall);
    angvel = 0;
    svel = 0;
    vel = 0;
    func_1058C();
}

void ShowSpriteData(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1689+LDIFF1);
    if (sprite[nSprite].extra > 0)
    {
        int nXSprite = sprite[nSprite].extra;
        sprintf(gTempBuf, "Sprite %d  Extra %d XRef %d", nSprite, nXSprite, xsprite[nXSprite].reference);
    }
    else
        sprintf(gTempBuf, "Sprite %d", nSprite);
    printmessage16(gTempBuf);
    int nXSprite = sprite[nSprite].extra;
    if (nXSprite > 0)
    {
        dassert(nXSprite < kMaxXSprites, 1707+LDIFF1);
        XSpriteControlSet(nSprite);
        ControlPrintList(controlXSprite);
    }
    else
        clearmidstatbar16();
}

void EditSpriteData(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1726+LDIFF1);
    func_1058C();
    sprintf(gTempBuf, "Sprite %d", nSprite);
    printmessage16(gTempBuf);
    func_10E50(nSprite);
    XSpriteControlSet(nSprite);
    if (ControlKeys(controlXSprite))
        XSpriteControlRead(nSprite);
    ShowSpriteData(nSprite);
    angvel = 0;
    svel = 0;
    vel = 0;
    func_1058C();
    func_10EA0();
}

int getlinehighlight(int x, int y) // Replace
{
    int wx;
    int wy;
    int nDist;
    int nMinDist;
    int nStat;

    nMinDist = divscale(gHighlightThreshold, gZoom, 14);
    nStat = -1;
    for (int i = 0; i < numwalls; i++)
    {
        int dx2 = wall[wall[i].point2].x - wall[i].x;
        int dy2 = wall[wall[i].point2].y - wall[i].y;
        int dx1 = x - wall[i].x;
        int dy1 = y - wall[i].y;
        if (dx1 * dy2 > dx2 * dy1) // inside
            continue;

        int dot = dmulscale(dx1, dx2, dy1, dy2, 4);
        int den = dmulscale(dx2, dx2, dy2, dy2, 4);
        if (dot > 0 && den > dot)
        {
            dassert(den > 0, 1771+LDIFF1);
            wx = wall[i].x+kscale(dx2, dot, den);
            wy = wall[i].y+kscale(dy2, dot, den);
            nDist = approxDist(wx-x, wy-y);
            if (nDist <= nMinDist)
            {
                nMinDist = nDist;
                nStat = i;
            }
        }
    }
    return nStat;
}

void DrawLine(int x1, int y1, int x2, int y2, int c, int a6) // DrawLine
{
    if ((x1 >= 0 || x2 >= 0) && (x1 < 640 || x2 < 640) && (y1 >= 0 || y2 >= 0) && (y1 < ydim16 || y2 < ydim16))
    {
        drawline16(x1, y1, x2, y2, c);
        if (a6)
        {
            long dx = x2 - x1;
            long dy = y2 - y1;
            RotateVector(&dx, &dy, 128);
            switch (GetOctant(dx, dy))
            {
            case 0:
            case 4:
                drawline16(x1, y1-1, x2, y2-1, c);
                drawline16(x1, y1+1, x2, y2+1, c);
                break;
            case 1:
            case 5:
                if (klabs(dx) < klabs(dy))
                {
                    drawline16(x1-1, y1+1, x2-1, y2+1, c);
                    drawline16(x1-1, y1, x2, y2+1, c);
                    drawline16(x1+1, y1, x2+1, y2, c);
                }
                else
                {
                    drawline16(x1-1, y1+1, x2-1, y2+1, c);
                    drawline16(x1, y1+1, x2, y2+1, c);
                    drawline16(x1, y1+1, x2, y2+1, c);
                }
                break;
            case 2:
            case 6:
                drawline16(x1-1, y1, x2-1, y2, c);
                drawline16(x1+1, y1, x2+1, y2, c);
                break;
            case 3:
            case 7:
                if (klabs(dx) < klabs(dy))
                {
                    drawline16(x1-1, y1-1, x2-1, y2-1, c);
                    drawline16(x1-1, y1, x2-1, y2, c);
                    drawline16(x1+1, y1, x2+1, y2, c);
                }
                else
                {
                    drawline16(x1-1, y1-1, x2-1, y2-1, c);
                    drawline16(x1, y1-1, x2, y2-1, c);
                    drawline16(x1, y1+1, x2, y2+1, c);
                }
                break;
            }
        }
    }
}

void setcolor16(int);
#pragma aux setcolor16 =\
	"mov dx, 0x3ce",\
	"shl ax, 8",\
	"out dx, ax",\
	parm [eax]\
	modify exact [eax edx]\

void drawpixel16(int);
#pragma aux drawpixel16 =\
	"mov ecx, edi",\
	"and cl, 7",\
	"xor cl, 7",\
	"mov ah, 1",\
	"shl ah, cl",\
	"mov dx, 0x3ce",\
	"mov al, 8",\
	"out dx, ax",\
	"shr edi, 3",\
	"or byte ptr [edi+0xa0000], al",\
	parm [edi]\
	modify exact [eax ecx edx edi]\

void DrawVertex(int x, int y, int c) // DrawVertex
{
    int offset = 640 * y + x + pageoffset;
    setcolor16(c);
    drawpixel16(offset-640*2-2);
    drawpixel16(offset-640*2-1);
    drawpixel16(offset-640*2-0);
    drawpixel16(offset-640*2+1);
    drawpixel16(offset-640*2+2);
    drawpixel16(offset-640*1-2);
    drawpixel16(offset-640*0-2);
    drawpixel16(offset+640*1-2);
    drawpixel16(offset-640*1+2);
    drawpixel16(offset-640*0+2);
    drawpixel16(offset+640*1+2);
    drawpixel16(offset+640*2-2);
    drawpixel16(offset+640*2-1);
    drawpixel16(offset+640*2-0);
    drawpixel16(offset+640*2+1);
    drawpixel16(offset+640*2+2);
}

void DrawFaceSprite(int x, int y, int c)
{
    int offset = 640 * y + x + pageoffset;
    setcolor16(c);
    drawpixel16(offset-640*3-1);
    drawpixel16(offset-640*3-0);
    drawpixel16(offset-640*3+1);
    drawpixel16(offset-640*2-2);
    drawpixel16(offset-640*2+2);
    drawpixel16(offset-640*1-3);
    drawpixel16(offset-640*1+3);
    drawpixel16(offset-640*0-3);
    drawpixel16(offset-640*0+3);
    drawpixel16(offset+640*1-3);
    drawpixel16(offset+640*1+3);
    drawpixel16(offset+640*2-2);
    drawpixel16(offset+640*2+2);
    drawpixel16(offset+640*3-1);
    drawpixel16(offset+640*3-0);
    drawpixel16(offset+640*3+1);
}

void DrawFloorSprite(int x, int y, char c)
{
    drawline16(x-3, y-3, x+3, y-3, c);
    drawline16(x-3, y+3, x+3, y+3, c);
    drawline16(x-3, y-3, x-3, y+3, c);
    drawline16(x+3, y-3, x+3, y+3, c);
}

void DrawMarkerSprite(int x, int y, int c)
{
    int offset = 640 * y + x + pageoffset;
    setcolor16(c);
    drawpixel16(offset-640*4-1);
    drawpixel16(offset-640*4-0);
    drawpixel16(offset-640*4+1);

    drawpixel16(offset-640*3-3);
    drawpixel16(offset-640*3-2);
    drawpixel16(offset-640*3+2);
    drawpixel16(offset-640*3+3);

    drawpixel16(offset-640*2-3);
    drawpixel16(offset-640*2-2);
    drawpixel16(offset-640*2+2);
    drawpixel16(offset-640*2+3);

    drawpixel16(offset-640*1-4);
    drawpixel16(offset-640*1-1);
    drawpixel16(offset-640*1+1);
    drawpixel16(offset-640*1+4);

    drawpixel16(offset-640*0-4);
    drawpixel16(offset-640*0-0);
    drawpixel16(offset-640*0+4);

    drawpixel16(offset+640*1-4);
    drawpixel16(offset+640*1-1);
    drawpixel16(offset+640*1+1);
    drawpixel16(offset+640*1+4);

    drawpixel16(offset+640*2-3);
    drawpixel16(offset+640*2-2);
    drawpixel16(offset+640*2+2);
    drawpixel16(offset+640*2+3);

    drawpixel16(offset+640*3-3);
    drawpixel16(offset+640*3-2);
    drawpixel16(offset+640*3+2);
    drawpixel16(offset+640*3+3);

    drawpixel16(offset+640*4-1);
    drawpixel16(offset+640*4-0);
    drawpixel16(offset+640*4+1);
}

void DrawCircle(int x, int y, int r, int c)
{
    int px, py;
    px = x + r;
    py = y;
    for (int i = 28; i <= 2048; i += 28)
    {
        int nx, ny;
        nx = x + mulscale30(Cos(i), r);
        ny = y + mulscale30(Sin(i), r);
        drawline16(px, py, nx, ny, c);
        px = nx;
        py = ny;
    }
}

void draw2dscreen(int posxe, int posye, short ange, int zoome, short gride)
{
    int i;
    int j;
    int xp1;
    int yp1;
    int xp2;
    int yp2;
    char color;
    char v4;
    int v10;
    short cstat;

    v4 = (gFrameClock & 8) ? 8 : 0;

    gZoom = zoome;
    gGrid = gride;

    if (qsetmode == 200)
        return;

    for (i = 0; i < numwalls; i++)
    {
        if (wall[i].nextwall > i)
            continue;
        if (wall[i].nextwall == -1)
        {
            color = 0x0f;
            v10 = 0;
            cstat = wall[i].cstat;
        }
        else
        {
            color = 0x04;
            v10 = 0;
            cstat = wall[i].cstat | wall[wall[i].nextwall].cstat;
            if (i == linehighlight)
                cstat = wall[i].cstat;
            else if (wall[i].nextwall == linehighlight)
                cstat = wall[wall[i].nextwall].cstat;
            if (cstat & kWallStat6)
                color = 0x05;
            if (cstat & kWallStat0)
                v10 = 1;
        }
        if (cstat & kWallStat14)
            color = 0x09;
        if (cstat & kWallStat15)
            color = 0x0a;
        if (linehighlight >= 0 && (i == linehighlight || wall[i].nextwall == linehighlight))
            color ^= v4;

        xp1 = 320+mulscale(wall[i].x - posxe, zoome, 14);
        yp1 = 200+mulscale(wall[i].y - posye, zoome, 14);
        xp2 = 320+mulscale(wall[wall[i].point2].x - posxe, zoome, 14);
        yp2 = 200+mulscale(wall[wall[i].point2].y - posye, zoome, 14);
        DrawLine(xp1, yp1, xp2, yp2, color, v10);
        if (zoome >= 256 && xp1 > 4 && xp1 < 640-5 && yp1 > 4 && yp1 < ydim16-5)
            DrawVertex(xp1, yp1, 6);
    }
    if (zoome >= 256)
    {
        for (i = 0; i < numwalls; i++)
        {
            if (i == pointhighlight || (highlightcnt > 0 && TestBitString(show2dwall, i)))
            {
                xp1 = 320+mulscale(wall[i].x - posxe, zoome, 14);
                yp1 = 200+mulscale(wall[i].y - posye, zoome, 14);
                if (xp1 > 4 && xp1 < 640-5 && yp1 > 4 && yp1 < ydim16-5)
                    DrawVertex(xp1, yp1, kColor6^v4);
            }
        }
    }
    if (zoome >= 256)
    {
        for (i = 0; i < numsectors; i++)
        {
            j = headspritesect[i];
            while (j != -1)
            {
                xp1 = 320+mulscale(sprite[j].x - posxe, zoome, 14);
                yp1 = 200+mulscale(sprite[j].y - posye, zoome, 14);
                if (sprite[j].statnum == 10)
                {
                    color = 0x0e;
                    if (sprite[j].owner == sectorhighlight)
                        color = 0x0f;
                    if (((j | 0x4000) == pointhighlight) || (highlightcnt > 0 && TestBitString(show2dsprite, j)))
                        color ^= v4;
                    if (xp1 > 4 && xp1 < 635 && yp1 > 4 && yp1 < ydim16-5)
                    {
                        switch (sprite[j].type)
                        {
                        case 3:
                        case 4:
                            DrawMarkerSprite(xp1, yp1, color);
                            break;
                        case 5:
                        case 8:
                            DrawMarkerSprite(xp1, yp1, color);
                            xp2 = mulscale30(zoome/128, Cos(sprite[j].ang));
                            yp2 = mulscale30(zoome/128, Sin(sprite[j].ang));
                            DrawLine(xp1, yp1, xp1+xp2, yp1+yp2, color, v10);
                            break;
                        }
                    }
                    if (sprite[j].type == 3)
                    {
                        int nSector = sprite[j].owner;
                        int nXSector = sector[nSector].extra;
                        dassert(nXSector > 0 && nXSector < kMaxXSectors, 2121+LDIFF1);
                        int k = xsector[nXSector].at2e_0;
                        xp2 = 320+mulscale(sprite[k].x - posxe, zoome, 14);
                        yp2 = 200+mulscale(sprite[k].y - posye, zoome, 14);
                        drawline16(xp1, yp1, xp2, yp2, 9);
                        int ang = getangle(xp1 - xp2, yp1 - yp2);
                        xp1 = mulscale30(zoome/64, Cos(ang+170));
                        yp1 = mulscale30(zoome/64, Sin(ang+170));
                        drawline16(xp2, yp2, xp2+xp1, yp2+yp1, 9);
                        xp1 = mulscale30(zoome/64, Cos(ang-170));
                        yp1 = mulscale30(zoome/64, Sin(ang-170));
                        drawline16(xp2, yp2, xp2+xp1, yp2+yp1, 9);
                    }
                }
                else if (sprite[j].statnum == 12)
                {
                    color = 0x0e;
                    if (sprite[j].owner == sectorhighlight)
                        color = 0x0f;
                    if (((j | 0x4000) == pointhighlight) || (highlightcnt > 0 && TestBitString(show2dsprite, j)))
                        color ^= v4;
                    if (xp1 > 4 && xp1 < 635 && yp1 > 4 && yp1 < ydim16-5)
                    {
                        DrawMarkerSprite(xp1, yp1, color);
                    }
                    int nXSprite = sprite[j].extra;
                    if (nXSprite > 0)
                    {
                        dassert(nXSprite > 0 && nXSprite < kMaxXSprites, 2152+LDIFF1);
                        XSPRITE* pXSprite = &xsprite[nXSprite];
#if APPVER_BLOODREV >= AV_BR_BL111A
                        if (char_CBA0C)
                        {
#endif
                            int r1 = mulscale(pXSprite->at10_0, zoome, 10);
                            int r2 = mulscale(pXSprite->at12_0, zoome, 10);
                            DrawCircle(xp1, yp1, r1, 0x0e);
                            DrawCircle(xp1, yp1, r2, 0x06);
#if APPVER_BLOODREV >= AV_BR_BL111A
                        }
#endif
                    }
                }
                else
                {
                    color = 0x03;
                    v10 = 0;
                    if (sprite[j].cstat & kSpriteStat8)
                        color = 0x05;
                    if (sprite[j].cstat & kSpriteStat15)
                        color = 0x08;
                    if (sprite[j].cstat & kSpriteStat13)
                        color = 0x09;
                    if (sprite[j].cstat & kSpriteStat14)
                        color = 0x0a;
                    if (sprite[j].cstat & kSpriteStat0)
                        v10 = 1;
                    if (((j | 0x4000) == pointhighlight) || (highlightcnt > 0 && TestBitString(show2dsprite, j)))
                        color ^= v4;
                    if (xp1 > 4 && xp1 < 635 && yp1 > 4 && yp1 < ydim16-5)
                    {
                        switch (sprite[j].cstat&kSpriteMask)
                        {
                        case 0x00:
                        case 0x30:
                        {
                            DrawFaceSprite(xp1, yp1, color);
                            xp2 = mulscale30(zoome/128, Cos(sprite[j].ang));
                            yp2 = mulscale30(zoome/128, Sin(sprite[j].ang));
                            DrawLine(xp1, yp1, xp1+xp2, yp1+yp2, color, v10);
                            break;
                        }
                        case 0x10:
                        {
                            DrawFaceSprite(xp1, yp1, color);
                            xp2 = mulscale30(zoome/128, Cos(sprite[j].ang+512));
                            yp2 = mulscale30(zoome/128, Sin(sprite[j].ang+512));
                            DrawLine(xp1-xp2, yp1-yp2, xp1+xp2, yp1+yp2, color, v10);
                            xp2 = mulscale30(zoome/256, Cos(sprite[j].ang));
                            yp2 = mulscale30(zoome/256, Sin(sprite[j].ang));
                            if (sprite[j].cstat & kSpriteStat6)
                                DrawLine(xp1, yp1, xp1+xp2, yp1+yp2, color, v10);
                            else
                                DrawLine(xp1-xp2, yp1-yp2, xp1+xp2, yp1+yp2, color, v10);
                            break;
                        }
                        case 0x20:
                        {
                            DrawFloorSprite(xp1, yp1, color);
                            xp2 = mulscale30(zoome/256, Cos(sprite[j].ang));
                            yp2 = mulscale30(zoome/256, Sin(sprite[j].ang));
                            DrawLine(xp1, yp1, xp1+xp2, yp1+yp2, color, v10);
                            break;
                        }
                        }
                    }
                }
                j = nextspritesect[j];
            }
        }
#if APPVER_BLOODREV >= AV_BR_BL111A
        gXTracker.Draw(posxe, posye, zoome);
#endif
    }
    xp1 = 320+mulscale30(zoome/128, Cos(ange));
    yp1 = 200+mulscale30(zoome/128, Sin(ange));
    drawline16(xp1, yp1, 640-xp1, 400-yp1, 0x0f);
    drawline16(xp1, yp1, 320-200+yp1, 200+320-xp1, 0x0f);
    drawline16(xp1, yp1, 320+200-yp1, 200-320+xp1, 0x0f);
}

void CheckKeys2D(void)
{
    char key;
    char shift;
    char ctrl;
    char alt;
    int mx, my;
    int numsprites = 0;

    static int int_13ADAC, int_13ADB0, int_13ADB4;
    static int int_CBA00 = -1, int_CBA04 = -1, int_CBA08 = -1;

    numsprites = 0;
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
            numsprites++;
    }

    if (numwalls != int_13ADAC || numsectors != int_13ADB0 || numsprites != int_13ADB4)
    {
        func_1058C();
        int_13ADAC = numwalls;
        int_13ADB0 = numsectors;
        int_13ADB4 = numsprites;
    }
    shift = keystatus[bsc_LShift] | keystatus[bsc_RShift];
    ctrl = keystatus[bsc_LCtrl] | keystatus[bsc_RCtrl];
    alt = keystatus[bsc_LAlt] | keystatus[bsc_RAlt];
    getpoint(searchx, searchy, &mx, &my);
    updatesector(mx, my, &sectorhighlight);
    if (pointhighlight != int_CBA00 || linehighlight != int_CBA04 || sectorhighlight != int_CBA08)
    {
        int_CBA00 = pointhighlight;
        int_CBA04 = linehighlight;
        int_CBA08 = sectorhighlight;
        if ((pointhighlight & 0xc000) == 0x4000)
            ShowSpriteData(pointhighlight & 0x3fff);
        else if (linehighlight >= 0)
            ShowWallData(linehighlight);
        else if (sectorhighlight >= 0)
            ShowSectorData(sectorhighlight);
        else
            clearmidstatbar16();
    }
    key = keyGet();
    switch (key)
    {
    case bsc_NumLock:
        sprintf(gTempBuf, "size xsprite=%d xsector=%d xwall=%d", sizeof(XSPRITE), sizeof(XSECTOR), sizeof(XWALL));
        printmessage16(gTempBuf);
        ModifyBeep();
        break;
#if APPVER_BLOODREV >= AV_BR_BL111A
    case bsc_Pad_1:
    case bsc_End:
        char_CBA0C = !char_CBA0C;
        break;
#endif
    case bsc_Home:
        if (ctrl)
        {
            short nSprite = getnumber16("Locate sprite #: ", 0, kMaxSprites);
            clearmidstatbar16();
            if (nSprite >= 0 && nSprite < kMaxSprites && sprite[nSprite].statnum < kMaxStatus)
            {
                posx = sprite[nSprite].x;
                posy = sprite[nSprite].y;
                posz = sprite[nSprite].z;
                ang = sprite[nSprite].ang;
                cursectnum = sprite[nSprite].sectnum;
            }
            else
                printmessage16("Sir Not Appearing In This Film");
        }
        break;
    case bsc_D:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            int nSprite = pointhighlight & 0x3fff;
            SPRITE* pSprite = &sprite[nSprite];
            if (alt)
            {
                short nClipDist = getnumber16("Sprite clipdist #: ", pSprite->clipdist, 256);
                clearmidstatbar16();
                if (nClipDist >= 0 && nClipDist < 256)
                {
                    pSprite->clipdist = nClipDist;
                    sprintf(gTempBuf, "sprite[%d].clipdist is %d", nSprite, pSprite->clipdist);
                    printmessage16(gTempBuf);
                    ModifyBeep();
                }
                else
                {
                    printmessage16("Clipdist must be between 0 and 255");
                    Beep();
                }
            }
            else
            {
                short nDetail = getnumber16("Sprite detail Level #: ", pSprite->detail, 5);
                clearmidstatbar16();
                if (nDetail >= 0 && nDetail <= 4)
                {
                    pSprite->detail = nDetail;
                    sprintf(gTempBuf, "sprite[%d].detail is %d", nSprite, pSprite->detail);
                    printmessage16(gTempBuf);
                    ModifyBeep();
                }
                else
                {
                    sprintf(gTempBuf, "Detail must be between %d and %d", 0, 4);
                    printmessage16(gTempBuf);
                    Beep();
                }
            }
        }
        break;
    case bsc_I:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            SPRITE* pSprite = &sprite[pointhighlight & 0x3fff];
            pSprite->cstat ^= 0x8000;
            ModifyBeep();
        }
        else
            Beep();
        break;
    case bsc_K:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            short nMotion = sprite[pointhighlight & 0x3fff].cstat & 0x6000;
            switch (nMotion)
            {
            case 0:
                nMotion = 0x2000;
                break;
            case 0x2000:
                nMotion = 0x4000;
                break;
            case 0x4000:
                nMotion = 0x0000;
                break;
            default:
                nMotion = 0;
                break;
            }
            sprite[pointhighlight & 0x3fff].cstat &= ~0x6000;
            sprite[pointhighlight & 0x3fff].cstat |= nMotion;
            ModifyBeep();
        }
        else if (linehighlight >= 0)
        {
            short nMotion = wall[linehighlight].cstat & 0xc000;
            switch (nMotion)
            {
            case 0:
                nMotion = 0x4000;
                break;
            case 0x4000:
                nMotion = 0x8000;
                break;
            case 0x8000:
                nMotion = 0x0000;
                break;
            default:
                nMotion = 0;
                break;
            }
            wall[linehighlight].cstat &= ~0xc000;
            wall[linehighlight].cstat |= nMotion;
            ModifyBeep();
        }
        else
            Beep();
        break;
    case bsc_M:
    {
        if (linehighlight >= 0)
        {
            int nWall = linehighlight;
            int nWall2 = wall[nWall].nextwall;
            if (nWall2 < 0)
            {
                Beep();
                break;
            }
            wall[nWall].cstat |= 0x10;
            wall[nWall].cstat &= ~0x08;
            wall[nWall2].cstat |= 0x10;
            wall[nWall2].cstat |= 0x08;
            if (wall[nWall].overpicnum < 0)
                wall[nWall].overpicnum = 0;
            wall[nWall2].overpicnum = wall[nWall].overpicnum;
            wall[nWall].cstat &= ~0x20;
            wall[nWall2].cstat &= ~0x20;
            sprintf(gTempBuf, "wall[%i] %s masked", searchwall, (wall[searchwall].cstat & kWallStat4) ? "is" : "not");
            printmessage16(gTempBuf);
            ModifyBeep();
        }
        else
            Beep();
        break;
    }
    case bsc_X:
        if (alt)
        {
            if (linehighlight >= 0)
            {
                int nSector = sectorofwall(linehighlight);
                dassert(nSector >= 0 && nSector < kMaxSectors, 2500+LDIFF2);
                SECTOR* pSector = &sector[nSector];
                int n = linehighlight - pSector->wallptr;
                pSector->align = n;
                char buf[80];
                sprintf(buf, "Sector will align to wall %d (%d)", linehighlight, n);
                printmessage16(buf);
                ModifyBeep();
            }
            else
                Beep();
        }
        break;
    case bsc_OpenBracket:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            int nSprite = pointhighlight & 0x3fff;
            int nStep = shift ? 16 : 256;
            sprite[nSprite].ang = DecStep(sprite[nSprite].ang, nStep);
            sprintf(gTempBuf, "sprite[%i].ang: %i", nSprite, sprite[nSprite].ang);
            printmessage16(gTempBuf);
            ModifyBeep();
        }
        break;
    case bsc_CloseBracket:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            int nSprite = pointhighlight & 0x3fff;
            int nStep = shift ? 16 : 256;
            sprite[nSprite].ang = IncStep(sprite[nSprite].ang, nStep);
            sprintf(gTempBuf, "sprite[%i].ang: %i", nSprite, sprite[nSprite].ang);
            printmessage16(gTempBuf);
            ModifyBeep();
        }
        break;
    }
    if (key != 0)
        keyFlushStream();
}
