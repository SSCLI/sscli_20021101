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
// Debug.cpp
//
// Helper code for debugging.
//*****************************************************************************
#include "stdafx.h"
#include "utilcode.h"

#ifdef _DEBUG
#define LOGGING
#endif


#include "log.h"


#ifdef _DEBUG

// On windows, we need to set the MB_SERVICE_NOTIFICATION bit on message
//  boxes, but that bit isn't defined under windows CE.  This bit of code
//  will provide '0' for the value, and if the value ever is defined, will
//  pick it up automatically.
 # define COMPLUS_MB_SERVICE_NOTIFICATION MB_SERVICE_NOTIFICATION


//*****************************************************************************
// This struct tracks the asserts we want to ignore in the rest of this
// run of the application.
//*****************************************************************************
struct _DBGIGNOREDATA
{
    char        rcFile[_MAX_PATH];
    long        iLine;
    bool        bIgnore;
};

typedef CDynArray<_DBGIGNOREDATA> DBGIGNORE;
static BYTE grIgnoreMemory[sizeof(DBGIGNORE)];
inline DBGIGNORE* GetDBGIGNORE()
{
    static bool fInit = false;
    if (!fInit)
    {
        new (grIgnoreMemory) CDynArray<_DBGIGNOREDATA>();
        fInit = true;
    }

    return (DBGIGNORE*)grIgnoreMemory;
}

BOOL NoGuiOnAssert()
{
    static ConfigDWORD fNoGui;
    return fNoGui.val(L"NoGuiOnAssert", 0);
}

BOOL DebugBreakOnAssert()
{
    static ConfigDWORD fDebugBreak;
    return fDebugBreak.val(L"DebugBreakOnAssert", 0);
}

VOID TerminateOnAssert()
{
    ShutdownLogging();
    TerminateProcess(GetCurrentProcess(), 123456789);
}


VOID LogAssert(
    LPCSTR      szFile,
    int         iLine,
    LPCSTR      szExpr
)
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    WCHAR exename[300];
    WszGetModuleFileName(NULL, exename, sizeof(exename)/sizeof(WCHAR));

    LOG((LF_ASSERT,
         LL_FATALERROR,
         "FAILED ASSERT(PID %d [0x%08x], Thread: %d [0x%x]) (%lu/%lu/%lu: %02lu:%02lu:%02lu %s): File: %s, Line %d : %s\n",
         GetCurrentProcessId(),
         GetCurrentProcessId(),
         GetCurrentThreadId(),
         GetCurrentThreadId(),
         (ULONG)st.wMonth,
         (ULONG)st.wDay,
         (ULONG)st.wYear,
         1 + (( (ULONG)st.wHour + 11 ) % 12),
         (ULONG)st.wMinute,
         (ULONG)st.wSecond,
         (st.wHour < 12) ? "am" : "pm",
         szFile,
         iLine,
         szExpr));
    LOG((LF_ASSERT, LL_FATALERROR, "RUNNING EXE: %ws\n", exename));
}

//*****************************************************************************

BOOL LaunchJITDebugger()
{
    return (FALSE);
}


//*****************************************************************************
// This function is called in order to ultimately return an out of memory
// failed hresult.  But this guy will check what environment you are running
// in and give an assert for running in a debug build environment.  Usually
// out of memory on a dev machine is a bogus alloction, and this allows you
// to catch such errors.  But when run in a stress envrionment where you are
// trying to get out of memory, assert behavior stops the tests.
//*****************************************************************************
HRESULT _OutOfMemory(LPCSTR szFile, int iLine)
{
    DbgWriteEx(L"WARNING:  Out of memory condition being issued from: %hs, line %d\n",
            szFile, iLine);
    return (E_OUTOFMEMORY);
}

int _DbgBreakCount = 0;

