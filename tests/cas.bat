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
if "%1" == "veron"  set ACTION=-q -m -cg 1.1. Everything
if "%1" == "veroff" set ACTION=-q -m -cg 1.1. FullTrust
clix %TARGETCOMPLUS%\caspol.exe %ACTION%
