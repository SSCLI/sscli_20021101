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
!INCLUDE $(NTMAKEENV)\makefile.csc


!if "$(PLATFORM_UNIX)" != "1"
MSCOREE_TARGET=$(TARGETPATH)\sscoree.dll
!else
MSCOREE_TARGET=$(TARGETPATH)\libsscoree.$(DYNLIB_SUFFIX)
!endif

!ifndef _TGTCPUTYPE
_TGTCPUTYPE=$(PROCESSOR_ARCHITECTURE)
!endif


####################################################################################
# Include extra source files specifying the assembly version and strong name as
# custom attributes.

$(O)\Version.cs: version.pp $(CORBASE)\src\inc\version\__file__.ver
        perl $(NTMAKEENV)\keylocationex.pl $(ASSEMBLY_KEY_FILE) > $(O)\KeyDefine.h

!if "$(PLATFORM_UNIX)" != "1"
        cl /EP /C /FI $(ROTOR_DIR)\palrt\inc\sscli_version.h /FI ..\inc\version\__file__.ver /FI $(O)\KeyDefine.h $(C_DEFINES) /DCSC_INCLUDE Version.pp > $(O)\Version.cs
!else
        $(CC_NAME) -x c++ -E -C -P -nostdinc -include $(ROTOR_DIR)/palrt/inc/sscli_version.h -include ../inc/version/__file__.ver -include $(O)\KeyDefine.h $(C_DEFINES) -DCSC_INCLUDE version.pp > $(O)/Version.cs
!endif


$(TARGETCOMPLUS)\config\machine.config: $(CORBASE)\bin\machine.rotor.config
        -$(MD) $(TARGETCOMPLUS)\config
        $(COPY_NAME) $(CORBASE)\bin\machine.rotor.config $(TARGETCOMPLUS)\config\machine.config


####################################################################################
# Include extra source files specifying the assembly version and strong name as

$(O)\AppDomainSetup.cs: system\appdomainsetup.cs
        perl $(NTMAKEENV)\finddefines.pl ..\inc\fusion.h > $(O)\FDefines.h
        perl $(NTMAKEENV)\finddefines.pl ..\inc\fusionpriv.h > $(O)\FPrivDefines.h
        perl $(NTMAKEENV)\finddefines.pl ..\inc\fusionsetup.h > $(O)\FusionSetup.h
!if "$(PLATFORM_UNIX)" != "1"
        cl /EP /C /I. /FI $(O)\FPrivDefines.h /FI $(O)\FDefines.h /FI $(O)\FusionSetup.h $(C_DEFINES) system\AppDomainSetup.cs > $(O)\AppDomainSetup.cs
!else
        $(CC_NAME) -x c++ -E -C -P -nostdinc -I. -include $(O)\FPrivDefines.h -include $(O)\FDefines.h -include $(O)\FusionSetup.h $(C_DEFINES) system\appdomainsetup.cs > $(O)\AppDomainSetup.cs
        perl -p -i -e "s/^# /#line /;" $(O)/AppDomainSetup.cs
!endif

####################################################################################
#
# Tools Stuff.
#
####################################################################################

RESGEN_PATH = $(CORBASE)\src\tools\internalresgen
RESGEN_SRC = $(RESGEN_PATH)\internalresgen.cpp
!ifdef PLATFORM_UNIX
RESGEN_TARGET = $(TARGETPATH)\internalresgen
!else
RESGEN_TARGET = $(TARGETPATH)\internalresgen.exe
!endif

$(RESGEN_TARGET): $(RESGEN_SRC) $(MSCOREE_TARGET)
    -$(DELETER) $(RESGEN_TARGET)
    cd $(RESGEN_PATH)
    build -c
    cd $(CSC_SOURCE_DIR)

SN_PATH = $(CORBASE)\src\tools\strongname
SN_SRC = $(SN_PATH)\main.cpp
!if "$(PLATFORM_UNIX)" != "1"
SN_TARGET = $(TARGETPATH)\sn.exe
!else
SN_TARGET = $(TARGETPATH)\sn
!endif

