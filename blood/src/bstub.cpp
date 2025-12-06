#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include "typedefs.h"
#include "globals.h"
#include "build.h"
#include "bstub.h"
#include "config.h"
#include "db.h"
#include "debug4g.h"
#include "error.h"
#include "file_lib.h"
#include "gameutil.h"
#include "gui.h"
#include "helix.h"
#include "inifile.h"
#include "key.h"
#include "misc.h"
#include "mouse.h"
#include "resource.h"
#include "screen.h"
#include "sectorfx.h"
#include "sound.h"
#include "textio.h"
#include "tile.h"
#include "satimer.h"
#include "trig.h"

#if APPVER_BLOODREV >= AV_BR_BL120
#define LDIFF1 0
#define LDIFF2 0
#elif APPVER_BLOODREV >= AV_BR_BL111A
#define LDIFF1 -2
#define LDIFF2 -2
#else
#define LDIFF1 -2
#define LDIFF2 -14
#endif

char byte_D9760[192][4];

int gCorrectedSprites;
int gHighlightThreshold;
int gStairHeight;
int gLightBombIntensity;
int gLightBombAttenuation;
int gLightBombReflections;
int gLightBombMaxBright;
int gLightBombRampDist;
int int_D9A80;
BOOL gBeep;
BOOL gOldKeyMapping;

char* int_D9A88[2];
char* int_D9A90[1024];
char* int_DAA90[1024];
char* int_DBA90[1024];
char* WaveForm2[8];
char* WaveForm[20];
char* int_DCB00[64];
char* int_DCC00[192];
char* int_DCF00[4];

int int_DCF10;
int gAutoSaveInterval;
int int_DCF18;
char* szSoundRes;
int int_DCF20;
ErrorHandler prevErrorHandler;
IniFile gMapEditIni("MAPEDIT.INI");

struct TextStruct {
    short id;
    char* text;
};

BOOL char_CA89C = 1;
short gGrid = 4;
short gGridLock = 1;
int gZoom = 768;
char char_CA8A8[256] = "";
char buildkeys[19] =
{
    0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
    0x1e,0x2c,0xd1,0xc9,0x47,0x49,
    0x9c,0x1c,0xd,0xc,0xf,
};

TextStruct struct_CA9BC[] = {
    { 0, "Off" },
    { 1, "On" },
};

TextStruct struct_CA9C8[] = {
    { 0, "Decoration" },
    { 1, "Player Start" },
    { 2, "Bloodbath Start" },
    { 3, "Off marker" },
    { 4, "On marker" },
    { 5, "Axis marker" },

    { 6, "Lower link" },
    { 7, "Upper link" },
    { 8, "Teleport target" },
    { 10, "Lower water" },
    { 9, "Upper water" },
    { 12, "Lower stack" },
    { 11, "Upper stack" },
    { 14, "Lower goo" },
    { 13, "Upper goo" },
    { 15, "Path marker" },
    { 16, "Alignable Region" },
    { 17, "Base Region" },
    { 18, "Dude Spawn" },
    { 19, "Earthquake" },

    { 20, "Toggle switch" },
    { 21, "1-Way switch" },
    { 22, "Combination switch" },
    { 23, "Padlock (1-shot)" },

    { 30, "Torch" },
    { 32, "Candle" },

    { 40, gWeaponText[0] },
    { 47, gWeaponText[7] },
    { 43, gWeaponText[3] },
    { 41, gWeaponText[1] },
    { 42, gWeaponText[2] },
    { 46, gWeaponText[6] },
    { 49, gWeaponText[9] },
    { 48, gWeaponText[8] },
    { 45, gWeaponText[5] },
    { 50, gWeaponText[10] },
    { 44, gWeaponText[4] },

    { 60, gAmmoText[0] },
    { 62, gAmmoText[2] },
    { 63, gAmmoText[3] },
    { 64, gAmmoText[4] },
    { 65, gAmmoText[5] },
    { 67, gAmmoText[7] },
    { 68, gAmmoText[8] },
    { 69, gAmmoText[9] },
    { 70, gAmmoText[10] },
    { 72, gAmmoText[12] },
    { 73, gAmmoText[13] },
    { 76, gAmmoText[16] },
    { 79, gAmmoText[19] },
    { 66, gAmmoText[6] },
    { 80, "Random Ammo" },

    { 100, gItemText[0] },
    { 101, gItemText[1] },
    { 102, gItemText[2] },
    { 103, gItemText[3] },
    { 104, gItemText[4] },
    { 105, gItemText[5] },
    { 106, gItemText[6] },
    { 107, gItemText[7] },
    { 108, gItemText[8] },
    { 109, gItemText[9] },
    { 110, gItemText[10] },
    { 111, gItemText[11] },
    { 112, gItemText[12] },
    { 113, gItemText[13] },
    { 114, gItemText[14] },
    { 115, gItemText[15] },
    { 116, gItemText[16] },
    { 117, gItemText[17] },
    { 118, gItemText[18] },
    { 119, gItemText[19] },
    { 120, gItemText[20] },
    { 121, gItemText[21] },
    { 122, gItemText[22] },
    { 123, gItemText[23] },
    { 124, gItemText[24] },
    { 125, gItemText[25] },
    { 126, gItemText[26] },
    { 127, gItemText[27] },
    { 128, gItemText[28] },
    { 129, gItemText[29] },
    { 130, gItemText[30] },
    { 131, gItemText[31] },
    { 132, gItemText[32] },
    { 133, gItemText[33] },
    { 134, gItemText[34] },
    { 135, gItemText[35] },
    { 136, gItemText[36] },
    { 137, gItemText[37] },
    { 138, gItemText[38] },
    { 139, gItemText[39] },
    { 140, gItemText[40] },
    { 141, gItemText[41] },
    { 142, gItemText[42] },
    { 143, gItemText[43] },
    { 144, gItemText[44] },
    { 145, gItemText[45] },
    { 146, gItemText[46] },

#if APPVER_BLOODREV < AV_BR_BL120
    { 200, "Random Creature" },
#endif
    { 201, "Cultist w/Tommy" },
    { 202, "Cultist w/Shotgun" },
#ifdef PLASMAPAK
    { 247, "Cultist w/Tesla" },
    { 248, "Cultist w/Dynamite" },
    { 249, "Beast Cultist" },
    { 250, "Tiny Caleb" },
    { 251, "Beast" },
#endif
    { 203, "Axe Zombie" },
    { 204, "Fat Zombie" },
    { 205, "Earth Zombie" },
    { 244, "Sleep Zombie" },
    { 245, "Innocent" },
    { 206, "Flesh Gargoyle" },
    { 207, "Stone Gargoyle" },
    { 208, "Flesh Statue" },
    { 209, "Stone Statue" },
    { 210, "Phantasm" },
    { 211, "Hound" },
    { 212, "Hand" },
    { 213, "Brown Spider" },
    { 214, "Red Spider" },
    { 216, "Mother Spider" },
#ifdef PLASMAPAK
    { 215, "Black Spider" },
#endif
    { 217, "GillBeast" },
    { 218, "Eel" },
    { 219, "Bat" },
    { 220, "Rat" },
#if defined(PLASMAPAK) || (APPVER_BLOODREV < AV_BR_BL120)
    { 221, "Green Pod" },
    { 222, "Green Tentacle" },
    { 223, "Fire Pod" },
    { 224, "Fire Tentacle" },
#endif
#if APPVER_BLOODREV < AV_BR_BL120
    { 225, "Mother Pod" },
    { 226, "Mother Tentacle" },
#endif
    { 227, "Cerberus" },
    { 229, "Tchernobog" },
    { 230, "TCultist prone" },
    { 246, "SCultist prone" },

    { 400, "TNT Barrel" },
    { 401, "Armed Prox Bomb" },
    { 402, "Armed Remote" },
    { 403, "Blue Vase" },
    { 404, "Brown Vase" },
    { 405, "Crate Face" },
    { 406, "Glass Window" },
    { 407, "Fluorescent Light" },
    { 408, "Wall Crack" },
    { 409, "Wood Beam" },
    { 410, "Spider's Web" },
    { 411, "MetalGrate1" },
    { 412, "FlammableTree" },
    { 413, "Machine Gun" },
    { 414, "Falling Rock" },
    { 415, "Kickable Pail" },
    { 416, "Gib Object" },
    { 417, "Explode Object" },
    { 427, "Zombie Head" },

    { 450, "Spike Trap" },
    { 451, "Rock Trap" },
    { 452, "Flame Trap" },
    { 454, "Saw Blade" },
    { 455, "Electric Zap" },
    { 456, "Switched Zap" },
    { 457, "Pendulum" },
    { 458, "Guillotine" },
    { 459, "Hidden Exploder" },

    { 700, "Trigger Gen" },
    { 701, "WaterDrip Gen" },
    { 702, "BloodDrip Gen" },
    { 703, "Fireball Gen" },
    { 704, "EctoSkull Gen" },
    { 705, "Dart Gen" },
    { 706, "Bubble Gen" },
    { 707, "Multi-Bubble Gen" },
    { 708, "SFX Gen" },
    { 709, "Sector SFX" },
    { 710, "Ambient SFX" },
    { 711, "Player SFX" },
};

