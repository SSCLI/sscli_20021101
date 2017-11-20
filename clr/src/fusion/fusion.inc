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

MSC_WARNING_LEVEL = /W3 /WX
NO_NTDLL=1
USE_MSVCRT=1

!if $(FREEBUILD)
C_DEFINES = $(C_DEFINES) 
!else
C_DEFINES = $(C_DEFINES) -DDEBUG
!endif

C_DEFINES = $(C_DEFINES) -DFUSION_QUALIFYASSEMBLY_ENABLED -DFUSION_PARTIAL_BIND_DEBUG -DFUSION_RETAIL_LOGGING -DFUSION_REDEFINE_ASSERT=1 -DMSOERT_NO_PROTSTOR=1



#
# Set private include paths
#

NO_BROWSER_FILE=1

FUSION_PATH = $(CORBASE)\src\fusion

FUSION_ALL_INC_PATH = $(FUSION_PATH)\inc

FUSION_WRAPPER_INC_PATH=$(FUSION_PATH)\wrappers

FUSION_URT_LIB_DEST = $(FUSION_PATH)\urtobj\$(_OBJ_DIR)
FUSION_URT_LIB_PATH = $(FUSION_URT_LIB_DEST)\*

FUSION_LIB_DEST = $(FUSION_URT_LIB_DEST)
FUSION_INC_PATH=$(FUSION_ALL_INC_PATH)
FUSION_BUILD_TYPE_PREFIX=

INCLUDES        = .; \
    $(FUSION_PATH)\inc; \
    $(FUSION_PATH)\idl\$(O); \
    $(CORBASE)\src\inc; \
    $(CORBASE)\src\inc\$(O); \
    $(WINDOWS_INC_PATH);\
    $(BASE_INC_PATH);\
    $(SDK_INC_PATH);\
    $(NET_INC_PATH);\
    $(INCLUDES)

!ifndef FUSION_MBCS
C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif

#USE_FUSION_WRAPPERS=1
!ifdef USE_FUSION_WRAPPERS
C_DEFINES = $(C_DEFINES) -D_WIN32_IE=0x0500 -DUSE_FUSWRAPPERS
INCLUDES        = \
    $(CORBASE)\src\fusion\wrappers; \
    $(INCLUDES)
!endif

!include $(NTMAKEENV)\sources.cor
SOURCES_USED=$(SOURCES_USED) $(NTMAKEENV)\sources.cor

EXCEPTION_HANDLING = $(SEHONLY)


# don't default to linking against any .lib files
USE_NOLIBS=1

# undefine USE_MSVCRT defined in sources.cor
!undef USE_MSVCRT


