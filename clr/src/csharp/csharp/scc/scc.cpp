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
// File: scc.cpp
//
// The command line driver for the C# compiler.
// ===========================================================================

#include "stdafx.h"
#include "msgsids.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <cor.h>
#include <mscoree.h>
#include <palstartup.h>

#include "scciface.h"
#include "atl.h"
#include "sscli_version.h"
#include "unilib.h"
#include "posdata.h"


#ifdef TRACKMEM
size_t memMax = 0, memCur = 0;
#endif

bool g_fullPaths = false;
bool g_firstInputSet = true;
bool g_fUTF8Output = false;
HINSTANCE g_hinstMessages;

void __cdecl PrintReproInfo(unsigned id, ...);


/* Compiler error numbers.
 */
#define ERRORDEF(num, level, name, strid) name,

enum ERRORIDS {
    ERR_NONE,
    #include "errors.h"
    ERR_COUNT
};

#undef ERRORDEF

/*
 * Information about each error or warning.
 */
struct ERROR_INFO {
    short number;       // error number
    short level;        // warning level; 0 means error
    int   resid;        // resource id.
};

#define ERRORDEF(num, level, name, strid) {num, level, strid},

static ERROR_INFO errorInfo[ERR_COUNT] = {
    {0000, -1, 0},          // ERR_NONE - no error.
    #include "errors.h"
};

#undef ERRORDEF

/*
 * Help strings for each of the COF_GRP flags
 * NOTE: keep this in the same order as the flags
 */
static DWORD helpGroups[] = {
    (DWORD) -1,
    IDS_OD_GRP_OUTPUT,
    IDS_OD_GRP_INPUT,
    IDS_OD_GRP_RES,
    IDS_OD_GRP_CODE,
    IDS_OD_GRP_ERRORS,
    IDS_OD_GRP_LANGUAGE,
    IDS_OD_GRP_MISC,
    IDS_OD_GRP_ADVANCED
};
// This is the amount to shift the flags to go from COF_GRP_* to a number
#define GRP_SHIFT   12

/*
 * Help strings for each of the COF_ARG flags
 * NOTE: keep this in the same order as the flags
 */
static DWORD helpArgs[] = {
    (DWORD) -1,
    IDS_OD_ARG_FILELIST,
    IDS_OD_ARG_FILE,
    IDS_OD_ARG_SYMLIST,
    IDS_OD_ARG_WILDCARD,
    IDS_OD_ARG_TYPE,
    IDS_OD_ARG_RESINFO,
    IDS_OD_ARG_WARNLSIT,
    IDS_OD_ARG_ADDR,
    IDS_OD_ARG_NUMBER,
    IDS_OD_ARG_DEBUGTYPE,
};
// This is the amount to shift the flags to go from COF_GRP_* to a number
#define ARG_SHIFT   20

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
 * Like WideCharToMultiByte, but translates to the console code page. Returns length, 
 * INCLUDING null terminator.
 */
int WideCharToConsole(LPCWSTR wideStr, LPSTR lpBuffer, int nBufferMax)
{
    if (g_fUTF8Output) {
        if (nBufferMax == 0) {
            return UTF8LengthOfUnicode(wideStr, (int)wcslen(wideStr)) + 1; // +1 for nul terminator
        }
        else {
            int cchConverted = NULL_TERMINATED_MODE;
            return UnicodeToUTF8 (wideStr, &cchConverted, lpBuffer, nBufferMax);
        }

    }
    else {
        return WideCharToMultiByte(GetConsoleOutputCP(), 0, wideStr, -1, lpBuffer, nBufferMax, 0, 0);
    }
}

/*
 * Convert Unicode string to Console ANSI string allocated with VSAlloc
 */
LPSTR WideToConsole(LPCWSTR wideStr)
{
    int cch = WideCharToConsole(wideStr, NULL, 0);
    LPSTR ansiStr = (LPSTR) VSAlloc(cch);
    WideCharToConsole(wideStr, ansiStr, cch);
    return ansiStr;
}

/*
 * Wrapper for LoadString that works on Win9x.
 */
int LoadStringWide(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax)
{
    int cch = LoadStringW(hInstance, uID, lpBuffer, nBufferMax);
    if (cch == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
        // Use ANSI version:
        CHAR * lpBufferAnsi = (CHAR *) _alloca(nBufferMax * 2);
        int cchAnsi = LoadStringA(hInstance, uID, lpBufferAnsi, nBufferMax);
        if (cchAnsi) {
            cch = MultiByteToWideChar(CP_ACP, 0, lpBufferAnsi, cchAnsi, lpBuffer, nBufferMax - 1);
            lpBuffer[cch] = 0; // zero terminate.
        }
        else 
            return 0;
    }
    return cch;
}

/*
 * Wrapper for LoadString that converts to the console code page.
 */
int LoadConsoleString(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int nBufferMax)
{
    WCHAR * lpWideString = (WCHAR *) _alloca(sizeof(WCHAR) * nBufferMax);

    int cch = LoadStringWide(hInstance, uID, lpWideString, nBufferMax);
    if (cch == 0)
        return 0;

    return WideCharToConsole(lpWideString, lpBuffer, nBufferMax) - 1;
}


/*
 * Print out the given text, with word-wraping based on screen size
 * and indenting the wrapped lines with pszIndent
 * ASSUMES there are NO newlines in pszText
 * ALWAYS finishes with a newline
 */
void PrettyPrint(LPCBYTE pszText, int cchIndent)
{
    ASSERT(cchIndent >= 0 && cchIndent <= 100);
    static const char pszIndent[] = "                                                                                                    ";

    print("%.*s", (int)cchIndent, pszIndent);
    print("%s\n", pszText);
}

DEFINE_GUID(IID_ICSCommandLineCompilerHost,
0xBD7CF4EF, 0x1821, 0x4719, 0xAA, 0x60, 0xDF, 0x81, 0xB9, 0xEA, 0xFA, 0x01);

/*
 * The compiler host class.
 */
class CompilerHost: public ICSCompilerHost, public ICSCommandLineCompilerHost
{
    ICSCompiler     *pCompiler;
    static  CHAR    m_szRelatedWarning[256], m_szRelatedError[256];
    UINT            m_uiCodePage;
public:
    CompilerHost () : pCompiler(NULL)
    {
        LoadConsoleString (g_hinstMessages, IDS_RELATEDWARNING, m_szRelatedWarning, sizeof (m_szRelatedWarning));
        LoadConsoleString (g_hinstMessages, IDS_RELATEDERROR, m_szRelatedError, sizeof (m_szRelatedError));
        m_uiCodePage = 0;
    }
    ~CompilerHost () {}

    void        SetCompiler (ICSCompiler *pComp) { pCompiler = pComp; }
    void        SetCodePage (UINT uiCodePage) { m_uiCodePage = uiCodePage; }

    static  void        ReportError (ICSError *pError);

    /* We don't support IUnknown in a standard way because we don't have to! */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) {
        if (riid == IID_ICSCommandLineCompilerHost)
        {
            *((ICSCommandLineCompilerHost**)ppvObject) = (ICSCommandLineCompilerHost*) this;
            return S_OK;
        }
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() {return 1;}
    STDMETHODIMP_(ULONG) Release() {return 1;}

    // ICSCompilerHost
    STDMETHOD(ReportErrors)(ICSErrorContainer *pErrors);
    STDMETHOD(GetSourceModule)(PCWSTR pszFile, ICSSourceModule **ppModule);
    STDMETHOD_(void, OnConfigChange)(long iOptionID, VARIANT vtValue) {}
    STDMETHOD_(void, OnCatastrophicError)(BOOL fException, DWORD dwException, void *pAddress);

    // ICSCommandLineCompilerHost
    STDMETHOD_(void, NotifyBinaryFile)(LPCWSTR pszFileName);
    STDMETHOD_(void, NotifyMetadataFile)(LPCWSTR pszFileName, LPCWSTR pszCorSystemDirectory);

};

CHAR    CompilerHost::m_szRelatedWarning[256];
CHAR    CompilerHost::m_szRelatedError[256];






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
    ASSERT(i);  // String should be there.

    if (i) {
    print("%s", buffer);
    if (newline)
        print("\n");
    }
}

static bool hadError = false;

/*
 * General error display routine -- report error or warning.
 * kind indicates fatal, warning, error
 * inputfile can be null if none, and line <= 0 if no location is available.
 */
