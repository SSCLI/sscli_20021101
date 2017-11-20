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
#
# If not defined, specify where to get incs and libs.
#

!IFNDEF _NTROOT
_NTROOT=\nt
!ENDIF

!IFNDEF BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!ENDIF

#
# If not defined, define the build message banner.
#

!IFNDEF BUILDMSG
BUILDMSG=
!ENDIF

!if ("$(NTDEBUG)" == "") || ("$(NTDEBUG)" == "retail") || ("$(NTDEBUG)" == "ntsdnodbg")
FREEBUILD=1
!else
FREEBUILD=0
!endif


# Allow alternate object directories.

!ifndef BUILD_ALT_DIR
BUILD_ALT_DIR=
! ifdef CHECKED_ALT_DIR
! if !$(FREEBUILD)
BUILD_ALT_DIR=d
!  endif
! endif
!endif

_OBJ_DIR = obj$(BUILD_ALT_DIR)


#
# Determine which target is being built (i386, axp64 or Alpha) and define
# the appropriate target variables.
#

!IFNDEF ROTOR_X86
ROTOR_X86=0
!ENDIF

386=0

!IFNDEF AXP64
AXP64=0
!ENDIF

ALPHA=0

!IFNDEF PPC
PPC=0
!ENDIF

!IFNDEF MPPC
MPPC=0
!ENDIF

!IFNDEF IA64
IA64=0
!ENDIF

!ifndef SPARC
SPARC=0
!endif

# Disable for now.
MIPS=0

#
# Default to building for the i386 target, if no target is specified.
#

!IF !$(ROTOR_X86)
!  IF !$(386)
!    IF !$(AXP64)
!           IF !$(PPC)
!               IF !$(MPPC)
!                   IF !$(IA64)
!                       IF !$(SPARC)
!                           IFDEF NTAXP64DEFAULT
AXP64=1
!                               IFNDEF TARGETCPU
TARGETCPU=AXP64
!                               ENDIF
!                           ELSE
!                               IFDEF NTALPHADEFAULT
ALPHA=1
!                                   IFNDEF TARGETCPU
TARGETCPU=ALPHA
!                                   ENDIF
!                               ELSE
!                                   IFDEF NTPPCDEFAULT
PPC=1
!                                       IFNDEF TARGETCPU
TARGETCPU=PPC
!                                       ENDIF
!                                   ELSE
!                                       IFDEF NTMPPCDEFAULT
MPPC=1
!                                           IFNDEF TARGETCPU
TARGETCPU=MPPC
!                                           ENDIF
!                                       ELSE
!                                           IFDEF NTIA64DEFAULT
IA64=1
!                                               IFNDEF TARGETCPU
TARGETCPU=IA64
!                                               ENDIF
!                                           ELSE
386=1
!                                               IFNDEF TARGETCPU
TARGETCPU=I386
!                                               ENDIF
!                                           ENDIF
!                                       ENDIF
!                                   ENDIF
!                               ENDIF
!                           ENDIF
!                       ENDIF
!                   ENDIF
!               ENDIF
!           ENDIF
!    ENDIF
!  ENDIF
!ENDIF

#
# Define the target platform specific information.
#
!if $(ROTOR_X86)
ASM_SUFFIX=asm
ASM_INCLUDE_SUFFIX=inc

TARGET_BRACES=
TARGET_CPP=cl
TARGET_DEFINES=-Di386 -D_X86_
TARGET_DIRECTORY=rotor_x86
TARGET_NTTREE=$(_NT386TREE)

!ifdef PLATFORM_UNIX
SHARED_LIB_EXT =so
!endif

VCCOM_SUPPORTED=1

!elseif $(386)

ASM_SUFFIX=asm
ASM_INCLUDE_SUFFIX=inc

TARGET_BRACES=
TARGET_CPP=cl
TARGET_DEFINES=-Di386=1 -D_X86_=1
TARGET_DIRECTORY=i386
TARGET_NTTREE=$(_NT386TREE)

MIDL_CPP=$(TARGET_CPP)
MIDL_FLAGS=$(TARGET_DEFINES) -D_WCHAR_T_DEFINED

!if "$(BUILD_ALT_DIR)" != ""
! if defined(_NT386TREE_ALT) && defined (_NT386TREE)
_NT386TREE = $(_NT386TREE_ALT)
! endif
! if defined(_NT386TREE_NS_ALT) && defined (_NT386TREE_NS)
_NT386TREE_NS = $(_NT386TREE_NS_ALT)
! endif
!endif

!IF DEFINED (CAIRO_PRODUCT)
! IF DEFINED (_CAIRO386TREE)
_NTTREE=$(_CAIRO386TREE)
_NTTREE_NO_SPLIT=$(_CAIRO386TREE_NS)
! ENDIF
!ELSEIF DEFINED (CHICAGO_PRODUCT)
! IF DEFINED (_CHICAGO386TREE)
_NTTREE=$(_CHICAGO386TREE)
_NTTREE_NO_SPLIT=$(_CHICAGO386TREE_NS)
! elseif defined(_NT386TREE)
_NTTREE=$(_NT386TREE)
_NTTREE_NO_SPLIT=$(_NT386TREE_NS)
! ENDIF
!ELSEIF DEFINED (_NT386TREE)
_NTTREE=$(_NT386TREE)
_NTTREE_NO_SPLIT=$(_NT386TREE_NS)
!ENDIF

