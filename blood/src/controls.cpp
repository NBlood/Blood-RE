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
#include <stdlib.h>
#include "typedefs.h"
#include "globals.h"
#include "types.h"
#include "config.h"
#include "controls.h"
#include "function.h"
#include "globals.h"
#include "keyboard.h"
#include "key.h"
#include "levels.h"
#include "loadsave.h"
#include "map2d.h"
#include "misc.h"
#include "view.h"

INPUT gInput;

#if APPVER_BLOODREV < AV_BR_BL111
BOOL gMouseAimDynamic;
BOOL gMouseAimCenter;
BOOL gMouseAim;
#endif

BOOL bSilentAim;

int iTurnCount;

byte translateCode[256];

int32 GetTime(void)
{
    return gGameClock;
}

byte SendCodes(byte a1, BOOL a2)
{
    KB_KeyEvent(translateCode[a1], a2);
    return a1;
}

void ctrlInit(void)
{
    memset(translateCode, 0, sizeof(translateCode));
    for (int i = 0; i < 128; i++)
        translateCode[i] = i;

    translateCode[bsc_Pad_Enter] = sc_kpad_Enter;
    translateCode[bsc_RCtrl] = sc_RightControl;
    translateCode[bsc_Pad_Slash] = sc_kpad_Slash;
    translateCode[bsc_PrntScrn] = sc_PrintScreen;
    translateCode[bsc_RAlt] = sc_RightAlt;
    translateCode[bsc_Pause] = sc_Pause;
    translateCode[bsc_Break] = sc_Pause;
    translateCode[bsc_Home] = sc_Home;
    translateCode[bsc_Up] = sc_UpArrow;
    translateCode[bsc_PgUp] = sc_PgUp;
    translateCode[bsc_Left] = sc_LeftArrow;
    translateCode[bsc_Right] = sc_RightArrow;
    translateCode[bsc_End] = sc_End;
    translateCode[bsc_Down] = sc_DownArrow;
    translateCode[bsc_PgDn] = sc_PgDn;
    translateCode[bsc_Ins] = sc_Insert;
    translateCode[bsc_Del] = sc_Delete;
    keyCallback = SendCodes;

    KB_ClearKeysDown();
    KB_FlushKeyboardQueue();
    CONTROL_Startup(ControllerType, GetTime, 120);
    CONTROL_DefineFlag(gamefunc_Move_Forward, false);
    CONTROL_DefineFlag(gamefunc_Move_Backward, false);
    CONTROL_DefineFlag(gamefunc_Turn_Left, false);
    CONTROL_DefineFlag(gamefunc_Turn_Right, false);
    CONTROL_DefineFlag(gamefunc_Turn_Around, false);
    CONTROL_DefineFlag(gamefunc_Strafe, false);
    CONTROL_DefineFlag(gamefunc_Strafe_Left, false);
    CONTROL_DefineFlag(gamefunc_Strafe_Right, false);
    CONTROL_DefineFlag(gamefunc_Jump, false);
    CONTROL_DefineFlag(gamefunc_Crouch, false);
    CONTROL_DefineFlag(gamefunc_Run, false);
    CONTROL_DefineFlag(gamefunc_AutoRun, false);
    CONTROL_DefineFlag(gamefunc_Open, false);
    CONTROL_DefineFlag(gamefunc_Weapon_Fire, false);
    CONTROL_DefineFlag(gamefunc_Weapon_Special_Fire, false);
    CONTROL_DefineFlag(gamefunc_Aim_Up, false);
    CONTROL_DefineFlag(gamefunc_Aim_Down, false);
    CONTROL_DefineFlag(gamefunc_Aim_Center, false);
    CONTROL_DefineFlag(gamefunc_Look_Up, false);
    CONTROL_DefineFlag(gamefunc_Look_Down, false);
    CONTROL_DefineFlag(gamefunc_Tilt_Left, false);
    CONTROL_DefineFlag(gamefunc_Tilt_Right, false);
    CONTROL_DefineFlag(gamefunc_Weapon_1, false);
    CONTROL_DefineFlag(gamefunc_Weapon_2, false);
    CONTROL_DefineFlag(gamefunc_Weapon_3, false);
    CONTROL_DefineFlag(gamefunc_Weapon_4, false);
    CONTROL_DefineFlag(gamefunc_Weapon_5, false);
    CONTROL_DefineFlag(gamefunc_Weapon_6, false);
    CONTROL_DefineFlag(gamefunc_Weapon_7, false);
    CONTROL_DefineFlag(gamefunc_Weapon_8, false);
    CONTROL_DefineFlag(gamefunc_Weapon_9, false);
    CONTROL_DefineFlag(gamefunc_Weapon_10, false);
    CONTROL_DefineFlag(gamefunc_Inventory_Use, false);
    CONTROL_DefineFlag(gamefunc_Inventory_Left, false);
    CONTROL_DefineFlag(gamefunc_Inventory_Right, false);
    CONTROL_DefineFlag(gamefunc_Map_Toggle, false);
    CONTROL_DefineFlag(gamefunc_Map_Follow_Mode, false);
    CONTROL_DefineFlag(gamefunc_Shrink_Screen, false);
    CONTROL_DefineFlag(gamefunc_Enlarge_Screen, false);
    CONTROL_DefineFlag(gamefunc_Send_Message, false);
    CONTROL_DefineFlag(gamefunc_See_Coop_View, false);
    CONTROL_DefineFlag(gamefunc_See_Chase_View, false);
    CONTROL_DefineFlag(gamefunc_Mouse_Aiming, false);
    CONTROL_DefineFlag(gamefunc_Toggle_Crosshair, false);
    CONTROL_DefineFlag(gamefunc_Next_Weapon, false);
    CONTROL_DefineFlag(gamefunc_Previous_Weapon, false);
    CONTROL_DefineFlag(gamefunc_Holster_Weapon, false);
    CONTROL_DefineFlag(gamefunc_Show_Opponents_Weapon, false);
    CONTROL_DefineFlag(gamefunc_BeastVision, false);
    CONTROL_DefineFlag(gamefunc_CrystalBall, false);
    CONTROL_DefineFlag(gamefunc_JumpBoots, false);
    CONTROL_DefineFlag(gamefunc_MedKit, false);
    CONTROL_DefineFlag(gamefunc_ProximityBombs, false);
    CONTROL_DefineFlag(gamefunc_RemoteBombs, false);
}

