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

!if "$(OS)" == "Windows_NT"
SYSTEMDIR=$(WINDIR)\system32
!else
SYSTEMDIR=$(WINDIR)\system
!endif

MSCOREE_TARGET=$(SYSTEMDIR)\mscoree.dll

!if "$(TARGETCOMPLUS)" != ""
SYM_TARGET1=$(TARGETCOMPLUS)\symwrtr.dll
SYM_TARGET2=$(TARGETCOMPLUS)\diasymreader.dll
SYM_TARGET3=$(TARGETCOMPLUS)\msdia70.dll
SYM_TARGET4=$(TARGETCOMPLUS)\mscoree.tlb
!else
SYM_TARGET1=$(CORENV)\bin\i386\symwrtr.dll
SYM_TARGET2=$(CORENV)\bin\i386\diasymreader.dll
SYM_TARGET3=$(CORENV)\bin\i386\msdia70.dll
SYM_TARGET4=$(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoree.tlb
!endif

!if "$(LINKONLY)" == "1"

PlaceDll:
!if "$(TARGETCOMPLUS)" != ""
!if "$(_TGTCPU)" == "i386"
        binplace -e -r $(SYSTEMDIR) -p placefil.txt $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoree.dll $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoree.pdb
!else
        copy $(URTTARGET)\mscoree.dll $(SYSTEMDIR)
        copy $(URTTARGET)\mscoree.pdb $(SYSTEMDIR)
!endif
!endif

RegisterThisDll: $(MSCOREE_TARGET)
        regsvr32 /s /c $(MSCOREE_TARGET)
        regsvr32 /s /c $(SYM_TARGET1)
        regsvr32 /s /c $(SYM_TARGET2)
        regsvr32 /s /c $(SYM_TARGET3)
                regtlb $(SYM_TARGET4)

!if "$(TARGETCOMPLUS)" != ""
        -mkdir $(TARGETCOMPLUS)\sdk\lib
        copy $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoree.lib $(TARGETCOMPLUS)\sdk\lib
!endif

CopyAdditionalBits:
        copy /y $(TARGETPATH)\mscorrc.reg $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
        copy /y $(TARGETPATH)\mscorrc.reg $(TARGETCOMPLUS)
!else

PlaceDll:
RegisterThisDll:
CopyAdditionalBits:

!endif