VCCOM_SUPPORTED=1

!elseif $(AXP64)

ASM_SUFFIX=s
ASM_INCLUDE_SUFFIX=h

TARGET_BRACES=-B
TARGET_CPP=cl
TARGET_DEFINES=-DALPHA -D_ALPHA_
TARGET_DIRECTORY=axp64
TARGET_NTTREE=$(_NTAXP64TREE)

MIDL_CPP=$(TARGET_CPP)
MIDL_FLAGS=$(TARGET_DEFINES) -D_WCHAR_T_DEFINED

!if "$(BUILD_ALT_DIR)" != ""
! if defined(_NTAXP64TREE_ALT) && defined (_NTAXP64TREE)
_NTAXP64TREE = $(_NTAXP64TREE_ALT)
! endif
! if defined(_NTAXP64TREE_NS_ALT) && defined (_NTAXP64TREE_NS)
_NTAXP64TREE_NS = $(_NTAXP64TREE_NS_ALT)
! endif
!endif

!IFDEF CAIRO_PRODUCT
!IFDEF _CAIROAXP64TREE
_NTTREE=$(_CAIROAXP64TREE)
_NTTREE_NO_SPLIT=$(_CAIROAXP64TREE_NS)
!ENDIF
!ELSE
!IFDEF _NTAXP64TREE
_NTTREE=$(_NTAXP64TREE)
_NTTREE_NO_SPLIT=$(_NTAXP64TREE_NS)
!ENDIF
!ENDIF

VCCOM_SUPPORTED=1

!elseif $(PPC)

ASM_SUFFIX=s
ASM_INCLUDE_SUFFIX=h

DYNLIB_PREFIX   =lib
DYNLIB_SUFFIX   =dylib
SHARED_LIB_EXT  =dylib

TARGET_DEFINES=-DPPC -D_PPC_
TARGET_DIRECTORY=ppc

!elseif $(SPARC)

ASM_SUFFIX=s
ASM_INCLUDE_SUFFIX=h

DYNLIB_PREFIX   =lib
DYNLIB_SUFFIX   =so
SHARED_LIB_EXT  =so

TARGET_DEFINES=-DSPARC -D_SPARC_
TARGET_DIRECTORY=sparc

!elseif $(MPPC)

ASM_SUFFIX=s
ASM_INCLUDE_SUFFIX=h

TARGET_BRACES=-B
TARGET_CPP= cl
TARGET_DEFINES=-DMPPC -D_MPPC_
TARGET_DIRECTORY=mppc
TARGET_NTTREE=$(_NTMPPCTREE)

MIDL_CPP=$(TARGET_CPP)
MIDL_FLAGS=$(TARGET_DEFINES) -D_WCHAR_T_DEFINED

!elseif $(IA64)

ASM_SUFFIX=s
ASM_INCLUDE_SUFFIX=h

TARGET_BRACES=-B
TARGET_CPP=cl
TARGET_DEFINES=-DIA64 -D_IA64_
TARGET_DIRECTORY=ia64
TARGET_NTTREE=$(_NTIA64TREE)

MIDL_CPP=$(TARGET_CPP)
MIDL_FLAGS=$(TARGET_DEFINES) -D_WCHAR_T_DEFINED

!if "$(BUILD_ALT_DIR)" != ""
! if defined(_NTIA64TREE_ALT) && defined (_NTIA64TREE)
_NTIA64TREE = $(_NTIA64TREE_ALT)
! endif
! if defined(_NTIA64TREE_NS_ALT) && defined (_NTIA64TREE_NS)
_NTIA64TREE_NS = $(_NTIA64TREE_NS_ALT)
! endif
!endif

!IFDEF CAIRO_PRODUCT
!IFDEF _CAIROIA64TREE
_NTTREE=$(_CAIROIA64TREE)
_NTTREE_NO_SPLIT=$(_CAIROIA64TREE_NS)
!ENDIF
!ELSE
!IFDEF _NTIA64TREE
_NTTREE=$(_NTIA64TREE)
_NTTREE_NO_SPLIT=$(_NTIA64TREE_NS)
!ENDIF
!ENDIF

!else
!error Must define the target as 386, axp64, alpha, ppc, mppc or ia64.
!endif

#
#  These flags don't depend on i386 etc. however have to be in this file.
#
#  MIDL_OPTIMIZATION is the optimization flag set for the current NT.
#  MIDL_OPTIMIZATION_NO_ROBUST is the current optimization without robust.
#
!IFNDEF MIDL_OPTIMIZATION
MIDL_OPTIMIZATION=-Oicf -robust -no_format_opt -error all
!ENDIF
MIDL_OPTIMIZATION_NO_ROBUST=-Oicf -no_format_opt -error all
MIDL_OPTIMIZATION_NT4=-Oicf -no_format_opt -error all
MIDL_OPTIMIZATION_NT5=-Oicf -robust -no_format_opt -error all

#
# If not defined, simply set to default
#

!IFNDEF HOST_TARGETCPU
HOST_TARGETCPU=$(TARGET_DIRECTORY)
# HOST_TARGET_DIRECTORY=$(TARGET_DIRECTORY)
# HOST_TARGET_DEFINES=$(TARGET_DEFINES)
# HOST_TOOLS=
!ENDIF
