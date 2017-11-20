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
#    rotor_x86mk.inc
#
#Abstract:
#
#     This module contains the rotor x86 specific build controls.  It is included
#     by makefile.def.
#
################################################################################

DELAYLOAD_SUPPORTED=1

#
# rotor_x86 option control
#
UMBASE=$(UMBASE:*=rotor_x86)
UMLIBS=$(UMLIBS:*=rotor_x86)
NTRES=$(NTRES:*=rotor_x86)
UMRES=$(UMRES:*=rotor_x86)
UMOBJS=$(UMOBJS:*=rotor_x86)
LINKLIBS=$(LINKLIBS:*=rotor_x86)
GDI_DRIVER_LIBS=$(GDI_DRIVER_LIBS:*=rotor_x86)
DLLBASE=$(DLLBASE:*=rotor_x86)
DLLDEF=$(DLLDEF:*=rotor_x86)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=rotor_x86)
TARGET=$(TARGET:*=rotor_x86)
DYNLINK_LIB=$(DYNLINK_LIB:*=rotor_x86)
TARGETEXEFILES=$(TARGETEXEFILES:*=rotor_x86)
TARGETLIBFILES=$(TARGETLIBFILES:*=rotor_x86)
TARGETOBJFILES=$(TARGETOBJFILES:*=rotor_x86)
UMOBJFILES=$(UMOBJFILES:*=rotor_x86)
UMEXEFILES=$(UMEXEFILES:*=rotor_x86)
HEADERFILE=$(HEADERFILE:*=rotor_x86)
HEADEROBJNAME=$(HEADEROBJNAME:*=rotor_x86)
HEADEROBJ=$(HEADEROBJ:*=rotor_x86)
PRECOMPILED=$(PRECOMPILED:*=rotor_x86)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=rotor_x86)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=rotor_x86)
MFC_STATIC_LIB=$(MFC_STATIC_LIB:*=i386)
CRT_LIB_PATH=$(CRT_LIB_PATH:*=i386)
SDK_LIB_PATH=$(SDK_LIB_PATH:*=i386)
ORDER=$(ORDER:*=i386)
OBJLIBFILES=$(OBJLIBFILES:*=rotor_x86)
COFFBASE_TXT_FILE=$(COFFBASE_TXT_FILE:*=rotor_x86)

MACHINE_TYPE=i386

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=rotor_x86)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=rotor_x86)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=rotor_x86)
!endif

!IF "$(DLLENTRY)" != "-noentry"
!   IF "$(DLLENTRY:@12=)" == "$(DLLENTRY)"
!ifndef PLATFORM_UNIX
DLLENTRY=$(DLLENTRY)@12
!endif
!   ENDIF
!ENDIF

!IFDEF STD_CALL_ENTRY
UMENTRY=$(UMENTRY)@4
!ENDIF

ENTRY_SUFFIX=@8
GDI_ENTRY_SUFFIX=@12

!IFNDEF MSC_WARNING_LEVEL
!IFDEF 386_WARNING_LEVEL
MSC_WARNING_LEVEL=$(386_WARNING_LEVEL)
!ENDIF
!ENDIF

!ifdef 386_PERFFLAGS
PERFFLAGS = $(386_PERFFLAGS)
!endif

# Set MSC_OPTIMIZATION.
# Order of precedence:
#   Platform specific override
#   Environment variable
#   System Default

!if defined(386_OPTIMIZATION)
MSC_OPTIMIZATION=$(386_OPTIMIZATION)
!elseif !defined(MSC_OPTIMIZATION)
MSC_OPTIMIZATION=$(DEFAULT_MSC_OPT)
!endif

#DBGFLAGS=$(DBGFLAGS) /Zvc6
DBGFLAGS=$(DBGFLAGS) $(MSC_OPTIMIZATION)

!IFDEF 386_CPPFLAGS
MSC_CPPFLAGS=$(386_CPPFLAGS)
!ENDIF

!ifdef NO_READONLY_STRINGS
STRING_POOLING = /Gf
!else
STRING_POOLING = /GF
!endif

!ifdef USE_NATIVE_EH
EH_FLAGS=/GX
RTTI_FLAGS=/GR
!else
EH_FLAGS=/GX-
! ifdef USE_RTTI
RTTI_FLAGS=/GR
! else
RTTI_FLAGS=/GR-
! endif
!endif

!ifdef USE_INCREMENTAL_COMPILING
C_INCREMENTAL_FLAG = /Gi /Gm
USE_PDB_TO_COMPILE = 1
!else
C_INCREMENTAL_FLAG = /Gi- /Gm-
!endif