TextStruct struct_CADDC[] = {
    { 0, "Normal" },
    { 20, "Toggle switch" },
    { 21, "1-Way switch" },
    { 500, "Wall Link" },
    { 501, "Wall Stack (unsupp.)" },
    { 511, "Gib Wall" },
};

TextStruct struct_CAE00[] = {
    { 0, "Normal" },
    { 600, "Z Motion" },
    { 602, "Z Motion SPRITE" },
    { 603, "Warp" },
    { 604, "Teleporter" },
    { 614, "Slide Marked" },
    { 615, "Rotate Marked" },
    { 616, "Slide" },
    { 617, "Rotate" },
    { 613, "Step Rotate" },
    { 612, "Path Sector" },
    { 618, "Damage Sector" },
    { 619, "Counter Sector" },
};

TextStruct struct_CAE50[] = {
    { 1, "Linear" },
    { 0, "Sine" },
    { 2, "SlowOff"},
    { 3, "SlowOn"},
};

TextStruct struct_CAE68[] = {
    { 0, "None" },
    { 1, "Square" },
    { 2, "Saw" },
    { 3, "Ramp up" },
    { 4, "Ramp down" },
    { 5, "Sine" },
    { 6, "Flicker1" },
    { 7, "Flicker2" },
    { 8, "Flicker3" },
    { 9, "Flicker4" },
    { 10, "Strobe" },
    { 11, "Search" },
};

TextStruct struct_CAEB0[] = {
    { 0, "OFF" },
    { 1, "ON" },
    { 2, "State" },
    { 3, "Toggle" },
    { 4, "!State" },
    { 5, "Link" },
    { 6, "Lock" },
    { 7, "Unlock" },
    { 8, "Toggle Lock" },
    { 9, "Stop OFF" },
    { 10, "Stop ON" },
    { 11, "Stop Next" },
};

TextStruct struct_CAEF8[] = {
    { 0, "Optional" },
    { 1, "Never" },
    { 2, "Always" },
    { 3, "Permanent" },
};

struct StructCAF10 {
    short at0;
    short at2;
    short at4;
    short at6;
    signed char at8;
    short at9;
};

