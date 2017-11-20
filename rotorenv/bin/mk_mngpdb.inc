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
# This file should be included from makefile.inc for managed componenets
# if you want to copy managed .pdb for the component to the right place.
#
# TARGET_MANAGED_PDB         defines the .pdb being copied
# TARGET_EXTENSION_          defines the type of component (dll / exe)
# Define these two macros before you include this file

!if "$(MANAGED_PDB_DESTINATION)" == ""

MANAGED_PDB_DESTINATION = $(TARGETCOMPLUS)\Symbols.pri\$(TARGET_EXTENSION_)

!endif


!IF ( "$(TARGET_MANAGED_PDB)" == "" || "$(TARGET_EXTENSION_)" == "" || \
      "$(TARGETCOMPLUS)" == "" )

# Someone calls us improperly or .pdb does not exist
CopyManagedPdb:
#   @echo ** MANAGED_PDB_DESTINATION        $(MANAGED_PDB_DESTINATION)
#   @echo ** TARGET_MANAGED_PDB             $(TARGET_MANAGED_PDB)
#   @echo ** MANAGED_PDB_DESTINATION: PDB does not exist
	 
!ELSE

CopyManagedPdb:
#	@echo ** MANAGED_PDB_DESTINATION        $(MANAGED_PDB_DESTINATION)
#	@echo ** TARGET_MANAGED_PDB             $(TARGET_MANAGED_PDB)
!ifdef PLATFORM_UNIX
    if [ -f $(TARGET_MANAGED_PDB) ] && [ ! -d $(MANAGED_PDB_DESTINATION) ]; then $(MD) $(MANAGED_PDB_DESTINATION); fi
    if [ -f $(TARGET_MANAGED_PDB) ]; then $(COPY_NAME) $(TARGET_MANAGED_PDB) $(MANAGED_PDB_DESTINATION); fi
!else
     if exist $(TARGET_MANAGED_PDB) (
       if not exist $(MANAGED_PDB_DESTINATION) mkdir $(MANAGED_PDB_DESTINATION) > nul
     )
     if exist $(TARGET_MANAGED_PDB) copy $(TARGET_MANAGED_PDB) $(MANAGED_PDB_DESTINATION)
!endif
!ENDIF

