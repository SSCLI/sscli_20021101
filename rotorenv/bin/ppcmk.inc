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
#    ppcmk.inc
#
#Abstract:
#
#     This module contains the PowerPC-specific build controls.  It is included
#     by makefile.def.
#
################################################################################

DELAYLOAD_SUPPORTED=1

#
# ppc option control
#
UMBASE=$(UMBASE:*=ppc)
UMLIBS=$(UMLIBS:*=ppc)
NTRES=$(NTRES:*=ppc)
UMRES=$(UMRES:*=ppc)
UMOBJS=$(UMOBJS:*=ppc)
LINKLIBS=$(LINKLIBS:*=ppc)
GDI_DRIVER_LIBS=$(GDI_DRIVER_LIBS:*=ppc)
DLLBASE=$(DLLBASE:*=ppc)
DLLDEF=$(DLLDEF:*=ppc)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=ppc)
TARGET=$(TARGET:*=ppc)
DYNLINK_LIB=$(DYNLINK_LIB:*=ppc)
TARGETEXEFILES=$(TARGETEXEFILES:*=ppc)
TARGETLIBFILES=$(TARGETLIBFILES:*=ppc)
TARGETOBJFILES=$(TARGETOBJFILES:*=ppc)
UMOBJFILES=$(UMOBJFILES:*=ppc)
UMEXEFILES=$(UMEXEFILES:*=ppc)
HEADERFILE=$(HEADERFILE:*=ppc)
HEADEROBJNAME=$(HEADEROBJNAME:*=ppc)
HEADEROBJ=$(HEADEROBJ:*=ppc)
PRECOMPILED=$(PRECOMPILED:*=ppc)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=ppc)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=ppc)
MFC_STATIC_LIB=$(MFC_STATIC_LIB:*=ppc)
CRT_LIB_PATH=$(CRT_LIB_PATH:*=ppc)
SDK_LIB_PATH=$(SDK_LIB_PATH:*=ppc)
ORDER=$(ORDER:*=ppc)
OBJLIBFILES=$(OBJLIBFILES:*=ppc)

MACHINE_TYPE=ppc
BIGENDIAN=1

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=ppc)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=ppc)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=ppc)
!endif

!ifdef PPC_PERFFLAGS
PERFFLAGS = $(PPC_PERFFLAGS)
!endif

# Set MSC_OPTIMIZATION.
# Order of precedence:
#   Platform specific override
#   Environment variable
#   System Default

!if defined(PPC_OPTIMIZATION)
MSC_OPTIMIZATION=$(PPC_OPTIMIZATION)
!elseif !defined(MSC_OPTIMIZATION)
MSC_OPTIMIZATION=$(DEFAULT_MSC_OPT)
!endif

DBGFLAGS=$(DBGFLAGS) $(MSC_OPTIMIZATION)

!IFDEF PPC_CPPFLAGS
MSC_CPPFLAGS=$(PPC_CPPFLAGS)
!ENDIF

STDFLAGS=-c -nostdinc -fno-rtti -fno-exceptions -no-cpp-precomp -fno-common \
            -fcheck-new -fmessage-length=0 -fPIC -DPIC -fno-schedule-insns

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
              -DBIGENDIAN
              
CFLAGS=$(PPC_FLAGS) $(STDFLAGS) $(DBGFLAGS) $(PERFFLAGS) $(USER_C_FLAGS)
PPC_ASMFLAGS=$(PPC_ADBGFLAGS) $(STD_DEFINES) $(ASM_DBG_DEFINES)\
              $(TARGET_DBG_DEFINES) $(ENV_DEFINES) $(ASM_DEFINES)

PPC_CDEFINES=$(CDEFINES)
PPC_CFLAGS=$(CFLAGS)

!ifndef CC_NAME
CC_NAME =cc
!endif

!ifndef RC_NAME
RC_NAME =resourcecompiler
!endif

CPP_MUNGER_NAME     =$(TARGETCOMPLUS)/cppmunge

C_COMPILER_NAME     =$(CC_NAME)
CXX_COMPILER_NAME   =$(CC_NAME)
C_PREPROCESSOR_NAME =$(CC_NAME)

EMPTY_RES       =empty_macho.o
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
SHARED_LIB          =-dylib
LINKER_FLAGS        =-arch ppc -dynamic -multiply_defined suppress $(LINKER_FLAGS)
!if "$(TARGETTYPE)" == "PROGRAM"
LINKER_FLAGS        =-bind_at_load $(LINKER_FLAGS)
!endif
PARTIAL_LINKING     =-r
ARCHIVE_FLAGS       =-rcs
RANLIB              =ranlib
RANLIB_FLAGS        =-s

!if "$(TARGETTYPE)" == "PROGRAM"
REQUIRED_C_LIBS     =/usr/lib/crt1.o
SONAME              =
!else if "$(TARGETTYPE)" == "DYNLINK"
REQUIRED_C_LIBS     =/usr/lib/dylib1.o
SONAME              =-dylib_install_name $(TARGETCOMPLUS)/$(TARGETPRE)$(TARGETLIBNAME).$(TARGETEXT)
!else
REQUIRED_C_LIBS     =
SONAME              =
!endif

CC_LIB              =/usr/lib/libcc_dynamic.a /usr/lib/libstdc++.a

!ifndef COMPILER_WARNINGS
COMPILER_WARNINGS=-Wall -Wno-non-virtual-dtor -Wno-non-template-friend
!endif

GLOBAL_C_FLAGS =-Ippc -I. $(INCPATH1) $(CDEFINES) $(CFLAGS)

NP_COMPILER_FLAGS =$(GLOBAL_C_FLAGS) $(COMPILER_WARNINGS)

C_PRECOMPILER_FLAGS =$(NP_COMPILER_FLAGS)
C_COMPILER_FLAGS =$(NP_COMPILER_FLAGS) $(PRECOMPILED)
CXX_COMPILER_FLAGS =$(NP_COMPILER_FLAGS) $(PRECOMPILED_CXX)

C_PREPROCESSOR_FLAGS =$(GLOBAL_C_FLAGS) -E

C_PREPROCESSOR =$(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS)
C_COMPILER     =$(C_COMPILER_NAME) $(C_COMPILER_FLAGS)
NP_C_COMPILER  =$(C_COMPILER_NAME) $(NP_COMPILER_FLAGS)
CXX_COMPILER   =$(CXX_COMPILER_NAME) $(CXX_COMPILER_FLAGS)

OUTPUT_FLAG     =-o

ECHO_MSG=$(C_COMPILER) $<
ECHO_MSG_P=$(NP_C_COMPILER) $**
ECHO_CXX_MSG=$(CXX_COMPILER) $<

{..\ppc\}.s{$(_OBJ_DIR)\ppc\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp $(OUTPUT_FLAG) $@ $<

{$(_OBJ_DIR)\ppc\}.s{$(_OBJ_DIR)\ppc\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp $(OUTPUT_FLAG) $@ $<

{ppc\}.s{$(_OBJ_DIR)\ppc\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp $(OUTPUT_FLAG) $@ $<