StructCAF10 struct_CAF10[] = {
    { 1, -1, -1, -1, 0, 0 },
    { 2, -1, -1, -1, 0, 5 },
    { 6, 2331, 64, 64, 0, 0 },
    { 7, 2332, 64, 64, 0, 0 },
    { 10, 2331, 64, 64, 0, 0 },
    { 9, 2332, 64, 64, 0, 0 },
    { 12, 2331, 64, 64, 0, 0 },
    { 11, 2332, 64, 64, 0, 0 },
    { 14, 2331, 64, 64, 0, 0 },
    { 13, 2332, 64, 64, 0, 0 },
    { 15, 2319, 64, 64, 0, 0 },
    { 18, 2077, 64, 64, 0, 0 },
    { 19, 2072, 64, 64, 0, 0 },
    { 20, -1, -1, -1, 1, -1 },
    { 21, -1, -1, -1, 1, -1 },
    { 22, -1, -1, -1, 1, -1 },
    { 23, 948, -1, -1, 1, -1 },
    { 30, 550, -1, -1, 1, -1 },
    { 30, 572, -1, -1, 1, -1 },
    { 30, 560, -1, -1, 1, -1 },
    { 30, 564, -1, -1, 1, -1 },
    { 30, 570, -1, -1, 1, -1 },
    { 30, 554, -1, -1, 1, -1 },
    { 32, 938, -1, -1, 1, -1 },

    { 40, 832, 48, 48, 0, 0 },
    { 43, 524, 48, 48, 0, 0 },
    { 41, 559, 48, 48, 0, 0 },
    { 42, 558, 48, 48, 0, 0 },
    { 46, 526, 48, 48, 0, 0 },
    { 45, 539, 48, 48, 0, 0 },
    { 50, 800, 48, 48, 0, 0 },

    { 60, 618, 40, 40, 1, 0 },
    { 62, 589, 48, 48, 1, 0 },
    { 63, 809, 48, 48, 1, 0 },
    { 64, 811, 40, 40, 0, 0 },
    { 65, 810, 40, 40, 0, 0 },
    { 66, 820, 24, 24, 0, 0 },
    { 67, 619, 48, 48, 0, 0 },
    { 68, 812, 48, 48, 0, 0 },
    { 69, 813, 48, 48, 0, 0 },
    { 70, 525, 48, 48, 0, 0 },
    { 72, 817, 48, 48, 0, 0 },
    { 73, 548, 24, 24, 0, 0 },
    { 76, 816, 48, 48, 0, 0 },
    { 79, 801, 48, 48, 0, 0 },
    { 80, 832, 40, 40, 0, 0 },
    { 100, 2552, 32, 32, 0, 0 },
    { 101, 2553, 32, 32, 0, 0 },
    { 102, 2554, 32, 32, 0, 0 },
    { 103, 2555, 32, 32, 0, 0 },
    { 104, 2556, 32, 32, 0, 0 },
    { 105, 2557, 32, 32, 0, 0 },
    { 106, -1, -1, -1, 0, 0 },
    { 107, 519, 48, 48, 0, 0 },
    { 108, 822, 40, 40, 0, 0 },
    { 109, 2169, 40, 40, 0, 0 },
    { 110, 2433, 40, 40, 0, 0 },
    { 113, 896, 40, 40, 0, 0 },
    { 114, 825, 40, 40, 0, 0 },
    { 115, 827, 40, 40, 0, 0 },
    { 117, 829, 40, 40, 0, 0 },
    { 118, 830, 80, 64, 0, 0 },
    { 121, 760, 40, 40, 0, 0 },
    { 124, 2428, 40, 40, 0, 0 },
    { 125, 839, 40, 40, 0, 0 },
    { 127, 840, 48, 48, 0, 0 },
    { 128, 841, 48, 48, 0, 0 },
    { 129, 842, 48, 48, 0, 0 },
    { 130, 843, 48, 48, 0, 0 },
    { 136, 518, 40, 40, 0, 0 },
    { 137, 522, 40, 40, 0, 0 },
    { 138, 523, 40, 40, 0, 0 },
    { 140, 2628, 64, 64, 0, 0 },
    { 141, 2586, 64, 64, 0, 0 },
    { 142, 2578, 64, 64, 0, 0 },
    { 143, 2602, 64, 64, 0, 0 },
    { 144, 2594, 64, 64, 0, 0 },
    { 144, 2594, 64, 64, 0, 0 },
    { 145, -1, 64, 64, 1, 0 },
    { 146, -1, 64, 64, 1, 0 },

    { 200, 832, 64, 64, 1, 0 },
    { 201, 2820, 40, 40, 1, 3 },
    { 202, 2825, 40, 40, 1, 0 },
#ifdef PLASMAPAK
    { 247, 2820, 40, 40, 1, 11 },
    { 248, 2820, 40, 40, 1, 13 },
    { 249, 2825, 48, 48, 1, 12 },
    { 250, 3870, 16, 16, 1, 12 },
    { 251, 2960, 48, 48, 1, 0 },
#endif
    { 203, 1170, 40, 40, 1, 0 },
    { 204, 1370, 48, 48, 1, 0 },
    { 205, 3054, 40, 40, 1, 0 },
    { 244, 1209, 40, 40, 1, 0 },
    { 245, 3798, 40, 40, 1, 0 },
    { 206, 1470, 40, 40, 1, 0 },
    { 207, 1470, 40, 40, 1, 5 },
    { 208, 1530, 40, 40, 1, 0 },
    { 209, 1530, 40, 40, 1, 5 },
    { 210, 3060, 40, 40, 1, 0 },
    { 211, 1270, 40, 40, 1, 0 },
    { 212, 1980, 32, 32, 1, 0 },
    { 213, 1920, 16, 16, 1, 7 },
    { 214, 1925, 24, 24, 1, 4 },
    { 216, 1930, 40, 40, 1, 0 },
#ifdef PLASMAPAK
    { 215, 1935, 32, 32, 1, 4 },
#endif
    { 217, 1570, 48, 48, 1, 0 },
    { 218, 1870, 32, 32, 1, 0 },
    { 219, 1948, 32, 32, 1, 0 },
    { 220, 1745, 24, 24, 1, 0 },
    { 221, 1792, 32, 32, 1, 0 },
    { 222, 1797, 32, 32, 1, 0 },
    { 223, 1792, 48, 48, 1, 2 },
    { 224, 1797, 48, 48, 1, 2 },
    { 225, 1792, 64, 64, 1, 6 },
    { 226, 1797, 64, 64, 1, 6 },
    { 227, 2680, 64, 64, 1, 0 },
    { 229, 3140, 64, 64, 1, 0 },
    { 230, 3385, 40, 40, 1, 3 },
    { 231, 2860, 40, 40, 1, 0 },
    { 232, 2860, 40, 40, 1, 0 },
    { 233, 2860, 40, 40, 1, 0 },
    { 234, 2860, 40, 40, 1, 0 },
    { 235, 2860, 40, 40, 1, 0 },
    { 236, 2860, 40, 40, 1, 0 },
    { 237, 2860, 40, 40, 1, 0 },
    { 238, 2860, 40, 40, 1, 0 },
    { 243, 2860, 40, 40, 1, 0 },
    { 246, 3385, 40, 40, 1, 0 },
    { 400, 907, 64, 64, 1, 0 },
    { 401, 3444, 40, 40, 1, 0 },
    { 402, 3457, 40, 40, 1, 0 },
    { 403, 739, -1, -1, 1, 0 },
    { 404, 642, -1, -1, 1, 0 },
    { 405, 462, -1, -1, 1, 0 },
    { 406, 266, -1, -1, 1, 0 },
    { 407, 796, -1, -1, 1, 0 },
    { 408, -1, -1, -1, 0, -1 },
    { 409, 1142, -1, -1, 1, 0 },
    { 410, 1069, -1, -1, 1, 0 },
    { 411, 483, -1, -1, 1, -1 },
    { 412, -1, -1, -1, 1, -1 },
    { 413, -1, 64, 64, 0, 0 },
    { 414, -1, -1, -1, 1, 0 },
    { 415, -1, 48, 48, 1, 0 },
    { 416, -1, -1, -1, -1, -1 },
    { 417, -1, -1, -1, -1, -1 },
    { 427, -1, 40, 40, 1, -1 },
    { 450, 968, 64, 64, 0, 0 },
    { 451, -1, 64, 64, 0, 0 },
    { 452, 2183, -1, -1, 0, 0 },
    { 454, 655, -1, -1, 0, 0 },
    { 455, 1156, -1, -1, 0, 0 },
    { 456, 1156, -1, -1, 0, 0 },
    { 457, 1080, -1, -1, 0, 0 },
    { 458, 835, -1, -1, 0, 0 },
    { 459, 908, 4, -1, 0, 0 },
    { 708, 2519, 64, 64, 0, 0 },
    { 709, 2520, 64, 64, 0, 0 },
    { 710, 2521, 64, 64, 0, 0 },
    { 711, 2519, 64, 64, 0, 5 },
};