!ifndef X86_CPU_OPTIMIZATION
X86_CPU_OPTIMIZATION=/GB
!endif


# ERATTA_FLAGS=/QIfdiv- /QIf
ERATTA_FLAGS=/QIfdiv- /QIf /QI0f

!ifdef PLATFORM_UNIX
STDFLAGS= -c -nostdinc -fno-builtin -fno-rtti -fno-exceptions -fcheck-new -fpic -DPIC
!else
STDFLAGS= /c /Zel /Zp8 /Gy $(CBSTRING) $(MSC_WARNING_LEVEL) $(MSC_CALL_TYPE) \
          $(ERATTA_FLAGS) $(X86_CPU_OPTMIZATION) $(C_INCREMENTAL_FLAG) $(EH_FLAGS) \
          $(RTTI_FLAGS) $(STRING_POOLING) $(MFC_FLAGS)
!endif

!IF "$(386_STDCALL)" == "0"
MSC_CALL_TYPE=/Gd
MSC_CALL_DEFINE=
!ELSE
!   IF "$(386_STDCALL)" == "2"
MSC_CALL_TYPE=/Gr
MSC_CALL_DEFINE=
!   ELSE
MSC_CALL_TYPE=/Gz
MSC_CALL_DEFINE=-DSTD_CALL
!   ENDIF
!ENDIF

FPO_OPT=0

!IFNDEF FPO_OPT
!IF "$(NTDEBUG)" == "retail" || "$(NTDEBUG)" == "" || "$(NTDEBUG)" == "ntsdnodbg"
FPO_OPT=1
!ELSE 
386_ADBGFLAGS=$(386_ADBGFLAGS) /Zi
!ENDIF
!ENDIF

!ifndef PLATFORM_UNIX
!IF "$(FPO_OPT)" == "1"
TARGET_DBG_DEFINES= $(TARGET_DBG_DEFINES) -DFPO=1
DBGFLAGS=$(DBGFLAGS) /Oy 
!ELSE
TARGET_DBG_DEFINES= $(TARGET_DBG_DEFINES) -DFPO=0
DBGFLAGS=$(DBGFLAGS) /Oy-
!ENDIF
!endif	

!ifndef PLATFORM_UNIX
!ifndef USE_PDB_TO_COMPILE
DBGFLAGS=$(DBGFLAGS:/Zi=-Z7)
DBGFLAGS=$(DBGFLAGS:-Zi=-Z7)
!else
! if "$(TARGETTYPE)" == "LIBRARY"
DBGFLAGS=$(DBGFLAGS) /Fd$(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).pdb
! else
DBGFLAGS=$(DBGFLAGS) /Fd$(MAKEDIR)\$(_OBJ_DIR)\rotor_x86^\
! endif
!endif
!endif

!ifndef PLATFORM_UNIX
! ifndef NO_BROWSER_INFO
!  ifdef BROWSER_INFO
DBGFLAGS=$(DBGFLAGS) /FR$(MAKEDIR)\$(_OBJ_DIR)\rotor_x86^\
!  endif
! endif
!endif

#
# 386 option control
#

ENV_DEFINES= $(MSC_CPPFLAGS) $(NTCPPFLAGS)

!IF "$(HALTYPE)" == ""
HALDEF=
!ELSE
HALDEF=-D$(HALTYPE)=1
!ENDIF

STD_DEFINES= $(TARGET_DEFINES) $(HALDEF) $(MSC_CALL_DEFINE) $(STD_DEFINES)

CDEFINES=$(STD_DEFINES) $(TARGET_DBG_DEFINES) $(ENV_DEFINES) \
              $(LIBC_DEFINES) $(C_DEFINES) $(NET_C_DEFINES)
CFLAGS=$(386_FLAGS) $(NT386FLAGS) $(STDFLAGS) $(DBGFLAGS) $(PERFFLAGS) $(USER_C_FLAGS)
386_ASMFLAGS=$(386_ADBGFLAGS) $(STD_DEFINES) $(ASM_DBG_DEFINES)\
              $(TARGET_DBG_DEFINES) $(ENV_DEFINES) $(ASM_DEFINES)

386_CDEFINES=$(CDEFINES)
386_CFLAGS=$(CFLAGS)

!IFNDEF CC_NAME
!IF defined(PLATFORM_UNIX)
CC_NAME =gcc
!ELSE
!IF defined(BOUNDSCHECK)
CC_NAME =nmcl -nologo
!ELSEIF defined(PREFIX_BUILD)
CC_NAME =$(INTRINSA_HOME)\bin\interceptor\cl -nologo
!ELSE
CC_NAME =cl -nologo
!ENDIF
!ENDIF
!ENDIF