$(SN_TARGET): $(SN_SRC) $(MSCOREE_TARGET)
    -$(DELETER) $(SN_TARGET)
    cd $(SN_PATH)
    build -c
    cd $(CSC_SOURCE_DIR)


####################################################################################
#
# This rule is always run and it is run before $(COMPLUSTARGET):: is run.
#
####################################################################################
SETUP: \
    $(RESGEN_TARGET) \
    $(O)\AppDomainSetup.cs \
    $(O)\Version.cs
   -$(DELETER) $(CSC_SECURITY_DB).raw
!if "$(_TGTCPUTYPE)" == "x86"
    $(TARGETPATH)\internalresgen $(CSC_SOURCE_DIR)\resources.txt $(TARGETPATH)\mscorlib.resources
!else
    $(INTTOOLSTARGET)\internalresgen $(CSC_SOURCE_DIR)\resources.txt $(TARGETPATH)\mscorlib.resources
!endif


####################################################################################
# PHASE 1 pass.
#   - Create a C# file containing the version that will be added in during the link 
#   - Create a C# file containing defines used in the runtime
#   - Copy the machine.config file
#   - create a config file for the compilers
####################################################################################
prep_target: \
    $(TARGETCOMPLUS)\config\machine.config \

####################################################################################

SECDBEDIT=clix $(TARGETCOMPLUS)/dump/secdbedit.exe

!if "$(BIGENDIAN)" == "1"
NLPSUFFIX=be.nlp
!else
NLPSUFFIX=.nlp
!endif
NLPDIR=$(CSC_SOURCE_DIR)\system\globalization\tables

####################################################################################
#
# This rule generates mscorlib.dll. It is important to add the source files that
# make up the dependents such as ResGen and CustomMarshalers so that they can be
# built if their source files change.
#
####################################################################################
!if "$(COMPLUSTARGET)" != ""
$(COMPLUSTARGET): $(CSC_SOURCES) $(CSC_SOURCE_DIR)\resources.txt $(SN_TARGET) $(O)\holder.foo $(O)\$(WIN32_RESOURCE_FILE:.rc=.res) 
####################################################################################
# Ensure strong name delay signed assemblies are treated properly.
####################################################################################
    sn -Vr *,b03f5f7f11d50a3a
    sn -Vr *,b77a5c561934e089
####################################################################################
# Copy all the nlp's to the current path to be able to run csc on them.
####################################################################################
    $(COPY_NAME) $(NLPDIR)\ctype$(NLPSUFFIX)    $(TARGETPATH)\ctype.nlp
    $(COPY_NAME) $(NLPDIR)\l_intl$(NLPSUFFIX)   $(TARGETPATH)\l_intl.nlp
    $(COPY_NAME) $(NLPDIR)\l_except$(NLPSUFFIX) $(TARGETPATH)\l_except.nlp
    $(COPY_NAME) $(NLPDIR)\culture$(NLPSUFFIX)  $(TARGETPATH)\culture.nlp
    $(COPY_NAME) $(NLPDIR)\region$(NLPSUFFIX)   $(TARGETPATH)\region.nlp
    $(COPY_NAME) $(NLPDIR)\sortkey$(NLPSUFFIX)  $(TARGETPATH)\sortkey.nlp
    $(COPY_NAME) $(NLPDIR)\sorttbls$(NLPSUFFIX) $(TARGETPATH)\sorttbls.nlp
    $(COPY_NAME) $(NLPDIR)\charinfo$(NLPSUFFIX) $(TARGETPATH)\charinfo.nlp

    $(COPY_NAME) $(NLPDIR)\big5$(NLPSUFFIX)     $(TARGETPATH)\big5.nlp
    $(COPY_NAME) $(NLPDIR)\bopomofo$(NLPSUFFIX) $(TARGETPATH)\bopomofo.nlp
    $(COPY_NAME) $(NLPDIR)\ksc$(NLPSUFFIX)      $(TARGETPATH)\ksc.nlp
    $(COPY_NAME) $(NLPDIR)\prc$(NLPSUFFIX)      $(TARGETPATH)\prc.nlp
    $(COPY_NAME) $(NLPDIR)\prcp$(NLPSUFFIX)     $(TARGETPATH)\prcp.nlp
    $(COPY_NAME) $(NLPDIR)\xjis$(NLPSUFFIX)     $(TARGETPATH)\xjis.nlp
    -$(DELETER) $(TARGETPATH)\$(TARGETNAME).$(TARGETEXT)
    -$(DELETER) $(O)\$(TARGETNAME).$(TARGETEXT)