void Sleep(int ticks)
{
    int end = gGameClock + ticks;
    while (end > gGameClock) {}
}

void ModifyBeep()
{
    if (gBeep)
    {
        sound(6000);
        Sleep(2);
        nosound();
        Sleep(2);
    }
    asksave = 1;
}

void Beep()
{
    sound(1000);
    Sleep(4);
    sound(800);
    Sleep(4);
    nosound();
}

void ExitMsg()
{
    setvmode(gOldDisplayMode);
    if (getErrorFlag())
    {
        printf("\n%s\n", getErrorMsg());
    }
    printf("\nBUILD engine by Ken Silverman\n");
    printf("MAPEDIT version by Nick Newhard and Peter Freese\n");
    printf("Copyright (c)1994-1997 Monolith Productions Inc.\n");
    printf("Your use of this program is restricted according to your license agreement.\n\n");
    printf("Check out www.blood.com for the latest version and information.\n\n");
    if (gCorrectedSprites)
        printf("WARNING: %d sprites were corrected\n", gCorrectedSprites);
}

void FillList(char** a1, TextStruct* a2, int a3)
{
    for (int i = 0; i < a3; i++)
    {
        a1[a2->id] = a2->text;
        a2++;
    }
}

void FillStringLists(void)
{
    FillList(int_D9A88, struct_CA9BC, 2);
    FillList(int_D9A90, struct_CA9C8, sizeof(struct_CA9C8)/sizeof(struct_CA9C8[0]));
    FillList(int_DAA90, struct_CADDC, 6);
    FillList(int_DBA90, struct_CAE00, 13);
    FillList(WaveForm2, struct_CAE50, 4);
    FillList(WaveForm, struct_CAE68, 12);
    FillList(int_DCB00, struct_CAEB0, 12);
    for (int i = 0; i < 192; i++)
    {
        sprintf(byte_D9760[i], "%d", i);
        int_DCC00[i] = byte_D9760[i];
    }
    FillList(int_DCF00, struct_CAEF8, 4);
}

void func_1058C(void)
{
    int nSprite;
    int i;
    int nNextSprite;
    dbXSectorClean();
    dbXWallClean();
    dbXSpriteClean();
    InitSectorFX();
    for (i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            switch (sector[i].type)
            {
            case kSectorType604:
            {
                if (xsector[nXSector].at2c_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2c_0;
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerWarpDest)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].at2c_0 = -1;
                }
                break;
            }
            case kSectorType614:
            case kSectorType616:
            {
                if (xsector[nXSector].at2c_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2c_0;
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarker3)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].at2c_0 = -1;
                }
                if (xsector[nXSector].at2e_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2e_0;
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarker4)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].at2e_0 = -1;
                }
                break;
            }
            case kSectorType613:
            case kSectorType615:
            case kSectorType617:
            {
                if (xsector[nXSector].at2c_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2c_0;
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarker5)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].at2c_0 = -1;
                }
                break;
            }
            }
        }
    }
    for (i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            switch (sector[i].type)
            {
            case kSectorType604:
            {
                if (xsector[nXSector].at2c_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2c_0;
                    if (sprite[nSprite].owner != (short)i)
                    {
                        int vd = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
                        sprite[vd] = sprite[nSprite];
                        sprite[vd].owner = i;
                        xsector[nXSector].at2c_0 = vd;
                    }
                }
                if (xsector[nXSector].at2c_0 < 0)
                {
                    int vd = InsertSprite(i, kStatMarker);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = kMarkerWarpDest;
                    xsector[nXSector].at2c_0 = vd;
                }
                break;
            }
            case kSectorType614:
            case kSectorType616:
            {
                if (xsector[nXSector].at2c_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2c_0;
                    if (sprite[nSprite].owner != (short)i)
                    {
                        int vd = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
                        sprite[vd] = sprite[nSprite];
                        sprite[vd].owner = i;
                        xsector[nXSector].at2c_0 = vd;
                    }
                }
                if (xsector[nXSector].at2c_0 < 0)
                {
                    int vd = InsertSprite(i, kStatMarker);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = kMarker3;
                    xsector[nXSector].at2c_0 = vd;
                }
                if (xsector[nXSector].at2e_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2e_0;
                    if (sprite[nSprite].owner != (short)i)
                    {
                        int vd = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
                        sprite[vd] = sprite[nSprite];
                        sprite[vd].owner = i;
                        xsector[nXSector].at2e_0 = (short)vd;
                    }
                }
                if (xsector[nXSector].at2e_0 < 0)
                {
                    int vd = InsertSprite(i, kStatMarker);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = kMarker4;
                    xsector[nXSector].at2e_0 = vd;
                }
                break;
            }
            case kSectorType613:
            case kSectorType615:
            case kSectorType617:
            {
                if (xsector[nXSector].at2c_0 >= 0)
                {
                    int nSprite = xsector[nXSector].at2c_0;
                    if (sprite[nSprite].owner != (short)i)
                    {
                        int vd = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
                        sprite[vd] = sprite[nSprite];
                        sprite[vd].owner = i;
                        xsector[nXSector].at2c_0 = vd;
                    }
                }

                if (xsector[nXSector].at2c_0 < 0)
                {
                    int vd = InsertSprite(i, kStatMarker);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = kMarker5;
                    xsector[nXSector].at2c_0 = vd;
                }
                break;
            }
            default:
                xsector[nXSector].at2c_0 = -1;
                xsector[nXSector].at2e_0 = -1;
                break;
            }
        }
    }
    for (nSprite = headspritestat[kStatMarker]; nSprite != -1; nSprite = nNextSprite)
    {
        nNextSprite = nextspritestat[nSprite];
        sprite[nSprite].extra = -1;
        sprite[nSprite].cstat |= 0x8000;
        sprite[nSprite].cstat &= ~0x101;
        int nSector = sprite[nSprite].owner;
        int nXSector = sector[nSector].extra;
        if (nSector >= 0 && nSector < numsectors)
        {
            if (nXSector > 0 && nXSector < kMaxXSectors)
            {
                switch (sprite[nSprite].type)
                {
                case kMarker3:
                    sprite[nSprite].picnum = 3997;
                    if (xsector[nXSector].at2c_0 == nSprite)
                        continue;
                    break;
                case kMarker4:
                    sprite[nSprite].picnum = 3997;
                    if (xsector[nXSector].at2e_0 == nSprite)
                        continue;
                    break;
                case kMarker5:
                    sprite[nSprite].picnum = 3997;
                    if (xsector[nXSector].at2c_0 == nSprite)
                        continue;
                    break;
                case kMarkerWarpDest:
                    if (xsector[nXSector].at2c_0 == nSprite)
                        continue;
                    break;
                }
            }
        }
        DeleteSprite(nSprite);
    }
}

