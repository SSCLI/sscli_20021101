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
if not exist obj%BUILD_ALT_DIR% goto NoObjExist
if "%1"=="clean" rmdir /s /q obj%BUILD_ALT_DIR%
if "%1"=="-c" rmdir /s /q obj%BUILD_ALT_DIR%
:NoObjExist
if not exist obj%BUILD_ALT_DIR% md obj%BUILD_ALT_DIR%
if not exist obj%BUILD_ALT_DIR%\rotor_x86 md obj%BUILD_ALT_DIR%\rotor_x86
type default.mac >obj%BUILD_ALT_DIR%\_objects.mac
if ERRORLEVEL 1 goto ReportError
nmake ROTOR_X86=1 PASS0ONLY=1 >build%BUILD_ALT_DIR%.log 2>&1
if ERRORLEVEL 1 goto ReportError
nmake ROTOR_X86=1 MAKEDLL=1 >>build%BUILD_ALT_DIR%.log 2>&1
if ERRORLEVEL 1 goto ReportError

rem Copy the C runtime DLL to targetcomplus
if exist %windir%\system32\msvcr71d.dll goto Use71
set MSVCRT_DLL_NAME=msvcr70d.dll
if %DDKBUILDENV%==free set MSVCRT_DLL_NAME=msvcr70.dll
goto CopyCRuntime

:Use71
set MSVCRT_DLL_NAME=msvcr71d.dll
if %DDKBUILDENV%==free set MSVCRT_DLL_NAME=msvcr71.dll

:CopyCRuntime
xcopy /d /y %windir%\system32\%MSVCRT_DLL_NAME% %targetcomplus% >>build%BUILD_ALT_DIR%.log 2>&1
if ERRORLEVEL 1 goto ReportError

echo Build successful.
endlocal
goto :EOF

:ReportError
if '%BUILDALL_BUILDING%'=='' (
    echo.
    for %%i in (build%BUILD_ALT_DIR%.log) do echo Build failed.  Please see %%~fi for details.
)
exit /B 1


