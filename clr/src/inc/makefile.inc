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


opcode.refops: opcode.def genrops.pl
	perl genrops.pl < opcode.def > opcode.refops

!if "$(PLATFORM_UNIX)" == "1"
COPY = cp -p -f
!else
COPY = xcopy
!endif


doscript: 
!if "$(TARGETCOMPLUS)" != ""
	binplace -e -r $(TARGETCOMPLUS) -p $(BINPLACE_PLACEFILE) *.h opcode.def
!endif

copyheaders:
    $(COPY) $(CORBASE)\..\palrt\idl\*.h $(PASS0_HEADERDIR)
    $(COPY) $(CORBASE)\..\palrt\idl\*_i.c $(PASS0_SOURCEDIR)
