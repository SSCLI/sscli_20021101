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
/*
 * Read from the class/offset table that resides within a debugged process.
 */

#include "strike.h"
#include "util.h"
#include "get-table-info.h"
#include <dump-tables.h>
#include "process-info.h"

#ifdef _DEBUG
    #define DOUT OutputDebugStringW
#else
static void _ods (const wchar_t* s)
{
}
    #define DOUT _ods
#endif

const ULONG_PTR InvalidOffset = static_cast<ULONG_PTR>(-1);

static ClassDumpTable g_ClassTable;
static BOOL           g_fClassTableInit = FALSE;

/**
 * Initialize g_ClassTable to point to the class dump table in the debuggee
 * process.
 */
bool InitializeOffsetTable ()
{
    if (g_fClassTableInit)
        return (true);

    // We use the process handle to determine if we're debugging the same
    // process.  It's conceivable that the same debugger session will debug
    // multiple programs, so the process may change, requiring everything to be
    // re-loaded.
#ifdef PLATFORM_WIN32
    HRESULT hr = E_FAIL;
    // Module names don't include file name extensions.
    ULONG64 BaseOfDll;
    if (FAILED(hr = g_ExtSymbols->GetModuleByModuleName (
                                                        "sscoree", 0, NULL, &BaseOfDll)))
    {
        DOUT (L"unable to get base of sscoree.dll; stopping.\n");
        return (false);
    }

    int tableName = 80;
    ULONG_PTR TableAddress = NULL;
    if (!GetExportByName ((ULONG_PTR) BaseOfDll, 
                          reinterpret_cast<const char*>(tableName), &TableAddress))
    {
        DOUT (L"unable to find class dump table");
        return (false);
    }
#else  //PLATFORM_UNIX
    HMODULE hMod = LoadLibraryA(MAKEDLLNAME_A("sscoree"));
    if (!hMod) {
      DOUT(L"unable to find libsscoree.so in the process");
      return (false);
    }
    ULONG_PTR TableAddress = (ULONG_PTR)GetProcAddress(hMod, "g_ClassDumpData");
    if (!TableAddress) {
      DOUT(L"unable to find g_ClassDumpData in libsscoree.so");
      return (false);
    }
#endif //PLATFORM_UNIX

    ULONG bytesRead;
    if (!SafeReadMemory (TableAddress, &g_ClassTable, 
                         sizeof(g_ClassTable), &bytesRead))
    {
        DOUT (L"Lunable to read class dump table");
        return (false);
    }

    // Is the version what we're expecting?  If it isn't, we don't know what the
    // correct indexes are.
    if (g_ClassTable.version != 1)
        return (false);

    // At this point, everything has been initialized properly.
    g_fClassTableInit = TRUE;
    return (true);
}

/**
 * Return pointer to initialized class dump table.
 */
ClassDumpTable *GetClassDumpTable()
{
    if (InitializeOffsetTable())
        return &g_ClassTable;

    return (NULL);
}

/**
 * Return the memory location of the beginning of the ClassDumpInfo for the
 * requested class.
 */
static ULONG_PTR GetClassInfo (size_t klass)
{
    // is the requested class correct?
    if (klass == (size_t)-1)
        return (InvalidOffset);

    // make sure our data is current.
    if (!InitializeOffsetTable ())
        return (InvalidOffset);

    if (klass >= g_ClassTable.nentries)
        return (InvalidOffset);


    // g_ClassTable.classes is a continuous array of pointers to ClassDumpInfo
    // objects.  We need the address of the correct object.
    ULONG BR; // bytes read
    ULONG_PTR Class;
    if (!SafeReadMemory (
                        reinterpret_cast<ULONG_PTR>(g_ClassTable.classes) + // base of array
                        (klass*sizeof(ClassDumpInfo*)), // memory offset into array
                        &Class, 
                        sizeof(Class), &BR))
        return (InvalidOffset);

    return (Class);
}


ULONG_PTR GetMemberInformation (size_t klass, size_t member)
{
    const ULONG_PTR error = InvalidOffset;

    // get the location of the class in memory
    ULONG_PTR pcdi;
    if ((pcdi = GetClassInfo(klass)) == InvalidOffset)
        return (error);

    ULONG BR; // bytes read
    ClassDumpInfo cdi;
    if (!SafeReadMemory (pcdi, &cdi, sizeof(cdi), &BR))
        return (error);

    // get the member
    if (member == (size_t)-1)
        return (error);
    if (member >= cdi.nmembers)
        return (error);

    ULONG_PTR size;
    if (!SafeReadMemory (
                        reinterpret_cast<ULONG_PTR>(cdi.memberOffsets) + // base of offset array
                        (member*sizeof(ULONG_PTR)),  // member index
                        &size,
                        sizeof(size),
                        &BR))
        return (error);

    return (size);
}


SIZE_T GetClassSize (size_t klass)
{
    // reminder: in C++, all classes must be at least 1 byte in size
    // (this is to prevent two variables from having the same memory address)
    // Thus, 0 is an invalid class size value.
    const SIZE_T error = 0;

    // get the location of the class in memory
    ULONG_PTR pcdi;
    if ((pcdi = GetClassInfo(klass)) == InvalidOffset)
        return (error);

    // read in the class information.
    ULONG BR; // bytes read
    ClassDumpInfo cdi;
    if (!SafeReadMemory (pcdi, &cdi, sizeof(cdi), &BR))
        return (error);

    return (cdi.classSize);
}

ULONG_PTR GetEEJitManager ()
{
    if (InitializeOffsetTable())
        return (g_ClassTable.pEEJitManagerVtable);
    return (0);
}

ULONG_PTR GetEconoJitManager ()
{
    if (InitializeOffsetTable())
        return (g_ClassTable.pEconoJitManagerVtable);
    return (0);
}

ULONG_PTR GetMNativeJitManager ()
{
    if (InitializeOffsetTable())
        return (g_ClassTable.pMNativeJitManagerVtable);
    return (0);
}

