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
#ifndef _ERROR_H_
#define _ERROR_H_

#include "typedefs.h"

struct Error {
    char *f_0;
    int f_4;
    char *f_8;
    char f_c;
    char __f_d[7];
    Error(char *m, int l, char *s) { f_0 = m; f_4 = l; f_8 = s; f_c = 0; }
};

typedef void (*ErrorHandler)(const Error &);

ErrorHandler errSetHandler(ErrorHandler eh);

void _SetErrorLoc(char *,int);
void _ThrowError(char *,...);
char* getErrorMsg(void);
BOOL getErrorFlag(void);

// #define ThrowError _SetErrorLoc(__FILE__,__LINE__), _ThrowError
#define ThrowError(line) _SetErrorLoc(__FILE__,line), _ThrowError

#endif
