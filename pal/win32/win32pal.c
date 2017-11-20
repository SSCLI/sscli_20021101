/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    win32pal.c

Abstract:

    PAL for Win32 subsystem processes.

--*/

#include "win32pal.h"

BOOL VerifyThreadNotInCurrentProcess(HANDLE hThread);
BOOL VerifyThreadInCurrentProcess(HANDLE hThread);

LONG PALAPI PAL_ExceptionFilter(PAL_PEXCEPTION_POINTERS lpExceptionPointers);
extern PAL_LPTOP_LEVEL_EXCEPTION_FILTER SEH_CurrentTopLevelFilter;
extern DWORD SEH_Tls;

#define ROTOR_NAME_A "Rotor"
#define ROTOR_NAME_W L"Rotor"
#define PROCESS_PELOADER_FILENAMEW  L"clix.exe "
#define PROCESS_PELOADER_FILENAMEA  "clix.exe "

#define DIRECTORYSEPARATOR L"\\"
#define DIRECTORYSEPARATORLENGTH 1
LONG PalReferenceCount;

HINSTANCE hInstPal;
HCRYPTPROV hCryptProv;
char *NameManglerA;    // Ansi hashed path to the PAL
WCHAR *NameManglerW;   // Unicode hashed path to the PAL
int NameManglerLength; // count of characters in the hashed path, without the null

#if DBG
HANDLE LogFileHandle = INVALID_HANDLE_VALUE;
#endif


#if DBG
// This just toggles all bits in the HANDLE, so that the value is
// likely invalid.  The purpose of the Win32 code is to ensure that
// calls to PAL_LocalHandleToRemote and PAL_RemoteHandleToLocal are
// in place in the calling code so that non-Win32 PALs can correctly
// implement DuplicateHandle()
#define MangleHandle(h) ( (HANDLE)( ~(UINT_PTR)(h) ) )
#else
#define MangleHandle(h) (h)
#endif


// This is the main DLL entrypoint
BOOL
WINAPI
DllMain(
        HANDLE hDllHandle,
        DWORD dwReason,
        LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
    	hInstPal = (HINSTANCE)hDllHandle;
        DisableThreadLibraryCalls(hDllHandle);
    }
    return TRUE;
}

/*
   Register the eventlog source, ignoring errors, in case
   this is being run by a non-admin.  If the registration doesn't
   success, the messages will still be logged, but the
   event viewer will prefix them all with  "The description for 
   Event ID ( 0 ) in Source ( Rotor ) cannot be found."
*/
void
RegisterWithEventLog(
		     LPWSTR ModulePath)
{
    HKEY hKey;
    DWORD dwData;

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE,
		      L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Rotor",
		      &hKey) != 0) {
	// Failed to create the key - ignore the failure
	return;
    }

    RegSetValueExW(hKey,
		   L"EventMessageFile",
		   0,
		   REG_SZ,
		   (LPBYTE)ModulePath,
		   (wcslen(ModulePath)+1)*sizeof(WCHAR));

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
             EVENTLOG_INFORMATION_TYPE; 

    RegSetValueExW(hKey,
		   L"TypesSupported",
		   0,
		   REG_DWORD,
		   (LPBYTE)&dwData,
		   sizeof(DWORD));

    RegCloseKey(hKey);
}

/*
    Initialize NameManglerA and NameManglerW, which are hashed versions
    of the pathname of the PAL.  These strings can be used as either
    portions of filenames or as portions of named-object names.  Names
    are mangled in order to support side-by-side Rotors running
    concurrently - each Rotor (identified by a different path to
    rotor_pal.dll) is automatically side-by-side with other Rotors.
*/
BOOL
InitializeObjectNameMangler(
		            LPWSTR ModulePath)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BOOL bRet = FALSE;
    DWORD cbCurrentValue;
    DWORD dwCount = sizeof(DWORD);
    BYTE *pbCurrentValue;
    char *pA;
    WCHAR *pW;

    // Lowercase the path before creating the hash
    _wcslwr(ModulePath);

    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ||
        (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) ||
        (!CryptHashData(hHash, (BYTE *)ModulePath, wcslen(ModulePath)*sizeof(WCHAR), 0)) ||
        (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *) &cbCurrentValue, 
                            &dwCount, 0))) {
        goto LExit;
    }

    pbCurrentValue = _alloca(cbCurrentValue);
    if(!CryptGetHashParam(hHash, 
                          HP_HASHVAL, 
                          pbCurrentValue, 
                          &cbCurrentValue, 
                          0)) {
        goto LExit;
    }

    NameManglerA = malloc(cbCurrentValue*2+sizeof(ROTOR_NAME_A));
    NameManglerW = malloc(cbCurrentValue*2*sizeof(WCHAR)+sizeof(ROTOR_NAME_W));
    if (!NameManglerA || !NameManglerW) {
	goto LExit;
    }

    memcpy(NameManglerA, ROTOR_NAME_A, sizeof(ROTOR_NAME_A));
    memcpy(NameManglerW, ROTOR_NAME_W, sizeof(ROTOR_NAME_W));
    pA = &NameManglerA[sizeof(ROTOR_NAME_A)-1];
    pW = &NameManglerW[sizeof(ROTOR_NAME_W)/sizeof(WCHAR)-1];

    // Convert the binary hash blob into a string that can be used as
    // part of a filename.
    for (dwCount=0; dwCount < cbCurrentValue; ++dwCount) {
	*pW = *pA = (pbCurrentValue[dwCount] & 0x0f)+'a';
	pA++;
	pW++;
	*pW = *pA = (pbCurrentValue[dwCount] >> 4)+'a';
	pA++;
	pW++;
    }
    *pA = '\0';
    *pW = L'\0';
    NameManglerLength = pA - NameManglerA; // length doesn't include the null

    bRet=TRUE;

LExit:
    if (hHash)
        CryptDestroyHash(hHash);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    if (!bRet) {
	free(NameManglerA);
	NameManglerA = NULL;
	free(NameManglerW);
	NameManglerW = NULL;
    }

    return bRet;
}

BOOL
MangleObjectNameA(
		  LPCSTR *lplpObjectName,
		  LPSTR lpMangledObjectName)
{
    size_t ObjectNameLength;
    LPCSTR lpObjectName;
    char *pSlash;

    lpObjectName = *lplpObjectName;
    if (!lpObjectName) {
	// No object name, so no work to do.
	return TRUE;
    }

    ObjectNameLength = strlen(lpObjectName);
    if (ObjectNameLength+NameManglerLength+1 >= MAX_PATH) {
	SetLastError(ERROR_FILENAME_EXCED_RANGE);
	return FALSE;
    }

    *lplpObjectName = lpMangledObjectName;

    pSlash = strchr(lpObjectName, '\\');
    if (pSlash) {
	size_t PathPartLength;

	// The mangling needs to be done after the Global\\ or Local\\ portion
        PathPartLength = pSlash-lpObjectName+1;
	memcpy(lpMangledObjectName, lpObjectName, PathPartLength);
	lpObjectName+=PathPartLength;
	lpMangledObjectName+=PathPartLength;
    }
    strncpy(lpMangledObjectName, NameManglerA, NameManglerLength);
    strcpy(lpMangledObjectName+NameManglerLength, lpObjectName);

    return TRUE;
}

BOOL
MangleObjectNameW(
		  LPCWSTR *lplpObjectName,
		  LPWSTR lpMangledObjectName)
{
    size_t ObjectNameLength;
    LPCWSTR lpObjectName;
    WCHAR *pSlash;

    lpObjectName = *lplpObjectName;
    if (!lpObjectName) {
	// No object name, so no work to do.
	return TRUE;
    }

    ObjectNameLength = wcslen(lpObjectName);
    if (ObjectNameLength+NameManglerLength+1 >= MAX_PATH) {
	SetLastError(ERROR_FILENAME_EXCED_RANGE);
	return FALSE;
    }

    *lplpObjectName = lpMangledObjectName;

    pSlash = wcschr(lpObjectName, L'\\');
    if (pSlash) {
	size_t PathPartLength;

	// The mangling needs to be done after the Global\\ or Local\\ portion
        PathPartLength = pSlash-lpObjectName+1;
	memcpy(lpMangledObjectName, lpObjectName, PathPartLength*sizeof(WCHAR));
	lpObjectName+=PathPartLength;
	lpMangledObjectName+=PathPartLength;
    }
    wcsncpy(lpMangledObjectName, NameManglerW, NameManglerLength);
    wcscpy(lpMangledObjectName+NameManglerLength, lpObjectName);

    return TRUE;
}


int
PALAPI
PAL_Startup(
            int argc,
            char *argv[])
{
    int RetVal = -1;
    HMODULE hMod;
    WCHAR ModulePath[_MAX_PATH];

    hMod = GetModuleHandleW(L"rotor_pal.dll");
    if (!hMod) {
	return RetVal;
    }

    if (!GetModuleFileNameW(hMod, 
			    ModulePath, 
			    sizeof(ModulePath)/sizeof(WCHAR))) {
	return RetVal;
    }

    SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);

    RegisterWithEventLog(ModulePath);

    if (!InitializeObjectNameMangler(ModulePath)) {
	return RetVal;
    }

#if DBG
        //
        // Do one-time initialization of the API tracing mechanism
        //
    {
        DWORD dw;
        WCHAR buffer[_MAX_PATH];

        dw = GetEnvironmentVariableW(L"PAL_API_TRACING", 
                                     buffer, 
                                     sizeof(buffer)/sizeof(WCHAR));
        if (dw != 0 && dw <= sizeof(buffer)/sizeof(WCHAR)) {
            // Environment variable exists and fit in the buffer
            if (wcscmp(buffer, L"stdout") == 0) {
                LogFileHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            } else if (wcscmp(buffer, L"stderr") == 0) {
                LogFileHandle = GetStdHandle(STD_ERROR_HANDLE);
            } else if (wcscmp(buffer, L"debugger") == 0) {
                LogFileHandle = 0;
            } else {
                LogFileHandle = CreateFileW(buffer, 
                                            GENERIC_WRITE,
                                            FILE_SHARE_READ,
                                            NULL,
                                            CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL);
            }
        }
    }
#endif //DBG

    // Note: MSVCRT has already registered their exception filter at this point
    // SEH_CurrentTopLevelFilter won't be NULL

    SEH_CurrentTopLevelFilter = (PAL_LPTOP_LEVEL_EXCEPTION_FILTER)
        SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)PAL_ExceptionFilter);

    if ((SEH_Tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
        goto LExit;
    }

    PALASSERT(PAL_TRY_LOCAL_SIZE >= sizeof(jmp_buf));

    // exceptions masked, round to nearest, 53 bit precision.
    _controlfp(_MCW_EM | _RC_NEAR | _PC_53 | _IC_PROJECTIVE | _DN_SAVE, 
               _MCW_EM | _MCW_RC | _MCW_PC | _MCW_IC | _MCW_DN);

    RetVal = 0;

LExit:
    return RetVal;
}

void
PALAPI
PAL_Shutdown(
              void)
{
    PAL_LPTOP_LEVEL_EXCEPTION_FILTER PreviousFilter;

    if (hCryptProv != NULL) {
        CryptReleaseContext(hCryptProv, 0);
        hCryptProv = NULL;
    }

    free(NameManglerA);
    NameManglerA = NULL;
    free(NameManglerW);
    NameManglerW = NULL;

    if (SEH_Tls != TLS_OUT_OF_INDEXES) {
        TlsFree(SEH_Tls);
    }

    // Reset the global exception filter
    PreviousFilter = (PAL_LPTOP_LEVEL_EXCEPTION_FILTER)
        SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)SEH_CurrentTopLevelFilter);

#if DBG
    // Do this after the LOGAPI, so the output gets logged
    if (LogFileHandle != INVALID_HANDLE_VALUE) {
        //
        // Refcount is zero and logging is enabled - close the
        // file handle now in preparation for unload.
        //
        if (LogFileHandle) {
            CloseHandle(LogFileHandle);
        }
        LogFileHandle = INVALID_HANDLE_VALUE;
    }
#endif
}

PALIMPORT
int
PALAPI 
PAL_Initialize(
            int argc,
            char *argv[])
{    
    int RetVal = 0;
    LONG RefCount;

    RefCount = InterlockedIncrement(&PalReferenceCount);

    if (RefCount == 1) {
        RetVal = PAL_Startup(argc, argv);
    }

    LOGAPI("PAL_Initialize(argc=0x%x, argv=%p)\n", argc, argv);

    LOGAPI("PAL_Initialize returns int 0x%x\n", RetVal);

    return RetVal;
}

PALIMPORT
void
PALAPI
PAL_Terminate(
              void)
{
    LONG RefCount;

    LOGAPI("PAL_Terminate()\n");

    //
    // log this before the InterlockedDecrement, so the assertion in the
    // API logging code isn't trigged by a PAL_Initialize/PAL_Terminate
    // mismatch
    //
    LOGAPI("PAL_Terminate returns void\n");

    RefCount = InterlockedDecrement(&PalReferenceCount);

    //
    // if this hits, the host has a refcounting bug - the
    // count has underflowed.  This is not a PAL bug.
    //
    PALASSERT(RefCount >= 0);

    if (RefCount == 0) {
        PAL_Shutdown();
    }
}

PALIMPORT
void
PALAPI
PAL_InitializeDebug(
              void)
{
   return; // Nothing to do on this platform
}

PALIMPORT
BOOL
PALAPI
PAL_GetUserConfigurationDirectoryW(
                                   OUT LPWSTR lpDirectoryName,
                                   IN UINT cbDirectoryName)
{
    static BOOL bCreated = FALSE;
    BOOL b = FALSE;
    DWORD dw;

    LOGAPI("PAL_GetUserConfigurationDirectoryW(lpDirectoryName=%p, "
           "cbDirectoryName=0x%x)\n", lpDirectoryName, cbDirectoryName);

    dw = GetEnvironmentVariableW(L"USERPROFILE",
                                 lpDirectoryName,
                                 cbDirectoryName);

    if (dw == 0 || dw >= cbDirectoryName) {
        // the call failed because the variable doesn't exist, or the
        // lpDirectoryName buffer is too small to hold the result
        goto LExit;
    }

    // append the hashed path to the PAL DLL to the path
    if (wcslen(lpDirectoryName) + NameManglerLength + 2*DIRECTORYSEPARATORLENGTH >= 
        cbDirectoryName) {
        goto LExit;
    }

    wcscat(lpDirectoryName, DIRECTORYSEPARATOR);
    wcscat(lpDirectoryName, ROTOR_NAME_W);
    // try to create the directory on the first call
    if (!bCreated) {
        CreateDirectoryW(lpDirectoryName, NULL);
    }
    wcscat(lpDirectoryName, DIRECTORYSEPARATOR);
    wcscat(lpDirectoryName, NameManglerW+sizeof(ROTOR_NAME_W)/sizeof(WCHAR)-1);
    if (!bCreated) {
        CreateDirectoryW(lpDirectoryName, NULL);
        bCreated = TRUE;
    }

    b = TRUE;

LExit:
    LOGAPI("PAL_GetUserConfigurationDirectoryW returns BOOL %d (%S)\n", 
           b, (b) ? lpDirectoryName : L"FAILED");
    return b;
}


PALIMPORT
BOOL
PALAPI
PAL_GetPALDirectoryW(
		     OUT LPWSTR lpDirectoryName,
		     IN UINT cbDirectoryName)
{
    BOOL Ret;

    LOGAPI("PAL_GetPALDirectoryW(lpDirectoryName=%p, cbDirectoryName=0x%x)\n",
	   lpDirectoryName, cbDirectoryName);

    *lpDirectoryName = '\0';
    if (!hInstPal) {
	//
	// The call happened before DllMain, so the PAL's hinstance isn't
	// known yet
	//
        SetLastError(ERROR_MOD_NOT_FOUND);
	Ret = FALSE;
    } else {
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];

	if (GetModuleFileNameW(hInstPal,
		  	       lpDirectoryName,
			       cbDirectoryName)) {
	    _wsplitpath(lpDirectoryName, szDrive, szDir, NULL, NULL);
	    wcsncpy(lpDirectoryName, szDrive, cbDirectoryName);
	    wcsncat(lpDirectoryName, szDir, cbDirectoryName);
	    // szDir contains the trailing '\' character, so the returned
	    // string will always contain the trailing '\'.
	    Ret = TRUE;
	} else {
	    Ret = FALSE;
	}
    }

    LOGAPI("PAL_GetPALDirectoryW returns BOOL %d (lpDirectoryName=%p %S)\n",
	   Ret, lpDirectoryName, lpDirectoryName);
    return Ret;
}


PALIMPORT
BOOL
PALAPI
PAL_Random(
        IN BOOL bStrong,
        IN OUT LPVOID lpBuffer,
        IN DWORD dwLength)
{
    BOOL Ret;
    HCRYPTPROV hProv;

    LOGAPI("PAL_Random(bStrong=%d, lpBuffer=%p, dwLength=0x%x)\n", bStrong, lpBuffer, dwLength);

    if (hCryptProv == NULL)
    {
        Ret = CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);

        if (!Ret)
            goto LExit;

        if (InterlockedCompareExchangePointer((PVOID*)&hCryptProv, (PVOID)hProv, NULL) != NULL)
        {
            // somebody beat us to it
            CryptReleaseContext(hProv, 0);
        }
    }

    Ret = CryptGenRandom(hCryptProv, dwLength, lpBuffer);

