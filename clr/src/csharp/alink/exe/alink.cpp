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
// File: alink.cpp
// 
// The command line driver for the assembly linker.
// ===========================================================================

#include "stdafx.h"

#include "msgsids.h"
#include "alink.h"
#include "common.h"
#include "merge.h"
#include "alinkexe.h"
#include "sscli_version.h"
#include <palstartupw.h>

// Options
LPWSTR          g_szAssemblyFile = NULL;
LPWSTR          g_szAppMain = NULL;
LPWSTR          g_szCopyAssembly = NULL;
VARIANT         g_Options[optLastAssemOption];
FileList       *g_pFileListHead = NULL;
FileList      **g_ppFileListEnd = &g_pFileListHead;
ResList        *g_pResListHead = NULL;
ResList       **g_ppResListEnd = &g_pResListHead;
DWORD           g_dwDllBase = 0;
bool            g_bNoLogo = false;
bool            g_bShowHelp = false;
bool            g_bTime = false;
bool            g_bFullPaths = false;
bool            g_bHadError = false;
bool            g_bMerge = false;
bool            g_bClean = false;
bool            g_bMake = false;
bool            g_bNoRefHash = false;
TargetType      g_Target = ttDll;
bool            g_fullPaths = false;
bool            g_bUnicode = false;
HINSTANCE       g_hinstMessages = NULL;
HMODULE         g_hmodCorPE = NULL;
_optionused    *_optionused::m_pOptions;

// Runtime Version stuff?
LPWSTR          g_szCorVersion = NULL;
HRESULT (__stdcall *g_pfnCreateCeeFileGen)(ICeeFileGen **ceeFileGen) = NULL; // call this to instantiate
HRESULT (__stdcall *g_pfnDestroyCeeFileGen)(ICeeFileGen **ceeFileGen) = NULL; // call this to delete


/*
 * Convert Unicode string to Console ANSI string allocated with VSAlloc
 */
LPSTR WideToConsole(LPCWSTR wideStr)
{
    int cch = WideCharToMultiByte(GetConsoleOutputCP(), 0, wideStr, -1, NULL, 0, 0, 0);
    LPSTR ansiStr = (LPSTR) VSAlloc(cch);
    WideCharToMultiByte(GetConsoleOutputCP(), 0, wideStr, -1, ansiStr, cch, 0, 0);
    return ansiStr;
}

/*
 * Wrapper for printf that handles the repro file also.
 */
int __cdecl print(const char * format, ...)
{
    va_list argptr;
    int i;

    va_start(argptr, format);

    i = vprintf(format, argptr);
        
    va_end(argptr);
    return i;
}

/*
 * Wrapper for LoadString that converts to the console code page.
 */
int LoadConsoleString(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int nBufferMax)
{
	int cch = 0;
    cch = LoadStringA(hInstance, uID, lpBuffer, nBufferMax);

    if (cch > 0 && GetConsoleOutputCP() != GetACP()) {
        // Translate from ACP to ConsoleOutput code page, via unicode.
        LPWSTR lpWideBuffer = (WCHAR *) _alloca((cch + 1) * sizeof(WCHAR));
        if (0 == LoadStringW(hInstance, uID, lpWideBuffer, cch + 1))
            return cch;
        return WideCharToMultiByte(GetConsoleOutputCP(), 0, lpWideBuffer, -1, lpBuffer, nBufferMax, 0, 0) - 1;
    }
    else
        return cch;
}

#if !defined(__GNUC__)
LPCSTR ErrorOptionName(AssemblyOptions opt) {
    static CHAR buffer[1024];
    if (LoadConsoleString(g_hinstMessages, Options[opt].HelpId, buffer, sizeof(buffer)))
        return buffer;
    else
        return NULL;
}
#endif  // !defined(__GNUC__)


/*
 * Read a paragraph of text from stdin. Blank line terminates.
 * Must be freed with VSFree.
 */
#define MAX_PARA_SIZE 32768
char * ReadParagraph()
{
    char * text = (char *) VSAlloc(MAX_PARA_SIZE);
    char * current = text;


    for(;;) {
        if (fgets(current, MAX_PARA_SIZE - (int)(current - text) - 2, stdin) == NULL)
            break;
        if (*current == '\0')
            break;  // empty string finishes.
        if (*current == '\n') {
            *current++ = '\r';
            *current++ = '\n';
            *current = '\0';
            break;  //empty string finishes.
        }

        current = current + strlen(current);  // advance beyond the line just read.

        *(current - 1) = '\r';
        *current++ = '\n';     // change \n to \r\n
    }

    return text;
}


/*
 * Load a string from a resource and print it, followed by an optional newline.
 */
void PrintString(unsigned id, bool newline = true)
{
    char buffer[512];
    int i;

    i = LoadConsoleString(g_hinstMessages, id, buffer, sizeof(buffer));

    if (i) {
    print("%s", buffer);
    if (newline)
        print("\n");
    }
}

/*
 * General error display routine -- report error or warning.
 * kind indicates fatal, warning, error
 * inputfile can be null if none, and line <= 0 if no location is available.
 */
void ShowError(int id, ERRORKIND kind, PCWSTR inputfile, int line, int col, int extent, LPSTR text)
{
    const CHAR * errkind;
    switch (kind) {
    case ERROR_FATAL:
        g_bHadError = true;
        errkind = "fatal error"; break;
    case ERROR_ERROR:
        g_bHadError = true;
        errkind = "error"; break;
    case ERROR_WARNING:
        errkind = "warning"; break;
    default:
        ASSERT(0);
        return;
    }

    // Print file and location, if available.
    // Output must be in ANSI, so convert.
    if (inputfile) {
        CHAR buffer[MAX_PATH];
        if (WideCharToMultiByte(GetConsoleOutputCP(), 0, inputfile, -1, buffer, sizeof(buffer), NULL, NULL)) {

                print("%s", buffer);
        }



        if (line > 0) {
            print("(%d,%d)", line, col);
        }

        print(": ");
    } else {
        print("ALINK: ");
    }

    // Print "error ####" -- only if not a related symbol location (id == -1)
    if (id != -1)
    {
        print("%s AL%04d: ", errkind, id);
    }

    // Print error text.
    if (text && text[0]) {
        print("%s", text);
    }

    print("\n");
}

/*
 * Simple error display routine, takes an error number, and error text, no location.
 */
void ShowFormattedError(int id, LPCWSTR text)
{
    int i;
    CHAR buffer[2000];

    i = WideCharToMultiByte(GetConsoleOutputCP(), 0, text, -1, buffer, 2000, NULL, NULL);
    ASSERT(i);  // String should be there.
    if (i) {
        ERRORKIND kind = ERROR_ERROR;
        if (errorInfo[id].level < 0) {
            ASSERT(errorInfo[id].level == -1);
            kind = ERROR_FATAL;
        } else if (errorInfo[id].level > 0) {
            ASSERT(errorInfo[id].level <= 4);
            kind = ERROR_WARNING;
        } else {
            ASSERT(errorInfo[id].level == 0);
        }

        // Call onto general method to show the error.
        ShowError(errorInfo[id].number, kind, NULL, -1, -1, -1, buffer);
    }
}

/*
 * Simple error display routine, takes resource id (textid), no location.
 */
void ShowErrorId(int id, ERRORKIND kind)
{
    int i;
    CHAR buffer[2000];

    i = LoadConsoleString(g_hinstMessages, errorInfo[id].resid, buffer, sizeof(buffer));
    ASSERT(i);  // String should be there.
    if (i) {
        // Call onto general method to show the error.
        ShowError(errorInfo[id].number, kind, NULL, -1, -1, -1, buffer);
    }
}

/*
 * Simple error display routine, takes resource id (textid), fill-in arguments, no location.
 */
void _cdecl ShowErrorIdString(int id, ERRORKIND kind, ...)
{
    int i;
    CHAR buffer1[1000];
    CHAR buffer2[2000];

    i = LoadConsoleString(g_hinstMessages, errorInfo[id].resid, buffer1, sizeof(buffer1));
    ASSERT(i);  // String should be there.
    if (i) {
        va_list args;
        va_start (args, kind);

        // Insert fill-ins.
        if (FormatMessageA(FORMAT_MESSAGE_FROM_STRING, buffer1, 0, 0,
                           buffer2, sizeof(buffer2), &args) == 0)
            return; // Failure -- nothing we can do.
        va_end (args);

        ShowError(errorInfo[id].number, kind, NULL, -1, -1, -1, buffer2);
    }
}

/*
 * Simple error display routine, takes resource id (textid), location, fill-in arguments.
 */
void _cdecl ShowErrorIdLocation(int id, ERRORKIND kind, LPCWSTR szLocation, ...)
{
    int i;
    CHAR buffer1[1000];
    CHAR buffer2[2000];

    i = LoadConsoleString(g_hinstMessages, errorInfo[id].resid, buffer1, sizeof(buffer1));
    ASSERT(i);  // String should be there.
    if (i) {
        va_list args;
        va_start (args, szLocation);

        // Insert fill-ins.
        if (FormatMessageA(FORMAT_MESSAGE_FROM_STRING, buffer1, 0, 0,
                           buffer2, sizeof(buffer2), &args) == 0)
            return; // Failure -- nothing we can do.
        va_end (args);
        
        ShowError(errorInfo[id].number, kind, szLocation, -1, -1, -1, buffer2);
    }
}

/*
 * Get a string (LPCWSTR) for an HRESULT
 */
LPCWSTR ErrorHR(HRESULT hr)
{
    static WCHAR buffer[2048];
    UINT r = 0;
    bool repeat;

    do {
        CComPtr<IErrorInfo> err;
        CComBSTR str;

        if ((HRESULT_FACILITY(hr) == FACILITY_COMPLUS || HRESULT_FACILITY(hr) == FACILITY_URT || HRESULT_FACILITY(hr) == FACILITY_ITF) &&
            GetErrorInfo( 0, &err) == S_OK && SUCCEEDED(err->GetDescription(&str))) {
            wcscpy(buffer, str);
            r = str.Length();
        } else {
            // Use FormatMessage to get the error string for the HRESULT from the system.

            r = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, hr, 0,
                               buffer, lengthof(buffer),
                               NULL);

        }

        // Check for errors, and possibly repeat.
        repeat = false;
        if (r == 0) {
            if (hr != E_FAIL) {
                // Use E_FAIL as generic error code if looking up the real code didn't work, and repeat.
                hr = E_FAIL;
                repeat = true;
            }
            else {
                // Something really extreme. I don't understand why this would happen.
                ASSERT(0);      // investigate please.
                return L"Unknown fatal error";
            }
        }
    } while (repeat);

    return buffer;
}

/*
 * Get a string for GetLastError()
 */
LPCWSTR ErrorLastError()
{
    static WCHAR buffer[2048];
    int r;
    DWORD result = GetLastError();

    // Use FormatMessage to get the error string for the HRESULT from the system.

    r = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, result, 0,
                       buffer, lengthof(buffer),
                       NULL);


    // Check for errors
    if (r == 0) {
        // Something really extreme. I don't understand why this would happen.
        ASSERT(0);      // investigate please.
        return L"Unknown fatal error";
    }

    return buffer;
}

/*
 * Print the help text for the compiler.
 */
