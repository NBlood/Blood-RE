@echo off
set MSG=Please select what to build:
call EDREVSEL.BAT

if "%CHOICE%" == "" goto end

mkdir %CHOICE%
mkdir %CHOICE%\obj
set WATINCBAK=%INCLUDE%
set INCLUDE=%INCLUDE%;..\qtools\include;..\helix32;..\audiolib;..\mact;..\smacker;..\ten\incl
wmake.exe %CHOICE%\mapedit.exe "appver_qtools = %QTCHOICE%" "appver_exedef = %CHOICE%" "appver_buildobj = %BLDCHOICE%" "MAPEDIT=1"
set CHOICE=
set QTCHOICE=
set BLDCHOICE=
set INCLUDE=%WATINCBAK%
set WATINCBAK=

:end
set CHOICE=
set QTCHOICE=
set BLDCHOICE=
