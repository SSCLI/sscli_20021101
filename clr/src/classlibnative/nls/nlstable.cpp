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
#include "common.h"
#include <winwrap.h>
#include <excep.h>          // For COMPlusThrow
#include <appdomain.hpp>
#include <assembly.hpp>
#include "nlstable.h"       // Class declaration


/*=================================NLSTable==========================
**Action: Constructor for NLSTable.  It caches the assembly from which we will read data table files.
**Returns: Create a new NLSTable instance.
**Arguments: pAssembly  the Assembly that NLSTable will retrieve data table files from.
**Exceptions: None.
============================================================================*/

NLSTable::NLSTable(Assembly* pAssembly) {
    _ASSERTE(pAssembly != NULL);
    m_pAssembly = pAssembly;
}

/*=================================OpenDataFile==================================
**Action: Open the specified NLS+ data file from system assembly.
**Returns: The file handle for the required NLS+ data file.
**Arguments: The required NLS+ data file name (in ANSI)
**Exceptions: ExecutionEngineException if error happens in get the data file
**            from system assembly.
==============================================================================*/

HANDLE NLSTable::OpenDataFile(LPCSTR pFileName) {
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(m_pAssembly != NULL);

    DWORD cbResource;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PBYTE pbInMemoryResource = NULL;
    //
    // Get the base system assembly (mscorlib.dll to most of us);
    //
    

    // Get the resource, and associated file handle, from the assembly.
    if (FAILED(m_pAssembly->GetResource(pFileName, &hFile,
                                        &cbResource, &pbInMemoryResource,
                                        NULL, NULL, NULL))) {
        _ASSERTE(!"Didn't get the resource for System.Globalization.");
        FATAL_EE_ERROR();
    }

    // Get resource could return S_OK, but hFile will not be set if
    // the found resource was in-memory.
    _ASSERTE(hFile != INVALID_HANDLE_VALUE);

    return (hFile);
}

/*=================================OpenDataFile==================================
**Action: Open a NLS+ data file from system assembly.
**Returns: The file handle for the required NLS+ data file.
**Arguments: The required NLS+ data file name (in Unicode)
**Exceptions: OutOfMemoryException if buffer can not be allocated.
**            ExecutionEngineException if error happens in calling OpenDataFile(LPCSTR)
==============================================================================*/

HANDLE NLSTable::OpenDataFile(LPCWSTR pFileName)
{
    THROWSCOMPLUSEXCEPTION();
    // The following marco will delete pAnsiFileName when
    // getting out of the scope of this function.
    MAKE_ANSIPTR_FROMWIDE(pAnsiFileName, pFileName);
    if (!pAnsiFileName)
    {
        COMPlusThrowOM();
    }

    HANDLE hFile = OpenDataFile((LPCSTR)pAnsiFileName);
    _ASSERTE(hFile != INVALID_HANDLE_VALUE);
    return (hFile);
}


/*=================================MapDataFile==================================
**Action: Open a named file mapping object specified by pMappingName.  If the
**  file mapping object is not created yet, create it from the file specified
**  by pFileName.
**Returns: a LPVOID pointer points to the view of the file mapping object.
**Arguments:
**  pMappingName: the name used to create file mapping.
**  pFileName: the required file name.
**  hFileMap: used to return the file mapping handle.
**Exceptions: ExecutionEngineException if error happens.
==============================================================================*/


LPVOID NLSTable::MapDataFile(LPCWSTR pMappingName, LPCSTR pFileName, HANDLE *hFileMap) {
    _ASSERTE(pMappingName != NULL); // Must be a named file mapping object.
    _ASSERTE(pFileName != NULL);    // Must have a valid file name.
    _ASSERTE(hFileMap != NULL);     // Must have a valid location for the handle.

    THROWSCOMPLUSEXCEPTION();

    *hFileMap = NULL;
    LPVOID pData=NULL; //It's silly to have this up here, but it makes the compiler happy.

    HANDLE hFile = OpenDataFile(pFileName);

    if (hFile == INVALID_HANDLE_VALUE) {
        goto ErrorExit;
    }
    *hFileMap = WszCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);
    if (*hFileMap == NULL) {
        _ASSERTE(!"Error in CreateFileMapping");
        goto ErrorExit;
    }

    //
    // Map a view of the file mapping.
    //
    pData = MapViewOfFile(*hFileMap, FILE_MAP_READ, 0, 0, 0);
    if (pData == NULL)
    {
        _ASSERTE(!"Error in MapViewOfFile");
        goto ErrorExit;
    }

    return (pData);
            
 ErrorExit:
    if (*hFileMap) {
        CloseHandle(*hFileMap);
    }

    //If we can't get the table, we're in trouble anyway.  Throw an EE Exception.
    FATAL_EE_ERROR();
    return NULL;
}


/*=================================MapDataFile==================================
**Action: Open a named file mapping object specified by pMappingName.  If the
**  file mapping object is not created yet, create it from the file specified
**  by pFileName.
**Returns: a LPVOID pointer points to the view of the file mapping object.
**Arguments:
**  pMappingName: the name used to create file mapping.
**  pFileName: the required file name.
**  hFileMap: used to return the file mapping handle.
**Exceptions: ExecutionEngineException if error happens.
==============================================================================*/

LPVOID NLSTable::MapDataFile(LPCWSTR pMappingName, LPCWSTR pFileName, HANDLE *hFileMap) 
{    
    THROWSCOMPLUSEXCEPTION();
    // The following macro will delete pAnsiFileName when
    // getting out of the scope of this function.
    MAKE_ANSIPTR_FROMWIDE(pAnsiFileName, pFileName);
    if (!pAnsiFileName)
    {
        COMPlusThrowOM();
    }

    return (MapDataFile(pMappingName, pAnsiFileName, hFileMap));
}
