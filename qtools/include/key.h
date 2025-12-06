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
#ifndef _KEY_H_
#define _KEY_H_

#include "typedefs.h"

extern "C" volatile byte keystatus[256];

extern byte ScanToAscii[];
extern byte ScanToAsciiShifted[];

#define bsc_Esc 0x01
#define bsc_1 0x02
#define bsc_2 0x03
#define bsc_Minus 0x0c
#define bsc_Plus 0x0d
#define bsc_Backspace 0x0e
#define bsc_Tab 0x0f
#define bsc_W 0x11
#define bsc_E 0x12
#define bsc_R 0x13
#define bsc_T 0x14
#define bsc_Y 0x15
#define bsc_U 0x16
#define bsc_I 0x17
#define bsc_O 0x18
#define bsc_P 0x19
#define bsc_OpenBracket 0x1a
#define bsc_CloseBracket 0x1b
#define bsc_Enter 0x1c
#define bsc_LCtrl 0x1d
#define bsc_A 0x1e
#define bsc_S 0x1f
#define bsc_D 0x20
#define bsc_F 0x21
#define bsc_G 0x22
#define bsc_H 0x23
#define bsc_K 0x25
#define bsc_L 0x26
#define bsc_LShift 0x2a
#define bsc_BackSlash 0x2b
#define bsc_Z 0x2c
#define bsc_X 0x2d
#define bsc_C 0x2e
#define bsc_V 0x2f
#define bsc_B 0x30
#define bsc_N 0x31
#define bsc_M 0x32
#define bsc_Comma 0x33
#define bsc_Period 0x34
#define bsc_Slash 0x35
#define bsc_RShift 0x36
#define bsc_Pad_Star 0x37
#define bsc_LAlt 0x38
#define bsc_SpaceBar 0x39
#define bsc_CapsLock 0x3a
#define bsc_F1 0x3b
#define bsc_F2 0x3c
#define bsc_F3 0x3d
#define bsc_F4 0x3e
#define bsc_F5 0x3f
#define bsc_F6 0x40
#define bsc_F7 0x41
#define bsc_F8 0x42
#define bsc_F9 0x43
#define bsc_F10 0x44
#define bsc_NumLock 0x45
#define bsc_ScrlLock 0x46
#define bsc_Pad_7 0x47
#define bsc_Pad_8 0x48
#define bsc_Pad_9 0x49
#define bsc_Pad_Minus 0x4a
#define bsc_Pad_4 0x4b
#define bsc_Pad_5 0x4c
#define bsc_Pad_6 0x4d
#define bsc_Pad_Plus 0x4e
#define bsc_Pad_1 0x4f
#define bsc_Pad_2 0x50
#define bsc_Pad_3 0x51
#define bsc_Pad_0 0x52
#define bsc_Pad_Period 0x53
#define bsc_SysReq 0x54
#define bsc_F11 0x57
#define bsc_F12 0x58
#define bsc_Pad_Enter 0x9c
#define bsc_RCtrl 0x9d
#define bsc_Pad_Slash 0xb5
#define bsc_PrntScrn 0xb7
#define bsc_RAlt 0xb8
#define bsc_Pause 0xc5
#define bsc_Break 0xc6
#define bsc_Home 0xc7
#define bsc_Up 0xc8
#define bsc_PgUp 0xc9
#define bsc_Left 0xcb
#define bsc_Right 0xcd
#define bsc_End 0xcf
#define bsc_Down 0xd0
#define bsc_PgDn 0xd1
#define bsc_Ins 0xd2
#define bsc_Del 0xd3

extern byte (*keyCallback)(byte, BOOL);

void keyInstall(void);
void keyRemove(void);
byte keyGet(void);

void keyFlushStream(void);


#endif
