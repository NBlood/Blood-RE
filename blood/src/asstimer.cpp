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
#include <i86.h>
#include <stdlib.h>
#include <string.h>
#include "typedefs.h"
#include "globals.h"
#include "error.h"
#include "satimer.h"
extern "C" {
#include "task_man.h"
}

struct CLIENT_INFO {
    void (*pCall)(void);
    task *pTask;
};

static CLIENT_INFO client[16];
static int nClients;
static BOOL timerActive;

void dpmiLock(void* ptr, int size);
#pragma aux dpmiLock = \
"shld ebx, ecx, 16" \
"shld edi, edi, 16" \
"mov ax, 0x600" \
"int 0x31" \
parm nomemory [ecx][edi] \
modify exact[ebx edi ax]

void dpmiUnlock(void* ptr, int size);
#pragma aux dpmiUnlock = \
"shld ebx, ecx, 16" \
"shld edi, edi, 16" \
"mov ax, 0x601" \
"int 0x31" \
parm nomemory [ecx][edi] \
modify exact[ebx edi ax]

void timerRemove(void)
{
    if (timerActive)
    {
        for (int i = 0; i < nClients; i++)
        {
            TS_Terminate(client[i].pTask);
        }
        TS_Shutdown();
        nClients = 0;
        dpmiUnlock(client, sizeof(client));
        dpmiUnlock(&nClients, sizeof(nClients));
        timerActive = FALSE;
    }
}

void timerInstall(void)
{
    if (!timerActive)
    {
        timerActive = TRUE;
        dpmiLock(client, sizeof(client));
        dpmiLock(&nClients, sizeof(nClients));
        TS_Dispatch();
        atexit(timerRemove);
    }
}

void func_3206C()
{
    ThrowError(76)("Call to unimplemented glue function");
}

typedef void (*task_cast)(task*);

void timerRegisterClient(void(*pCall)(void), int nRate)
{
    if (nClients >= 16)
        return;
    client[nClients].pCall = pCall;
    client[nClients].pTask = TS_ScheduleTask((task_cast)pCall, nRate, 1, NULL);
    nClients++;
}

void timerRemoveClient(void(*pCall)(void))
{
    int i;
    for (i = 0; i < nClients; i++)
    {
        if (client[i].pCall == pCall)
            break;
    }
    if (i >= nClients)
        return;
    _disable();
    client[i] = client[--nClients];
    _enable();
}

void timerSetClientRate(void(*pCall)(void), int nRate)
{
    int i;
    for (i = 0; i < nClients; i++)
    {
        if (client[i].pCall == pCall)
            break;
    }
    if (i >= nClients)
        return;
    TS_SetTaskRate(client[i].pTask, nRate);
}
