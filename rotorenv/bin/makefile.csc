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
##################################################################################################
# Required:
# --------
# ASSEMBLYTITLE                     This is the name of the assembly
# ASSEMBLYDESC                      Description for the Assembly
# ASSEMBLYVERSION                   Version string
#
# EMBEDEDRESOURCE                   Embed COM+ resources in this assembly
# LINKRESOURCE                      Link COM+ resources to this assembly
#
# CSC_SOURCES                       List of Source files
# CSC_COMPILE_FLAGS                 Additional flags for preprocessing
# WIN32_RESOURCE_FILE               Resource file to compile and add to ceegen'ed output
# CSC_BASE                          Base the exe or dll at this address
# CSC_REBASE                        Rebase the exe or dll to this address
# IMPORTS                           Dll's to import you must specify
#
##################################################################################################


#
# Build.exe gives us some somewhat unintuitive environment variables
# to play with.  Resolve these into easier to understand targets.
#
CURRENT_PASS=
PASS0=0
PASS1=0
PASS2=0
!if "$(NOPASS0)" != "1"
CURRENT_PASS=Pass0
PASS0=1
!endif
!if "$(PASS0ONLY)" != "1" && "$(LINKONLY)" != "1"
CURRENT_PASS=Pass1
PASS1=1
!endif
!if "$(NOLINK)" != "1"
CURRENT_PASS=Pass2
PASS2=1
!endif

#
# Determine Type of Optimizations to use
#

USE_COMPILER_OPTIMIZATIONS=1
ENABLE_JIT_TRACKING=0

!if     "$(URTBLDENV)" == "chk"
USE_COMPILER_OPTIMIZATIONS=0
ENABLE_JIT_TRACKING=1
!elseif "$(URTBLDENV)" == "fstchk"
ENABLE_JIT_TRACKING=1
!endif


#
# Determine type of target link we are doing
#

#
# define COMPLUSTARGET
#

!if "$(TARGETTYPE)" == "PROGRAM"
TARGETEXT=exe
!elseif "$(TARGETTYPE)" == "DYNLINK"
TARGETEXT=dll
!else if "$(TARGETTYPE)" != ""
! ERROR Your .\sources. contains an unrecognized value for TARGETTYPE
!endif


COMPLUSLIB=

! if $(PASS1) && "$(TARGETTYPE)" == "DYNLINK" && defined(COMPLUS_LIB_IMPORTS)
COMPLUSLIB=$(TARGETPATH)\$(TARGETNAME).$(TARGETEXT)
! else if $(PASS2)
COMPLUSTARGET=$(TARGETPATH)\$(TARGETNAME).$(TARGETEXT)
! endif

!if "$(BUILD_COMPLUSTARGET_PASS0)" == "1"
COMPLUSTARGET=$(TARGETPATH)\$(TARGETNAME).$(TARGETEXT)
NTTARGETFILE0=$(COMPLUSTARGET) $(NTTARGETFILE0)
!endif


#
# Determine where the csc compiler can be found
#

!ifndef CSC_COMPILER
!if "$(FEATURE_PAL_VC7)" == "1" || "$(PLATFORM_UNIX)" == "1"
#Just use the ones from the path
CSC_COMPILER = csc
AL = al
!elseif "$(TARGETCOMPLUS)" != ""
CSC_COMPILER = $(NTMAKEENV)\devlkg\csc.exe
AL = $(NTMAKEENV)\devlkg\al.exe
!else
#Just use the ones from the path
CSC_COMPILER = csc
!endif
!endif

#
# Set a few globals
#

OFFICIAL_BUILD=0

OFFICIAL_BUILD=0

BETA=0

BETA=0


#
# Assume assembly is CLS compliant unless otherwise indicated in sources
#
!ifndef CLS_COMPLIANT
CLS_COMPLIANT=1
!endif

!INCLUDE $(NTMAKEENV)\strongnamekey.inc

!ifndef ASSEMBLY_KEY_FILE
! if "$(ASSEMBLY_KEY_TYPE)"=="ECMA"
ASSEMBLY_KEY_FILE    = $(CORBASE)\bin\ecmapublickey.snk
! else
! if "$(ASSEMBLY_KEY_TYPE)"=="MICROSOFT"
ASSEMBLY_KEY_FILE    = $(MICROSOFT_KEY_FILE)
! endif
! endif
!endif

