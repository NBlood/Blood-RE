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
#ifndef _GUI_H_
#define _GUI_H_

#include "gui.h"
#include "resource.h"
extern Resource gGuiRes;

struct GEVENT_MOUSE
{
    int at0;
    int at4;
    int at8;
    int atc;
    int at10;
    int at14;
    byte at18; // double click
};

struct GEVENT_KEYBOARD
{
    byte at0; // ascii
    byte at1; // scan
    union {
        int number;
        struct {
            unsigned int at0 : 1; // shift
            unsigned int at1 : 1; // ctrl
            unsigned int at2 : 1; // alt
            unsigned int at3 : 1; // lshift
            unsigned int at4 : 1; // rshift
            unsigned int at5 : 1; // lctrl
            unsigned int at6 : 1; // rctrl
            unsigned int at7 : 1; // lalt
            unsigned int at8 : 1; // ralt
        } bits;
    } at2;
};

enum GEVENT_TYPE {
    GEVENT_TYPE_NONE = 0x00,
    GEVENT_TYPE_MOUSE_1 = 0x01,
    GEVENT_TYPE_MOUSE_2 = 0x02,
    GEVENT_TYPE_MOUSE_4 = 0x04,
    GEVENT_TYPE_MOUSE_8 = 0x08,
    GEVENT_TYPE_MOUSE_MASK = 0xFF,
    GEVENT_TYPE_KEYBOARD = 0x100,
};


struct GEVENT
{
    GEVENT_TYPE at0;
    int at4;
    union {
        GEVENT_MOUSE mouse;
        GEVENT_KEYBOARD keyboard;
    } at6;
};

enum MODAL_RESULT {
    MODAL_RESULT_0 = 0,
    MODAL_RESULT_1,
    MODAL_RESULT_2
};

class Widget
{
public:
    Widget(int a1, int a2, int a3, int a4) : at0(a1), at4(a2), at8(a3), atc(a4), at1c(0), at1d(0), at1e(0), at1f(0)
    {
    }
    virtual ~Widget() { };

    int at0;
    int at4;
    int at8;
    int atc;
    Widget* at10;
    Widget* at14;
    Widget* at18;
    char at1c;
    char at1d;
    char at1e;
    char at1f;
    BOOL Inside(int x, int y) {
        return x >= at0 && y >= at4 && x < at0+at8 && y < at4+atc;
    }
    virtual void Paint(int, int, char) = 0;
    virtual void HandleEvent(GEVENT*) = 0;
    virtual void EndModal(MODAL_RESULT a1)
    {
        at18->EndModal(a1);
    }
};

class HeadWidget : public Widget
{
public:
    HeadWidget() : Widget(0, 0, 0, 0) {}

    virtual void Paint(int, int, char) {}
    virtual void HandleEvent(GEVENT*) {}
};

class Label : public Widget
{
public:
    Label(int, int, char*);

    char at24[0x100];

    virtual void Paint(int, int, char);
    virtual void HandleEvent(GEVENT*) { };
};

class Container : public Widget
{
public:
    Container(int, int, int, int);
    virtual ~Container();
    char at24;
    MODAL_RESULT at25;
    Widget *at26;
    Widget *at2a;
    HeadWidget at2e;

    virtual void Paint(int, int, char);
    virtual void HandleEvent(GEVENT*);
    virtual void EndModal(MODAL_RESULT);
    virtual BOOL SetFocus(int);

    void Remove(Widget*);
    void Insert(Widget*);
};

class Panel : public Container
{
public:
    int at52, at56, at5a;
    Panel(int a1, int a2, int a3, int a4, int a5, int a6, int a7) : Container(a1, a2, a3, a4),
        at52(a5), at56(a6), at5a(a7) { at1c = 0; }
    virtual void Paint(int, int, char);
};

class TitleBar : public Widget
{
public:
    TitleBar(int, int, int, int, char*);
    virtual void Paint(int, int, char);
    virtual void HandleEvent(GEVENT*);
    char at24[256];
    int at124;
};

class Window : public Panel
{
public:
    Container* at5e;
    TitleBar* at62;
    Window(int, int, int, int, char*);
};

class Button : public Widget
{
public:
    Button(int a1, int a2, int a3, int a4, MODAL_RESULT a5) : Widget(a1, a2, a3, a4),
        at24(a5), at25(NULL), at29(0) {}
    Button(int a1, int a2, int a3, int a4, void (*a5)(Widget*)) : Widget(a1, a2, a3, a4),
        at24(MODAL_RESULT_0), at25(a5), at29(0) {}
    MODAL_RESULT at24;
    void (*at25)(Widget*);
    char at29; // pressed
    virtual void Paint(int, int, char);
    virtual void HandleEvent(GEVENT*);
};

class TextButton : public Button
{
public:
    TextButton(int, int, int, int, char*, MODAL_RESULT);
    char* at2a;
    virtual void Paint(int, int, char);
    virtual void HandleEvent(GEVENT*);
};

class BitButton : public Button
{
public:
    BitButton(int a1, int a2, int a3, int a4, DICTNODE *_bitmap, void (*a6)(Widget*)) :
        Button(a1, a2, a3, a4, a6), bitmap(_bitmap) {}
    DICTNODE* bitmap;
    virtual void Paint(int, int, char);
};

class EditText : public Widget
{
public:
    EditText(int, int, int, int, char*);
    char at24[0x100];
    int at124, at128, at12c;
    virtual void Paint(int, int, char);
    virtual void HandleEvent(GEVENT*);
};

class EditNumber : public EditText
{
public:
    EditNumber(int, int, int, int, int);
    int at130;
    virtual void HandleEvent(GEVENT*);
};

class ThumbButton : public Button
{
public:
    ThumbButton(int a1, int a2, int a3, int a4) : Button(a1, a2, a3, a4, MODAL_RESULT_0) {}
    virtual void HandleEvent(GEVENT*);
};

class ScrollButton : public BitButton
{
public:
    ScrollButton(int a1, int a2, int a3, int a4, DICTNODE* _bitmap, void (*a6)(Widget*)) :
        BitButton(a1, a2, a3, a4, _bitmap, a6) {}
    virtual void HandleEvent(GEVENT*);
};

class ScrollBar : public Container
{
public:
    ScrollBar(int, int, int, int, int, int);
    int at52;
    int at56;
    int at5a;
    ScrollButton* at5e;
    ScrollButton* at62;
    byte __f_at66[4];
    ThumbButton* at6a;
    int at6e;
    void ScrollRelative(int);
    virtual void Paint(int, int, char);
};


BOOL IsBlinkOn();

void SetBlinkOn();

void UpdateBlinkClock(int);

int GetNumberBox(char *, int, int);
MODAL_RESULT ShowModal(Container* container);

#endif