!ifndef RC_NAME
!ifdef PLATFORM_UNIX
RC_NAME=resourcecompiler
EMPTY_RES=empty.o
!else
RC_NAME=resourcecompiler.exe
EMPTY_RES=empty.res
!endif
!endif

!ifndef COPY_NAME
!ifdef PLATFORM_UNIX
COPY_NAME=cp -f
!else
COPY_NAME=copy
!endif
!endif

!ifndef XCOPY
!ifdef PLATFORM_UNIX
XCOPY=cp -rf
!else
XCOPY=xcopy /r
!endif
!endif

!ifndef TYPE_COMMAND
!ifdef PLATFORM_UNIX
TYPE_COMMAND=cat
!else
TYPE_COMMAND=type
!endif
!endif

C_COMPILER_NAME     = $(CC_NAME)
CXX_COMPILER_NAME   = $(CC_NAME)
C_PREPROCESSOR_NAME = $(CC_NAME)

!ifdef PLATFORM_UNIX
CPP_MUNGER_NAME     =$(TARGETCOMPLUS)/cppmunge
!endif

!ifdef PLATFORM_UNIX
DELETER         = rm -f
MD              = mkdir -p
!else
DELETER         = erase /q
MD              = md
!endif

# Sometimes we need to tell nmake to abort. Anything that
# returns an error will do that. These examples are used
# only due to inertia...feel free to change them to
# something similarly useless that returns an error.
!ifdef PLATFORM_UNIX
CAUSE_ERROR     = mkdir / 2> /dev/null
!else
CAUSE_ERROR     = md \ >nul 2>nul
!endif

# Create a directory that doesn't already exist.
!ifdef PLATFORM_UNIX
MAKE_NEW_DIR    = if [ ! -d $(@D) ]; then $(MD) $(@D); fi
!else
MAKE_NEW_DIR    = if not exist $(@D) $(MD) $(@D)
!endif

# These are prepended and appended to filenames to
# produce the correct name for various files on each
# platform. Since the resulting string must be exactly
# correct, do not insert a space after the '=' sign or
# after the value in any of these assignments.
!ifdef PLATFORM_UNIX
ASM_SUFFIX          =s
DYNLIB_PREFIX       =lib
DYNLIB_SUFFIX       =so
SHARED_LIB_EXT      =so
!ifdef OTHER_X86
CC_LIB              =$(GCC_LIB_DIR)/libgcc.a $(GCC_EH_LIB) \
                     /usr/local/lib/libstdc++.a
!else
CC_LIB              =/usr/lib/libgcc.a
!endif
!else
ASM_SUFFIX          =asm
DYNLIB_PREFIX       =
DYNLIB_SUFFIX       =lib
!endif

# Linker details specific to Unix.
!ifdef PLATFORM_UNIX
START_GROUP         =
END_GROUP           =
SHARED_LIB          =-shared
PARTIAL_LINKING     =-Ur
ARCHIVE_FLAGS       =-rcs
!ifdef OTHER_X86
!ifdef GCC_EH_LIB
STDFLAGS            =$(STDFLAGS) -fmessage-length=0
!endif
REQUIRED_C_LIBS     =/usr/lib/crt1.o /usr/lib/crti.o /usr/lib/crtn.o \
                     $(GCC_LIB_DIR)/crtbegin.o $(GCC_LIB_DIR)/crtend.o
!else
REQUIRED_C_LIBS     =/usr/lib/crt1.o /usr/lib/crti.o /usr/lib/crtbeginS.o \
                     /usr/lib/crtendS.o /usr/lib/crtn.o
!endif
!endif

!ifndef 386_ASSEMBLER_NAME
386_ASSEMBLER_NAME  = ml
!endif

!IFNDEF COMPILER_WARNINGS
!IFDEF PLATFORM_UNIX
COMPILER_WARNINGS=-Wall -Wno-non-virtual-dtor -Wno-non-template-friend
!ENDIF
!ENDIF

GLOBAL_C_FLAGS = -Ii386\ -I. $(INCPATH1) $(CDEFINES) $(CFLAGS)

NP_COMPILER_FLAGS = $(GLOBAL_C_FLAGS) $(COMPILER_WARNINGS)

