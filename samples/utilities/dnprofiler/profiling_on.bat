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

set Cor_Enable_Profiling=0x1
set COR_PROFILER={9AB84088-18E7-42F0-8F8D-E022AE3C4517}
set COR_PROFILER_DLL=%TARGETCOMPLUS%\samples\utilities\dnprofiler\dnprofiler.dll

@REM The following are bit mask values for the DN_PROFILER_MASK environment variable.
@REM 
@REM COR_PRF_MONITOR_FUNCTION_UNLOADS        = 0x1
@REM COR_PRF_MONITOR_CLASS_LOADS             = 0x2
@REM COR_PRF_MONITOR_MODULE_LOADS            = 0x4
@REM COR_PRF_MONITOR_ASSEMBLY_LOADS          = 0x8
@REM COR_PRF_MONITOR_APPDOMAIN_LOADS         = 0x10
@REM COR_PRF_MONITOR_JIT_COMPILATION         = 0x20
@REM COR_PRF_MONITOR_EXCEPTIONS              = 0x40
@REM COR_PRF_MONITOR_GC                      = 0x80
@REM COR_PRF_MONITOR_OBJECT_ALLOCATED        = 0x100
@REM COR_PRF_MONITOR_THREADS                 = 0x200
@REM COR_PRF_MONITOR_REMOTING                = 0x400
@REM COR_PRF_MONITOR_CODE_TRANSITIONS        = 0x800
@REM COR_PRF_MONITOR_ENTERLEAVE              = 0x1000
@REM COR_PRF_MONITOR_CCW                     = 0x2000
@REM COR_PRF_MONITOR_REMOTING_COOKIE         = 0x4000 | COR_PRF_MONITOR_REMOTING
@REM COR_PRF_MONITOR_REMOTING_ASYNC          = 0x8000 | COR_PRF_MONITOR_REMOTING
@REM COR_PRF_MONITOR_SUSPENDS                = 0x10000
@REM COR_PRF_MONITOR_CACHE_SEARCHES          = 0x20000
@REM COR_PRF_MONITOR_CLR_EXCEPTIONS          = 0x1000000
@REM COR_PRF_MONITOR_ALL                     = 0x107ffff
@REM COR_PRF_ENABLE_REJIT                    = 0x40000
@REM COR_PRF_ENABLE_INPROC_DEBUGGING         = 0x80000
@REM COR_PRF_ENABLE_JIT_MAPS                 = 0x100000
@REM COR_PRF_DISABLE_INLINING                = 0x200000
@REM COR_PRF_DISABLE_OPTIMIZATIONS           = 0x400000
@REM COR_PRF_ENABLE_OBJECT_ALLOCATED         = 0x800000
@REM COR_PRF_ALL=0x1ffffff


@REM This is a default setting to demonstrate a large portion of the profiler functionality.
set DN_PROFILER_MASK=0x1A7ffff
