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


!if "$(COMPLUSTARGET)"!=""
$(COMPLUSTARGET): $(CSC_SOURCES) $(O)\System.Runtime.Remoting.resources  CopyManagedPdb

csc_target_sign: $(COMPLUSTARGET)
        gacutil -i $(TARGETPATH)\$(TARGETNAME).$(TARGETEXT)
!endif

$(O)\System.Runtime.Remoting.resources: ..\resources.txt
    $(TARGETPATH)\internalresgen ..\resources.txt $(TARGETPATH)\System.Runtime.Remoting.resources

TARGET_EXTENSION_ =dll
TARGET_MANAGED_PDB = $(TARGETPATH)\System.Runtime.Remoting.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