//*****************************************************************************
// This function will handle ignore codes and tell the user what is happening.
//*****************************************************************************
int _DbgBreakCheck(
    LPCSTR      szFile, 
    int         iLine, 
    LPCSTR      szExpr)
{
    TCHAR       rcBuff[1024+_MAX_PATH];
    TCHAR       rcPath[_MAX_PATH];
    TCHAR       rcTitle[64];
    _DBGIGNOREDATA *psData;
    long        i;

    if (DebugBreakOnAssert())
    {        
        DebugBreak();
    }

    DBGIGNORE* pDBGIFNORE = GetDBGIGNORE();

    // Check for ignore all.
    for (i=0, psData = pDBGIFNORE->Ptr();  i<pDBGIFNORE->Count();  i++, psData++)
    {
        if (psData->iLine == iLine && _stricmp(psData->rcFile, szFile) == 0 && 
            psData->bIgnore == true)
            return (false);
    }

    // Give assert in output for easy access.
    WszGetModuleFileName(0, rcPath, NumItems(rcPath));
    swprintf(rcBuff, L"Assert failure(PID %d [0x%08x], Thread: %d [0x%x]): %hs\n"
                L"    File: %hs, Line: %d Image:\n%s\n", 
                GetCurrentProcessId(), GetCurrentProcessId(),
                GetCurrentThreadId(), GetCurrentThreadId(), 
                szExpr, szFile, iLine, rcPath);
    WszOutputDebugString(rcBuff);
    // Write out the error to the console
    printf("%S\n", rcBuff);

    LogAssert(szFile, iLine, szExpr);
    FlushLogging();         // make certain we get the last part of the log
    fflush(stdout);

    if (NoGuiOnAssert())
    {
        TerminateOnAssert();
    }

    if (DebugBreakOnAssert())
    {        
        return(true);       // like a retry
    }

    // Change format for message box.  The extra spaces in the title
    // are there to get around format truncation.
    swprintf(rcBuff, L"%hs\n\n%hs, Line: %d\n\nAbort - Kill program\nRetry - Debug\nIgnore - Keep running\n"
             L"\n\nImage:\n%s\n",
        szExpr, szFile, iLine, rcPath);
    swprintf(rcTitle, L"Assert Failure (PID %d, Thread %d/%x)        ", 
             GetCurrentProcessId(), GetCurrentThreadId(), GetCurrentThreadId());

    // Tell user there was an error.
    _DbgBreakCount++;
    int ret = WszMessageBoxInternal(NULL, rcBuff, rcTitle, 
            MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION | COMPLUS_MB_SERVICE_NOTIFICATION);
	--_DbgBreakCount;

    HMODULE hKrnl32;

    switch(ret)
    {
        // For abort, just quit the app.
        case IDABORT:
          TerminateProcess(GetCurrentProcess(), 1);
//        WszFatalAppExit(0, L"Shutting down");
        break;

        // Tell caller to break at the correct loction.
        case IDRETRY:

        hKrnl32 = WszLoadLibrary(L"kernel32.dll");
        _ASSERTE(hKrnl32 != NULL);

        if(hKrnl32)
        {
            typedef BOOL (WINAPI *t_pDbgPres)();
            t_pDbgPres pFcn = (t_pDbgPres) GetProcAddress(hKrnl32, "IsDebuggerPresent");

            // If this function is available, use it.
            if (pFcn)
            {
                if (pFcn())
                {
                    SetErrorMode(0);
                }
                else
    				LaunchJITDebugger();
            }

            FreeLibrary(hKrnl32);
        }

        return (true);

        // If we want to ignore the assert, find out if this is forever.
        case IDIGNORE:
        swprintf(rcBuff, L"Ignore the assert for the rest of this run?\nYes - Assert will never fire again.\nNo - Assert will continue to fire.\n\n%hs\nLine: %d\n",
            szFile, iLine);
        if (WszMessageBoxInternal(NULL, rcBuff, L"Ignore Assert Forever?", MB_ICONQUESTION | MB_YESNO | COMPLUS_MB_SERVICE_NOTIFICATION) != IDYES)
            break;

        if ((psData = pDBGIFNORE->Append()) == 0)
            return (false);
        psData->bIgnore = true;
        psData->iLine = iLine;
        strcpy(psData->rcFile, szFile);
        break;
    }

    return (false);
}

    // Get the timestamp from the PE file header.  This is useful 
unsigned DbgGetEXETimeStamp()
{
    static ULONG cache = 0;


    return cache;
}

// // //  
// // //  The following function
// // //  computes the binomial distribution, with which to compare 
// // //  hash-table statistics.  If a hash function perfectly randomizes
// // //  its input, one would expect to see F chains of length K, in a
// // //  table with N buckets and M elements, where F is
// // //
// // //    F(K,M,N) = N * (M choose K) * (1 - 1/N)^(M-K) * (1/N)^K.  
// // //
// // //  Don't call this with a K larger than 159.
// // //

#if !defined(NO_CRT)

#include <math.h>

#define MAX_BUCKETS_MATH 160

double Binomial (DWORD K, DWORD M, DWORD N)
{
    if (K >= MAX_BUCKETS_MATH)
        return -1 ;

    static double rgKFact [MAX_BUCKETS_MATH] ;
    DWORD i ;

    if (rgKFact[0] == 0)
    {
        rgKFact[0] = 1 ;
        for (i=1; i<MAX_BUCKETS_MATH; i++)
            rgKFact[i] = rgKFact[i-1] * i ;
    }

    double MchooseK = 1 ;

    for (i = 0; i < K; i++)
        MchooseK *= (M - i) ;

    MchooseK /= rgKFact[K] ;

    double OneOverNToTheK = pow (1./N, K) ;

    double QToTheMMinusK = pow (1.-1./N, M-K) ;

    double P = MchooseK * OneOverNToTheK * QToTheMMinusK ;

    return N * P ;
}

#endif // !NO_CRT

// Called from within the IfFail...() macros.  Set a breakpoint here to break on
// errors.
VOID DebBreak() 
{
  static int i = 0;  // add some code here so that we'll be able to set a BP
  i++;
}
VOID DebBreakHr(HRESULT hr) 
{
  static int i = 0;  // add some code here so that we'll be able to set a BP
  _ASSERTE(hr != (HRESULT) 0xcccccccc);
  i++;
}

VOID DbgAssertDialog(char *szFile, int iLine, char *szExpr)
{
    if (DebugBreakOnAssert())
    {        
        DebugBreak();
    }

    if (1 == _DbgBreakCheck(szFile, iLine, szExpr))
        _DbgBreak();
}

#endif // _DEBUG
