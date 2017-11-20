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

echo CLR environment (%~f0)
REM
REM RotorEnv.Cmd
REM
REM     Driver to set checked, fastchecked or free CLR build environment
REM for use with the NT build tools.
REM

REM Clear out variables to make sure we don't conflict
set LIB=
set INCLUDE=
set _NTROOT=
set _NTTREE=
set _NTDRIVE=
set DDKBUILDENV=
set BUILD_OPTIONS=
set BUILD_ALT_DIR=
set BUILD_DEFAULT_TARGETS=
set C_DEFINES=
set DEBUGGING_SUPPORTED_BUILD=
set PROFILING_SUPPORTED_BUILD=

REM ------------------------------------------
REM Figure out what the root of the world is.
REM This is computed as the parent of the
REM directory in which we are found.
REM ------------------------------------------

call :ComputeCorBase    %~f0
if "%CORBASE%" NEQ "%RESULT%" if "%CORBASE%" NEQ "" echo Changing CORBASE from %CORBASE% to %RESULT%
set CORBASE=%RESULT%

REM ------------------------------------------
REM If the environment isn't somewhere else,
REM ------------------------------------------

if not "%CORENV%" == "" goto CorEnvSet
call :ComputeCorEnv    %~f0
set CORENV=%RESULT%
:CorEnvSet

REM --------------------------------------------
REM Allow per machine customization (pre loop)
REM --------------------------------------------

REM ------------------------------------------
REM See what options were specified.
REM ------------------------------------------
set _TGTOS=NT32
set _TGTCPUTYPE=x86
set _TGTCPU=i386

if not defined _ENV set _ENV=ConfigFastChecked
for %%i in (%*) do call :Parse %%i

set COMPlus_BuildFlavor=wks
set SVR_WKS_DIRS=wks

REM ------------------------------------------
REM BUILD.EXE insists on having some particular
REM environment variables pointing to our root
REM ------------------------------------------

Call :DrivePart         %CORBASE%
Set _NTDRIVE=%RESULT%

Call :AfterDrivePart    %CORBASE%
Set _NTROOT=%RESULT%

echo Building for Operating System - %_TGTOS%
echo              Processor Family - %_TGTCPUTYPE%
echo                     Processor - %_TGTCPU%
echo             Build Environment = %CORENV%

goto %_ENV%

REM Any spaces before the ampersand are part of the environment value.
:Parse
if /I "%1" == "free"        set _ENV=ConfigFree&goto :EOF
if /I "%1" == "checked"     set _ENV=ConfigChecked&goto :EOF
if /I "%1" == "fastchecked" set _ENV=ConfigFastChecked&goto :EOF

if /I "%1" == "nobrowse" (
   set BROWSER_INFO=
   set NO_BROWSER_INFO=1
   set NO_BROWSER_FILE=1
   goto :EOF
)

set _ENV=usage
goto :EOF

REM ---------------------
REM Fast Checked Build
REM ---------------------
:ConfigFastChecked
set DUMP_CRT_LEAKS=
set DDKBUILDLIB=checked
set DDKBUILDENV=fastchecked
set URTBUILDENV=fstchk
call :SetDebug %DEBUG_TYPE%
set C_DEFINES=-DNTMAKEENV -D_DEBUG
set NTDBGFILES=
set FPO_OPT=1
set MSC_OPTIMIZATION=/O1 
set BUILD_ALT_DIR=df
if "%NO_BROWSER_INFO%" NEQ "1" set BROWSER_INFO=1
set DEBUG_CRTS=1
if "%COLOR_FASTCHECKED%" NEQ "" color %COLOR_FASTCHECKED%
goto ConfigEnd


REM ---------------------
REM Free Build
REM ---------------------
:ConfigFree
set DDKBUILDLIB=free
set DDKBUILDENV=free
set URTBUILDENV=fre
set C_DEFINES=-DNTMAKEENV -DNDEBUG -DPERF_TRACKING

REM These settings give us PDB files in release build
set NTDBGFILES=
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=windbg

set MSC_OPTIMIZATION=/O1
set BUILD_ALT_DIR=
set BROWSER_INFO=
set DEBUG_CRTS=
if "%COLOR_FREE%" NEQ "" color %COLOR_FREE%

goto ConfigEnd


REM ---------------------
REM Checked Build
REM ---------------------
:ConfigChecked
set DUMP_CRT_LEAKS=
set DDKBUILDLIB=checked
set DDKBUILDENV=checked
set URTBUILDENV=chk
set C_DEFINES=-DNTMAKEENV -D_DEBUG
call :SetDebug %DEBUG_TYPE%
set FPO_OPT=
set NTDBGFILES=
set MSC_OPTIMIZATION=/Od /Oi /GZ
set BUILD_ALT_DIR=d
if "%NO_BROWSER_INFO%" NEQ "1" set BROWSER_INFO=1
set DEBUG_CRTS=1
if "%COLOR_CHECKED%" NEQ "" color %COLOR_CHECKED%
goto ConfigEnd


