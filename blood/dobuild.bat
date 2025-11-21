@echo off
set MSG=Please select what to build:
call REVSEL.BAT

if "%CHOICE%" == "" goto end

mkdir %CHOICE%
mkdir %CHOICE%\obj
set WATINCBAK=%INCLUDE%
set INCLUDE=%INCLUDE%;..\qtools\include;..\helix32;..\audiolib;..\mact;..\smacker;..\ten\incl
wmake.exe "appver_qtools = %QTCHOICE%" "appver_exedef = %CHOICE%"
set CHOICE=
set QTCHOICE=
set INCLUDE=%WATINCBAK%
set WATINCBAK=

:end
set CHOICE=
set QTCHOICE=
