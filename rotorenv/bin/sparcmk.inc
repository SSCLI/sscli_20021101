# ==++==
#
#   
#    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
#   
#    The use and distribution terms for this software are contained in the file
#    named license.txt, which can be found in the root of this distribution.
#    By using this software in any fashion, you are agreeing to be bound by the
#    terms of this license.
#   
#    You must not remove this notice, or any other, from this software.
#   
#
# ==--==
################################################################################
#
#Module Name:
#
#    sparcmk.inc
#
#Abstract:
#
#     This module contains the SPARC-specific build controls.  It is included
#     by makefile.def.
#
################################################################################

DELAYLOAD_SUPPORTED=1

#
# sparc option control
#
UMBASE=$(UMBASE:*=sparc)
UMLIBS=$(UMLIBS:*=sparc)
NTRES=$(NTRES:*=sparc)
UMRES=$(UMRES:*=sparc)
UMOBJS=$(UMOBJS:*=sparc)
LINKLIBS=$(LINKLIBS:*=sparc)
GDI_DRIVER_LIBS=$(GDI_DRIVER_LIBS:*=sparc)
DLLBASE=$(DLLBASE:*=sparc)
DLLDEF=$(DLLDEF:*=sparc)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=sparc)
TARGET=$(TARGET:*=sparc)
DYNLINK_LIB=$(DYNLINK_LIB:*=sparc)
TARGETEXEFILES=$(TARGETEXEFILES:*=sparc)
TARGETLIBFILES=$(TARGETLIBFILES:*=sparc)
TARGETOBJFILES=$(TARGETOBJFILES:*=sparc)
UMOBJFILES=$(UMOBJFILES:*=sparc)
UMEXEFILES=$(UMEXEFILES:*=sparc)
HEADERFILE=$(HEADERFILE:*=sparc)
HEADEROBJNAME=$(HEADEROBJNAME:*=sparc)
HEADEROBJ=$(HEADEROBJ:*=sparc)
PRECOMPILED=$(PRECOMPILED:*=sparc)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=sparc)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=sparc)
MFC_STATIC_LIB=$(MFC_STATIC_LIB:*=sparc)
CRT_LIB_PATH=$(CRT_LIB_PATH:*=sparc)
SDK_LIB_PATH=$(SDK_LIB_PATH:*=sparc)
ORDER=$(ORDER:*=sparc)
OBJLIBFILES=$(OBJLIBFILES:*=sparc)

MACHINE_TYPE=sparc
BIGENDIAN=1

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=sparc)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=sparc)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=sparc)
!endif

!ifdef SPARC_PERFFLAGS
PERFFLAGS = $(SPARC_PERFFLAGS)
!endif

# Set MSC_OPTIMIZATION.
# Order of precedence:
#   Platform specific override
#   Environment variable
#   System Default

!if defined(SPARC_OPTIMIZATION)
MSC_OPTIMIZATION=$(SPARC_OPTIMIZATION)
!elseif !defined(MSC_OPTIMIZATION)
MSC_OPTIMIZATION=$(DEFAULT_MSC_OPT)
!endif

DBGFLAGS=$(DBGFLAGS) $(MSC_OPTIMIZATION)

!IFDEF SPARC_CPPFLAGS
MSC_CPPFLAGS=$(SPARC_CPPFLAGS)
!ENDIF

STDFLAGS=-c -nostdinc -fno-rtti -fno-exceptions -fcheck-new -fPIC -DPIC \
         -fno-schedule-insns

FPO_OPT=0

!IFNDEF FPO_OPT
!IF "$(NTDEBUG)" == "retail" || "$(NTDEBUG)" == "" || "$(NTDEBUG)" == "ntsdnodbg"
FPO_OPT=1
!ENDIF
!ENDIF

ENV_DEFINES= $(MSC_CPPFLAGS) $(NTCPPFLAGS)

!IF "$(HALTYPE)" == ""
HALDEF=
!ELSE
HALDEF=-D$(HALTYPE)=1
!ENDIF

STD_DEFINES=$(TARGET_DEFINES) $(STD_DEFINES)
CDEFINES=$(STD_DEFINES) $(TARGET_DBG_DEFINES) $(ENV_DEFINES) \
              $(LIBC_DEFINES) $(C_DEFINES) $(NET_C_DEFINES) \
              -DBIGENDIAN -DALIGN_ACCESS
              