!if "$(TARGETCOMPLUS)" != ""
    -$(DELETER) $(TARGETCOMPLUS)\$(TARGETNAME).$(TARGETEXT)
!endif
# Generate a response file for the CSC compiler flags.
    echo <<BclFiles.$(DDKBUILDENV)
/out:$@
$(CSC_COMPILE_FLAGS: =
) 
$(CSC_RESOURCE_FLAGS)
/nostdlib
$(CSC_SOURCES: =
)
<<keep
!if "$(PLATFORM_UNIX)" != "1"
    set __SECURITY_BOOTSTRAP_DB=$(CSC_SOURCE_DIR)
    $(CSC_COMPILER) @BclFiles.$(DDKBUILDENV) /noconfig $(O)\Version.cs $(O)\AppDomainSetup.cs
    set __SECURITY_BOOTSTRAP_DB=
    perl $(NTMAKEENV)\getbaseaddress.pl $(TARGETPATH)\mscorlib.dll $(CSC_BASE)
!else
    $(CSC_SOURCE_DIR)/buildmscorlib $(CSC_SOURCE_DIR) $(CSC_COMPILER) $(DDKBUILDENV) $(O)
!endif
!if "$(DELAY_SIGN)" != "1"
    sn -R $@ $(MICROSOFT_KEY_FILE)
!endif


####################################################################################
# Copy all the files to the deployed CLR installation. (Part 1)
####################################################################################
!if "$(TARGETCOMPLUS)" != ""
        $(COPY_NAME) $(TARGETPATH)\mscorlib.dll $(TARGETCOMPLUS)
        $(COPY_NAME) $(TARGETPATH)\*.nlp $(TARGETCOMPLUS)
!endif

####################################################################################
# Check whether mscorlib.dll is using security permissions that it wasn't before.
# This will result in null declarative security blobs being placed into the metadata
# (since we need a working mscorlib to do the permission set translation) and a
# $(CSC_SECURITY_DB).raw file being generated (during the compile of mscorlib.dll)
# to indicate that we need to update the mapping database used to locate the correct
# metadata blobs. Once the security mapping database has been updated we rebuild
# mscorlib from scratch, and this time all the permissions should be mapped into
# metadata correctly.                                                                    
####################################################################################

!if "$(PLATFORM_UNIX)" != "1"
    cmd /C "if exist $(CSC_SECURITY_DB).raw pushd ..\ToolBox\SecDBEdit && build -cC && popd"
    cmd /C "if exist $(CSC_SECURITY_DB).raw $(SECDBEDIT) -regen $(CSC_SECURITY_DB) xml"
    cmd /C "if exist $(CSC_SECURITY_DB).raw build -c"
!else

    if [ -f $(CSC_SECURITY_DB).raw ]; then cd ../toolbox/secdbedit && build -cC; fi
    if [ -f $(CSC_SECURITY_DB).raw ]; then $(SECDBEDIT) -regen $(CSC_SECURITY_DB) xml; fi
    if [ -f $(CSC_SECURITY_DB).raw ]; then build -c; fi

!endif

!endif # $(COMPLUSTARGET)

TARGET_EXTENSION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\mscorlib.pdb
TARGET_MANAGED_PDB = $(TARGETPATH)\mscorlib.ildb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
