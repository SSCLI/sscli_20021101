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

####################################################################################
# Include extra source files specifying the assembly version and strong name as
# custom attributes.

$(O)\buildversioninfo.cs: version.pp $(CORBASE)\src\inc\version\__file__.ver
        perl $(NTMAKEENV)\keylocationex.pl $(ASSEMBLY_KEY_FILE) > $(O)\keydefine.h
!ifdef PLATFORM_UNIX
        $(CC_NAME) -x c++ -E -C -P -nostdinc -include $(CORBASE)\src\inc\version\__file__.ver -include $(O)\keydefine.h -DCOR_BUILD_YEAR=7 -DCSC_INCLUDE version.pp > $(O)/buildversioninfo.cs
!else
        cl /EP /C /FI $(CORBASE)\src\inc\version\__file__.ver /FI $(O)\keydefine.h /DCOR_BUILD_YEAR=7 /DCSC_INCLUDE version.pp > $(O)\buildversioninfo.cs
!endif

####################################################################################
#
# This rule is always run and it is run before $(COMPLUSTARGET):: is run.
#
####################################################################################

$(MANAGED_RESOURCES): $(CSC_SOURCE_DIR)\resources.txt
!if "$(_TGTCPU)" == "i386"
    $(TARGETPATH)\internalresgen $(CSC_SOURCE_DIR)\resources.txt $(MANAGED_RESOURCES)
!else
    $(INTTOOLSTARGET)\internalresgen $(CSC_SOURCE_DIR)\resources.txt $(MANAGED_RESOURCES)
!endif


####################################################################################
# PHASE 1 pass.
#   - Create a C# file containing the version that will be added in during the link 
#   - Create a C# file containing defines used in the runtime
#   - Copy the machine.config file
#   - create a config file for the compilers
####################################################################################
prep_target: $(O)\BuildVersionInfo.cs

####################################################################################


TARGET_EXTENSION_ = exe
TARGET_MANAGED_PDB = $(TARGETPATH)\jsc.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