int func_10DBC(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors, 820+LDIFF1);
    int nXSector = sector[nSector].extra;
    if (nXSector <= 0)
        nXSector = dbInsertXSector(nSector);
    return nXSector;
}

int func_10E08(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls, 840+LDIFF1);
    int nXWall = wall[nWall].extra;
    if (nXWall <= 0)
        nXWall = dbInsertXWall(nWall);
    return nXWall;
}

int func_10E50(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 860+LDIFF1);
    int nXSprite = sprite[nSprite].extra;
    if (nXSprite <= 0)
        nXSprite = dbInsertXSprite(nSprite);
    return nXSprite;
}

void func_10EA0(void)
{
    for (int i = 0; i < kMaxSprites; i++)
    {
        SPRITE* pSprite = &sprite[i];
        if (pSprite->statnum < kMaxStatus)
        {
            if ((pSprite->cstat & kSpriteMask) == kSpriteVoxel)
                pSprite->cstat &= ~kSpriteMask;
            if (pSprite->statnum == 1)
                continue;
            int j, vbp, vdi;
            vbp = vdi = -1;
            if (pSprite->type != 0)
            {
                if (!int_D9A90[sprite[i].type])
                    pSprite->type = 0;
            }
            for (j = 0; j < sizeof(struct_CAF10)/sizeof(struct_CAF10[0]); j++)
            {
                if (struct_CAF10[j].at2 >= 0 && struct_CAF10[j].at2 == pSprite->picnum)
                {
                    vdi = j;
                    break;
                }
            }
            for (j = 0; j < sizeof(struct_CAF10)/sizeof(struct_CAF10[0]); j++)
            {
                if (struct_CAF10[j].at0 == pSprite->type)
                {
                    vbp = j;
                    break;
                }
            }
            j = -1;
            if (vdi >= 0)
                j = vdi;
            if (vbp >= 0)
                j = vbp;
            if (vdi >= 0 && struct_CAF10[vdi].at0 == pSprite->type)
                j = vdi;
            if (j < 0)
                continue;
            int vdi2 = struct_CAF10[j].at0;
            if (vdi2 == 1 || vdi2 == 2)
            {
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 0);
                int nXSprite = func_10E50(i);
                xsprite[nXSprite].at10_0 &= 0x07;
                pSprite->picnum = 2522 + xsprite[nXSprite].at10_0;
            }
            else if (vdi2 == 18)
            {
                pSprite->cstat &= ~0x01;
                pSprite->cstat |= 0x8000;
                ChangeSpriteStat(i, 0);
                func_10E50(i);
            }
            else if (vdi2 == 19)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                ChangeSpriteStat(i, 0);
                func_10E50(i);
            }
            else if (vdi2 == 7 || vdi2 == 6 || vdi2 == 9
                || vdi2 == 10 || vdi2 == 13 || vdi2 == 14
                || vdi2 == 11 || vdi2 == 12)
            {
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 0);
                func_10E50(i);
                pSprite->cstat &= ~0x08;
            }
            else if (vdi2 == 15)
            {
                ChangeSpriteStat(i, 16);
                func_10E50(i);
            }
            else if (vdi2 >= 20 && vdi2 < 24)
            {
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 0);
                func_10E50(i);
            }
            else if (vdi2 >= 40 && vdi2 < 51)
            {
                if ((pSprite->cstat & kSpriteMask) != kSpriteFace)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 3);
                func_10E50(i);
            }
            else if (vdi2 >= 60 && vdi2 < 81)
            {
                if ((pSprite->cstat & kSpriteMask) != kSpriteFace)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 3);
                func_10E50(i);
            }
            else if (vdi2 >= 100 && vdi2 < 149)
            {
                if ((pSprite->cstat & kSpriteMask) != kSpriteFace)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 3);
                func_10E50(i);
            }
            else if (vdi2 >= 200 && vdi2 < 254)
            {
                if ((pSprite->cstat & kSpriteMask) != kSpriteFace)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 6);
                func_10E50(i);
            }
            else if (vdi2 >= 400 && vdi2 < 433)
            {
                ChangeSpriteStat(i, 4);
                func_10E50(i);
            }
            else if (vdi2 >= 450 && vdi2 < 460)
            {
                ChangeSpriteStat(i, 11);
                func_10E50(i);
            }
            else if (vdi2 == 709)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                pSprite->shade = -128;
                ChangeSpriteStat(i, 0);
            }
            else if (vdi2 == 710)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                pSprite->shade = -128;
                ChangeSpriteStat(i, 12);
            }
            else if (vdi2 == 711)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                pSprite->shade = -128;
                ChangeSpriteStat(i, 0);
            }
            else
            {
                ChangeSpriteStat(i, 0);
            }
            pSprite->type = vdi2;
            if (struct_CAF10[j].at2 >= 0)
                pSprite->picnum = struct_CAF10[j].at2;
            if (struct_CAF10[j].at4 >= 0)
                pSprite->xrepeat = struct_CAF10[j].at4;
            if (struct_CAF10[j].at6 >= 0)
                pSprite->yrepeat = struct_CAF10[j].at6;

            if (struct_CAF10[j].at8 == 0)
                pSprite->cstat &= ~0x100;
            else if (struct_CAF10[j].at8 > 0)
                pSprite->cstat |= 0x100;
            if (struct_CAF10[j].at9 >= 0)
                pSprite->pal = struct_CAF10[j].at9;

            if (pSprite->statnum == 4 || pSprite->statnum == 6)
            {
                int top, bottom;
                GetSpriteExtents(pSprite, &top, &bottom);
                if (!(sector[pSprite->sectnum].ceilingstat & kSectorStat0))
                {
                    pSprite->z += ClipLow(sector[pSprite->sectnum].ceilingz - top, 0);
                }
                if (!(sector[pSprite->sectnum].floorstat & kSectorStat0))
                {
                    pSprite->z += ClipHigh(sector[pSprite->sectnum].floorz - bottom, 0);
                }
            }
        }
    }
    int vbx = gCorrectedSprites;
    for (i = 0; i < kMaxSprites; i++)
    {
        SPRITE* pSprite = &sprite[i];
        if ((pSprite->statnum == 4 && (pSprite->type < 400 || pSprite->type >= 433))
           || (pSprite->statnum == 3 && (pSprite->type < 100 || pSprite->type >= 433) && (pSprite->type < 60 || pSprite->type >= 81) && (pSprite->type < 40 || pSprite->type >= 51))
            || (pSprite->statnum == 6 && (pSprite->type < 200 || pSprite->type >= 254))
             || pSprite->statnum == 1)
        {
            pSprite->statnum = 0;
            gCorrectedSprites++;
        }
    }
    if (vbx != gCorrectedSprites)
    {
        char buffer[40];
        sprintf(buffer, "Fixed %d sprites", gCorrectedSprites - vbx);
        if (qsetmode == 200)
            scrSetMessage(buffer);
        else
            printmessage16(buffer);
    }
}

