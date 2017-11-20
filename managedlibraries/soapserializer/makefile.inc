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

csc_target_sign: $(COMPLUSTARGET)

SoapFormatter.resources: soapformatter.txt
    $(TARGETPATH)\internalresgen soapformatter.txt $(O)\SoapFormatter.resources

System.XML.Errors.resources: system.xml.errors.txt
    $(TARGETPATH)\internalresgen system.xml.errors.txt $(O)\System.XML.Errors.resources

TARGET_EXTENSION_ =dll
TARGET_MANAGED_PDB = $(TARGETPATH)\System.Runtime.Serialization.Formatters.Soap.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

        gacutil -i $(TARGETPATH)\$(TARGETNAME).$(TARGETEXT)
