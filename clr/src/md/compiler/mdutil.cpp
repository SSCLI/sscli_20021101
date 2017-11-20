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
//*****************************************************************************
// MDUtil.cpp
//
// contains utility code to MD directory
//
//*****************************************************************************
#include "stdafx.h"
#include "metadata.h"
#include "mdutil.h"
#include "regmeta.h"
#include "disp.h"
#include "mdcommon.h"
#include "importhelper.h"


// Enable these three lines to turn off thread safety
#ifdef MD_THREADSAFE
 #undef MD_THREADSAFE
#endif
// Enable this line to turn on thread safety                            
#define MD_THREADSAFE 1

#include <rwutil.h>

// global variable to keep track all loaded modules
LOADEDMODULES	*g_LoadedModules = NULL;
UTSemReadWrite *LOADEDMODULES::m_pSemReadWrite = NULL;

//*****************************************************************************
// Add a RegMeta pointer to the loaded module list
//*****************************************************************************
HRESULT LOADEDMODULES::AddModuleToLoadedList(RegMeta *pRegMeta)
{
	HRESULT		hr = NOERROR;
	RegMeta		**ppRegMeta;

    LOCKWRITE();
    
	// create the dynamic array if it is not created yet.
	if (g_LoadedModules == NULL)
	{
		g_LoadedModules = new LOADEDMODULES;
        IfNullGo(g_LoadedModules);
	}

	ppRegMeta = g_LoadedModules->Append();
    IfNullGo(ppRegMeta);
	
	*ppRegMeta = pRegMeta;
    
ErrExit:    
    return hr;
}	// LOADEDMODULES::AddModuleToLoadedList

//*****************************************************************************
// Remove a RegMeta pointer from the loaded module list
//*****************************************************************************
BOOL LOADEDMODULES::RemoveModuleFromLoadedList(RegMeta *pRegMeta)
{
	int			count;
	int			index;
    BOOL        bRemoved = FALSE;
    ULONG       cRef;
    
    LOCKWRITE();
    
    // The cache is locked for write, so no other thread can obtain the RegMeta
    //  from the cache.  See if some other thread has a ref-count.
    cRef = pRegMeta->GetRefCount();
    
    // If some other thread has a ref-count, don't remove from the cache.
    if (cRef > 0)
        return FALSE;
    
	// If there is no loaded modules, don't bother
	if (g_LoadedModules == NULL)
	{
		return TRUE; // Can't be cached, same as if removed by this thread.
	}

	// loop through each loaded modules
	count = g_LoadedModules->Count();
	for (index = 0; index < count; index++)
	{
		if ((*g_LoadedModules)[index] == pRegMeta)
		{
			// found a match to remove
			g_LoadedModules->Delete(index);
            bRemoved = TRUE;
			break;
		}
	}

	// If no more loaded modules, delete the dynamic array
	if (g_LoadedModules->Count() == 0)
	{
		delete g_LoadedModules;
		g_LoadedModules = NULL;
	}
    
    return bRemoved;
}	// LOADEDMODULES::RemoveModuleFromLoadedList


//*****************************************************************************
// Remove a RegMeta pointer from the loaded module list
//*****************************************************************************
HRESULT LOADEDMODULES::ResolveTypeRefWithLoadedModules(
	mdTypeRef   tr,			            // [IN] TypeRef to be resolved.
	IMetaModelCommon *pCommon,  		// [IN] scope in which the typeref is defined.
	REFIID		riid,					// [IN] iid for the return interface
	IUnknown	**ppIScope,				// [OUT] return interface
	mdTypeDef	*ptd)					// [OUT] typedef corresponding the typeref
{
	HRESULT		hr = NOERROR;
	RegMeta		*pRegMeta;
    CQuickArray<mdTypeRef> cqaNesters;
    CQuickArray<LPCUTF8> cqaNesterNamespaces;
    CQuickArray<LPCUTF8> cqaNesterNames;
	int			count;
	int			index;

	if (g_LoadedModules == NULL)
	{
		// No loaded module!
		_ASSERTE(!"Bad state!");
		return E_FAIL;
	}

    LOCKREAD();
    
    // Get the Nesting hierarchy.
    IfFailGo(ImportHelper::GetNesterHierarchy(pCommon, tr, cqaNesters,
                                cqaNesterNamespaces, cqaNesterNames));

    count = g_LoadedModules->Count();
	for (index = 0; index < count; index++)
	{
		pRegMeta = (*g_LoadedModules)[index];

        hr = ImportHelper::FindNestedTypeDef(
                                pRegMeta->GetMiniMd(),
                                cqaNesterNamespaces,
                                cqaNesterNames,
                                mdTokenNil,
                                ptd);
		if (SUCCEEDED(hr))
		{
            // found a loaded module containing the TypeDef.
            hr = pRegMeta->QueryInterface(riid, (void **)ppIScope);			
            break;
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
	}
	if (FAILED(hr))
	{
		// cannot find the match!
		hr = E_FAIL;
	}
ErrExit:
	return hr;
}	// LOADEDMODULES::ResolveTypeRefWithLoadedModules




//*******************************************************************************
//
// Determine the blob size base of the ELEMENT_TYPE_* associated with the blob.
// This cannot be a table lookup because ELEMENT_TYPE_STRING is an unicode string.
// The size of the blob is determined by calling wcsstr of the string + 1.
//
//*******************************************************************************
ULONG _GetSizeOfConstantBlob(
	DWORD		dwCPlusTypeFlag,			// ELEMENT_TYPE_*
	void		*pValue,					// BLOB value
	ULONG		cchString)					// String length in wide chars, or -1 for auto.
{
	ULONG		ulSize = 0;

	switch (dwCPlusTypeFlag)
	{
    case ELEMENT_TYPE_BOOLEAN:
		ulSize = sizeof(BYTE);
		break;
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
		ulSize = sizeof(BYTE);
		break;
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
		ulSize = sizeof(SHORT);
		break;
    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_R4:
		ulSize = sizeof(LONG);
		break;
		
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
		ulSize = sizeof(DOUBLE);
		break;

    case ELEMENT_TYPE_STRING:
		if (pValue == 0)
			ulSize = 0;
		else
        if (cchString != (ULONG) -1)
			ulSize = cchString * sizeof(WCHAR);
		else
            ulSize = (ULONG)(sizeof(WCHAR) * wcslen((LPWSTR)pValue));
		break;

	case ELEMENT_TYPE_CLASS:
		ulSize = sizeof(IUnknown *);
		break;
	default:
		_ASSERTE(!"Not a valid type to specify default value!");
		break;
	}
	return ulSize;
}   // _GetSizeOfConstantBlob