void func_1159C(int nSprite) // ???
{
    short v800[1024];
    memset(v800, 0, sizeof(v800));
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            v800[sprite[i].type]++;
        }
    }
}

const char* ExtGetSectorCaption(short nSector)
{
    char v100[256];
    dassert(nSector >= 0 && nSector < kMaxSectors, 1366+LDIFF1);
    int nXSector = sector[nSector].extra;
    char_CA8A8[0] = 0;
    if (nXSector > 0)
    {
        if (xsector[nXSector].at8_0 > 0)
            sprintf(char_CA8A8, "%i:", xsector[nXSector].at8_0);

        strcat(char_CA8A8, int_DBA90[sector[nSector].type]);

        if (xsector[nXSector].at6_0 > 0)
        {
            sprintf(v100, ":%i", xsector[nXSector].at6_0);
            strcat(char_CA8A8, v100);
        }

        if (xsector[nXSector].at14_0)
        {
            sprintf(v100, " PAN(%i,%i)", xsector[nXSector].at15_0, xsector[nXSector].at14_0);
            strcat(char_CA8A8, v100);
        }

        strcat(char_CA8A8, " ");
        strcat(char_CA8A8, int_D9A88[xsector[nXSector].at1_6]);
    }
    else if (sector[nSector].type != 0 || sector[nSector].hitag != 0)
    {
        sprintf(char_CA8A8, "{%i:%i}", sector[nSector].hitag, sector[nSector].type);
    }
    return char_CA8A8;
}

const char* ExtGetWallCaption(short nWall)
{
    char v100[256];
    dassert(nWall >= 0 && nWall < kMaxWalls, 1413+LDIFF1);
    int nXWall = wall[nWall].extra;
    char_CA8A8[0] = 0;
    if (nXWall > 0)
    {
        if (xwall[nXWall].at8_0 > 0)
        {
            sprintf(v100, "%i:", xwall[nXWall].at8_0);
            strcat(char_CA8A8, v100);
        }

        strcat(char_CA8A8, int_DAA90[wall[nWall].type]);

        if (xwall[nXWall].at6_0 > 0)
        {
            sprintf(v100, ":%i", xwall[nXWall].at6_0);
            strcat(char_CA8A8, v100);
        }

        if (xwall[nXWall].atd_7 || xwall[nXWall].ate_7)
        {
            sprintf(v100, " PAN(%i,%i)", xwall[nXWall].atd_7, xwall[nXWall].ate_7);
            strcat(char_CA8A8, v100);
        }

        strcat(char_CA8A8, " ");
        strcat(char_CA8A8, int_D9A88[xwall[nXWall].at1_6]);
    }
    else if (wall[nWall].type != 0 || wall[nWall].hitag != 0)
    {
        sprintf(char_CA8A8, "{%i:%i}", wall[nWall].hitag, wall[nWall].type);
    }
    return char_CA8A8;
}