void ctrlTerm(void)
{
    CONTROL_Shutdown();
}

void func_2906C(void)
{
    KB_ClearKeysDown();
    KB_FlushKeyboardQueue();
    keyFlushStream();
    for (int i = 0; i < 256; i++)
        keystatus[i] = 0;
}

void ctrlGetInput(void)
{
    schar forward = 0, strafe = 0;
    sshort turn = 0;
    memset(&gInput, 0, sizeof(gInput));
    ControlInfo info;
    CONTROL_GetInput(&info);

    if (gQuitRequest)
        gInput.keyFlags.quit = 1;

    if (gGameStarted && gInputMode != INPUT_MODE_2 && gInputMode != INPUT_MODE_1
        && BUTTON(gamefunc_Send_Message))
    {
        CONTROL_ClearButton(gamefunc_Send_Message);
        keyFlushStream();
        gInputMode = INPUT_MODE_2;
    }

    if (!gGameStarted)
        return;

    if (gInputMode != INPUT_MODE_0)
        return;

    if (BUTTON(gamefunc_AutoRun))
    {
        CONTROL_ClearButton(gamefunc_AutoRun);
        gAutoRun = !gAutoRun;
        if (gAutoRun)
            viewSetMessage("Auto run ON");
        else
            viewSetMessage("Auto run OFF");
    }

    if (BUTTON(gamefunc_Map_Toggle))
    {
        CONTROL_ClearButton(gamefunc_Map_Toggle);
        viewToggle(gViewMode);
    }

    if (BUTTON(gamefunc_Map_Follow_Mode))
    {
        CONTROL_ClearButton(gamefunc_Map_Follow_Mode);
        gFollowMap = !gFollowMap;
        gViewMap.FollowMode(gFollowMap);
    }

    if (BUTTON(gamefunc_Shrink_Screen))
    {
        if (gViewMode == 3)
        {
            CONTROL_ClearButton(gamefunc_Shrink_Screen);
            viewResizeView(gViewSize + 1);
#ifdef _3DFX
            if (gViewSize > 2)
                gViewSize = 2;
#endif
        }
        if (gViewMode == 2 || gViewMode == 4)
        {
            gZoom = ClipLow(gZoom - (gZoom >> 4), 64);
            gViewMap.SetZoom(gZoom);
        }
    }

    if (BUTTON(gamefunc_Enlarge_Screen))
    {
        if (gViewMode == 3)
        {
            CONTROL_ClearButton(gamefunc_Enlarge_Screen);
            viewResizeView(gViewSize - 1);
        }
        if (gViewMode == 2 || gViewMode == 4)
        {
            gZoom = ClipHigh(gZoom + (gZoom >> 4), 4096);
            gViewMap.SetZoom(gZoom);
        }
    }

    if (BUTTON(gamefunc_Toggle_Crosshair))
    {
        CONTROL_ClearButton(gamefunc_Toggle_Crosshair);
        gAimReticle = !gAimReticle;
    }

    if (BUTTON(gamefunc_Next_Weapon))
    {
        CONTROL_ClearButton(gamefunc_Next_Weapon);
        gInput.keyFlags.nextWeapon = 1;
    }

    if (BUTTON(gamefunc_Previous_Weapon))
    {
        CONTROL_ClearButton(gamefunc_Previous_Weapon);
        gInput.keyFlags.prevWeapon = 1;
    }

    if (BUTTON(gamefunc_Show_Opponents_Weapon))
    {
        CONTROL_ClearButton(gamefunc_Show_Opponents_Weapon);
        gShowWeapon = !gShowWeapon;
    }

    if (BUTTON(gamefunc_Jump))
        gInput.buttonFlags.jump = 1;

    if (BUTTON(gamefunc_Crouch))
        gInput.buttonFlags.crouch = 1;

    if (BUTTON(gamefunc_Weapon_Fire))
        gInput.buttonFlags.shoot = 1;

    if (BUTTON(gamefunc_Weapon_Special_Fire))
        gInput.buttonFlags.shoot2 = 1;

    if (BUTTON(gamefunc_Open))
    {
        CONTROL_ClearButton(gamefunc_Open);
        gInput.keyFlags.action = 1;
    }

    gInput.buttonFlags.lookUp = 0;
    gInput.buttonFlags.lookDown = 0;

    gInput.buttonFlags.lookUp = BUTTON(gamefunc_Look_Up);
    gInput.buttonFlags.lookDown = BUTTON(gamefunc_Look_Down);

    if (gInput.buttonFlags.lookUp || gInput.buttonFlags.lookDown)
        gInput.keyFlags.lookCenter = 1;
    else
    {
        gInput.buttonFlags.lookUp = BUTTON(gamefunc_Aim_Up);
        gInput.buttonFlags.lookDown = BUTTON(gamefunc_Aim_Down);
    }

    if (BUTTON(gamefunc_Aim_Center))
    {
        CONTROL_ClearButton(gamefunc_Aim_Center);
        gInput.keyFlags.lookCenter = 1;
    }

    if (gMouseAiming)
        gMouseAim = 0;

    if (BUTTON(gamefunc_Mouse_Aiming))
    {
        if (gMouseAiming)
            gMouseAim = 1;
        else
        {
            CONTROL_ClearButton(gamefunc_Mouse_Aiming);
            gMouseAim = !gMouseAim;
            if (gMouseAim)
            {
                if (!bSilentAim)
                    viewSetMessage("Mouse aiming ON");
            }
            else
            {
                if (!bSilentAim)
                    viewSetMessage("Mouse aiming OFF");
                gInput.keyFlags.lookCenter = 1;
            }
        }
    }
    else if (gMouseAiming)
        gInput.keyFlags.lookCenter = 1;

    gInput.keyFlags.spin180 = BUTTON(gamefunc_Turn_Around);

    if (BUTTON(gamefunc_Inventory_Left))
    {
        CONTROL_ClearButton(gamefunc_Inventory_Left);
        gInput.keyFlags.prevItem = 1;
    }

    if (BUTTON(gamefunc_Inventory_Right))
    {
        CONTROL_ClearButton(gamefunc_Inventory_Right);
        gInput.keyFlags.nextItem = 1;
    }

    if (BUTTON(gamefunc_Inventory_Use))
    {
        CONTROL_ClearButton(gamefunc_Inventory_Use);
        gInput.keyFlags.useItem = 1;
    }

    if (BUTTON(gamefunc_BeastVision))
    {
        CONTROL_ClearButton(gamefunc_BeastVision);
        gInput.useFlags.useBeastVision = 1;
    }

    if (BUTTON(gamefunc_CrystalBall))
    {
        CONTROL_ClearButton(gamefunc_CrystalBall);
        gInput.useFlags.useCrystalBall = 1;
    }

    if (BUTTON(gamefunc_JumpBoots))
    {
        CONTROL_ClearButton(gamefunc_JumpBoots);
        gInput.useFlags.useJumpBoots = 1;
    }

    if (BUTTON(gamefunc_MedKit))
    {
        CONTROL_ClearButton(gamefunc_MedKit);
        gInput.useFlags.useMedKit = 1;
    }

    for (int i = 0; i < 10; i++)
    {
        if (BUTTON(gamefunc_Weapon_1 + i))
        {
            CONTROL_ClearButton(gamefunc_Weapon_1 + i);
            gInput.newWeapon = 1 + i;
        }
    }

    if (BUTTON(gamefunc_ProximityBombs))
    {
        CONTROL_ClearButton(gamefunc_ProximityBombs);
        gInput.newWeapon = 11;
    }

    if (BUTTON(gamefunc_RemoteBombs))
    {
        CONTROL_ClearButton(gamefunc_RemoteBombs);
        gInput.newWeapon = 12;
    }

    if (BUTTON(gamefunc_Holster_Weapon))
    {
        CONTROL_ClearButton(gamefunc_Holster_Weapon);
        gInput.keyFlags.holsterWeapon = 1;
    }

    byte run = BUTTON(gamefunc_Run) || gAutoRun;
    byte run2 = BUTTON(gamefunc_Run);

    gInput.syncFlags.run = run;

    if (BUTTON(gamefunc_Move_Forward))
        forward += (1+run)*4;

    if (BUTTON(gamefunc_Move_Backward))
        forward -= (1+run)*4;

    BOOL turnLeft = 0, turnRight = 0;

    if (BUTTON(gamefunc_Strafe))
    {
        if (BUTTON(gamefunc_Turn_Left))
            strafe += (1 + run) * 4;
        if (BUTTON(gamefunc_Turn_Right))
            strafe -= (1 + run) * 4;
    }
    else
    {
        if (BUTTON(gamefunc_Strafe_Left))
            strafe += (1 + run) * 4;
        if (BUTTON(gamefunc_Strafe_Right))
            strafe -= (1 + run) * 4;
        if (BUTTON(gamefunc_Turn_Left))
            turnLeft = 1;
        if (BUTTON(gamefunc_Turn_Right))
            turnRight = 1;
    }

    if (turnLeft || turnRight)
        iTurnCount += 4;
    else
        iTurnCount = 0;

    if (turnLeft)
        turn -= ClipHigh(iTurnCount * 12, gTurnSpeed);
    if (turnRight)
        turn += ClipHigh(iTurnCount * 12, gTurnSpeed);

    if ((run2 || run) && iTurnCount > 24)
        turn <<= 1;

    if (BUTTON(gamefunc_Strafe))
        strafe = (schar)ClipRange(strafe - (info.dyaw>>8), -8, 8);
    else
        turn = (sshort)ClipRange(turn + (info.dyaw>>4), -1024, 1024);

    strafe = (schar)ClipRange(strafe-(info.dx>>3), -8, 8);

    if (gMouseAim)
    {
        if (info.dz < 0)
            gInput.mlook = (schar)ClipRange((info.dz+127)>>7, -127, 127);
        else
            gInput.mlook = (schar)ClipRange(info.dz>>7, -127, 127);
        if (gMouseAimingFlipped)
            gInput.mlook = -gInput.mlook;
    }
    else
        forward = (schar)ClipRange(forward - (info.dz>>8), -8, 8);

    if (keystatus[bsc_Pause])
    {
        gInput.keyFlags.pause = 1;
        keystatus[bsc_Pause] = 0;
    }

    if (gViewMap.Mode() == 0 && gViewMode == 4)
    {
        gViewMap.SetInput(forward, turn, strafe);
        forward = 0;
        turn = 0;
        strafe = 0;
    }
    gInput.forward = forward;
    gInput.strafe = strafe;
    gInput.turn = turn;
}

class ControlsLoadSave : public LoadSave
{
    virtual void Load(void);
    virtual void Save(void);
};

void ControlsLoadSave::Load(void)
{
#if APPVER_BLOODREV < AV_BR_BL111
    Read(&gMouseAimDynamic, 1);
    Read(&gMouseAimCenter, 1);
#endif
}

void ControlsLoadSave::Save(void)
{
#if APPVER_BLOODREV < AV_BR_BL111
    Write(&gMouseAimDynamic, 1);
    Write(&gMouseAimCenter, 1);
#endif
}

static ControlsLoadSave myLoadSave;

