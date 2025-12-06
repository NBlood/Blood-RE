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
#ifndef _MISC_H_
#define _MISC_H_

#include <stdlib.h>
#include "typedefs.h"

char *ReadLine(char *, int, char **);
BOOL FileRead(int, void*, ulong);
BOOL FileLoad(char*, void*, ulong);

void ChangeExtension(char *name, char *ext);

ulong qrand(void);

ulong func_A8B30(void);
ulong func_A8B50(void);
ulong dpmiDetermineMaxRealAlloc(void);

inline int Min(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

inline int Max(int a, int b)
{
    if (a < b)
        return b;
    else
        return a;
}

inline byte TestBitString(byte *pArray, int nIndex)
{
    return pArray[nIndex>>3] & (1<<(nIndex&7));
}

inline void SetBitString(byte *pArray, int nIndex)
{
    pArray[nIndex>>3] |= 1<<(nIndex&7);
}

inline void ClearBitString(byte *pArray, int nIndex)
{
    pArray[nIndex >> 3] &= ~(1 << (nIndex & 7));
}

inline int scale(int a1, int a2, int a3, int a4, int a5)
{
    return (a1-a2) * (a5-a4) / (a3-a2) + a4;
}

inline int IncRotate(int a1, int a2)
{
    a1++;
    if (a1 >= a2)
        a1 = 0;
    return a1;
}

inline int DecRotate(int a1, int a2)
{
    a1--;
    if (a1 < 0)
        a1 += a2;
    return a1;
}

inline int IncBy(int a, int b)
{
    a += b;
    int q = a % b;
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}

inline int DecBy(int a, int b)
{
    a--;
    int q = a % b;
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}

inline int IncStep(int a, int b)
{
    return (a + b) & (~(b - 1));
}

inline int DecStep(int a, int b)
{
    return (a - 1) & (~(b - 1));
}

inline int ClipLow(int a, int b)
{
    if (a < b)
        return b;
    else
        return a;
}

inline int ClipHigh(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

inline int ClipRange(int a, int b, int c)
{
    if (a < b)
        return b;
    if (a > c)
        return c;
    return a;
}

struct POINT2D {
    int x, y;
};

struct POINT3D {
    int x, y, z;
};

struct VECTOR2D {
    int dx, dy;
};

struct VECTOR3D {
    long dx, dy, dz;
};

int dmulscale30r(int, int, int, int);
#pragma aux dmulscale30r = \
"imul ebx", \
"xchg eax,esi", \
"mov ebx,edx", \
"imul edi", \
"add eax,esi", \
"adc edx,ebx", \
"add eax,20000000h", \
"adc edx,0", \
"shrd eax,edx,30", \
parm nomemory  [eax] [ebx] [esi] [edi] \
modify nomemory exact [eax ebx edx esi edi]


int klabs(int);
#pragma aux klabs = \
"test eax,eax", \
"jns short L1", \
"neg eax", \
"L1:", \
parm nomemory [eax] \
modify nomemory exact [eax]

int isneg(int);
#pragma aux isneg = \
"add eax,eax" \
"sbb eax,eax" \
parm nomemory [eax] \
modify nomemory exact [eax]

void setvmode(int);
#pragma aux setvmode = \
"int 0x10" \
parm nomemory [eax]

int getvmode(void);
#pragma aux getvmode = \
"mov ah,0xf"\
"int 0x10" \
"and eax,0xff"\
value [eax]

void int3(void);
#pragma aux int3 = \
"int 3"

int kscale(int, int, int);
#pragma aux kscale = \
"imul edx" \
"idiv ebx" \
parm nomemory [eax] [edx] [ebx] \
modify nomemory exact [eax edx]

int ksgn(int);
#pragma aux ksgn = \
"add eax,eax" \
"sbb edx,edx" \
"cmp edx,eax" \
"adc dl,0" \
parm nomemory [eax] \
value [edx] \
modify nomemory exact [eax edx]

int mulscale8(int, int);
#pragma aux mulscale8 = \
"imul edx" \
"shrd eax,edx,8" \
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx]

int mulscale14(int, int);
#pragma aux mulscale14 = \
"imul edx" \
"shrd eax,edx,14" \
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx]

int mulscale16(int, int);
#pragma aux mulscale16 = \
"imul edx" \
"shrd eax,edx,16" \
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx]

int mulscale24(int, int);
#pragma aux mulscale24 = \
"imul edx" \
"shrd eax,edx,24" \
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx]

int mulscale28(int, int);
#pragma aux mulscale28 = \
"imul edx" \
"shrd eax,edx,28" \
parm nomemory [eax] [edx] \
modify exact [eax edx]

int mulscale30(int, int);
#pragma aux mulscale30 = \
"imul edx" \
"shrd eax,edx,30" \
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx] \
value [eax]

int interpolate16(int, int, int);
#pragma aux interpolate16 = \
"sub ecx, ebx" \
"imul ecx" \
"shrd eax,edx,16" \
"add eax, ebx" \
parm nomemory [ebx] [ecx] [eax] \
modify nomemory exact [eax ebx ecx edx esi]