const char* ExtGetSpriteCaption(short nSprite)
{
    char v100[256];
    dassert(nSprite >= 0 && nSprite < kMaxSprites, 1461+LDIFF1);
    SPRITE* pSprite = &sprite[nSprite];
    if (pSprite->type == 0)
        return "";
    if (pSprite->statnum == 10)
        return "";

    const char* pzString = int_D9A90[pSprite->type];
    if (!pzString)
        return "";

    int nXSprite = pSprite->extra;
    if (nXSprite > 0)
    {
        XSPRITE* pXSprite = &xsprite[nXSprite];
        switch (pSprite->type)
        {
        case 1:
        case 2:
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 18:
        case 19:
            sprintf(char_CA8A8, "%s [%d]", pzString, pXSprite->at10_0);
            return char_CA8A8;
        }
        char_CA8A8[0] = 0;
        if (pXSprite->at5_2 > 0)
        {
            sprintf(v100, "%i:", pXSprite->at5_2);
            strcat(char_CA8A8, v100);
        }

        strcat(char_CA8A8, pzString);

        if (pXSprite->at4_0 > 0)
        {
            sprintf(v100, ":%i", pXSprite->at4_0);
            strcat(char_CA8A8, v100);
        }

        if (pSprite->type >= 20 && pSprite->type < 33)
        {
            strcat(char_CA8A8, " ");
            strcat(char_CA8A8, int_D9A88[pXSprite->at1_6]);
        }
        return char_CA8A8;
    }

    return pzString;
}

void ExtShowSectorData(short nSector)
{
    if (keystatus[bsc_LAlt] | keystatus[bsc_RAlt])
        EditSectorData(nSector);
    else
        ShowSectorData(nSector);
}

void ExtShowWallData(short nWall)
{
    if (keystatus[bsc_LAlt] | keystatus[bsc_RAlt])
        EditWallData(nWall);
    else
        ShowWallData(nWall);
}

void ExtShowSpriteData(short nSprite)
{
    if (keystatus[bsc_LCtrl] | keystatus[bsc_RCtrl])
        func_1159C(nSprite);
    else if (keystatus[bsc_LAlt] | keystatus[bsc_RAlt])
        EditSpriteData(nSprite);
    else
        ShowSpriteData(nSprite);
}

void ExtEditSectorData(short nSector)
{
#if APPVER_BLOODREV >= AV_BR_BL111A
    if (keystatus[bsc_LAlt] | keystatus[bsc_RAlt])
        gXTracker.TrackSector(nSector, 0);
    else
        gXTracker.TrackSector(nSector, 1);
#endif
}

void ExtEditWallData(short nWall)
{
#if APPVER_BLOODREV >= AV_BR_BL111A
    if (keystatus[bsc_LAlt] | keystatus[bsc_RAlt])
        gXTracker.TrackWall(nWall, 0);
    else
        gXTracker.TrackWall(nWall, 1);
#endif
}

void ExtEditSpriteData(short nSprite)
{
#if APPVER_BLOODREV >= AV_BR_BL111A
    if (keystatus[bsc_LAlt] | keystatus[bsc_RAlt])
        gXTracker.TrackSprite(nSprite, 0);
    else
        gXTracker.TrackSprite(nSprite, 1);
#endif
}

void inittimer(void)
{
}

void uninittimer(void)
{
}

void faketimerhandler(void)
{
}

void ExtSaveMap(const char* mapname)
{
}

void ExtLoadMap(const char* mapname)
{
    int i;
    for (i = 0; i < numsectors; i++)
    {
        tilePreloadTile(sector[i].ceilingpicnum);
        tilePreloadTile(sector[i].floorpicnum);
    }
    for (i = 0; i < numwalls; i++)
    {
        tilePreloadTile(wall[i].picnum);
        if (wall[i].overpicnum >= 0)
            tilePreloadTile(wall[i].overpicnum);
    }
    for (i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
            tilePreloadTile(sprite[i].picnum);
    }
}

void setbrightness(char, char*)
{
    scrSetGamma(gGamma);
    scrSetDac();
}

int loadboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum)
{
    if (access(filename, 0) == -1)
        return -1;

    ulong crc;
    dbLoadMap(filename, daposx, daposy, daposz, daang, dacursectnum, &crc);
    func_1058C();
    if (qsetmode != 200)
    {
        sprintf(char_CA8A8, "Map Revisions: %i", gMapRev);
        printext16(4, 28, 11, 8, char_CA8A8, 0);
    }
    return 0;
}

void func_12010(void)
{
    ++gGameClock;
    totalclock = gGameClock;
    keytimerstuff();
}

void initkeys(void)
{
    keyInstall();
}

void uninitkeys(void)
{
    keyRemove();
}

int saveboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum)
{
    UndoSectorLighting();
    func_1058C();
    func_10EA0();
    char_1A76C6 = char_1A76C8 = char_1A76C7 = 1;
    dbSaveMap(filename, *daposx, *daposy, *daposz, *daang, *dacursectnum);

    asksave = 0;
    return 0;
}

void ExtPreCheckKeys(void)
{
    if (qsetmode == 200)
    {
        visibility = gVisibility;
        DoSectorLighting();
        switch (vidoption)
        {
            case 0:
                break;
            case 1:
                gPageTable[0].begin = (int)frameplace;
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                break;
        }
    }
}

void ExtAnalyzeSprites(void)
{
    int i;
    int nSprite;
    int nXSprite;
    for (i = 0; i < spritesortcnt; i++)
    {
        SPRITE* pTSprite = &tsprite[i];
        int nTile = pTSprite->picnum;
        dassert(nTile >= 0 && nTile < kMaxTiles, 1908+LDIFF2);
        nSprite = pTSprite->owner;
        dassert(nSprite >= 0 && nSprite < kMaxSprites,1911+LDIFF2);
        int nShade = pTSprite->shade;
        SECTOR* pSector = &sector[pTSprite->sectnum];
        if ((pSector->ceilingstat & kSectorStat0) && !(pSector->floorstat & kSectorStat15))
        {
            nShade += pSector->ceilingshade+tileShade[pSector->ceilingpicnum];
        }
        else
        {
            nShade += pSector->floorshade+tileShade[pSector->floorpicnum];
        }
        nShade += tileShade[pTSprite->picnum];
        pTSprite->shade = ClipRange(nShade, -128, 127);
        nXSprite = pTSprite->extra;
        int vsi = 0;
        switch (picanm[nTile].at3_4)
        {
            case 0:
                if (nXSprite > 0)
                {
                    dassert(nXSprite < kMaxXSprites, 1934+LDIFF2);
                    switch (sprite[nSprite].type)
                    {
                    case 20:
                    case 21:
                        if (xsprite[nXSprite].at1_6)
                            vsi = 1;
                        break;
                    case 22:
                        vsi = xsprite[nXSprite].at10_0;
                        break;
                    }
                }
                break;
            case 1:
            {
                long dx = posx - pTSprite->x;
                long dy = posy - pTSprite->y;
                RotateVector(&dx, &dy, -pTSprite->ang + 128);
                int octant = GetOctant(dx, dy);
                if (octant <= 4)
                {
                    vsi = octant;
                    pTSprite->cstat &= ~0x04;
                }
                else
                {
                    vsi = 8 - octant;
                    pTSprite->cstat |= 0x04;
                }
                break;
            }
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
        }
        for (; vsi > 0; vsi--)
            pTSprite->picnum += 1 + picanm[pTSprite->picnum].animframes;
    }
}

