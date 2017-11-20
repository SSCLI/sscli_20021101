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
if test "`echo ${PWD} | sed -e 's/ //g'`" != "${PWD}"
then
    echo "You cannot have spaces in the path to the SSCLI directory."
    return 1;
fi

perl -e "exit 1 if '${PWD}' =~ /[\x80-\xff]/;"
if test $? -eq 1
then
    echo "You cannot have high-bit characters in the path to the SSCLI directory."
    return 1
fi

if test X"$SHELL" = "X"
then 
	echo "The SHELL environment variable is not set -- checking \$0."
	SH=$(echo $0 | tr -d "-")
	SH=$(basename $SH)
else
	SH=$(basename $SHELL)
fi
export SH

if test X"$SH" = "Xcsh" || test X"$SH" = "Xtcsh"
then 
	echo "Your shell is [t]csh -- please source env.csh instead."
	return 1
fi

if test X"$ROTOR_DIR" != "X"
then
    echo "You cannot run env.csh more than once"
    return 1
fi

# Do a simple check to see if we're in the right directory.
if [ ! -d "${PWD}/pal" ] || [ ! -d "${PWD}/clr" ]
then
    echo "env.sh must be run from the root of the SSCLI directory."
    return 1
fi

# Set a few environment variables for the SSCLI build process.
# This file must be run from the root of the SSCLI distribution.
ROTOR_DIR="${PWD}"
DNAROOT="${ROTOR_DIR}/fx"
NTMAKEENV="${ROTOR_DIR}/rotorenv/bin"
PLATFORM_UNIX=1
FEATURE_PAL=1
CRT_INC_PATH="${ROTOR_DIR}/palrt/inc"
SDK_INC_PATH="${ROTOR_DIR}/pal"
export ROTOR_DIR
export DNAROOT
export NTMAKEENV
export PLATFORM_UNIX
export FEATURE_PAL
export CRT_INC_PATH
export SDK_INC_PATH

# This isn't a perfect test, but the likelihood of both /TMP and /ETC
# existing on a case-sensitive system is very, very low.
if [ -s /TMP ] && [ -s /ETC ]; then
    FEATURE_CASE_SENSITIVE_FS=0
else
    FEATURE_CASE_SENSITIVE_FS=1
fi

CSC_COMPILE_FLAGS=""

ARCHITECTURE=`uname -p`
if [ "$ARCHITECTURE" = "unknown" ]; then
    ARCHITECTURE=`uname -m`
fi

case $ARCHITECTURE in
i[3-6]86)
    PROCESSOR_ARCHITECTURE=x86
    TARGET_DIRECTORY=rotor_x86
    _TGTCPU=i386
    ROTOR_X86=1
    if [ `uname -s` != "FreeBSD" ]; then
        OTHER_X86=1
        export OTHER_X86
    fi
    GCC_VERSION_OUTPUT=`sh -c "gcc -v 2>&1 | sed -e 1\\!d"`
    if [ "$GCC_VERSION_OUTPUT" = "Using builtin specs." ]; then
        GCC_LIB_DIR=/usr/lib
    else
        GCC_LIB_DIR=`sh -c "gcc -v 2>&1 | sed -e 1\\!d -e 's/^Reading specs from //g' -e 's/\/specs//g'"`
    fi
    if [ -f "${GCC_LIB_DIR}/libgcc_eh.a" ]; then
        GCC_EH_LIB=${GCC_LIB_DIR}/libgcc_eh.a
        export GCC_EH_LIB
    fi
    PLATFORM=`uname -s | awk '{ print tolower($0) }'`
    export PLATFORM
    export GCC_LIB_DIR
    export ROTOR_X86
    ;;
"powerpc")
    PROCESSOR_ARCHITECTURE=ppc
    TARGET_DIRECTORY=ppc
    _TGTCPU=ppc
    PPC=1
    CSC_COMPILE_FLAGS="${CSC_COMPILE_FLAGS} /d:__APPLE__ /d:BIGENDIAN"
    export PPC
    # Build with precompiled headers
    if test X"$1" != "Xchecked" 
    then
        FEATURE_PRECOMPILED_HEADERS=1
        export FEATURE_PRECOMPILED_HEADERS
    fi
    ;;
"sparc")
    PROCESSOR_ARCHITECTURE=sparc
    TARGET_DIRECTORY=sparc
    _TGTCPU=sparc
    SPARC=1
    CSC_COMPILE_FLAGS="${CSC_COMPILE_FLAGS} /d:BIGENDIAN"
    GCC_VERSION_OUTPUT=`sh -c "gcc -v 2>&1 | sed -e 1\\!d"`
    if [ "$GCC_VERSION_OUTPUT" = "Using builtin specs." ]; then
        GCC_LIB_DIR=/usr/lib
    else
        GCC_LIB_DIR=`sh -c "gcc -v 2>&1 | sed -e 1\\!d -e 's/^Reading specs from //g' -e 's/\/specs//g'"`
    fi
    if [ -f "${GCC_LIB_DIR}/libgcc_eh.a" ]; then
        GCC_EH_LIB=${GCC_LIB_DIR}/libgcc_eh.a
        export GCC_EH_LIB
    fi
    export GCC_LIB_DIR
    export SPARC
    ;;
