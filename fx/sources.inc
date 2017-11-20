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

##############
# sources.inc
#
# Must be included in every sources file for FX build system to work.
#
##############

#
#                           to force build.exe to execute a link pass (pass 2) in
# directories that build managed binaries, we define NTTARGETFILES.
#

!ifndef NTTARGETFILES
! ifdef COMPLUSTYPE
NTTARGETFILES=
! endif
!endif