void ExtCheckKeys(void)
{
    gFrameTicks = gGameClock - gFrameClock;
    gFrameClock += gFrameTicks;
    sndProcess();
    if (int_DCF10 + gAutoSaveInterval < gFrameClock)
    {
        int_DCF10 = gFrameClock;
        if (asksave)
        {
            UndoSectorLighting();
            func_1058C();
            func_10EA0();
            dbSaveMap("AUTOSAVE.MAP", posx, posy, posz, ang, cursectnum);
            if (qsetmode == 200)
                scrSetMessage("Map autosaved to AUTOSAVE.MAP");
            else
                printmessage16("Map autosaved to AUTOSAVE.MAP");
        }
    }
    CalcFrameRate();
    if (qsetmode == 200)
    {
        UndoSectorLighting();
        Check3DKeys();
        sprintf(char_CA8A8, "%3i", gFrameRate);
        printext256(xdim-12, 0, gStdColor[15], -1, char_CA8A8, 1);
        scrDisplayMessage(gStdColor[15]);
        if (char_CA89C)
            DoSectorPanning();
    }
    else
    {
        CheckKeys2D();
        sprintf(char_CA8A8, "%3i", gFrameRate);
        printext16(616, pageoffset / 640, 15, -1, char_CA8A8, 0);
    }
}

void MapEditErrorHandler(const Error& err)
{
    timerRemove();
    sndTerm();
    uninitengine();
    keyRemove();
    setvmode(gOldDisplayMode);
    prevErrorHandler(err);
}

void ExtInit(void)
{
    char buf[256];

    if (_grow_handles(40) < 40)
        ThrowError(2079+LDIFF2)("Not enough file handles available");

    gOldDisplayMode = getvmode();

    sprintf(buf, "MapEdit Build %s [%s]", GetVersionString(), gBuildDate);
    tioInit(1);
    tioCenterString(0, 0, tioScreenCols-1, buf, 0x2f);
    tioCenterString(tioScreenRows-1, 0, tioScreenCols-1, "Copyright (c) 1994-1997 Monolith Productions", 0x2f);
    tioWindow(1, 0, tioScreenRows-2, tioScreenCols);
    tioPrint("");
    tioPrint("YOUR USE OF THIS PROGRAM IS RESTRICED UNDER YOUR LICENSE AGREEMENT");
#ifdef PLASMAPAK
    tioPrint("");
    tioPrint("");
    tioPrint("            PLASMA PAK version");
#endif
    tioPrint("");
    tioPrint("Initializing heap and resource system");
    Resource::heap = new QHeap(dpmiDetermineMaxRealAlloc());
    tioPrint("Initializing resource archive");
    gSysRes.Init("BLOOD.RFF", "*.*");
    gGuiRes.Init("GUI.RFF", NULL);
    atexit(ExitMsg);
    CONFIG_ReadSetup();
    tioPrint("Loading preferences");

    gBeep = gMapEditIni.GetKeyInt("Options", "Beep", 1);
    gHighlightThreshold = gMapEditIni.GetKeyInt("Options", "HighlightThreshold", 40);
    gStairHeight = gMapEditIni.GetKeyInt("Options", "StairHeight", 8);
    gOldKeyMapping = gMapEditIni.GetKeyInt("Options", "OldKeyMapping", 0);
    gAutoSaveInterval = (unsigned char)gMapEditIni.GetKeyInt("Options", "AutoSaveInterval", 300) * 120;
    gLightBombIntensity = gMapEditIni.GetKeyInt("LightBomb", "Intensity", 16);
    gLightBombAttenuation = gMapEditIni.GetKeyInt("LightBomb", "Attenuation", 4096);
    gLightBombReflections = gMapEditIni.GetKeyInt("LightBomb", "Reflections", 2);
    gLightBombMaxBright = gMapEditIni.GetKeyInt("LightBomb", "MaxBright", -4);
    gLightBombRampDist = gMapEditIni.GetKeyInt("LightBomb", "RampDist", 65536);
    FillStringLists();
    tioPrint("Initializing mouse");
    if (!initmouse())
        tioPrint("Mouse not detected");
    prevErrorHandler = errSetHandler(MapEditErrorHandler);
    scrInit();
    tioPrint("Loading tiles");
    if (!SafeFileExists("TILES000.ART"))
    {
        tioPrint("No REGISTERED art found.  Trying to use SHAREWARE.");
        tileInit(FALSE, "SHARE%03i.ART");
    }
    if (!tileInit(FALSE, NULL))
        ThrowError(2153+LDIFF2)("ART files not found");
    tioPrint("Loading cosine table");
    trigInit(gSysRes);
    tioPrint("Creating standard color lookups");
    scrCreateStdColors();
    tioPrint("Installing timer");
    timerRegisterClient(func_12010, 120);
    timerInstall();
    tioPrint("Engaging sound subsystem...");
    sndInit(FALSE);
    dbInit();
    
    visibility = 800;
    kensplayerheight = 0x3700;
    zmode = 0;
    showinvisibility = 1;
    defaultspritecstat = 0x80;
    for (int i = 0; i < kMaxSectors; i++)
    {
        sector[i].extra = -1;
    }
    for (i = 0; i < kMaxWalls; i++)
    {
        wall[i].extra = -1;
    }
    for (i = 0; i < kMaxSprites; i++)
    {
        sprite[i].extra = -1;
    }

    scrSetGameMode(ScreenMode, ScreenWidth, ScreenHeight);
    Mouse::SetRange(xdim, ydim);
}

void ExtUnInit(void)
{
    timerRemove();
    sndTerm();
    unlink("AUTOSAVE.MAP");
}

