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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "typedefs.h"
#include "error.h"

void DefaultHandler(const Error& err);
ErrorHandler curHandler = DefaultHandler;

char *module;
int line;
BOOL inHandler;
char char_3DF0C4[80];
BOOL gErrorFlag;

ErrorHandler errSetHandler(ErrorHandler eh)
{
    ErrorHandler old = curHandler;
    curHandler = eh;
    return old;
}

void DefaultHandler(const Error &err)
{
    sprintf(char_3DF0C4, "%s(%i): %s\n", err.f_0, err.f_4, err.f_8);
    printf(char_3DF0C4);
    _exit(1);
}

void _SetErrorLoc(char *_module, int _line)
{
    module = _module;
    line = _line;
}

void _ThrowError(char *s, ...)
{
    char buffer[256];
    gErrorFlag = TRUE;
    va_list args;
    va_start(args, s);
    vsprintf(buffer, s, args);
    va_end(args);
    if (!inHandler)
    {
        inHandler = TRUE;
        curHandler(Error(module, line, buffer));
    }
}

char *getErrorMsg(void)
{
    if (gErrorFlag)
        return char_3DF0C4;
    return NULL;
}

BOOL getErrorFlag(void)
{
    return gErrorFlag;
}

