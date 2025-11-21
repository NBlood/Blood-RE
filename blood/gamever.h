// *** VERSIONS RESTORATION ***
#ifndef GAMEVER_H
#define GAMEVER_H

// It is assumed here that:
// 1. The compiler is set up to appropriately define APPVER_EXEDEF
// as an EXE identifier.
// 2. This header is included (near) the beginning of every compilation unit,
// in order to have an impact in any place where it's expected to.


#define AV_BT_SHAREWARE 0
#define AV_BT_SWRETAIL 1
#define AV_BT_REGISTERED 2
#define AV_BT_PLASMAPAK 3

// APPVER_BLOODREV definitions
#define AV_BR_BL110 199708120
#define AV_BR_BS110 (AV_BR_BL110+AV_BT_SHAREWARE)
#define AV_BR_BSR110 (AV_BR_BL110+AV_BT_SWRETAIL)
#define AV_BR_BR110 (AV_BR_BL110+AV_BT_REGISTERED)
#define AV_BR_BP110 (AV_BR_BL110+AV_BT_PLASMAPAK)
#define AV_BR_BL111 199708190 // 19 August 1997
#define AV_BR_BS111 (AV_BR_BL111+AV_BT_SHAREWARE)
#define AV_BR_BSR111 (AV_BR_BL111+AV_BT_SWRETAIL)
#define AV_BR_BR111 (AV_BR_BL111+AV_BT_REGISTERED)
#define AV_BR_BP111 (AV_BR_BL111+AV_BT_PLASMAPAK)
#define AV_BR_BL111A 199709230 // 23 September 1997
#define AV_BR_BS111A (AV_BR_BL111A+AV_BT_SHAREWARE)
#define AV_BR_BSR111A (AV_BR_BL111A+AV_BT_SWRETAIL)
#define AV_BR_BR111A (AV_BR_BL111A+AV_BT_REGISTERED)
#define AV_BR_BP111A (AV_BR_BL111A+AV_BT_PLASMAPAK)
#define AV_BR_BL120 199803180 // 3DFX
#define AV_BR_BR120 (AV_BR_BL120+AV_BT_REGISTERED)
#define AV_BR_BR120FX (AV_BR_BL120+AV_BT_REGISTERED)
#define AV_BR_BP120 (AV_BR_BL120+AV_BT_PLASMAPAK)
#define AV_BR_BP120FX (AV_BR_BL120+AV_BT_PLASMAPAK)
#define AV_BR_BL121 199807150 // One Unit Whole Blood
#define AV_BR_BP121 (AV_BR_BL121+AV_BT_PLASMAPAK)

// Now define APPVER_BLOODREV to one of the above, based on APPVER_EXEDEF

#define APPVER_CONCAT1(x,y) x ## y
#define APPVER_CONCAT2(x,y) APPVER_CONCAT1(x,y)
#define APPVER_BLOODREV APPVER_CONCAT2(AV_BR_,APPVER_EXEDEF)

#if (APPVER_BLOODREV%5==AV_BT_SHAREWARE)
#define SHAREWARE
#endif
#if (APPVER_BLOODREV%5==AV_BT_SWRETAIL)
#define SHAREWARE
#define SWRETAIL
#endif
#if (APPVER_BLOODREV%5==AV_BT_REGISTERED)
#define REGISTERED
#endif
#if (APPVER_BLOODREV%5==AV_BT_PLASMAPAK)
#define REGISTERED
#define PLASMAPAK
#endif

#if APPVER_BLOODREV >= AV_BR_BL111A
#define PLASMAPAK111
#else
#ifdef PLASMAPAK
#define PLASMAPAK111
#endif
#endif

#endif // GAMEVER_H
