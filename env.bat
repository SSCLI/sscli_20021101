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

REM Check to to see if MSVCDir is set
IF "%MSVCDir%"=="" for /f "usebackq delims=," %%i in ('%VS71COMNTOOLS%') do call :TRYSETMSVC "%%~i"
IF "%MSVCDir%"=="" for /f "usebackq delims=," %%i in ('%VSCOMNTOOLS%') do call :TRYSETMSVC "%%~i"
rem IF "%MSVCDir%"=="" call :TRYSETMSVC "C:\Program Files\Microsoft Visual Studio .Net 2003\Common7\Tools\"
rem IF "%MSVCDir%"=="" call :TRYSETMSVC "C:\Program Files\Microsoft Visual Studio .Net\Common7\Tools\"
IF "%MSVCDir%"=="" goto MSVCDirNotSet

REM Try and determine if Perl is installed.
perl -v >NUL: 2>NUL:
if ERRORLEVEL 1 goto NoPerlInstalled

REM Try and setup the PERLLIB environment variable
if "%PERLLIB%"=="" for %%i in (perl.exe) do set PERLLIB=%%~dp$PATH:i..\lib
if "%PERLLIB%"=="" goto PerlLibDirNotSet
if "%PERLLIB%"=="..\lib" goto PerlLibDirNotSet

REM Expand the PERLLIB environment variable
Call :FullPath %PERLLIB%
set PERLLIB=%RESULT%
if "%PERLLIB%"=="" goto PerlLibDirNotSet

REM Figure out the path to the current directory
call :ComputeBase %~f0
set ROTOR_DIR=%RESULT%

if not "%ROTOR_DIR%"=="%ROTOR_DIR: =%" goto SpaceInPath

REM Verify that there's no high characters in the path
perl -e "exit 1 if '%ROTOR_DIR%' =~ /[\x80-\xff]/;"
if ERRORLEVEL 1 goto NoHighBitCharacters

REM Setup the destination directory for the build if one isn't set already
if "%_URTTARGET%"=="" set _URTTARGET=%ROTOR_DIR%\build

if "%DNAROOT%"=="" set dnaroot=%ROTOR_DIR%\fx

REM Setup Rotor Enviroment variables used by rotorenv
set ROTOR=1
set FEATURE_PAL_VC7=1
set CSC_NO_DOC=1

REM Call corenv to setup the cor build environment
if "%1"=="" (
   call %ROTOR_DIR%\clr\bin\rotorenv fastchecked
) else (
   call %ROTOR_DIR%\clr\bin\rotorenv  %*
)
if ERRORLEVEL 1 goto DisplayError

goto Done

REM ------------------------------------------
REM Expand a string to a full path
REM ------------------------------------------
:FullPath
set RESULT=%~f1
goto :EOF

REM ------------------------------------------
REM Compute the current directory
REM given a path to this batch script.
REM ------------------------------------------
:ComputeBase
set RESULT=%~dp1
REM Remove the trailing \
set RESULT=%RESULT:~0,-1%
Call :FullPath %RESULT%
goto :EOF

REM ------------------------------------------
REM Check if the file vsvars32.bat exists at
REM the specified path and if it does, call it.
REM The supplied path may or may not be wrapped
REM in "", so we unconditionally strip them then
REM add them under our own control.
REM ------------------------------------------
:TRYSETMSVC
IF NOT "%~1" == "" (
  IF EXIST "%~1\vsvars32.bat" call "%~1\vsvars32.bat"
)
goto :EOF

:MSVCDirNotSet
echo.
echo *** Error *** 
echo MSVCDir not defined. VC++ environment variables must be set before executing.
echo.
exit /B 1

:NoPerlInstalled
echo.
echo *** Error *** 
echo Perl does not appear to be installed. Perl.exe failed to execute.
echo Please ensure Perl is installed and can be found in the PATH.
exit /B 1

:PerlLibDirNotSet
echo.
echo *** Error *** 
echo PERLLIB not defined. Make sure you have Perl correctly installed and configured.
echo.
exit /B 1

:SpaceInPath
echo.
echo *** Error *** 
echo You cannot have spaces in the path to the SSCLI directory.
echo.
exit /B 1

:NoHighBitCharacters
echo.
echo *** Error *** 
echo You cannot have high-bit characters in the path to the SSCLI directory.
echo.
exit /B 1

:DisplayError
echo.
echo *** Error while running env.bat.
exit /B 1

:Done
exit /B 0
