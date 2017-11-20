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

NO_CRT = 1
!include $(NTMAKEENV)\sources.cor

!if "$(PLATFORM_UNIX)" != "1"
# WarningControl.h disables too many warnings.  classlibnative/nls folder will not use it.
COMPILER_WARNINGS	=/WX
!else
# This warns about behavior in fcall.h that's intended.
COMPILER_WARNINGS   =$(COMPILER_WARNINGS) -Wno-ctor-dtor-privacy
!endif

INCLUDES = $(INCLUDES);..;..\..;..\..\inc;..\..\..\vm;..\..\..\vm\$(_TGTCPU);..\..\..\inc;..\..\..\..\inc;..\..\..\interp;..\..\..\ceegen\inc;

MINORCOMP = cor

TARGETNAME = $(DDKBUILDENV)\comnls_$(_CORFLAVOR)
TARGETPATH = $(TARGETCORBIN)
TARGETTYPE = LIBRARY

C_DEFINES = $(C_DEFINES) 

C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE -DCRTDLL -DUSE_MAPSYM -DJIT_OR_NATIVE_SUPPORTED


CDEFINES = $(CDEFINES) $(C_DEFINES) -W4

USE_STL = 1


SOURCES =                 \
	..\CalendarTable.cpp \
    ..\COMNlsInfo.cpp        \
    ..\NLSTable.cpp          \
    ..\GlobalizationAssembly.cpp   \
    ..\CasingTable.cpp       \
    ..\BaseInfoTable.cpp     \
    ..\CultureInfoTable.cpp  \
    ..\RegionInfoTable.cpp   \
    ..\SortingTable.cpp \
    ..\SortingTableFile.cpp \
    ..\UnicodeCatTable.cpp \
    ..\jamo.cpp \
    ..\NativeTextInfo.cpp \
    ..\EncodingData.cpp