*)
    echo "`uname -p` is not a supported processor."
    exit 1
    ;;
esac
export PROCESSOR_ARCHITECTURE
export TARGET_DIRECTORY
export _TGTCPU
export FEATURE_CASE_SENSITIVE_FS

CORBASE="${ROTOR_DIR}/clr"
CORENV="${ROTOR_DIR}/rotorenv"
_NTROOT="${CORBASE}"
TARGETCORLIB="${CORBASE}/bin"
TARGETPATH="${CORBASE}/bin"
BUILD_DEFAULT="-iwe -nmake -i -a"
BUILD_DEFAULT_TARGETS="-dynamic ${TARGET_DIRECTORY}"
C_DEFINES="-DFEATURE_PAL -DPLATFORM_UNIX -DDEBUGGING_SUPPORTED"
C_DEFINES="${C_DEFINES} -DUSE_CDECL_HELPERS -DPAL_PORTABLE_SEH"
C_DEFINES="${C_DEFINES} -DFUSION_SUPPORTED -DPROFILING_SUPPORTED"
CSC_COMPILE_FLAGS="${CSC_COMPILE_FLAGS} /d:FEATURE_PAL /d:PLATFORM_UNIX"
DEBUGGING_SUPPORTED_BUILD=1
PROFILING_SUPPORTED_BUILD=1
SVR_WKS_DIRS=wks
CSC_NO_DOC=1

export CORBASE
export CORENV
export _NTROOT
export TARGETCORLIB
export TARGETPATH
export BUILD_DEFAULT
export BUILD_DEFAULT_TARGETS
export DEBUGGING_SUPPORTED_BUILD
export PROFILING_SUPPORTED_BUILD
export SVR_WKS_DIRS
export CSC_NO_DOC

if test X"$FEATURE_CASE_SENSITIVE_FS" = "X1"
then
    C_DEFINES="${C_DEFINES} -DFEATURE_CASE_SENSITIVE_FILESYSTEM"
    CSC_COMPILE_FLAGS="${CSC_COMPILE_FLAGS} /d:FEATURE_CASE_SENSITIVE_FILESYSTEM"
fi

if test X"$1" = "Xfree"
then
    echo "Free Environment"
    DDKBUILDENV=free
    C_DEFINES="${C_DEFINES} -DNTMAKEENV -DNDEBUG -DPERF_TRACKING"
    BUILD_ALT_DIR=r
    TARGETCOMPLUS="${ROTOR_DIR}/build/v1.${PROCESSOR_ARCHITECTURE}fre.rotor"
    NTDEBUG=ntsdnodbg
else if test X"$1" = "Xchecked"
then
    echo "Checked Environment"
    DDKBUILDENV=checked
    C_DEFINES="${C_DEFINES} -DNTMAKEENV -D_DEBUG"
    BUILD_ALT_DIR=d
    TARGETCOMPLUS="${ROTOR_DIR}/build/v1.${PROCESSOR_ARCHITECTURE}chk.rotor"
    NTDEBUG=ntsd
    MSC_OPTIMIZATION=
    export MSC_OPTIMIZATION
else
    echo "Fastchecked Environment"
    DDKBUILDENV=fastchecked
    C_DEFINES="${C_DEFINES} -DNTMAKEENV -D_DEBUG"
    BUILD_ALT_DIR=df
    TARGETCOMPLUS="${ROTOR_DIR}/build/v1.${PROCESSOR_ARCHITECTURE}fstchk.rotor"
    NTDEBUG=ntsd
fi
fi
 
export C_DEFINES
export CSC_COMPILE_FLAGS
export DDKBUILDENV
export BUILD_ALT_DIR
export TARGETCOMPLUS
export NTDEBUG

FUSION_LIB_PATH="${CORBASE}"/bin/${TARGET_DIRECTORY}/${DDKBUILDENV}
INTTOOLSTARGET="${TARGETCOMPLUS}/int_tools"

CORBUILDENV=${DDKBUILDENV}/
PATH="${PATH}:${TARGETCOMPLUS}:${TARGETCOMPLUS}/sdk/bin"
PATH="${PATH}:${TARGETCOMPLUS}/int_tools"
if test X"$LD_LIBRARY_PATH" = "X"
then
    LD_LIBRARY_PATH="${TARGETCOMPLUS}"
else
    LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${TARGETCOMPLUS}"
fi
LD_LIB_DIRS="-L${TARGETCOMPLUS}"

export FUSION_LIB_PATH
export INTTOOLSTARGET
export CORBUILDENV
export PATH
export LD_LIBRARY_PATH
export LD_LIB_DIRS
