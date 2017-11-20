# ==++==
#
#   Copyright (c) Microsoft Corporation.  All rights reserved.
#
# ==--==


setenv Cor_Enable_Profiling "0x1"
setenv COR_PROFILER "{9AB84088-18E7-42F0-8F8D-E022AE3C4517}"
set LIB_NAME="libdnprofiler.so"
if (`uname -s` == "Darwin") then
    set LIB_NAME="libdnprofiler.dylib"
endif
setenv COR_PROFILER_DLL "${TARGETCOMPLUS}/samples/utilities/dnprofiler/${LIB_NAME}"


# The following are bit mask values for the DN_PROFILER_MASK environment variable.
#
# COR_PRF_MONITOR_FUNCTION_UNLOADS        = 0x1
# COR_PRF_MONITOR_CLASS_LOADS             = 0x2
# COR_PRF_MONITOR_MODULE_LOADS            = 0x4
# COR_PRF_MONITOR_ASSEMBLY_LOADS          = 0x8
# COR_PRF_MONITOR_APPDOMAIN_LOADS         = 0x10
# COR_PRF_MONITOR_JIT_COMPILATION         = 0x20
# COR_PRF_MONITOR_EXCEPTIONS              = 0x40
# COR_PRF_MONITOR_GC                      = 0x80
# COR_PRF_MONITOR_OBJECT_ALLOCATED        = 0x100
# COR_PRF_MONITOR_THREADS                 = 0x200
# COR_PRF_MONITOR_REMOTING                = 0x400
# COR_PRF_MONITOR_CODE_TRANSITIONS        = 0x800
# COR_PRF_MONITOR_ENTERLEAVE              = 0x1000
# COR_PRF_MONITOR_CCW                     = 0x2000
# COR_PRF_MONITOR_REMOTING_COOKIE         = 0x4000 | COR_PRF_MONITOR_REMOTING
# COR_PRF_MONITOR_REMOTING_ASYNC          = 0x8000 | COR_PRF_MONITOR_REMOTING
# COR_PRF_MONITOR_SUSPENDS                = 0x10000
# COR_PRF_MONITOR_CACHE_SEARCHES          = 0x20000
# COR_PRF_MONITOR_CLR_EXCEPTIONS          = 0x1000000
# COR_PRF_MONITOR_ALL                     = 0x107ffff
# COR_PRF_ENABLE_REJIT                    = 0x40000
# COR_PRF_ENABLE_INPROC_DEBUGGING         = 0x80000
# COR_PRF_ENABLE_JIT_MAPS                 = 0x100000
# COR_PRF_DISABLE_INLINING                = 0x200000
# COR_PRF_DISABLE_OPTIMIZATIONS           = 0x400000
# COR_PRF_ENABLE_OBJECT_ALLOCATED         = 0x800000
# COR_PRF_ALL=0x1ffffff


# This is a default setting to demonstrate a large portion of the profiler functionality.
setenv DN_PROFILER_MASK "0x1A7ffff"


