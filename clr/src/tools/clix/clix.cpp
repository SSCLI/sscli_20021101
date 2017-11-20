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
// File: clix.cpp
// 
// Managed application launcher
// ===========================================================================

#include "windows.h"
#include "stdlib.h"

#include "cor.h"
#include "corhdr.h"
#include "corimage.h"

#include "clix.h"
#include "palclr.h"
#include <palstartup.h>

static const WCHAR *c_wzSatellite = L"clix.satellite";

static const WCHAR *c_wzRuntimeName = MSCOREE_SHIM_W;

HSATELLITE LoadSatellite(void)
{
    WCHAR Path[_MAX_PATH];

    if (!PAL_GetPALDirectoryW(Path, _MAX_PATH)) {
	return NULL;
    }
    if (wcslen(Path) + sizeof(c_wzSatellite)/sizeof(WCHAR) >= _MAX_PATH) {
	return NULL;
    }
    wcscat(Path, c_wzSatellite);
    return PAL_LoadSatelliteResourceW(Path);
}

void DisplayMessage(short MsgID, DWORD LastErr, const WCHAR *pText)
{
    HSATELLITE hSat = NULL;
    UINT u;
    DWORD dw;
    INT_PTR MessageArgs[3];
    WCHAR SatelliteBuffer[256];
    WCHAR ErrorBuffer[256];
    WCHAR MessageBuffer[512];

    hSat = LoadSatellite();
    if (!hSat) {
	goto LError;
    }

    u = PAL_LoadSatelliteStringW(hSat, 
				 MsgID, 
				 SatelliteBuffer, 
				 sizeof(SatelliteBuffer)/sizeof(WCHAR));
    if (u == sizeof(SatelliteBuffer)/sizeof(WCHAR) || u == 0) {
	goto LError;
    }

    dw = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
		        NULL,
		        LastErr,
		        0,
		        ErrorBuffer,
		        sizeof(ErrorBuffer)/sizeof(WCHAR),
		        NULL);
    if (dw == 0) {
	goto LError;
    }

    MessageArgs[0] = (INT_PTR)pText;
    MessageArgs[1] = (INT_PTR)ErrorBuffer;

    dw = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
			SatelliteBuffer,
			0,
			0,
			MessageBuffer,
			sizeof(MessageBuffer)/sizeof(WCHAR),
			(va_list *)MessageArgs);
    if (dw == 0) {
	goto LError;
    }

    fwprintf(stderr, L"%s", MessageBuffer);
    goto LExit;
    
LError:
    fwprintf(stderr, L"ERROR:  MessageID=%d, LastError=%d, String=%s\n", 
	     MsgID, LastErr, pText);
LExit:
    if (hSat) {
	PAL_FreeSatelliteResource(hSat);
    }
}


void DisplayMessage(short MsgID)
{
    HSATELLITE hSat = NULL;
    UINT u;
    WCHAR SatelliteBuffer[256];

    hSat = LoadSatellite();
    if (!hSat) {
	goto LError;
    }

    u = PAL_LoadSatelliteStringW(hSat, 
				 MsgID, 
				 SatelliteBuffer, 
				 sizeof(SatelliteBuffer)/sizeof(WCHAR));
    if (u == sizeof(SatelliteBuffer)/sizeof(WCHAR) || u == 0) {
	goto LError;
    }

    fwprintf(stderr, SatelliteBuffer);
    goto LExit;
    
LError:
    fwprintf(stderr, L"ERROR:  MessageID=%d\n", MsgID);
LExit:
    if (hSat) {
	PAL_FreeSatelliteResource(hSat);
    }
}



