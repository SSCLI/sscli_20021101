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

!IF "$(FREEBUILD)"!="1"


!ENDIF

STD_DEFINES = $(STD_DEFINES) $(C_DEFINES)

!if "$(ASM_SUFFIX)" == "asm"
$(O)/asmconstants.inc: ..\$(_TGTCPU)\asmconstants.h
    h2inc /Fa$@ /C $(C_DEFINES) $(TARGET_DEFINES) ..\$(_TGTCPU)\asmconstants.h
!else
# Nothing to do
$(O)/asmconstants.inc:
!endif
