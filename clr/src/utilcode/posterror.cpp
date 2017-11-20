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
// PostErrors.cpp
//
// This module contains the error handling/posting code for the engine.  It
// is assumed that all methods may be called by a dispatch client, and therefore
// errors are always posted using IErrorInfo.
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
#include <utilcode.h>                   // Utility helpers.
#include <corerror.h>
#include "../dlls/mscorrc/resource.h"

#include <posterror.h>

#define FORMAT_MESSAGE_LENGTH       1024
#if !defined(lengthof)
#define lengthof(x) (sizeof(x)/sizeof(x[0]))
#endif

// Local prototypes.
HRESULT FillErrorInfo(LPCWSTR szMsg, DWORD dwHelpContext);


//For FEATURE_PAL build, resource strings are stored in satellite files and 
//use the PAL_ methods to load the values.  These methods use the HSATELLITE 
//type rather then HINSTANCE.  Thus, all references to HINSTANCE below 
//should be HSATELLITE for the FEATURE_PAL case.
#define HINSTANCE HSATELLITE 

//********** Code. ************************************************************

//*****************************************************************************
// Must associate each handle to an instance of MsCorRc.dll with the int
// that it represents
//*****************************************************************************
struct CCulturedHInstance
{
    int         m_LangId;
    HINSTANCE   m_hInst;
};

//*****************************************************************************
// CCompRC manages string Resource access for COM+. This includes loading 
// the MsCorRC.dll for resources as well allowing each thread to use a 
// a different localized version.
//*****************************************************************************
class CCompRC
{
public:
    HRESULT LoadString(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet=false);

    void Init();

    void SetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName
    );
    
private:
    HRESULT LoadResourceLibrary(HINSTANCE * pHInst);

// We must map between a thread's int and a dll instance. 
// Since we only expect 1 language almost all of the time, we'll special case 
// that and then use a variable size map for everything else.
    CCulturedHInstance m_Primary;

    CCulturedHInstance * m_pHash;   
    int m_nHashSize;

    CRITICAL_SECTION m_csMap;
    

// Main accessors for hash
    HINSTANCE LookupNode(int langId);
    void AddMapNode(int langId, HINSTANCE hInst);

    
    FPGETTHREADUICULTURENAME m_fpGetThreadUICultureName;
    FPGETTHREADUICULTUREID m_fpGetThreadUICultureId;
    FPGETTHREADUICULTUREPARENTNAME  m_fpGetThreadUICultureParentName;
};

//*****************************************************************************
// Do the mapping from an langId to an hinstance node
//*****************************************************************************
HINSTANCE CCompRC::LookupNode(int langId)
{
    if (m_pHash == NULL) return NULL;

// Linear search
    int i;
    for(i = 0; i < m_nHashSize; i ++) {
        if (m_pHash[i].m_LangId == langId) {
            return m_pHash[i].m_hInst;
        }
    }

    return NULL;
}

//*****************************************************************************
// Add a new node to the map and return it.
//*****************************************************************************
const int MAP_STARTSIZE = 7;
const int MAP_GROWSIZE = 5;

void CCompRC::AddMapNode(int langId, HINSTANCE hInst)
{
    if (m_pHash == NULL) {
        m_pHash = new CCulturedHInstance[MAP_STARTSIZE];
        ZeroMemory(m_pHash, sizeof(CCulturedHInstance) * MAP_STARTSIZE);
        m_nHashSize = MAP_STARTSIZE;
    }

// For now, place in first open slot
    int i;
    for(i = 0; i < m_nHashSize; i ++) {
        if (m_pHash[i].m_LangId == 0) {
            m_pHash[i].m_LangId = langId;
            m_pHash[i].m_hInst = hInst;
            return;
        }
    }

// Out of space, regrow
    CCulturedHInstance * pNewHash = new CCulturedHInstance[m_nHashSize + MAP_GROWSIZE];
    if (pNewHash)
    {
        CopyMemory(pNewHash, m_pHash, sizeof(CCulturedHInstance) * m_nHashSize);
        ZeroMemory(pNewHash + m_nHashSize, sizeof(CCulturedHInstance) * MAP_GROWSIZE);
        delete [] m_pHash;
        m_pHash = pNewHash;
        m_pHash[m_nHashSize].m_LangId = langId;
        m_pHash[m_nHashSize].m_hInst = hInst;
        m_nHashSize += MAP_GROWSIZE;
    }
}

