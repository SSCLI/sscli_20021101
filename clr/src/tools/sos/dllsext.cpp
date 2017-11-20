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

#include "strike.h"
#include "data.h"
#include "util.h"

#if !PLATFORM_UNIX



VOID
DllsName(
    ULONG_PTR addrContaining,
    WCHAR *dllName
    )
{
    dllName[0] = L'\0';
    
    ULONG Index;
    ULONG64 base;
    if (g_ExtSymbols->GetModuleByOffset(addrContaining, 0, &Index, &base) != S_OK)
        return;
    CHAR name[MAX_PATH+1];
    ULONG length;
    WCHAR wname[MAX_PATH+1] = L"\0";
    if (g_ExtSymbols->GetModuleNames(Index,base,name,MAX_PATH,&length,NULL,0,NULL,NULL,0,NULL) == S_OK)
    {
        MultiByteToWideChar (CP_ACP,0,name,-1,wname,MAX_PATH);
    }

    MatchDllsName (wname, dllName, base);
}

VOID
MatchDllsName (WCHAR *wname, WCHAR *dllName, ULONG64 base)
{
    if (!IsDumpFile() && !IsKernelDebugger()) {
        if (FileExist(wname)) {
            wcscpy (dllName,wname);
            return;
        }
    }
    else
    {
        if (IsKernelDebugger() && DllPath == NULL) {
            ExtOut ("Path for managed Dll not set yet\n");
            goto NotFound;
        }
        
        WCHAR *wptr = wcsrchr (wname, '\\');
        
        if (wptr == NULL) {
            wptr = wname;
        }
        else
            wptr ++;
        
        if (wptr && DllPath == NULL) {
            if (FileExist(wname)) {
                wcscpy (dllName, wname);
                return;
            }
        }
        if (DllPath == NULL) {
            ExtOut ("Path for managed Dll not set yet\n");
            goto NotFound;
        }
        
        const WCHAR *path = DllPath->PathToDll(wptr);
    
        if (path) {
            wcscpy (dllName,path);
            wcscat (dllName,L"\\");
            wcscat (dllName,wptr);
            return;
        }
    }
    
NotFound:
    // We do not find the module
    wcscpy (dllName,L"Not Available: ");
    int len = wcslen (wname);
    WCHAR *wptr = wname;
    if (len > 200) {
        wptr += len-200;
    }
    wcscat (dllName, wptr);
    wptr = dllName + wcslen (dllName);

    wsprintfW (wptr, L" [Base %p]", base);
    return;
}

#else //PLATFORM_UNIX

VOID
MatchDllsName (WCHAR *wname, WCHAR *dllName, ULONG64 base)
{
   if (FileExist(wname)) {
       wcscpy (dllName,wname);
       return;
    }

    // We do not find the module
    wcscpy (dllName,L"Not Available: ");
    int len = wcslen (wname);
    WCHAR *wptr = wname;
    if (len > 200) {
        wptr += len-200;
    }
    wcscat (dllName, wptr);
    wptr = dllName + wcslen (dllName);

    wsprintfW (wptr, L" [Base %p]", base);
    return;
}

#endif //!PLATFORM_UNIX