LExit:
    LOGAPI("PAL_Random returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT 
PAL_FILE * 
__cdecl 
PAL_get_stdout()
{
    FILE *Ret;

    LOGAPI("PAL_get_stdout\n");

    Ret = stdout;

    LOGAPI("PAL_get_stdout returns FILE * %p\n", Ret);
    return (PAL_FILE *)Ret;
}

PALIMPORT 
PAL_FILE * 
__cdecl 
PAL_get_stdin()
{
    FILE *Ret;

    LOGAPI("PAL_get_stdin\n");

    Ret = stdin;

    LOGAPI("PAL_get_stdin returns FILE * %p\n", Ret);
    return (PAL_FILE *)Ret;
}

PALIMPORT 
PAL_FILE * 
__cdecl 
PAL_get_stderr()
{
    FILE *Ret;

    LOGAPI("PAL_get_stderr\n");

    Ret = stderr;

    LOGAPI("PAL_get_stderr returns FILE * %p\n", Ret);
    return (PAL_FILE *)Ret;
}

#define LIB_SUFFIX          L".dll"
#define LIB_SUFFIX_LENGTH   4

PALIMPORT
PAL_HMODULE
PALAPI
PAL_RegisterLibraryW(
	     IN LPCWSTR lpLibFileName)
{
    HMODULE Ret;
    LPWSTR libFileName;

    LOGAPI("PAL_RegisterLibraryW(lpLibFileName=%p (%S))\n", 
            lpLibFileName, lpLibFileName);

    libFileName = (LPWSTR) malloc((wcslen(lpLibFileName) +
                                LIB_SUFFIX_LENGTH + 1) * sizeof(WCHAR));
    if (libFileName == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    wcscpy(libFileName, lpLibFileName);
    wcscat(libFileName, LIB_SUFFIX);
    Ret = GetModuleHandleW(libFileName);
    free(libFileName);

    LOGAPI("PAL_RegisterLibraryW returns HMODULE %p\n", Ret);
    return (PAL_HMODULE)Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_UnregisterLibraryW(
	     IN OUT PAL_HMODULE hLibModule)
{
    BOOL Ret;

    LOGAPI("PAL_UnregisterLibraryW(hLibModule=%p)\n", hLibModule);

    Ret = TRUE;

    LOGAPI("PAL_UnregisterLibraryW returns BOOL %d\n", Ret);
    return Ret;
}


static const char * const ResponseStrings[7] = {
    "Ok",       /* IDOK */
    "Cancel",   /* IDCANCEL */
    "Abort",    /* IDABORT */
    "Retry",    /* IDRETRY */
    "Ignore",   /* IDIGNORE */
    "Yes",      /* IDYES */
    "No"        /* IDNO */
};

PALIMPORT
int
PALAPI
PAL_MessageBoxW(
            IN LPVOID hWnd,  // NOTE: diff from winuser.h
            IN LPCWSTR lpText,
            IN LPCWSTR lpCaption,
            IN UINT uType)
{
    int Ret;
    HANDLE hEventLog;
    WORD LogType;
    const char *ResponseString;

    LOGAPI("MessageBoxW(hWnd=%p, lpText=%p (%S), lpCaption=%p (%S),"
           " uType=0x%x)\n", 
           hWnd, lpText, lpText, lpCaption, lpCaption, uType);

    PALASSERT(hWnd == NULL);

    switch (uType & MB_TYPEMASK) {
    case MB_OK:
        Ret = IDOK;
        break;

    case MB_OKCANCEL:
        Ret = IDCANCEL;
        break;

    case MB_ABORTRETRYIGNORE:
        Ret = IDABORT;
        break;

    case MB_YESNO:
        Ret = IDNO;
        break;

    case MB_RETRYCANCEL:
        Ret = IDCANCEL;
        break;

    default:
        PALASSERT(FALSE);
        Ret = IDOK;
        break;
    }

    ResponseString = ResponseStrings[Ret-IDOK];

    switch (uType & MB_ICONMASK) {
    case MB_ICONEXCLAMATION:
    case 0:
        LogType = EVENTLOG_WARNING_TYPE;
        break;

    case MB_ICONINFORMATION:
        LogType = EVENTLOG_INFORMATION_TYPE;
        break;

    case MB_ICONSTOP:
        LogType = EVENTLOG_ERROR_TYPE;
        break;

    case MB_ICONQUESTION:
        LogType = EVENTLOG_INFORMATION_TYPE;
        break;

    default:
        PALASSERT(FALSE);
        LogType = EVENTLOG_ERROR_TYPE;
        break;
    }

    hEventLog= RegisterEventSourceA(NULL, "Rotor");
    if (hEventLog != NULL) {
        char buf[1024];
        char *bufptr = buf;

        _snprintf(buf, sizeof(buf), "Pid %x Tid %x %S --- %S -- %s",
                  GetCurrentProcessId(),
                  GetCurrentThreadId(),
                  lpText,
                  lpCaption,
                  ResponseString);

        // be sure to null-terminate the buffer
        buf[sizeof(buf)-1] = '\0';

        ReportEventA(hEventLog, LogType, 0, LogType, NULL, 1, 0, &bufptr, NULL);
        DeregisterEventSource(hEventLog);
    }

    if (!getenv("PAL_DISABLE_MESSAGEBOX")) {
        //
        // Call real user32!MessageBoxW - MessageBoxes are used to display
        // CLR assertions.
        //
        Ret = MessageBoxW(hWnd, lpText, lpCaption, uType);
    }

    LOGAPI("MessageBoxW returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT 
int * 
__cdecl
PAL_errno(
    void
    )
{
    int *Ret;

    LOGAPI("PAL_errno(void)\n");

    Ret = _errno();

    LOGAPI("PAL_errno returns %p (errno=%d)\n", Ret, *Ret);
    return Ret;
}

PALIMPORT
RHANDLE
PALAPI
PAL_LocalHandleToRemote(
    IN HANDLE hLocal
)
{
    RHANDLE Ret;

    LOGAPI("PAL_LocalHandleToRemote(hLocal=%p)\n", hLocal);

    Ret = (RHANDLE)MangleHandle(hLocal);

    LOGAPI("PAL_LocalHandleToRemote returns HANDLE %p\n", Ret);
    return Ret;
}




PALIMPORT
HANDLE
PALAPI
PAL_RemoteHandleToLocal(
    IN RHANDLE hRemote
)
{
    HANDLE Ret;

    LOGAPI("PAL_RemoteHandleToLocal(hRemote=%p)\n", hRemote);
  
    // demangle the remote handle
    Ret = MangleHandle((HANDLE)hRemote);

    LOGAPI("PAL_RemoteHandleToLocal returns HANDLE %p\n", Ret);
    return Ret;
}


#if DBG

struct _startupinfo
{
        int newmode;
};

// internal function exported from msvcrt.dll
__declspec(dllimport) int __cdecl __getmainargs(int *pargc, char ***pargv, char ***penvp, 
    int dowildcard, struct _startupinfo * startinfo);

PALIMPORT 
int
__cdecl 
PAL___getmainargs(int *pargc, char ***pargv, char ***penvp, 
    int dowildcard, struct _startupinfo * startinfo)
{
    int Ret;

    LOGAPI("__getmainargs(pargc=%p, pargv=%p, penvp=%p, dowildcard=%d, startinfo=%p\n", 
        pargc, pargv, penvp, dowildcard, startinfo);

    Ret = __getmainargs(pargc, pargv, penvp, dowildcard, startinfo);

    LOGAPI("__getmainargs returns int 0x%x, argc=%d, argv=%p, envp=%p\n", Ret, *pargc, *pargv, *penvp);
    return Ret;
}

// internal function exported from msvcrt.dll
__declspec(dllimport) int __cdecl __wgetmainargs(int *pargc, wchar_t ***pargv, wchar_t ***penvp, 
    int dowildcard, struct _startupinfo * startinfo);

PALIMPORT 
int
__cdecl 
PAL___wgetmainargs(int *pargc, wchar_t ***pargv, wchar_t ***penvp, 
    int dowildcard, struct _startupinfo * startinfo)
{
    int Ret;

    LOGAPI("__wgetmainargs(pargc=%p, pargv=%p, penvp=%p, dowildcard=%d, startinfo=%p\n", 
        pargc, pargv, penvp, dowildcard, startinfo);

    Ret = __wgetmainargs(pargc, pargv, penvp, dowildcard, startinfo);

    LOGAPI("__wgetmainargs returns int 0x%x, argc=%d, argv=%p, envp=%p\n", Ret, *pargc, *pargv, *penvp);
    return Ret;
}

PALIMPORT
LPSTR
PALAPI
PAL_CharNextA(
            IN LPCSTR lpsz)
{
    LPSTR Ret;

    LOGAPI("CharNextA(lpsz=%p)\n", lpsz);

    Ret = CharNextA(lpsz);

    LOGAPI("CharNextA returns LPCSTR %p\n", Ret);
    return Ret;
}

PALIMPORT
LPSTR
PALAPI
PAL_CharNextExA(
            IN WORD CodePage,
            IN LPCSTR lpCurrentChar,
            IN DWORD dwFlags)
{
    LPSTR Ret;

    LOGAPI("CharNextExA(CodePage=%d, lpCurrentChar=%p, dwFlags=0x%x)\n", 
           CodePage, lpCurrentChar, dwFlags);

    PALASSERT(CP_ACP==CodePage || CP_OEMCP == CodePage);
    PALASSERT(dwFlags == 0);

    Ret = CharNextExA(CodePage, lpCurrentChar, dwFlags);

    LOGAPI("CharNextExA returns LPSTR %p\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPIV
PAL_wsprintfA(
          OUT LPSTR buf,
          IN LPCSTR fmt,
          ...)
{
    int Ret;
    va_list valist;

    LOGAPI("wsprintfA(buf=%p, fmt=%p, ...)\n", buf, fmt);

    va_start(valist, fmt);
    Ret = wvsprintfA(buf, fmt, valist);
    va_end(valist);

    LOGAPI("wsprintfA returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPIV
PAL_wsprintfW(
          OUT LPWSTR buf,
          IN LPCWSTR fmt,
          ...)
{
     int Ret;
     va_list valist;

     LOGAPI("wsprintfW(buf=%p, fmt=%p, ...)\n", buf, fmt);

     va_start(valist, fmt);
     Ret = wvsprintfW(buf, fmt, valist);
     va_end(valist);

     LOGAPI("wsprintfW returns int 0x%x\n", Ret);
     return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetConsoleCtrlHandler(
    IN PHANDLER_ROUTINE HandlerRoutine,
    IN BOOL Add)
{
    BOOL Ret;

    LOGAPI("SetConsoleCtrlHandler(HandlerRoutine=%p, Add=%d)\n",
	   HandlerRoutine, Add);

    PALASSERT(HandlerRoutine != NULL);

    Ret = SetConsoleCtrlHandler(HandlerRoutine, Add);

    LOGAPI("SetConsoleCtrlHandler return BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GenerateConsoleCtrlEvent(
    IN DWORD dwCtrlEvent,
    IN DWORD dwProcessGroupId
    )
{
    BOOL Ret;

    LOGAPI("GenerateConsoleCtrlEvent(dwCtrlEvent=0x%x, "
           "dwProcessGroupId=0x%x)\n", dwCtrlEvent, dwProcessGroupId);

    PALASSERT(dwProcessGroupId == 0);

    Ret = GenerateConsoleCtrlEvent(dwCtrlEvent, dwProcessGroupId);

    LOGAPI("GenerateConsoleCtrlEvent returns BOOL %d\n", Ret);
    return Ret;
}


PALIMPORT
BOOL
PALAPI
PAL_AreFileApisANSI(
    VOID)
{
    BOOL Ret;

    LOGAPI("AreFileApisANSI()\n");

    Ret = AreFileApisANSI();

    LOGAPI("AreFileApisANSI returns BOOL %d\n", Ret);
    return Ret;
}

#define PAL_LEGAL_DESIRED_ACCESS (GENERIC_READ|GENERIC_WRITE)
#define PAL_LEGAL_SHARE_MODE (FILE_SHARE_READ| \
                              FILE_SHARE_WRITE| \
                              FILE_SHARE_DELETE)
#define PAL_LEGAL_FLAGS_ATTRIBS (FILE_ATTRIBUTE_NORMAL| \
                                 FILE_FLAG_SEQUENTIAL_SCAN| \
                                 FILE_FLAG_WRITE_THROUGH| \
                                 FILE_FLAG_NO_BUFFERING| \
                                 FILE_FLAG_RANDOM_ACCESS| \
                                 FILE_FLAG_BACKUP_SEMANTICS)

PALIMPORT
HANDLE
PALAPI
PAL_CreateFileA(
    IN LPCSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN PAL_LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    )
{
    HANDLE Ret;

    LOGAPI("CreateFileA(lpFileName=%p (%s), dwDesiredAccess=0x%x, "
           "dwShareMode=0x%x, lpSecurityAttributes=%p, "
           "dwCreationDisposition=0x%x, dwFlagsAndAttributes=0x%x, "
           "hTemplateFile=%p)\n", lpFileName, lpFileName, dwDesiredAccess, 
           dwShareMode, lpSecurityAttributes, dwCreationDisposition, 
           dwFlagsAndAttributes, hTemplateFile);

    PALASSERT(strncmp(lpFileName, "\\\\.\\", 4) != 0);  // not an NT pathname
    PALASSERT((dwDesiredAccess & PAL_LEGAL_DESIRED_ACCESS) == dwDesiredAccess);
    PALASSERT((dwShareMode & PAL_LEGAL_SHARE_MODE) == dwShareMode);
    PALASSERT(lpSecurityAttributes == NULL ||
              (lpSecurityAttributes->bInheritHandle == TRUE &&
               lpSecurityAttributes->lpSecurityDescriptor == NULL));
    PALASSERT(dwCreationDisposition == CREATE_NEW ||
              dwCreationDisposition == CREATE_ALWAYS ||
              dwCreationDisposition == OPEN_EXISTING ||
              dwCreationDisposition == OPEN_ALWAYS ||
              dwCreationDisposition == TRUNCATE_EXISTING);
    PALASSERT((dwFlagsAndAttributes & PAL_LEGAL_FLAGS_ATTRIBS) == 
              dwFlagsAndAttributes);
    PALASSERT(hTemplateFile == NULL);

    Ret = CreateFileA(lpFileName,
                      dwDesiredAccess,
                      dwShareMode,
                      (LPSECURITY_ATTRIBUTES)lpSecurityAttributes,
                      dwCreationDisposition,
                      dwFlagsAndAttributes,
                      hTemplateFile);

    LOGAPI("CreateFileA returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_CreateFileW(
    IN LPCWSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN PAL_LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    )
{
    HANDLE Ret;

    LOGAPI("CreateFileW(lpFileName=%p (%S), dwDesiredAccess=0x%x, "
           "dwShareMode=0x%x, lpSecurityAttributes=%p, "
           "dwCreationDisposition=0x%x, dwFlagsAndAttributes=0x%x, "
           "hTemplateFile=%p)\n", lpFileName, lpFileName, dwDesiredAccess, 
           dwShareMode, lpSecurityAttributes, dwCreationDisposition, 
           dwFlagsAndAttributes, hTemplateFile);

    PALASSERT(wcsncmp(lpFileName, L"\\\\.\\", 4) != 0);  // not an NT pathname
    PALASSERT((dwDesiredAccess & PAL_LEGAL_DESIRED_ACCESS)==dwDesiredAccess);
    PALASSERT((dwShareMode & PAL_LEGAL_SHARE_MODE) == dwShareMode);
    PALASSERT(lpSecurityAttributes == NULL ||
              (lpSecurityAttributes->bInheritHandle == TRUE &&
               lpSecurityAttributes->lpSecurityDescriptor == NULL));
    PALASSERT(dwCreationDisposition == CREATE_NEW ||
              dwCreationDisposition == CREATE_ALWAYS ||
              dwCreationDisposition == OPEN_EXISTING ||
              dwCreationDisposition == OPEN_ALWAYS ||
              dwCreationDisposition == TRUNCATE_EXISTING);
    PALASSERT((dwFlagsAndAttributes & PAL_LEGAL_FLAGS_ATTRIBS) == 
              dwFlagsAndAttributes);
    PALASSERT(hTemplateFile == NULL);

    Ret = CreateFileW(lpFileName,
                      dwDesiredAccess,
                      dwShareMode,
                      (LPSECURITY_ATTRIBUTES)lpSecurityAttributes,
                      dwCreationDisposition,
                      dwFlagsAndAttributes,
                      hTemplateFile);

    LOGAPI("CreateFileW returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_LockFile(
    IN HANDLE hFile,
    IN DWORD dwFileOffsetLow,
    IN DWORD dwFileOffsetHigh,
    IN DWORD nNumberOfBytesToLockLow,
    IN DWORD nNumberOfBytesToLockHigh
    )
{
    BOOL Ret;

    LOGAPI("LockFile(hFile=%p, dwFileOffsetLow=0x%x, dwFileOffsetHigh=0x%x, "
           "nNumberOfBytesToLockLow=0x%x, nNumberOfBytesToLockHigh=0x%x)\n",
	   hFile, dwFileOffsetLow, dwFileOffsetHigh,
	   nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh);

    Ret = LockFile(hFile, dwFileOffsetLow, dwFileOffsetHigh,
	           nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh);

    LOGAPI("LockFile returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_UnlockFile(
    IN HANDLE hFile,
    IN DWORD dwFileOffsetLow,
    IN DWORD dwFileOffsetHigh,
    IN DWORD nNumberOfBytesToUnlockLow,
    IN DWORD nNumberOfBytesToUnlockHigh
    )
{
    BOOL Ret;

    LOGAPI("UnlockFile(hFile=%p, dwFileOffsetLow=0x%x, dwFileOffsetHigh=0x%x, "
           "nNumberOfBytesToUnlockLow=0x%x, nNumberOfBytesToUnlockHigh=0x%x)\n",
	   hFile, dwFileOffsetLow, dwFileOffsetHigh,
	   nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh);

    Ret = UnlockFile(hFile, dwFileOffsetLow, dwFileOffsetHigh,
	             nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh);

    LOGAPI("UnlockFile returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_SearchPathA(
    IN LPCSTR lpPath,
    IN LPCSTR lpFileName,
    IN LPCSTR lpExtension,
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer,
    OUT LPSTR *lpFilePart
    )
{
    DWORD Ret;

    LOGAPI("SearchPathA(lpPath=%p (%s), lpFileName=%p (%s), "
           "lpExtension=%p, nBufferLength=0x%x, lpBuffer=%p, "
           "lpFilePart=%p)\n", lpPath, lpPath, lpFileName, lpFileName,
           lpExtension, nBufferLength, lpBuffer, lpFilePart);

    PALASSERT(lpPath != NULL);
    PALASSERT(lpExtension == NULL);

    Ret = SearchPathA(lpPath, lpFileName, lpExtension, nBufferLength,
		      lpBuffer, lpFilePart);

    LOGAPI("SearchPathA returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_SearchPathW(
    IN LPCWSTR lpPath,
    IN LPCWSTR lpFileName,
    IN LPCWSTR lpExtension,
    IN DWORD nBufferLength,
    OUT LPWSTR lpBuffer,
    OUT LPWSTR *lpFilePart
    )
{
    DWORD Ret;

    LOGAPI("SearchPathW(lpPath=%p (%S), lpFileName=%p (%S), "
           "lpExtension=%p, nBufferLength=0x%x, lpBuffer=%p, "
           "lpFilePart=%p)\n", lpPath, lpPath, lpFileName, lpFileName,
           lpExtension, nBufferLength, lpBuffer, lpFilePart);

    PALASSERT(lpPath != NULL);
    PALASSERT(lpExtension == NULL);

    Ret = SearchPathW(lpPath, lpFileName, lpExtension, nBufferLength,
		      lpBuffer, lpFilePart);

    LOGAPI("SearchPathW returns DWORD 0x%x\n", Ret);
    return Ret;
}



PALIMPORT
BOOL
PALAPI
PAL_CreatePipe(
    OUT PHANDLE hReadPipe,
    OUT PHANDLE hWritePipe,
    IN PAL_LPSECURITY_ATTRIBUTES lpPipeAttributes,
    IN DWORD nSize
    )
{
    BOOL Ret;

    LOGAPI("CreatePipe(hReadPipe=%p, hWritePipe=%p, lpPipeAttributes=%p "
	   "nSize=0x%x)\n",
	   hReadPipe, hWritePipe, lpPipeAttributes, nSize);

    PALASSERT(lpPipeAttributes && 
              lpPipeAttributes->lpSecurityDescriptor == NULL &&
	      lpPipeAttributes->bInheritHandle);

    Ret = CreatePipe(hReadPipe, 
                     hWritePipe, 
                     (LPSECURITY_ATTRIBUTES)lpPipeAttributes, 
                     nSize);

    LOGAPI("CreatePipe returns BOOL %d\n", Ret);
    return Ret;
}


PALIMPORT
BOOL
PALAPI
PAL_CopyFileA(
    IN LPCSTR lpExistingFileName,
    IN LPCSTR lpNewFileName,
    IN BOOL bFailIfExists
    )
{
    BOOL Ret;

    LOGAPI("CopyFileA(lpExistingFileName=%p (%s), lpNewFileName=%p (%s), "
           "bFailIfExists=%d)\n", lpExistingFileName, lpExistingFileName, 
           lpNewFileName, lpNewFileName, bFailIfExists);

    Ret = CopyFileA(lpExistingFileName, lpNewFileName, bFailIfExists);

    LOGAPI("CopyFileA returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_CopyFileW(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName,
    IN BOOL bFailIfExists
    )
{
    BOOL Ret;

    LOGAPI("CopyFileW(lpExistingFileName=%p (%S), lpNewFileName=%p (%S), "
           "bFailIfExists=%d)\n", lpExistingFileName, lpExistingFileName, 
           lpNewFileName, lpNewFileName, bFailIfExists);

    Ret = CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);

    LOGAPI("CopyFileW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_DeleteFileA(
    IN LPCSTR lpFileName
    )
{
    BOOL Ret;

    LOGAPI("DeleteFileA(lpFileName=%p (%s)\n", lpFileName, lpFileName);

    Ret = DeleteFileA(lpFileName);

    LOGAPI("DeleteFileA returns BOOL %d\n", Ret);
    return Ret;
}


PALIMPORT
BOOL
PALAPI
PAL_DeleteFileW(
    IN LPCWSTR lpFileName
    )
{
    BOOL Ret;

    LOGAPI("DeleteFileW(lpFileName=%d (%S))\n", lpFileName, lpFileName);

    Ret = DeleteFileW(lpFileName);

    LOGAPI("DeleteFileW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_MoveFileW(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName
    )
{
    BOOL Ret;

    LOGAPI("MoveFileW(lpExistingFileName=%p (%S), lpNewFileName=%p (%S))\n",
           lpExistingFileName, lpExistingFileName, lpNewFileName, 
           lpNewFileName);

    Ret = MoveFileW(lpExistingFileName, lpNewFileName);

    LOGAPI("MoveFileW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_MoveFileExW(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName,
    IN DWORD dwFlags
    )
{
    BOOL Ret;

    LOGAPI("MoveFileExW(lpExistingFileName=%p (%S), lpNewFileName=%p (%S), "
           "dwFlags=0x%x)\n", lpExistingFileName, lpExistingFileName, 
           lpNewFileName, lpNewFileName, dwFlags);

    PALASSERT((dwFlags & (MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) == dwFlags);

    Ret = MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);

    LOGAPI("MoveFileExW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_CreateDirectoryA(
    IN LPCSTR lpPathName,
    IN PAL_LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    BOOL Ret;

    LOGAPI("CreateDirectoryA(lpPathName=%p (%s), lpSecurityAttributes=%p\n)",
           lpPathName, lpPathName, lpSecurityAttributes);

    PALASSERT(lpSecurityAttributes == NULL);
    Ret = CreateDirectoryA(lpPathName, 
                           (LPSECURITY_ATTRIBUTES)lpSecurityAttributes);

    LOGAPI("CreateDirectoryA returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_CreateDirectoryW(
    IN LPCWSTR lpPathName,
    IN PAL_LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    BOOL Ret;

    LOGAPI("CreateDirectoryW(lpPathName=%p (%S), lpSecurityAttributes=%p\n)",
           lpPathName, lpPathName, lpSecurityAttributes);

    PALASSERT(lpSecurityAttributes == NULL);
    Ret = CreateDirectoryW(lpPathName, 
                           (LPSECURITY_ATTRIBUTES)lpSecurityAttributes);

    LOGAPI("CreateDirectoryW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_RemoveDirectoryW(
    IN LPCWSTR lpPathName
    )
{
    BOOL Ret;

    LOGAPI("RemoveDirectoryW(lpPathName=%p (%S))\n", lpPathName, lpPathName);

    Ret = RemoveDirectoryW(lpPathName);

    LOGAPI("RemoveDirectoryW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_FindFirstFileA(
    IN LPCSTR lpFileName,
    OUT PAL_LPWIN32_FIND_DATAA lpFindFileData
    )
{
    HANDLE Ret;

    LOGAPI("FindFirstFileA(lpFileName=%p (%s), lpFindData=%p)\n", 
           lpFileName, lpFileName, lpFindFileData);

    Ret = FindFirstFileA(lpFileName, (LPWIN32_FIND_DATAA)lpFindFileData);

    LOGAPI("FindFirstFileA returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_FindFirstFileW(
    IN LPCWSTR lpFileName,
    OUT PAL_LPWIN32_FIND_DATAW lpFindFileData
    )
{
    HANDLE Ret;

    LOGAPI("FindFirstFileW(lpFileName=%p (%S), lpFindData=%p)\n", 
           lpFileName, lpFileName, lpFindFileData);

    Ret = FindFirstFileW(lpFileName, (LPWIN32_FIND_DATAW)lpFindFileData);

    LOGAPI("FindFirstFileW returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FindNextFileA(
    IN HANDLE hFindFile,
    OUT PAL_LPWIN32_FIND_DATAA lpFindFileData
    )
{
    BOOL Ret;

    LOGAPI("FindNextFileA(hFindFile=%p, lpFindFileData=%p)\n", 
           hFindFile, lpFindFileData);

    Ret = FindNextFileA(hFindFile, (LPWIN32_FIND_DATAA)lpFindFileData);

    LOGAPI("FindNextFileA returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FindNextFileW(
    IN HANDLE hFindFile,
    OUT PAL_LPWIN32_FIND_DATAW lpFindFileData
    )
{
    BOOL Ret;

    LOGAPI("FindNextFileW(hFindFile=%p, lpFindFileData=%p)\n", 
           hFindFile, lpFindFileData);

    Ret = FindNextFileW(hFindFile, (LPWIN32_FIND_DATAW)lpFindFileData);

    LOGAPI("FindNextFileW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FindClose(
    IN OUT HANDLE hFindFile
    )
{
    BOOL Ret;

    LOGAPI("FindClose(hFindFile=%p)\n", hFindFile);

    Ret = FindClose(hFindFile);

    LOGAPI("FindClose returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetFileAttributesA(
    IN LPCSTR lpFileName
    )
{
    DWORD Ret;

    LOGAPI("GetFileAttributesA(lpFileName=%p (%s))\n", lpFileName, lpFileName);

    Ret = GetFileAttributesA(lpFileName);

    LOGAPI("GetFileAttributesA returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetFileAttributesW(
    IN LPCWSTR lpFileName
    )
{
    DWORD Ret;

    LOGAPI("GetFileAttributesW(lpFileName=%p (%S))\n", 
           lpFileName, lpFileName);

    Ret = GetFileAttributesW(lpFileName);

    LOGAPI("GetFileAttributesW returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetFileAttributesExW(
    IN LPCWSTR lpFileName,
    IN GET_FILEEX_INFO_LEVELS fInfoLevelId,
    OUT LPVOID lpFileInformation
    )
{
    BOOL Ret;

    LOGAPI("GetFileAttributesExW(lpFileName=%p (%S), fInfoLevelId=0x%x, "
           "lpFileInformation=%p)\n", lpFileName, lpFileName, 
           fInfoLevelId, lpFileInformation);

    PALASSERT(fInfoLevelId == GetFileExInfoStandard);

    Ret = GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);

    LOGAPI("GetFileAttributesExW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetFileAttributesW(
    IN LPCWSTR lpFileName,
    IN DWORD dwFileAttributes
    )
{
    BOOL Ret;

    LOGAPI("SetFileAttributesW(lpFileName=%p (%S), dwFileAttributes=0x%x)\n",
           lpFileName, lpFileName, dwFileAttributes);

    Ret = SetFileAttributesW(lpFileName, dwFileAttributes);

    LOGAPI("SetFileAttributesW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetFileAttributesA(
    IN LPCSTR lpFileName,
    IN DWORD dwFileAttributes
    )
{
    BOOL Ret;

    LOGAPI("SetFileAttributesA(lpFileName=%p (%s), dwFileAttributes=0x%x)\n",
           lpFileName, lpFileName, dwFileAttributes);

    Ret = SetFileAttributesA(lpFileName, dwFileAttributes);

    LOGAPI("SetFileAttributesA returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_WriteFile(
    IN HANDLE hFile,
    IN LPCVOID lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    OUT LPDWORD lpNumberOfBytesWritten,
    IN PAL_LPOVERLAPPED lpOverlapped
    )
{
    BOOL Ret;

    LOGAPI("WriteFile(hFile=%p, lpBuffer=%p, nNumberOfBytesToWrite=0x%x, "
           "lpNumberOfBytesWritten=%p, lpOverlapped=%p)\n", hFile, lpBuffer, 
           nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

    PALASSERT(lpOverlapped == NULL);

    Ret = WriteFile(hFile, 
                    lpBuffer, 
                    nNumberOfBytesToWrite, 
                    lpNumberOfBytesWritten, 
                    (LPOVERLAPPED)lpOverlapped);

    LOGAPI("WriteFile returns BOOL %d, NumberOfBytesWritten=0x%x\n", 
           Ret, *lpNumberOfBytesWritten);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_ReadFile(
    IN HANDLE hFile,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT LPDWORD lpNumberOfBytesRead,
    IN PAL_LPOVERLAPPED lpOverlapped
    )
{
    BOOL Ret;

    LOGAPI("ReadFile(hFile=%p, lpBuffer=%p, nNumberOfBytesToRead=0x%x, "
           "lpNumberOfBytesRead=%p, lpOverlapped=%p)\n", hFile, lpBuffer, 
           nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

    PALASSERT(lpOverlapped == NULL);

    Ret = ReadFile(hFile, 
                   lpBuffer, 
                   nNumberOfBytesToRead, 
                   lpNumberOfBytesRead,
                   (LPOVERLAPPED)lpOverlapped);

    LOGAPI("ReadFile returns BOOL %d, NumberOfBytesRead=0x%x\n", 
           Ret, *lpNumberOfBytesRead);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_GetStdHandle(
    IN DWORD nStdHandle
    )
{
    HANDLE Ret;

    LOGAPI("GetStdHandle(nStdHandle=0x%x)\n", nStdHandle);

    Ret = GetStdHandle(nStdHandle);

    LOGAPI("GetStdHandle returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetEndOfFile(
    IN HANDLE hFile
    )
{
    BOOL Ret;

    LOGAPI("SetEndOfFile(hFile=%p)\n", hFile);

    Ret = SetEndOfFile(hFile);

    LOGAPI("SetEndOfFile returns %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_SetFilePointer(
    IN HANDLE hFile,
    IN LONG lDistanceToMove,
    IN PLONG lpDistanceToMoveHigh,
    IN DWORD dwMoveMethod
    )
{
    DWORD Ret;

    LOGAPI("SetFilePointer(hFile=%p, lDistanceToMove=0x%x, "
           "lpDistanceToMoveHigh=%p (0x%x), dwMoveMethod=0x%x)\n", hFile, 
           lDistanceToMove, lpDistanceToMoveHigh, 
           (lpDistanceToMoveHigh) ? *lpDistanceToMoveHigh : 0, dwMoveMethod);

    Ret = SetFilePointer(hFile, 
                         lDistanceToMove,
                         lpDistanceToMoveHigh, 
                         dwMoveMethod);

    LOGAPI("PAL_SetFilePointer returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetFileSize(
    IN HANDLE hFile,
    OUT LPDWORD lpFileSizeHigh
    )
{
    DWORD Ret;

    LOGAPI("GetFileSize(hFile=%p, lpFileSizeHigh=%p)\n", 
           hFile, lpFileSizeHigh);

    Ret = GetFileSize(hFile, lpFileSizeHigh);

    LOGAPI("GetFileSize returns DWORD 0x%x (*lpFileSizeHigh=0x%x)\n", 
           Ret, (lpFileSizeHigh) ? *lpFileSizeHigh : 0);
    return Ret;
}

PALIMPORT
LONG
PALAPI
PAL_CompareFileTime(
    IN CONST PAL_FILETIME *lpFileTime1,
    IN CONST PAL_FILETIME *lpFileTime2
    )
{
    LONG Ret;

    LOGAPI("CompareFileTime(lpFileTime1=%p, lpFileTime2=%p)\n", 
           lpFileTime1, lpFileTime2);

    Ret = CompareFileTime((FILETIME *)lpFileTime1, (FILETIME *)lpFileTime2);

    LOGAPI("CompareFileTime returns LONG 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetFileTime(
    IN HANDLE hFile,
    IN CONST PAL_FILETIME *lpCreationTime,
    IN CONST PAL_FILETIME *lpLastAccessTime,
    IN CONST PAL_FILETIME *lpLastWriteTime
    )
{
    BOOL Ret;

    LOGAPI("SetFileTime(hFile=%p, lpCreationTime=%p, lpLastAccessTime=%p, "
           "lpLastWriteTime=%p)\n", hFile, lpCreationTime, lpLastAccessTime, 
           lpLastWriteTime);

    Ret = SetFileTime(hFile, 
                      (FILETIME *)lpCreationTime,
                      (FILETIME *)lpLastAccessTime, 
                      (FILETIME *)lpLastWriteTime);

    LOGAPI("SetFileTime returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetFileTime(
    IN HANDLE hFile,
    OUT PAL_FILETIME *lpCreationTime,
    OUT PAL_FILETIME *lpLastAccessTime,
    OUT PAL_FILETIME *lpLastWriteTime
    )
{
    BOOL Ret;

    LOGAPI("GetFileTime(hFile=%p, lpCreationTime=%p, lpLastAccessTime=%p, "
           "lpLastWriteTime=%p)\n", hFile, lpCreationTime, lpLastAccessTime, 
           lpLastWriteTime);

    Ret = GetFileTime(hFile, 
                      (FILETIME *)lpCreationTime,
                      (FILETIME *)lpLastAccessTime, 
                      (FILETIME *)lpLastWriteTime);

    LOGAPI("GetFileTime returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FileTimeToLocalFileTime(
    IN CONST PAL_FILETIME *lpFileTime,
    OUT PAL_LPFILETIME lpLocalFileTime
    )
{
    BOOL Ret;

    LOGAPI("FileTimeToLocalFileTime(lpFileTime=%p, lpLocalFileTime=%p)\n", 
           lpFileTime, lpLocalFileTime);

    Ret = FileTimeToLocalFileTime((CONST FILETIME *)lpFileTime, 
                                  (LPFILETIME)lpLocalFileTime);

    LOGAPI("FileTimeToLocalFileTime returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_LocalFileTimeToFileTime(
    IN CONST PAL_FILETIME *lpLocalFileTime,
    OUT PAL_LPFILETIME lpFileTime
    )
{
    BOOL Ret;

    LOGAPI("LocalFileTimeToFileTime(lpLocalFileTime=%p, lpFileTime=%p)\n", 
           lpFileTime, lpLocalFileTime);

    Ret = LocalFileTimeToFileTime((CONST FILETIME *)lpLocalFileTime, 
                                  (LPFILETIME)lpFileTime);

    LOGAPI("LocalFileTimeToFileTime returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FlushFileBuffers(
    IN HANDLE hFile
    )
{
    BOOL Ret;

    LOGAPI("FlushFileBuffers(hFile=%p)\n", hFile);

    Ret = FlushFileBuffers(hFile);

    LOGAPI("FlushFileBuffers returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetFileType(
    IN HANDLE hFile
    )
{
    DWORD Ret;

    LOGAPI("GetFileType(hFile=%p)\n", hFile);

    Ret = GetFileType(hFile);

    LOGAPI("GetFileType returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
UINT
PALAPI
PAL_GetConsoleCP(
             VOID)
{
    UINT Ret;

    LOGAPI("GetConsoleCP(void)\n");

    Ret = GetConsoleCP();

    LOGAPI("GetConsoleCP returns UINT 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
UINT
PALAPI
PAL_GetConsoleOutputCP(
                   VOID)
{
    UINT Ret;

    LOGAPI("GetConsoleOutputCP(void)\n");

    Ret = GetConsoleOutputCP();

    LOGAPI("GetConsoleOutputCP returns UINT 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetFullPathNameA(
    IN LPCSTR lpFileName,
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer,
    OUT LPSTR *lpFilePart
    )
{
    DWORD Ret;

    LOGAPI("GetFullPathNameA(lpFileName=%p (%s), nBufferLength=0x%x, "
           "lpBuffer=%p, lpFilePart=%p)\n", lpFileName, lpFileName, 
           nBufferLength, lpBuffer, lpFilePart);

    Ret = GetFullPathNameA(lpFileName,
                           nBufferLength,
                           lpBuffer,
                           lpFilePart);

    LOGAPI("GetFullPathNameA returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetFullPathNameW(
    IN LPCWSTR lpFileName,
    IN DWORD nBufferLength,
    OUT LPWSTR lpBuffer,
    OUT LPWSTR *lpFilePart
    )
{
    DWORD Ret;

    LOGAPI("GetFullPathNameW(lpFileName=%p (%S), nBufferLength=0x%x, "
           "lpBuffer=%p, lpFilePart=%p)\n", lpFileName, lpFileName, 
           nBufferLength, lpBuffer, lpFilePart);

    Ret = GetFullPathNameW(lpFileName,
                           nBufferLength,
                           lpBuffer,
                           lpFilePart);

    LOGAPI("GetFullPathNameW returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetLongPathNameW(
    IN LPCWSTR lpszShortPath,
    OUT LPWSTR  lpszLongPath,
    IN DWORD    cchBuffer
    )
{
    DWORD Ret;

    LOGAPI("GetLongPathNameW(lpszShortPath=%p (%S), lpszLongPath=%p, "
           "cchBuffer=0x%x)\n", lpszShortPath, lpszShortPath, lpszLongPath, 
           cchBuffer);

    Ret = GetLongPathNameW(lpszShortPath, lpszLongPath, cchBuffer);

    LOGAPI("GetLongPathNameW returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
UINT
WINAPI
PAL_GetTempFileNameA(
    IN LPCSTR lpPathName,
    IN LPCSTR lpPrefixString,
    IN UINT uUnique,
    OUT LPSTR lpTempFileName
    )
{
    UINT Ret;

    LOGAPI("GetTempFileNameA(lpPathName=%p (%s), lpPrefixString=%p (%s), "
           "uUnique=0x%x, lpTempFileName=%p)\n", lpPathName, lpPathName, 
           lpPrefixString, lpPrefixString, uUnique, lpTempFileName);

    PALASSERT(lpPathName != NULL);
    PALASSERT(uUnique == 0);

    Ret = GetTempFileNameA(lpPathName,
                           lpPrefixString,
                           uUnique,
                           lpTempFileName);

    LOGAPI("GetTempFileNameA returns UINT 0x%x, lpTempFileName=%s\n", 
           Ret, (Ret) ? lpTempFileName : "(API failed)");
    return Ret;
}

PALIMPORT
UINT
WINAPI
PAL_GetTempFileNameW(
    IN LPCWSTR lpPathName,
    IN LPCWSTR lpPrefixString,
    IN UINT uUnique,
    OUT LPWSTR lpTempFileName
    )
{
    UINT Ret;

    LOGAPI("GetTempFileNameW(lpPathName=%p (%S), lpPrefixString=%p (%S), "
           "uUnique=0x%x, lpTempFileName=%p)\n", lpPathName, lpPathName, 
           lpPrefixString, lpPrefixString, uUnique, lpTempFileName);

    PALASSERT(lpPathName != NULL);
    PALASSERT(uUnique == 0);

    Ret = GetTempFileNameW(lpPathName,
                           lpPrefixString,
                           uUnique,
                           lpTempFileName);

    LOGAPI("GetTempFileNameW returns UINT 0x%x, lpTempFileName=%S\n", 
           Ret, (Ret) ? lpTempFileName : L"(API failed)");
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetTempPathW(
    IN DWORD nBufferLength,
    OUT LPWSTR lpBuffer
    )
{
    DWORD Ret;

    LOGAPI("GetTempPathW(nBufferLength=0x%x, lpBuffer=%p)\n", 
           nBufferLength, lpBuffer);

    Ret = GetTempPathW(nBufferLength, lpBuffer);

    LOGAPI("GetTempPathW returns DWORD 0x%x, lpBuffer=%S\n", 
           Ret, 
           (Ret != 0 && Ret <= nBufferLength) ? lpBuffer : L"(not filled in)");
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetTempPathA(
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer
    )
{
    DWORD Ret;

    LOGAPI("GetTempPathA(nBufferLength=0x%x, lpBuffer=%p)\n", 
           nBufferLength, lpBuffer);

    Ret = GetTempPathA(nBufferLength, lpBuffer);

    LOGAPI("GetTempPathA returns DWORD 0x%x, lpBuffer=%s\n", 
           Ret, 
           (Ret != 0 && Ret <= nBufferLength) ? lpBuffer : "(not filled in)");
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetCurrentDirectoryA(
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer
    )
{
    DWORD Ret;

    LOGAPI("GetCurrentDirectoryA(nBufferLength=0x%x, lpBuffer=%p)\n", 
           nBufferLength, lpBuffer);

    Ret = GetCurrentDirectoryA(nBufferLength, lpBuffer);

    LOGAPI("GetCurrentDirectoryA returns DWORD 0x%x, lpBuffer=%s\n", 
           Ret, 
           (Ret != 0 && Ret <= nBufferLength) ? lpBuffer : "(not filled in)");
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetCurrentDirectoryW(
    IN DWORD nBufferLength,
    OUT LPWSTR lpBuffer
    )
{
    DWORD Ret;

    LOGAPI("GetCurrentDirectoryW(nBufferLength=0x%x, lpBuffer=%p)\n", 
           nBufferLength, lpBuffer);

    Ret = GetCurrentDirectoryW(nBufferLength, lpBuffer);

    LOGAPI("GetCurrentDirectoryW returns DWORD 0x%x, lpBuffer=%S\n", 
           Ret, 
           (Ret != 0 && Ret <= nBufferLength) ? lpBuffer : L"(not filled in)");
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetCurrentDirectoryA(
    IN LPCSTR lpPathName
    )
{
    BOOL Ret;

    LOGAPI("SetCurrentDirectoryA(lpPathName=%p (%s))\n", 
           lpPathName, lpPathName);

    Ret = SetCurrentDirectoryA(lpPathName);

    LOGAPI("SetCurrentDirectoryA returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetCurrentDirectoryW(
    IN LPCWSTR lpPathName
    )
{
    BOOL Ret;

    LOGAPI("SetCurrentDirectoryW(lpPathName=%p (%S))\n", 
           lpPathName, lpPathName);

    Ret = SetCurrentDirectoryW(lpPathName);

    LOGAPI("SetCurrentDirectoryW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_CreateSemaphoreA(
    IN PAL_LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    IN LONG lInitialCount,
    IN LONG lMaximumCount,
    IN LPCSTR lpName
    )
{
    HANDLE Ret;

    LOGAPI("CreateSemaphoreA(lpSemaphoreAttributes=%p, lInitialCount=0x%x, "
           "lMaximumCount=0x%x, lpName=%p)\n", lpSemaphoreAttributes, 
           lInitialCount, lMaximumCount, lpName);

    PALASSERT(lpSemaphoreAttributes == NULL);
    PALASSERT(lpName == NULL);

    Ret = CreateSemaphoreA((LPSECURITY_ATTRIBUTES)lpSemaphoreAttributes,
                           lInitialCount,
                           lMaximumCount,
                           lpName);

    LOGAPI("CreateSemaphoreA returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_CreateSemaphoreW(
    IN PAL_LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    IN LONG lInitialCount,
    IN LONG lMaximumCount,
    IN LPCWSTR lpName
    )
{
    HANDLE Ret;

    LOGAPI("CreateSemaphoreW(lpSemaphoreAttributes=%p, lInitialCount=0x%x, "
           "lMaximumCount=0x%x, lpName=%p)\n", lpSemaphoreAttributes, 
           lInitialCount, lMaximumCount, lpName);

    PALASSERT(lpSemaphoreAttributes == NULL);
    PALASSERT(lpName == NULL);

    Ret = CreateSemaphoreW((LPSECURITY_ATTRIBUTES)lpSemaphoreAttributes,
                           lInitialCount,
                           lMaximumCount,
                           lpName);

    LOGAPI("CreateSemaphoreW returns HANDLE %p\n", Ret);
    return Ret;
}


PALIMPORT
BOOL
PALAPI
PAL_ReleaseSemaphore(
    IN HANDLE hSemaphore,
    IN LONG lReleaseCount,
    OUT LPLONG lpPreviousCount
    )
{
    BOOL Ret;

    LOGAPI("ReleaseSemaphore(hSemaphore=%p, lReleaseCount=0x%x, "
           "lpPreviousCount=%p)\n", hSemaphore, lReleaseCount, 
           lpPreviousCount);

    Ret = ReleaseSemaphore(hSemaphore,
                           lReleaseCount,
                           lpPreviousCount);

    if (lpPreviousCount) {
        LOGAPI("ReleaseSemaphore returns BOOL %d, PreviousCount=0x%x\n", 
               Ret, *lpPreviousCount);
    } else {
        LOGAPI("ReleaseSemaphore returns BOOL %d\n", Ret);
    }
    return Ret;
}
#endif //DBG

PALIMPORT
HANDLE
PALAPI
PAL_CreateEventA(
    IN PAL_LPSECURITY_ATTRIBUTES lpEventAttributes,
    IN BOOL bManualReset,
    IN BOOL bInitialState,
    IN LPCSTR lpName
    )
{
    HANDLE Ret;
    LPCSTR lpObjectName;
    char MangledObjectName[MAX_PATH];

    LOGAPI("CreateEventA(lpEventAttributes=%p, bManualReset=%d, "
           "bInitialState=%d, lpName=%p (%s))\n", lpEventAttributes, 
           bManualReset, bInitialState, lpName, lpName);

    PALASSERT(lpEventAttributes == NULL);

    lpObjectName=lpName;
    if (!MangleObjectNameA(&lpObjectName, MangledObjectName)) {
	Ret = NULL;
    } else {
        Ret = CreateEventA((LPSECURITY_ATTRIBUTES)lpEventAttributes,
                           bManualReset,
                           bInitialState,
                           lpObjectName);
    }

    LOGAPI("CreateEventA returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_CreateEventW(
    IN PAL_LPSECURITY_ATTRIBUTES lpEventAttributes,
    IN BOOL bManualReset,
    IN BOOL bInitialState,
    IN LPCWSTR lpName
    )
{
    HANDLE Ret;
    LPCWSTR lpObjectName;
    WCHAR MangledObjectName[MAX_PATH];

    LOGAPI("CreateEventW(lpEventAttributes=%p, bManualReset=%d, "
           "bInitialState=%d, lpName=%p (%S))\n", lpEventAttributes, 
           bManualReset, bInitialState, lpName, lpName);

    PALASSERT(lpEventAttributes == NULL);

    lpObjectName=lpName;
    if (!MangleObjectNameW(&lpObjectName, MangledObjectName)) {
	Ret = NULL;
    } else {
        Ret = CreateEventW((LPSECURITY_ATTRIBUTES)lpEventAttributes,
                           bManualReset,
                           bInitialState,
                           lpObjectName);
    }

    LOGAPI("CreateEventW returns HANDLE %p\n", Ret);
    return Ret;
}

#if DBG

PALIMPORT
BOOL
PALAPI
PAL_SetEvent(
    IN HANDLE hEvent
    )
{
    BOOL Ret;

    LOGAPI("SetEvent(hEvent=%p)\n", hEvent);

    Ret = SetEvent(hEvent);

    LOGAPI("SetEvent returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_ResetEvent(
    IN HANDLE hEvent
    )
{
    BOOL Ret;

    LOGAPI("ResetEvent(hEvent=%p)\n", hEvent);

    Ret = ResetEvent(hEvent);

    LOGAPI("ResetEvent returns BOOL %d\n", Ret);
    return Ret;
}

#endif //DBG

PALIMPORT
HANDLE
PALAPI
PAL_OpenEventW(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCWSTR lpName
    )
{
    HANDLE Ret;
    LPCWSTR lpObjectName;
    WCHAR MangledObjectName[MAX_PATH];

    LOGAPI("OpenEventW(dwDesiredAccess=0x%x, bInheritHandle=%d, "
           "lpName=%p (%S))\n", dwDesiredAccess, bInheritHandle, lpName,
           lpName);

    PALASSERT(dwDesiredAccess == EVENT_ALL_ACCESS);

    lpObjectName=lpName;
    if (!MangleObjectNameW(&lpObjectName, MangledObjectName)) {
	Ret= NULL;
    } else {
        Ret = OpenEventW(dwDesiredAccess, bInheritHandle, lpObjectName);
    }

    LOGAPI("OpenEventW returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_CreateMutexW(
    IN PAL_LPSECURITY_ATTRIBUTES lpMutexAttributes,
    IN BOOL bInitialOwner,
    IN LPCWSTR lpName
    )
{
    HANDLE Ret;
    LPCWSTR lpObjectName;
    WCHAR MangledObjectName[MAX_PATH];

    LOGAPI("CreateMutexW(lpMutexAttributes=%p, bInitialOwner=%d, "
           "lpName=%p (%S))\n", lpMutexAttributes, 
           bInitialOwner, lpName, lpName);

    PALASSERT(lpMutexAttributes == NULL);

    lpObjectName=lpName;
    if (!MangleObjectNameW(&lpObjectName, MangledObjectName)) {
	Ret= NULL;
    } else {
        Ret = CreateMutexW((LPSECURITY_ATTRIBUTES)lpMutexAttributes,
                           bInitialOwner,
                           lpObjectName);
    }

    LOGAPI("CreateMutexW returns HANDLE %p\n", Ret);
    return Ret;
}

#if DBG

PALIMPORT
BOOL
PALAPI
PAL_ReleaseMutex(
    IN HANDLE hMutex
    )
{
    BOOL Ret;

    LOGAPI("ReleaseMutex(hMutex=%p)\n", hMutex);

    Ret = ReleaseMutex(hMutex);

    LOGAPI("ReleaseMutex returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetCurrentProcessId(
    VOID
    )
{
    DWORD Ret;

    LOGAPI("GetCurrentProcessId(void)\n");

    Ret = GetCurrentProcessId();

    LOGAPI("GetCurrentProcessId returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_GetCurrentProcess(
    VOID
    )
{
    HANDLE Ret;

    LOGAPI("GetCurrentProcess(void)\n");

    Ret = GetCurrentProcess();

    LOGAPI("GetCurrentProcess returns HANDLE 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetCurrentThreadId(
    VOID
    )
{
    DWORD Ret;

    LOGAPI("GetCurrentThreadId(void)\n");

    Ret = GetCurrentThreadId();

    LOGAPI("GetCurrentThreadId returns DWORD 0x%x\n", Ret);
    return Ret;
}


PALIMPORT
HANDLE
PALAPI
PAL_GetCurrentThread(
    VOID
    )
{
    HANDLE Ret;

    LOGAPI("GetCurrentThread(void)\n");

    Ret = GetCurrentThread();

    LOGAPI("GetCurrentThread returns HANDLE 0x%x\n", Ret);
    return Ret;
}

#endif //DBG

//
//  Grabs the first parameter from the command line
//
static
void
getFileName( LPWSTR lpCommandLine, LPWSTR FileName, DWORD FileNameLength)
{
    BOOL inquote = FALSE;
    WCHAR * psrc = lpCommandLine;
    WCHAR * pdst = FileName;
 
    PALASSERT(lpCommandLine != NULL && FileName != NULL );
   
    // Parse the first arg - the name of the module to run
    while ( (unsigned)(pdst - FileName) < (FileNameLength-1) )
    {
        switch (*psrc)
        {
        case L'\"':
            inquote = !inquote;
            psrc++;
            continue;

        case '\0':
            break;

        case L' ':
        case L'\t':
            if (!inquote)
                break;
            // intentionally fall through
        default:
            *pdst++ = *psrc++;
            continue;
        }
        break;
    }
    // zero terminate
    *pdst = 0;
}

//
//  Determines if the passed in file is a managed executable
//
static
int
isManagedExecutable( LPWSTR lpFileName)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dResult = 0, cbRead;
    const WCHAR *exeExtension = L".exe";
    IMAGE_DOS_HEADER        dosheader;
    IMAGE_NT_HEADERS32      NtHeaders; 
    BOOL ret = 0;
    
    // Make sure filename is non-null
    PALASSERT( lpFileName != NULL );
    
    // then check if it is a PE/COFF file 
    if((hFile = CreateFileW(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL)) == INVALID_HANDLE_VALUE)
    {
      // Try to append the ".exe" to the end of the file. The file name should have been allocated with
      // the extra space
      wcscat( lpFileName, exeExtension ); 

      if((hFile = CreateFileW(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL)) == INVALID_HANDLE_VALUE)
          goto isManagedExecutableExit;
    }
      
    
    // Open the file and read the IMAGE_DOS_HEADER structure 
    if(!ReadFile(hFile, &dosheader, sizeof(IMAGE_DOS_HEADER), &cbRead, NULL) || cbRead != sizeof(IMAGE_DOS_HEADER) )
      goto isManagedExecutableExit;
       
    // check the DOS headers
    if ( (dosheader.e_magic != IMAGE_DOS_SIGNATURE) || (dosheader.e_lfanew <= 0) ) 
      goto isManagedExecutableExit;         
 
    //Advance the file pointer to File address of new exe header
    if( SetFilePointer(hFile, dosheader.e_lfanew, NULL, FILE_BEGIN) == 0xffffffff)
      goto isManagedExecutableExit;
	        
    if( !ReadFile(hFile, &NtHeaders , sizeof(IMAGE_NT_HEADERS32), &cbRead, NULL) || cbRead != sizeof(IMAGE_NT_HEADERS32) )
      goto isManagedExecutableExit;
   
    // check the NT headers   
    if ((NtHeaders.Signature != IMAGE_NT_SIGNATURE) ||
        (NtHeaders.FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL32_HEADER) ||
        (NtHeaders.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC))
        goto isManagedExecutableExit;
     
    // Check that the virtial address of IMAGE_DIRECTORY_ENTRY_COMHEADER is non-null
    if ( NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress == NULL )
        goto isManagedExecutableExit;
  
    // The file is a managed executable
    ret =  1;
     
 isManagedExecutableExit:
    // Close the file handle if we opened it
    if ( hFile != INVALID_HANDLE_VALUE )
      CloseHandle(hFile);

    return ret;
}


PALIMPORT
BOOL
PALAPI
PAL_CreateProcessW(
    IN LPCWSTR lpApplicationName,
    IN LPWSTR lpCommandLine,
    IN PAL_LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN PAL_LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCWSTR lpCurrentDirectory,
    IN PAL_LPSTARTUPINFOW lpStartupInfo,
    OUT PAL_LPPROCESS_INFORMATION lpProcessInformation
    )
{
    BOOL Ret;
    BOOL ManagedExecutable = 0;  // Flag to indicate if the file is a managed executable
    LPWSTR FileName = NULL, lpNewCommandLine = NULL;
    DWORD FileNameLength;
    WCHAR PALPathW[_MAX_PATH]; 

    LOGAPI("CreateProcessW(lpApplicationName=%p, lpCommandLine=%p (%S), "
           "lpProcessAttributes=%p, lpThreadAttributes=%p, "
           "bInheritHandles=%d, dwCreationFlags=0x%x, lpEnvironment=%p, "
           "lpCurrentDirectory=%p (%S), lpStartupInfo=%p, "
           "lpProcessInformation=%p)\n", lpApplicationName, lpCommandLine, 
           lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
           bInheritHandles, dwCreationFlags, lpEnvironment, 
           lpCurrentDirectory, lpCurrentDirectory, lpStartupInfo, 
           lpProcessInformation);

    PALASSERT(lpApplicationName == NULL );
    PALASSERT(lpProcessAttributes == NULL ||
              (lpProcessAttributes->bInheritHandle == TRUE &&
               lpProcessAttributes->lpSecurityDescriptor == NULL));
    PALASSERT(lpThreadAttributes == NULL);
    PALASSERT((dwCreationFlags & ~(CREATE_NEW_CONSOLE|CREATE_SUSPENDED))
	      == 0);
    PALASSERT(lpStartupInfo->cb != 0);
    PALASSERT((lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) == 
              lpStartupInfo->dwFlags);

    // Make sure that the command line has value
    if ( lpCommandLine == NULL )
    {
        // Since lpApplicationName is not used, lpCommandLine must have a value
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CreateProcessWExit;
    }

    // Check if the application is a managed executable and append the preloader if
    // necessary      
    FileNameLength = wcslen(lpCommandLine) + 1 + 4 /* possibly need to add ".exe" */; 
    FileName       = malloc( sizeof(WCHAR)*( FileNameLength ) );
    // Check for out of memory condition
    if ( FileName == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto CreateProcessWExit;
    }
   
    // Parse the module name from the command line
    getFileName(lpCommandLine, FileName, FileNameLength );
    // Read the header to see if this file is a managed executable 
    if (isManagedExecutable(FileName))
    {
      if (!PAL_GetPALDirectoryW(PALPathW, _MAX_PATH)) 
        goto CreateProcessWExit;   // Preserve the error code set by PAL_GetPALDirectoryW and return false
      
      lpNewCommandLine = malloc( sizeof(WCHAR)*(wcslen(PROCESS_PELOADER_FILENAMEW) + wcslen(lpCommandLine) + 
                                                wcslen(PALPathW) + 1) );
      if (lpNewCommandLine == NULL ) 
      {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto CreateProcessWExit;
      }
            
      wcscpy( lpNewCommandLine, PALPathW );
      wcscat( lpNewCommandLine, PROCESS_PELOADER_FILENAMEW );
      wcscat( lpNewCommandLine, lpCommandLine );
      ManagedExecutable = 1;
    }
   

    Ret = CreateProcessW(lpApplicationName,
                         ManagedExecutable ?  lpNewCommandLine : lpCommandLine,
                         (LPSECURITY_ATTRIBUTES)lpProcessAttributes,
                         (LPSECURITY_ATTRIBUTES)lpThreadAttributes,
                         bInheritHandles,
                         dwCreationFlags,
                         lpEnvironment,
                         lpCurrentDirectory,
                         (LPSTARTUPINFOW)lpStartupInfo,
                         (LPPROCESS_INFORMATION)lpProcessInformation);
CreateProcessWExit:
    if (FileName)
      free(FileName);
    if (lpNewCommandLine)
      free(lpNewCommandLine);

    if (Ret) {
        LOGAPI("CreateProcessW returned BOOL %d, "
               "lpProcessInformation->hProcess=%p\n", Ret, 
               lpProcessInformation->hProcess);
    } else {
        LOGAPI("CreateProcessW returned BOOL %d, "
               "lpProcessInformation not filled out\n", Ret);
    }
    return Ret;
}


PALIMPORT
BOOL
PALAPI
PAL_CreateProcessA(
    IN LPCSTR lpApplicationName,
    IN LPSTR lpCommandLine,
    IN PAL_LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN PAL_LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCSTR lpCurrentDirectory,
    IN PAL_LPSTARTUPINFOA lpStartupInfo,
    OUT PAL_LPPROCESS_INFORMATION lpProcessInformation
    )
{
    BOOL Ret;
    BOOL ManagedExecutable = 0;  // Flag to indicate if the file is a managed executable
    LPWSTR FileName = NULL, lpCommandLineW = NULL;
    DWORD FileNameLength, CommandLineLength;
    LPSTR lpNewCommandLine = NULL;
    WCHAR PALPathW[MAX_PATH];
    char  PALPathA[MAX_PATH]; 

    LOGAPI("CreateProcessA(lpApplicationName=%p, lpCommandLine=%p (%s), "
           "lpProcessAttributes=%p, lpThreadAttributes=%p, "
           "bInheritHandles=%d, dwCreationFlags=0x%x, lpEnvironment=%p, "
           "lpCurrentDirectory=%p (%s), lpStartupInfo=%p, "
           "lpProcessInformation=%p)\n", lpApplicationName, lpCommandLine, 
           lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
           bInheritHandles, dwCreationFlags, lpEnvironment, 
           lpCurrentDirectory, lpCurrentDirectory, lpStartupInfo, 
           lpProcessInformation);

    PALASSERT(lpApplicationName == NULL);
    PALASSERT(lpProcessAttributes == NULL ||
              (lpProcessAttributes->bInheritHandle == TRUE &&
               lpProcessAttributes->lpSecurityDescriptor == NULL));
    PALASSERT(lpThreadAttributes == NULL);
    PALASSERT(dwCreationFlags == CREATE_NEW_CONSOLE ||
	      dwCreationFlags == 0);
    PALASSERT(lpStartupInfo->cb != 0);
    PALASSERT((lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) == 
              lpStartupInfo->dwFlags);

    // Since lpApplicationName is not used, lpCommandLine must have a value
    if (lpCommandLine == NULL)
    {
        
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CreateProcessAExit;
    }   

    // Check if the application is a managed executable and append the preloader if
    // necessary 
    
    // Calculate the length of temporary buffers  
    if  ( (CommandLineLength = MultiByteToWideChar(CP_ACP, 0, lpCommandLine, -1, NULL, 0 )) == 0) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CreateProcessAExit;
    }     
    FileNameLength = CommandLineLength + 4 /* possibly need to add ".exe" */; 
    lpCommandLineW = malloc( sizeof(WCHAR)*( CommandLineLength ) );
    FileName       = malloc( sizeof(WCHAR)*( FileNameLength ) );

    // Check for out of memory condition
    if ( FileName == NULL || lpCommandLineW == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto CreateProcessAExit;
    }

    // Convert the command line to wide char
    if ( !MultiByteToWideChar(CP_ACP, 0, lpCommandLine, -1, lpCommandLineW, CommandLineLength) )
    {
        // Since lpApplicationName is not used, lpCommandLine must have a value
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CreateProcessAExit;
    }
    // Parse the name of the executable from the command line
    getFileName(lpCommandLineW, FileName, FileNameLength );

    // If this is a managed executable - append the preloader 
    if (isManagedExecutable(FileName))
    {
       if (!PAL_GetPALDirectoryW(PALPathW, MAX_PATH) ||
           !WideCharToMultiByte(CP_ACP, 0, PALPathW, -1, PALPathA, MAX_PATH, NULL, 0))
          goto CreateProcessAExit; // Preserve the error code set by PAL_GetPALDirectoryW and return false
 
       lpNewCommandLine = malloc( sizeof(char)*(strlen(PROCESS_PELOADER_FILENAMEA) + strlen(lpCommandLine) + 
                                                strlen(PALPathA) + 1) );
       if (lpNewCommandLine == NULL ) 
       {
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         goto CreateProcessAExit;
       }
       strcpy( lpNewCommandLine, PALPathA );
       strcat( lpNewCommandLine, PROCESS_PELOADER_FILENAMEA );
       strcat( lpNewCommandLine, lpCommandLine );
       ManagedExecutable = 1;
    }

    Ret = CreateProcessA(lpApplicationName,
                         ManagedExecutable ?  lpNewCommandLine : lpCommandLine,
                         (LPSECURITY_ATTRIBUTES)lpProcessAttributes,
                         (LPSECURITY_ATTRIBUTES)lpThreadAttributes,
                         bInheritHandles,
                         dwCreationFlags,
                         lpEnvironment,
                         lpCurrentDirectory,
                         (LPSTARTUPINFOA)lpStartupInfo,
                         (LPPROCESS_INFORMATION)lpProcessInformation);
CreateProcessAExit:

    if (FileName)
      free(FileName);
    if (lpCommandLineW)
      free(lpCommandLineW);
    if (lpNewCommandLine)
      free(lpNewCommandLine);

    if (Ret) {
        LOGAPI("CreateProcessA returned BOOL %d, "
               "lpProcessInformation->hProcess=%p\n", Ret, 
               lpProcessInformation->hProcess);
    } else {
        LOGAPI("CreateProcessA returned BOOL %d, "
               "lpProcessInformation not filled out\n", Ret);
    }
    return Ret;
}

#if DBG

PALIMPORT
DECLSPEC_NORETURN
VOID
PALAPI
PAL_ExitProcess(
    IN UINT uExitCode
    )
{
    // Bump the refcount, so the LOGAPI call won't assert - 
    // ExitProcess() may be called without having first called
    // PAL_Initialize().
    InterlockedIncrement(&PalReferenceCount);

    LOGAPI("ExitProcess(uExitCode=0x%x)\n", uExitCode);

    ExitProcess(uExitCode);

    // no need to log here - ExitProcess never returns.
}

PALIMPORT
BOOL
PALAPI
PAL_TerminateProcess(
    IN HANDLE hProcess,
    IN UINT uExitCode
    )
{
    BOOL Ret;

    LOGAPI("TerminateProcess(hProcess=%p, uExitCode=0x%x\n", 
           hProcess, uExitCode);

    Ret = TerminateProcess(hProcess, uExitCode);

    LOGAPI("TerminateProcess returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetExitCodeProcess(
    IN HANDLE hProcess,
    IN LPDWORD lpExitCode
    )
{
    BOOL Ret;

    LOGAPI("GetExitCodeProcess(hProcess=%p, lpExitCode=%p\n",
	   hProcess, lpExitCode);

    Ret = GetExitCodeProcess(hProcess, lpExitCode);

    LOGAPI("GetExitCodeProcess returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetProcessTimes(
    IN HANDLE hProcess,
    OUT PAL_LPFILETIME lpCreationTime,
    OUT PAL_LPFILETIME lpExitTime,
    OUT PAL_LPFILETIME lpKernelTime,
    OUT PAL_LPFILETIME lpUserTime
    )
{
    BOOL Ret;

    LOGAPI("GetProcessTimes(hProcess=%p, lpCreationTime=%p, lpExitTime=%p, "
           "lpKernelTime=%p, lpUserTime=%p)\n", hProcess, lpCreationTime, 
           lpExitTime, lpKernelTime, lpUserTime);

    Ret = GetProcessTimes(hProcess,
                          (LPFILETIME)lpCreationTime,
                          (LPFILETIME)lpExitTime,
                          (LPFILETIME)lpKernelTime,
                          (LPFILETIME)lpUserTime);

    LOGAPI("GetProcessTimes returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_WaitForSingleObject(
    IN HANDLE hHandle,
    IN DWORD dwMilliseconds
    )
{
    DWORD Ret;

    LOGAPI("WaitForSingleObject(hHandle=%p, dwMilliseconds=0x%x)\n", 
           hHandle, dwMilliseconds);

    Ret = WaitForSingleObject(hHandle, dwMilliseconds);

    LOGAPI("WaitForSingleObject returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_WaitForMultipleObjects(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds
    )
{
    DWORD Ret;

    LOGAPI("WaitForMultipleObjects(nCount=0x%x, lpHandles=%p, bWaitAll=%d, "
           "dwMilliseconds=0x%x)\n", nCount, lpHandles, bWaitAll, 
           dwMilliseconds);

    Ret = WaitForMultipleObjects(nCount,
                                 lpHandles,
                                 bWaitAll,
                                 dwMilliseconds);

    LOGAPI("WaitForMultipleObjects returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_WaitForMultipleObjectsEx(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    )
{
    DWORD Ret;

    LOGAPI("WaitForMultipleObjectsEx(nCount=0x%x, lpHandles=%p, bWaitAll=%d, "
           "dwMilliseconds=0x%x, bAlertable=%d)\n", nCount, lpHandles, 
           bWaitAll, dwMilliseconds, bAlertable);

    Ret = WaitForMultipleObjectsEx(nCount,
                                   lpHandles,
                                   bWaitAll,
                                   dwMilliseconds,
                                   bAlertable);

    LOGAPI("WaitForMultipleObjectsEx returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_DuplicateHandle(
    IN HANDLE hSourceProcessHandle,
    IN HANDLE hSourceHandle,
    IN HANDLE hTargetProcessHandle,
    OUT LPHANDLE lpTargetHandle,
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwOptions
    )
{
    BOOL Ret;

    LOGAPI("DuplicateHandle(hSourceProcessHandle=%p, hSourceHandle=%p, "
           "hTargetProcessHandle=%p, lpTargetHandle=%p, dwDesiredAccess=0x%x, "
           "bInheritHandle=%d, dwOptions=0x%x)\n", hSourceProcessHandle, 
           hSourceHandle, hTargetProcessHandle, lpTargetHandle, 
           dwDesiredAccess, bInheritHandle, dwOptions);

    PALASSERT(hSourceProcessHandle == GetCurrentProcess() ||
              hTargetProcessHandle == GetCurrentProcess());
    PALASSERT(bInheritHandle == FALSE);
    PALASSERT(dwOptions == DUPLICATE_SAME_ACCESS);

    Ret = DuplicateHandle(hSourceProcessHandle, 
                          hSourceHandle,
                          hTargetProcessHandle,
                          lpTargetHandle,
                          dwDesiredAccess,
                          bInheritHandle,
                          dwOptions);

    if (Ret) {
        LOGAPI("DuplicateHandle returns BOOL %d, TargetHandle=%p\n", 
               Ret, *lpTargetHandle);
    } else {
        LOGAPI("DuplicateHandle returns BOOL %d, TargetHandle not set\n", Ret);
    }
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_Sleep(
    IN DWORD dwMilliseconds
    )
{
    LOGAPI("Sleep(dwMilliseconds=0x%x)\n", dwMilliseconds);

    Sleep(dwMilliseconds);

    LOGAPI("Sleep returns void\n");
}

PALIMPORT
DWORD
PALAPI
PAL_SleepEx(
            IN DWORD dwMilliseconds,
            IN BOOL bAlertable
    )
{
    DWORD Ret;

    LOGAPI("SleepEx(dwMilliseconds=0x%x, bAlertable=%d)\n", 
            dwMilliseconds, bAlertable);

    Ret = SleepEx(dwMilliseconds, bAlertable);

    LOGAPI("SleepEx returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SwitchToThread(
    VOID
    )
{
    DWORD Ret;

    LOGAPI("SwitchToThread(void)\n");

    Ret = SwitchToThread();

    LOGAPI("SwitchToThread returns BOOL %d\n", Ret);
    return Ret;
}

#endif //DBG

typedef struct _PAL_ThreadStart
{
    LPTHREAD_START_ROUTINE  lpStartAddress;
    LPVOID                  lpParameter;
} PAL_ThreadStart;

DWORD_PTR 
PALAPI 
PAL_ThreadProc(PAL_ThreadStart *pThreadStart)
{
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpParameter;

    // exceptions masked, round to nearest, 53 bit precision.
    _controlfp(_MCW_EM | _RC_NEAR | _PC_53 | _IC_PROJECTIVE | _DN_SAVE, 
               _MCW_EM | _MCW_RC | _MCW_PC | _MCW_IC | _MCW_DN);
    
    lpStartAddress = pThreadStart->lpStartAddress;
    lpParameter = pThreadStart->lpParameter;
    free(pThreadStart);

    return PAL_LocalFrame(lpStartAddress, lpParameter);
}

PALIMPORT
HANDLE
PALAPI
PAL_CreateThread(
    IN PAL_LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN DWORD dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwCreationFlags,
    OUT LPDWORD lpThreadId
    )
{
    HANDLE Ret;
    PAL_ThreadStart *pThreadStart;

    LOGAPI("CreateThread(lpThreadAttributes=%p, dwStackSize=0x%x, "
           "lpStartAddress=%p, lpParameter=%p, dwCreationFlags=0x%x, "
           "lpThreadId=%p)\n", lpThreadAttributes, dwStackSize, 
           lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);

    PALASSERT(lpThreadAttributes == NULL);
    PALASSERT(dwCreationFlags == 0 || dwCreationFlags == CREATE_SUSPENDED);

    if ((pThreadStart = (PAL_ThreadStart*)malloc(sizeof(PAL_ThreadStart))) == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Ret = NULL;
        goto LExit;
    }

    pThreadStart->lpStartAddress = lpStartAddress;
    pThreadStart->lpParameter = lpParameter;

    Ret = CreateThread((LPSECURITY_ATTRIBUTES)lpThreadAttributes, 
                       dwStackSize,
                       PAL_ThreadProc,
                       pThreadStart,
                       dwCreationFlags,
                       lpThreadId);

    if (Ret == NULL)
        free(pThreadStart);

LExit:
    LOGAPI("CreateThread returns HANDLE %p\n", Ret);
    return Ret;
}

#if DBG

PALIMPORT
DECLSPEC_NORETURN
VOID
PALAPI
PAL_ExitThread(
    IN DWORD dwExitCode
    )
{
    LOGAPI("ExitThread(dwExitCode=0x%x)\n", dwExitCode);

    ExitThread(dwExitCode);

    // no need to log - ExitThread never returns
}

PALIMPORT
DWORD
PALAPI
PAL_SuspendThread(
    IN HANDLE hThread
    )
{
    DWORD Ret;

    LOGAPI("SuspendThread(hThread=%p)\n", hThread);

    if (VerifyThreadInCurrentProcess(hThread)) {
       Ret = SuspendThread(hThread);
    } else {
	PALASSERT(FALSE);
	Ret = 0xffffffff;
    }

    LOGAPI("SuspendThread returned DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_ResumeThread(
    IN HANDLE hThread
    )
{
    DWORD Ret;

    LOGAPI("ResumeThread(hThread=%p)\n", hThread);

    Ret = ResumeThread(hThread);

    LOGAPI("ResumeThread returned DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_QueueUserAPC(
    IN PAPCFUNC pfnAPC,
    IN HANDLE hThread,
    IN ULONG_PTR dwData
    )
{
    DWORD Ret;

    LOGAPI("QueueUserAPC(pfnAPC=%p, hThread=%p, dwData=%p)\n", 
            pfnAPC, hThread, dwData);

    Ret = QueueUserAPC(pfnAPC, hThread, dwData);

    LOGAPI("QueueUserAPC returns DWORD 0x%x\n", Ret);
    return Ret;
}

#endif //DBG

PALIMPORT
BOOL
PALAPI
PAL_GetThreadContext(
    IN HANDLE hThread,
    IN OUT PAL_LPCONTEXT lpContext
    )
{
    BOOL Ret;

    LOGAPI("GetThreadContext(hThread=%p, lpContext=%p)\n", hThread, lpContext);

    // Make sure the thread isn't in our current process, or is the current
    // thread.
    if (VerifyThreadNotInCurrentProcess(hThread))
    {
        Ret = GetThreadContext(hThread, (LPCONTEXT)lpContext);
    }
    else
    {
        PALASSERT(FALSE);
        Ret = FALSE;
    }

    LOGAPI("GetThreadContext returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetThreadContext(
    IN HANDLE hThread,
    IN CONST PAL_CONTEXT * lpContext
    )
{
    BOOL Ret;

    LOGAPI("SetThreadContext(hThread=%p, lpContext=%p)\n", hThread, lpContext);

    // Make sure the thread isn't in our current process
    if (VerifyThreadNotInCurrentProcess(hThread))
    {
        Ret = SetThreadContext(hThread, (CONST CONTEXT *)lpContext);
    }
    else
    {
        PALASSERT(FALSE);
        Ret = FALSE;
    }

    LOGAPI("SetThreadContext returns BOOL %d\n", Ret);
    return Ret;
}

#if DBG

PALIMPORT
int
PALAPI
PAL_GetThreadPriority(
    IN HANDLE hThread
    )
{
    int Ret;

    LOGAPI("GetThreadPriority(hThread=%p)\n", hThread);

    Ret = GetThreadPriority(hThread);

    LOGAPI("GetThreadPriority returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetThreadPriority(
    IN HANDLE hThread,
    IN int nPriority
    )
{
    BOOL Ret;

    LOGAPI("SetThreadPriority(hThread=%p, nPriority=%d)\n", 
           hThread, nPriority);

    Ret = SetThreadPriority(hThread, nPriority);

    LOGAPI("SetThreadPriority returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_TlsAlloc(
    VOID
    )
{
    DWORD Ret;

    LOGAPI("TlsAlloc(void)\n");

    Ret = TlsAlloc();

    LOGAPI("TlsAlloc returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
LPVOID
PALAPI
PAL_TlsGetValue(
    IN DWORD dwTlsIndex
    )
{
    LPVOID Ret;

    LOGAPI("TlsGetValue(dwTlsIndex=0x%x)\n", dwTlsIndex);

    Ret = TlsGetValue(dwTlsIndex);

    LOGAPI("TlsGetValue returns LPVOID %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_TlsSetValue(
    IN DWORD dwTlsIndex,
    IN LPVOID lpTlsValue
    )
{
    BOOL Ret;

    LOGAPI("TlsSetValue(dwTlsIndex=0x%x, lpTlsValue=%p)\n", 
           dwTlsIndex, lpTlsValue);

    Ret = TlsSetValue(dwTlsIndex, lpTlsValue);

    LOGAPI("TlsSetValue returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_TlsFree(
    IN DWORD dwTlsIndex
    )
{
    BOOL Ret;

    LOGAPI("TlsFree(dwTlsIndex=0x%x)\n", dwTlsIndex);

    Ret = TlsFree(dwTlsIndex);

    LOGAPI("TlsFree returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_InitializeCriticalSection(
    OUT PAL_LPCRITICAL_SECTION lpCriticalSection
    )
{
    LOGAPI("InitializeCriticalSection(lpCriticalSecion=%p)\n", 
           lpCriticalSection);

    InitializeCriticalSection((LPCRITICAL_SECTION)lpCriticalSection);

    LOGAPI("InitializeCriticalSection returns void\n");
}

PALIMPORT
VOID
PALAPI
PAL_EnterCriticalSection(
    IN OUT PAL_LPCRITICAL_SECTION lpCriticalSection
    )
{
    LOGAPI("EnterCriticalSection(lpCriticalSecion=%p)\n", lpCriticalSection);

    EnterCriticalSection((LPCRITICAL_SECTION)lpCriticalSection);

    LOGAPI("EnterCriticalSection returns void\n");
}

PALIMPORT
VOID
PALAPI
PAL_LeaveCriticalSection(
    IN OUT PAL_LPCRITICAL_SECTION lpCriticalSection
    )
{
    LOGAPI("LeaveCriticalSection(lpCriticalSecion=%p)\n", lpCriticalSection);

    LeaveCriticalSection((LPCRITICAL_SECTION)lpCriticalSection);

    LOGAPI("LeaveCriticalSection returns void\n");
}

PALIMPORT
VOID
PALAPI
PAL_DeleteCriticalSection(
    IN OUT PAL_LPCRITICAL_SECTION lpCriticalSection
    )
{
    LOGAPI("DeleteCriticalSection(lpCriticalSecion=%p)\n", lpCriticalSection);

    DeleteCriticalSection((LPCRITICAL_SECTION)lpCriticalSection);

    LOGAPI("DeleteCriticalSection returns void\n");
}

PALIMPORT
BOOL
PALAPI
PAL_TryEnterCriticalSection(
    IN OUT PAL_LPCRITICAL_SECTION lpCriticalSection
    )
{
    BOOL Ret;

    LOGAPI("TryEnterCriticalSection(lpCriticalSecion=%p)\n", 
           lpCriticalSection);

    Ret = TryEnterCriticalSection((LPCRITICAL_SECTION)lpCriticalSection);

    LOGAPI("TryEnterCriticalSection returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
UINT
PALAPI
PAL_SetErrorMode(
    IN UINT uMode
    )
{
    UINT Ret;

    LOGAPI("SetErrorMode(uMode=0x%x)\n", uMode);

    Ret = SetErrorMode(uMode);

    LOGAPI("SetErrorMode returns UINT 0x%x\n", Ret);
    return Ret;
}

#endif

PALIMPORT
HANDLE
PALAPI
PAL_CreateFileMappingA(
    IN HANDLE hFile,
    IN PAL_LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    IN DWORD flProtect,
    IN DWORD dwMaximumSizeHigh,
    IN DWORD dwMaximumSizeLow,
    IN LPCSTR lpName
    )
{
    HANDLE Ret;
    LPCSTR lpObjectName;
    char MangledObjectName[MAX_PATH];

    LOGAPI("CreateFileMappingA(hFile=%p, lpFileMappingAttributes=%p, "
           "flProtect=0x%x, dwMaximumSizeHigh=0x%x, dwMaximumSizeLow=0x%x, "
           "lpName=%p (%s))\n", hFile, lpFileMappingAttributes, flProtect, 
           dwMaximumSizeHigh, dwMaximumSizeLow, lpName, lpName);

    PALASSERT(lpFileMappingAttributes == NULL);
    PALASSERT((flProtect & (PAGE_READONLY|PAGE_READWRITE|SEC_IMAGE|PAGE_WRITECOPY)) == 
              flProtect);
    PALASSERT(dwMaximumSizeHigh == 0);
    PALASSERT(hFile == (HANDLE)-1 || lpName == NULL);

    lpObjectName=lpName;
    if (!MangleObjectNameA(&lpObjectName, MangledObjectName)) {
	Ret = NULL;
    } else {
        Ret = CreateFileMappingA(hFile,
                                 (LPSECURITY_ATTRIBUTES)lpFileMappingAttributes,
                                 flProtect,
                                 dwMaximumSizeHigh,
                                 dwMaximumSizeLow,
                                 lpObjectName);
    }

    LOGAPI("CreateFileMappingA returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_CreateFileMappingW(
    IN HANDLE hFile,
    IN PAL_LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    IN DWORD flProtect,
    IN DWORD dwMaximumSizeHigh,
    IN DWORD dwMaximumSizeLow,
    IN LPCWSTR lpName
    )
{
    HANDLE Ret;
    LPCWSTR lpObjectName;
    WCHAR MangledObjectName[MAX_PATH];

    LOGAPI("CreateFileMappingW(hFile=%p, lpFileMappingAttributes=%p, "
           "flProtect=0x%x, dwMaximumSizeHigh=0x%x, dwMaximumSizeLow=0x%x, "
           "lpName=%p (%S))\n", hFile, lpFileMappingAttributes, flProtect, 
           dwMaximumSizeHigh, dwMaximumSizeLow, lpName, lpName);

    PALASSERT(lpFileMappingAttributes == NULL);
    PALASSERT((flProtect & (PAGE_READONLY|PAGE_READWRITE|SEC_IMAGE|PAGE_WRITECOPY)) == 
              flProtect);
    PALASSERT(dwMaximumSizeHigh == 0);
    PALASSERT(hFile == (HANDLE)-1 || lpName == NULL);

    lpObjectName=lpName;
    if (!MangleObjectNameW(&lpObjectName, MangledObjectName)) {
	Ret = NULL;
    } else {
        Ret = CreateFileMappingW(hFile,
                                 (LPSECURITY_ATTRIBUTES)lpFileMappingAttributes,
                                 flProtect,
                                 dwMaximumSizeHigh,
                                 dwMaximumSizeLow,
                                 lpObjectName);
    }

    LOGAPI("CreateFileMappingW returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_OpenFileMappingA(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpName
    )
{
    HANDLE Ret;
    LPCSTR lpObjectName;
    char MangledObjectName[MAX_PATH];

    LOGAPI("OpenFileMappingA(dwDesiredAccess=0x%x, bInheritHandle=%d, "
           "lpName=%p (%s))\n", dwDesiredAccess, bInheritHandle, lpName, 
           lpName);

    PALASSERT((dwDesiredAccess & 
               (FILE_MAP_READ|FILE_MAP_WRITE|FILE_MAP_ALL_ACCESS)) == 
              dwDesiredAccess);

    lpObjectName=lpName;
    if (!MangleObjectNameA(&lpObjectName, MangledObjectName)) {
	Ret = NULL;
    } else {
        Ret = OpenFileMappingA(dwDesiredAccess, 
                               bInheritHandle, 
                               lpObjectName);
    }

    LOGAPI("OpenFileMappingA returns HANDLE %p\n", Ret);
    return Ret;
}

PALIMPORT
HANDLE
PALAPI
PAL_OpenFileMappingW(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCWSTR lpName
    )
{
    HANDLE Ret;
    LPCWSTR lpObjectName;
    WCHAR MangledObjectName[MAX_PATH];

    LOGAPI("OpenFileMappingW(dwDesiredAccess=0x%x, bInheritHandle=%d, "
           "lpName=%p (%S))\n", dwDesiredAccess, bInheritHandle, lpName, 
           lpName);

    PALASSERT((dwDesiredAccess & 
               (FILE_MAP_READ|FILE_MAP_WRITE|FILE_MAP_ALL_ACCESS)) == 
              dwDesiredAccess);

    lpObjectName=lpName;
    if (!MangleObjectNameW(&lpObjectName, MangledObjectName)) {
	Ret = NULL;
    } else {
        Ret = OpenFileMappingW(dwDesiredAccess, 
                               bInheritHandle, 
                               lpObjectName);
    }

    LOGAPI("OpenFileMappingW returns HANDLE %p\n", Ret);
    return Ret;
}

#if DBG

PALIMPORT
LPVOID
PALAPI
PAL_MapViewOfFile(
    IN HANDLE hFileMappingObject,
    IN DWORD dwDesiredAccess,
    IN DWORD dwFileOffsetHigh,
    IN DWORD dwFileOffsetLow,
    IN SIZE_T dwNumberOfBytesToMap
    )
{
    LPVOID Ret;

    LOGAPI("MapViewOfFile(hFileMappingObject=%p, dwDesiredAccess=0x%x, "
           "dwFileOffsetHigh=0x%x, dwFileOffsetLow=0x%x, " 
           "dwNumberOfBytesToMap=%p)\n", hFileMappingObject, dwDesiredAccess, 
           dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);

    PALASSERT(dwFileOffsetHigh == 0);
    PALASSERT(dwFileOffsetLow == 0);

    Ret = MapViewOfFile(hFileMappingObject,
                        dwDesiredAccess,
                        dwFileOffsetHigh,
                        dwFileOffsetLow,
                        dwNumberOfBytesToMap);

    LOGAPI("MapViewOfFile returns LPVOID %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_UnmapViewOfFile(
    IN LPCVOID lpBaseAddress
    )
{
    BOOL Ret;

    LOGAPI("UnmapViewOfFile(lpBaseAddress=%p)\n", lpBaseAddress);

    Ret = UnmapViewOfFile(lpBaseAddress);

    LOGAPI("UnmapViewOfFile returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
PAL_HMODULE
PALAPI
PAL_LoadLibraryA(
    IN LPCSTR lpLibFileName
    )
{
    HMODULE Ret;

    LOGAPI("LoadLibraryA(lpLibFileName=%p (%s))\n", 
           lpLibFileName, lpLibFileName);

    Ret = LoadLibraryA(lpLibFileName);

    LOGAPI("LoadLibraryA returns HMODULE %p\n", Ret);
    return (PAL_HMODULE)Ret;
}

PALIMPORT
PAL_HMODULE
PALAPI
PAL_LoadLibraryW(
    IN LPCWSTR lpLibFileName
    )
{
    HMODULE Ret;

    LOGAPI("LoadLibraryW(lpLibFileName=%p (%S))\n", 
            lpLibFileName, lpLibFileName);

    Ret = LoadLibraryW(lpLibFileName);

    LOGAPI("LoadLibraryW returns HMODULE %p\n", Ret);
    return (PAL_HMODULE)Ret;
}

PALIMPORT
FARPROC
PALAPI
PAL_GetProcAddress(
    IN PAL_HMODULE hModule,
    IN LPCSTR lpProcName
    )
{
    FARPROC Ret;

    LOGAPI("GetProcAddress(hModule=%p, lpProcName=%p (%s))\n", 
           hModule, lpProcName, 
           ((UINT_PTR)lpProcName > 0xffff) ? lpProcName : "ORDINAL");

    //
    // Do not assert here that lpProcName is an ordinal.  On Win32, this
    // API may be called via P/Invoke on behalf of a managed application
    // which really does want to import a function by ordinal.
    //

    Ret = GetProcAddress((HMODULE)hModule, lpProcName);

    LOGAPI("GetProcAddress returns FARPROC %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FreeLibrary(
    IN OUT PAL_HMODULE hLibModule
    )
{
    BOOL Ret;

    LOGAPI("FreeLibrary(hLibModule=%p)\n", hLibModule);

    Ret = FreeLibrary((HMODULE)hLibModule);

    LOGAPI("FreeLibrary returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DECLSPEC_NORETURN
VOID
PALAPI
PAL_FreeLibraryAndExitThread(
    IN PAL_HMODULE hLibModule,
    IN DWORD dwExitCode
    )
{
    LOGAPI("FreeLibraryAndExitThread(hLibModule=%p, dwExitCode=0x%x)\n", 
           hLibModule, dwExitCode);

    FreeLibraryAndExitThread((HMODULE)hLibModule, dwExitCode);

    // this API never returns
}

PALIMPORT
BOOL
PALAPI
PAL_DisableThreadLibraryCalls(
    IN PAL_HMODULE hLibModule
    )
{
    BOOL Ret;

    LOGAPI("DisableThreadLibraryCalls(hLibModule=%p)\n", hLibModule);

    Ret = DisableThreadLibraryCalls((HMODULE)hLibModule);

    LOGAPI("DisableThreadLibraryCalls returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetModuleFileNameA(
    IN PAL_HMODULE hModule,
    OUT LPSTR lpFilename,
    IN DWORD nSize
    )
{
    DWORD Ret;

    LOGAPI("GetModuleFileNameA(hModule=%p, lpFilename=%p, nSize=0x%x)\n", 
           hModule, lpFilename, nSize);

    PALASSERT(hModule == NULL);

    Ret = GetModuleFileNameA((HMODULE)hModule, lpFilename, nSize);

    LOGAPI("GetModuleFileNameA returns DWORD 0x%x, lpFileName=%s\n", 
           hModule, (Ret) ? lpFilename : "API filed");
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetModuleFileNameW(
    IN PAL_HMODULE hModule,
    OUT LPWSTR lpFilename,
    IN DWORD nSize
    )
{
    DWORD Ret;

    LOGAPI("GetModuleFileNameW(hModule=%p, lpFilename=%p, nSize=0x%x)\n", 
           hModule, lpFilename, nSize);

    PALASSERT(hModule == NULL);

    Ret = GetModuleFileNameW((HMODULE)hModule, lpFilename, nSize);

    LOGAPI("GetModuleFileNameW returns DWORD 0x%x, lpFileName=%S\n", 
           hModule, (Ret) ? lpFilename : L"API filed");
    return Ret;
}

PALIMPORT
LPVOID
PALAPI
PAL_VirtualAlloc(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD flAllocationType,
    IN DWORD flProtect
    )
{
    LPVOID Ret;

    LOGAPI("VirtualAlloc(lpAddress=%p, dwSize=%p, flAllocationType=0x%x, "
           "flProtect=0x%x)\n", lpAddress, dwSize, flAllocationType, 
           flProtect);

    PALASSERT((flAllocationType & (MEM_COMMIT|MEM_RESERVE|MEM_TOP_DOWN)) == 
              flAllocationType);
    PALASSERT((flProtect & 
               (PAGE_NOACCESS|PAGE_READONLY|PAGE_READWRITE|
                PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE)) == 
              flProtect);

    Ret = VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);

    LOGAPI("VirtualAlloc returns LPVOID %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_VirtualFree(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    )
{
    BOOL Ret;

    LOGAPI("VirtualFree(lpAddress=%p, dwSize=%p, dwFreeType=0x%x)\n", 
           lpAddress, dwSize, dwFreeType);

    Ret = VirtualFree(lpAddress, dwSize, dwFreeType);

    LOGAPI("VirtualFree returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_VirtualProtect(
    IN  LPVOID lpAddress,
    IN  SIZE_T dwSize,
    IN  DWORD flNewProtect,
    OUT PDWORD lpflOldProtect
    )
{
    BOOL Ret;

    LOGAPI("VirtualProtect(lpAddress=%p, dwSize=%p, flNewProtect=0x%x, "
           "lpflOldProtect=%p)\n", lpAddress, dwSize, flNewProtect, 
           lpflOldProtect);

    PALASSERT((flNewProtect & 
               (PAGE_NOACCESS|PAGE_READONLY|PAGE_READWRITE|
                PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE)) == 
              flNewProtect);

    Ret = VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);

    if (lpflOldProtect) {
        LOGAPI("VirtualProtect returns BOOL %d, flOldProtect=0x%x\n", 
               Ret, *lpflOldProtect);
    } else {
        LOGAPI("VirtualProtect returns BOOL %d\n", Ret);
    }
    return Ret;
}

PALIMPORT
SIZE_T
PALAPI
PAL_VirtualQuery(
    IN LPCVOID lpAddress,
    OUT PAL_PMEMORY_BASIC_INFORMATION lpBuffer,
    IN SIZE_T dwLength
    )
{
    SIZE_T Ret;

    LOGAPI("VirtualQuery(lpAddress=%p, lpBuffer=%p, dwLength=%p)\n", 
           lpAddress, lpBuffer, dwLength);

    Ret = VirtualQuery(lpAddress, 
                       (PMEMORY_BASIC_INFORMATION)lpBuffer,
                       dwLength);

    LOGAPI("VirtualQuery returns SIZE_T %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_ReadProcessMemory(
    IN HANDLE hProcess,
    IN LPCVOID lpBaseAddress,
    OUT LPVOID lpBuffer,
    IN SIZE_T nSize,
    OUT SIZE_T * lpNumberOfBytesRead
    )
{
    BOOL Ret;

    LOGAPI("ReadProcessMemory(hProcess=%p, lpBaseAddress=%p, lpBuffer=%p, "
           "nSize=%p, lpNumberOfBytesRead=%p)\n", hProcess, lpBaseAddress, 
           lpBuffer, nSize, lpNumberOfBytesRead);

    Ret = ReadProcessMemory(hProcess, 
                            lpBaseAddress,
                            lpBuffer,
                            nSize,
                            lpNumberOfBytesRead);

    LOGAPI("ReadProcessMemory returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_WriteProcessMemory(
    IN HANDLE hProcess,
    IN LPVOID lpBaseAddress,
    IN LPVOID lpBuffer,
    IN SIZE_T nSize,
    OUT SIZE_T * lpNumberOfBytesWritten
    )
{
    BOOL Ret;

    LOGAPI("WriteProcessMemory(hProcess=%p, lpBaseAddress=%p, lpBuffer=%p, "
	   "nSize=%p, lpNumberOfBytesWritten=%p)\n",
	   hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);

    Ret = WriteProcessMemory(hProcess, 
			     lpBaseAddress,
			     lpBuffer,
			     nSize,
			     lpNumberOfBytesWritten);

    LOGAPI("WriteProcessMemory returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_RtlMoveMemory(
          IN PVOID Destination,
          IN CONST VOID *Source,
          IN SIZE_T Length)
{
    memmove(Destination, Source, Length);
}

PALIMPORT
HANDLE
PALAPI
PAL_GetProcessHeap( VOID )
{
    HANDLE Ret;

    LOGAPI("GetProcessHeap(void)\n");

    Ret = GetProcessHeap();

    LOGAPI("GetProcessHeap returns HANDLE %p\n", Ret);
    return Ret;
}


PALIMPORT
LPVOID
PALAPI
PAL_HeapAlloc(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN SIZE_T dwBytes
    )
{
    LPVOID Ret;

    LOGAPI("HeapAlloc(hHeap=%p, dwFlags=0x%x, dwBytes=%p)\n", 
           hHeap, dwFlags, dwBytes);

    PALASSERT(dwFlags == 0 || dwFlags == HEAP_ZERO_MEMORY);

    Ret = HeapAlloc(hHeap, dwFlags, dwBytes);

    LOGAPI("HeapAlloc returns LPVOID %p\n", Ret);
    return Ret;
}

PALIMPORT
LPVOID
PALAPI
PAL_HeapReAlloc(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem,
    IN SIZE_T dwBytes
    )
{
    LPVOID Ret;

    LOGAPI("HeapReAlloc(hHeap=%p, dwFlags=0x%x, lpMem=%p, dwBytes=%p)\n",
	   hHeap, dwFlags, lpMem, dwBytes);

    PALASSERT(dwFlags == 0);

    Ret = HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);

    LOGAPI("HEapReAlloc returns LPVOID %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_HeapFree(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem
    )
{
    BOOL Ret;

    LOGAPI("HeapFree(hHeap=%p, dwFlags=0x%x, lpMem=%p)\n", 
           hHeap, dwFlags, lpMem);

    PALASSERT(dwFlags == 0);

    Ret = HeapFree(hHeap, dwFlags, lpMem);

    LOGAPI("HeapFree returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
HLOCAL
PALAPI
PAL_LocalAlloc(
    IN UINT uFlags,
    IN SIZE_T uBytes
    )
{
    HLOCAL Ret;

    LOGAPI("LocalAlloc(uFlags=0x%x, uBytes=%p)\n", uFlags, uBytes);

    PALASSERT(uFlags == 0);

    Ret = LocalAlloc(uFlags, uBytes);

    LOGAPI("LocalAlloc returns HLOCAL %p\n", Ret);
    return Ret;
}

PALIMPORT
HLOCAL
PALAPI
PAL_LocalFree(
    IN HLOCAL hMem
    )
{
    HLOCAL Ret;

    LOGAPI("LocalFree(hMem=%p)\n", hMem);

    Ret = LocalFree(hMem);

    LOGAPI("LocalFree returns HLOCAL %p\n", Ret);
    return Ret;
}

PALIMPORT
HGLOBAL
PALAPI
PAL_GlobalFree(
    IN HGLOBAL hMem
    )
{
    HGLOBAL Ret;

    LOGAPI("GlobalFree(hMem=%p)\n", hMem);

    Ret = GlobalFree(hMem);

    LOGAPI("GlobalFree returns HGLOBAL %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FlushInstructionCache(
    IN HANDLE hProcess,
    IN LPCVOID lpBaseAddress,
    IN SIZE_T dwSize
    )
{
    BOOL Ret;

    LOGAPI("FlushInstructionCache(hProcess=%p, lpBaseAddress=%p, dwSize=%p)\n",
           hProcess, lpBaseAddress, dwSize);

    Ret = FlushInstructionCache(hProcess, lpBaseAddress, dwSize);

    LOGAPI("FlushInstructionCache returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetStringTypeExW(
                 IN LCID Locale,
                 IN DWORD dwInfoType,
                 IN LPCWSTR lpSrcStr,
                 IN int cchSrc,
                 OUT LPWORD lpCharType)
{
    BOOL Ret;

    LOGAPI("GetStringTypeExW(Locale=0x%x, dwInfoType=0x%x, lpSrcStr=%p (%S), "
           "cchSrc=0x%x, lpCharType=%p)\n", Locale, dwInfoType, lpSrcStr, 
           lpSrcStr, cchSrc, lpCharType);

    PALASSERT(Locale == LOCALE_USER_DEFAULT);
    PALASSERT(dwInfoType == CT_CTYPE1);
    PALASSERT(cchSrc == 1);

    Ret = GetStringTypeExW(Locale, 
                           dwInfoType,
                           lpSrcStr,
                           cchSrc,
                           lpCharType);

    LOGAPI("GetStringTypeExW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_CompareStringW(
                   IN LCID     Locale,
                   IN DWORD    dwCmpFlags,
                   IN LPCWSTR  lpString1,
                   IN int      cchCount1,
                   IN LPCWSTR  lpString2,
                   IN int      cchCount2)
{
    int Ret;

    LOGAPI("CompareStringW(Locale=0x%x, dwCmpFlags=0x%x, lpString1=%p (%S), "
           "cchCount1=0x%x, lpString2=%p, cchCount2=0x%x)\n", 
           Locale, dwCmpFlags, lpString1, 
           cchCount1, lpString2, cchCount2);

    PALASSERT((dwCmpFlags & (NORM_IGNORECASE|NORM_IGNOREWIDTH)) ==
              dwCmpFlags);
    PALASSERT(Locale == 0x0409);

    Ret = CompareStringW(Locale, 
                         dwCmpFlags,
                         lpString1,
                         cchCount1,
                         lpString2,
                         cchCount2);

    LOGAPI("CompareStringW returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
UINT
PALAPI
PAL_GetACP(
	   void)
{
    UINT Ret;

    LOGAPI("GetACP(void)\n");

    Ret = GetACP();

    LOGAPI("GetACP returns UINT 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetCPInfo(
          IN UINT CodePage,
          OUT PAL_LPCPINFO lpCPInfo)
{
    BOOL Ret;

    LOGAPI("GetCPInfo(CodePage=0x%x, lpCPInfo=%p)\n", CodePage, lpCPInfo);

    Ret = GetCPInfo(CodePage, (LPCPINFO)lpCPInfo);

    LOGAPI("GetCPInfo returns BOOL %d, lpCPInfo->MaxCharSize=0x%x\n", 
            Ret, lpCPInfo->MaxCharSize);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_IsDBCSLeadByteEx(
                 IN UINT CodePage,
                 IN BYTE TestChar)
{
    BOOL Ret;

    LOGAPI("IsDBCSLeadByteEx(CodePage=0x%x, TestChar=0x%x)\n", 
           CodePage, TestChar);

    Ret = IsDBCSLeadByteEx(CodePage, TestChar);

    LOGAPI("IsDBCSLeadByteEx returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_IsValidCodePage(
                 IN UINT CodePage)
{
    BOOL Ret;

    LOGAPI("IsValidCodePage(CodePage=0x%x)\n", 
           CodePage);

    Ret = IsValidCodePage(CodePage);

    LOGAPI("IsValidCodePage returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_MultiByteToWideChar(
                    IN UINT CodePage,
                    IN DWORD dwFlags,
                    IN LPCSTR lpMultiByteStr,
                    IN int cbMultiByte,
                    OUT LPWSTR lpWideCharStr,
                    IN int cchWideChar)
{
    int Ret;

    LOGAPI("MultiByteToWideChar(CodePage=0x%x, dwFlags=0x%x, "
           "lpMultiByteStr=%p, cbMultiByte=0x%x, lpWideCharStr=%p, "
           "cchWideChar=0x%x)\n", CodePage, dwFlags, lpMultiByteStr, 
           cbMultiByte, lpWideCharStr, cchWideChar);

    PALASSERT(dwFlags == 0 || dwFlags == MB_PRECOMPOSED);

    Ret = MultiByteToWideChar(CodePage,
                              dwFlags,
                              lpMultiByteStr,
                              cbMultiByte,
                              lpWideCharStr,
                              cchWideChar);

    LOGAPI("MultiByteToWideChar returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WideCharToMultiByte(
                    IN UINT CodePage,
                    IN DWORD dwFlags,
                    IN LPCWSTR lpWideCharStr,
                    IN int cchWideChar,
                    OUT LPSTR lpMultiByteStr,
                    IN int cbMultiByte,
                    IN LPCSTR lpDefaultChar,
                    OUT LPBOOL lpUsedDefaultChar)
{
    int Ret;

    LOGAPI("WideCharToMultiByte(CodePage=0x%x, dwFlags=0x%x, "
           "lpWideCharStr=%p, cchWideChar=0x%x, lpMultiByteStr=%p, "
           "cbMultiByte=0x%x, lpDefaultChar=%p, lpUsedDefaultChar=%p)\n", 
           CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, 
           cbMultiByte, lpDefaultChar, lpUsedDefaultChar);

    PALASSERT(dwFlags == 0);
    PALASSERT(lpDefaultChar == NULL);
    PALASSERT(lpUsedDefaultChar == NULL);

    Ret = WideCharToMultiByte(CodePage,
                              dwFlags,
                              lpWideCharStr,
                              cchWideChar,
                              lpMultiByteStr,
                              cbMultiByte,
                              lpDefaultChar,
                              lpUsedDefaultChar);

    LOGAPI("WideCharToMultiByte returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
LANGID
PALAPI
PAL_GetSystemDefaultLangID(
                       void)
{
    LANGID Ret;

    LOGAPI("GetSystemDefaultLangID(void)\n");

    Ret = GetSystemDefaultLangID();

    LOGAPI("GetSystemDefaultLangID returns LANGID 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
LANGID
PALAPI
PAL_GetUserDefaultLangID(
                     void)
{
    LANGID Ret;

    LOGAPI("GetUserDefaultLangID(void)\n");

    Ret = GetUserDefaultLangID();

    LOGAPI("GetUserDefaultLangID returns LANGID 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetThreadLocale(
                IN LCID Locale)
{
    BOOL Ret;

    LOGAPI("SetThreadLocale(Locale=0x%x)\n", Locale);

    Ret = SetThreadLocale(Locale);

    LOGAPI("SetThreadLocale returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
LCID
PALAPI
PAL_GetThreadLocale(
                void)
{
    LCID Ret;

    LOGAPI("GetThreadLocale(void)\n");

    Ret = GetThreadLocale();

    LOGAPI("GetThreadLocale returns LCID 0x%x\n", Ret);
    return Ret;
}

#define PAL_LEGAL_GETLOCALE_INFO_LCTYPES (LOCALE_SDECIMAL| \
                                          LOCALE_STHOUSAND| \
                                          LOCALE_ILZERO| \
                                          LOCALE_SCURRENCY| \
                                          LOCALE_SMONDECIMALSEP| \
                                          LOCALE_SMONTHOUSANDSEP)

PALIMPORT
int
PALAPI
PAL_GetLocaleInfoW(
    IN LCID     Locale,
    IN LCTYPE   LCType,
    OUT LPWSTR  lpLCData,
    IN int      cchData)
{
    int Ret;

    LOGAPI("GetLocaleInfoW(Locale=0x%lx, LCType=%d, lpLCData=%p, cchData=%d)\n", 
           Locale, LCType, lpLCData, cchData);


    PALASSERT(Locale == LOCALE_NEUTRAL);
    PALASSERT((LCType & ~PAL_LEGAL_GETLOCALE_INFO_LCTYPES) == 0);

    Ret = GetLocaleInfoW(Locale, LCType, lpLCData, cchData);

    LOGAPI("GetLocaleInfoW returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
LCID
PALAPI
PAL_GetUserDefaultLCID(
                   void)
{
    LCID Ret;

    LOGAPI("GetUserDefaultLCID(void)\n");

    Ret = GetUserDefaultLCID();

    LOGAPI("GetUserDefaultLCID returns LCID 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetTimeZoneInformation(
    OUT PAL_LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    )
{
    DWORD Ret;

    LOGAPI("GetTimeZoneInformation(lpTimeZoneInformation=%p)\n", 
           lpTimeZoneInformation);

    Ret = GetTimeZoneInformation(
            (LPTIME_ZONE_INFORMATION)lpTimeZoneInformation);

    LOGAPI("GetTimeZoneInformation returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_IsValidLocale(
              IN LCID Locale,
              IN DWORD dwFlags)
{
    BOOL Ret;

    LOGAPI("IsValidLocale(Locale=0x%x, dwFlags=0x%x)\n", Locale, dwFlags);

    PALASSERT(dwFlags == LCID_SUPPORTED || dwFlags == LCID_INSTALLED);

    Ret = IsValidLocale(Locale, dwFlags);

    LOGAPI("IsValidLocale returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_GetCalendarInfoW(
                 IN LCID Locale,
                 IN CALID Calendar,
                 IN CALTYPE CalType,
                 OUT LPWSTR lpCalData,
                 IN int cchData,
                 OUT LPDWORD lpValue)
{
    int Ret;

    LOGAPI("GetCalendarInfoW(Locale=0x%x, Calendar=0x%x, CalType=0x%x, "
           "lpCalData=%p, cchData=0x%x, lpValue=%p)\n", Locale, 
           Calendar, CalType, lpCalData, cchData, lpValue);

    PALASSERT(Locale == LOCALE_USER_DEFAULT);
    PALASSERT(CalType == (CAL_ITWODIGITYEARMAX|CAL_RETURN_NUMBER));
    PALASSERT(lpCalData == NULL);
    PALASSERT(cchData == 0);

    Ret = GetCalendarInfoW(Locale, 
                           Calendar,
                           CalType,
                           lpCalData,
                           cchData,
                           lpValue);

    LOGAPI("GetCalendarInfoW returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_GetDateFormatW(
               IN LCID Locale,
               IN DWORD dwFlags,
               IN CONST PAL_SYSTEMTIME *lpDate,
               IN LPCWSTR lpFormat,
               OUT LPWSTR lpDateStr,
               IN int cchDate)
{
    int Ret;

    LOGAPI("GetDateFormatW(Locale=0x%x, dwFlags=0x%x, lpDate=%p, "
           "lpFormat=%p (%S), lpDateStr, cchDate)\n", Locale, dwFlags,
           lpDate, lpFormat, lpFormat, lpDateStr, cchDate);

    // bcl\system\globalization\datetimeformatinfo.cs passes hard-coded
    // 0x0404 (SUBLANG_CHINESE_TRADITIONAL/LANG_CHINESE)
    PALASSERT(Locale == GetSystemDefaultLCID() || Locale == 0x0404);
    PALASSERT(dwFlags == DATE_USE_ALT_CALENDAR);
    PALASSERT(lpDate == NULL);
    PALASSERT(wcscmp(lpFormat, L"gg") == 0);

    Ret = GetDateFormatW(Locale,
                         dwFlags,
                         (SYSTEMTIME *)lpDate,
                         lpFormat,
                         lpDateStr,
                         cchDate);

    LOGAPI("GetDateFormatW returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_OutputDebugStringA(
                   IN LPCSTR lpOutputString)
{
    LOGAPI("OutputDebugStringA(lpOutputString=%p (%s))\n", 
            lpOutputString, lpOutputString);

    OutputDebugStringA(lpOutputString);

    LOGAPI("OutputDebugStringA returns void\n");
}

PALIMPORT
VOID
PALAPI
PAL_OutputDebugStringW(
    LPCWSTR lpOutputString
    )
{
    LOGAPI("OutputDebugStringW(lpOutputString=%p (%S))\n", 
           lpOutputString, lpOutputString);

    OutputDebugStringW(lpOutputString);

    LOGAPI("OutputDebugStringW returns void\n");
}


PALIMPORT
VOID
PALAPI
PAL_DebugBreak(
           VOID)
{
    LOGAPI("DebugBreak(void)\n");

    DebugBreak();

    LOGAPI("DebugBreak returns void\n");
}


PALIMPORT
HANDLE
PALAPI
PAL_OpenProcess(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwProcessId
    )
{
    HANDLE Ret;

    LOGAPI("OpenProcess(dwDesiredAccess=0x%x, bInheritHandle=%d, "
           "dwProcessId=0x%x)\n", dwDesiredAccess, bInheritHandle,
	   dwProcessId);

    PALASSERT(dwDesiredAccess == PROCESS_ALL_ACCESS);
    PALASSERT(bInheritHandle == FALSE);

    Ret = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);

    LOGAPI("OpenProcess returns HANDLE %p\n", Ret);
    return Ret;
}


PALIMPORT
DWORD
PALAPI
PAL_GetEnvironmentVariableA(
    IN LPCSTR lpName,
    OUT LPSTR lpBuffer,
    IN DWORD nSize
    )
{
    DWORD Ret;

    LOGAPI("GetEnvironmentVariableA(lpName=%p (%s), lpBuffer=%p, "
           "nSize=0x%x)\n", lpName, lpName, lpBuffer, nSize);

    Ret = GetEnvironmentVariableA(lpName, lpBuffer, nSize);

    if (Ret == 0 || Ret >= nSize) {
        LOGAPI("GetEnvironmentVariableA returns DWORD 0x%x\n", Ret);
    } else {
        LOGAPI("GetEnvironmentVariableA returns DWORD 0x%x, lpBuffer=%s\n", 
               Ret, lpBuffer);
    }
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetEnvironmentVariableW(
    IN LPCWSTR lpName,
    OUT LPWSTR lpBuffer,
    IN DWORD nSize
    )
{
    DWORD Ret;

    LOGAPI("GetEnvironmentVariableW(lpName=%p (%S), lpBuffer=%p, "
           "nSize=0x%x)\n", lpName, lpName, lpBuffer, nSize);

    Ret = GetEnvironmentVariableW(lpName, lpBuffer, nSize);

    if (Ret == 0 || Ret >= nSize) {
        LOGAPI("GetEnvironmentVariableW returns DWORD 0x%x\n", Ret);
    } else {
        LOGAPI("GetEnvironmentVariableW returns DWORD 0x%x, lpBuffer=%S\n", 
               Ret, lpBuffer);
    }
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetEnvironmentVariableA(
    IN LPCSTR lpName,
    IN LPCSTR lpValue
    )
{
    BOOL Ret;

    LOGAPI("SetEnvironmentVariableA(lpName=%p (%s), lpValue=%p (%s))\n", 
           lpName, lpName, lpValue, lpValue);

    Ret = SetEnvironmentVariableA(lpName, lpValue);

    LOGAPI("SetEnvironmentVariableA returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_SetEnvironmentVariableW(
    IN LPCWSTR lpName,
    IN LPCWSTR lpValue
    )
{
    BOOL Ret;

    LOGAPI("SetEnvironmentVariableW(lpName=%p (%S), lpValue=%p (%S))\n", 
           lpName, lpName, lpValue, lpValue);

    Ret = SetEnvironmentVariableW(lpName, lpValue);

    LOGAPI("SetEnvironmentVariableW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
LPWSTR
PALAPI
PAL_GetEnvironmentStringsW(
    VOID
    )
{
    LPWSTR Ret;

    LOGAPI("GetEnvironmentStringsW(void)\n");

    Ret = GetEnvironmentStringsW();

    LOGAPI("GetEnvironmentStringsW returns LPWSTR %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_FreeEnvironmentStringsW(
    IN LPWSTR EnvStrings
    )
{
    BOOL Ret;

    LOGAPI("FreeEnviromentStringsW(EnvStrings=%p)\n", EnvStrings);

    Ret = FreeEnvironmentStringsW(EnvStrings);

    LOGAPI("FreeEnvironmentStrings returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_CloseHandle(
    IN OUT HANDLE hObject
    )
{
    BOOL Ret;

    LOGAPI("CloseHandle(hObject=%p)\n", hObject);

    Ret = CloseHandle(hObject);

    LOGAPI("CloseHandle returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_RaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments
    )
{
    LOGAPI("RaiseException(dwExceptionCode=0x%x, dwExceptionFlags=0x%x, "
           "nNumberOfArguments=0x%x, lpArguments=%p)\n", dwExceptionCode, 
           dwExceptionFlags, nNumberOfArguments, lpArguments);

    PALASSERT(dwExceptionCode != 0);

    RaiseException(dwExceptionCode, 
                   dwExceptionFlags,
                   nNumberOfArguments,
                   lpArguments);

    LOGAPI("RaiseException returns void\n");
}

#endif //DBG


PALIMPORT
PAL_LPTOP_LEVEL_EXCEPTION_FILTER
PALAPI
PAL_SetUnhandledExceptionFilter(
    IN PAL_LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )
{
    LPTOP_LEVEL_EXCEPTION_FILTER Ret;

    LOGAPI("SetUnhandledExceptionFilter(lpTopLevelExceptionFilter=%p)\n", 
           lpTopLevelExceptionFilter);

    // not thread safe - same as the real Win32 implementation
    Ret = (LPTOP_LEVEL_EXCEPTION_FILTER)SEH_CurrentTopLevelFilter;
    SEH_CurrentTopLevelFilter = lpTopLevelExceptionFilter;

    LOGAPI("SetUnhandledExceptionFilter returns LPTOP_LEVEL_EXCEPTION_FILTER "
           "%p\n", Ret);
    return (PAL_LPTOP_LEVEL_EXCEPTION_FILTER)Ret;
}


#if DBG

PALIMPORT
DWORD
PALAPI
PAL_GetTickCount(
    VOID
    )
{
    DWORD Ret;

    LOGAPI("GetTickCount(void)\n");

    Ret = GetTickCount();

    LOGAPI("GetTickCount returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
LONG
PALAPI
PAL_InterlockedIncrement(
    IN OUT LONG volatile *Addend
)
{
    LONG Ret;

    LOGAPI("InterlockedIncrement(Addend=%p)\n", Addend);

    Ret = InterlockedIncrement((LPLONG)Addend);

    LOGAPI("InterlockedIncrement returns LONG 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
LONG
PALAPI
PAL_InterlockedDecrement(
    IN OUT LONG volatile *Addend
    )
{
    LONG Ret;

    LOGAPI("InterlockedDecrement(Addend=%p)\n", Addend);

    Ret = InterlockedDecrement((LPLONG)Addend);

    LOGAPI("InterlockedDecrement returns LONG 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
LONG
PALAPI
PAL_InterlockedExchange(
                    IN OUT LONG volatile *Target,
                    IN LONG Value
                    )
{
    LONG Ret;

    LOGAPI("InterlockedExchange(Target=%p, Value=0x%x)\n", Target, Value);

    Ret = InterlockedExchange((LONG *)Target, Value);

    LOGAPI("InterlockedExchange returns LONG 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
PVOID
PALAPI
PAL_InterlockedExchangePointer(
                    IN OUT PVOID volatile *Target,
                    IN PVOID Value
                    )
{
    PVOID Ret;

    LOGAPI("InterlockedExchangePointer(Target=%p, Value=%p)\n", Target, Value);

    Ret = InterlockedExchangePointer((PVOID *)Target, Value);

    LOGAPI("InterlockedExchangePointer returns PVOID %p\n", Ret);
    return Ret;
}

PALIMPORT
LONG
PALAPI
PAL_InterlockedCompareExchange(
    IN OUT LONG volatile *Target,
    IN LONG ExChange,
    IN LONG Comperand)
{
    LONG Ret;

    LOGAPI("InterlockedCompareExchange(Target=%p, ExChange=0x%x, "
           "Comperand=0x%x)\n", Target, ExChange, Comperand);

    Ret = InterlockedCompareExchange((LONG *)Target, ExChange, Comperand);

    LOGAPI("InterlockedCompareExchange returns LONG 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
PVOID
PALAPI
PAL_InterlockedCompareExchangePointer(
    IN OUT PVOID volatile *Target,
    IN PVOID ExChange,
    IN PVOID Comperand)
{
    PVOID Ret;

    LOGAPI("InterlockedCompareExchangePointer(Target=%p, ExChange=%p, "
           "Comperand=%p)\n", Target, ExChange, Comperand);

    Ret = InterlockedCompareExchangePointer(Target, ExChange, Comperand);

    LOGAPI("InterlockedCompareExchangePointer returns PVOID %p\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_IsBadReadPtr(
    IN CONST VOID *lp,
    IN UINT_PTR ucb
    )
{
    BOOL Ret;

    LOGAPI("IsBadReadPtr(lp=%p, ucb=%p)\n", lp, ucb);

    Ret = IsBadReadPtr(lp, ucb);

    LOGAPI("IsBadReadPtr returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_IsBadWritePtr(
    IN LPVOID lp,
    IN UINT_PTR ucb
    )
{
    BOOL Ret;

    LOGAPI("IsBadWritePtr(lp=%p, ucb=%p)\n", lp, ucb);

    Ret = IsBadWritePtr(lp, ucb);

    LOGAPI("IsBadWritePtr returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_IsBadCodePtr(
    IN FARPROC lp
    )
{
    BOOL Ret;

    LOGAPI("IsBadCodePtr(lp=%p)\n", lp);

    Ret = IsBadCodePtr(lp);

    LOGAPI("IsBadCodePtr returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_GetSystemTime(
    OUT PAL_LPSYSTEMTIME lpSystemTime
    )
{
    LOGAPI("GetSystemTime(lpSystemTime=%p)\n", lpSystemTime);

    GetSystemTime((LPSYSTEMTIME)lpSystemTime);

    LOGAPI("GetSystemTime returns void\n");
}

PALIMPORT
BOOL
PALAPI
PAL_FileTimeToDosDateTime(
    IN CONST PAL_FILETIME *lpFileTime,
    OUT LPWORD lpFatDate,
    OUT LPWORD lpFatTime
    )
{
    BOOL Ret;
    LOGAPI("FileTimeToDosDateTime(lpFileTime=%p, lpFatDate=%p, "
           "lpFatTime=%p)\n", lpFileTime, lpFatDate, lpFatTime);

    Ret = FileTimeToDosDateTime((FILETIME *)lpFileTime, lpFatDate, lpFatTime);

    LOGAPI("FileTimeToDosDateTime returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_DosDateTimeToFileTime(
    IN WORD wFatDate,
    IN WORD wFatTime,
    OUT PAL_LPFILETIME lpFileTime
    )
{
    BOOL Ret;

    LOGAPI("DosDateTimeToFileTime(wFatDate=0x%x, wFatTime=0x%x, "
	   "lpFileTime=%p)\n", wFatDate, wFatTime, lpFileTime);

    Ret = DosDateTimeToFileTime(wFatDate, wFatTime, (LPFILETIME)lpFileTime);

    LOGAPI("DosDateTimeToFileTime returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_GetSystemTimeAsFileTime(
    OUT PAL_LPFILETIME lpSystemTimeAsFileTime
    )
{
    LOGAPI("GetSystemTimeAsFileTime(lpSystemTimeAsFileTime=%p)\n", 
           lpSystemTimeAsFileTime);

    GetSystemTimeAsFileTime((LPFILETIME)lpSystemTimeAsFileTime);

    LOGAPI("GetSystemTimeAsFileTime returns void\n");
}


#define VALID_FORMATMESSAGE_FLAGS   \
    (FORMAT_MESSAGE_FROM_SYSTEM |   \
    FORMAT_MESSAGE_FROM_STRING |    \
    FORMAT_MESSAGE_IGNORE_INSERTS | \
    FORMAT_MESSAGE_ARGUMENT_ARRAY | \
    FORMAT_MESSAGE_ALLOCATE_BUFFER)

PALIMPORT
DWORD
PALAPI
PAL_FormatMessageW(
    IN DWORD dwFlags,
    IN LPCVOID lpSource,
    IN DWORD dwMessageId,
    IN DWORD dwLanguageId,
    OUT LPWSTR lpBuffer,
    IN DWORD nSize,
    va_list *Arguments
    )
{
    DWORD Ret;

    LOGAPI("FormatMessageW(dwFlags=0x%x, lpSource=%p, dwMessageId=0x%x, "
           "dwLanguageId=0x%x, lpBuffer=%p, nSize=0x%x, Arguments=%p)\n", 
           dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments);

    PALASSERT((dwFlags & VALID_FORMATMESSAGE_FLAGS) == dwFlags);
    PALASSERT(lpSource == NULL || (dwFlags & FORMAT_MESSAGE_FROM_STRING));
    PALASSERT(dwLanguageId == 0 || 
              dwLanguageId == MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));

    Ret = FormatMessageW(dwFlags, 
                         lpSource,
                         dwMessageId,
                         dwLanguageId,
                         lpBuffer,
                         nSize,
                         Arguments);

    if (Ret) {
        if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
            LOGAPI("FormatMessageW returns DWORD 0x%x, lpBuffer contains "
                   "[out] buffer pointer 0x%x (%S)\n",
                   Ret, lpBuffer, *(LPWSTR *)lpBuffer, *(LPWSTR *)lpBuffer);
        } else {
            LOGAPI("FormatMessageW returns DWORD 0x%x, lpBuffer=%S\n", 
                   Ret, lpBuffer);
        }
    } else {
        LOGAPI("FormatMessageW returns DWORD 0x%x\n", Ret);
    }
    return Ret;
}

PALIMPORT
DWORD
PALAPI
PAL_GetLastError(
    VOID
    )
{
    DWORD Ret;

    LOGAPI("GetLastError(void)\n");

    Ret = GetLastError();

    LOGAPI("GetLastError returns DWORD 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_SetLastError(
    IN DWORD dwErrCode
    )
{
    LOGAPI("SetLastError(dwErrCode=0x%x)\n", dwErrCode);

    SetLastError(dwErrCode);

    LOGAPI("SetLastError returns void\n");
}

PALIMPORT
LPWSTR
PALAPI
PAL_GetCommandLineW(
    VOID
    )
{
    LPWSTR Ret;

    LOGAPI("GetCommandLineW(void)\n");

    Ret = GetCommandLineW();

    LOGAPI("GetCommandLineW returns LPWSTR %p=%S\n", Ret, Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetVersionExA(
    IN OUT PAL_LPOSVERSIONINFOA lpVersionInformation
    )
{
    BOOL Ret;

    LOGAPI("GetVersionExA(lpVersionInformation=%p)\n", lpVersionInformation);

    Ret = GetVersionExA((LPOSVERSIONINFOA)lpVersionInformation);

    LOGAPI("GetVersionExA returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_GetVersionExW(
    IN OUT PAL_LPOSVERSIONINFOW lpVersionInformation
    )
{
    BOOL Ret;

    LOGAPI("GetVersionExW(lpVersionInformation=%p)\n", lpVersionInformation);

    Ret = GetVersionExW((LPOSVERSIONINFOW)lpVersionInformation);

    LOGAPI("GetVersionExW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
VOID
PALAPI
PAL_GetSystemInfo(
    OUT PAL_LPSYSTEM_INFO lpSystemInfo
    )
{
    LOGAPI("GetSystemInfo(lpSystemInfo=%p)\n", lpSystemInfo);

    GetSystemInfo((LPSYSTEM_INFO)lpSystemInfo);

    LOGAPI("GetSystemInfo returns void\n");
}

PALIMPORT
BOOL
PALAPI
PAL_GetDiskFreeSpaceW(
    IN LPCWSTR lpRootPathName,
    OUT LPDWORD lpSectorsPerCluster,
    OUT LPDWORD lpBytesPerSector,
    OUT LPDWORD lpNumberOfFreeClusters,
    OUT LPDWORD lpTotalNumberOfClusters
    )
{
    BOOL Ret;

    LOGAPI("GetDiskFreeSpaceW(lpRootPathName=%p (%S), "
           "lpSectorsPerCluster=%p, lpBytesPerSector=%p, "
           "lpNumberOfFreeClusters=%p, lpTotalNumberOfClusters=%p)\n", 
           lpRootPathName, lpRootPathName, lpSectorsPerCluster, 
           lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);

    Ret = GetDiskFreeSpaceW(lpRootPathName, 
                            lpSectorsPerCluster,
                            lpBytesPerSector,
                            lpNumberOfFreeClusters,
                            lpTotalNumberOfClusters);

    LOGAPI("GetDiskFreeSpaceW returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
void * 
__cdecl 
PAL_malloc(
    size_t bytes
    )
{
    void *Ret;

    LOGAPI("malloc(bytes=%p)\n", bytes);

    Ret = malloc(bytes);

    LOGAPI("malloc returns void* %p\n", Ret);
    return Ret;
}

PALIMPORT
void
__cdecl
PAL_free(
    void *pv
    )
{
    LOGAPI("free(pv=%p)\n", pv);

    free(pv);

    LOGAPI("free returns void\n");
}

PALIMPORT
void *
__cdecl
PAL_realloc(
    void *pv,
    size_t bytes
    )
{
    void *Ret;

    LOGAPI("realloc(pv=%p, bytes=%p)\n", pv, bytes);

    Ret = realloc(pv, bytes);

    LOGAPI("realloc returns void* %p\n", Ret);
    return Ret;
}

PALIMPORT
PAL_NORETURN 
void 
__cdecl
PAL_exit(
    int ExitCode
    )
{
    // Bump the refcount, so the LOGAPI call won't assert - 
    // exit() may be called without having first called
    // PAL_Initialize().
    InterlockedIncrement(&PalReferenceCount);

    LOGAPI("exit(ExitCode=0x%x)\n", ExitCode);

    exit(ExitCode);
    // no need to log - exit() doesn't return
}

PALIMPORT
int
__cdecl
PAL_atexit(
    void (__cdecl *function)(void)
    )
{
    int ret;
    
    LOGAPI("atexit(function=%p)\n", function);
    ret = atexit(function);
    LOGAPI("atexit returns int %d\n", ret);
    return ret;
}

PALIMPORT
void
__cdecl
PAL_qsort(
    void *base, 
    size_t num, 
    size_t width, 
    int (__cdecl *compare)(const void *, const void *)
    )
{
    LOGAPI("qsort(base=%p, num=%p, width=%p, compare=%p)\n", 
           base, num, width, compare);

    qsort(base, num, width, compare);

    LOGAPI("qsort returns void\n");
}

PALIMPORT
void *
__cdecl
PAL_bsearch(
    const void *list, 
    const void *target, 
    size_t elemsize, 
    size_t listsize,
    int (__cdecl *pfnCompare)(const void *, const void *)
    )
{
    void *Ret;

    LOGAPI("bsearch(list=%p, target=%p, elemsize=%p, listsize=%p, "
           "pfnCompare=%p)\n", list, target, elemsize, listsize, pfnCompare);

    Ret = bsearch(list, target, elemsize, listsize, pfnCompare);

    LOGAPI("bsearch returns void* %p\n", Ret);
    return Ret;
}

PALIMPORT
void
__cdecl
PAL__splitpath(
    const char *path, 
    char *drive, 
    char *dir,
    char *fname,
    char *ext
    )
{
    LOGAPI("_splitpath(path=%p, drive=%p, dir=%p, fname=%p, ext=%p)\n", 
           path, drive, dir, fname, ext);

    _splitpath(path, drive, dir, fname, ext);

    LOGAPI("_splitpath returns void\n");
}

PALIMPORT
void
__cdecl
PAL__wsplitpath(
    const wchar_t *path,
    wchar_t *drive,
    wchar_t *dir,
    wchar_t *fname,
    wchar_t *ext
    )
{
    LOGAPI("_wsplitpath(path=%p, drive=%p, dir=%p, fname=%p, ext=%p)\n", 
           path, drive, dir, fname, ext);

    _wsplitpath(path, drive, dir, fname, ext);

    LOGAPI("_wsplitpath returns void\n");
}

PALIMPORT
void
__cdecl
PAL__makepath(
	      char *path,
	      const char *drive,
	      const char *dir,
	      const char *fname,
	      const char *ext)
{
    LOGAPI("_makepath(path=%p, drive=%p (%s), dir=%p (%s), fname=%p (%s) "
           "ext=%p (%s))\n", 
           path, drive, drive, dir, dir, fname, fname, ext, ext);

    _makepath(path, drive, dir, fname, ext);

    LOGAPI("_makepath returns void, path=%p (%s)\n", path, path);
}

PALIMPORT
void
__cdecl
PAL__wmakepath(
	       wchar_t *path,
	       const wchar_t *drive,
	       const wchar_t *dir,
	       const wchar_t *fname,
	       const wchar_t *ext)
{
    LOGAPI("_wmakepath(path=%p, drive=%p (%S), dir=%p (%S), fname=%p (%S) "
           "ext=%p (%S))\n", 
           path, drive, drive, dir, dir, fname, fname, ext, ext);

    _wmakepath(path, drive, dir, fname, ext);

    LOGAPI("_wmakepath returns void, path=%p (%S)\n", path, path);
}

PALIMPORT
char * 
__cdecl 
PAL__fullpath(char *abspath, const char *relpath, size_t maxLength)
{
    char *ret;

    LOGAPI("_fullpath(abspath=%p, relpath=%p (%s), maxLength=%p)\n",
	   abspath, relpath, relpath, maxLength);

    PALASSERT(abspath != NULL);
    PALASSERT(maxLength <= _MAX_PATH);

    ret = _fullpath(abspath, relpath, maxLength);

    LOGAPI("_fullpath returns char *%p (%s)\n", ret, ret);
    return ret;
}


PALIMPORT
void
__cdecl
PAL__swab(
    char *a, 
    char *b, 
    int c
    )
{
    LOGAPI("_swab(a=%p, b=%p, c=0x%x)\n", a, b, c);

    _swab(a, b, c);

    LOGAPI("_swab returns void\n");
}

PALIMPORT
time_t
__cdecl
PAL_time(
    time_t *pTime
    )
{
    time_t Ret;

    LOGAPI("time(pTime=%p)\n", pTime);

    Ret = time(pTime);

    LOGAPI("time returns time_t %p\n", Ret);
    return Ret;
}

PALIMPORT
struct PAL_tm *
__cdecl
PAL_localtime(
    const time_t *pTime
    )
{
    struct tm *Ret;

    LOGAPI("localtime(pTime=%p)\n", pTime);

    Ret = localtime(pTime);

    LOGAPI("localtime returns struct tm * %p\n", Ret);
    return (struct PAL_tm *)Ret;
}

PALIMPORT
time_t
__cdecl
PAL_mktime(
    struct PAL_tm *pTm
    )
{
    time_t Ret;

    LOGAPI("mktime(pTm=%p)\n", pTm);

    Ret = mktime((struct tm*)pTm);

    LOGAPI("mktime returns time_t %p\n", Ret);
    return Ret;
}

PALIMPORT
int
__cdecl
PAL__open_osfhandle(
    INT_PTR osfhandle,
    int flags
    )
{
    int Ret;

    LOGAPI("_open_osfhandle(osfhandle=%p, flags=0x%x)\n", osfhandle, flags);

    PALASSERT(flags == _O_RDONLY);

    Ret = _open_osfhandle(osfhandle, flags);

    LOGAPI("_open_osfhandle returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
__cdecl
PAL_fclose(
    PAL_FILE *fp
    )
{
    int Ret;

    LOGAPI("fclose(fp=%p)\n", fp);

    Ret = fclose((FILE *)fp);

    LOGAPI("fclose returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
__cdecl
PAL_fflush(
    PAL_FILE *fp
    )
{
    int Ret;

    LOGAPI("fflush(fp=%p)\n", fp);

    Ret = fflush((FILE *)fp);

    LOGAPI("fflush returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
size_t
__cdecl
PAL_fwrite(
    const void *pv,
    size_t size, 
    size_t count, 
    PAL_FILE *fp
    )
{
    size_t Ret;

    LOGAPI("fwrite(pv=%p, size=%p, count=%p, fp=%p)\n", pv, size, count, fp);

    Ret = fwrite(pv, size, count, (FILE *)fp);

    LOGAPI("fwrite returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
size_t
__cdecl
PAL_fread(
    void *pv,
    size_t size, 
    size_t count, 
    PAL_FILE *fp
    )
{
    size_t Ret;

    LOGAPI("fread(pv=%p, size=%p, count=%p, fp=%p)\n", pv, size, count, fp);

    Ret = fread(pv, size, count, (FILE *)fp);

    LOGAPI("fread returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT 
char *
__cdecl
PAL_fgets(
    char *string, 
    int n, 
    PAL_FILE *stream
    )
{
    char *Ret;

    LOGAPI("fgets(string=%p, n=0x%x, stream=%p)\n", string, n, stream);

    Ret = fgets(string, n, (FILE *)stream);

    LOGAPI("fgets returns char * %p\n", Ret);
    return Ret;
}

PALIMPORT 
int 
__cdecl 
PAL_fputs(
	  const char *string,
	  PAL_FILE *stream
	  )
{
    int Ret;

    LOGAPI("fputs(string=%p, stream=%p)\n", string, stream);

    Ret = fputs(string, (FILE *)stream);

    LOGAPI("fputs returns int 0x0x\n", Ret);
    return Ret;
}


PALIMPORT
int 
__cdecl 
PAL__getw(
    PAL_FILE *fp
    )
{
    int Ret;

    LOGAPI("_getw(fp=%p)\n", fp);

    Ret = _getw((FILE *)fp);

    LOGAPI("getw returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
__cdecl
PAL__putw(
    int Value, 
    PAL_FILE *fp
    )
{
    int Ret;

    LOGAPI("_putw(Value=0x%x, fp=%p)\n", Value, fp);

    Ret = _putw(Value, (FILE *)fp);

    LOGAPI("_putw returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int 
__cdecl 
PAL_fseek(
    PAL_FILE *fp, 
    long off, 
    int origin
    )
{
    int Ret;

    LOGAPI("fseek(fp=%p, off=0x%lx, origin=0x%x)\n", fp, off, origin);

    Ret = fseek((FILE *)fp, off, origin);

    LOGAPI("fseek returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
long
__cdecl
PAL_ftell(
    PAL_FILE *fp
    )
{
    long Ret;

    LOGAPI("ftell(fp=%p)\n", fp);

    Ret = ftell((FILE *)fp);

    LOGAPI("ftell returns long 0x%lx\n", Ret);
    return Ret;
}

PALIMPORT 
int 
__cdecl 
PAL_feof(
    PAL_FILE *fp
    )
{
    int Ret;

    LOGAPI("feof(fp=%p)\n", fp);

    Ret = feof((FILE *)fp);

    LOGAPI("feof returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int 
__cdecl 
PAL_ferror(
    PAL_FILE *fp
    )
{
    int Ret;

    LOGAPI("ferror(fp=%p)\n", fp);

    Ret = ferror((FILE *)fp);

    LOGAPI("ferror returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT 
PAL_FILE *
__cdecl
PAL_fopen(
    const char *filename,
    const char *mode
    )
{
    FILE *Ret;

    LOGAPI("fopen(filename=%p (%s), mode=%p (%s)\n", filename, filename, mode, mode);

    Ret = fopen(filename, mode);

    LOGAPI("fopen returns FILE * %p\n", Ret);
    return (PAL_FILE *)Ret;
}

PALIMPORT 
PAL_FILE *
__cdecl
PAL__fdopen(
    int handle, 
    const char *mode
    )
{
    FILE *Ret;

    LOGAPI("_fdopen(handle=0x%x, mode=%p (%s)\n", handle, mode, mode);

    Ret = _fdopen(handle, mode);

    LOGAPI("_fdopen returns FILE * %p\n", Ret);
    return (PAL_FILE *)Ret;
}

PALIMPORT
int
__cdecl
PAL__close(
    int handle
)
{
    int Ret;

    LOGAPI("_close(handle=0x%x)\n", handle);

    Ret = _close(handle);

    LOGAPI("_close returns int 0x%x\n", Ret);
    return Ret;
}


PALIMPORT 
PAL_FILE *
__cdecl
PAL__wfopen(
    const wchar_t *filename,
    const wchar_t *mode
    )
{
    FILE *Ret;

    LOGAPI("_wfopen(filename=%p (%S), mode=%p (%S)\n", filename, filename, mode, mode);

    Ret = _wfopen(filename, mode);

    LOGAPI("_wfopen returns FILE * %p\n", Ret);
    return (PAL_FILE *)Ret;
}

PALIMPORT
BOOL
PALAPI
PAL_IsDBCSLeadByte(
    IN BYTE  TestChar)
{
    BOOL Ret;

    LOGAPI("IsDBCSLeadByte(TestChar=0x%x)\n", TestChar);

    Ret = IsDBCSLeadByte(TestChar);

    LOGAPI("IsDBCSLeadByte returns BOOL %d\n", Ret);
    return Ret;
}

PALIMPORT
char *
__cdecl
PAL_getenv(
	   const char *name)
{
    char *Ret;

    LOGAPI("getenv(name=%p (%s))\n", name, name);

    Ret = getenv(name);

    LOGAPI("getenv returns char *%p (%s)\n", Ret, Ret);
    return Ret;
}

PALIMPORT
int 
__cdecl 
PAL__putenv(const char *var)
{
    int Ret;

    LOGAPI("_putenv(var=%p (%s))\n", var, var);

    Ret = _putenv(var);

    LOGAPI("_putenv returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT 
struct PAL_hostent FAR * 
PALAPI 
PAL_gethostbyname(
    	          IN const char FAR * name)
{
    struct hostent FAR *Ret;

    LOGAPI("gethostbyname(name=%p (%s))\n", name);

    Ret = gethostbyname(name);

    LOGAPI("gethostbyname returns struct hostent FAR *%p\n", Ret);
    return (struct PAL_hostent FAR *)Ret;
}

PALIMPORT 
struct PAL_hostent FAR *
PALAPI 
PAL_gethostbyaddr(
	          IN const char FAR * addr,
	          IN int len,
	          IN int type)
{
    struct hostent FAR *Ret;

    LOGAPI("gethostbyaddr(addr=%p, len=0x%x, type=0x%x)\n",
	   addr, len, type);

    PALASSERT(sizeof(int) == len);
    PALASSERT(AF_INET == type);

    Ret = gethostbyaddr(addr, len, type);

    LOGAPI("gethostbyaddr returns struct hostent FAR *%p\n", Ret);
    return (struct PAL_hostent FAR *)Ret;
}

PALIMPORT
int 
PALAPI FAR
PAL_gethostname(
	        OUT char FAR * name,
	        IN int namelen)
{
    int Ret;

    LOGAPI("gethostname(name=%p, namelen=0x%x)\n", name, namelen);

    Ret = gethostname(name, namelen);

    LOGAPI("gethostname returns int 0x%x\n", Ret);
    return Ret;
}

// note: diff from win32
PALIMPORT
LONG 
PALAPI 
PAL_inet_addr(
	      IN const char FAR * cp)
{
    LONG Ret;

    LOGAPI("inet_addr(cp=%p (%s))\n", cp, cp);

    Ret = inet_addr(cp);

    LOGAPI("inet_addr returns LONG 0x%x\n", Ret);
    return Ret;
}

PALIMPORT 
int
PALAPI
PAL_getpeername(
	        IN SOCKET s,
	        OUT struct PAL_sockaddr FAR *name,
	        IN OUT int FAR * namelen)
{
    int Ret;

    LOGAPI("getpeername(s=%p, name=%p, namelen=%p)\n", s, name, namelen);

    Ret = getpeername(s, (struct sockaddr FAR *)name, namelen);

    LOGAPI("getpeername returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI 
PAL_getsockopt(
	       IN SOCKET s,
	       IN int level,
	       IN int optname,
	       OUT char FAR * optval,
	       IN OUT int FAR *optlen)
{
    int Ret;

    LOGAPI("getsockopt(s=%p, level=0x%x, optname=0x%x, optval=%p, "
           "optlen=%p)\n", s, level, optname, optval, optlen);

    Ret = getsockopt(s, level, optname, optval, optlen);

    LOGAPI("getsockopt returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI 
PAL_setsockopt(
	       IN SOCKET s,
	       IN int level,
	       IN int optname,
	       IN const char FAR *optval,
	       IN int optlen)
{
    int Ret;

    LOGAPI("setsockopt(s=%p, level=0x%x, optname=0x%x, optval=%p, "
           "optlen=%d)\n", s, level, optname, optval, optlen);

    Ret = setsockopt(s, level, optname, optval, optlen);

    LOGAPI("setsockopt returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int 
PALAPI
PAL_connect(
	    IN SOCKET s,
	    IN const struct PAL_sockaddr FAR *name,
	    IN int namelen)
{
    int Ret;

    LOGAPI("connect(s=%p, name=%p, namelen=0x%x)\n", s, name, namelen);

    Ret = connect(s, (const struct sockaddr FAR *)name, namelen);

    LOGAPI("connect returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int 
PALAPI
PAL_send(
	 IN SOCKET s,
	 IN const char FAR * buf,
	 IN int len,
	 IN int flags)
{
    int Ret;

    LOGAPI("send(s=%p, buf=%p, len=0x%x, flags=0x%x)\n", s, buf, len, flags);

    Ret = send(s, buf, len, flags);

    LOGAPI("send returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int 
PALAPI 
PAL_recv(
	 IN SOCKET s,
	 OUT char FAR * buf,
	 IN int len,
	 IN int flags)
{
    int Ret;

    LOGAPI("recv(s=%p, buf=%p, len=0x%x, flags=0x%x)\n", s, buf, len, flags);

    Ret = recv(s, buf, len, flags);

    LOGAPI("recv returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_closesocket(
	        IN SOCKET s)
{
    int Ret;

    LOGAPI("closesocket(s=%p)\n", s);

    Ret = closesocket(s);

    LOGAPI("closesocket returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
SOCKET 
PALAPI 
PAL_accept(
	   IN SOCKET s,
	   OUT struct PAL_sockaddr FAR *addr,
	   IN OUT int FAR *addrlen)
{
    SOCKET Ret;

    LOGAPI("accept(s=%p, addr=%p, addrlen=%p)\n", s, addr, addrlen);

    Ret = accept(s, (struct sockaddr FAR *)addr, addrlen);

    LOGAPI("accept returns SOCKET %p\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_listen(
	   IN SOCKET s,
	   IN int backlog)
{
    int Ret;

    LOGAPI("listen(s=%p, backlock=0x%x)\n", s, backlog);

    Ret = listen(s, backlog);

    LOGAPI("listen returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int 
PALAPI
PAL_bind(
	 IN SOCKET s,
	 IN const struct PAL_sockaddr FAR *addr,
	 IN int namelen)
{
    int Ret;

    LOGAPI("bind(s=%p, addr=%p, namelen=0x%x)\n", s, addr, namelen);

    Ret = bind(s, (const struct sockaddr FAR *)addr, namelen);

    LOGAPI("bind returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_shutdown(
	     IN SOCKET s,
	     IN int how)
{
    int Ret;

    LOGAPI("shutdown(s=%p, how=0x%x)\n", s, how);

    Ret = shutdown(s, how);

    LOGAPI("shutdown returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_sendto(
	   IN SOCKET s,
	   IN const char FAR * buf,
	   IN int len,
	   IN int flags,
	   IN const struct PAL_sockaddr FAR *to,
	   IN int tolen)
{
    int Ret;

    LOGAPI("sendto(s=%p, buf=%p, len=0x%x, flags=0x%x, to=%p, tolen=0x%x)\n",
	   s, buf, len, flags, to, tolen);

    Ret = sendto(s, buf, len, flags, (const struct sockaddr FAR *)to, tolen);

    LOGAPI("sendto returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_recvfrom(
	     IN SOCKET s,
	     OUT char FAR * buf,
	     IN int len,
	     IN int flags,
	     OUT struct PAL_sockaddr FAR *from,
	     IN OUT int FAR * fromlen)
{
    int Ret;

    LOGAPI("recvfrom(s=%p, buf=%p, len=0x%x, flags=0x%x, from=%p, "
           "fromlen=%p)\n", s, buf, len, flags, from, fromlen);

    Ret = recvfrom(s, buf, len, flags, (struct sockaddr FAR *)from, fromlen);

    LOGAPI("recvfrom returns int 0x%x\n", Ret);
    return Ret;
}


PALIMPORT
int 
PALAPI 
PAL_getsockname(
		IN SOCKET s,
		OUT struct PAL_sockaddr FAR *name,
		IN OUT int FAR * namelen)
{
    int Ret;

    LOGAPI("getsockname(s=%p, name=%p, namelen=%p)\n", s, name, namelen);

    Ret = getsockname(s, (struct sockaddr FAR *)name, namelen);

    LOGAPI("getsockname returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int 
PALAPI 
PAL_select(
	   IN int nfds,
	   IN OUT PAL_fd_set FAR *readfds,
	   IN OUT PAL_fd_set FAR *writefds,
	   IN OUT PAL_fd_set FAR *exceptfds,
	   IN const struct PAL_timeval FAR *timeout)
{
    int Ret;

    LOGAPI("select(nfds=0x%x, readfds=%p, writefds=%p, exceptfds=%p, "
	   "timeout=%p)\n", nfds, readfds, writefds, exceptfds, timeout);

    PALASSERT(0 == nfds);

    Ret = select(nfds, (fd_set FAR *)readfds, (fd_set FAR *)writefds,
                 (fd_set FAR *)exceptfds, (const struct timeval FAR *)timeout);

    LOGAPI("select returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
SOCKET
PALAPI
PAL_socket(
	   IN int af,
	   IN int type,
	   IN int protocol)
{
    SOCKET Ret;

    LOGAPI("socket(af=0x%x, type=0x%x, protocol=0x%x)\n", af, type, protocol);

    Ret = socket(af, type, protocol);

    LOGAPI("socket returns SOCKET %p\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSAStartup(
	       IN WORD wVersionRequired,
	       OUT PAL_LPWSADATA lpWSAData)
{
    int Ret;

    LOGAPI("WSAStartup(wVersionRequired=0x%x, lpWSAData=%p)\n",
	   wVersionRequired, lpWSAData);

    PALASSERT(0x0202 == wVersionRequired);

    Ret = WSAStartup(wVersionRequired, (LPWSADATA)lpWSAData);

    LOGAPI("WSAStartup returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSACleanup(
	       void)
{
    int Ret;

    LOGAPI("WSACleanup(void)\n");

    Ret = WSACleanup();

    LOGAPI("WSACleanup returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSASend(
	    IN SOCKET s,
	    IN PAL_LPWSABUF lpBuffers,
	    IN DWORD dwBufferCount,
	    OUT LPDWORD lpNumberOfBytesSent,
	    IN DWORD dwFlags,
	    IN PAL_LPWSAOVERLAPPED lpOverlapped,
	    IN PAL_LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int Ret;

    LOGAPI("WSASend(s=%p, lpBuffers=%p, dwBufferCount=0x%x, "
           "lpNumberOfBytesSent=%p, dwFlags=0x%x, lpOverlapped=%p, "
           "lpCompletionRoutine=%p)\n", s, lpBuffers, dwBufferCount,
           lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);

    PALASSERT(lpOverlapped);
    PALASSERT(!lpCompletionRoutine);

    Ret = WSASend(s, (LPWSABUF)lpBuffers, dwBufferCount, lpNumberOfBytesSent, 
                  dwFlags, (LPWSAOVERLAPPED) lpOverlapped, 
                  (LPWSAOVERLAPPED_COMPLETION_ROUTINE)lpCompletionRoutine);

    LOGAPI("WSASend returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSASendTo(
	      IN SOCKET s,
	      IN PAL_LPWSABUF lpBuffers,
	      IN DWORD dwBufferCount,
	      OUT LPDWORD lpNumberOfBytesSent,
	      IN DWORD dwFlags,
	      IN const struct PAL_sockaddr FAR *lpTo,
	      IN int iTolen,
	      IN PAL_LPWSAOVERLAPPED lpOverlapped,
	      IN PAL_LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int Ret;

    LOGAPI("WSASendTo(s=%p, lpBuffers=%p, dwBufferCount=0x%x, "
           "lpNumberOfBytesSent=%p, dwFlags=0x%x, lpTo=%p, iToLen=0x%x, "
           "lpOverlapped=%p, lpCompletionRoutine=%p)\n",
	   s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags,
	   lpTo, iTolen, lpOverlapped, lpCompletionRoutine);

    PALASSERT(1 == dwBufferCount);
    PALASSERT(lpOverlapped);
    PALASSERT(!lpCompletionRoutine);

    Ret = WSASendTo(s, (LPWSABUF)lpBuffers, dwBufferCount, 
                    lpNumberOfBytesSent, dwFlags,
                    (const struct sockaddr FAR *)lpTo, iTolen, 
		    (LPWSAOVERLAPPED) lpOverlapped, 
                    (LPWSAOVERLAPPED_COMPLETION_ROUTINE)lpCompletionRoutine);

    LOGAPI("WSASendTo returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSARecv(
	    IN SOCKET s,
	    IN OUT PAL_LPWSABUF lpBuffers,
	    IN DWORD dwBufferCount,
	    OUT LPDWORD lpNumberOfBytesRecvd,
	    IN OUT LPDWORD lpFlags,
	    IN PAL_LPWSAOVERLAPPED lpOverlapped,
	    IN PAL_LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int Ret;

    LOGAPI("WSARecv(s=%p, lpBuffers=%p, dwBufferCount=0x%x, "
           "lpNumberOfBytesRecvd=%p, lpFlags=%p, lpOverlapped=%p, "
           "lpCompletionRoutine=%p)\n", s, lpBuffers, dwBufferCount,
	   lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);

    PALASSERT(1 == dwBufferCount);
    PALASSERT(lpOverlapped);
    PALASSERT(!lpCompletionRoutine);
    
    Ret = WSARecv(s, (LPWSABUF)lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
		  lpFlags, (LPWSAOVERLAPPED) lpOverlapped, 
                  (LPWSAOVERLAPPED_COMPLETION_ROUTINE)lpCompletionRoutine);

    LOGAPI("WSARecv returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSARecvFrom(
		IN SOCKET s,
		IN OUT PAL_LPWSABUF lpBuffers,
		IN DWORD dwBufferCount,
		OUT LPDWORD lpNumberOfBytesRecvd,
		IN OUT LPDWORD lpFlags,
		OUT struct PAL_sockaddr FAR *lpFrom,
		IN OUT LPINT lpFromLen,
		IN PAL_LPWSAOVERLAPPED lpOverlapped,
		IN PAL_LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int Ret;

    LOGAPI("WSARecvFrom(s=%p, lpBuffers=%p, dwBufferCount=0x%x, "
           "lpNumberOfBytesRecvd=%p, lpFlags=%p, lpFrom=%p, lpFromLen=%p, "
           "lpOverlapped=%p, lpCompletionRoutine=%p)\n",
	   s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags,
	   lpFrom, lpFromLen, lpOverlapped, lpCompletionRoutine);

    PALASSERT(1 == dwBufferCount);
    PALASSERT(lpOverlapped);
    PALASSERT(!lpCompletionRoutine);

    Ret = WSARecvFrom(s, (LPWSABUF)lpBuffers, dwBufferCount, 
		      lpNumberOfBytesRecvd, lpFlags, 
                      (struct sockaddr FAR *)lpFrom, lpFromLen, 
                      (LPWSAOVERLAPPED) lpOverlapped, 
                      (LPWSAOVERLAPPED_COMPLETION_ROUTINE)lpCompletionRoutine);

    LOGAPI("WSARevFrom returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSAEventSelect(
		   IN SOCKET s,
		   IN WSAEVENT hEventObject,
		   IN int lNetworkEvents)
{
    int Ret;

    LOGAPI("WSAEventSelect(s=%p, hEventObject=%p, lNetworkEvents=0x%x)\n",
	   s, hEventObject, lNetworkEvents);

    Ret = WSAEventSelect(s, hEventObject, lNetworkEvents);

    LOGAPI("WSAEventSelect returns 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSAEnumNetworkEvents(
		   IN SOCKET s,
		   IN WSAEVENT hEventObject,
		   OUT PAL_LPWSANETWORKEVENTS lpNetworkEvents)
{
    int Ret;

    LOGAPI("WSAEnumNetworkEvents(s=%p, hEventObject=%p, lpNetworkEvents=%p)\n",
	   s, hEventObject, lpNetworkEvents);

    Ret = WSAEnumNetworkEvents(s, hEventObject, (LPWSANETWORKEVENTS) lpNetworkEvents);

    LOGAPI("WSAEnumNetworkEvents returns 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
SOCKET
PALAPI
PAL_WSASocketA(
	       IN int af,
	       IN int type,
	       IN int protocol,
	       PAL_LPWSAPROTOCOL_INFOA lpProtocolInfo,
	       IN GROUP g,
	       IN DWORD dwFlags)
{
    SOCKET Ret;

    LOGAPI("WSASocketA(af=0x%x, type=0x%x, protocol=0x%x, "
           "lpProtocolInfo=%p, g=0x%x, dwFlags=0x%x)\n",
	   af, type, protocol, lpProtocolInfo, g, dwFlags);

    PALASSERT(!lpProtocolInfo);
    PALASSERT(0 == g);
    PALASSERT(WSA_FLAG_OVERLAPPED == dwFlags);

    Ret = WSASocketA(af, type, protocol, (LPWSAPROTOCOL_INFOA)lpProtocolInfo, 
                     g, dwFlags);

    LOGAPI("WSASocketA returns SOCKET %p\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_WSAIoctl(
	     IN SOCKET s,
	     IN DWORD dwIoControlCode,
	     IN LPVOID lpvInBuffer,
	     IN DWORD cbInBuffer,
	     OUT LPVOID lpvOutBuffer,
	     IN DWORD cbOutBuffer,
	     OUT LPDWORD lpcbBytesReturned,
	     IN PAL_LPWSAOVERLAPPED lpOverlapped,
	     IN PAL_LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int Ret;

    LOGAPI("WSAIoctl(s=%p, dwIoControlCode=0x%x, lpvInBuffer=%p, "
           "cbInBuffer=0x%x, lpvOutBuffer=%p, cbOutBuffer=0x%x, "
           "lpcbBytesReturned=%p, lpOverlapped=%p, lpCompletionRoutine=%p)\n",
	   s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer,
	   cbOutBuffer, lpcbBytesReturned, lpOverlapped,
	   lpCompletionRoutine);

    PALASSERT(lpOverlapped);
    PALASSERT(!lpCompletionRoutine);

    Ret = WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer,
		   cbOutBuffer, lpcbBytesReturned, 
		   (LPWSAOVERLAPPED) lpOverlapped,
		   (LPWSAOVERLAPPED_COMPLETION_ROUTINE)lpCompletionRoutine);

    LOGAPI("WSAIoctl returns int 0x%x\n", Ret);
    return Ret;
}

PALIMPORT
int
PALAPI
PAL_ioctlsocket(
		IN SOCKET s,
		IN int cmd,  // note: diff from Win32
		IN OUT u_long FAR *argp)
{
    int Ret;

    LOGAPI("ioctlsocket(s=%p, cmd=0x%x, argp=%p)\n",s, cmd, argp);

    PALASSERT(FIONREAD == cmd || FIONBIO == cmd);

    Ret = ioctlsocket(s, cmd, argp);

    LOGAPI("ioctlsocket returns int 0x%x\n", Ret);
    return Ret;
}


void
PalAssertFailed(
    const char *FileName, 
    int LineNumber,
    const char *Expr)
{
    char buffer[1024];
    DWORD LastErrValue;
    DWORD NumberOfBytesWritten;

    LastErrValue = GetLastError();

    _snprintf(buffer, sizeof(buffer), 
              "PAL Assertion failed - %s - line %d in file %s\n",
              Expr, LineNumber, FileName);
    //
    // null-terminate the buffer in case the _snprintf call filled it - 
    // on buffer overflow, _snprintf() doesn't write in a null-terminator.
    // 
    buffer[sizeof(buffer)-1] = '\0';

    OutputDebugStringA(buffer);
    if (LogFileHandle != INVALID_HANDLE_VALUE && LogFileHandle != 0) {
        //
        // Logging is enabled, but not to the debugger, so log the
        // assertion text to the file too.
        //
        WriteFile(LogFileHandle,
                  buffer,
                  strlen(buffer),
                  &NumberOfBytesWritten,
                  NULL);
    }
    DebugBreak();
    // continue execution after an assert fires

    SetLastError(LastErrValue);
}

// The actual logging code is separated in PalLogApiCore to make no-op logging fast
// (filling 2k buffer with 0xCC for stack checking takes time even in the debug build...)

void
__cdecl
PalLogApiCore(
          const char *fmt,
          va_list list)
{
    char buffer[2048];
    int StringLength;
    int IDLength;
    DWORD LastErrValue;
    int errnoValue;
    DWORD NumberOfBytesWritten;

    LastErrValue = GetLastError();
    errnoValue = errno;

    __try {
        IDLength = sprintf(buffer, 
                           "%x %x ",
                           GetCurrentProcessId(),
                           GetCurrentThreadId());
        StringLength = _vsnprintf(buffer+IDLength, 
                                  sizeof(buffer)-IDLength, 
                                  fmt, 
                                  list);

        if (StringLength == -1) {
            //
            // The buffer filled before the end of the format string.
            // include a linefeed and null-terminator.
            //
            buffer[sizeof(buffer)-2] = '\n';
            buffer[sizeof(buffer)-1] = '\0';
            StringLength = sizeof(buffer);
        } else {
            StringLength += IDLength;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        strcpy(buffer, "*** Exception thrown in PalLogApi() - an API call "
               "was not logged\n");
        StringLength = strlen(buffer);
    }

    // Write the buffer out to the log file, ignoring any errors
    if (LogFileHandle == 0) {
        OutputDebugStringA(buffer);
    } else {
        WriteFile(LogFileHandle,
                  buffer,
                  StringLength,
                  &NumberOfBytesWritten,
                  NULL);
    }

    SetLastError(LastErrValue);
    errno = errnoValue;
}

void
__cdecl
PalLogApi(
          const char *fmt,
          ...)
{
    va_list list;

    // Assert that the PAL APIs are only being called when the PAL
    // is properly initialized.
    PALASSERT(PalReferenceCount != 0);

    if (LogFileHandle == INVALID_HANDLE_VALUE) {
        // Logging isn't enabled
        return;
    }

    va_start(list, fmt);
    PalLogApiCore(fmt, list);
    va_end(list);
}

#endif //DBG