:ConfigEnd

REM ------------------------------------------
REM Set Common Build & Environment options
REM ------------------------------------------

set BUILD_DEFAULT=-mwe -nmake -i -a

call :ComputeNtMakeEnv   %~f0
set NTMAKEENV=%RESULT%

if "%tmp%" EQU "" set tmp=\
set COPYCMD=/Y

call :SetTitle %CORBASE%

set MSC_OPTIMIZATION=%MSC_OPTIMIZATION% /G6

REM --------------------------------------------
REM  Set values of URTTARGET variables.
REM --------------------------------------------

if not "%URTTARGET%" == "" goto URTTargetSet
if not "%_URTTARGET%" == "" goto URTTargetSet
set _URTTARGET=%windir%\complus
:URTTargetSet
REM Set derived URT variables.
if not "%_URTTARGET%" == "" set URTTARGET=%_URTTARGET%\v1.%_TGTCPUTYPE%%URTBUILDENV%

REM ------------------------------------------
REM Set up our include paths. These environment variables are the four that are specially
REM understood by BUILD.EXE.
REM
REM Set up the lib paths too.
REM
REM Finally, set the version file and the file used for specifying the base address
REM of each of our various DLLs.
REM ------------------------------------------

set NTDEBUGTYPE=vc6

set FUSION_LIB_PATH=%CORBASE%\bin\*\%DDKBUILDENV%

set COFFBASE_TXT_FILE=%CORBASE%\bin\*\%DDKBUILDENV%\coffbase.txt


REM ------------------------------------------
:SetEnvI386
REM ------------------------------------------

set TARGETCOMPLUS=%URTTARGET%.rotor

set TARGETCOMPLUSSDK=%TARGETCOMPLUS%\sdk

if not exist %TARGETCOMPLUS% mkdir %TARGETCOMPLUS%
if not exist %TARGETCOMPLUSSDK% mkdir %TARGETCOMPLUSSDK%

REM ----> turn on debugging support
set DEBUGGING_SUPPORTED_BUILD=1
set C_DEFINES=%C_DEFINES% -DDEBUGGING_SUPPORTED
REM <----

REM ----> turn on profiling support
set PROFILING_SUPPORTED_BUILD=1
set C_DEFINES=%C_DEFINES% -DPROFILING_SUPPORTED
REM <----

REM ----> general I386 variables.  These MUST be set after the above variables
REM set BATCH_NMAKE=1
set C_DEFINES=%C_DEFINES% -DFUSION_SUPPORTED -DPLATFORM_WIN32
REM <----

if "%PREVIOUSPATHSET%" NEQ ""  goto SetEnvI386PreviousDone
set PREVIOUSCORPATH=%PATH%
set PREVIOUSPATHSET=1
:SetEnvI386PreviousDone
rem ... ;%CORBASE%\bin\i386\cor ... omitted
set PATH=%CORBASE%\bin;%CORENV%\bin;%PREVIOUSCORPATH%

set FEATURE_PAL=1
set C_DEFINES=%C_DEFINES% -DFEATURE_PAL -DUSE_CDECL_HELPERS -DPAL_PORTABLE_SEH
set CSC_COMPILE_FLAGS=%CSC_COMPILE_FLAGS% /d:FEATURE_PAL

set SDK_INC_PATH=%ROTOR_DIR%\pal
set CRT_INC_PATH=%ROTOR_DIR%\palrt\inc

set BUILD_DEFAULT_TARGETS=%BUILD_DEFAULT_TARGETS% -dynamic rotor_x86
set BUILD_DEFAULT=%BUILD_DEFAULT:-m=-i%

set URTSDKTARGET=%TARGETCOMPLUS%\sdk
set INTTOOLSTARGET=%TARGETCOMPLUS%\int_tools

REM Make sure MSVCDIR is using the short names
Call :ShortName "%MSVCDIR%"
set MSVCDIR=%RESULT%
if "%MSVCDIR%"=="" goto MSVCDirNotSet

set SDK_LIB_PATH=%MSVCDIR%\PlatformSDK\lib
set CRT_LIB_PATH=%MSVCDIR%\lib
set SDK_INC_PATH=%SDK_INC_PATH%;

rem Make sure the PAL and PALRT are on the path
set PATH=%TARGETCOMPLUS%;%TARGETCOMPLUS%\sdk\bin;%TARGETCOMPLUS%\int_tools;%PATH%

REM --------------------------------------------
REM Allow perl scripts to be run as commands (for vssGet vssCheckOut ...)
REM --------------------------------------------
(ASSOC .PL=Perl.Script) > NUL:
(FTYPE Perl.Script=perl.exe "%%1" %%*) > NUL:

REM --------------------------------------------
REM add TARGETCOMPLUS to _NT_SYMBOL_PATH so that NTSD works

