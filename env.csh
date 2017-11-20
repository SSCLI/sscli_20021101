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
if ("`echo ${PWD} | sed -e 's/ //g'`" != "${PWD}") then
    echo "You cannot have spaces in the path to the SSCLI directory."
    exit 1
endif

perl -e "exit 1 if '${PWD}' =~ /[\x80-\xff]/;"
if ($? == "1") then
    echo "You cannot have high-bit characters in the path to the SSCLI directory."
    exit 1
endif

if ($shell == "") then
    echo "The SHELL environment variable is not set -- checking \$0."
    setenv SH `echo $0 | tr -d "-"`
    setenv SH `basename $SH`
else
    setenv SH `basename $shell`
endif

if ($SH == "sh" || $SH == "bash" || $SH == "ksh") then
    echo "Your shell is [ba|k]sh -- please dot source env.sh instead"
    exit 1
endif

if ($?ROTOR_DIR == "1") then
    echo "You cannot run env.csh more than once"
    exit 1
endif

# Do a simple check to see if we're in the right directory.
if ((! -d "${PWD}/pal") || (! -d "${PWD}/clr")) then
    echo "env.csh must be run from the root of the SSCLI directory."
    exit 1
endif

# Set a few environment variables for the SSCLI build process.
# This file must be run from the root of the SSCLI directory.
setenv ROTOR_DIR "${PWD}"
setenv DNAROOT "${ROTOR_DIR}/fx"
setenv NTMAKEENV "${ROTOR_DIR}/rotorenv/bin"
setenv PLATFORM_UNIX 1
setenv FEATURE_PAL 1
setenv CRT_INC_PATH "${PWD}/palrt/inc"
setenv SDK_INC_PATH "${ROTOR_DIR}/pal"

# This isn't a perfect test, but the likelihood of both /TMP and /ETC
# existing on a case-sensitive system is very, very low.
if ((-s /TMP) && (-s /ETC)) then
    setenv FEATURE_CASE_SENSITIVE_FS 0
else
    setenv FEATURE_CASE_SENSITIVE_FS 1
endif

setenv CSC_COMPILE_FLAGS ""

set ARCHITECTURE = `uname -p`
if ("$ARCHITECTURE" == "unknown") then
    set ARCHITECTURE = `uname -m`
endif

switch ($ARCHITECTURE)
case "i686":
case "i386":
    setenv PROCESSOR_ARCHITECTURE x86
    setenv TARGET_DIRECTORY rotor_x86
    setenv _TGTCPU i386
    setenv ROTOR_X86 1
    if (`uname -s` != "FreeBSD") then
    	setenv OTHER_X86 1
    endif
    set GCC_VERSION_OUTPUT = `sh -c "gcc -v 2>&1 | sed -e 1\\!d"`
    if ( "$GCC_VERSION_OUTPUT" == "Using builtin specs." ) then
        setenv GCC_LIB_DIR /usr/lib
    else
        setenv GCC_LIB_DIR `sh -c "gcc -v 2>&1 | sed -e 1\\!d -e 's/^Reading specs from //g' -e 's/\/specs//g'"`
    endif
    if ( -f "${GCC_LIB_DIR}/libgcc_eh.a" ) then
        setenv GCC_EH_LIB  ${GCC_LIB_DIR}/libgcc_eh.a
    endif
    setenv PLATFORM `uname -s | awk '{ print tolower($0) }'`
    breaksw
case "powerpc":
    setenv PROCESSOR_ARCHITECTURE ppc
    setenv TARGET_DIRECTORY ppc
    setenv _TGTCPU ppc
    setenv PPC 1
    setenv CSC_COMPILE_FLAGS "${CSC_COMPILE_FLAGS} /d:__APPLE__ /d:BIGENDIAN"
    if ($1 != "checked") then
        # Build with precompiled headers
        setenv FEATURE_PRECOMPILED_HEADERS 1
    endif
    breaksw
case "sparc":
    setenv PROCESSOR_ARCHITECTURE sparc
    setenv TARGET_DIRECTORY sparc
    setenv _TGTCPU sparc
    setenv SPARC 1
    set GCC_VERSION_OUTPUT = `sh -c "gcc -v 2>&1 | sed -e 1\\!d"`
    if ( "$GCC_VERSION_OUTPUT" == "Using builtin specs." ) then
        setenv GCC_LIB_DIR /usr/lib
    else
        setenv GCC_LIB_DIR `sh -c "gcc -v 2>&1 | sed -e 1\\!d -e 's/^Reading specs from //g' -e 's/\/specs//g'"`
    endif
    if ( -f "${GCC_LIB_DIR}/libgcc_eh.a" ) then
        setenv GCC_EH_LIB  ${GCC_LIB_DIR}/libgcc_eh.a
    endif
    setenv CSC_COMPILE_FLAGS "${CSC_COMPILE_FLAGS} /d:BIGENDIAN"
    breaksw