int divscale16(int, int);
#pragma aux divscale16 = \
"mov edx,eax" \
"sar edx,16" \
"shl eax,16" \
"idiv ebx" \
parm nomemory [eax] [ebx] \
modify nomemory exact [eax edx]

int divscale24(int, int);
#pragma aux divscale24 = \
"mov edx,eax" \
"sar edx,8" \
"shl eax,24" \
"idiv ebx" \
parm nomemory [eax] [ebx] \
modify nomemory exact [eax edx]

int divscale(int, int, int);
#pragma aux divscale = \
"mov edx,eax" \
"shl eax,cl" \
"neg cl" \
"sar edx,cl" \
"idiv ebx" \
parm nomemory [eax] [ebx] [ecx] \
modify nomemory exact [eax edx ecx]

void WaitVBL(void);
#pragma aux WaitVBL = \
"mov dx,0x3da" \
"L1:" \
"in al,dx" \
"test al,8" \
"jz short L1" \
"L2:" \
"in al,dx" \
"test al,1" \
"jz short L2" \
modify nomemory exact [eax edx]

int approxDist(int, int);
#pragma aux approxDist = \
"test eax,eax" \
"jns short L1" \
"neg eax" \
"L1:" \
"test edx,edx" \
"jns short L2" \
"neg edx" \
"L2:" \
"cmp eax,edx"\
"ja short L3"\
"lea eax,[eax+eax*2]"\
"shr eax,3"\
"jmp short L4"\
"L3:"\
"lea edx,[edx+edx*2]"\
"shr edx,3"\
"L4:"\
"add eax,edx"\
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx]

int mulscale16r(int, int);
#pragma aux mulscale16r = \
"imul edx" \
"add eax,0x8000"\
"adc edx,0"\
"shrd eax,edx,16" \
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx]

int mulscale30r(int, int);
#pragma aux mulscale30r = \
"imul edx" \
"add eax,0x20000000"\
"adc edx,0"\
"shrd eax,edx,30" \
parm nomemory [eax] [edx] \
modify nomemory exact [eax edx]


int dmulscale(int, int, int, int, int);
#pragma aux dmulscale = \
"imul ebx" \
"xchg eax,esi" \
"mov ebx,edx" \
"imul edi" \
"add eax,esi" \
"adc edx,ebx" \
"shrd eax,edx,cl" \
parm nomemory [eax] [ebx] [esi] [edi] [ecx] \
modify nomemory exact [eax ebx edx esi]


int dmulscale16(int, int, int, int);
#pragma aux dmulscale16 = \
"imul ebx" \
"xchg eax,esi" \
"mov ebx,edx" \
"imul edi" \
"add eax,esi" \
"adc edx,ebx" \
"shrd eax,edx,16" \
parm nomemory [eax] [ebx] [esi] [edi] \
modify nomemory exact [eax ebx edx esi]

int dmulscale30(int, int, int, int);
#pragma aux dmulscale30 = \
"imul ebx" \
"xchg eax,esi" \
"mov ebx,edx" \
"imul edi" \
"add eax,esi" \
"adc edx,ebx" \
"shrd eax,edx,30" \
parm nomemory [eax] [ebx] [esi] [edi] \
modify nomemory exact [eax ebx edx esi]

int dmulscale32(int, int, int, int);
#pragma aux dmulscale32 = \
"imul edx" \
"mov ebx,eax"\
"mov eax,esi"\
"mov esi,edx"\
"imul edi" \
"add eax,ebx" \
"adc edx,esi" \
parm nomemory [eax] [edx] [esi] [edi] \
modify exact [eax ebx edx esi]\
value [edx]


int mulscale(int, int, int);
#pragma aux mulscale = \
"imul edx"\
"shrd eax,edx,cl"\
parm nomemory [eax] [edx] [ecx]\
modify nomemory exact [eax edx]


int tmulscale16(int, int, int, int, int, int);
#pragma aux tmulscale16 = \
"imul edx"\
"xchg eax,ebx"\
"xchg edx,ecx"\
"imul edx"\
"add ebx,eax"\
"adc ecx,edx"\
"mov eax,esi"\
"imul edi"\
"add eax,ebx"\
"adc edx,ecx"\
"shrd eax,edx,16"\
parm nomemory [eax] [edx] [ebx] [ecx] [esi] [edi] \
modify exact [eax ebx ecx edx]

void debugTrap(void);
#pragma aux debugTrap = \
"int 3"

inline int QRandom(int n)
{
    return mulscale(qrand(), n, 15);
}

inline int QRandom2(int n)
{
    return mulscale(qrand(), n, 14) - n;
}

inline uint Random(int a1)
{
    return mulscale(rand(), a1, 15);
}

inline int Random2(int a1)
{
    return mulscale(rand(), a1, 14)-a1;
}

inline int Random3(int a1)
{
    return mulscale(rand()+rand(), a1, 15) - a1;
}

inline BOOL Chance(int a1)
{
    return rand() < (a1>>1);
}

#endif // !_MISC_H_
