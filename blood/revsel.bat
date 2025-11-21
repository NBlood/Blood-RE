@echo off
if "%MSG%" == "" goto error
set CHOICE=
cls
echo [1] Shareware  1.10
echo [2] Shareware  1.11 (19 August 1997)
echo [3] Shareware  1.11 (23 September 1997)
echo [4] SW Retail  1.10
echo [5] SW Retail  1.11 (19 August 1997)
echo [6] SW Retail  1.11 (23 September 1997)
echo [7] Registered 1.10
echo [8] Registered 1.11 (19 August 1997)
echo [9] Registered 1.11 (23 September 1997)
echo [A] Registered 1.20
echo [B] Registered 1.20 (3DFX)
echo [C] Plasma Pak 1.10
echo [D] Plasma Pak 1.11 (19 August 1997)
echo [E] Plasma Pak 1.11 (23 September 1997)
echo [F] Plasma Pak 1.20
echo [G] Plasma Pak 1.20 (3DFX)
echo [H] Plasma Pak 1.21 (One Unit Whole Blood)
echo.
echo [0] Cancel and quit
echo.
echo * Watcom 10.6 should be used for a more accurate code generation,
echo along with TASM 3.1.
echo.
echo %MSG%
set MSG=
choice /S /C:123456789AaBbCcDdEeFfGgHh0 /N
echo.

if ERRORLEVEL 26 goto end
if ERRORLEVEL 24 goto BP121
if ERRORLEVEL 22 goto BP120FX
if ERRORLEVEL 20 goto BP120
if ERRORLEVEL 18 goto BP111A
if ERRORLEVEL 16 goto BP111
if ERRORLEVEL 14 goto BP110
if ERRORLEVEL 12 goto BR120FX
if ERRORLEVEL 10 goto BR120
if ERRORLEVEL 9 goto BR111A
if ERRORLEVEL 8 goto BR111
if ERRORLEVEL 7 goto BR110
if ERRORLEVEL 6 goto BSR111A
if ERRORLEVEL 5 goto BSR111
if ERRORLEVEL 4 goto BSR110
if ERRORLEVEL 3 goto BS111A
if ERRORLEVEL 2 goto BS111
if ERRORLEVEL 1 goto BS110

:BS110
set CHOICE=BS110
set QTCHOICE=QT110
goto end
:BS111
set CHOICE=BS111
set QTCHOICE=QT110
goto end
:BS111A
set CHOICE=BS111A
set QTCHOICE=QT111A
goto end
:BSR110
set CHOICE=BSR110
set QTCHOICE=QT110
goto end
:BSR111
set CHOICE=BSR111
set QTCHOICE=QT110
goto end
:BSR111A
set CHOICE=BSR111A
set QTCHOICE=QT111A
goto end
:BR110
set CHOICE=BR110
set QTCHOICE=QT110
goto end
:BR111
set CHOICE=BR111
set QTCHOICE=QT110
goto end
:BR111A
set CHOICE=BR111A
set QTCHOICE=QT111A
goto end
:BR120
set CHOICE=BR120
set QTCHOICE=QT111A
goto end
:BR120FX
set CHOICE=BR120FX
set QTCHOICE=QT111A
goto end
:BP110
set CHOICE=BP110
set QTCHOICE=QT110
goto end
:BP111
set CHOICE=BP111
set QTCHOICE=QT110
goto end
:BP111A
set CHOICE=BP111A
set QTCHOICE=QT111A
goto end
:BP120
set CHOICE=BP120
set QTCHOICE=QT111A
goto end
:BP120FX
set CHOICE=BP120FX
set QTCHOICE=QT111A
goto end
:BP121
set CHOICE=BP121
set QTCHOICE=QT111A
goto end

:error
echo This script shouldn't be run independently

:end