DWORD Launch(WCHAR* pRunTime, WCHAR* pFileName, WCHAR* pCmdLine)
{
    HANDLE hFile = NULL;
    HANDLE hMapFile = NULL;
    PVOID pModule = NULL;
    HINSTANCE hRuntime = NULL;
    DWORD nExitCode = 1;
    DWORD dwSize;
    DWORD dwSizeHigh;
    IMAGE_DOS_HEADER* pdosHeader;
    IMAGE_NT_HEADERS32* pNtHeaders;
    IMAGE_SECTION_HEADER*   pSectionHeader;
    WCHAR exeFileName[MAX_PATH + 1];

    // open the file & map it
    hFile = ::CreateFile(pFileName, GENERIC_READ, FILE_SHARE_READ,
                         0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // If the file doesn't exist, append a '.exe' extension and
        // try again.
        nExitCode = ::GetLastError();
        if (nExitCode == ERROR_FILE_NOT_FOUND)
        {
            const WCHAR *exeExtension = L".exe";
            if (wcslen(pFileName) + wcslen(exeExtension) <
                    sizeof(exeFileName) / sizeof(WCHAR))
            {
                wcscpy(exeFileName, pFileName);
                wcscat(exeFileName, exeExtension);
                hFile = ::CreateFile(exeFileName, GENERIC_READ, FILE_SHARE_READ,
                                     0, OPEN_EXISTING, 0, 0);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    pFileName = exeFileName;
                }
            }
        }
        if (hFile == INVALID_HANDLE_VALUE)
        {
            DisplayMessage(MSG_CantOpenExe, nExitCode, pFileName);
            goto Error;
        }
    }

    hMapFile = ::CreateFileMapping(hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    if (hMapFile == NULL)
    {
        nExitCode = ::GetLastError();
        DisplayMessage(MSG_OutOfMemory);
        goto Error;
    }

    pModule = ::MapViewOfFile(hMapFile, FILE_MAP_COPY, 0, 0, 0);
    if (pModule == NULL)
    {
        nExitCode = ::GetLastError();
        DisplayMessage(MSG_OutOfMemory);
        goto Error;
    }

    dwSize = GetFileSize(hFile, &dwSizeHigh);
    if (dwSize == INVALID_FILE_SIZE)
    {
        nExitCode = ::GetLastError();
        if (nExitCode == 0)
            nExitCode = ERROR_BAD_FORMAT;            
        DisplayMessage(MSG_BadFileSize);
        goto Error;
    }

    if (dwSizeHigh != 0)
    {
        nExitCode = ERROR_BAD_FORMAT;
        DisplayMessage(MSG_BadFileSize);
        goto Error;
    }

    // check the DOS headers
    pdosHeader = (IMAGE_DOS_HEADER*) pModule;
    if (pdosHeader->e_magic != VAL16(IMAGE_DOS_SIGNATURE) || 
        VAL32(pdosHeader->e_lfanew) <= 0 || dwSize <= VAL32(pdosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS32))
    {
        nExitCode = ERROR_BAD_FORMAT;
        DisplayMessage(MSG_BadFormat);
        goto Error;
    }

    // check the NT headers
    pNtHeaders = (IMAGE_NT_HEADERS32*) ((BYTE*)pModule + VAL32(pdosHeader->e_lfanew));
    if ((pNtHeaders->Signature != VAL32(IMAGE_NT_SIGNATURE)) ||
        (pNtHeaders->FileHeader.SizeOfOptionalHeader != VAL16(IMAGE_SIZEOF_NT_OPTIONAL32_HEADER)) ||
        (pNtHeaders->OptionalHeader.Magic != VAL16(IMAGE_NT_OPTIONAL_HDR32_MAGIC)))
    {
        nExitCode = ERROR_INVALID_EXE_SIGNATURE;
        DisplayMessage(MSG_InvalidExeSignature);
        goto Error;
    }

    // check the COR headers
    pSectionHeader = (PIMAGE_SECTION_HEADER) Cor_RtlImageRvaToVa(pNtHeaders, (PBYTE)pModule, 
        VAL32(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress),
        dwSize);
    if (pSectionHeader == NULL)
    {
        DisplayMessage(MSG_NotIL);
        nExitCode = ERROR_EXE_MARKED_INVALID;
        goto Error;
    }

    // load the runtime and go
    hRuntime = ::LoadLibrary(pRunTime);
    if (hRuntime == NULL)
    {
        nExitCode = ::GetLastError();
        DisplayMessage(MSG_CantLoadEE, nExitCode, pRunTime);
        goto Error;
    }        

    __int32 (STDMETHODCALLTYPE * pCorExeMain2)(
            PBYTE   pUnmappedPE,                // -> memory mapped code
            DWORD   cUnmappedPE,                // Size of memory mapped code
            LPWSTR  pImageNameIn,               // -> Executable Name
            LPWSTR  pLoadersFileName,           // -> Loaders Name
            LPWSTR  pCmdLine);                  // -> Command Line

    *((VOID**)&pCorExeMain2) = (LPVOID) ::GetProcAddress(hRuntime, "_CorExeMain2");
    if( pCorExeMain2 == NULL)
    {
        nExitCode = ::GetLastError();
        DisplayMessage(MSG_CantFindExeMain, nExitCode, pRunTime);
        goto Error;
    }        

    nExitCode = (int)pCorExeMain2((PBYTE)pModule, dwSize, 
                             pFileName,                  // -> Executable Name
                             NULL,                       // -> Loaders Name
                             pCmdLine);                  // -> Command Line

Error:
    ::UnmapViewOfFile(pModule);
    ::CloseHandle(hMapFile);
    ::CloseHandle(hFile);

    return nExitCode;
}

