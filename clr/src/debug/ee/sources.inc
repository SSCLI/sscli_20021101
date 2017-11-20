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

!include $(NTMAKEENV)\sources.cor

INCLUDES        =$(INCLUDES);.;..;..\..\..\vm;..\..\..\vm\$(_TGTCPU);..\..\..\CeeGen\inc;..\..\inc;..\..\..\inc

C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE -DJIT_OR_NATIVE_SUPPORTED=1 $(USER_SPECIFIC_C_DEFINES)

!if "$(PLATFORM_UNIX)" == "1"
# This warns about behavior in fcall.h that's intended.
COMPILER_WARNINGS   =$(COMPILER_WARNINGS) -Wno-ctor-dtor-privacy
!endif

# Don't turn this on!
#USE_RTTI        = FALSE

MINORCOMP       = cor

TARGETNAME      =$(CORBUILDENV)cordbee_$(_CORFLAVOR)
TARGETPATH      =$(TARGETCORLIB)
TARGETTYPE      =LIBRARY

### CCover, for some reason, has a problem instrumenting 
### source in the precompiled headers for this directory.
PRECOMPILED_INCLUDE     = ..\stdafx.h
PRECOMPILED_CXX         = 1
PRECOMPILED_SOURCEFILE  = ..\stdafx.cpp

#
# BUILD_DEBUGGER is defined in sources.cor
#
#BUILD_DEBUGGER=yes
#!if "$(BUILD_DEBUGGER)"=="yes"
SOURCES=                \
    ..\Debugger.cpp        \
    ..\RCThread.cpp        \
    ..\Thread.cpp          \
    ..\controller.cpp      \
    ..\frameinfo.cpp       \
    ..\LsCommon.cpp        \
    ..\LsProcess.cpp       \
    ..\LsHash.cpp          \
    ..\LsModule.cpp        \
    ..\LsThread.cpp        \
    ..\LsDiValue.cpp


ROTOR_X86_SOURCES=..\rotor_x86\dbghelpers.asm \
                  ..\rotor_x86\x86walker.cpp

PPC_SOURCES=..\ppc\dbghelpers.s \
            ..\ppc\ppcwalker.cpp

SPARC_SOURCES=..\sparc\dbghelpers.s

#!else
#SOURCES=
#!endif