CFLAGS=$(SPARC_FLAGS) $(STDFLAGS) $(DBGFLAGS) $(PERFFLAGS) $(USER_C_FLAGS)
SPARC_ASMFLAGS=$(SPARC_ADBGFLAGS) $(STD_DEFINES) $(ASM_DBG_DEFINES) \
               $(TARGET_DBG_DEFINES) $(ENV_DEFINES) $(ASM_DEFINES)

SPARC_CDEFINES=$(CDEFINES)
SPARC_CFLAGS=$(CFLAGS)

!ifndef CC_NAME
CC_NAME =gcc
!endif

!ifndef RC_NAME
RC_NAME =resourcecompiler
!endif

CPP_MUNGER_NAME     =$(TARGETCOMPLUS)/cppmunge

C_COMPILER_NAME     =$(CC_NAME)
CXX_COMPILER_NAME   =$(CC_NAME)
C_PREPROCESSOR_NAME =$(CC_NAME)

EMPTY_RES       =empty_sparc.o
!ifndef COPY_NAME
COPY_NAME       =cp -f
!endif

!ifndef XCOPY
XCOPY           =cp -rf
!endif

!ifndef TYPE_COMMAND
TYPE_COMMAND    =cat
!endif

DELETER         =rm -f
MD              =mkdir -p
CAUSE_ERROR     =mkdir / 2> /dev/null
MAKE_NEW_DIR    =if [ ! -d $(@D) ]; then $(MD) $(@D); fi

# Linker details specific to Unix.
START_GROUP         =
END_GROUP           =
SHARED_LIB          =-G
LINKER_FLAGS        =
PARTIAL_LINKING     =-r
ARCHIVE_FLAGS       =-rcs

GCC_LIB_DIR=/usr/local/lib/gcc-lib/sparc-sun-solaris2.8/2.95.3
!if "$(TARGETTYPE)" == "PROGRAM"
REQUIRED_C_LIBS     =$(GCC_LIB_DIR)/crt1.o
!endif
REQUIRED_C_LIBS     =$(REQUIRED_C_LIBS) $(GCC_LIB_DIR)/crti.o \
                     /usr/ccs/lib/values-Xa.o $(GCC_LIB_DIR)/crtbegin.o \
                     $(GCC_LIB_DIR)/crtend.o $(GCC_LIB_DIR)/crtn.o

CC_LIB              =$(GCC_LIB_DIR)/libgcc.a $(GCC_LIB_DIR)/libstdc++.a \
                     /usr/lib/libc.so /usr/lib/libm.so

!ifndef COMPILER_WARNINGS
COMPILER_WARNINGS=-Wall -Wno-non-virtual-dtor -Wno-non-template-friend
!endif

GLOBAL_C_FLAGS =-Isparc -I. $(INCPATH1) $(CDEFINES) $(CFLAGS)

NP_COMPILER_FLAGS =$(GLOBAL_C_FLAGS) $(COMPILER_WARNINGS)

C_COMPILER_FLAGS =$(NP_COMPILER_FLAGS)
CXX_COMPILER_FLAGS =$(NP_COMPILER_FLAGS)
C_PREPROCESSOR_FLAGS =$(GLOBAL_C_FLAGS) -E

C_PREPROCESSOR =$(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS)
C_COMPILER     =$(C_COMPILER_NAME) $(C_COMPILER_FLAGS)
NP_C_COMPILER  =$(C_COMPILER_NAME) $(NP_COMPILER_FLAGS)
CXX_COMPILER   =$(CXX_COMPILER_NAME) $(CXX_COMPILER_FLAGS)

OUTPUT_FLAG     =-o

ECHO_MSG=$(C_COMPILER) $<
ECHO_MSG_P=$(NP_C_COMPILER) $**
ECHO_CXX_MSG=$(CXX_COMPILER) $<

{..\sparc\}.s{$(_OBJ_DIR)\sparc\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp -Wa,-Av9 $(OUTPUT_FLAG) $@ $<

{$(_OBJ_DIR)\sparc\}.s{$(_OBJ_DIR)\sparc\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp -Wa,-Av9 $(OUTPUT_FLAG) $@ $<

{sparc\}.s{$(_OBJ_DIR)\sparc\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp -Wa,-Av9 $(OUTPUT_FLAG) $@ $<