!ifdef PLATFORM_UNIX
C_COMPILER_FLAGS = $(NP_COMPILER_FLAGS)
CXX_COMPILER_FLAGS = $(NP_COMPILER_FLAGS)
C_PREPROCESSOR_FLAGS = $(GLOBAL_C_FLAGS) -E
!else
C_COMPILER_FLAGS = $(NP_COMPILER_FLAGS) $(PRECOMPILED)
CXX_COMPILER_FLAGS = $(NP_COMPILER_FLAGS) $(PRECOMPILED_CXX)
C_PREPROCESSOR_FLAGS = $(GLOBAL_C_FLAGS) -EP -Tc
!endif

C_PREPROCESSOR = $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS)
C_COMPILER     = $(C_COMPILER_NAME) $(C_COMPILER_FLAGS)
NP_C_COMPILER  = $(C_COMPILER_NAME) $(NP_COMPILER_FLAGS)
CXX_COMPILER   = $(CXX_COMPILER_NAME) $(CXX_COMPILER_FLAGS)

!ifdef PLATFORM_UNIX
OUTPUT_FLAG     = -o
!else
OUTPUT_FLAG     = -Fo
!endif

!if defined(NOMASMCOMPATIBILITY)
386_ASSEMBLER=$(386_ASSEMBLER_NAME) -c -coff -Cx -nologo -Ii386\ -I. $(INCPATH1) $(386_ASMFLAGS)
!else
386_ASSEMBLER=$(386_ASSEMBLER_NAME) -c -coff -Cx -nologo -Ii386\ -I. $(INCPATH1) -Zm $(386_ASMFLAGS)
!endif

ECHO_MSG=$(C_COMPILER) $<
ECHO_MSG_P=$(NP_C_COMPILER) $**
ECHO_CXX_MSG=$(CXX_COMPILER) $<

ECHO_PRECOMPILED_MSG1=$(C_COMPILER_NAME) $(PRECOMPILED_FLAG) $(PRECOMPILED_SOURCEFILE) \
               $(C_COMPILER_FLAGS) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) $(HEADEROBJ)

ECHO_PRECOMPILED_MSG2=$(C_COMPILER_NAME) $(PRECOMPILED_FLAG) \
               $(C_COMPILER_FLAGS) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) $(HEADEROBJ)


{}.c{}.asm:
    $(C_COMPILER_NAME) @<< /Fa $(MAKEDIR)\$(<F)
$(C_COMPILER_FLAGS: =
)
<<NOKEEP

!ifdef PLATFORM_UNIX
{..\rotor_x86\}.s{$(_OBJ_DIR)\rotor_x86\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp $(OUTPUT_FLAG) $@ $<

{$(_OBJ_DIR)\rotor_x86\}.s{$(_OBJ_DIR)\rotor_x86\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp $(OUTPUT_FLAG) $@ $<

{rotor_x86\}.s{$(_OBJ_DIR)\rotor_x86\}.obj:
    $(C_COMPILER_NAME) $(C_COMPILER_FLAGS) -x assembler-with-cpp $(OUTPUT_FLAG) $@ $<
!else
{..\rotor_x86\}.asm{}.lst:
    $(386_ASSEMBLER) /Fl$@ $(OUTPUT_FLAG)$(_OBJ_DIR)\rotor_x86\$(@B).obj $<

{rotor_x86\}.asm{}.lst:
    $(386_ASSEMBLER) /Fl$@ $(OUTPUT_FLAG)$(MAKEDIR)\$(_OBJ_DIR)\rotor_x86\$(@B).obj $<

{..\rotor_x86\}.asm{$(_OBJ_DIR)\rotor_x86\}.obj:
    $(386_ASSEMBLER) $(OUTPUT_FLAG)$(MAKEDIR)\$@ $<

{$(_OBJ_DIR)\rotor_x86\}.asm{$(_OBJ_DIR)\rotor_x86\}.obj:
    $(386_ASSEMBLER) $(OUTPUT_FLAG)$(MAKEDIR)\$@ $<

{rotor_x86\}.asm{$(_OBJ_DIR)\rotor_x86\}.obj:
    $(386_ASSEMBLER) $(OUTPUT_FLAG)$(MAKEDIR)\$@ $<
!endif

!IFDEF _NT386LIBS
_NTLIBS=$(_NT386LIBS)
!ENDIF

COPYDST=$(386COPYDST)
LIB_COPY=nti386cp.cmd

!IF "$(NTDEBUGTYPE)" == "windbg" || "$(NTDEBUGTYPE)" == "both"
NTTEST_LINK_OPTIONS=-entry:main
!ELSE
NTTEST_LINK_OPTIONS=-base:0x80100000 -entry:main
!ENDIF