default:
    echo "$ARCHITECTURE is not a supported processor."
    exit 1
    breaksw
endsw

setenv CORBASE "${ROTOR_DIR}/clr"
setenv CORENV "${ROTOR_DIR}/rotorenv"
setenv _NTROOT "${CORBASE}"
setenv TARGETCORLIB    "${CORBASE}/bin"
setenv TARGETPATH      "${CORBASE}/bin"
setenv BUILD_DEFAULT    "-iwe -nmake -i -a"
setenv BUILD_DEFAULT_TARGETS "-dynamic ${TARGET_DIRECTORY}"
setenv C_DEFINES    "-DFEATURE_PAL -DPLATFORM_UNIX -DDEBUGGING_SUPPORTED"
setenv C_DEFINES    "${C_DEFINES} -DUSE_CDECL_HELPERS -DPAL_PORTABLE_SEH"
setenv C_DEFINES    "${C_DEFINES} -DFUSION_SUPPORTED -DPROFILING_SUPPORTED"
setenv CSC_COMPILE_FLAGS    "${CSC_COMPILE_FLAGS} /d:FEATURE_PAL /d:PLATFORM_UNIX"
setenv DEBUGGING_SUPPORTED_BUILD    1
setenv PROFILING_SUPPORTED_BUILD    1
setenv SVR_WKS_DIRS     wks
setenv CSC_NO_DOC 1

if ($FEATURE_CASE_SENSITIVE_FS == "1") then
setenv C_DEFINES            "${C_DEFINES} -DFEATURE_CASE_SENSITIVE_FILESYSTEM"
setenv CSC_COMPILE_FLAGS    "${CSC_COMPILE_FLAGS} /d:FEATURE_CASE_SENSITIVE_FILESYSTEM"
endif

if ($1 == "free") then
echo "Free Environment"
setenv DDKBUILDENV     free
setenv C_DEFINES       "${C_DEFINES} -DNTMAKEENV -DNDEBUG -DPERF_TRACKING"
setenv BUILD_ALT_DIR   r
setenv TARGETCOMPLUS   "${ROTOR_DIR}/build/v1.${PROCESSOR_ARCHITECTURE}fre.rotor"
setenv NTDEBUG         ntsdnodbg
else if ($1 == "checked") then
echo "Checked Environment"
setenv DDKBUILDENV     checked
setenv C_DEFINES       "${C_DEFINES} -DNTMAKEENV -D_DEBUG"
setenv BUILD_ALT_DIR   d
setenv TARGETCOMPLUS   "${ROTOR_DIR}/build/v1.${PROCESSOR_ARCHITECTURE}chk.rotor"
setenv NTDEBUG         ntsd
setenv MSC_OPTIMIZATION
else
echo "Fastchecked Environment"
setenv DDKBUILDENV     fastchecked
setenv C_DEFINES       "${C_DEFINES} -DNTMAKEENV -D_DEBUG"
setenv BUILD_ALT_DIR   df
setenv TARGETCOMPLUS   "${ROTOR_DIR}/build/v1.${PROCESSOR_ARCHITECTURE}fstchk.rotor"
setenv NTDEBUG         ntsd
endif

setenv CORBUILDENV     ${DDKBUILDENV}/
setenv FUSION_LIB_PATH  "${CORBASE}"/bin/${TARGET_DIRECTORY}/${DDKBUILDENV}
setenv INTTOOLSTARGET "${TARGETCOMPLUS}/int_tools"

setenv PATH "${PATH}:${TARGETCOMPLUS}:${TARGETCOMPLUS}/sdk/bin"
setenv PATH "${PATH}:${TARGETCOMPLUS}/int_tools"
if ($?LD_LIBRARY_PATH == "1") then
setenv LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:${TARGETCOMPLUS}"
else
setenv LD_LIBRARY_PATH "${TARGETCOMPLUS}"
endif
setenv LD_LIB_DIRS "-L${TARGETCOMPLUS}"