HRESULT PrintHelp ()
{
    // The help text is numbered 10, 20, 30... so there is
    // ample room to put in additional strings in the middle.

    PrintString(IDS_HELP10, true);      // "Usage:"
    PrintString(IDS_HELP20, true);      // "Options:"

    print ("\n");
    // Load and print the options
    for (int i=IDS_HELP20 + 1; i <= IDS_H_LASTOPTION; i++)
    {
        PrintString(i, true);
    }

    print ("\n");

    // Load and print the Source statements
    PrintString(IDS_HELP30, true);
    PrintString(IDS_H_SOURCEFILE, true);
    PrintString(IDS_H_EMBED1, true);
    PrintString(IDS_H_EMBED2, true);
    PrintString(IDS_H_LINK1, true);
    PrintString(IDS_H_LINK2, true);
    print ("\n");

    return S_OK;
}


/*
 * Create a simple leaf tree node with the given text
 */
b_tree *MakeLeaf(LPCWSTR text)
{
    b_tree *t = (b_tree*)VSAllocZero(sizeof(b_tree));
    t->name = (LPWSTR)VSAlloc((wcslen(text)+1) * sizeof(WCHAR));
    wcscpy(t->name, text);
    /* You don't need to do this because of VSAllocZero
     * t->lChild = NULL;
     * t->rChild = NULL;
     */
    return t;
}

/*
 * Free the memory allocated by the tree (recursive)
 */
void CleanupTree(b_tree *root)
{
    if (root == NULL)
        return;
    CleanupTree(root->lChild);
    CleanupTree(root->rChild);
    if (root->name) VSFree(root->name);
    VSFree(root);
}

/*
 * Search the binary tree and add the given string
 * return true if it was added or false if it already
 * exists
 */
bool TreeAdd(b_tree **root, LPCWSTR add)
{
    if (*root == NULL)
        return ((*root = MakeLeaf(add)) != NULL);

    int cmp = _wcsicmp((*root)->name, add);
    if (cmp < 0)
        return TreeAdd(&(*root)->lChild, add);
    else if (cmp > 0)
        return TreeAdd(&(*root)->rChild, add);
    else
        return false;
}

/*
 * Opens a text file as either ASCII, UTF8, or Unicode
 * and converts the text to unicode
 * returns S_OK for success
 * returns S_FALSE for failure if it already reported the error
 * otherwise returns HRESULT_FROM_WIN32(GetLastError())
 */
