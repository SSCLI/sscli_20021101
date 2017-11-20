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
csc /debug intlresource.cs
clix.exe intlresource
csc /debug /res:genstrings.resources /d:DEBUG /t:library genstrings.cs
csc /debug /r:genstrings.dll a.cs
clix.exe a
