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

MSC_WARNING_LEVEL=/W4 /WX

TARGETNAME=fusuuid
TARGETTYPE=LIBRARY

INCLUDES= $(FUSION_PATH)\idl\$(O)

!include $(CORBASE)\src\fusion\fusion.inc
SOURCES_USED=$(SOURCES_USED) $(CORBASE)\src\fusion\fusion.inc

# prevent system headers from being included.  They are not needed just to
# process a bunch of DEFINE_GUIDs anyhow.  Excluding them generates a much
# smaller library too.
#
!if "$(PLATFORM_UNIX)" != "1"
C_DEFINES=$(C_DEFINES) /D_MIDL_USE_GUIDDEF_\
    /D__RPCNDR_H__ /D__RPC_H__ /DRPC_NO_WINDOWS_H /DCOM_NO_WINDOWS_H\
    /DGUID_DEFS_ONLY
!endif

SOURCES= \
	fusion_uuid.c \
	fuspriv_uuid.c

SOURCES_USED=sources.inc
