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



!if "$(LINKONLY)" == "1" && "$(PLATFORM_UNIX)" != "1"

SetStack:
	editbin /STACK:4194304 $(TARGETCORBIN)\*\$(DDKBUILDENV)\ilasm.exe
	editbin /STACK:4194304 $(TARGETCOMPLUSSDK)\bin\ilasm.exe

!else

SetStack:

!endif