if "%PREVIOUS_NT_SYMBOL_PATH_SET%" NEQ ""  goto _NT_SYMBOL_PATH_SET
set PREVIOUS_NT_SYMBOL_PATH_SET=1
set PREVIOUS_NT_SYMBOL_PATH=%_NT_SYMBOL_PATH%
:_NT_SYMBOL_PATH_SET

set _NT_SYMBOL_PATH=%TARGETCOMPLUS%\Symbols;%TARGETCOMPLUS%;%PREVIOUS_NT_SYMBOL_PATH%

REM --------------------------------------------
REM add TARGETCOMPLUS to _NT_DEBUGGER_EXTENSION_PATH so that NTSD works

if "%PREVIOUS_NT_DEBUGGER_EXTENSION_PATH_SET%" NEQ ""  goto _NT_DEBUGGER_EXTENSION_PATH_SET
set PREVIOUS_NT_DEBUGGER_EXTENSION_PATH_SET=1
set PREVIOUS_NT_DEBUGGER_EXTENSION_PATH=%_NT_DEBUGGER_EXTENSION_PATH%
:_NT_DEBUGGER_EXTENSION_PATH_SET

set _NT_DEBUGGER_EXTENSION_PATH=%TARGETCOMPLUS%\int_tools;%PREVIOUS_NT_DEBUGGER_EXTENSION_PATH%


REM ------------------------------------------
REM All done
REM ------------------------------------------
set RESULT=

goto Done


REM ============================================================================
REM Subroutines
REM

REM ------------------------------------------
REM Extract the drive letter from a path
REM ------------------------------------------
:DrivePart
set RESULT=%~d1
goto :EOF

REM ------------------------------------------
REM Extract everything after the driver letter
REM from a path
REM ------------------------------------------
:AfterDrivePart
set RESULT=%~pnx1
goto :EOF

REM ------------------------------------------
REM Expand a string to a full path
REM ------------------------------------------
:FullPath
set RESULT=%~f1
goto :EOF

REM ------------------------------------------
REM Expand a string to a full path using a Short Name
REM ------------------------------------------
:ShortName
set RESULT=%~sf1
goto :EOF

REM ------------------------------------------
REM Compute the CORBASE environment variable
REM given a path to this batch script.
REM ------------------------------------------
:ComputeCorBase
set RESULT=%~dp1..
Call :FullPath %RESULT%
goto :EOF

REM ------------------------------------------
REM Compute the CORENV environment variable
REM given a path to this batch script.
REM If we're building rotor assume it's ..\..\rotorenv
REM ------------------------------------------
:ComputeCorEnv
if "%ROTOR%"=="1" goto SetRotorCorEnv
set RESULT=%~dp1..
Call :FullPath %RESULT%
goto :EOF
:SetRotorCorEnv
set RESULT=%~dp1..\..\rotorenv
Call :FullPath %RESULT%
goto :EOF


REM ------------------------------------------
REM Compute the NTMAKEENV environment variable
REM given a path to this batch script.
REM ------------------------------------------
:ComputeNTMakeEnv
if defined FEATURE_PAL_VC7 (
    set RESULT=%CORBASE%\..\rotorenv\bin
) else (
    set RESULT=%CORENV%\bin
)
Call :FullPath %RESULT%
goto :EOF

REM ------------------------------------------
REM Configure the debug file type output.
REM ------------------------------------------
:SetDebug
if "%1" == "PDB_ONLY" goto PdbOnly
set NTDEBUG=ntsd
set NTDEBUGTYPE=both
set USE_PDB=1
goto DebugExitTag
:PdbOnly
set NTDEBUG=ntsd
set NTDEBUGTYPE=windbg
:DebugExitTag
goto :EOF

REM ============================================================================
REM Error handling routines
REM

:SetTitle
setlocal
set _Execute=%COMPlus_BuildFlavor%

title CLR %t% %DDKBUILDENV% - Build - %ROTOR_DIR%
echo                    Build Type - %DDKBUILDENV%
endlocal
goto :EOF


:MSVCDirNotSet
echo.
echo *** Error *** 
echo MSVCDIR not set, make sure 8.3 support is enabled and VC is installed correctly
echo  
exit /B 1

:usage
echo.
echo usage: CorEnv [free^|^checked^|^fastchecked^]
echo               [nobrowse]
echo.
echo  If there is an environment variable named "COLOR_FREE", "CorEnv free"
echo    will run "color %%COLOR_FREE%%" to set screen colors.
echo    Likewise, "COLOR_CHECKED"
echo    correspond to the other build environments.
echo.
echo   Example:  CorEnv free          set free environment and build for NT32,x86,i386
echo   Example:  set COLOR_CHECKED=70     use black on gray for checked env.
echo             CorEnv                   defaults to fastchecked environment
echo.
echo   "color -?" for usage and color codes.
echo.

REM ============================================================================

:Done
exit /B 0
