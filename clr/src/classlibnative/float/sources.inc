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

!if "$(PLATFORM_UNIX)" != "1"
# WarningControl.h disables too many warnings.  classlibnative/float will not use it.
COMPILER_WARNINGS	=/WX
!else
# This warns about behavior in fcall.h that's intended.
COMPILER_WARNINGS   =$(COMPILER_WARNINGS) -Wno-ctor-dtor-privacy
!endif

INCLUDES        = ..;..\..\inc;$(INCLUDES);..\..;..\..\inc;..\..\..\vm;..\..\..\vm\$(_TGTCPU);..\..\..\inc;..\..\..\..\inc;..\..\..\interp;..\..\..\ceegen\inc;

MINORCOMP       =cor

TARGETNAME      =$(DDKBUILDENV)\comfloat_$(_CORFLAVOR)
TARGETPATH      =$(TARGETCORBIN)
TARGETTYPE      =LIBRARY

C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE -DCRTDLL -DUSE_MAPSYM -DJIT_OR_NATIVE_SUPPORTED
!if "$(PLATFORM_UNIX)" != "1"
C_DEFINES       = $(C_DEFINES) /TP /FmCOMFloat.map -W4
!endif

# Get the FPU intrinics for sin, cos, etc.
!if "$(FREEBUILD)"=="1" && "$(PLATFORM_UNIX)" != "1"
MSC_OPTIMIZATION=/Ox
!endif

USE_STL         = 1

TARGETLIBS      =

SOURCES=        \
    ..\COMFloat.cpp \
   