!ifndef PLATFORM_UNIX
ASSEMBLY_KEY_FILE    = $(ASSEMBLY_KEY_FILE:\=\\)
!endif

#
# Start setting the CSharp flags
#

CSC_COMPILE_FLAGS=$(CSC_COMPILE_FLAGS)                \
                  /nowarn:1595                        \
                  /nowarn:649                         \
                  /nowarn:679                         \
                  /nologo                             \
                  /warn:4                             \
                  /fullpaths                          \
                  /checked-                           \
                  /codepage:1252                      \

!if "$(CSC_TREAT_WARNINGS_AS_ERRORS)" == "1"
CSC_COMPILE_FLAGS   =$(CSC_COMPILE_FLAGS) /warnaserror+
!else
CSC_COMPILE_FLAGS   =$(CSC_COMPILE_FLAGS) /warnaserror-
!endif




#
# Debugging information
#

!if $(PASS1)
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /debug-
!else
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /debug+
! if "$(ENABLE_JIT_TRACKING)" == "1"
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /debug:full
! else
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /debug:pdbonly
! endif
!endif

!if "$(USE_COMPILER_OPTIMIZATIONS)" == "1"
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /optimize+
!else
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /optimize-
!endif

!if "$(CSHARP_INCREMENTAL_COMPILE)" == "1"
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /incremental+
!else
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /incremental-
!endif

!if "$(DDKBUILDENV)" == "checked" || "$(DDKBUILDENV)" == "fastchecked"
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /d:_DEBUG /d:_LOGGING
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /D:DEBUG /D:DBG
!endif


#
# build COMPLUSIMPORTS and COMPLUSIMPORTSDEPS macro
#

!if $(PASS1) && defined(COMPLUS_LIB_IMPORTS)
COMPLUSIMPORTS=$(COMPLUS_LIB_IMPORTS)
!endif

#
# Convert semi-delimited list to space-delimited
#

COMPLUSIMPORTS=$(COMPLUSIMPORTS:;= )

#
# remove extraneous, leading, and trailing spaces
#

