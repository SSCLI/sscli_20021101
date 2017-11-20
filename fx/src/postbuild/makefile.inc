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
!If "$(LINKONLY)" != "1"
DoPostBuild:
   @echo nothing to do until pass 2.

!else
DoPostBuild: DoTitle \
	     DoGacInstall \
	     DoResetSecurity

!endif

DoTitle:
    @echo Postbuild_Steps


#
# Reset the security policy
#

DoResetSecurity:
    clix $(TARGETCOMPLUS)\caspol.exe -q -u -reset -q -m -reset -q -en -reset -buildcache



#
# Run gacinstall
#
DoGacInstall:
    gacutil -i $(TARGETCOMPLUS)\System.dll
    gacutil -i $(TARGETCOMPLUS)\System.Xml.dll