//*****************************************************************************
// Initialize
//*****************************************************************************
void CCompRC::Init()
{
    m_pHash = NULL;
    m_nHashSize = 0;

    m_fpGetThreadUICultureName = NULL;
    m_fpGetThreadUICultureId = NULL;

    InitializeCriticalSection (&m_csMap);
}

void CCompRC::SetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName
)
{
    m_fpGetThreadUICultureName = fpGetThreadUICultureName;
    m_fpGetThreadUICultureId = fpGetThreadUICultureId;
    m_fpGetThreadUICultureParentName = fpGetThreadUICultureParentName;
}


//*****************************************************************************
// Load the string 
// We load the localized libraries and cache the handle for future use.
// Mutliple threads may call this, so the cache structure is thread safe.
//*****************************************************************************
HRESULT CCompRC::LoadString(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet)
{
    HRESULT     hr;
    HINSTANCE   hInst = 0; //instance of cultured resource dll
    HINSTANCE   hLibInst = 0; //Holds early library instance
    BOOL        fLibAlreadyOpen = FALSE; //Determine if we can close the opened library.

    // Must resolve current thread's langId to a dll.   
    int langId;
    if (m_fpGetThreadUICultureId) {
        langId = (*m_fpGetThreadUICultureId)();

    // Callback can't return 0, since that indicates empty.
    // To indicate empty, callback should return UICULTUREID_DONTCARE
        _ASSERTE(langId != 0);

    } else {
        langId = UICULTUREID_DONTCARE;
    }


    // Try to match the primary entry
    if (m_Primary.m_LangId == langId)
        hInst = m_Primary.m_hInst;

    // If this is the first visit, we must set the primary entry
    else if (m_Primary.m_LangId == 0) {
        IfFailGo(LoadResourceLibrary(&hLibInst));
        
        EnterCriticalSection (&m_csMap);

            // As we expected
            if (m_Primary.m_LangId == 0) {
                hInst = m_Primary.m_hInst = hLibInst;
                m_Primary.m_LangId = langId;
            }

            // Someone got into this critical section before us and set the primary already
            else if (m_Primary.m_LangId == langId) {
                hInst = m_Primary.m_hInst;
                fLibAlreadyOpen = TRUE;
            }

            // If neither case is true, someone got into this critical section before us and
            //  set the primary to other than the language we want...
            else
                fLibAlreadyOpen = TRUE;

        LeaveCriticalSection(&m_csMap);

        if (fLibAlreadyOpen)
        {
            PAL_FreeSatelliteResource(hLibInst);

            fLibAlreadyOpen = FALSE;
        }
    }


    // If we enter here, we know that the primary is set to something other than the
    // language we want - multiple languages use the hash table
    if (hInst == NULL) {

        // See if the resource exists in the hash table
        EnterCriticalSection (&m_csMap);
            hInst = LookupNode(langId);
        LeaveCriticalSection (&m_csMap);

        // If we didn't find it, we have to load the library and insert it into the hash
        if (hInst == NULL) {
            IfFailGo(LoadResourceLibrary(&hLibInst));
        
            EnterCriticalSection (&m_csMap);

                // Double check - someone may have entered this section before us
                hInst = LookupNode(langId);
                if (hInst == NULL) {
                    hInst = hLibInst;
                    AddMapNode(langId, hInst);
                }
                else
                    fLibAlreadyOpen = TRUE;

            LeaveCriticalSection (&m_csMap);


            if (fLibAlreadyOpen){
                PAL_FreeSatelliteResource(hLibInst);
            }
        }
    }



    // Now that we have the proper dll handle, load the string
    _ASSERTE(hInst != NULL);
    if (PAL_LoadSatelliteStringW(hInst, iResourceID, szBuffer, iMax))
        return (S_OK);

    // Allows caller to check for string not found without extra debug checking.
    if (bQuiet) {
        hr = E_FAIL;
    }
    else {
        // Shouldn't be any reason for this condition but the case where we
        // used the wrong ID or didn't update the resource DLL.
        _ASSERTE(0);
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

ErrExit:
    // Return an empty string to save the people with a bad error handling
    if (szBuffer && iMax)
        *szBuffer = L'\0';

    return hr;
}

//*****************************************************************************
// Load the library for this thread's current language
// Called once per language. 
// Search order is: 
//  1. Dll in localized path (<dir of this module><lang name (en-US format)>\mscorrc.dll)
//  2. Dll in localized (parent) path (<dir of this module><lang name> (en format)\mscorrc.dll)
//  3. Dll in root path (<dir of this module>\mscorrc.dll)
//  4. Dll in current path   (<current dir>\mscorrc.dll)
//*****************************************************************************

#define _LoadResourceFile(lpFileName) PAL_LoadSatelliteResourceW(lpFileName)
static const WCHAR szMscorrc[] = L"mscorrc.satellite";

HRESULT CCompRC::LoadResourceLibrary(HINSTANCE * pHInst)
{
    HINSTANCE   hInst = NULL;
    const int   MAX_LANGPATH = 20;

    WCHAR       szPath[_MAX_PATH];      // Path to resource DLL.
    WCHAR       szLang[MAX_LANGPATH + 2];   // extension to path for language

    DWORD       dwDirLen;
    DWORD       dwLangLen;
    const DWORD dwMscorrcLen = NumItems(szMscorrc) - 1;

    _ASSERTE(pHInst != NULL);

    VERIFY(PAL_GetPALDirectory(szPath, NumItems(szPath)));

    dwDirLen = Wszlstrlen(szPath);

    if (m_fpGetThreadUICultureName) {
        int len = (*m_fpGetThreadUICultureName)(szLang, MAX_LANGPATH);
        
        if (len != 0) {
            _ASSERTE(len <= MAX_LANGPATH);
            szLang[len] = '\\';
            szLang[len+1] = '\0';
        }
    } else {
        szLang[0] = 0;
    }

    dwLangLen  = Wszlstrlen(szLang);
    if (dwDirLen + dwLangLen + dwMscorrcLen < NumItems(szPath)) {
        Wszlstrcpy(szPath + dwDirLen, szLang);
        Wszlstrcpy(szPath + dwDirLen + dwLangLen, szMscorrc);

        // Feedback for debugging to eliminate unecessary loads.
        DEBUG_STMT(DbgWriteEx(L"Loading %s to load strings.\n", szPath));

        hInst = _LoadResourceFile(szPath);
        if (hInst != NULL)
            goto Done;
    }

    // If we can't find the specific language implementation, try the language parent
    if (m_fpGetThreadUICultureParentName) {
        int len = (*m_fpGetThreadUICultureParentName)(szLang, MAX_LANGPATH);
    
        if (len != 0) {
            _ASSERTE(len <= MAX_LANGPATH);
            szLang[len] = '\\';
            szLang[len+1] = '\0';
        }
    } 
    else {
        szLang[0] = 0;
    }

    dwLangLen = Wszlstrlen(szLang);
    if (dwDirLen + dwLangLen + dwMscorrcLen < NumItems(szPath)) {
        Wszlstrcpy(szPath + dwDirLen, szLang);
        Wszlstrcpy(szPath + dwDirLen + dwLangLen, szMscorrc);

        hInst = _LoadResourceFile(szPath);
        if (hInst != NULL)
            goto Done;
    }

    // If we can't find the language specific one, just use what's in the root
    if (dwDirLen + dwMscorrcLen < NumItems(szPath)) {
        Wszlstrcpy(szPath + dwDirLen, szMscorrc);
        hInst = _LoadResourceFile(szPath);
        if (hInst != NULL)
            goto Done;
    }

    // Last ditch search effort in current directory
    hInst = _LoadResourceFile(szMscorrc);

    if (hInst == NULL) {
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

Done:
    *pHInst = hInst;
    return (S_OK);
}


CCompRC         g_ResourceDll;          // Used for all clients in process.
LONG            g_ResourceDllInit = 0;

CCompRC* GetResourceDll()
{
    if (g_ResourceDllInit == -1)
        return &g_ResourceDll;

    if (InterlockedCompareExchange(&g_ResourceDllInit, 1, 0) == 0)
    {
        g_ResourceDll.Init();
        g_ResourceDllInit = -1;
    }
    else // someone has already begun initializing. 
    {
        // just wait until it finishes
        while (g_ResourceDllInit != -1)
            ::Sleep(1);
    }

    return &g_ResourceDll;
}

//*****************************************************************************
// Function that we'll expose to the outside world to fire off the shutdown method
//*****************************************************************************

//*****************************************************************************
// Set callbacks to get culture info
//*****************************************************************************
void SetResourceCultureCallbacks(
    FPGETTHREADUICULTURENAME fpGetThreadUICultureName,
    FPGETTHREADUICULTUREID fpGetThreadUICultureId,
    FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName
)
{
// Either both are NULL or neither are NULL
    _ASSERTE((fpGetThreadUICultureName != NULL) == 
        (fpGetThreadUICultureId != NULL));

    GetResourceDll()->SetResourceCultureCallbacks(
        fpGetThreadUICultureName, 
        fpGetThreadUICultureId,
        fpGetThreadUICultureParentName
    );

}

//*****************************************************************************
// Public function to load a resource string
//*****************************************************************************
HRESULT LoadStringRC(
    UINT iResourceID, 
    LPWSTR szBuffer, 
    int iMax, 
    int bQuiet
)
{
    return (GetResourceDll()->LoadString(iResourceID, szBuffer, iMax, bQuiet));
}


//*****************************************************************************
// Format a Runtime Error message.
//*****************************************************************************
HRESULT _cdecl FormatRuntimeErrorVa(        
    WCHAR       *rcMsg,                 // Buffer into which to format.         
    ULONG       cchMsg,                 // Size of buffer, characters.          
    HRESULT     hrRpt,                  // The HR to report.                    
    va_list     marker)                 // Optional args.                       
{
    WCHAR       rcBuf[512];             // Resource string.
    HRESULT     hr;
    
    // Ensure nul termination.
    *rcMsg = L'\0';

    // If this is one of our errors, then grab the error from the rc file.
    if (HRESULT_FACILITY(hrRpt) == FACILITY_URT)
    {
        hr = LoadStringRC(LOWORD(hrRpt), rcBuf, NumItems(rcBuf), true);
        if (hr == S_OK)
        {
            _vsnwprintf(rcMsg, cchMsg, rcBuf, marker);
            rcMsg[cchMsg - 1] = 0;
        }
    }
    // Otherwise it isn't one of ours, so we need to see if the system can
    // find the text for it.
    else
    {
        if (WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                0, hrRpt, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                rcMsg, cchMsg, 0/*                          */))
        {
            hr = S_OK;

            // System messages contain a trailing \r\n, which we don't want normally.
            int iLen = lstrlenW(rcMsg);
            if (iLen > 3 && rcMsg[iLen - 2] == '\r' && rcMsg[iLen - 1] == '\n')
                rcMsg[iLen - 2] = '\0';
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // If we failed to find the message anywhere, then issue a hard coded message.
    if (FAILED(hr))
    {
        swprintf(rcMsg, L"Common Language Runtime Internal error: 0x%08x", hrRpt);
        DEBUG_STMT(DbgWriteEx(rcMsg));
    }

    return hrRpt;    
}

//*****************************************************************************
// Format a Runtime Error message, varargs.
//*****************************************************************************
HRESULT _cdecl FormatRuntimeError(
    WCHAR       *rcMsg,                 // Buffer into which to format.
    ULONG       cchMsg,                 // Size of buffer, characters.
    HRESULT     hrRpt,                  // The HR to report.
    ...)                                // Optional args.
{
    va_list     marker;                 // User text.
    va_start(marker, hrRpt);
    hrRpt = FormatRuntimeErrorVa(rcMsg, cchMsg, hrRpt, marker);
    va_end(marker);
    return hrRpt;
}

//*****************************************************************************
// This function will post an error for the client.  If the LOWORD(hrRpt) can
// be found as a valid error message, then it is loaded and formatted with
// the arguments passed in.  If it cannot be found, then the error is checked
// against FormatMessage to see if it is a system error.  System errors are
// not formatted so no add'l parameters are required.  If any errors in this
// process occur, hrRpt is returned for the client with no error posted.
//*****************************************************************************
HRESULT _cdecl PostError(               // Returned error.
    HRESULT     hrRpt,                  // Reported error.
    ...)                                // Error arguments.
{
    WCHAR       rcMsg[512];             // Error message.
    va_list     marker;                 // User text.
    HRESULT     hr;

    // Return warnings without text.
    if (!FAILED(hrRpt))
        return (hrRpt);

    // Format the error.
    va_start(marker, hrRpt);
    FormatRuntimeErrorVa(rcMsg, lengthof(rcMsg), hrRpt, marker);
    va_end(marker);

    // Check for an old message and clear it.  Our public entry points do not do
    // a SetErrorInfo(0, 0) because it takes too long.
    IErrorInfo  *pIErrInfo;
    if (GetErrorInfo(0, &pIErrInfo) == S_OK)
        pIErrInfo->Release();

    // Turn the error into a posted error message.  If this fails, we still
    // return the original error message since a message caused by our error
    // handling system isn't going to give you a clue about the original error.
    hr = FillErrorInfo(rcMsg, LOWORD(hrRpt));
    _ASSERTE(hr == S_OK);

    return (hrRpt);
}


//*****************************************************************************
// Create, fill out and set an error info object.  Note that this does not fill
// out the IID for the error object; that is done elsewhere.
//*****************************************************************************
HRESULT FillErrorInfo(                  // Return status.
    LPCWSTR     szMsg,                  // Error message.
    DWORD       dwHelpContext)          // Help context.
{
    ICreateErrorInfo *pICreateErr;      // Error info creation Iface pointer.
    IErrorInfo *pIErrInfo = NULL;       // The IErrorInfo interface.
    HRESULT     hr;                     // Return status.

    // Get the ICreateErrorInfo pointer.
    if (FAILED(hr = CreateErrorInfo(&pICreateErr)))
        return (hr);

    // Set message text description.
    if (FAILED(hr = pICreateErr->SetDescription((LPWSTR) szMsg)))
        goto Exit1;

    // Set the help file and help context.
    if (FAILED(hr = pICreateErr->SetHelpFile(L"complib.hlp")) ||
        FAILED(hr = pICreateErr->SetHelpContext(dwHelpContext)))
        goto Exit1;

    // Get the IErrorInfo pointer.
    if (FAILED(hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PVOID *) &pIErrInfo)))
        goto Exit1;

    // Save the error and release our local pointers.
    SetErrorInfo(0L, pIErrInfo);

Exit1:
    pICreateErr->Release();
    if (pIErrInfo) {
	pIErrInfo->Release();
    }
    return hr;
}

//*****************************************************************************
// Diplays a message box with details about error if client  
// error mode is set to see such messages; otherwise does nothing. 
//*****************************************************************************
void DisplayError(HRESULT hr, LPWSTR message, UINT nMsgType)
{
    WCHAR   rcMsg[FORMAT_MESSAGE_LENGTH];       // Error message to display
    WCHAR   rcTemplate[FORMAT_MESSAGE_LENGTH];  // Error message template from resource file
    WCHAR   rcTitle[24];        // Message box title

    // Retrieve error mode
    UINT last = SetErrorMode(0);
    SetErrorMode(last);         //set back to previous value
                    
    // Display message box if appropriate
    if(last & SEM_FAILCRITICALERRORS)
        return;
    
    //Format error message
    LoadStringRC(IDS_EE_ERRORTITLE, rcTitle, NumItems(rcTitle), true);
    LoadStringRC(IDS_EE_ERRORMESSAGETEMPLATE, rcTemplate, NumItems(rcTemplate), true);

    _snwprintf(rcMsg, FORMAT_MESSAGE_LENGTH, rcTemplate, hr, message);
    WszMessageBoxInternal(NULL, rcMsg, rcTitle , nMsgType);
}

int CorMessageBox(
                  HWND hWnd,        // Handle to Owner Window
                  UINT uText,       // Resource Identifier for Text message
                  UINT uCaption,    // Resource Identifier for Caption
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle, // Flag to show FileName in Caption
                  ...)              // Additional Arguments
{
    //Assert if none of MB_ICON is set
    _ASSERTE((uType & MB_ICONMASK) != 0);

    int result = IDCANCEL;
    WCHAR   *rcMsg = new WCHAR[FORMAT_MESSAGE_LENGTH];      // Error message to display
    WCHAR   *rcCaption = new WCHAR[FORMAT_MESSAGE_LENGTH];      // Message box title

    if (!rcMsg || !rcCaption)
            goto exit1;

    //Load the resources using resource IDs
    if (SUCCEEDED(LoadStringRC(uCaption, rcCaption, FORMAT_MESSAGE_LENGTH, true)) &&  
        SUCCEEDED(LoadStringRC(uText, rcMsg, FORMAT_MESSAGE_LENGTH, true)))
    {
        WCHAR *rcFormattedMessage = new WCHAR[FORMAT_MESSAGE_LENGTH];
        WCHAR *rcFormattedTitle = new WCHAR[FORMAT_MESSAGE_LENGTH];
        WCHAR *fileName = new WCHAR[MAX_PATH];

        if (!rcFormattedMessage || !rcFormattedTitle || !fileName)
            goto exit;
        
        //Format message string using optional parameters
        va_list     marker;
        va_start(marker, ShowFileNameInTitle);
        vswprintf(rcFormattedMessage, rcMsg, marker);

        //Try to get filename of Module and add it to title
        if (ShowFileNameInTitle && WszGetModuleFileName(NULL, fileName, MAX_PATH))
        {
            LPWSTR name = new WCHAR[wcslen(fileName) + 1];
            LPWSTR ext = new WCHAR[wcslen(fileName) + 1];
        
            SplitPath(fileName, NULL, NULL, name, ext);     //Split path so that we discard the full path

            swprintf(rcFormattedTitle,
                     L"%s%s - %s",
                     name, ext, rcCaption);
            if(name)
                delete [] name;
            if(ext)
                delete [] ext;
        }
        else
        {
            wcscpy(rcFormattedTitle, rcCaption);
        }
        result = WszMessageBoxInternal(hWnd, rcFormattedMessage, rcFormattedTitle, uType);
exit:
        if (rcFormattedMessage)
            delete [] rcFormattedMessage;
        if (rcFormattedTitle)
            delete [] rcFormattedTitle;
        if (fileName)
            delete [] fileName;
    }
    else
    {
        //This means that Resources cannot be loaded.. show an appropriate error message. 
        result = WszMessageBoxInternal(NULL, L"Failed to load resources from resource file\nPlease check your Setup", L"Setup Error", MB_OK | MB_ICONSTOP); 
    }

exit1:
    if (rcMsg)
        delete [] rcMsg;
    if (rcCaption)
        delete [] rcCaption;
    return result;
}


int CorMessageBoxCatastrophic(
                  HWND hWnd,        // Handle to Owner Window
                  UINT iText,       // Text for MessageBox
                  UINT iTitle,      // Title for MessageBox
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle) // Flag to show FileName in Caption
{
    WCHAR wszText[500];
    WCHAR wszTitle[500];

    HRESULT hr;

    hr = LoadStringRC(iText,
                      wszText,
                      sizeof(wszText)/sizeof(wszText[0]),
                      FALSE);
    if (FAILED(hr)) {
        wszText[0] = L'?';
        wszText[1] = L'\0';
    }

    hr = LoadStringRC(iTitle,
                      wszTitle,
                      sizeof(wszTitle)/sizeof(wszTitle[0]),
                      FALSE);
    if (FAILED(hr)) {
        wszTitle[0] = L'?';
        wszTitle[1] = L'\0';
    }

    return CorMessageBoxCatastrophic(
            hWnd, wszText, wszTitle, uType, ShowFileNameInTitle );
}


int CorMessageBoxCatastrophic(
                  HWND hWnd,        // Handle to Owner Window
                  LPWSTR lpText,    // Text for MessageBox
                  LPWSTR lpTitle,   // Title for MessageBox
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle, // Flag to show FileName in Caption
                  ...)
{
    _ASSERTE((uType & MB_ICONMASK) != 0);

    WCHAR rcFormattedMessage[FORMAT_MESSAGE_LENGTH];
    WCHAR rcFormattedTitle[FORMAT_MESSAGE_LENGTH];
    WCHAR fileName[MAX_PATH];

    //Format message string using optional parameters
    va_list     marker;
    va_start(marker, ShowFileNameInTitle);
    vswprintf(rcFormattedMessage, lpText, marker);

    //Try to get filename of Module and add it to title
    if (ShowFileNameInTitle && WszGetModuleFileName(NULL, fileName, MAX_PATH)){
        LPWSTR name = new WCHAR[wcslen(fileName) + 1];
        LPWSTR ext = new WCHAR[wcslen(fileName) + 1];
        
        SplitPath(fileName, NULL, NULL, name, ext); //Split path so that we discard the full path

        swprintf(rcFormattedTitle,
                 L"%s%s - %s",
                 name, ext, lpTitle);
        if(name)
            delete [] name;
        if(ext)
            delete [] ext;
    }
    else{
        wcscpy(rcFormattedTitle, lpTitle);
    }
    return WszMessageBoxInternal(hWnd, rcFormattedMessage, rcFormattedTitle, uType);
}