COMPLUSIMPORTS=$(COMPLUSIMPORTS: =^#^%)
COMPLUSIMPORTS=$(COMPLUSIMPORTS:^%^#=)
COMPLUSIMPORTS=$(COMPLUSIMPORTS:^#^%= )
COMPLUSIMPORTS=+++$(COMPLUSIMPORTS)+++
COMPLUSIMPORTS=$(COMPLUSIMPORTS:+++ =)
COMPLUSIMPORTS=$(COMPLUSIMPORTS: +++=)
COMPLUSIMPORTS=$(COMPLUSIMPORTS:+++=)

#
# This is the list of import dependencies
#

COMPLUSIMPORTSDEPS=$(COMPLUSIMPORTS)

#
# Add any imports to the Compile flags
#
!if "$(COMPLUSIMPORTS)" != ""
CSC_COMPILE_FLAGS= $(CSC_COMPILE_FLAGS) /R:$(COMPLUSIMPORTS: = /R:)
!endif

#
# Could also have the Imports setting set which already has the correct format
#
!ifdef IMPORTS
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) $(IMPORTS)
!endif

!ifndef BUILDING_MSCORLIB
CSC_COMPILE_FLAGS= $(CSC_COMPILE_FLAGS) /nostdlib+ /R:$(TARGETCOMPLUS)\mscorlib.dll
!endif


#
# we always want TRACE defined
#
CSC_COMPILE_FLAGS       = $(CSC_COMPILE_FLAGS) /D:TRACE

#
# Set whether to allow Unsafe
#
!if "$(CSHARP_ALLOW_UNSAFE)"=="1"
CSC_COMPILE_FLAGS=$(CSC_COMPILE_FLAGS) /unsafe+
!endif

#
# If we're in Pass 1, we just want to build a LIB
#
!if $(PASS1)
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /D:LIB
!endif


#
# Set the Target
#

DEFAULT_EXE_ADDRESS=0x00400000
DEFAULT_DLL_ADDRESS=0x10000000

!if "$(TARGETTYPE)" == "DYNLINK"
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /target:library
DEFAULT_ADDRESS     =$(DEFAULT_DLL_ADDRESS)
!elseif "$(TARGETTYPE)" == "PROGRAM"
!if "$(UMTYPE)" == "windows"
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /target:winexe
!elseif "$(UMTYPE)" == "module"
DEFAULT_ADDRESS     =$(DEFAULT_EXE_ADDRESS)
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /target:module
!else
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /target:exe
DEFAULT_ADDRESS     =$(DEFAULT_EXE_ADDRESS)
!endif
!else
!    error TARGETTYPE must be DYNLINK or PROGRAM.
!endif

#
# Set various Assembly info
#
!ifdef ASSEMBLYTITLE
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /a+ /a.title:$(ASSEMBLYTITLE) /a.config:$(DDKBUILDENV)
!endif

!ifdef ASSEMBLYDESC
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /a.description:$(ASSEMBLYDESC)
!endif

!ifdef ASSEMBLYVERSION
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /a+ /a.version:$(ASSEMBLYVERSION)
!endif


!ifdef CSC_SET_VERSION
CSC_COMPILE_FLAGS=$(CSC_COMPILE_FLAGS) $(O)\version.cs
# Whenever we increment our version number, rebuild our managed apps that
# hard-code that version number into their source and their assembly version.
# Also add a dependency on the preprocessed version.pp file.
VERSION_FILES = $(CORBASE)\src\inc\version\__file__.ver $(CORBASE)\src\inc\version\__official__.ver $(CORBASE)\src\inc\version\__private__.ver $(CORBASE)\src\inc\version\version.pp
!endif #CSC_SET_VERSION

# binplace command.
!if "$(TARGETCOMPLUS)" != ""
BINPLACE_CMD=@binplace -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $@
!endif

#
# to support cross-compile on ia64
#
!if "$(_TGTCPUTYPE)" == "IA64"
RESGENPATH=$(INTTOOLSTARGET)
!else
RESGENPATH=$(TARGETPATH)
!endif 

RESGEN=$(INTTOOLSTARGET)\internalresgen

#
# Resources
#

!ifdef EMBEDEDRESOURCE
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /res:$(EMBEDEDRESOURCE)
!endif


!ifdef LINKRESOURCE
CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) /linkres:$(LINKRESOURCE)
!endif

#
# Pass in the Win32_Resourcefile
#



COMPLUSRESDEPS=

#
# Native resources are present in the OBJECTS macro defined in obj?\_objects.mac
#

WIN32RES=
!if "$(OBJECTS)" != ""
WIN32RES=$(OBJECTS)
!else if "$(COMPLUSRES)" != ""
WIN32RES=$(COMPLUSRES)
!endif


# Menu resources - build before native resources
!if "$(MENU_DEPS)" != ""
USER_INCLUDES=$(O)
!endif

# resx resources
!if "$(RESX_DEPS)" != ""
COMPLUSRESDEPS=$(COMPLUSRESDEPS) $(RESX_DEPS)
!endif

COMPLUSRESDEPS=$(COMPLUSRESDEPS) $(OBJECTS)

#
# Managed resources
#

!if "$(RESGEN_SOURCE)" != ""
RESGEN_SOURCE_NOPATH=$(RESGEN_SOURCE:..\=)
RESGEN_RESOUTPUT=$(RESGEN_SOURCE_NOPATH:.txt=.resources)
COMPLUSRESDEPS=$(COMPLUSRESDEPS) $(O)\$(RESGEN_RESOUTPUT)

! if "$(RESGEN_CLASS)" != "" && "$(CSC_SOURCES)" != "" 
RESGEN_CSOUTPUT=$(RESGEN_SOURCE_NOPATH:.txt=.cs)
CSC_SOURCES      = $(O)\$(RESGEN_CSOUTPUT) \
                      $(CSC_SOURCES)
! endif

CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /res:$(O)\$(RESGEN_RESOUTPUT),$(RESGEN_RESOUTPUT)
!endif

#
# Set the file version type (URTVFT) based on the target
#

!if "$(TARGETTYPE)" == "PROGRAM"
URTVFT=VFT_APP
!elseif "$(TARGETTYPE)" == "DYNLINK"
URTVFT=VFT_DLL
!elseif "$(TARGETTYPE)" == "PROGRAM" || "$(TARGETTYPE)" == "PROGLIB"
URTVFT=VFT_APP
!elseif "$(TARGETTYPE)" == "DYNLINK" 
URTVFT=VFT_DLL
!else
URTVFT=VFT_UNKNOWN
!endif

!if "$(URTVFT)" != "VFT_UNKNOWN"
C_DEFINES = $(C_DEFINES) -DURTVFT=$(URTVFT)
!endif

!if "$(URTVFT)" == "VFT_DLL" || "$(URTVFT)" == "VFT_APP"
!ifndef COMPONENT_VER_INTERNALNAME_STR
COMPONENT_VER_INTERNALNAME_STR=$(TARGETNAME).$(TARGETEXT)
!endif
C_DEFINES = $(C_DEFINES) "-DCOMPONENT_VER_INTERNALNAME_STR=$(COMPONENT_VER_INTERNALNAME_STR)"
!endif

!ifndef URTBLDENV_FRIENDLY
URTBLDENV_FRIENDLY=$(DDKBUILDENV)
!endif

C_DEFINES = $(C_DEFINES) -DOFFICIAL_BUILD=$(OFFICIAL_BUILD) -DBETA=$(BETA) 
C_DEFINES = $(C_DEFINES) "-DFX_VER_PRIVATEBUILD_STR=$(COMPUTERNAME)"
C_DEFINES = $(C_DEFINES) "-DURTBLDENV_FRIENDLY=$(URTBLDENV_FRIENDLY)"
C_DEFINES = $(C_DEFINES) -DASPNET_PRODUCT_IS_REDIST=1

!if "$(CLS_COMPLIANT)" == "1"
C_DEFINES      = $(C_DEFINES) -DASSEMBLY_ATTRIBUTE_CLS_COMPLIANT=true
!else
C_DEFINES      = $(C_DEFINES) -DASSEMBLY_ATTRIBUTE_CLS_COMPLIANT=false
!endif

! if "$(COM_VISIBLE)" == "1"
C_DEFINES      = $(C_DEFINES) -DASSEMBLY_ATTRIBUTE_COM_VISIBLE=true
! else
C_DEFINES      = $(C_DEFINES) -DASSEMBLY_ATTRIBUTE_COM_VISIBLE=false
! endif

C_DEFINES = $(C_DEFINES) "-DASSEMBLY_KEY_FILE=$(ASSEMBLY_KEY_FILE)" 

C_DEFINES = $(C_DEFINES) -DMICROSOFT_KEY_TOKEN=$(MICROSOFT_KEY_TOKEN) -DMICROSOFT_KEY_FULL=$(MICROSOFT_KEY_FULL)

#
# Assembly Strong Signing
#

!ifndef SIGN_ASSEMBLY
SIGN_ASSEMBLY=0
!endif

!ifndef DELAY_SIGN
DELAY_SIGN=1
!endif

!ifndef ALLOW_PARTIALTRUSTCALLS
ALLOW_PARTIALTRUSTCALLS=0
!endif

C_DEFINES = $(C_DEFINES) "-DSIGN_ASSEMBLY=$(SIGN_ASSEMBLY)" "-DALLOW_PARTIALTRUSTCALLS=$(ALLOW_PARTIALTRUSTCALLS)"

!if "$(RESGEN_SOURCE2)" != ""
RESGEN_SOURCE2_NOPATH=$(RESGEN_SOURCE2:..\=)
RESGEN_RESOUTPUT2=$(RESGEN_SOURCE2_NOPATH:.txt=.resources)
COMPLUSRESDEPS=$(COMPLUSRESDEPS) $(O)\$(RESGEN_RESOUTPUT2)

! if "$(RESGEN_CLASS2)" != "" && "$(CSC_SOURCES)" != "" 
RESGEN_CSOUTPUT2=$(RESGEN_SOURCE2_NOPATH:.txt=.cs)
CSC_SOURCES      = $(O)\$(RESGEN_CSOUTPUT2) \
                      $(CSC_SOURCES)
! endif

CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /res:$(O)\$(RESGEN_RESOUTPUT2),$(RESGEN_RESOUTPUT2)
!endif

!if "$(RESOURCE_FILES)" != ""
#
# remove extraneous, leading, and trailing spaces
#
RESOURCE_FILES=$(RESOURCE_FILES: =^#^%)
RESOURCE_FILES=$(RESOURCE_FILES:^%^#=)
RESOURCE_FILES=$(RESOURCE_FILES:^#^%= )
RESOURCE_FILES=+++$(RESOURCE_FILES)+++
RESOURCE_FILES=$(RESOURCE_FILES:+++ =)
RESOURCE_FILES=$(RESOURCE_FILES: +++=)
RESOURCE_FILES=$(RESOURCE_FILES:+++=)
!endif

!if "$(RESOURCE_FILES)" != ""
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /res:$(RESOURCE_FILES: = /res:)
!endif

!if "$(LINKRESOURCE_FILES)" != ""
#
# remove extraneous, leading, and trailing spaces
#
LINKRESOURCE_FILES=$(LINKRESOURCE_FILES: =^#^%)
LINKRESOURCE_FILES=$(LINKRESOURCE_FILES:^%^#=)
LINKRESOURCE_FILES=$(LINKRESOURCE_FILES:^#^%= )
LINKRESOURCE_FILES=+++$(LINKRESOURCE_FILES)+++
LINKRESOURCE_FILES=$(LINKRESOURCE_FILES:+++ =)
LINKRESOURCE_FILES=$(LINKRESOURCE_FILES: +++=)
LINKRESOURCE_FILES=$(LINKRESOURCE_FILES:+++=)
!endif

!if "$(LINKRESOURCE_FILES)" != ""
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS) /linkres:$(LINKRESOURCE_FILES: = /linkres:)
!endif

#
# remove extraneous spaces from CSC_COMPILE_FALGS
#
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS: =^#^%)
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS:^%^#=)
CSC_COMPILE_FLAGS         =$(CSC_COMPILE_FLAGS:^#^%= )

#
# Add Any AssemblyAttributes files
#
!ifdef INCLUDE_ASSEMBLY_ATTRIBUTES
! if "$(CSC_SOURCES)" != "" 
!  if "$(COMPLUSLIB)" != ""
CSC_SOURCES      = $(O)\AssemblyLibAttributes.cs \
                    $(O)\AssemblyLibRefs.cs \
                    $(CSC_SOURCES)
!  else
CSC_SOURCES      = $(O)\AssemblyAttributes.cs \
                    $(O)\AssemblyRefs.cs \
                    $(CSC_SOURCES)
!  endif
! endif
!endif


# remove extraneous spaces
CSC_SOURCES=$(CSC_SOURCES: =^#^%)
CSC_SOURCES=$(CSC_SOURCES:^%^#=)
CSC_SOURCES=$(CSC_SOURCES:^#^%= )

CSC_RSP=$(O)\csc.rsp

##################################################################################################
# Production rules.
##################################################################################################

#
# Cleanup rules.
#

clean:
    -$(DELETER) $(COMPLUSTARGET)

#
# Build a managed resource from a txt file
#

.SUFFIXES: .txt .resources

.txt.resources:
    @echo Running $(RESGEN) on $@
    $(RESGEN) $< $@

{}.txt{$O\}.resources:
    @echo Running $(RESGEN) on $@
    $(RESGEN) $< $@

{..\}.txt{$O\}.resources:
    @echo Running $(RESGEN) on $@
    $(RESGEN) $< $@

{$O\}.txt{$O\}.resources:
    @echo Running $(RESGEN) on $@
    $(RESGEN) $< $@


#
# Build .cs file from cspp files (CS preprocess file)
#

.SUFFIXES: .cspp .cs


!ifdef PLATFORM_UNIX
{..\}.cspp{}.cs:
    $(C_PREPROCESSOR_NAME) -x c -E -P $(USE_FC) -DCSC_INVOKED=1 $(GLOBAL_C_FLAGS) $< > $@

{}.cspp{}.cs:
    $(C_PREPROCESSOR_NAME) -x c -E -P $(USE_FC) -DCSC_INVOKED=1 $(GLOBAL_C_FLAGS) $< > $@

{$O\}.cspp{$O\}.cs:
    $(C_PREPROCESSOR_NAME) -x c -E -P $(USE_FC) -DCSC_INVOKED=1 $(GLOBAL_C_FLAGS) $< > $@

{}.cspp{$O\}.cs:
    $(C_PREPROCESSOR_NAME) -x c -E -P $(USE_FC) -DCSC_INVOKED=1 $(GLOBAL_C_FLAGS) $< > $@

{..\}.cspp{$O\}.cs:
    $(C_PREPROCESSOR_NAME) -x c -E -P $(USE_FC) -DCSC_INVOKED=1 $(GLOBAL_C_FLAGS) $< > $@

{$(TARGET_DIRECTORY)\}.cspp{$O\}.cs:
    $(C_PREPROCESSOR_NAME) -x c -E -P $(USE_FC) -DCSC_INVOKED=1 $(GLOBAL_C_FLAGS) $< > $@

!else

{..\}.cspp{}.cs:
    @$(TYPE_COMMAND) <<$(ECHO_RSP)
$(ECHO_CXX_MSG)
<<NOKEEP
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $(USE_FC) /DCSC_INVOKED=1 $< > $@
$(GLOBAL_C_FLAGS: =
)
<<$(KEEPFILES)

{}.cspp{}.cs:
    @$(TYPE_COMMAND) <<$(ECHO_RSP)
$(ECHO_CXX_MSG)
<<NOKEEP
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $(USE_FC) /DCSC_INVOKED=1 $< > $@
$(GLOBAL_C_FLAGS: =
)
<<$(KEEPFILES)

{$O\}.cspp{$O\}.cs:
    @$(TYPE_COMMAND) <<$(ECHO_RSP)
$(ECHO_CXX_MSG)
<<NOKEEP
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $(USE_FC) /DCSC_INVOKED=1 $< > $@
$(GLOBAL_C_FLAGS: =
)
<<$(KEEPFILES)

{}.cspp{$O\}.cs:
    @$(TYPE_COMMAND) <<$(ECHO_RSP)
$(ECHO_CXX_MSG)
<<NOKEEP
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $(USE_FC) /DCSC_INVOKED=1 $< > $@
$(GLOBAL_C_FLAGS: =
)
<<$(KEEPFILES)

{..\}.cspp{$O\}.cs:
    @$(TYPE_COMMAND) <<$(ECHO_RSP)
$(ECHO_CXX_MSG)
<<NOKEEP
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $(USE_FC) /DCSC_INVOKED=1 $< > $@
$(GLOBAL_C_FLAGS: =
)
<<$(KEEPFILES)

{$(TARGET_DIRECTORY)\}.cspp{$O\}.cs:
    @$(TYPE_COMMAND) <<$(ECHO_RSP)
$(ECHO_CXX_MSG)
<<NOKEEP
    $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $(USE_FC) /DCSC_INVOKED=1 $< > $@
$(GLOBAL_C_FLAGS: =
)
<<$(KEEPFILES)
!endif


!ifdef REQUIRES_SETUP_PHASE
csc_target: SETUP $(COMPLUSTARGET)
!else
csc_target: $(COMPLUSTARGET)
!endif


#
# Build rules for native rc files
#

.SUFFIXES: .rc .res

{.\}.rc{$O\}.res:
    $(TARGETCOMPLUS)\$(RC_NAME) -fo $(TARGETPATH)\$(TARGETNAME).satellite $(CDEFINERC) $(INCPATH0) $<
    $(TYPE_COMMAND) $(CORBASE)\..\tools\bin\$(EMPTY_RES) >$@
!ifdef PLATFORM_UNIX
    if [ -f $(TARGETPATH)\$(TARGETNAME).satellite ]; then $(BINPLACE_NAME) -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $(TARGETPATH)\$(TARGETNAME).satellite; fi
!else
    if exist $(TARGETPATH)\$(TARGETNAME).satellite $(BINPLACE_NAME) -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $(TARGETPATH)\$(TARGETNAME).satellite
!endif

# this is a temp file to force a rebuild with the -c switch
$(O)\holder.foo :
    echo this is a test >$(O)\holder.foo


#
# Build the SR file from a txt file
#

.SUFFIXES: .txt .sr

!if "$(RESGEN_CLASS)" != ""

RESFILE=$(RESGEN_SOURCE:..\=)
RESFILE=$(RESFILE:.txt=)

$(O)\$(RESGEN_CSOUTPUT) : $(RESGEN_SOURCE)
    $(PERL) -w $(NTMAKEENV)\gensr.pl $(RESGEN_SOURCE) $(O)\$(RESGEN_CSOUTPUT:..\=) $(RESGEN_CLASS) $(RESFILE)

!endif

!if "$(RESGEN_CLASS2)" != ""

$(O)\$(RESGEN_CSOUTPUT2) : $(RESGEN_SOURCE2)
    $(PERL) -w $(NTMAKEENV)\gensr.pl $(RESGEN_SOURCE2) $(O)\$(RESGEN_CSOUTPUT2) $(RESGEN_CLASS2) $(RESGEN_SOURCE2:.txt=)

!endif

#####################################
# CSHARP Assembly attributes specific
#####################################
        
$(O)\AssemblyLibAttributes.cspp: $(NTMAKEENV)\assemblyattributes.cspp
        $(COPY_NAME) $** $@

$(O)\AssemblyAttributes.cspp: $(NTMAKEENV)\assemblyattributes.cspp
        $(COPY_NAME) $** $@

$(O)\AssemblyLibRefs.cspp: $(NTMAKEENV)\assemblyrefs.cspp
        $(COPY_NAME) $** $@

$(O)\AssemblyRefs.cspp: $(NTMAKEENV)\assemblyrefs.cspp
        $(COPY_NAME) $** $@

#
# Build a COM+ CSharp "library"
#
!if "$(CSC_SOURCES)" != "" && "$(COMPLUSLIB)" != ""
$(COMPLUSLIB) : $(CSC_SOURCES) $(COMPLUSRESDEPS) $(COMPLUSIMPORTSDEPS) $(LINKRESOURCE_FILES)
    @echo Compiling $(COMPLUSLIB) 
#
# Generate the rsp file
#
    perl $(NTMAKEENV)\gencscrsp.pl $(CSC_RSP).lib "$(CSC_COMPILE_FLAGS) $(CSC_SOURCES)" $(CSC_BASE)
#
# now compile
#
    $(CSC_COMPILER) /noconfig /out:$@ @$(CSC_RSP).lib
!endif

#
# Build a COM+ CSharp binary
#

!ifndef BUILDING_MSCORLIB

!if "$(CSC_SOURCES)" != "" && "$(COMPLUSTARGET)" != ""

!ifdef WIN32_RESOURCE_FILE
$(COMPLUSTARGET): $(O)\$(WIN32_RESOURCE_FILE:.rc=.res) $(CSC_SOURCES) $(MANAGED_RESOURCES) $(O)\holder.foo $(CORLIBS)\mscorlib.dll sources $(VERSION_FILES) $(COMPLUSRESDEPS) 
!else
$(COMPLUSTARGET): $(CSC_SOURCES) $(O)\holder.foo $(CORLIBS)\mscorlib.dll sources $(VERSION_FILES) $(COMPLUSRESDEPS) 
!endif #WIN32_RESOURCE_FILE
    -$(DELETER) $(TARGETPATH)\$(TARGETNAME)
!ifdef CSC_SET_VERSION
    perl $(NTMAKEENV)\keylocationex.pl $(ASSEMBLY_KEY_FILE) > $(O)\KeyDefine.h
!ifdef PLATFORM_UNIX
    $(CC_NAME) -x c++ -E -C -P -nostdinc -include $(ROTOR_DIR)\palrt\inc\sscli_version.h -include $(CORBASE)\src\inc\version\__file__.ver -include $(O)\KeyDefine.h -DCSC_INCLUDE -DFEATURE_PAL $(CORBASE)\src\inc\version\version.pp > $(O)\version.cs
!else
    cl /EP /C /FI$(ROTOR_DIR)\palrt\inc\sscli_version.h /FI$(CORBASE)\src\inc\version\__file__.ver /I$(O) /FI KeyDefine.h /DCSC_INCLUDE /DFEATURE_PAL $(CORBASE)\src\inc\Version\Version.pp > $(O)\Version.cs
!endif
!endif

    perl $(NTMAKEENV)\gencscrsp.pl $(CSC_RSP) "$(CSC_COMPILE_FLAGS) $(CSC_SOURCES)" $(CSC_BASE)
#
# now compile
#
    $(CSC_COMPILER) /noconfig /out:$@ @$(CSC_RSP)
!if "$(DELAY_SIGN)" != "1"
! if "$(ASSEMBLY_KEY_TYPE)"=="ECMA"
    sn -R $@ $(MICROSOFT_KEY_FILE)
!else
    sn -R $@ $(ASSEMBLY_KEY_FILE)
!endif
!endif
    $(BINPLACE_CMD)

!endif #COMPLUSTARGET
!endif #BUILDING_MSCORLIB

