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

SYNCHRONIZE_DRAIN=1
SYNCHRONIZE_BLOCK=1

NO_CRT                                  = 1
!include $(NTMAKEENV)\sources.cor

USE_NOLIBS                              = 1
NO_NTDLL                                = 1

INCLUDES                                = $(INCLUDES);.;..;..\..\inc;..\..\..\inc;

DLLENTRY                                = DllMain
MINORCOMP                               = sscoree
TARGETPRIORPATH                         = $(CORBUILDENV)
TARGETLIBNAME                           = sscoree
COFFBASE                                = sscoree
!if "$(PLATFORM_UNIX)" != "1"
NTTARGETFILES                           = $(TARGETCORBIN)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoree.lib
!endif
TARGETNAME                              = $(TARGETPRIORPATH)$(TARGETLIBNAME)

TARGETPATH                              = $(TARGETCORBIN)
TARGETTYPE                              = DYNLINK
UMTYPE                                  = windows
CORFLIBS                                =

COR_C_FLAGS                             = $(COR_C_FLAGS) -DUNICODE -D_UNICODE
CDEFINES                                = $(CDEFINES) $(COR_C_FLAGS)

EXCEPTION_HANDLING                      = $(SEHONLY)
PRECOMPILED_INCLUDE                     = ..\stdafx.h
PRECOMPILED_CXX                         = 1
PRECOMPILED_SOURCEFILE                  = ..\stdafx.cpp

!if "$(PLATFORM_UNIX)" != "1"
DLLDEF                          = ..\sscoree.RotorDef
!endif

CORTHUNKLIBS    =

LINKLIBS = \
                $(CORLIBS)\format.lib                           \
                $(CORLIBS)\cee_$(_CORFLAVOR).lib                \
                $(CORLIBS)\ipcmanager.lib                       \
                $(CORLIBS)\mdcompiler.lib                       \
                $(CORLIBS)\mdruntime.lib                        \
                $(CORLIBS)\mdruntimerw.lib                      \
                $(CORLIBS)\utilcode.lib                         \
!IF "$(PROFILING_SUPPORTED_BUILD)"=="1"
                $(CORLIBS)\perfcounters.lib \
!ENDIF
!IF "$(FEATURE_PAL)"!="1" && "$(_TGTOS)"!="NT64"
                $(CORLIBS)\minidump.lib \
!ENDIF
!if "$(PLATFORM_UNIX)" != "1"
                $(CORLIBS)\mscorsn.lib \
!endif
                $(CORLIBS)\shim.lib \
                $(CORLIBS)\xmlparser.lib \

SOURCELIBS = $(LINKLIBS)

TARGETLIBS =    $(TARGETLIBS)                                   \
                $(CORLIBS)\corguids.lib                         \
                $(CORLIBS)\comfloat_$(_CORFLAVOR).lib           \
                $(CORLIBS)\comnls_$(_CORFLAVOR).lib             \
                $(CORLIBS)\ceefgen.lib                          \
!if "$(PLATFORM_UNIX)" != "1"
                $(CORLIBS)\mscorpe.lib                          \
                $(FUSION_LIB_PATH)\fusion.lib                   \
!endif
!IF "$(DEBUGGING_SUPPORTED_BUILD)"=="1"
                $(CORLIBS)\cordbee_$(_CORFLAVOR).lib            \
!ENDIF

!if "$(PLATFORM_UNIX)" != "1"
WIN32DLL_LIBS=                         \
        $(CORLIBS)\rotor_pal.lib       \
        $(CORLIBS)\rotor_palrt.lib
!endif

UNIX_DLL_LIBS= -lrotor_pal -lrotor_palrt -lmscorpe -lfusion -lmscorsn
USE_CC_LIB = 1


SOURCES=                        \
        ..\MsCorver.rc          \
        ..\MSCoree.cpp