HRESULT OpenTextFile(LPCWSTR pszFilename, LPWSTR *ppszText)
{
    // Open the file
    HRESULT     hr;
    HANDLE      hFile = W_CreateFile (pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    enum        eTextType { txUnicode, txSwappedUnicode, txUTF8, txASCII } textType;

    if (hFile == INVALID_HANDLE_VALUE || hFile == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD   dwSizeHigh, dwSize, dwRead;
    long    i;

    // Find out how big it is
    dwSize = GetFileSize (hFile, &dwSizeHigh);
    if (dwSizeHigh != 0)
    {
        // Whoops!  Too big.
        ShowErrorIdString(FTL_FileTooBig, ERROR_FATAL, pszFilename);
        CloseHandle (hFile);
        return S_FALSE;
    }

    // Allocate space and read it in.
    PSTR    pszTemp = (PSTR)VSAlloc ((dwSize + 1) * sizeof (CHAR));

    if (pszTemp == NULL)
    {
        CloseHandle (hFile);
        return E_OUTOFMEMORY;
    }

    if (!ReadFile (hFile, pszTemp, dwSize, &dwRead, NULL) || (dwRead != dwSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CloseHandle (hFile);
        VSFree (pszTemp);
        return hr;
    }

    CloseHandle (hFile);

    // Determine Character encoding of file
    switch( pszTemp[0] ) {
    case '\xFE':
    case '\xFF':
        textType = ( pszTemp[0] == '\xFF' ? txUnicode : txSwappedUnicode );
        if ( dwSize < 2 || dwSize & 1 || pszTemp[1] != ( pszTemp[0] == '\xFE' ? '\xFF' : '\xFE') )
            textType = txASCII;     // Not a Unicode File
        break;
    case '\xEF':
        textType = txUTF8;
        if ( dwSize < 3 || pszTemp[1] != '\xBB' || pszTemp[2] != '\xBF')
            textType = txASCII;     // Not a UTF-8 File
        break;
    case 'M':
        if (dwSize > 1 && pszTemp[1] == 'Z') {
            // 'M' 'Z' indicates a DLL or EXE; give good error and fail.
            ShowErrorIdString(ERR_CantOpenBinaryAsText, ERROR_ERROR, pszFilename);
            VSFree(pszTemp);
            return S_FALSE;
        }
        else
            textType = txASCII;

        break;
        
    default:
        textType = txASCII;
        break;
    }

    switch( textType ) {
    case txUnicode:
        // No conversion needed
        *ppszText = (PWSTR)VSAlloc (dwSize * sizeof (CHAR));
        memcpy( *ppszText, pszTemp + 2, dwSize - 2);
        (*ppszText)[ (dwSize - 2) / 2 ] = 0;
        VSFree (pszTemp);
        break;
    case txSwappedUnicode:
        // swap the bytes please
        *ppszText = (PWSTR)VSAlloc (dwSize * sizeof (CHAR));
        if (*ppszText == NULL)
        {
            VSFree (pszTemp);
            return E_OUTOFMEMORY;
        }
        // We know this file has an even number of bytes because of the earlier tests
        ASSERT( (dwSize & 1) == 0);
        _swab( pszTemp + 2, (PSTR)*ppszText, dwSize - 2);   // Swap all possible bytes
        (*ppszText)[ (dwSize - 2) / 2 ] = 0;
        VSFree (pszTemp);
        break;
    case txUTF8:
        SetLastError(0);
        // Calculate the size needed
        i = UnicodeLengthOfUTF8 (pszTemp + 3, dwSize - 3);
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        *ppszText = (PWSTR)VSAlloc ((i + 1) * sizeof (WCHAR));
        if (*ppszText == NULL)
        {
            VSFree (pszTemp);
            return E_OUTOFMEMORY;
        }

        // Convert, remembering number of characters
        i = UTF8ToUnicode(pszTemp + 3, dwSize - 3, *ppszText, (i + 1));
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        (*ppszText)[i] = 0;

        VSFree (pszTemp);
        break;
    case txASCII:
        if (dwSize == 0) { // Special case for completely empty files
            *ppszText = (PWSTR)VSAlloc ( sizeof(WCHAR));
            VSFree (pszTemp);
            (*ppszText)[0] = 0;
            break;
        }

        // Calculate the size needed
        i = MultiByteToWideChar (CP_ACP, 0, pszTemp, dwSize, NULL, 0);
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        *ppszText = (PWSTR)VSAlloc ((dwSize + 1) * sizeof (WCHAR));
        if (*ppszText == NULL)
        {
            VSFree (pszTemp);
            return E_OUTOFMEMORY;
        }

        // Convert, remembering number of characters
        i = MultiByteToWideChar (CP_ACP, 0, pszTemp, dwSize, *ppszText, dwSize + 1);
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        (*ppszText)[i] = 0;

        VSFree (pszTemp);
        break;
    }

    return S_OK;
}

/*
 * Process Response files on the command line
 * Returns true if it allocated a new argv array that must be freed later
 */
bool ProcessResponseArgs(int & argc, LPWSTR *argv[], WStrList **vals)
{
    HRESULT hr;
    b_tree *response_files = NULL;

    WStrList **argList, **argLast = vals;
    LPCWSTR pszText, pCur;
    WCHAR   pszFilename[MAX_PATH];
    int iCount, i;
    bool bAlloced = false;

    for (int iArg = 0; iArg < argc; ++iArg)
    {
        // Skip everything except Response files
        if ((*argv)[iArg] == NULL || (*argv)[iArg][0] != '@')
            continue;

        argList = argLast;

        if (wcslen((*argv)[iArg]) == 1) {
            ShowErrorIdString( ERR_NoFileSpec, ERROR_FATAL, (*argv)[iArg]);
            (*argv)[iArg] = NULL;
            continue;
        }

        // Check for duplicates
        if (!GetFullFileName( &(*argv)[iArg][1], pszFilename, MAX_PATH))
            continue;

        if (!TreeAdd(&response_files, pszFilename)) {
            ShowErrorIdString(ERR_DuplicateResponseFile, ERROR_ERROR, pszFilename);
            (*argv)[iArg] = NULL;
            continue;
        }

        pszText = NULL;
        if (FAILED(hr = OpenTextFile(pszFilename, (LPWSTR*)&pszText)) || hr == S_FALSE)
        {
            if (pszText) VSFree((LPWSTR)pszText);
            if (hr == E_OUTOFMEMORY) {
                ShowErrorId(FTL_NoMemory, ERROR_FATAL);
            } else if (FAILED(hr)) {
                ShowErrorIdString(ERR_OpenResponseFile, ERROR_FATAL, pszFilename, ErrorHR(hr));
            }
            continue;
        }

        pCur = pszText;
        iCount = 0;
        LPWSTR pszTemp = (LPWSTR)VSAlloc(sizeof(WCHAR) * (wcslen(pszText) + 1));
        while (*pCur != '\0')
        {
            WCHAR *pPut;
            bool bQuoted;
            int iSlash;

LEADINGWHITE:
            while (IsWhitespace(*pCur) && *pCur != '\0')
                pCur++;

            if (*pCur == '\0')
                break;
            else if (*pCur == L'#')
            {
                while ( *pCur != '\0' && *pCur != '\n')
                    pCur++; // Skip to end of line
                goto LEADINGWHITE;
            }

            bQuoted = false;
            pPut = pszTemp;
            while ((!IsWhitespace( *pCur) || bQuoted) && *pCur != '\0')
            {
                switch (*pCur)
                {
                    // All this wierd slash stuff follows the standard argument processing routines
                case L'\\':
                    iSlash = 0;
                    while (*pCur == L'\\')
                        iSlash++, pCur++;

                    if (*pCur == L'\"') {
                        for (; iSlash >= 2; iSlash -= 2)
                            *pPut++ = L'\\';

                        if (iSlash & 1) { // Is it odd?
                            *pPut++ = *pCur++;
                        } else {
                            bQuoted = !bQuoted;
                            pCur++;
                        }
                    } else {
                        for (; iSlash > 0; iSlash--)
                            *pPut++ = L'\\';
                    }
                    break;
                case L'\"':
                    bQuoted = !bQuoted;
                    pCur++;
                    break;
                default:
                    *pPut++ = *pCur++;  // Copy the char and advance
                }
            }

            *pPut++ = '\0';

            *argLast = (WStrList*)VSAllocZero( sizeof(WStrList));
            if (!*argLast) {
                ShowErrorId(FTL_NoMemory, ERROR_FATAL);
                break;
            }

            (*argLast)->arg = (WCHAR*)VSAlloc( sizeof(WCHAR) * (pPut - pszTemp));
            if (!(*argLast)->arg) {
                ShowErrorId(FTL_NoMemory, ERROR_FATAL);
                break;
            }

            wcscpy((*argLast)->arg, pszTemp);
            iCount++;
            argLast = &(*argLast)->next; // Next is NULLed by the AllocZero
        }

        VSFree((LPWSTR)pszText);
        VSFree((LPWSTR)pszTemp);

        LPWSTR * newArgs;
        newArgs = (LPWSTR*)VSAlloc( sizeof(LPWSTR*) * (argc + iCount));
        memcpy(newArgs, *argv, iArg * sizeof(LPWSTR*));
        for (i = iArg++; *argList; argList = &(*argList)->next)
            newArgs[i++] = (*argList)->arg;
        memcpy( &newArgs[i], &(*argv)[iArg], (argc - iArg) * sizeof(LPWSTR));
        argc += iCount - 1;
        if (bAlloced)
            VSFree( *argv);
        *argv = newArgs;
        bAlloced = true;
        iArg -= 2;
    }

    CleanupTree(response_files);

    return bAlloced;
}

/*
 * Process the "pre-switches": /nologo, /?, /help, /time, /repro, /fullpaths. If they exist,
 * they are nulled out.
 */
void ProcessPreSwitches(int argc, LPWSTR argv[])
{
    LPWSTR arg;

    if (argc <= 1)
    {
        g_bShowHelp = true;
        return;
    }


    // Now do all the other options
    for (int iArg = 0; iArg < argc; ++iArg)
    {
        arg = argv[iArg];
        if (arg == NULL)
            continue;
        if (arg[0] == '-' || arg[0] == '/')
            ++arg; // Advance to next character
        else
            continue;  // Not an option.

        if (_wcsicmp(arg, L"nologo") == 0)
            g_bNoLogo = true;
        else if (_wcsicmp(arg, L"?") == 0)
            g_bShowHelp = true;
        else if (_wcsicmp(arg, L"help") == 0)
            g_bShowHelp = true;
        else if (_wcsicmp(arg, L"time") == 0)
            g_bTime = true;
        else if (_wcsicmp(arg, L"fullpaths") == 0)
            g_bFullPaths = true;
        else
            continue;       // Not a recognized argument.

        argv[iArg] = NULL;  // NULL out recognized argument.
    }
}

/*
 * Get the file version as a string.
 */
void GetFileVersion(HINSTANCE hinst, CHAR *szVersion)
{
    strcpy(szVersion, SSCLI_VERSION_STR);
}

// Get version of the command language runtime.
typedef HRESULT (__stdcall *pfnGetCORVersion)(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength);
void GetCLRVersion(CHAR *szVersion, int maxBuf)
{
    WCHAR ver[MAX_PATH];
    DWORD dwLen;
    ASSERT(maxBuf > 50);         // don't be silly!
    strcpy(szVersion, "---");  // default in case we can't get it.
    HMODULE                              hMod;

    pfnGetCORVersion  pfnGetCORVersionFunction = NULL;
    hMod = LoadLibraryW(MSCOREE_SHIM_W);
    if (!hMod) {
        return;
    }

    pfnGetCORVersionFunction = (pfnGetCORVersion)GetProcAddress(hMod, "GetCORVersion");
    if (SUCCEEDED((*pfnGetCORVersionFunction)(ver, MAX_PATH, &dwLen))) {
        if (ver[0] == L'v') {
            WideCharToMultiByte(GetConsoleOutputCP(), 0, ver + 1, -1, szVersion, maxBuf, 0, 0);
            return;
        }
    }
}


/*
 * Print the introductory banner to the compiler.
 */
void PrintBanner()
{
    char version[100];
    char versionCLR[100];


    // Get the version string for this file.
    GetFileVersion(NULL, version);

    // Get the version string for CLR
    GetCLRVersion(versionCLR, sizeof(versionCLR));

    // Print it out.
    PrintString(IDS_BANNER1, false);
    print("%s\n", version);
    PrintString(IDS_BANNER1PART2, false);
    print("%s\n", versionCLR);
    PrintString(IDS_BANNER2, true);
    print("\n");

}

////////////////////////////////////////////////////////////////////////////////
// ParseCommandLine

HRESULT ParseCommandLine (int argc, PWSTR argv[])
{
    HRESULT                 hr = S_OK;
    bool                    usedShort;
    const WCHAR             delim[] = { ',', ';', '\0' };
    tree<const wchar_t*>    DupResCheck(L"Security.Evidence");

    // Okay, we're ready to process the command line
    for (int iArg = 0; iArg < argc; ++iArg)
    {
        // Skip blank args -- they've already been processed by ProcessPreSwitches or the like
        if (argv[iArg] == NULL || wcslen(argv[iArg]) == 0)
            continue;

        PWSTR   pszArg = argv[iArg];

        // If this is a switch, see what it is
        // NOTE: include "," here so that we give a good error message
        if (pszArg[0] == '-' || pszArg[0] == '/' || pszArg[0] == ',')
        {
            // Skip the switch leading char
            pszArg++;
            usedShort = false;

            // Check for options we know about
            // NOTE: If you add an option here, be sure to add the non-colon version
            // in the else clause for better error messages
            if (_wcsnicmp (pszArg, L"out:", 4) == 0)
            {
                if (pszArg[4] == '\0') { // Check for an empty string
                    pszArg[3] = L'\0';
                    ShowErrorIdString( ERR_NoFileSpec, ERROR_ERROR, pszArg);
                    hr = S_FALSE;
                    continue;
                }

                WCHAR szName[MAX_PATH];
                if (!GetFullFileName(pszArg + 4, szName, MAX_PATH))
                    continue;
                g_szAssemblyFile = VSAllocStr(szName);
                g_bMake = true;
            }

            else if (_wcsnicmp (pszArg, L"template:", 9) == 0)
            {
                if (pszArg[9] == '\0') { // Check for an empty string
                    pszArg[8] = L'\0';
                    ShowErrorIdString( ERR_NoFileSpec, ERROR_ERROR, pszArg);
                    hr = S_FALSE;
                    continue;
                }

                // Parent Assembly Info
                g_szCopyAssembly = pszArg + 9;
            }

            else if ((usedShort = (_wcsnicmp (pszArg, L"embed:", 6) == 0)) ||
                (_wcsnicmp (pszArg, L"embedresource:", 14) == 0))
            {
                int iOptLen = 14;
                if (usedShort)
                    iOptLen = 6;

                // Embedded resource
                WCHAR szName[MAX_PATH];
                LPWSTR pTemp = wcstok(pszArg + iOptLen, delim);
                if (wcslen(pszArg + iOptLen) == 0) {
                    pszArg[iOptLen - 1] = L'\0';
                    ShowErrorIdString (ERR_NoFileSpec, ERROR_ERROR, pszArg);
                    hr = S_FALSE;
                    continue;
                } else {
                    if (!GetFullFileName( pTemp, szName, MAX_PATH))
                        continue;
                }

                resource *res = (resource*)VSAlloc(sizeof(resource));
                res->bEmbed = true;
                res->dwOffset = 0;
                res->pszSource = VSAllocStr(szName);
                if ((pTemp = wcstok( NULL, delim)) == NULL) {
                    if ((pTemp = wcsrchr(szName, L'\\')) == NULL)
                        res->pszIdent = VSAllocStr(szName); // defaults to filename minus path
                    else {
                        pTemp++;
                        res->pszIdent = VSAllocStr(pTemp);
                    }
                    res->bVis = true; // default
                } else {
                    res->pszIdent = VSAllocStr(pTemp);
                    pTemp = wcstok( NULL, delim);
                    if (pTemp == NULL)
                        res->bVis = true;
                    else if (_wcsicmp(pTemp, L"private") == 0)
                        res->bVis = false;
                    else {
                        pszArg[iOptLen - 1] = L'\0';
                        ShowErrorIdString (ERR_BadOptionValue, ERROR_ERROR, pszArg, pTemp);
                        VSFree(res->pszSource);
                        VSFree(res->pszIdent);
                        VSFree(res);
                        hr = S_FALSE;
                        continue;
                    }
                }

                *g_ppResListEnd = (ResList*)VSAllocZero(sizeof(ResList));
                (*g_ppResListEnd)->arg = res;
                g_ppResListEnd = &(*g_ppResListEnd)->next;
                if (!DupResCheck.Add( res->pszIdent, wcscmp))
                    ShowErrorIdString(ERR_DupResourceIdent, ERROR_ERROR, res->pszIdent);
            }

            else if ((usedShort = (_wcsnicmp (pszArg, L"link:", 5)) == 0) || (_wcsnicmp (pszArg, L"linkresource:", 13) == 0))
            {
                // Link in a resource
                WCHAR szName[MAX_PATH];
                int iOptLen = (usedShort ? 5 : 13);
                LPWSTR pTemp = wcstok(pszArg + iOptLen, delim);

                if (wcslen(pszArg + iOptLen) == 0) {
                    pszArg[iOptLen - 1] = L'\0';
                    ShowErrorIdString (ERR_NoFileSpec, ERROR_ERROR, pszArg);
                    hr = S_FALSE;
                    continue;
                } else {
                    if (!GetFullFileName( pTemp, szName, MAX_PATH))
                        continue;
                }

                resource *res = (resource*)VSAlloc(sizeof(resource));
                res->bEmbed = false;
                res->bVis = true; // default
                res->pszSource = VSAllocStr(szName);
                if ((pTemp = wcstok( NULL, delim)) == NULL) {
                    if ((pTemp = wcsrchr(szName, L'\\')) == NULL)
                        res->pszIdent = VSAllocStr(szName); // defaults to filename minus path
                    else {
                        pTemp++;
                        res->pszIdent = VSAllocStr(pTemp);
                    }
                    res->pszTarget = VSAllocStr(szName);    // default to filename
                } else {
                    res->pszIdent = VSAllocStr(pTemp);
                    if ((pTemp = wcstok( NULL, delim)) == NULL) {
                        res->pszTarget = VSAllocStr(szName);    // defualt to filename
                    } else {
                        if (!GetFullFileName( pTemp, szName, MAX_PATH)) {
                            VSFree(res->pszSource);
                            VSFree(res->pszIdent);
                            VSFree(res);
                            continue;
                        }
                        res->pszTarget = VSAllocStr(szName);
                        pTemp = wcstok( NULL, delim);
                        if (pTemp == NULL)
                            res->bVis = true;
                        else if (_wcsicmp(pTemp, L"private") == 0)
                            res->bVis = false;
                        else {
                            pszArg[iOptLen - 1] = L'\0';
                            ShowErrorIdString (ERR_BadOptionValue, ERROR_ERROR, pszArg, pTemp);
                            VSFree(res->pszSource);
                            VSFree(res->pszIdent);
                            VSFree(res->pszTarget);
                            VSFree(res);
                            hr = S_FALSE;
                            continue;
                        }
                    }
                }

                *g_ppResListEnd = (ResList*)VSAllocZero(sizeof(ResList));
                (*g_ppResListEnd)->arg = res;
                g_ppResListEnd = &(*g_ppResListEnd)->next;
                if (!DupResCheck.Add( res->pszIdent, wcscmp))
                    ShowErrorIdString(ERR_DupResourceIdent, ERROR_ERROR, res->pszIdent);
            }

            // Evidence is nothing more than a glorified Embedded resource
            else if ((usedShort = (_wcsnicmp (pszArg, L"e:", 2)) == 0) || (_wcsnicmp (pszArg, L"evidence:", 9) == 0))
            {
                // Embedded resource
                WCHAR szName[MAX_PATH];
                int iOptLen = (usedShort ? 2 : 9);
                if (wcslen(pszArg + iOptLen) == 0) {
                    pszArg[iOptLen - 1] = L'\0';
                    ShowErrorIdString (ERR_NoFileSpec, ERROR_ERROR, pszArg);
                    hr = S_FALSE;
                    continue;
                } else {
                    if (!GetFullFileName( pszArg + iOptLen, szName, MAX_PATH))
                        continue;
                }

                resource *res = (resource*)VSAlloc(sizeof(resource));
                res->bEmbed = true;
                res->dwOffset = 0;
                res->pszSource = VSAllocStr(szName);
                res->pszIdent = VSAllocStr(DupResCheck.name); // "Security.Evidence"
                res->bVis = false;

                *g_ppResListEnd = (ResList*)VSAllocZero(sizeof(ResList));
                (*g_ppResListEnd)->arg = res;
                g_ppResListEnd = &(*g_ppResListEnd)->next;
            }

            else if ((usedShort = (_wcsnicmp (pszArg, L"base:", 5) == 0)) ||
                (_wcsnicmp(pszArg, L"baseaddress:", 12) == 0))
            {
                WCHAR *endCh;
                int iOptLen = (usedShort ? 5 : 12);
                DWORD imageBase = (wcstoul( pszArg + iOptLen, &endCh, 0) + 0x00008000) & 0xFFFF0000; // Round the low order word to align it
                if (*endCh == L'\0' && endCh == (pszArg + iOptLen)) // No number
                {
                    pszArg[iOptLen - 1] = L'\0';
                    ShowErrorIdString( ERR_SwitchNeedsString, ERROR_ERROR, pszArg);
                    hr = S_FALSE;
                    continue;
                }
                else if (*endCh != L'\0')    // Invalid number
                {
                    pszArg[iOptLen - 1] = L'\0';
                    ShowErrorIdString( ERR_BadOptionValue, ERROR_ERROR, pszArg, pszArg + iOptLen + 1);
                    hr = S_FALSE;
                    continue;
                }

                // Set the image base.
                g_dwDllBase = imageBase;
            }

            else if ((usedShort = (_wcsnicmp (pszArg, L"t:", 2) == 0)) ||
                (_wcsnicmp(pszArg, L"target:", 7) == 0))
            {
                int iOptLen = (usedShort ? 2 : 7);
                LPWSTR pType = pszArg + iOptLen;
                if (_wcsnicmp(pType, L"lib", 3) == 0 &&
                    (pType[3] == 0 || _wcsicmp(pType, L"library") == 0))
                    g_Target = ttDll;
                else if (_wcsnicmp(pType, L"exe", 3) == 0)
                    g_Target = ttConsole;
                else if (_wcsicmp(pType, L"win") == 0 &&
                    (pType[3] == 0 || _wcsicmp(pType, L"winexe") == 0))
                    g_Target = ttWinApp;
                else {
                    pszArg[iOptLen - 1] = 0;
                    ShowErrorIdString( ERR_BadOptionValue, ERROR_ERROR, pszArg, pType);
                    hr = S_FALSE;
                    continue;
                }
            }

            else if (_wcsnicmp (pszArg, L"main:", 5) == 0)
            {
                if (wcslen(pszArg + 5) <= 0) {
                    pszArg[4] = 0;
                    ShowErrorIdString (ERR_SwitchNeedsString, ERROR_ERROR, pszArg);
                    hr = S_FALSE;
                } else {
                    g_szAppMain = VSAllocStr(pszArg + 5);
                }
            }


            else
            {
                size_t  iArgLen;
                size_t  iSwitchLen = 0;
                bool    match;

                // It isn't one we recognize, so it must be an option exposed by the compiler.
                // See if we can find it.
                iArgLen = wcslen (pszArg);
                long i;
                for (i=0; i<optLastAssemOption; i++)
                {
                    if ((Options[i].flag & 0x40) != 0)
                        // This options has been removed, do not use it
                        continue;
                    // See if it matches the arg we have (up to length of switch name only)
                    match = false;

                    // FYI: below test of pszArg[iSwitchLen] < 0x40 tests for '\0', ':', '+', '-', but not a letter -- e.g., make sure
                    // this isn't a prefix of some longer switch.

                    // Try new short option
                    if (Options[i].ShortName)
                        match = (iArgLen >= (iSwitchLen = wcslen (Options[i].ShortName))
                                && (_wcsnicmp (Options[i].ShortName, pszArg, iSwitchLen) == 0
                                && pszArg[iSwitchLen] < 0x40 ));

                    // Try long option if it exists
                    if (!match && Options[i].LongName) {
                        ASSERT(Options[i].LongName);
                        match = (iArgLen >= (iSwitchLen = wcslen (Options[i].LongName))
                                && (_wcsnicmp (Options[i].LongName, pszArg, iSwitchLen) == 0
                                && pszArg[iSwitchLen] < 0x40 ));
                    }

                    if (match)
                    {
                        // We have a match, possibly.  If this is a string switch, we need to have
                        // a colon and some text.  If boolean, we need to check for optional [+|-]
                        if (Options[i].vt == VT_BOOL)
                        {
                            // A boolean option -- look for [+|-]
                            if (iArgLen == iSwitchLen) {
                                V_VT(&(g_Options[i])) = VT_BOOL;
                                V_BOOL(&(g_Options[i])) = VARIANT_TRUE;
                            } else if (iArgLen == iSwitchLen + 1 && (pszArg[iSwitchLen] == '-' || pszArg[iSwitchLen] == '+')) {
                                V_VT(&(g_Options[i])) = VT_BOOL;
                                V_BOOL(&(g_Options[i])) = (pszArg[iSwitchLen] == '+') ? VARIANT_TRUE : VARIANT_FALSE;
                            } else
                                continue;   // Keep looking
                        }
                        else
                        {
                            ASSERT(Options[i].vt == VT_BSTR || Options[i].vt == VT_UI4);
                            
                            // String option.  Following the argument should be a colon, and we'll
                            // use whatever text follows as the string value for the option.
                            if (iArgLen <= iSwitchLen + 1)
                            {
                                if (Options[i].vt == VT_BSTR)
                                    ShowErrorIdString (ERR_SwitchNeedsString, ERROR_ERROR, pszArg);
                                else
                                    ShowErrorIdString (ERR_MissingOptionArg, ERROR_ERROR, pszArg);
                                break;
                            }

                            if (pszArg[iSwitchLen] != ':')
                                continue;   // Keep looking

                            if (Options[i].vt == VT_BSTR) {
                                WCHAR buffer[MAX_PATH];
                                LPWSTR pszValue = pszArg + iSwitchLen + 1;

                                if ((Options[i].flag & 0x10) != 0) {
                                    // Get full paths for filename options
                                    if (GetFullFileName( pszValue, buffer, MAX_PATH))
                                        pszValue = buffer;
                                    else
                                        break;
                                }

                                if (!(Options[i].flag & 0x04) && V_VT(&g_Options[i]) != VT_EMPTY)
                                    // Only allow lists if this is an 'allow-multi' option
                                    VariantClear(&g_Options[i]);
      
                                VARIANT svt;
                                VariantInit(&svt);
                                V_VT(&svt) = VT_BSTR;
                                V_BSTR(&svt) = SysAllocString(pszValue);
                                AddVariantToList(g_Options[i], svt); // Transfer ownership, so don't clean-up

                            } else {
                                LPWSTR pEnd = NULL;
                                ULONG val = wcstoul(pszArg + iSwitchLen + 1, &pEnd, 16);
                                if (*pEnd != L'\0') // Invalid Value!
                                {
                                    pszArg[iSwitchLen] = L'\0';
                                    ShowErrorIdString (ERR_BadOptionValue, ERROR_ERROR, pszArg, pszArg + iSwitchLen + 1);
                                }
                                else
                                {
                                    if (!(Options[i].flag & 0x04) && V_VT(&g_Options[i]) != VT_EMPTY)
                                        // Only allow lists if this is an 'allow-multi' option
                                        VariantClear(&g_Options[i]);
      
                                    VARIANT svt;
                                    VariantInit(&svt);
                                    V_VT(&svt) = VT_UI4;
                                    V_UI4(&svt) = val;
                                    AddVariantToList(g_Options[i], svt); // Transfer ownership, so don't clean-up
                                }
                            }

                            if ((Options[i].flag & 0x20) != 0) {
                                ShowErrorIdString (WRN_FeatureDeprecated, ERROR_WARNING, pszArg, L"nothing");
                            }
                        }

                        break;
                    }
                }

                if (i == optLastAssemOption)
                {
                    // Check for normal options minus the colon
                    // So we can give a better error message
                    if (_wcsicmp(pszArg, L"out") == 0 ||
                        _wcsicmp(pszArg, L"template") == 0 ||
                        _wcsicmp(pszArg, L"embed") == 0 || _wcsicmp(pszArg, L"embedresource") == 0 ||
                        _wcsicmp(pszArg, L"link") == 0 || _wcsicmp(pszArg, L"linkresource") == 0 ||
                        _wcsicmp(pszArg, L"e") == 0 || _wcsicmp(pszArg, L"evidence") == 0)
                        // Options that need a file-spec
                        ShowErrorIdString( ERR_NoFileSpec, ERROR_ERROR, pszArg);

                    else if (_wcsicmp(pszArg, L"base") == 0 || _wcsicmp(pszArg, L"baseaddress") == 0 ||
                        _wcsicmp(pszArg, L"t") == 0 || _wcsicmp(pszArg, L"target") == 0 ||
                        _wcsicmp(pszArg, L"main") == 0)
                        // Options that need a string
                        ShowErrorIdString( ERR_SwitchNeedsString, ERROR_ERROR, pszArg);

                    else if (pszArg[0] == L'\0')
                        ShowErrorIdString (ERR_BadSwitch, ERROR_ERROR, argv[iArg]);
                    else
                    {
#if PLATFORM_UNIX
                        if (pszArg[-1] == '/')
                        {
                            // On BSD it could be a fully qualified file that starts with '/'
                            // Not a switch, so it might be a FileName
                            goto AssumeFileName;
                        }
                        else
#endif // PLATFORM_UNIX
                        {
                            // Didn't find it...
                            ShowErrorIdString (ERR_BadSwitch, ERROR_ERROR, pszArg);
                        }
                    }

                    hr = S_FALSE;
                }
            }
        }
        else
        {
#ifdef PLATFORM_UNIX
AssumeFileName:
#endif
            // Not a switch, so it must be a file spec
            WCHAR szName[MAX_PATH];
            LPWSTR pNewName = wcschr(argv[iArg], L',');
            if (pNewName) {
                *pNewName = 0;
                pNewName++;
                if (wcslen(pNewName) == 0)
                    pNewName = NULL;
            }
            if (!GetFullFileName(argv[iArg], szName, MAX_PATH))
                continue;

            *g_ppFileListEnd = new FileList();
            (*g_ppFileListEnd)->arg.pszSource = VSAllocStr(szName);
            if (pNewName) {
                if (!GetFullFileName(pNewName, szName, MAX_PATH)) {
                    VSFree((*g_ppFileListEnd)->arg.pszSource);
                    VSFree(*g_ppFileListEnd);
                    *g_ppFileListEnd = NULL;
                    continue;
                }
                (*g_ppFileListEnd)->arg.pszTarget = VSAllocStr(szName);
            }
            g_ppFileListEnd = &(*g_ppFileListEnd)->next;
        }
    }

    if (FAILED (hr) || g_bHadError)
        return hr;

    // at least on input must be specified
    if (g_bMake && g_pResListHead == NULL && g_pFileListHead == NULL)
        ShowErrorId(ERR_NoInputs, ERROR_ERROR);

    // Make sure at least one "output" was specified
    if (g_szAssemblyFile == NULL || (!g_bMake && g_szAssemblyFile != NULL && (g_pResListHead != NULL || g_pFileListHead != NULL)))
        ShowErrorId(ERR_NoOutput, ERROR_ERROR);

    // Check for invalid main conditions
    if (g_Target == ttDll && g_szAppMain != NULL) {
        ShowErrorId(ERR_NoMainOnDlls, ERROR_ERROR);
    } else if (g_Target != ttDll && g_szAppMain == NULL) {
        ShowErrorId(ERR_AppNeedsMain, ERROR_ERROR);
    }

    if (g_szAssemblyFile && g_pFileListHead && !g_bHadError) {

        // Check to see if a filename is the same as the assembly name
        FileList *files, *prev;
        files = g_pFileListHead;
        prev = NULL;
        while(files != NULL) {
            if ((files->arg.pszTarget && _wcsicmp(files->arg.pszTarget, g_szAssemblyFile) == 0) || 
                _wcsicmp(files->arg.pszSource, g_szAssemblyFile) == 0) {
                ShowErrorIdString(ERR_SameOutAndSource, ERROR_ERROR, g_szAssemblyFile);
                break;
            }
            prev = files;
            files = files->next;
        }

    }

    // always return S_OK if we report the error
    // because a failure HR will cause another error to be reported
    hr = S_OK;

    return hr;
}

/*
 * Main entry point
 */
extern "C" int __cdecl wmain(int argc, WCHAR **argv)
{
    HRESULT hr = E_FAIL;  // assume failure unless otherwise.
    LARGE_INTEGER timeStart, timeStop, frequency;
    LPWSTR * args = NULL;
    WStrList *vals = NULL;
    IALink  *pLinker = NULL;
    ICeeFileGen *pFileGen = NULL;
    IMetaDataDispenserEx *dispenser = NULL;
    CErrors *errHandler = NULL;
    FileList *files = NULL;
    ResList *res = NULL;

    VsDebugInitialize();

    // Initialize Unilib
    g_bUnicode = (W_IsUnicodeSystem() == TRUE);

    // Try to load the message DLL.
    g_hinstMessages = GetALinkMessageDll();
    if (! g_hinstMessages) {
        // we can't do anything without the messages DLL. This is the only message
        // that can't be localized!
        ShowError(errorInfo[FTL_NoMessagesDLL].number, ERROR_FATAL, NULL, -1, -1, -1, "Unable to find messages file 'alink.satellite'");
        g_bHadError = true;
        goto Error;
    }

    if (!PAL_RegisterLibraryW(L"sscoree"))
    {
        ShowErrorIdString(FTL_ComPlusInit, ERROR_FATAL, ErrorHR(hr));
        g_bHadError = true;
        goto Error;
    }

    // Initialize the global options
    for (int i = 0; i < optLastAssemOption; i++)
        VariantInit(&g_Options[i]);
  
    // Process Response Files
    // We do this before the PreSwitches in case there are any
    // switches in the response files.
    if (ProcessResponseArgs( argc, &argv, &vals))
        args = argv;    // Save this to free later

    // Do initial switch processing.
    ProcessPreSwitches(argc, argv);

    // All errors previous to this are fatal
    // This solve the error-before-logo problem
    if (g_bHadError)
        goto Error;

    // Print the logo banner, unless noLogo was specified.
    if (!g_bNoLogo)
        PrintBanner();

    // If timing, start the timer.
    if (g_bTime) {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&timeStart);
    }


    errHandler = new CErrors();

    if (g_bShowHelp)
    {
        // Display help and do nothing else
        hr = PrintHelp ();
    }
    else
    {
        // Parse the command line, and if successful, compile.
        if (FAILED (hr = ParseCommandLine (argc - 1, argv + 1)))
        {
            ShowErrorIdString (FTL_InitError, ERROR_FATAL, ErrorHR(hr));
        } else if ((FAILED (hr = CreateALink (IID_IALink, (IUnknown**)&pLinker))) ||
            (FAILED(hr = CoCreateInstance(CLSID_CorMetaDataDispenser, NULL, CLSCTX_SERVER,
                                             IID_IMetaDataDispenserEx,
                                             (LPVOID *) & dispenser))) ||
            (FAILED(hr = SetDispenserOptions( dispenser))) ||
            ((pFileGen = CreateCeeFileGen(dispenser)) == NULL) ||
            (FAILED(hr = pLinker->Init(dispenser, errHandler)))) {
            if (hr != S_OK && FAILED(CheckHR(hr)))
                ShowErrorIdString(FTL_ComPlusInit, ERROR_FATAL, ErrorHR(hr));
        } else if (!g_bHadError && g_bMake) {
            MakeAssembly(pFileGen, dispenser, pLinker, errHandler);
        }
    }

    if (pLinker)
        pLinker->Release();
    if (pFileGen)
        DestroyCeeFileGen(pFileGen);
    FreeLibrary(g_hmodCorPE);
    if (dispenser)
        dispenser->Release();
    if (errHandler)
        VSFree(errHandler);

    // Free all the files
    while((files = g_pFileListHead) != NULL) {
        VSFree(files->arg.pszSource);
        VSFree(files->arg.pszTarget);
        g_pFileListHead = files->next;
        VSFree(files);
    }
    if (g_szAssemblyFile)
        VSFree(g_szAssemblyFile);
    if (g_szAppMain)
        VSFree(g_szAppMain);

    // Free the resources
    while((res = g_pResListHead) != NULL) {
        VSFree( res->arg->pszSource);
        VSFree( res->arg->pszIdent);
        if (!res->arg->bEmbed)
            VSFree( res->arg->pszTarget);
        VSFree( res->arg);
        g_pResListHead = res->next;
        VSFree(res);
    }

    if (args)
    {
        for (WStrList *l = vals; l;)
        {
            WStrList *t;
            VSFree(l->arg);
            t = l->next;
            VSFree(l);
            l = t;
        }
        VSFree(args);
    }


    if (g_bTime) {
        double elapsedTime;
        QueryPerformanceCounter(&timeStop);
        elapsedTime = (double) (timeStop.QuadPart - timeStart.QuadPart) / (double) frequency.QuadPart;
        printf("\nElapsed time: %#.4g seconds\n", elapsedTime);
    }


Error:

    // Cleanup
    VsDebugTerminate();
    _optionused::FreeOptionUsed();

    // Return success code.
    return (hr != S_OK || g_bHadError) ? 1 : 0;
}

#define ICEEFILEGENDLL MAKEDLLNAME_W(L"mscorpe")

/*
 * Loads mscorpe.dll and gets an ICeeFileGen interface from it.
 * The ICeeFileGen interface is used for the entire compile.
 */
ICeeFileGen* CreateCeeFileGen(IMetaDataDispenserEx *pDisp)
{
    // Dynamically bind to ICeeFileGen functions.
    if (!g_pfnCreateCeeFileGen || !g_pfnDestroyCeeFileGen) {

        WCHAR wszPath[MAX_PATH];
        CHAR  szPath[MAX_PATH];
        DWORD dwWLen = 0, dwLen = 0;
        HRESULT hr;

        if (FAILED(hr = pDisp->GetCORSystemDirectory( wszPath, MAX_PATH, &dwWLen))) {
            ShowErrorIdString(FTL_ComPlusInit, ERROR_FATAL, ErrorHR(hr));
            return NULL;
        }

        dwLen = WideCharToMultiByte(CP_ACP, 0, wcscat(wszPath, ICEEFILEGENDLL), -1, szPath, MAX_PATH, NULL, NULL) ;

        if (dwLen != dwWLen + wcslen(ICEEFILEGENDLL)) {
            ShowErrorIdString(FTL_ComPlusInit, ERROR_FATAL, ErrorLastError());
            return NULL;
        }

        g_hmodCorPE = LoadLibraryA(szPath);
        if (g_hmodCorPE) {
            // Get the required methods.
            g_pfnCreateCeeFileGen  = (HRESULT (__stdcall *)(ICeeFileGen **ceeFileGen)) GetProcAddress(g_hmodCorPE, "CreateICeeFileGen");
            g_pfnDestroyCeeFileGen = (HRESULT (__stdcall *)(ICeeFileGen **ceeFileGen)) GetProcAddress(g_hmodCorPE, "DestroyICeeFileGen");
            if (!g_pfnCreateCeeFileGen || !g_pfnDestroyCeeFileGen) {
                ShowErrorIdString(FTL_ComPlusInit, ERROR_FATAL, ErrorLastError());
                return NULL;
            }
        }
        else {
            // MSCorPE.DLL wasn't found.
            ShowErrorIdString(FTL_RequiredFileNotFound, ERROR_FATAL, wszPath);
            return NULL;
        }
    }

    ICeeFileGen *ceefilegen = NULL;
    HRESULT hr = g_pfnCreateCeeFileGen(& ceefilegen);
    if (FAILED(hr)) {
        ShowErrorIdString(FTL_ComPlusInit, ERROR_FATAL, ErrorHR(hr));
    }

    return ceefilegen;
}

void DestroyCeeFileGen(ICeeFileGen *ceefilegen)
{
    ASSERT(g_pfnDestroyCeeFileGen != NULL ||
           ceefilegen == NULL);
    if (ceefilegen != NULL)
        g_pfnDestroyCeeFileGen(&ceefilegen);
}

/*
 * sets the options on the current metadata dispenser
 */
HRESULT SetDispenserOptions(IMetaDataDispenserEx * dispenser)
{
    VARIANT v;
    HRESULT hr = E_POINTER;

    if (dispenser) {
        // We shouldn't emit any Dups, so check for everything
        V_VT(&v) = VT_UI4;
        V_UI4(&v) = MDDupAll;
        if (FAILED(hr = dispenser->SetOption(MetaDataCheckDuplicatesFor, &v)))
            return hr;

        // Never change refs to defs
        V_VT(&v) = VT_UI4;
        if (g_bMerge)
            V_UI4(&v) = MDRefToDefAll;
        else
            V_UI4(&v) = MDRefToDefNone;
        if (FAILED(hr = dispenser->SetOption(MetaDataRefToDefCheck, &v)))
            return hr;

        // Give error if emitting out of order
        V_VT(&v) = VT_UI4;
        V_UI4(&v) = MDErrorOutOfOrderAll;
        if (FAILED(hr = dispenser->SetOption(MetaDataErrorIfEmitOutOfOrder, &v)))
            return hr;

        // Turn on regular update mode
        V_VT(&v) = VT_UI4;
        V_UI4(&v) = MDUpdateFull;
        hr = dispenser->SetOption(MetaDataSetUpdate, &v);
    }
    return hr;
}

/*
 * This actually creates the manifest file and calls into ALink to create the metadata
 */
void MakeAssembly(ICeeFileGen *pFileGen, IMetaDataDispenserEx *pDisp, IALink *pLinker, IMetaDataError *pErrHandler)
{
    HRESULT                     hr;
    HCEEFILE                    file;
    HCEESECTION                 ilSection;
    HCEESECTION                 resSection;
    IMetaDataEmit              *metaemit;
    IMetaDataAssemblyImport    *pImport = NULL;
    LPCWSTR                     moduleName = NULL;
    LPCWSTR                     pszLocation = NULL;
    mdAssembly                  assemID = mdTokenNil;
    mdToken                     fileID = mdTokenNil;
    DWORD                       dwTemp = 0, dwStrongName = 0;
    DWORD                       dwResOffset = 0;
    WCHAR                       buffer1[_MAX_PATH];
    WCHAR                       buffer2[_MAX_PATH];
    FileList                   *files;
    ResList                    *res;
    int                         iArgs = 0;
    bool                        bHasBogusMain = false;
    bool                        bHadFile = false;
    mdToken                     tkEntryPoint = mdTokenNil;
    mdMethodDef                 tkMain = mdTokenNil;
    LPWSTR                      szTypeName = NULL;
    LPWSTR                      szMethodName = NULL;
    AssemblyFlags               f = afNone;

    // Create a new scope to emit to
    IfFailGo(pDisp->DefineScope(CLSID_CorMetaDataRuntime,  // Format of metadata
                                       0,                         // flags
                                       IID_IMetaDataEmit,         // Emitting interface
                                       (IUnknown **) &metaemit));

    if (g_bClean)
        f = (AssemblyFlags)(f | afCleanModules);
    if (g_bNoRefHash)
        f = (AssemblyFlags)(f | afNoRefHash);
    IfFailGo(pLinker->SetAssemblyFile(g_szAssemblyFile, metaemit, f, &assemID));

    // Split the 'main' into TypeName and MethodName
    if (g_szAppMain) {
        szMethodName = wcsrchr(g_szAppMain, L'.');
        if (szMethodName) {
            *szMethodName = 0;
            szMethodName++;
            szTypeName = g_szAppMain;
        } else {
            szMethodName = g_szAppMain;
            szTypeName = NULL;
        }

        if ((szTypeName != NULL && wcslen(szTypeName) < 1) || wcslen(szMethodName) < 1) {
            ShowErrorIdString(ERR_BadOptionValue, ERROR_ERROR, L"main", g_szAppMain);
            VSFree(g_szAppMain);
            szTypeName = szMethodName = g_szAppMain = NULL;
        }
    }

    // Get the parent assembly info, and copy into this one
    if (g_szCopyAssembly) {
        pszLocation = g_szCopyAssembly;
        IfFailGo(CopyAssembly(assemID, pLinker));
    }
    pszLocation = NULL;

    // Import all the files 
    files = g_pFileListHead;
    while(files != NULL) {
        pszLocation = files->arg.pszSource;
        AddBinaryFile(pszLocation);
        IfFailGo(pLinker->ImportFile( files->arg.pszSource, files->arg.pszTarget, FALSE, &fileID, &pImport, &dwTemp));
        if (pImport != NULL || dwTemp != 1) {
            // This is an assembly
            ShowErrorIdString(WRN_IgnoringAssembly, ERROR_WARNING, files->arg.pszSource);
            pImport->Release();
            pImport = NULL;
        } else {
            CComPtr<IMetaDataImport> pI = NULL;
            if (SUCCEEDED(hr = pLinker->GetScope(assemID, fileID, 0, &pI))) {
                if (g_szAppMain && IsNilToken(tkEntryPoint)) {
                    // Look for the 'Main' routine
                    // this also creates the __EntryPoint method, and the methodref to the original main
                    IfFailGo(FindMain( pI, g_bMerge ? NULL : metaemit, szTypeName, szMethodName, tkEntryPoint, tkMain, iArgs, bHasBogusMain));
                }
                if (g_bMerge) {
                } else {
                    IfFailGo(pLinker->AddImport(assemID, fileID, 0, &fileID));
                }
                bHadFile = true;
            }
        }
        files = files->next;
    }
    pszLocation = NULL;
    if (!bHadFile && !g_pResListHead) {
        ShowErrorId(ERR_NoInputs, ERROR_ERROR);
        return;
    }

    // Set the file info, etc.
    IfFailGo(pFileGen->CreateCeeFile( &file));

    IfFailGo(pFileGen->GetIlSection( file, &ilSection));

    IfFailGo(pFileGen->GetRdataSection( file, &resSection));

    _wsplitpath(g_szAssemblyFile, NULL, NULL, buffer1, buffer2);    // Strip the path
    moduleName = wcscat(buffer1, buffer2);
    IfFailGo(metaemit->SetModuleProps(moduleName));

    // Merge them all together and deal with the token remaps

    // Report an appropriate error
    if (g_szAppMain && IsNilToken(tkEntryPoint)) {
        if (szMethodName != g_szAppMain)
            *(szMethodName - 1) = '.'; // reconstruct the full name
        if (bHasBogusMain)
            ShowErrorIdString( ERR_BadMainFound, ERROR_ERROR, g_szAppMain);
        else
            ShowErrorIdString( ERR_NoMainFound, ERROR_ERROR, g_szAppMain);
    }


    // Set the options, after importing files so they override the CAs
    IfFailGo(SetAssemblyOptions(assemID, pLinker));

    IfFailGo(pLinker->EmitManifest( assemID, &dwStrongName, NULL));

    if (!IsNilToken(tkEntryPoint)) {
        if (!g_bMerge)
            IfFailGo(MakeMain(pFileGen, file, ilSection, metaemit, tkMain, tkEntryPoint, iArgs));

        IfFailGo(pFileGen->SetEntryPoint( file, tkMain));
    }

    if (dwStrongName != 0) {
        void  *pvBuffer = NULL;
        IfFailGo(pFileGen->GetSectionBlock( ilSection, dwStrongName, 1, &pvBuffer));
        memset(pvBuffer, 0, dwStrongName); // Zero it out

        IfFailGo(pFileGen->GetSectionDataLen(ilSection, &dwTemp));

        IfFailGo(pFileGen->GetMethodRVA(file, dwTemp - dwStrongName, &dwTemp));

        IfFailGo(pFileGen->SetStrongNameEntry( file, dwStrongName, dwTemp));
    }

    // Add all the COM+ Resources
    for( res = g_pResListHead; res != NULL; res = res->next) {
        resource *r = res->arg;
        AddBinaryFile(r->pszSource);
        if (r->bEmbed) {
            HANDLE hFile;
            DWORD fileLen = 0, dwRead;
            void *pvBuffer;

            r->dwOffset = dwResOffset;

            hFile = OpenFileEx (r->pszSource, &fileLen);
            if (hFile == INVALID_HANDLE_VALUE) {
                ShowErrorIdString( ERR_CantReadResource, ERROR_ERROR, r->pszSource, ErrorLastError());
                continue;
            }
            dwResOffset += fileLen + sizeof(DWORD);

            if (FAILED(hr = pFileGen->GetSectionBlock( resSection, fileLen + sizeof(DWORD), 1, &pvBuffer))) {
                ShowErrorIdLocation( ERR_CantEmbedResource, ERROR_ERROR, g_szAssemblyFile, r->pszSource, ErrorHR(hr));
                CloseHandle( hFile);
                continue;
            }

            SET_UNALIGNED_VAL32(pvBuffer, fileLen);
            if (!ReadFile( hFile, ((DWORD*)pvBuffer) + 1, fileLen, &dwRead, NULL)) {
                ShowErrorIdString( ERR_CantReadResource, ERROR_ERROR, r->pszSource, ErrorLastError());
                CloseHandle( hFile);
                continue;
            }
            CloseHandle( hFile);

            if (FAILED(hr = pLinker->EmbedResource( assemID, assemID, r->pszIdent, r->dwOffset, r->bVis ? mrPublic : mrPrivate))) {
                if (FAILED(CheckHR(hr)))
                    ShowErrorIdString(ERR_MetaDataError, ERROR_ERROR, ErrorHR(hr));
            }

        } else {
            if (FAILED(hr = pLinker->LinkResource( assemID, r->pszSource, r->pszTarget, r->pszIdent, r->bVis ? mrPublic : mrPrivate))) {
                if (FAILED(CheckHR(hr)))
                    ShowErrorIdString(ERR_MetaDataError, ERROR_ERROR, ErrorHR(hr));
            }
        }
    }

    // This really sets the start of the COM+ resource section
    IfFailGo(pFileGen->GetSectionRVA(resSection, &dwTemp));
    IfFailGo(pFileGen->SetManifestEntry( file, dwResOffset, dwTemp));

    IfFailGo(pFileGen->SetOutputFileName( file, g_szAssemblyFile));


    IfFailGo(pFileGen->SetComImageFlags( file, COMIMAGE_FLAGS_ILONLY));
    if (g_Target == ttDll) {
        IfFailGo(pFileGen->SetDllSwitch( file, TRUE));
        if (g_dwDllBase != 0) {
            IfFailGo(pFileGen->SetImageBase( file, g_dwDllBase));
        }
    } else {
        IfFailGo(pFileGen->SetSubsystem(file, g_Target == ttConsole ? IMAGE_SUBSYSTEM_WINDOWS_CUI : IMAGE_SUBSYSTEM_WINDOWS_GUI, 4, 0));
    }

    IfFailGo(pLinker->PreCloseAssembly(assemID));

    IfFailGo(pFileGen->EmitMetaDataEx( file, metaemit));

    IfFailGo(pFileGen->GenerateCeeFile(file));

    IfFailGo(pLinker->CloseAssembly(assemID));

    // Cleanup - Make sure we delete the temp file
ErrExit:
    if (FAILED(hr)) {
        // Check for 'known' HRESULT's
        if (FAILED(CheckHR(hr))) {
            // Then fall back ona more generic error
            if (pszLocation)
                ShowErrorIdString(ERR_ModuleImportError, ERROR_ERROR, pszLocation, ErrorHR(hr));
            else
                ShowErrorIdString(ERR_MetaDataError, ERROR_ERROR, ErrorHR(hr));
        }
    }


    return;
}

/*
 * searches the import scope for the 'Main' method
 * once found, it generates a TypeRef and MemberRef in the current emit scope
 * and a MethodDef with the same signature
 */
HRESULT FindMain( IMetaDataImport *pImport, IMetaDataEmit *pEmit, LPCWSTR pszTypeName, LPCWSTR pszMethodName, // Inputs
                 mdMemberRef &tkMain, mdMethodDef &tkNewMain, int &countArgs, bool &bHadBogusMain)  // Outputs
{
    HRESULT hr = S_OK;
    mdTypeDef tkClass = mdTokenNil;
    countArgs = 0;
    tkNewMain = tkMain = mdTokenNil;

    if (pszTypeName == NULL || SUCCEEDED(hr = pImport->FindTypeDefByName(pszTypeName, mdTokenNil, &tkClass))) {
        HCORENUM hEnum = 0;
        mdMethodDef meth[4];
        DWORD cMeth = 0;

        do {
            if (FAILED(hr = pImport->EnumMethodsWithName( &hEnum, tkClass, pszMethodName, meth, lengthof(meth), &cMeth)))
                break;
            
            if (cMeth > 0) bHadBogusMain = true;
            for (DWORD iMeth = 0; iMeth < cMeth; iMeth++) {
                mdToken tkRef = mdTokenNil;
                PCCOR_SIGNATURE pSig;
                PCCOR_SIGNATURE sig;
                COR_SIGNATURE newSig[10]; // this should be big enough
                PCOR_SIGNATURE pNewSig = newSig;
                DWORD cbSig = 0, flags = 0, params = 0;

                if (FAILED(hr = pImport->GetMethodProps(meth[iMeth], NULL, NULL, 0, NULL, &flags, &sig, &cbSig, NULL, NULL)))
                    continue;
                if (!IsMdStatic(flags) || IsMdAbstract(flags))
                    continue; // we don't allow non-static or virtual Mains
                if (pEmit != NULL && !(IsMdPublic(flags) || IsMdAssem(flags) || IsMdFamORAssem(flags)))
                    continue;
                pSig = sig;
                if (*pSig++ != IMAGE_CEE_CS_CALLCONV_DEFAULT)
                    continue; // Only allow default calling convention
                if ((params = CorSigUncompressData(pSig)) > 2)
                    continue; // Only 0, 1, or 2 arguments
                if (*pSig == ELEMENT_TYPE_CMOD_OPT) {
                    pSig++;
                    CorSigUncompressToken( pSig); // Ignore the token
                }
                if (*pSig != ELEMENT_TYPE_I4 && *pSig != ELEMENT_TYPE_VOID)
                    continue; // must return void or int

                pNewSig = newSig;
                *pNewSig++ = IMAGE_CEE_CS_CALLCONV_DEFAULT;
                pNewSig += CorSigCompressData(params, pNewSig);
                *pNewSig++ = *pSig;
                pSig++;
                if (*pSig == ELEMENT_TYPE_CMOD_OPT) {
                    pSig++;
                    CorSigUncompressToken( pSig); // Ignore the token
                }
                if (params == 1) {
                    // C# style main, takes a string[]
                    countArgs = 1;
                    
                    if (*pSig++ != ELEMENT_TYPE_SZARRAY || *pSig++ != ELEMENT_TYPE_STRING)
                        continue;
                    *pNewSig++ = ELEMENT_TYPE_SZARRAY;
                    *pNewSig++ = ELEMENT_TYPE_STRING;
                } else if (params == 2) {
                    // MC++ style main, takes int, char**
                    countArgs = 2;
                    if (*pSig++ != ELEMENT_TYPE_I4)
                        continue;
                    if (*pSig == ELEMENT_TYPE_CMOD_OPT) {
                        pSig++;
                        CorSigUncompressToken( pSig); // Ignore the token
                    }
                    if (*pSig++ != ELEMENT_TYPE_PTR || *pSig++ != ELEMENT_TYPE_PTR || *pSig++ != ELEMENT_TYPE_CHAR)
                        continue;
                    *pNewSig++ = ELEMENT_TYPE_I4;
                    *pNewSig++ = ELEMENT_TYPE_PTR;
                    *pNewSig++ = ELEMENT_TYPE_PTR;
                    *pNewSig++ = ELEMENT_TYPE_CHAR;
                } else if (params == 0) {
                    countArgs = 0;
                } else
                    // Not allowed
                    continue;

                if (pEmit != NULL) {
                    if (tkClass != (mdTokenNil) && FAILED(hr = pEmit->DefineImportType(NULL, NULL, 0, pImport, tkClass, NULL, &tkRef)))
                        continue;
                    if (SUCCEEDED(hr = pEmit->DefineImportMember( NULL, NULL, 0, pImport, meth[iMeth], NULL, tkRef, &tkMain))) {
                        hr = pEmit->DefineMethod(mdTokenNil, L"__EntryPoint", mdStatic, newSig, (ULONG)(pNewSig - newSig), (ULONG) -1, miManaged | miIL, &tkNewMain);
                        cMeth = 0;
                        break;
                    }
                } else {
                    // This is the merging case, so we don't need to create and entrypoints
                    tkMain = tkNewMain = meth[iMeth];
                    cMeth = 0;
                    break;
                }
            }
        } while (cMeth > 0 && hr == S_OK);
    }
    
    if (hr == CLDB_E_RECORD_NOTFOUND) {
        hr = S_FALSE;
    }

    return hr;
}

/*
 * Converts a filename to it's fully-qualified, fully-expanded name
 */
bool GetFullFileName(LPWSTR szFilename, DWORD cchFilename)
{
    WCHAR buffer[MAX_PATH];
    DWORD len = 0;

    len = W_GetFullPathName(szFilename, MAX_PATH, buffer, NULL);
    if (len >= MAX_PATH || len >= cchFilename || cchFilename < MAX_PATH ||
        len == 0 || wcspbrk(szFilename, L"?*")) {
        ShowErrorIdString(ERR_FileNameTooLong, ERROR_FATAL, szFilename);
        return false;
    }

    // More than just getting the actual case, this also gets the 'long' filename
    W_GetActualFileCase(buffer, szFilename);
    return true;
}

/*
 * Converts a filename to it's fully-qualified, fully-expanded name
 */
bool GetFullFileName(LPCWSTR szSource, LPWSTR szFilename, DWORD cchFilename)
{
    WCHAR buffer[MAX_PATH];
    DWORD len = 0;

    len = W_GetFullPathName(szSource, MAX_PATH, buffer, NULL);
    if (len >= MAX_PATH || len >= cchFilename || cchFilename < MAX_PATH ||
        len == 0 || wcspbrk(szSource, L"?*")) {
        ShowErrorIdString(ERR_FileNameTooLong, ERROR_FATAL, szSource);
        return false;
    }

    // More than just getting the actual case, this also gets the 'long' filename
    W_GetActualFileCase(buffer, szFilename);
    return true;
}

// Error handler/reporter
HRESULT CErrors::OnError(HRESULT hr, mdToken tkLocation)
{
    if (hr == S_OK)
        return S_OK;

    if (hr == MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, errorInfo[WRN_OptionConflicts].number) &&
        (mdToken) Options[tkLocation].opt == tkLocation) {
        if (V_VT(&g_Options[tkLocation]) == VT_EMPTY && !_optionused::GetOptionUsed()->get(Options[tkLocation].opt)) {
            // If the user didn't give a command-line override
            // change this warning into a duplicate CA error!
            ShowErrorIdString(ERR_DuplicateCA, ERROR_ERROR, Options[tkLocation].CA);
        } else if (_optionused::GetOptionUsed()->get(Options[tkLocation].opt)) {
            // A module setting is just overriding a template setting
            // Ignore the first one, but reset this, so we'll report the next one
            _optionused::GetOptionUsed()->set(Options[tkLocation].opt, false);
        }
        // Never report a dupe error if the command-line overrides it
        return S_OK;
    }

    return CheckHR(hr);
}

int __cdecl MatchNumber(const void *p1, const void *p2)
{
    const ERROR_INFO * e1 = (const ERROR_INFO*)p1;
    const ERROR_INFO * e2 = (const ERROR_INFO*)p2;

    return e1->number - e2->number;
}

HRESULT CheckHR(HRESULT hr)
{
    if (hr == S_OK)
        return S_OK;

    HRESULT hr2;
    CComPtr<IErrorInfo> err;
    CComBSTR str;
    GUID g = GUID_NULL;

    if ((hr2 = GetErrorInfo( 0, &err)) == S_OK) {
        if (FAILED(hr2 = err->GetGUID( &g)))
            g = GUID_NULL;
    }
    if (SUCCEEDED(hr2) && err != NULL && SUCCEEDED(err->GetDescription(&str)) &&
        HRESULT_FACILITY(hr) == FACILITY_ITF
        && g == IID_IALink) { // One of our errors

        ERROR_INFO match, *found;
        match.number = (short)(HRESULT_CODE(hr));
        found = (ERROR_INFO*)bsearch( &match, errorInfo, ERR_COUNT, sizeof(ERROR_INFO), MatchNumber);
        ASSERT(found);
        if (found) {
            ShowFormattedError((int)(found - errorInfo), str);
        } else {
            SetErrorInfo(0L, err); // Put it back
            return hr;
        }
    } else {
        SetErrorInfo(0L, err); // Put it back
        return hr;
    }

    return S_OK;
}

__forceinline void putOpcode( BYTE **temp, ILCODE code) {
    if ((code & 0xFF00) == 0xFF00) {
        **temp = (BYTE)(code & 0xFF);
        *temp += 1;
    } else {
        **temp = (BYTE)((code >> 8) & 0xFF);
        *temp += 1;
        **temp = (BYTE)(code & 0xFF);
        *temp += 1;
    }
}

/*
 * Generates the IL for the __EntryPoint method, that does a tail call into the real main
 */
HRESULT MakeMain(ICeeFileGen *pFileGen, HCEEFILE file, HCEESECTION ilSection, IMetaDataEmit *pEmit,
                 mdMethodDef tkMain, mdMemberRef tkEntryPoint, int iArgs)
{
    HRESULT hr;
    BYTE *buffer = NULL;
    BYTE *temp = NULL;
    BYTE code[64];
    BYTE len = 0;
    DWORD rva = 0;

    ASSERT(iArgs < 4); // How'd we get so many args!

    // Generate the IL and Calculate the size
    temp = code;
    if (iArgs > 0) {
        putOpcode( &temp, CEE_LDARG_0);
        if (iArgs > 1) {
            putOpcode( &temp, CEE_LDARG_1);
        }
    }
    putOpcode( &temp, CEE_TAILCALL);
    putOpcode( &temp, CEE_CALL);
    SET_UNALIGNED_VAL32(temp, tkEntryPoint);
    temp += sizeof(tkEntryPoint);
    putOpcode( &temp, CEE_RET);

    len = (BYTE)(temp - code);

    if (FAILED(hr = pFileGen->GetSectionBlock(ilSection, len + 1, 1, (void**)&buffer))) // +1 for header
        return hr;

    temp = buffer;
    *temp = ((len << 2) | CorILMethod_TinyFormat);
    temp++;
    memcpy(temp, code, len);

    if (FAILED(hr = pFileGen->ComputeSectionOffset(ilSection, (char*)buffer, (UINT*)&rva)))
        return hr;
    if (FAILED(hr = pFileGen->GetMethodRVA(file, rva, &rva)))
        return hr;
    return pEmit->SetMethodProps(tkMain, mdStatic | mdPrivateScope, rva, miManaged | miIL);
}

// *pwszFileName is a possibly relative path to be interpreted relative to wszOldBase. If it
// is a relative path, then converted it to a path, relative if possible, that is relative to 
// newBase. Otherwise, leave it unchanged.
void RerelativizeFileName(LPWSTR * pwszFileName, LPCWSTR wszOldBase, LPCWSTR wszNewBase)
{
    WCHAR wszNewFileName[MAX_PATH];
    WCHAR wszCanonicalFileName[MAX_PATH];

    // Only need to do work if the path is relative.
    if (! PathIsRelativeW(*pwszFileName))
        return;

    // Get directory part of old  base.
	if (! PathRemoveFileSpecW( wcscpy(wszCanonicalFileName, wszOldBase)))
        return;

    // Make absolute path based on the relative path and the old directory.
    if (! PathCombineW(wszNewFileName, wszCanonicalFileName, *pwszFileName))
        return;

    // Canonicalize it.
    if (! PathCanonicalizeW(wszCanonicalFileName, wszNewFileName))
        return;
    
    // Relativize it based on the new base
    if (! PathRelativePathToW(wszNewFileName, wszNewBase, FILE_ATTRIBUTE_NORMAL, wszCanonicalFileName, FILE_ATTRIBUTE_NORMAL))
        return; // can't make relative, don't want to use the absolute path, so give up.

    // Allocate new buffer.
    size_t cch = wcslen(wszNewFileName);
    if (!cch)
        return;
    WCHAR * wszNewBuffer = (WCHAR *) VSAlloc((++cch) * sizeof(WCHAR));
    if (wszNewBuffer == NULL)
        return;

    // Free the old buffer and return the new.
    VSFree(*pwszFileName);
    *pwszFileName = wcscpy(wszNewBuffer, wszNewFileName);
}


HRESULT ImportCA(const BYTE * pvData, DWORD cbSize, AssemblyOptions opt, mdAssembly assemID, IALink * pLinker)
{
    // Make sure that data is in the correct format. Check the prolog, then
    // move beyond it.
    if (cbSize < sizeof(WORD) || GET_UNALIGNED_VAL16(pvData) != 1)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    pvData += sizeof(WORD);
    cbSize -= sizeof(WORD);
    HRESULT hr = E_INVALIDARG;
    VARIANT svt;
    VariantInit(&svt);

    switch(Options[opt].vt) {
    case VT_BSTR:
    {
        ASSERT(opt != optAssemOS);
        LPWSTR str;
        DWORD  len, cchLen;

        // String is stored as compressed length, UTF8.
        if (*pvData == 0xFF) {
            pvData++;
            cbSize--;
            str = NULL;
        } else {
            PCCOR_SIGNATURE sig = (PCCOR_SIGNATURE)pvData;
            len = CorSigUncompressData(sig);
            pvData = (const BYTE *)sig;
            cchLen = UnicodeLengthOfUTF8((PCSTR)pvData, len) + 1;
            str = new WCHAR[cchLen];
            UTF8ToUnicode((const char*)pvData, len, str, cchLen);
            str[cchLen - 1] = L'\0';
            pvData += len;
            cbSize -= len;
        }
        if (str && *str && Options[opt].flag & 0x10) {
            // this option is a filename, so convert from being relative to the source,
            // to being relative to the destination
            RerelativizeFileName(&str, g_szCopyAssembly, g_szAssemblyFile);
            AddBinaryFile(str);
        }

        V_VT(&svt) = VT_BSTR;
        V_BSTR(&svt) = SysAllocString(str);
        hr = pLinker->SetAssemblyProps( assemID, assemID, opt, svt);
        VSFree(str);
        break;
    }
    case VT_BOOL:
        if (cbSize < sizeof(BYTE))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        V_VT(&svt) = VT_BOOL;
        V_BOOL(&svt) = (*(BYTE*)pvData) ? VARIANT_TRUE : VARIANT_FALSE;
        hr = pLinker->SetAssemblyProps( assemID, assemID, opt, svt);
        cbSize -= sizeof(BYTE);
        pvData += sizeof(BYTE);
        break;

    case VT_UI4:
        if (cbSize < sizeof(ULONG))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        V_VT(&svt) = VT_UI4;
        V_UI4(&svt) = GET_UNALIGNED_VAL32(pvData);
        hr = pLinker->SetAssemblyProps( assemID, assemID, opt, svt);
        cbSize -= sizeof(ULONG);
        pvData += sizeof(ULONG);
    }

    VariantClear(&svt);

    return hr;
}

void AddVariantToList( VARIANT &var, VARIANT &val)
{
    if (V_VT(&var) == VT_EMPTY)
        var = val;
    else if (V_VT(&var) == VT_BYREF)
        V_BYREF(&var) = new list<VARIANT>(val, (list<VARIANT>*)V_BYREF(&var));
    else {
        list<VARIANT> * temp = new list<VARIANT>(var, NULL);
        VariantClear(&var);
        V_VT(&var) = VT_BYREF;
        V_BYREF(&var) = new list<VARIANT>(val, temp);
    }
}

HRESULT CopyAssembly(mdAssembly assemID, IALink * pLinker)
{
    // Get the parent assembly info, and copy into this one
    HRESULT hr = S_OK;
    mdToken tkParent = mdTokenNil;
    CComPtr<IMetaDataImport> pImp;
    CComPtr<IMetaDataAssemblyImport> pAsm;
    mdAssembly tkAssembly = mdAssemblyNil;
    DWORD dwCount = 0;

    ASSERT(g_szCopyAssembly != NULL);
    AddBinaryFile(g_szCopyAssembly);

    if (FAILED(hr = pLinker->ImportFile( g_szCopyAssembly, NULL, FALSE, &tkParent, &pAsm, &dwCount)))
        return hr;
    if (!pAsm || dwCount == 0) {
        ShowErrorIdString(ERR_ParentNotAnAssembly, ERROR_ERROR, g_szCopyAssembly);
        return S_FALSE;
    }
    if (FAILED(hr = pLinker->GetScope( assemID, tkParent, 0, &pImp)))
        return hr;
    if (FAILED(hr = pAsm->GetAssemblyFromScope(&tkAssembly)))
        return hr;

    if (SUCCEEDED(hr) && pAsm && pImp) {
        ASSEMBLYMETADATA md;
        WCHAR buffer[1024];
        DWORD dwAlgId = 0;
        VARIANT svt;
        VariantInit(&svt);
        memset( &md, 0, sizeof(md));

        // Get everything
        if (SUCCEEDED(hr = pAsm->GetAssemblyProps( tkAssembly, NULL, NULL, &dwAlgId, NULL, 0, NULL, &md, NULL))) {
            if (md.ulOS)
                md.rOS = new OSINFO[md.ulOS];
            if (md.ulProcessor)
                md.rProcessor = new ULONG[md.ulProcessor];
            if (md.cbLocale)
                md.szLocale = new WCHAR[md.cbLocale + 1];

            if (SUCCEEDED(hr = pAsm->GetAssemblyProps( tkAssembly, NULL, NULL, &dwAlgId, NULL, 0, NULL, &md, NULL))) {

                if (V_VT(&g_Options[optAssemOS]) == VT_EMPTY) {
                    ASSERT(Options[optAssemOS].flag & 0x04);
                    for (ULONG o = 0; o < md.ulOS && SUCCEEDED(hr); o++) {
                        swprintf(buffer, L"%d.%d.%d", md.rOS[o].dwOSPlatformId, md.rOS[o].dwOSMajorVersion, md.rOS[o].dwOSMinorVersion);
                        V_VT(&svt) = VT_BSTR;
                        V_BSTR(&svt) = SysAllocString(buffer);
                        if (SUCCEEDED(hr = pLinker->SetAssemblyProps( assemID, assemID, optAssemOS, svt)))
                            _optionused::GetOptionUsed()->set(optAssemOS);
                        VariantClear(&svt);
                    }
                }

                if (V_VT(&g_Options[optAssemProcessor]) == VT_EMPTY && SUCCEEDED(hr)) {
                    ASSERT(Options[optAssemProcessor].flag & 0x04);
                    V_VT(&svt) = VT_UI4;
                    for (ULONG p = 0; p < md.ulProcessor && SUCCEEDED(hr); p++) {
                        V_UI4(&svt) = md.rProcessor[p];
                        if (SUCCEEDED(hr = pLinker->SetAssemblyProps( assemID, assemID, optAssemProcessor, svt)))
                            _optionused::GetOptionUsed()->set(optAssemProcessor);
                    }
                    VariantClear(&svt);
                }

                if (V_VT(&g_Options[optAssemLocale]) == VT_EMPTY &&
                    SUCCEEDED(hr) && md.cbLocale > 0 &&
                    md.szLocale != NULL && md.szLocale[0] != L'\0') {
                    V_VT(&svt) = VT_BSTR;
                    V_BSTR(&svt) = SysAllocString(md.szLocale);
                    if (SUCCEEDED(hr = pLinker->SetAssemblyProps( assemID, assemID, optAssemLocale, svt)))
                        _optionused::GetOptionUsed()->set(optAssemLocale);
                    VariantClear(&svt);
                }

                if (V_VT(&g_Options[optAssemVersion]) == VT_EMPTY && SUCCEEDED(hr)) {
                    swprintf(buffer, L"%hu.%hu.%hu.%hu", md.usMajorVersion, md.usMinorVersion, md.usBuildNumber, md.usRevisionNumber);
                    V_VT(&svt) = VT_BSTR;
                    V_BSTR(&svt) = SysAllocString(buffer);
                    if (SUCCEEDED(hr = pLinker->SetAssemblyProps( assemID, assemID, optAssemVersion, svt)))
                        _optionused::GetOptionUsed()->set(optAssemVersion);
                    VariantClear(&svt);
                }

            }

            if (V_VT(&g_Options[optAssemAlgID]) == VT_EMPTY && SUCCEEDED(hr)) {
                V_VT(&svt) = VT_UI4;
                V_UI4(&svt) = dwAlgId;
                if (SUCCEEDED(hr = pLinker->SetAssemblyProps( assemID, assemID, optAssemAlgID, svt)))
                    _optionused::GetOptionUsed()->set(optAssemAlgID);
                VariantClear(&svt);
            }

            if (md.rOS)
                VSFree(md.rOS);
            if (md.rProcessor)
                VSFree(md.rProcessor);
            if (md.szLocale)
                VSFree(md.szLocale);
        }
    }

    for (int i = 0; i < optLastAssemOption && SUCCEEDED(hr); i++) {
        if (V_VT(&g_Options[i]) != VT_EMPTY ||
            // It's overriden by the command-line

            i == optAssemConfig || i == optAssemFlags ||
            // We don't copy these

            i == optAssemOS || i == optAssemProcessor ||
            i == optAssemLocale || i == optAssemVersion ||
            i == optAssemAlgID) {
            // These come from bits, and are already done

            continue;

        } else {
            const void *pvData = NULL;
            DWORD   cbData = 0;

            // This only works for non-AllowMulti CAs
            ASSERT((Options[i].flag & 0x04) == 0);
            if (S_OK == (hr = pImp->GetCustomAttributeByName( tkAssembly, Options[i].CA, &pvData, &cbData))) {
                AssemblyOptions o = (i == optAssemSatelliteVer ? optAssemVersion : (AssemblyOptions)i);
                if (SUCCEEDED(hr = ImportCA( (const BYTE *)pvData, cbData, o, assemID, pLinker)))
                    _optionused::GetOptionUsed()->set(o);
            }
            // hr == S_FLASE means the CA doesn't exist
        }
    }

    return hr;
}

HRESULT SetAssemblyOptions(mdAssembly assemID, IALink *pLinker)
{
    HRESULT hr = S_OK;
    // Set the options, after importing files so they override the CAs
    for (int i = 0; i < optLastAssemOption && SUCCEEDED(hr); i++) {
        if (V_VT(&g_Options[i]) != VT_EMPTY) {
            if (V_VT(&g_Options[i]) == VT_BYREF) {
                ASSERT(Options[i].flag & 0x04 && V_BYREF(&g_Options[i]) != NULL);

                for (list<VARIANT> * current = (list<VARIANT>*)V_BYREF(&g_Options[i]);  // Init
                    current != NULL && SUCCEEDED(hr);                                       // Condition
                    current = current->next) {                                              // Increment
                    hr = pLinker->SetAssemblyProps( assemID, assemID, Options[i].opt, current->arg);
                }
            } else {
                if (i == optAssemKeyFile)
                    AddBinaryFile(V_BSTR(&g_Options[i]));

                hr = pLinker->SetAssemblyProps( assemID, assemID, Options[i].opt, g_Options[i]);
                if (hr == S_FALSE) {
                    ASSERT(!_optionused::GetOptionUsed()->get(Options[i].opt)); // We shouldn't have copied this from the template
                    ShowErrorIdString(WRN_OptionConflicts, ERROR_WARNING, Options[i].LongName);
                }
            }
        }
    }

    // Cleanup our lists
    for (int i = 0; i < optLastAssemOption; i++) {
        if (V_VT(&g_Options[i]) != VT_EMPTY && V_VT(&g_Options[i]) == VT_BYREF) {
            ASSERT(Options[i].flag & 0x04 && V_BYREF(&g_Options[i]) != NULL);

            list<VARIANT> * current = (list<VARIANT>*)V_BYREF(&g_Options[i]);
            while (current != NULL) {
                list<VARIANT> * temp = current->next;
                current->next = NULL;
                VariantClear(&current->arg);
                VSFree(current);
                current = temp;
            }
            V_BYREF(&g_Options[i]) = NULL;
        }
    }

    return hr;
}


void AddBinaryFile(LPCWSTR filename)
{
}