void ShowError(int id, ERRORKIND kind, PCWSTR inputfile, int line, int col, int extent, LPSTR text)
{
    const CHAR *errkind;
    CHAR buffer[MAX_PATH];

    switch (kind) {
    case ERROR_FATAL:
        hadError = true;
        errkind = "fatal error"; break;
    case ERROR_ERROR:
        hadError = true;
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
        if (WideCharToConsole(inputfile, buffer, sizeof(buffer))) {

                print("%s", buffer);
        }



        if (line > 0) {
            print("(%d,%d)", line, col);
        }

        print(": ");
    }

    // Print "error ####" -- only if not a related symbol location (id == -1)
    if (id != -1)
    {
        print("%s CS%04d: ", errkind, id);
    }

    // Print error text. (This will indent subsequent lines by 8 characters and terminate with a newline)
    if (text && text[0])
        PrettyPrint((LPCBYTE)text, 8);
    else
        print("\n");
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

////////////////////////////////////////////////////////////////////////////////
// CompilerHost::ReportError

void CompilerHost::ReportError (ICSError *pError)
{
    PSTR        szBuffer = NULL;
    long        iErrorId;
    ERRORKIND   iKind;
    PCWSTR      pszText;

    if (FAILED (pError->GetErrorInfo (&iErrorId, &iKind, &pszText)))
    {
        ShowErrorId (FTL_NoMemory, ERROR_FATAL);        // REVIEW:  This is hokey...
        return;
    }

    // Convert the text of the error to multibyte for display...
    if (pszText != NULL) {
        szBuffer = WideToConsole (pszText);
    }

    long        iLocations = 0;

    // Get the location(s) if any.
    if (SUCCEEDED (pError->GetLocationCount (&iLocations)) && iLocations > 0)
    {
        PCWSTR      pszFileName = NULL;
        PSTR        pszText = szBuffer;
        POSDATA     posStart;

        // For each location, display the error.  For locations past the first one,
        // display the "related symbol to previous error" as the text
        for (long i=0; i<iLocations; i++)
        {
            if (SUCCEEDED (pError->GetLocationAt (i, &pszFileName, &posStart, NULL)))
            {
                int iLine = posStart.IsEmpty() ? -1 : (int) posStart.iLine + 1;

                ShowError (iErrorId, iKind, pszFileName, iLine, posStart.iChar + 1, 0, pszText);
                iErrorId = -1;
                pszText = (iKind == ERROR_WARNING) ? m_szRelatedWarning : m_szRelatedError;
            }
        }
    }
    else
    {
        ShowError (iErrorId, iKind, NULL, -1, 0, 0, szBuffer);
    }
    if (szBuffer)
        VSFree(szBuffer);
}

////////////////////////////////////////////////////////////////////////////////
// CompilerHost::ReportErrors
//
// This is called from the compiler when errors are available for "display".

STDMETHODIMP CompilerHost::ReportErrors (ICSErrorContainer *pErrors)
{
    long    iErrors;

    if (FAILED (pErrors->GetErrorCount (NULL, NULL, NULL, &iErrors)))
    {
        ShowErrorId (FTL_NoMemory, ERROR_FATAL);            // REVIEW:  This is hokey...
        return S_OK;
    }

    for (long i=0; i<iErrors; i++)
    {
        CComPtr<ICSError>   spError;

        if (FAILED (pErrors->GetErrorAt (i, &spError)))
        {
            ShowErrorId (FTL_NoMemory, ERROR_FATAL);        // REVIEW:  This is hokey...
            return S_OK;
        }

        ReportError (spError);
    }

    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
// CompilerHost::OnCatastrophicError

STDMETHODIMP_(void) CompilerHost::OnCatastrophicError (BOOL fException, DWORD dwException, void *pAddress)
{
    ShowErrorIdString (FTL_InternalError, ERROR_FATAL, E_FAIL);
}



////////////////////////////////////////////////////////////////////////////////
// CheckResult
//
// Helper function for reporting errors for HRESULT values coming from COM
// functions whose failure is reported generically (i.e. compiler init failed
// unexpectedly)

inline HRESULT CheckResult (HRESULT hr)
{
    if (FAILED (hr))
        ShowErrorId (ERR_InitError, ERROR_FATAL);
    return hr;
}



////////////////////////////////////////////////////////////////////////////////
// CSourceText -- implements ICSSourceText for a text file

class CSourceText : public ICSSourceText
{
private:
    PWSTR       m_pszText;
    PWSTR       m_pszName;
    LONG        m_iRef;


public:
    CSourceText () : m_pszText(NULL), m_pszName(NULL), m_iRef(0) {}
    ~CSourceText () { if (m_pszText) VSFree (m_pszText); if (m_pszName) VSFree (m_pszName); }

    HRESULT     Initialize (PCWSTR pszFileName, UINT uiCodePage = 0);

    // IUnknown
    STDMETHOD_(ULONG, AddRef)() { return InterlockedIncrement (&m_iRef); }
    STDMETHOD_(ULONG, Release)() { ULONG i = InterlockedDecrement (&m_iRef); if (i == 0) delete this; return i; }
    STDMETHOD(QueryInterface)(REFIID riid, void **ppObj) { return E_NOINTERFACE; }

    // ICSSourceText
    STDMETHOD(GetText)(PCWSTR *ppszText, POSDATA *pposEnd) { *ppszText = m_pszText; if (pposEnd != NULL) { pposEnd->iLine = 0; pposEnd->iChar = 0; } return S_OK; }
    STDMETHOD(GetName)(PCWSTR *ppszName) { *ppszName = m_pszName; return S_OK; }

    void        OutputToReproFile();
};

////////////////////////////////////////////////////////////////////////////////
// OpenTextFile -- opens a file, returns a handle.  Uses the ANSI version if on
// Win9x...

HANDLE OpenTextFile (PCWSTR pszFileName)
{
    HANDLE hFile;

    hFile = W_CreateFile (pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    return hFile;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceText::Initialize

HRESULT CSourceText::Initialize (PCWSTR pszFileName, UINT uiCodePage)
{
    // Open the file
    HANDLE      hFile = OpenTextFile (pszFileName);
    enum        eTextType { txUnicode, txSwappedUnicode, txUTF8, txASCII } textType;

    if (hFile == INVALID_HANDLE_VALUE || hFile == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD   dwSizeHigh, dwSize, dwRead, dwStartChar;
    long    i;

    // Find out how big it is
    dwSize = GetFileSize (hFile, &dwSizeHigh);
    if (dwSizeHigh != 0)
    {
        // Whoops!  Too big.
        CloseHandle (hFile);
        return E_FAIL;
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
        CloseHandle (hFile);
        VSFree (pszTemp);
        return E_FAIL;
    }

    CloseHandle (hFile);

    // Determine Character encoding of file
    dwStartChar = 0;
    switch( pszTemp[0] ) {
    case '\xFE':
    case '\xFF':
        textType = ( pszTemp[0] == '\xFF' ? txUnicode : txSwappedUnicode );
        if ( dwSize < 2 || dwSize & 1 || pszTemp[1] != ( pszTemp[0] == '\xFE' ? '\xFF' : '\xFE') )
            textType = txASCII;     // Not a Unicode File
        else
            dwStartChar = 2;
        break;
    case '\xEF':
        textType = txUTF8;
        if ( dwSize < 3 || pszTemp[1] != '\xBB' || pszTemp[2] != '\xBF')
            textType = txASCII;     // Not a UTF-8 File
        else
            dwStartChar = 3;
        break;
    case 'M':
        if (dwSize > 1 && pszTemp[1] == 'Z') {
            // 'M' 'Z' indicates a DLL or EXE; give good error and fail.
            ShowErrorIdString(ERR_BinaryFile, ERROR_ERROR, pszFileName);
            VSFree(pszTemp);
            return E_FAIL;
        }
        else
            textType = txASCII;

        break;
        
    default:
        textType = txASCII;
        break;
    }

    // handle UTF8 and UNICODE codepage specifiers
    if (textType == txASCII) {
        if (uiCodePage == CP_UTF8)
            textType = txUTF8;
        else if (!(dwSize & 1) && (uiCodePage == CP_WINUNICODE || uiCodePage == CP_UNICODE))
            textType = txUnicode;
        else if (!(dwSize & 1) && uiCodePage == CP_UNICODESWAP)
            textType = txSwappedUnicode;
        else if (uiCodePage == 0) {
            // Attempt to auto-detect if it is UTF8
            int len = NULL_TERMINATED_MODE;
            if (!(dwSize & 0x80000000))
                len = dwSize;
                // otherwise, go with the active code-page
                uiCodePage = GetACP();
        }
    }


    switch( textType ) {
    case txUnicode:
        // No conversion needed
        m_pszText = (PWSTR)VSAlloc ((dwSize - dwStartChar + 2) * sizeof (CHAR)); // 2 for WCHAR NULL
        memcpy( m_pszText, pszTemp + dwStartChar, dwSize - dwStartChar);
        m_pszText[ (dwSize - dwStartChar) / 2 ] = 0;
        VSFree (pszTemp);
        break;
    case txSwappedUnicode:
        // swap the bytes please
        m_pszText = (PWSTR)VSAlloc ((dwSize - dwStartChar + 2) * sizeof (CHAR)); // 2 for WCHAR NULL
        if (m_pszText == NULL)
        {
            VSFree (pszTemp);
            return E_OUTOFMEMORY;
        }
        // We know this file has an even number of bytes because of the earlier tests
        ASSERT( (dwSize & 1) == 0);
        _swab( pszTemp + dwStartChar, (PSTR)m_pszText, dwSize - dwStartChar);   // Swap all possible bytes
        m_pszText[ (dwSize - dwStartChar) / 2 ] = 0;
        VSFree (pszTemp);
        break;
    case txUTF8:
        SetLastError(0);
        // Calculate the size needed
        i = UnicodeLengthOfUTF8 (pszTemp + dwStartChar, dwSize - dwStartChar);
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        m_pszText = (PWSTR)VSAlloc ((i + 1) * sizeof (WCHAR));
        if (m_pszText == NULL)
        {
            VSFree (pszTemp);
            return E_OUTOFMEMORY;
        }

        // Convert, remembering number of characters
        i = UTF8ToUnicode (pszTemp + dwStartChar, dwSize - dwStartChar, m_pszText, (i + 1));
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        m_pszText[i] = 0;

        VSFree (pszTemp);
        break;
    case txASCII:
        if (dwSize == 0) { // Special case for completely empty files
            m_pszText = (PWSTR)VSAlloc ( sizeof(WCHAR));
            VSFree (pszTemp);
            m_pszText[0] = 0;
            break;
        }

        // Calculate the size needed
        i = MultiByteToWideChar (uiCodePage, 0, pszTemp, dwSize, NULL, 0);
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        m_pszText = (PWSTR)VSAlloc ((dwSize + 1) * sizeof (WCHAR));
        if (m_pszText == NULL)
        {
            VSFree (pszTemp);
            return E_OUTOFMEMORY;
        }

        // Convert, remembering number of characters
        i = MultiByteToWideChar (uiCodePage, 0, pszTemp, dwSize, m_pszText, dwSize + 1);
        if (i == 0 && GetLastError() != ERROR_SUCCESS)
        {
            VSFree (pszTemp);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        m_pszText[i] = 0;

        VSFree (pszTemp);
        break;
    }

    // Remember the file name as given to us
    m_pszName = (PWSTR)VSAlloc ((wcslen (pszFileName) + 1) * sizeof (WCHAR));
    if (m_pszName == NULL)
        return E_OUTOFMEMORY;

    wcscpy (m_pszName, pszFileName);

    return S_OK;
}


/*
 * Given a file name (fully qualified by the compiler), load the source text into a
 * buffer wrapped by an ICSSourceText implementation, and turn around and ask the
 * compiler to create an ICSSourceModule object for it.
 */
STDMETHODIMP CompilerHost::GetSourceModule (PCWSTR pszFileName, ICSSourceModule **ppModule)
{
    *ppModule = NULL;

    CSourceText     *pSourceText = new CSourceText();

    if (pSourceText == NULL)
        return E_OUTOFMEMORY;

    HRESULT     hr;

    if (FAILED (hr = pSourceText->Initialize (pszFileName, m_uiCodePage)) ||
        FAILED (hr = pCompiler->CreateSourceModule (pSourceText, ppModule)))
    {
        delete pSourceText;
    }

    return hr;
}

/*
 * Get the file version as a string.
 */
static void GetFileVersion(HINSTANCE hinst, CHAR *szVersion)
{
    strcpy(szVersion, SSCLI_VERSION_STR);
}

// Get version of the command language runtime.
typedef HRESULT (__stdcall *pfnGetCORVersion)(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength);
static void GetCLRVersion(CHAR *szVersion, int maxBuf)
{
    ASSERT(maxBuf > 50);         // don't be silly!
    WCHAR ver[MAX_PATH];
    DWORD dwLen;
    
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
            WideCharToConsole(ver + 1, szVersion, maxBuf);
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

/*
 * Print single option name and description
 */
HRESULT PrintOption( DWORD CurrentGrp, LPCWSTR pszLongSwitch, LPCWSTR pszShortSwitch, LPCSTR pszDescription, DWORD dwFlags)
{
    PSTR    pszLongSwitchAnsi, pszShortSwitchAnsi;
    char    szBuf[2048];

    // Never show Hidden options
    if (dwFlags & COF_HIDDEN)
        return S_FALSE;

    // Only do the current group's options or ungrouped options (for the misc. group)
    if (((dwFlags & COF_GRP_MASK) != CurrentGrp) &&
        (((dwFlags & COF_GRP_MASK) != 0) || (CurrentGrp != COF_GRP_MISC)))
        return S_FALSE;

    pszLongSwitchAnsi = WideToConsole(pszLongSwitch);
    if (pszShortSwitch)
        pszShortSwitchAnsi = WideToConsole(pszShortSwitch);
    else
        pszShortSwitchAnsi = NULL;
    if (dwFlags & COF_BOOLEAN)
        sprintf (szBuf, "/%s[+|-]", pszLongSwitchAnsi);
    else {
        if (dwFlags & COF_ARG_MASK) {
            CHAR pszArgStr[32];
            if (0 == LoadConsoleString( g_hinstMessages, helpArgs[(dwFlags & COF_ARG_MASK) >> ARG_SHIFT], pszArgStr, lengthof(pszArgStr))) {
                if (dwFlags & COF_ARG_NOCOLON)
                    pszArgStr[0] = L'\0';
                else
                    strcpy(pszArgStr, "<str>");
            }
            if (dwFlags & COF_ARG_NOCOLON)
                sprintf (szBuf, "%s%s", pszLongSwitchAnsi, pszArgStr);
            else
                sprintf (szBuf, "/%s:%s", pszLongSwitchAnsi, pszArgStr);
        } else {
            if (dwFlags & COF_ARG_NOCOLON)
                sprintf (szBuf, "%s", pszLongSwitchAnsi);
            else
                sprintf (szBuf, "/%s", pszLongSwitchAnsi);
        }
    }

    ASSERT(strlen(szBuf) <= 24);
    print ("%-24s", szBuf);

    // optionally add the short form to the description
    if (pszShortSwitchAnsi) {
        CHAR szShortForm[64];
        LoadConsoleString( g_hinstMessages, IDS_SHORTFORM, szShortForm, lengthof(szShortForm));

        sprintf( szBuf, "%s (%s: /%s)", pszDescription, szShortForm, pszShortSwitchAnsi);
        pszDescription = szBuf;
    }

    // Pretty-print an indented description text
    // Use a 24 character indent just to make things pretty
    PrettyPrint((LPCBYTE)pszDescription, 24);

    VSFree(pszLongSwitchAnsi);
    if (pszShortSwitchAnsi)
        VSFree(pszShortSwitchAnsi);

    return S_OK;
}

/*
 * Print the help text for the compiler.
 */
HRESULT PrintHelp (ICSCompilerConfig *pConfig)
{
    // The help text is numbered 10, 20, 30... so there is
    // ample room to put in additional strings in the middle.

    print( "                      "); // 24 blanks to line up the title
    PrintString(IDS_HELP10, true);      // "Available compiler options:"

    long    iCount;
    HRESULT hr;
    DWORD   iGrp;

    static struct _CmdOptions {
        DWORD   idDesc;
        PCWSTR  pszShort;
        PCWSTR  pszLong;
        DWORD   dwFlags;
    } CmdOptions[] = {
#define CMDOPTDEF( id, s, l, f) {id, s, l, f},
#include "cmdoptdef.h"
    };

    // Iterate through the compiler options and display the appropriate ones
    if (FAILED (hr = CheckResult (pConfig->GetOptionCount (&iCount))))
        return hr;

    for (iGrp = 1; iGrp < lengthof(helpGroups); iGrp++) {

        print( "\n                        "); // 24 blanks to line up the title
        PrintString( helpGroups[iGrp]);

        for (long i=0; i < (long) lengthof(CmdOptions); i++)
        {
            CHAR pszBuffer[1024];

            if (!LoadConsoleString( g_hinstMessages, CmdOptions[i].idDesc, pszBuffer, lengthof(pszBuffer)))
                return E_OUTOFMEMORY;

            if (FAILED(hr = PrintOption(iGrp << GRP_SHIFT, CmdOptions[i].pszLong, CmdOptions[i].pszShort, pszBuffer, CmdOptions[i].dwFlags)))
                return hr;
        }

        for (long i=0; i<iCount; i++)
        {
            PCWSTR  pszLongSwitch, pszShortSwitch, pszDesc;
            PSTR    pszDescAnsi;
            long    iID;
            DWORD   dwFlags;

            if (FAILED (hr = CheckResult (pConfig->GetOptionInfoAtEx (i, &iID, &pszShortSwitch, &pszLongSwitch, NULL, &pszDesc, &dwFlags))))
                return hr;

            pszDescAnsi = WideToConsole(pszDesc);
            PrintOption(iGrp << GRP_SHIFT, pszLongSwitch, pszShortSwitch, pszDescAnsi, dwFlags);
            VSFree(pszDescAnsi);

            if (FAILED(hr))
                return hr;
        }
    }

    print ("\n");

    return S_OK;
}


 
// Remove quote marks from a string. Also, if translate is non-null, any
// characters in translate that are not quotes are converted to the pipe (|) character.
// The translation is done in-place, and the argument is returned.
WCHAR * RemoveQuotes(WCHAR * input, WCHAR * translate = NULL)
{
    WCHAR * pIn, *pOut;
    WCHAR ch;
    bool inQuote;

    if (input == NULL)
        return NULL;

    pIn = pOut = input;
    inQuote = false;
    for (;;) {
        switch (ch = *pIn) {
        case L'\0':
            // End of string. We're done.
            *pOut = L'\0';
            return input;
        case L'\"': 
            inQuote = !inQuote;
            break;
        default:
            if (!inQuote && translate && wcschr(translate, ch)) 
                *pOut++ = L'|';
            else
                *pOut++ = ch;
            
            break;
        }

        ++pIn;
    } 
    
    return input;
}


/*
 * Process the "pre-switches": /nologo, /?, /help, /time, /repro, /fullpaths. If they exist,
 * they are nulled out.
 */
void ProcessPreSwitches(int argc, LPWSTR argv[], bool * noLogo, bool * showHelp, bool * timeCompile, UINT * uiCodePage)
{
    LPWSTR arg;

    *noLogo = *showHelp = *timeCompile = false;

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
            *noLogo = true;
        else if (_wcsicmp(arg, L"?") == 0)
            *showHelp = true;
        else if (_wcsicmp(arg, L"help") == 0)
            *showHelp = true;
        else if (_wcsicmp(arg, L"time") == 0)
            *timeCompile = true;
        else if (_wcsnicmp(arg, L"codepage:", 9) == 0) {
            WCHAR * cp = RemoveQuotes(arg + 9);
            if (*cp == '\0') {
                ShowErrorIdString( ERR_SwitchNeedsString, ERROR_FATAL, arg);
            }
            else {
                int len1, len2 = (int)wcslen(cp);
                if (swscanf( cp, L"%u%n", uiCodePage, &len1) != 1 ||
                    len1 != len2 ||     // Did we read the WHOLE CodePageID?
                    ! (IsValidCodePage( *uiCodePage) || *uiCodePage == CP_UTF8 || *uiCodePage == CP_WINUNICODE)) {
                    ShowErrorIdString( ERR_BadCodepage, ERROR_FATAL, cp);
                }
            }
        }
        else if (_wcsicmp(arg, L"codepage") == 0) {
            ShowErrorIdString( ERR_SwitchNeedsString, ERROR_FATAL, arg);
        }
        else if (_wcsicmp(arg, L"bugreport") == 0) {
            ShowErrorIdString( ERR_NoFileSpec, ERROR_FATAL, arg);
        }
        else if (_wcsicmp(arg, L"utf8output") == 0) {
            g_fUTF8Output = true;
        }
        else if (_wcsicmp(arg, L"fullpaths") == 0)
            g_fullPaths = true;
#ifdef _DEBUG
        else if (_wcsicmp(arg, L"debugbreak") == 0) {
            VSASSERT(0,"DebugBreak option caught");
        }
#endif
        else
            continue;       // Not a recognized argument.

        argv[iArg] = NULL;  // NULL out recognized argument.
    }
}

/*
 * Process a single file name from the command line. Expand wild cards if there
 * are any.
 */
HRESULT ProcessFileName (LPWSTR filename, ICSInputSet *pInput, UINT uiCodePage, BOOL *pfFilesAdded, BOOL bRecurse)
{
    HANDLE hSearch;
    WIN32_FIND_DATAW findData;
    WCHAR fullPath[MAX_PATH * 2];
    WCHAR * pFilePart;
    WCHAR searchPart[MAX_PATH];
    bool bFound = false;
    HRESULT hr = S_OK;

    if (wcslen(filename) >= MAX_PATH) {
        ShowErrorIdString(FTL_InputFileNameTooLong, ERROR_FATAL, filename);
        return S_OK;
    }

    // Copy file name, and find location of file part.
    wcscpy(fullPath, filename);
    pFilePart = wcsrchr(fullPath, L'\\');
#if PLATFORM_UNIX
    WCHAR *pSlashFilePart = wcsrchr(fullPath, L'/');
    if (pSlashFilePart > pFilePart)
    {
        pFilePart = pSlashFilePart;
    }
#endif  // PLATFORM_UNIX
    if (!pFilePart)
        pFilePart = wcsrchr(fullPath, L':');
    if (!pFilePart)
        pFilePart = fullPath;
    else
        ++pFilePart;

    if (bRecurse) {
        // Search for subdirectories
        wcscpy(searchPart, pFilePart);  // Save this for later
        wcscpy(pFilePart, L"*.*");   // Search for ALL subdirectories

        hSearch = W_FindFirstFile(fullPath, &findData);
        if (hSearch == 0 || hSearch == INVALID_HANDLE_VALUE) {
            // No Subdirectories
            goto FileSearch;
        }

        for (;;) {
            if ((~findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
                (findData.cFileName[0] != '.' ||
                (findData.cFileName[1] != '\0' && (findData.cFileName[1] != '.' || findData.cFileName[2] != '\0'))) &&
                (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                wcslen(findData.cFileName) + wcslen(filename) + 1 <= sizeof(fullPath) / sizeof(WCHAR)) {
                swprintf( pFilePart, L"%s\\%s", findData.cFileName, searchPart);
                hr = ProcessFileName (fullPath, pInput, uiCodePage, pfFilesAdded, TRUE);
            }

            if (FAILED (hr) || !W_FindNextFile(hSearch, &findData))
                break;
        }

        FindClose(hSearch);
        if (FAILED (hr))
            return hr;
        wcscpy(pFilePart, searchPart);
    }

FileSearch:
    // Search for first matching file.
    hSearch = W_FindFirstFile(fullPath, &findData);
    if ((hSearch == 0 || hSearch == INVALID_HANDLE_VALUE)) {
        if (!bRecurse)
            ShowErrorIdString(ERR_FileNotFound, ERROR_ERROR, filename);
        return S_OK;
    }

    // Add this file name and all subsequent ones.
    for (;;) {
        if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN)) &&
            wcslen(findData.cFileName) + pFilePart - fullPath + 1 <= (int) (sizeof(fullPath) / sizeof(WCHAR))) {

            WCHAR temp[_MAX_PATH];

            bFound = true;
            wcscpy (pFilePart, findData.cFileName);
            W_GetFullPathName( fullPath, _MAX_PATH, temp, NULL);
            if (SUCCEEDED (hr = pInput->AddSourceFile (temp, &findData.ftLastWriteTime))) {
                *pfFilesAdded = TRUE;


            }

            if (hr == S_FALSE)
                ShowErrorIdString(ERR_FileAlreadyIncluded, ERROR_WARNING, fullPath);
        }

        if (FAILED (hr) || !W_FindNextFile(hSearch, &findData))
            break;
    }

    if (!bFound && !bRecurse)
        ShowErrorIdString(ERR_FileNotFound, ERROR_ERROR, filename);

    FindClose(hSearch);
    return hr;
}

// Set AllowWild to true, to get the first matching filename
// Otherwise, it will error if the filename has wildcards
bool GetFullFileName(LPWSTR szFilename, DWORD cchFilename, bool AllowWild = false)
{
    WCHAR buffer[MAX_PATH];
    DWORD len = 0;

    len = W_GetFullPathName(szFilename, MAX_PATH, buffer, NULL);
    if (len >= MAX_PATH || len >= cchFilename || cchFilename < MAX_PATH || len == 0 ||
        (!AllowWild && wcspbrk(szFilename, L"?*"))) {
        ShowErrorIdString(FTL_InputFileNameTooLong, ERROR_FATAL, szFilename);
        return false;
    }
    W_GetActualFileCase(buffer, szFilename);
    return true;
}

// Set AllowWild to true, to get the first matching filename
// Otherwise, it will error if the filename has wildcards
bool GetFullFileName(LPCWSTR szSource, LPWSTR szFilename, DWORD cchFilename, bool AllowWild = false)
{
    WCHAR buffer[MAX_PATH];
    DWORD len = 0;

    len = W_GetFullPathName(szSource, MAX_PATH, buffer, NULL);
    if (len >= MAX_PATH || len >= cchFilename || cchFilename < MAX_PATH || len == 0 ||
        (!AllowWild && wcspbrk(szSource, L"?*"))) {
        ShowErrorIdString(FTL_InputFileNameTooLong, ERROR_FATAL, szSource);
        return false;
    }
    W_GetActualFileCase(buffer, szFilename);
    return true;
}

/* The tree and list are used for the response file processing to keep track
 * to keep track of added strings for freeing later, and for duplicate detection
 */
struct list {
    WCHAR *arg;
    list *next;
};

struct b_tree {
    LPWSTR name;
    b_tree *lChild;
    b_tree *rChild;
};

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
 * Parse the text into a list of argument
 * return the total count 
 * and set 'args' to point to the last list element's 'next'
 * This function assumes the text is NULL terminated
 */
int TextToArgs(LPCWSTR pszText, list ***args)
{
    list **argList = NULL, **argLast = NULL;
    const WCHAR *pCur;
    int iCount;

    argList = argLast = *args;
    pCur = pszText;
    iCount = 0;

    // Guaranteed that all tokens are no bigger than the entire file.
    LPWSTR pszTemp = (LPWSTR)VSAlloc(sizeof(WCHAR) * (wcslen(pszText) + 1));
    while (*pCur != '\0')
    {
        WCHAR *pPut, *pFirst, *pLast;
        bool bQuoted;
        size_t iSlash;

LEADINGWHITE:
        while (IsWhitespace( *pCur) && *pCur != '\0')
            pCur++;

        if (*pCur == '\0')
            break;
        else if (*pCur == L'#')
        {
            while ( *pCur != '\0' && *pCur != '\n')
                pCur++; // Skip to end of line
            goto LEADINGWHITE;
        }

        // The treatment of quote marks is a bit different than the standard
        // treatment. We only remove quote marks at the very beginning and end of the
        // string. We still consider interior quotemarks for space ignoring purposes.
        // All the below are considered a single argument:
        //   "foo bar"  ->   foo bar
        //   "foo bar";"baz"  ->  "foo bar";"baz"
        //   fo"o ba"r -> fo"o ba"r
        bQuoted = false;
        int cQuotes = 0;
        pPut = pFirst = pszTemp;
        while ((!IsWhitespace( *pCur) || bQuoted) && *pCur != '\0')
        {
            switch (*pCur)
            {
                // All this weird slash stuff follows the standard argument processing routines
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
                        *pPut++ = *pCur++;
                        bQuoted = !bQuoted;
                        ++cQuotes;
                    }
                } else {
                    for (; iSlash > 0; iSlash--)
                        *pPut++ = L'\\';
                }
                break;
            case L'\"':
                bQuoted = !bQuoted;
                ++cQuotes;
                *pPut++ = *pCur++;
                break;
            default:
                *pPut++ = *pCur++;  // Copy the char and advance
            }
        }

        pLast = pPut;
        *pPut++ = '\0';

        // If the string is surrounded by quotes, with no interior quotes, remove them.
        if (cQuotes == 2 && *pFirst == L'\"' && *(pLast - 1) == L'\"') {
            ++pFirst;
            --pLast;
            *pLast = L'\0';
        }

        *argLast = (list*)VSAllocZero( sizeof(list));
        if (!*argLast) {
            ShowErrorId(FTL_NoMemory, ERROR_FATAL);
            break;
        }

        (*argLast)->arg = (WCHAR*)VSAlloc( sizeof(WCHAR) * (pLast - pFirst + 1));
        if (!(*argLast)->arg) {
            ShowErrorId(FTL_NoMemory, ERROR_FATAL);
            break;
        }

        wcscpy((*argLast)->arg, pFirst);
        iCount++;
        argLast = &(*argLast)->next; // Next is NULLed by the AllocZero
    }

    VSFree(pszTemp);
    *args = argLast;
    return iCount;
}

// Copy an argument list into an argv array. The argv array is allocated, the
// arguments themselves are not allocated -- just pointer copied.
int ArgListToArgV(list * argList, LPWSTR * * argv)
{
    int argc = 0;
    for (list * argTemp = argList; argTemp != NULL; argTemp = argTemp->next)
        ++argc;

    *argv = (LPWSTR *)VSAlloc(sizeof(LPWSTR*) * (argc));
    int i = 0;
    while (argList) {
        (*argv)[i++] = argList->arg;
        argList = argList->next;
    }

    return argc;
}



/*
 * Process Response files on the command line. Inserts the new args into the vals
 * list of all arguments.
 */
void ProcessResponseArgs(list *vals)
{
    b_tree *response_files = NULL; // track response files already seen.

    LPCWSTR pszText;
    WCHAR   pszFilename[MAX_PATH];
    CSourceText     *pSourceText = NULL;
    list * newArgs = NULL;
    list ** endNewArgs = &newArgs;


    for (list * args = vals; 
         args != NULL; 
         args = args->next)
    {
        WCHAR * arg = args->arg;

        // Skip everything except Response files
        if (arg == NULL || arg[0] != '@')
            continue;

        if (wcslen(arg) == 1) {
            ShowErrorIdString( ERR_NoFileSpec, ERROR_FATAL, arg);
            goto CONTINUE;
        }

        // Check for duplicates
        if (!GetFullFileName( RemoveQuotes(&arg[1]), pszFilename, MAX_PATH)) {
            break;
        }
        if (!TreeAdd(&response_files, pszFilename)) {
            ShowErrorIdString(ERR_DuplicateResponseFile, ERROR_ERROR, pszFilename);
            goto CONTINUE;
        }

        pSourceText = new CSourceText();
        if (pSourceText == NULL)
        {
            ShowErrorId(FTL_NoMemory, ERROR_FATAL);
            goto CONTINUE;
        }

        // NOTE: We can't do a codepage setting here because we haven't parsed it yet
        // Also this file itself could include a codepage setting!
        // so the CP_ACP will just have to do here!
        if (FAILED(pSourceText->Initialize (pszFilename)))
        {
            delete pSourceText;
            ShowErrorIdString(ERR_NoResponseFile, ERROR_FATAL, pszFilename);
            goto CONTINUE;
        }

        pSourceText->GetText( &pszText, NULL);
        TextToArgs( pszText, &endNewArgs);   // get new args.
        *endNewArgs = args->next;
        args->next = newArgs;                // link in with the rest of the args.
        delete pSourceText;

CONTINUE: // remove the response file argument, and continue to the next.
        args->arg = NULL;
        VSFree(arg);
    }

    CleanupTree(response_files);
}

/*
 * Process Auto Config options:
 * #1 search for '/noconfig'
 * if not present and csc.cfg exists in EXE dir, inject after env var stuff
 * if not present and csc.cfg exists in CurDir, inject after env var stuff
 * Set bAlloced true if it allocated a new argv array that must be freed later
 */
void ProcessAutoConfig(list ** vals)
{
    list * argList = *vals;
    list * argNew;

    // Scan the argument list for the "/noconfig" options. If present, just kill it and bail.
    for (list * valTemp = argList; valTemp; valTemp = valTemp->next)
    {
        // Skip everything except options
        WCHAR * arg = valTemp->arg;
        if (arg == NULL || (arg[0] != '/' && arg[0] != '-'))
            continue;

        if (_wcsicmp(arg + 1, L"noconfig") == 0) {
            valTemp->arg = NULL;
            VSFree(arg);
            // We found it, empty it and get out quick!
            return;
        }
    }

    // If we got here it means there was no '/noconfig'

    // Just add "@csc.rsp" if it exists (global and then local)
    // we do local first, then global, but add at front of list, so net that global
    // will actually be processed first.
    WCHAR pszFilename[MAX_PATH];
    WCHAR pszCurDir[MAX_PATH];
    if (W_GetCurrentDirectory(MAX_PATH, pszCurDir)) {
        _wmakepath(pszFilename, NULL, pszCurDir, L"csc", L"rsp");

        if (W_Access( pszFilename, 4) == 0) { 
            //We know the file exists and that we have read access
            // so add into the list
            argNew = (list*)VSAllocZero( sizeof(list));
            if (!argNew) {
                ShowErrorId(FTL_NoMemory, ERROR_FATAL);
            } else {

                argNew->arg = (WCHAR*)VSAlloc( sizeof(WCHAR) * (wcslen(pszFilename) + 2)); // +2 for @ and terminator
                if (!argNew->arg) {
                    ShowErrorId(FTL_NoMemory, ERROR_FATAL);
                } else {
                    argNew->arg[0] = L'@';
                    wcscpy(argNew->arg + 1, pszFilename);
                    argNew->next = argList;  // link at front of list
                    argList = argNew;
                }
            }
        }
    }

    WCHAR pszPath[MAX_PATH];
    if (W_IsUnicodeSystem()) {
        if(!GetModuleFileNameW(NULL, pszPath, MAX_PATH))
            pszPath[0] = 0;
    } else {
        CHAR pszTemp[MAX_PATH];
        if (!GetModuleFileNameA(NULL, pszTemp, MAX_PATH) ||
            !MultiByteToWideChar( AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                0, pszTemp, -1, pszPath, MAX_PATH))
            pszPath[0] = 0;
    }
    if (*pszPath) {
        WCHAR wszDrive[_MAX_DRIVE], wszDir[MAX_PATH];
        // Strip off the file-name, and replace it with the config file
        _wsplitpath( pszPath, wszDrive, wszDir, NULL, NULL);
        _wmakepath( pszPath, wszDrive, wszDir, L"csc", L"rsp");

        // Don't process csc.rsp twice if current dir and program dir are the same!
        if (_wcsicmp(pszFilename, pszPath) != 0 && W_Access( pszPath, 4) == 0) { 
            //We know the file exists and that we have read access
            // so add into the list
            argNew = (list*)VSAllocZero( sizeof(list));
            if (!argNew) {
                ShowErrorId(FTL_NoMemory, ERROR_FATAL);
                return;
            } else {

                argNew->arg = (WCHAR*)VSAlloc( sizeof(WCHAR) * (wcslen(pszPath) + 2)); // +2 for @ and terminator
                if (!argNew->arg) {
                    ShowErrorId(FTL_NoMemory, ERROR_FATAL);
                } else {
                    argNew->arg[0] = L'@';
                    wcscpy(argNew->arg + 1, pszPath);
                    argNew->next = argList;  // link at front of list.
                    argList = argNew;
                }
            }
        }
    }

    // return the new head of the list.
    *vals = argList;
}


////////////////////////////////////////////////////////////////////////////////
// BeginNewInputSet
//
// This function is called to make sure the given input set is "fresh", meaning
// it hasn't had any files added to it yet.

HRESULT BeginNewInputSet (ICSCompiler *pCompiler, CComPtr<ICSInputSet> &spInput, BOOL *pfFilesAdded)
{
    if (*pfFilesAdded)
    {
        // Need to create a new one
        HRESULT hr;
        spInput.Release();
        *pfFilesAdded = FALSE;
        hr = pCompiler->AddInputSet (&spInput);
        g_firstInputSet = false;
        if (SUCCEEDED(hr)) {
            // default for subsequent executables is DLL.
            spInput->SetOutputFileType(OUTPUT_MODULE);
        }
        return hr;
    }

    // This one is still fresh...
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ParseCommandLine

HRESULT ParseCommandLine (ICSCompiler *pCompiler, ICSCompilerConfig *pConfig, int argc, PWSTR argv[], UINT uiCodePage)
{
    HRESULT                 hr = E_FAIL;
    CComPtr<ICSInputSet>    spInput;
    long                    iOptionCount;
    bool                    usedShort;
    bool                    madeAssembly = true;

    // First, get the compiler option count, and create the first input set.
    if (FAILED (hr = pConfig->GetOptionCount (&iOptionCount)) ||
        FAILED (hr = pCompiler->AddInputSet (&spInput)))
    {
        ShowErrorId (ERR_InitError, ERROR_FATAL);
        return hr;
    }

    CComBSTR    sbstrDefines, sbstrImports, sbstrModules, sbstrLibPaths;
    BOOL        fFilesAdded = FALSE, fResourcesAdded = FALSE, fModulesAdded = FALSE;
    LPWSTR      pTemp = NULL;

    // Okay, we're ready to process the command line
    for (int iArg = 0; iArg < argc; ++iArg)
    {
        // Skip blank args -- they've already been processed by ProcessPreSwitches or the like
        if (argv[iArg] == NULL)
            continue;

        PWSTR   pszArg = argv[iArg];

        // If this is a switch, see what it is
        if (pszArg[0] == '-' || pszArg[0] == '/')
        {
            // Skip the switch leading char
            pszArg++;
            usedShort = false;

            // Check for options we know about
            if (_wcsnicmp (pszArg, L"out:", 4) == 0)
            {
                if (pszArg[4] == '\0') { // Check for an empty string
                    ShowErrorIdString( ERR_NoFileSpec, ERROR_FATAL, (pszArg - 1));
                    break;
                }

                // Output file specification.  Make sure we have a fresh input set, and
                // set its output files name to the next arg
                WCHAR szFilename[MAX_PATH];
                if (!GetFullFileName(RemoveQuotes(pszArg + 4), szFilename, MAX_PATH)) {
                    hr = E_FAIL;
                    break;
                }
                if (FAILED (hr = CheckResult (BeginNewInputSet (pCompiler, spInput, &fFilesAdded))) ||
                    FAILED (hr = CheckResult (spInput->SetOutputFileName (szFilename))))
                    break;
            }

            else if ((usedShort = ((*pszArg == 'm' || *pszArg == 'M') && pszArg[1] == ':')) ||
                _wcsnicmp (pszArg, L"main:", 5) == 0)
            {
                int iOptLen = usedShort ? 2 : 5;
                if (pszArg[iOptLen] == '\0') { // Check for an empty string
                    ShowErrorIdString( ERR_SwitchNeedsString, ERROR_FATAL, (pszArg - 1));
                    break;
                }

                // Output file specification.  Make sure we have a fresh input set, and
                // set its output files name to the next arg
                if (FAILED (hr = spInput->SetMainClass( RemoveQuotes(pszArg + iOptLen))))
                {
                    if (hr == E_INVALIDARG)
                        ShowErrorIdString( ERR_NoMainOnDLL, ERROR_FATAL);
                    else
                        CheckResult(hr);
                    break;
                }
            }


            else if ((usedShort = (_wcsnicmp (pszArg, L"res:", 4) == 0)) ||
                (_wcsnicmp (pszArg, L"resource:", 9) == 0))
            {
                int iOptLen = 9;
                if (usedShort)
                    iOptLen = 4;

                // Embedded resource
                WCHAR * option = RemoveQuotes(pszArg + iOptLen, L",");  // change ',' to '|'
                WCHAR szName[MAX_PATH], szIdent[MAX_PATH];
                int i = swscanf(option, L"%[^|]|%s", szName, szIdent);
                switch(i) {
                case 0:
                    wcscpy(szName, option);
                case 1:
                     // defaults to filename minus path
                    if ((pTemp = wcsrchr(szName, L'\\')) == NULL)
                        wcscpy(szIdent, szName);
                    else
                        wcscpy(szIdent, pTemp + 1);
                }
                if (wcslen(szName) == 0 || szName[0] == L'|') {
                    wcsncpy(szName, pszArg - 1, 10);
                    szName[10] = L'\0';
                    ShowErrorIdString (ERR_NoFileSpec, ERROR_FATAL, szName);
                    hr = E_FAIL;
                    break;
                }

                if (!GetFullFileName(szName, MAX_PATH)) {
                    hr = E_FAIL;
                    break;
                }
                if (FAILED (hr = CheckResult (spInput->AddResourceFile (szName, szIdent, true, true))))
                    break;
                if (hr == S_FALSE)
                    ShowErrorIdString( ERR_ResourceNotUnique, ERROR_ERROR, szIdent);
                fResourcesAdded = TRUE;
            }

            else if ((usedShort = (_wcsnicmp (pszArg, L"linkres:", 8)) == 0) || (_wcsnicmp (pszArg, L"linkresource:", 13) == 0))
            {
                // Link in a resource
                WCHAR szName[MAX_PATH], szIdent[MAX_PATH];
                int iOptLen = (usedShort ? 8 : 13);
                WCHAR * option = RemoveQuotes(pszArg + iOptLen, L",");  // change ',' to '|'

                int i = swscanf(option, L"%[^|]|%s", szName, szIdent);
                switch(i) {
                case 0:
                    wcscpy(szName, option);
                case 1:
                     // defaults to filename minus path
                    if ((pTemp = wcsrchr(szName, L'\\')) == NULL)
                        wcscpy(szIdent, szName);
                    else
                        wcscpy(szIdent, pTemp + 1);
                }
                if (wcslen(szName) == 0 || szName[0] == L'|') {
                    wcsncpy(szName, pszArg - 1, 10);
                    szName[10] = L'\0';
                    ShowErrorIdString (ERR_NoFileSpec, ERROR_FATAL, szName);
                    hr = E_FAIL;
                    break;
                }

                if (!GetFullFileName(szName, MAX_PATH)) {
                    hr = E_FAIL;
                    break;
                }
                if (FAILED (hr = CheckResult (spInput->AddResourceFile (szName, szIdent, false, true))))
                    break;
                if (hr == S_FALSE)
                    ShowErrorIdString( ERR_ResourceNotUnique, ERROR_ERROR, szIdent);
                fResourcesAdded = TRUE;
            }

            else if ((usedShort = ((*pszArg == 'd' || *pszArg == 'D') && pszArg[1] == ':')) ||
                (_wcsnicmp(pszArg, L"define:", 7) == 0))
            {
                int iOptLen = (usedShort ? 2 : 7);
                if (pszArg[iOptLen] == '\0') // Check for an empty string
                    ShowErrorIdString( ERR_SwitchNeedsString, ERROR_ERROR, (pszArg - 1));

                // Conditional compilation symbol(s) -- we separate this one out ourselves to allow multiple instances to be accumulative
                if (sbstrDefines != NULL)
                    sbstrDefines += L";";
                sbstrDefines += RemoveQuotes(pszArg+iOptLen);
            }

            else if ((usedShort = ((*pszArg == 'r' || *pszArg == 'R') && pszArg[1] == ':')) ||
                (_wcsnicmp(pszArg, L"reference:", 10) == 0))
            {
                int iOptLen = (usedShort ? 2 : 10);
                if (pszArg[iOptLen] == '\0') // Check for an empty string
                    ShowErrorIdString( ERR_NoFileSpec, ERROR_ERROR, (pszArg - 1));

                // imports switch has multiple file names seperated by |
                if (sbstrImports != NULL)
                    sbstrImports += L"|";
                sbstrImports += RemoveQuotes(pszArg+iOptLen, L";,"); // change ';' or ',' to '|'
            }

            else if (_wcsnicmp(pszArg, L"addmodule:", 10) == 0)
            {
                if (pszArg[10] == '\0') // Check for an empty string
                    ShowErrorIdString( ERR_NoFileSpec, ERROR_ERROR, (pszArg - 1));
                else
                    fModulesAdded = TRUE;

                // imports switch has multiple file names seperated by |
                if (sbstrModules != NULL)
                    sbstrModules += L"|";
                sbstrModules += RemoveQuotes(pszArg+10, L";,"); // change ';' or ',' to '|');
            }

            else if (_wcsnicmp(pszArg, L"lib:", 4) == 0)
            {
                if (pszArg[4] == '\0') // Check for an empty string
                    ShowErrorIdString( ERR_NoFileSpec, ERROR_ERROR, (pszArg - 1));
                else
                {
                    // Similar to /D switch -- we allow multiple of these separated by a ';'
                    if (sbstrLibPaths != NULL)
                        sbstrLibPaths += L";";
                    sbstrLibPaths += pszArg+4; // we don't remove quotes here, because that's done later by SearchPath API.
                }
            }

            else if (_wcsnicmp (pszArg, L"baseaddress:", 12) == 0)
            {
                WCHAR *endCh;
                DWORD imageBase = (wcstoul( RemoveQuotes(pszArg + 12), &endCh, 0) + 0x00008000) & 0xFFFF0000; // Round the low order word to align it
                if (*endCh != L'\0')    // Invalid number
                {
                    ShowErrorIdString( ERR_BadBaseNumber, ERROR_ERROR, pszArg + 12);
                    continue;
                }

                // Set the image base.
                if (FAILED (hr = CheckResult (spInput->SetImageBase( imageBase))))
                {
                    break;
                }
            }

            else if (_wcsnicmp (pszArg, L"filealign:", 10) == 0)
            {
                WCHAR *endCh;
                DWORD align = wcstoul( RemoveQuotes(pszArg + 10), &endCh, 0);
                if (*endCh != L'\0')    // Invalid number
                {
                    ShowErrorIdString( ERR_BadFileAlignment, ERROR_ERROR, pszArg + 10);
                    continue;
                }

                // Set the alignment - this also does some checking.
                hr = spInput->SetFileAlignment( align);
                if (hr == E_INVALIDARG)
                    ShowErrorIdString( ERR_BadFileAlignment, ERROR_ERROR, pszArg + 10);
                else if (FAILED(hr = CheckResult(hr)))
                    break;
            }

            else if ((usedShort = (_wcsnicmp(pszArg, L"t:", 2) == 0)) || _wcsnicmp(pszArg, L"target:", 7) == 0) 
            {
                // Begin a new input set.
                if (FAILED (hr = CheckResult (BeginNewInputSet (pCompiler, spInput, &fFilesAdded))))
                    break;

                WCHAR * subOption = pszArg + (usedShort ? 2 : 7);
                if (wcscmp(subOption, L"module") == 0) {
                    if (g_firstInputSet)
                        madeAssembly = false;
                    if (FAILED (hr = CheckResult (spInput->SetOutputFileType(OUTPUT_MODULE))))
                    {
                        break;
                    }
                }
                else if (wcscmp(subOption, L"library") == 0) {
                    if (!g_firstInputSet)
                        ShowErrorIdString(ERR_BadTargetForSecondInputSet, ERROR_ERROR);
                    else {
                        if (FAILED (hr = CheckResult (spInput->SetOutputFileType(OUTPUT_LIBRARY))))
                            break;
                    }
                }
                else if (wcscmp(subOption, L"exe") == 0) {
                    if (!g_firstInputSet)
                        ShowErrorIdString(ERR_BadTargetForSecondInputSet, ERROR_ERROR);
                    else {
                        if (FAILED (hr = CheckResult (spInput->SetOutputFileType(OUTPUT_CONSOLE))))
                            break;
                    }
                }
                else if (wcscmp(subOption, L"winexe") == 0) {
                    if (!g_firstInputSet)
                        ShowErrorIdString(ERR_BadTargetForSecondInputSet, ERROR_ERROR);
                    else {
                        if (FAILED (hr = CheckResult (spInput->SetOutputFileType(OUTPUT_WINDOWS))))
                            break;
                    }
                }
                else {
                    ShowErrorIdString(FTL_InvalidTarget, ERROR_FATAL);
                }
                if (hr == S_FALSE) {
                    ShowErrorIdString( ERR_NoMainOnDLL, ERROR_FATAL);
                    hr = E_FAIL;
                    break;
                }
            }
            else if (_wcsnicmp (pszArg, L"recurse:", 8) == 0)
            {
                unsigned int iOptLen = usedShort ? 2 : 8;

                // Recursive file specification.
                if (wcslen(pszArg) <= iOptLen)
                {
                    ShowErrorIdString (ERR_NoFileSpec, ERROR_FATAL, pszArg - 1);
                    hr = E_FAIL;
                    break;
                }

                if (FAILED (hr = CheckResult (ProcessFileName (RemoveQuotes(&pszArg[iOptLen]), spInput, uiCodePage, &fFilesAdded, TRUE))))
                    break;
            }
            else if (_wcsicmp (pszArg, L"noconfig") == 0)
            {
                // '/noconfig' would've been handled earlier if it was on the original command line
                // so just give a warning that we're ignoring it.
                ShowErrorId (WRN_NoConfigNotOnCommandLine, ERROR_WARNING);
            }

            else
            {
                PCWSTR  pszShortSwitch, pszLongSwitch, pszDescSwitch;
                long    iOptionId;
                size_t  iArgLen;
                size_t  iSwitchLen = 0;
                DWORD   dwFlags;
                bool    match;

                // It isn't one we recognize, so it must be an option exposed by the compiler.
                // See if we can find it.
                long i;
                for (i=0; i<iOptionCount; i++)
                {
                    // Get the info for this switch
                    if (FAILED (hr = CheckResult (pConfig->GetOptionInfoAtEx (i, &iOptionId, &pszShortSwitch, &pszLongSwitch, &pszDescSwitch, NULL, &dwFlags))))
                        return hr;

                    // See if it matches the arg we have (up to length of switch name only)
                    iArgLen = wcslen (pszArg);
                    match = false;

                    // FYI: below test of pszArg[iSwitchLen] < 0x40 tests for '\0', ':', '+', '-', but not a letter -- e.g., make sure
                    // this isn't a prefix of some longer switch.

                    // Try new short option
                    if (pszShortSwitch)
                        match = (iArgLen >= (iSwitchLen = wcslen (pszShortSwitch))
                                && (_wcsnicmp (pszShortSwitch, pszArg, iSwitchLen) == 0
                                && pszArg[iSwitchLen] < 0x40 ));
                    // Try new long option
                    if (!match) {
                        ASSERT(pszLongSwitch);
                        match = (iArgLen >= (iSwitchLen = wcslen (pszLongSwitch))
                                && (_wcsnicmp (pszLongSwitch, pszArg, iSwitchLen) == 0
                                && pszArg[iSwitchLen] < 0x40 ));
                    }

                    if (match)
                    {
                        VARIANT vt;
                        V_VT(&vt) = VT_EMPTY;

                        // We have a match, possibly.  If this is a string switch, we need to have
                        // a colon and some text.  If boolean, we need to check for optional [+|-]
                        if (dwFlags & COF_BOOLEAN)
                        {
                            VARIANT_BOOL    fValue;

                            // A boolean option -- look for [+|-]
                            if (iArgLen == iSwitchLen)
                                fValue = TRUE;
                            else if (iArgLen == iSwitchLen + 1 && (pszArg[iSwitchLen] == '-' || pszArg[iSwitchLen] == '+'))
                                fValue = (pszArg[iSwitchLen] == '+');
                            else
                                continue;   // Keep looking
                            V_VT(&vt) = VT_BOOL;
                            V_BOOL(&vt) = fValue;
                        }
                        else
                        {
                            // String option.  Following the argument should be a colon, and we'll
                            // use whatever text follows as the string value for the option.
                            if (iArgLen == iSwitchLen)
                            {
                                ShowErrorIdString (ERR_SwitchNeedsString, ERROR_FATAL, argv[iArg]);
                                return E_FAIL;
                            }

                            if (pszArg[iSwitchLen] != ':')
                                continue;   // Keep looking
                            V_VT(&vt) = VT_BSTR;
                            V_BSTR(&vt) = SysAllocString(RemoveQuotes(pszArg + iSwitchLen + 1));
                        }

                        // assemble is deprecated.
                        if (dwFlags & COF_WARNONOLDUSE) {
                            PCWSTR pszAltText = NULL;
                            //No deprecated options for now
                            //switch(iOptionId) {
                            //default:
                                VSFAIL("It's a deprecated switch but we don't know the replacement!");
                                pszAltText = L"unknown";
                            //    break;
                            //}
                            ShowErrorIdString(WRN_UseNewSwitch, ERROR_WARNING, pszDescSwitch, pszAltText);
                        }


                        // Got the option -- call in to set it
                        hr = CheckResult (pConfig->SetOption (iOptionId, vt));
                        if (V_VT(&vt) == VT_BSTR)
                        {
                            SysFreeString(V_BSTR(&vt));
                        }
                        if (FAILED (hr))
                        {
                            return hr;
                        }
                        break;
                    }
                }

                if (i == iOptionCount)
                {
#if PLATFORM_UNIX
                    if (pszArg[-1] == '/')
                    {
                        // On BSD it could be a file that starts with '/'
                        // Not a switch, so it might be a source file spec
                        if (FAILED (hr = CheckResult (ProcessFileName (RemoveQuotes(argv[iArg]), spInput, uiCodePage, &fFilesAdded, FALSE))))
                            break;
                    }
                    else
#endif
                    {
                        // Didn't find it...
                        ShowErrorIdString (ERR_BadSwitch, ERROR_FATAL, argv[iArg]);
                        return E_FAIL;
                    }
                }
            }
        }
        else
        {
            // Not a switch, so it must be a source file spec
            if (FAILED (hr = CheckResult (ProcessFileName (RemoveQuotes(argv[iArg]), spInput, uiCodePage, &fFilesAdded, FALSE))))
                break;
        }
    }

    if (FAILED (hr))
        return hr;

    // Make sure at least one "output" was created successfully
    // Non assemblies must have sources, but assemblies may be resource-only or only have addmodules
    if ((!madeAssembly && !fFilesAdded) || (madeAssembly && !fFilesAdded && !fResourcesAdded && !fModulesAdded))
    {
        if (g_firstInputSet)
            ShowErrorId (ERR_NoSources, ERROR_FATAL);
        else
            ShowErrorId (ERR_NoSourcesInLastInputSet, ERROR_FATAL);
        return E_FAIL;
    }

    // Don't forget out our /D switch accumulation...
    if (sbstrDefines != NULL)
    {
        VARIANT vt;
        V_VT(&vt) = VT_BSTR;
        // No need to allocate a new BSTR and Free it in this case
        V_BSTR(&vt) = sbstrDefines;

        if (FAILED (hr = CheckResult (pConfig->SetOption (OPTID_CCSYMBOLS, vt))))
            return hr;
    }

    // ...or our import specifications
    if (sbstrImports != NULL)
    {
        VARIANT vt;
        V_VT(&vt) = VT_BSTR;
        // No need to allocate a new BSTR and Free it in this case
        V_BSTR(&vt) = sbstrImports;

        if (FAILED (hr = CheckResult (pConfig->SetOption (OPTID_IMPORTS, vt))))
            return hr;
    }

    // ...or our added modules
    if (sbstrModules != NULL)
    {
        VARIANT vt;
        V_VT(&vt) = VT_BSTR;
        // No need to allocate a new BSTR and Free it in this case
        V_BSTR(&vt) = sbstrModules;

        if (FAILED (hr = CheckResult (pConfig->SetOption (OPTID_MODULES, vt))))
            return hr;
    }

    // ...or the LIB path
    if (sbstrLibPaths != NULL)
    {
        VARIANT vt;
        V_VT(&vt) = VT_BSTR;
        // No need to allocate a new BSTR and Free it in this case
        V_BSTR(&vt) = sbstrLibPaths;

        if (FAILED (hr = CheckResult (pConfig->SetOption (OPTID_LIBPATH, vt))))
            return hr;
    }

    CComPtr<ICSError>   spError;

    // Okay, we've successfully finished parsing options.  Now we must validate the settings.
    if (FAILED (hr = CheckResult (pConfig->CommitChanges (&spError))))
        return hr;

    if (hr == S_FALSE)
    {
        ASSERT (spError != NULL);

        // Something with the compiler settings is no good.
        CompilerHost::ReportError (spError);
        hr = E_FAIL;
    }
    return hr;
}


/*
 * Main entry point
 */
int __cdecl main(int argcIgnored, char **argvIgnored)
{
    int argc;
    LPWSTR * argv;
    HRESULT hr = E_FAIL;  // assume failure unless otherwise.
    bool noLogo, showHelp, timeCompiler;
    UINT uiCodePage = 0;
    LARGE_INTEGER timeStart, timeStop, frequency;
    list *vals;
    list **valsLast;

    if (!PAL_RegisterLibraryW(L"rotor_palrt") ||
        !PAL_RegisterLibraryW(L"cscomp") ||
        !PAL_RegisterLibraryW(L"sscoree"))
    {
        ShowError(errorInfo[FTL_NoMessagesDLL].number, ERROR_FATAL, NULL, -1, -1, -1, "Unable to load critical libraries");
        return 1;
    }
    VsDebugInitialize();

//    CHAR chars[255];
//    wsprintf(chars, "%u", GetCurrentProcessId());
//    MessageBox(0, "pause", chars, 0);

    // Initialize Unilib
    W_IsUnicodeSystem();

    // Try to load the message DLL.
    g_hinstMessages = GetMessageDll();
    if (! g_hinstMessages) {
        ShowError(errorInfo[FTL_NoMessagesDLL].number, ERROR_FATAL, NULL, -1, -1, -1, "Unable to find messages file 'cscomp.satellite'");
        return 1;
    }

    // The first part of command line processing is done as a linked list of
    // arguments, which is created, added to, an manipulated be several functions.
    // "vals" is the beginning of the list. "valsLast" points to where to add to the end.

    // Get command line, split it into arguments.
    LPWSTR cmdLine = GetCommandLineW();
    vals = NULL;
    valsLast = &vals;
    argc = TextToArgs(cmdLine, &valsLast);

    // Process '/noconfig', and CSC.CFG, modifying the vals argument list. We
    // pass "&(vals->next)" because we don't want to bother with the very first argument,
    // the program name.
    ProcessAutoConfig(&(vals->next));

    // Process Response Files
    // We do this before the PreSwitches in case there are any
    // switches in the response files.
    ProcessResponseArgs(vals);

    // Now convert to an argc/argv form for remaining processing.
    argc = ArgListToArgV(vals, &argv);

    // Do initial switch processing.
    ProcessPreSwitches(argc, argv, &noLogo, &showHelp, &timeCompiler, &uiCodePage);

    // All errors previous to this are fatal
    // This solve the error-before-logo problem
    if (hadError)
        return 1;

    // Print the logo banner, unless noLogo was specified.
    if (!noLogo)
        PrintBanner();

    // If timing, start the timer.
    if (timeCompiler) {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&timeStart);
    }


    CompilerHost        host;
    ICSCompiler         *compiler = NULL;
    ICSCompilerFactory  *pFactory = NULL;
    ICSCompilerConfig   *pConfig = NULL;

    // Instanciate the compiler factory and create a compiler.  Let it
    // create its own name table.
    if (FAILED (hr = CreateCompilerFactory (&pFactory)) ||
        FAILED (hr = pFactory->CreateCompiler (0, &host, NULL, &compiler)) ||
        FAILED (hr = compiler->GetConfiguration (&pConfig)))
    {
        if (pFactory != NULL)
            pFactory->Release();
        if (compiler != NULL)
            compiler->Release();
        ShowErrorId(ERR_InitError, ERROR_FATAL);
        return 1;
    }

    host.SetCompiler (compiler);
    host.SetCodePage (uiCodePage);

    if (showHelp)
    {
        // Display help and do nothing else
        hr = PrintHelp (pConfig);
    }
    else
    {
        // Parse the command line, and if successful, compile.
        if (SUCCEEDED (hr = ParseCommandLine (compiler, pConfig, argc - 1, argv + 1, uiCodePage)))
        {
            hr = compiler->Compile (NULL);

            if (FAILED (hr))
                ShowErrorId (ERR_InitError, ERROR_FATAL);
        }
    }

    // Free the arguments that were created.
    for (list *l = vals; l;)
    {
        list *t;
        if (l->arg)
            VSFree(l->arg);
        t = l->next;
        VSFree(l);
        l = t;
    }
    VSFree(argv);

    if (pConfig != NULL)
        pConfig->Release();

    if (compiler)
    {
        compiler->Shutdown();
        compiler->Release();
    }

    if (pFactory != NULL)
        pFactory->Release();


    if (timeCompiler) {
        double elapsedTime;
        QueryPerformanceCounter(&timeStop);
        elapsedTime = (double) (timeStop.QuadPart - timeStart.QuadPart) / (double) frequency.QuadPart;
        print("\nElapsed time: %#.4g seconds\n", elapsedTime);
    }


#ifdef TRACKMEM
//    wprintf (L"Total VsAlloc: %u\n", memMax);

#endif

    // Return success code.
    return (hr != S_OK || hadError) ? 1 : 0;
}


/* Support needed for VS memory leak checking */
#ifdef DEBUG
#undef new
PVOID __cdecl operator new(size_t size)
      { return VsDebAlloc(0, size); }
PVOID __cdecl operator new(size_t size, LPSTR pszFile, UINT uLine)
      { return VsDebugAllocInternal(DEFAULT_HEAP, 0, size, pszFile, uLine, INSTANCE_GLOBAL, NULL); }
void  __cdecl operator delete(PVOID pv)
      { VsDebFree(pv); }
#endif //DEBUG





////////////////////////////////////////////////////////////////////////////////
// BugReprt helpers

char NibbleToChar(BYTE b)
{
    if (b < 10)
    {
        return b + '0';
    }
    else
    {
        return (char) (b + 'A' - 10);
    } 
}

void ByteToHex(BYTE b, LPSTR str)
{
    str[0] = NibbleToChar(b >> 4);
    str[1] = NibbleToChar(b & 0xF);
}

void CompilerHost::NotifyBinaryFile(LPCWSTR filename)
{
}

void CompilerHost::NotifyMetadataFile(LPCWSTR pszFileName, LPCWSTR pszCorSystemDirectory)
{
    if (_wcsnicmp(pszFileName, pszCorSystemDirectory, wcslen(pszCorSystemDirectory)))
    {
        NotifyBinaryFile(pszFileName);
    }
}

