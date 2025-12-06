#ifndef _BSTUB_H_
#define _BSTUB_H_

#include "typedefs.h"
#include "globals.h"
#include "build.h"
#include "db.h"

enum {
    kColor0 = 0,
    kColor1,
    kColor2,
    kColor3,
    kColor4,
    kColor5,
    kColor6,
    kColor7,
    kColor8,
    kColor9,
    kColor10,
    kColor11,
    kColor12,
    kColor13,
    kColor14,
    kColor15,
};

#if APPVER_BLOODREV >= AV_BR_BL111A
class CXTracker {
public:
    BOOL m_type;
    int m_nSector;
    int m_nWall;
    int m_nSprite;
    SECTOR* m_pSector;
    WALL* m_pWall;
    SPRITE* m_pSprite;
    XSECTOR* m_pXSector;
    XWALL* m_pXWall;
    XSPRITE* m_pXSprite;
    CXTracker();
    void TrackClear();
    void TrackSector(int a1, char a2);
    void TrackWall(int a1, char a2);
    void TrackSprite(int a1, char a2);
    void Draw(int a1, int a2, int a3);
};

extern CXTracker gXTracker;
#endif


extern char* int_D9A88[2];
extern char* int_D9A90[1024];
extern char* int_DAA90[1024];
extern char* int_DBA90[1024];
extern char* WaveForm2[8];
extern char* WaveForm[20];
extern char* int_DCB00[64];
extern char* int_DCC00[192];
extern char* int_DCF00[4];

extern int gHighlightThreshold;
extern int gZoom;
extern short gGrid, gGridLock;

extern int gLightBombMaxBright;
extern int gLightBombRampDist;
extern int gLightBombReflections;
extern int gLightBombAttenuation;
extern int gLightBombIntensity;
extern BOOL gOldKeyMapping;
extern BOOL gBeep;
extern int int_D9A80;
extern BOOL char_CA89C;

void ModifyBeep(void);
void Beep(void);
void func_1058C(void);
int func_10DBC(int nSector);
int func_10E08(int nWall);
int func_10E50(int nSprite);
void func_10EA0(void);

void EditSectorData(int nSector);
void ShowSectorData(int nSector);
void EditWallData(int nWall);
void ShowWallData(int nWall);
void EditSpriteData(int nSprite);
void ShowSpriteData(int nSprite);

void CheckKeys2D(void);
void Check3DKeys(void);


#endif