int __cdecl main(int argc, char **argv)
{
    DWORD nExitCode = 1; // error
    WCHAR* pwzCmdLine;

    PAL_RegisterLibrary(L"rotor_palrt");

#if FV_DEBUG_CORDBDB
    // This gives us the opertunity to attach a debugger, modify
    // DebuggerAttached and continue
    static bool DebuggerAttached = false;    
    while (DebuggerAttached == false)
    {
        Sleep(1);
    }
#endif
    pwzCmdLine = ::GetCommandLineW();

    size_t nRuntimeNameLength = wcslen(c_wzRuntimeName);

    // Allocate for the worst case storage requirement.
    WCHAR *pAlloc = (WCHAR*)malloc((wcslen(pwzCmdLine) + nRuntimeNameLength + 1) * sizeof(WCHAR));
    if (!pAlloc) {
	DisplayMessage(MSG_OutOfMemory);
        return 1;
    }

    WCHAR* psrc = pwzCmdLine;
    WCHAR* pdst = pAlloc;
    BOOL inquote = FALSE;

    WCHAR* pRuntimeName;
    WCHAR* pModuleName;
    WCHAR* pActualCmdLine;

    // First, parse the program name. Anything up to the first whitespace outside 
    // a quoted substring is accepted (algorithm from clr/src/vm/util.cpp)
    pRuntimeName = pdst;

    for (;;)
    {
        switch (*psrc)
        {
        case L'\"':
            inquote = !inquote;
            psrc++;
            continue;

        case '\0':
            DisplayMessage(MSG_Usage);
            goto UnexpectedCommandLine;

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

    // Now, load the runtime from the clix directory

    // Get the directory - eat the clix filename
    pdst--;
    while (pdst >= pRuntimeName)
    {
        if ( *pdst == '/' || *pdst == '\\')
            break;

        pdst--;
    }
    pdst++;

    // Append runtime library name & zero terminate
    memcpy(pdst, c_wzRuntimeName, (nRuntimeNameLength + 1) * sizeof(WCHAR));
    pdst += nRuntimeNameLength + 1;

    // Skip the whitespace
    while (*psrc == ' ' || *psrc == '\t')
    {
        psrc++;
    }

    pActualCmdLine = psrc;

    // Parse the first arg - the name of the module to run
    pModuleName = pdst;

    for (;;)
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

    nExitCode = Launch(pRuntimeName, pModuleName, pActualCmdLine);

UnexpectedCommandLine:
    free(pAlloc);

    return nExitCode;
}
