#
# 
#  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
# 
#  The use and distribution terms for this software are contained in the file
#  named license.txt, which can be found in the root of this distribution.
#  By using this software in any fashion, you are agreeing to be bound by the
#  terms of this license.
# 
#  You must not remove this notice, or any other, from this software.
# 
#

NO_CRT                  =1
!include $(NTMAKEENV)\sources.cor

INCLUDES        = $(INCLUDES);.;..\inc;..\..\inc;

MINORCOMP       =utilcode

PRECOMPILED_INCLUDE     = stdafx.h
PRECOMPILED_CXX         = 1
PRECOMPILED_SOURCEFILE  = stdafx.cpp

TARGETNAME      =$(CORBUILDENV)utilcode$(_CRTFLAVOR)
TARGETPATH      =$(TARGETCORLIB)
!if "$(PLATFORM_UNIX)" == "1"
TARGETTYPE      =ARCHIVE
!endif

!if "$(PLATFORM_UNIX)" != "1"
TARGETTYPE      =LIBRARY
!endif

PRECOMPILED_INCLUDE     = ..\stdafx.h
PRECOMPILED_CXX         = 1
PRECOMPILED_SOURCEFILE  = ..\stdafx.cpp
PRECOMPILED_Obj         = stdafx_utilcode.obj

COR_C_FLAGS     = -DUNICODE -D_UNICODE $(_CRTIMPFLAGS)

COR_C_FLAGS       = $(COR_C_FLAGS) -DENABLE_PERF_COUNTERS

CDEFINES                = $(CDEFINES) $(COR_C_FLAGS)

SOURCES=                    \
        ..\arraylist.cpp           \
        ..\FusionBind.cpp          \
        ..\FusionSink.cpp          \
        ..\CompletePath.cpp        \
        ..\CorImage.cpp            \
        ..\Debug.cpp               \
        ..\GuidFromName.cpp        \
        ..\jitperf.cpp             \
        ..\md5.cpp                 \
        ..\regutil.cpp             \
        ..\format1.cpp             \
        ..\sighelper.cpp           \
        ..\util.cpp                \
        ..\UTSem.cpp               \
        ..\PostError.cpp           \
        ..\PrettyPrintSig.cpp      \
        ..\StgPool.cpp             \
        ..\StgPooli.cpp            \
        ..\StgPoolReadOnly.cpp     \
        ..\splitpat.cpp            \
        ..\timeline.cpp            \
        ..\InternalDebug.cpp       \
        ..\makepath.cpp            \
        ..\log.cpp                 \
        ..\loaderheap.cpp          \
        ..\winfix.cpp              \
        ..\DbgAlloc.cpp            \
        ..\wsperf.cpp              \
        ..\PerfLog.cpp             \
        ..\NamespaceUtil.cpp       \
        ..\OutString.cpp           \
        ..\ilFormatter.cpp         \
        ..\OpInfo.cpp              \
        ..\wsinfo.cpp              \

