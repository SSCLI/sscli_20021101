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

!INCLUDE $(NTMAKEENV)\makefile.csc

csc_target: $(COMPLUSTARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
!if "$(TARGETCOMPLUS)" != ""
        $(COPY_NAME) $(TARGETPATH)\$(TARGETNAME).$(TARGETEXT) $(TARGETCOMPLUS)
!endif

csc_compile_resource: caspol.cs resources.txt
        internalresgen resources.txt caspol.resources

TARGET_EXTENSION_ = exe
TARGET_MANAGED_PDB = $(COMPLUSTARGET:.exe=.pdb)
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
