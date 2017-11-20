// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
// ===========================================================================
// File: MethodImpl.CPP
//
// ===========================================================================
//
// ===========================================================================
//

#include "common.h"
#include "methodimpl.h"

MethodDesc *MethodImpl::FindMethodDesc(DWORD slot, MethodDesc* defaultReturn)
{
    if(pdwSlots == NULL) return defaultReturn;

    DWORD dwSize = *pdwSlots;
    if(dwSize == 0) return defaultReturn;

    DWORD l = 1;      // the table is biased by one. The first entry is the size
    DWORD r = dwSize;
    DWORD pivot;
    while(1) {
        pivot =  (l + r) / 2;
        if(pdwSlots[pivot] == slot)
            break; // found it
        else if(pdwSlots[pivot] < slot) 
            l = pivot + 1;
        else
            r = pivot - 1;

        if(l > r) return defaultReturn; // Not here
    }

    MethodDesc *result = pImplementedMD[pivot-1]; // The method descs are not offset by one

    // Prejitted images may leave NULL in this table if
    // the methoddesc is declared in another module.
    // In this case we need to manually compute & restore it
    // from the slot number.

    if (result == NULL)
        result = RestoreSlot(pivot-1, defaultReturn->GetMethodTable());

    return result;
}

MethodDesc *MethodImpl::RestoreSlot(DWORD index, MethodTable *pMT)
{
    MethodDesc *result;

    DWORD slot = pdwSlots[index+1];

    // Since the overridden method is in a different module, we 
    // are guaranteed that it is from a different class.  It is
    // either an override of a parent virtual method or parent-implemented
    // interface, or of an interface that this class has introduced.  
            
    // In the former 2 cases, the slot number will be in the parent's 
    // vtable section, and we can retrieve the implemented MethodDesc from
    // there.  In the latter case, we can search through our interface
    // map to determine which interface it is from.

    EEClass *pParentClass = pMT->GetClass()->GetParentClass();
    if (pParentClass != NULL
        && slot < pParentClass->GetNumVtableSlots())
    {
        result = pParentClass->GetMethodDescForSlot(slot);
    }
    else
    {
        _ASSERTE(slot < pMT->GetClass()->GetNumVtableSlots());
                
        InterfaceInfo_t *pInterface = pMT->GetInterfaceForSlot(slot);
        _ASSERTE(pInterface != NULL);

        result = pInterface->m_pMethodTable->
          GetMethodDescForSlot(slot - pInterface->m_wStartSlot);
    }
            
    _ASSERTE(result != NULL);

    // Don't worry about races since we would all be setting the same result
    pImplementedMD[index] = result;

    return result;
}

MethodImpl* MethodImpl::GetMethodImplData(MethodDesc* pDesc)
{
    if(pDesc->IsMethodImpl() == FALSE)
        return NULL;
    else {
        MethodImpl* pImpl = NULL;
        switch(pDesc->GetClassification()) {
        case mcNDirect:
            pImpl = ((MI_NDirectMethodDesc*) pDesc)->GetImplData();
            break;
        case mcECall:
        case mcIL:
        case mcEEImpl:
            pImpl = ((MI_MethodDesc*) pDesc)->GetImplData();
            break;
        default:
            _ASSERTE(!"We have an invalid method type for a method impl body");
        }
        
        return pImpl;
    }
}

MethodDesc* MethodImpl::GetFirstImplementedMD(MethodDesc *pContainer)
{
    _ASSERTE(GetSize() > 0);
    _ASSERTE(pImplementedMD != NULL);

    MethodDesc *pMD = pImplementedMD[0];

    if (pMD == NULL)
    {
        // Restore slot in prejit image if necessary
        RestoreSlot(0, pContainer->GetMethodTable());
        pMD = pImplementedMD[0];
        _ASSERTE(pMD != NULL);
    }

    return pImplementedMD[0];
}

