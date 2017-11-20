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

if "%TARGETCOMPLUS%" == "" goto TargetComplusNotSet

REM Note:
REM The BUILDALL_BUILDING, BUILDALL_ERRORLOG and BUILDALL_WARNINGLOG variables are
REM used by other scripts to report errors so setlocal/endlocal may not be used.
REM

echo.
echo --- Building the PAL ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\pal\win32
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.log
pushd %ROTOR_DIR%\pal\win32
call make.cmd %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building the binplace tool ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\tools\binplace
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.log
pushd %ROTOR_DIR%\tools\binplace
call make.cmd %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building the build tool
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\tools\build
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.log
pushd %ROTOR_DIR%\tools\build
call make.cmd %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building the Resource Compiler ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\tools\resourcecompiler
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %ROTOR_DIR%\tools\resourcecompiler
build %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building the PAL RT ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\palrt\src
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %ROTOR_DIR%\palrt\src
build %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building the CLR ---
echo.
set BUILDALL_BUILDING=%CORBASE%\src
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %CORBASE%\src
build %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building FX ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\fx\src
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %ROTOR_DIR%\fx\src
build %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building ManagedLibraries ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\ManagedLibraries
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %ROTOR_DIR%\ManagedLibraries
build %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building JScript ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\jscript
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %ROTOR_DIR%\jscript
build %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building the pdb to ildb Tool ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\tools\ildbconv
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %ROTOR_DIR%\tools\ildbconv
build %*
popd
if ERRORLEVEL 1 goto DisplayError

echo.
echo --- Building the samples ---
echo.
set BUILDALL_BUILDING=%ROTOR_DIR%\samples
set BUILDALL_ERRORLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.err
set BUILDALL_WARNINGLOG=%BUILDALL_BUILDING%\build%BUILD_ALT_DIR%.wrn
pushd %ROTOR_DIR%\samples
build %*
popd
if ERRORLEVEL 1 goto DisplayError

goto Done

:DisplayError
echo *** Error while building %BUILDALL_BUILDING%
echo     Open %BUILDALL_ERRORLOG% to see the error log.
echo     Open %BUILDALL_WARNINGLOG% to see any warnings.
exit /B 1


:TargetComplusNotSet
echo ERROR: The build environment variable isn't set.
echo Please run env.bat [fastchecked^|free^|checked]
exit /B 1

:Done
set BUILDALL_BUILDING=
set BUILDALL_ERRORLOG=
set BUILDALL_WARNINGLOG=
