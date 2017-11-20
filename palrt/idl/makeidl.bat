@if "%_echo%"=="" echo off
REM ==++==
REM 
REM 
REM  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
REM 
REM  The use and distribution terms for this software are contained in the file
REM  named license.txt, which can be found in the root of this distribution.
REM  By using this software in any fashion, you are agreeing to be bound by the
REM  terms of this license.
REM 
REM  You must not remove this notice, or any other, from this software.
REM 
REM 
REM ==--==
setlocal

if "%1" == "" goto LUsage
for %%i in (-h, -help, -?, /?) do @if "%1" EQU "%%i" goto :LUsage

if "%CORBASE%" == "" goto LUsage
if "%CORENV%" == "" goto LUsage

rem Generic flags
set MIDL_FLAGS=%MIDL_FLAGS% -char unsigned -ms_ext -c_ext -error all 

rem Include path
set MIDL_FLAGS=%MIDL_FLAGS% -I%CORBASE%\src\inc 

if     "%FEATURE_PAL_VC7%" == "1" set MIDL_FLAGS=%MIDL_FLAGS% -I%MSVCDIR%\PlatformSDK\include
if not "%FEATURE_PAL_VC7%" == "1" set MIDL_FLAGS=%MIDL_FLAGS% -I%CORENV%\crt\inc\i386

rem Don't generate garbage
set MIDL_FLAGS=%MIDL_FLAGS% -notlb -client none -server none -proxy nul -dlldata nul

if "%1" == "*" goto ProcessAll

for %%F in (%*) do call :ProcessFile %%F
goto LExit

:ProcessAll
for /f "eol=;" %%F in (files.lst) do call :ProcessFile %%F
goto LExit

:ProcessFile
del /F %~n1_i.c_ %~n1.h_ %~n1.h %~n1_i.c 
midl %MIDL_FLAGS% -header %~n1.h -iid %~n1_i.c %CORBASE%\%1
ren %~n1.h %~n1.h_
perl idlstrip.pl <%~n1.h_ >%~n1.h
ren %~n1_i.c %~n1_i.c_
perl idlstrip.pl <%~n1_i.c_ >%~n1_i.c
del %~n1_i.c_ %~n1.h_ 
goto LExit

:LUsage

echo.
echo makeidl.bat [files]
echo.
echo    Builds MIDL-generated CLR headers for PALRT
echo.
echo    Use * as parameter to process all files.
echo.
echo    Should be run inside CLR build environment (corenv), depends on %%CORBASE%% 
echo    and %%CORENV%% environment variables.
echo.

:LExit

