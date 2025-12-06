@echo off
if "%MSG%" == "" goto error
set CHOICE=
cls
echo [1] MAPEDIT 1.10
echo [2] MAPEDIT 1.11 (19 August 1997)
echo [3] MAPEDIT 1.11 (23 September 1997)
echo [4] MAPEDIT 1.20
echo [5] MAPEDIT Plasma Pak 1.10
echo [6] MAPEDIT Plasma Pak 1.11 (19 August 1997)
echo [7] MAPEDIT Plasma Pak 1.11 (23 September 1997)
echo [8] MAPEDIT Plasma Pak 1.20
echo [9] MAPEDIT Plasma Pak 1.21 (One Unit Whole Blood)
echo.
echo [0] Cancel and quit
echo.
echo * Watcom 10.6 should be used for a more accurate code generation,
echo along with TASM 3.1.
echo.
echo %MSG%
set MSG=
choice /S /C:1234567890 /N
echo.

if ERRORLEVEL 10 goto end
if ERRORLEVEL 9 goto MP121
if ERRORLEVEL 8 goto MP120
if ERRORLEVEL 7 goto MP111A
if ERRORLEVEL 6 goto MP111
if ERRORLEVEL 5 goto MP110
if ERRORLEVEL 4 goto MR120
if ERRORLEVEL 3 goto MR111A
if ERRORLEVEL 2 goto MR111
if ERRORLEVEL 1 goto MR110

:MR110
set CHOICE=MR110
set QTCHOICE=QT110
set BLDCHOICE=BUILDOLD
goto end
:MR111
set CHOICE=MR111
set QTCHOICE=QT110
set BLDCHOICE=BUILDOLD
goto end
:MR111A
set CHOICE=MR111A
set QTCHOICE=QT111A
set BLDCHOICE=BUILDOLD
goto end
:MR120
set CHOICE=MR120
set QTCHOICE=QT111A
set BLDCHOICE=BUILD
goto end
:MP110
set CHOICE=MP110
set QTCHOICE=QT110
set BLDCHOICE=BUILDOLD
goto end
:MP111
set CHOICE=MP111
set QTCHOICE=QT110
set BLDCHOICE=BUILDOLD
goto end
:MP111A
set CHOICE=MP111A
set QTCHOICE=QT111A
set BLDCHOICE=BUILDOLD
goto end
:MP120
set CHOICE=MP120
set QTCHOICE=QT111A
set BLDCHOICE=BUILD
goto end
:MP121
set CHOICE=MP121
set QTCHOICE=QT111A
set BLDCHOICE=BUILD
goto end

:error
echo This script shouldn't be run independently

:end