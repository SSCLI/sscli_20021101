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
    perl $(NTMAKEENV)\gencscrsp.pl $(CSC_RSP) "$(CSC_COMPILE_FLAGS) $(CSC_SOURCES)" $(CSC_BASE)
#
# now compile
#
    $(CSC_COMPILER) /noconfig /out:$(O)\$(TARGETNAME).dll @$(CSC_RSP)
    ildasm $(O)\$(TARGETNAME).dll /out:$(O)\$(TARGETNAME).il1
    perl callconvhack.pl <$(O)\$(TARGETNAME).il1 >$(O)\$(TARGETNAME).il2
    ilasm /DLL /QUIET $(O)\$(TARGETNAME).il2
    binplace -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $(COMPLUSTARGET)
