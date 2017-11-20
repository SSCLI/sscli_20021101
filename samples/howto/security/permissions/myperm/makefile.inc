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

target: $(CSC_SOURCES)
# Generate the response file.
    perl $(NTMAKEENV)\gencscrsp.pl $(CSC_RSP) "$(CSC_COMPILE_FLAGS) $(CSC_SOURCES)" $(CSC_BASE)
# 
# Compile using response file. 
    $(CSC_COMPILER) /noconfig /out:$(O)\$(TARGETNAME).dll @$(CSC_RSP)
    @binplace -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $(COMPLUSTARGET)
    @binplace -R $(_NTTREE) $(BINPLACE_FLAGS) myperm.xml
    gacutil -i $(O)\$(TARGETNAME).dll
