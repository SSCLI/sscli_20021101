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
// File: options.cpp
//
// Code to parse and set the compiler options.
// ===========================================================================

#include "stdafx.h"
#include "compiler.h"
#include "options.h"
#include "controller.h"
#include "inputset.h"

////////////////////////////////////////////////////////////////////////////////
// Create the options table

#define BOOLOPTDEF(id,descid,sh,lg,des,f) { NULL, NULL, NULL, 0, 0, 0, 0 },
#define STROPTDEF(id,descid,sh,lg,des,f) BOOLOPTDEF(id,descid,sh,lg,des,f)
OPTIONDEF   COptionData::m_rgOptionTable[] =
{
    #include "optdef.h"
    { NULL, NULL, NULL, 0, 0, NULL, 0 }
};

const int TOTAL_OPTIONS = sizeof (COptionData::m_rgOptionTable) / sizeof (COptionData::m_rgOptionTable[0]) - 1;

long    COptionData::m_rgiOptionIndexMap[LARGEST_OPTION_ID];
BOOL    COptionData::m_fMapCreated = FALSE;

////////////////////////////////////////////////////////////////////////////////
// COptionData::CreateIndexMap

void COptionData::CreateIndexMap ()
{
    long i;
    for (i=0; i<LARGEST_OPTION_ID; i++)
        m_rgiOptionIndexMap[i] = -1;

    for (i=0; i<TOTAL_OPTIONS; i++)
        m_rgiOptionIndexMap[m_rgOptionTable[i].iOptionID] = i;

    m_fMapCreated = TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// COptionData::InitOptionTable
// Avoids a global constructor by inserting the correct data
// into the table. This should be called from DllMain and
// any other entry points.

void COptionData::InitOptionTable ()
{
    static bool sTableInited = false;
    if (!sTableInited)
    {
#define BOOLOPTDEF(id,descid,sh,lg,des,f) { sh, lg, des, OPTID_##id, offsetof (COptionData, m_f##id), IDS_OD_##descid, NULL, f|COF_BOOLEAN },
#define STROPTDEF(id,descid,sh,lg,des,f) { sh, lg, des, OPTID_##id, offsetof (COptionData, m_sbstr##id), IDS_OD_##descid, NULL, f },
        OPTIONDEF   tempOptionsTable[] =
        {
            #include "optdef.h"
            { NULL, NULL, NULL, 0, 0, NULL, 0 }
        };
        ASSERT(sizeof(tempOptionsTable) == sizeof(m_rgOptionTable));
        memcpy(m_rgOptionTable, tempOptionsTable, sizeof(tempOptionsTable));
        sTableInited = true;
    }
}

////////////////////////////////////////////////////////////////////////////////
// COptionData::ResetToDefaults

void COptionData::ResetToDefaults ()
{
    for (long i=0; i<TOTAL_OPTIONS; i++)
    {
        if (m_rgOptionTable[i].dwFlags & COF_BOOLEAN)
        {
            *((BOOL *)(((BYTE *)this) + m_rgOptionTable[i].iOffset)) = m_rgOptionTable[i].dwFlags & COF_DEFAULTON;
        }
        else if (m_rgOptionTable[i].dwFlags & COF_HASDEFAULT)
        {
            // These are special cased...
            if (m_rgOptionTable[i].iOptionID == OPTID_WARNINGLEVEL)
                m_sbstrWARNINGLEVEL = L"4";
            else
                VSFAIL ("HEY!  A string option has a default value that isn't handled!");
        }
        else
        {
            ((CComBSTR *)(((BYTE *)this) + m_rgOptionTable[i].iOffset))->Empty();
        }
    }

    // Set the post-commit options
    warnLevel = 4;
    pdbOnly = false;
    noWarnNumbers.Empty();

#define HACKBOOLOPT( var, str) var = FALSE;
#define HACKSTROPT( var, str) var.Empty();
#include "hacks.h"
    unknownTestSwitch.Empty();
    m_fNOCODEGEN = false;
}


////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::CCompilerConfig

CCompilerConfig::CCompilerConfig () :
    m_pController(NULL),
    m_pData(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::~CCompilerConfig

CCompilerConfig::~CCompilerConfig ()
{
    if (m_pController != NULL)
        m_pController->Release();

    if (m_pData != NULL)
        delete m_pData;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::Initialize

HRESULT CCompilerConfig::Initialize (CController *pController, COptionData *pData)
{
    m_pController = pController;
    pController->AddRef();

    m_pData = new COptionData (*pData);
    if (m_pData == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::GetOptionCount

STDMETHODIMP CCompilerConfig::GetOptionCount (long *piCount)
{
    *piCount = TOTAL_OPTIONS;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::GetOptionInfoAt

STDMETHODIMP CCompilerConfig::GetOptionInfoAt (long iIndex, long *piOptionID, PCWSTR *ppszSwitchName, PCWSTR *ppszDescription, DWORD *pdwFlags)
{
    if (iIndex < 0 || iIndex >= TOTAL_OPTIONS)
        return E_INVALIDARG;

    OPTIONDEF   *pDef = COptionData::GetOptionDef (iIndex);

    // Provide the option ID
    if (piOptionID != NULL)
        *piOptionID = pDef->iOptionID;

    // Provide the switch name
    if (ppszSwitchName != NULL) {
        if (pDef->pszShortSwitch)
            *ppszSwitchName = pDef->pszShortSwitch;
        else
            *ppszSwitchName = pDef->pszLongSwitch;
    }

    // Provide the description string
    if (ppszDescription != NULL)
    {
        // Make sure the description string is loaded
        if (pDef->bstrDesc == NULL && (pDef->iDescID != IDS_OD_HIDDEN))
        {
            CComBSTR    sbstr;
            sbstr.LoadString (GetMessageDll(), pDef->iDescID);
            if (sbstr == NULL)
                return E_UNEXPECTED;
            pDef->bstrDesc = sbstr.Detach();
        }

        *ppszDescription = pDef->bstrDesc;
        if (*ppszDescription == NULL)
            *ppszDescription = L"";
    }

    // Provide flags
    if (pdwFlags != NULL)
        *pdwFlags = pDef->dwFlags;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::GetOptionInfoAtEx

STDMETHODIMP CCompilerConfig::GetOptionInfoAtEx (long iIndex, long *piOptionID, PCWSTR *ppszShortSwitchName, PCWSTR *ppszLongSwitchName, PCWSTR *ppszDescSwitchName, PCWSTR *ppszDescription, DWORD *pdwFlags)
{
    if (iIndex < 0 || iIndex >= TOTAL_OPTIONS)
        return E_INVALIDARG;

    OPTIONDEF   *pDef = COptionData::GetOptionDef (iIndex);

    // Provide the option ID
    if (piOptionID != NULL)
        *piOptionID = pDef->iOptionID;

    // Provide the switch names
    if (ppszShortSwitchName != NULL)
        *ppszShortSwitchName = pDef->pszShortSwitch;
    if (ppszLongSwitchName != NULL)
        *ppszLongSwitchName = pDef->pszLongSwitch;
    if (ppszDescSwitchName != NULL)
        *ppszDescSwitchName = pDef->pszDescSwitch;

    // Provide the description string
    if (ppszDescription != NULL)
    {
        // Make sure the description string is loaded
        if (pDef->bstrDesc == NULL && (pDef->iDescID != IDS_OD_HIDDEN))
        {
            CComBSTR    sbstr;
            sbstr.LoadString (GetMessageDll(), pDef->iDescID);
            if (sbstr == NULL)
                return E_UNEXPECTED;
            pDef->bstrDesc = sbstr.Detach();
        }

        *ppszDescription = pDef->bstrDesc;
        if (*ppszDescription == NULL)
            *ppszDescription = L"";
    }

    // Provide flags
    if (pdwFlags != NULL)
        *pdwFlags = pDef->dwFlags;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::ResetAllOptions

STDMETHODIMP CCompilerConfig::ResetAllOptions ()
{
    m_pData->ResetToDefaults ();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::GetOption

STDMETHODIMP CCompilerConfig::GetOption (long iOptionID, VARIANT *pvtValue)
{
    if (iOptionID < 0 || iOptionID >= LARGEST_OPTION_ID)
        return E_INVALIDARG;

    long        iIndex = COptionData::GetOptionIndex (iOptionID);
    if (iIndex == -1)
        return E_INVALIDARG;

    OPTIONDEF   *pOpt = COptionData::GetOptionDef (iIndex);
    void        *pMember = (void *)(((BYTE *)m_pData) + pOpt->iOffset);

    if (pOpt->dwFlags & COF_BOOLEAN)
	{
		V_VT(pvtValue) = VT_BOOL;
		V_BOOL(pvtValue) = (!!*((BOOL *)pMember)) ? VARIANT_TRUE : VARIANT_FALSE;
	}
    else
	{
		V_VT(pvtValue) = VT_BSTR;
		V_BSTR(pvtValue) = SysAllocString((OLECHAR *)pMember);
	}

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::SetOption

STDMETHODIMP CCompilerConfig::SetOption (long iOptionID, VARIANT vtValue)
{
    if (iOptionID < 0 || iOptionID >= LARGEST_OPTION_ID)
        return E_INVALIDARG;

    long        iIndex = COptionData::GetOptionIndex (iOptionID);
    if (iIndex == -1)
        return E_INVALIDARG;

    OPTIONDEF   *pOpt = COptionData::GetOptionDef (iIndex);
    void        *pMember = (void *)(((BYTE *)m_pData) + pOpt->iOffset);

    // Check for booleans first...
    if (pOpt->dwFlags & COF_BOOLEAN)
    {
        if (V_VT(&vtValue) != VT_BOOL)
            return E_INVALIDARG;

        *(BOOL *)pMember = V_BOOL(&vtValue);
        return S_OK;
    }

    // All other options are string...
    if (V_VT(&vtValue) != VT_BSTR)
        return E_INVALIDARG;

    // Special-case the 'test' switch used for undocumented internal/test switches.
    if (iOptionID == OPTID_INTERNALTESTS)
    {
        WCHAR* opt;
        WCHAR* next = V_BSTR(&vtValue);
        DWORD chLen = 0, chTextLen = 0;

        for(next = V_BSTR(&vtValue); next != NULL; opt = next) {
            opt = next;
            next = wcschr(opt, L';');
            if (next == NULL)
                chLen = (int)wcslen(opt);
            else {
                chLen = (DWORD)(next - opt);
                if ( !*(++next))
                    next = NULL;
            }
#define HACKBOOLOPT(var, text) \
            if (_wcsnicmp(opt, text, chLen) == 0) {     \
                m_pData -> var = TRUE;                  \
                continue;                               \
            }
#define HACKSTROPT(var, text) \
            chTextLen = (DWORD)wcslen(text);                                            \
            if (chLen > chTextLen && _wcsnicmp(opt, text, chTextLen) == 0 &&     \
                opt[chTextLen] == L'=') {                                        \
                m_pData -> var = CComBSTR(chLen - chTextLen - 1, opt + chTextLen + 1); \
                continue;                                                        \
            }
#include "hacks.h"

            m_pData->unknownTestSwitch = CComBSTR(chLen, opt);
        }

        m_pData->m_sbstrINTERNALTESTS += L";";
        m_pData->m_sbstrINTERNALTESTS += V_BSTR(&vtValue);
        return S_OK;
    }

    // If NoWarnList is already set, then concatinate with the current values (and a ';').
    if (iOptionID == OPTID_NOWARNLIST && *((CComBSTR *)pMember)) {
        *((CComBSTR *)pMember) += L",";
        *((CComBSTR *)pMember) += V_BSTR(&vtValue);
        return S_OK;
    }

    // Setting the debug type automatically turns debugging on.
    if (iOptionID == OPTID_DEBUGTYPE)
        m_pData->m_fEMITDEBUGINFO = true;

    // Set all others...
    *((CComBSTR *)pMember) = (V_BSTR(&vtValue) == NULL || wcslen(V_BSTR(&vtValue)) == 0) ? NULL : V_BSTR(&vtValue);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::ReportOptionError -- report an error with an option.

HRESULT __cdecl CCompilerConfig::ReportOptionError(ICSError **ppError, long iErrorId, ...)
{
    HRESULT     hr;
    va_list     args;
    CError      *pError;

    va_start (args, iErrorId);
    if (FAILED (hr = m_pController->CreateError (iErrorId, args, &pError)) ||
        FAILED (hr = pError->QueryInterface (IID_ICSError, (void **)ppError)))
    {
        va_end(args);
        return hr;
    }

    va_end(args);
    return S_FALSE;        // Indicate error
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::CommitChanges

STDMETHODIMP CCompilerConfig::CommitChanges (ICSError **ppError)
{
    CComBSTR    &pW = m_pData->m_sbstrWARNINGLEVEL;
    CComBSTR    &pDebugType = m_pData->m_sbstrDEBUGTYPE;

    // Before committing, we must verify that the options that have been set so
    // far don't "conflict".  Also, certain string options that should boil down
    // to a numeric range (for example) is checked here (rather than in
    // SetOption, because we have a way to describe the error).
    if (pW == NULL || wcslen (pW) != 1 || pW[0] < '0' || pW[0] > '4')
    {
        return ReportOptionError(ppError, ERR_BadWarningLevel);
    }

    m_pData->warnLevel = pW[0] - '0';
    if (m_pData->warnLevel == 0 && m_pData->m_fWARNINGSAREERRORS)
    {
        return ReportOptionError(ppError, ERR_NoWarningsAsErrors);
    }

    if (pDebugType == NULL || wcslen(pDebugType) == 0 || _wcsicmp(pDebugType, L"full") == 0)
        m_pData->pdbOnly = false;
    else if (_wcsicmp(pDebugType, L"pdbonly") == 0)
        m_pData->pdbOnly = true;
    else
        return ReportOptionError(ppError, ERR_BadDebugType, (BSTR) pDebugType);

    // Validate and set the "no warn" list. Use wcstok on a copy of the option,
    // because wcstok modifies the string.
    if (m_pData->m_sbstrNOWARNLIST.Length() > 0) {
        LPWSTR noWarnText;
        LPWSTR warnNumberStr, endPtr;
        unsigned long warnNumber;
        unsigned short * warnNumberList;
        int cWarnings;
        const WCHAR delimiters[] = { ' ', ',', ';', '\0' };  // delimiters in the warning list.

        noWarnText = (LPWSTR) _alloca((m_pData->m_sbstrNOWARNLIST.Length() + 1) * sizeof(WCHAR));
        warnNumberList = (unsigned short *) _alloca(m_pData->m_sbstrNOWARNLIST.Length()); // Can't have more warnings than this!
        wcscpy(noWarnText, m_pData->m_sbstrNOWARNLIST);

        // First count how many numbers, and validate them.
        cWarnings = 0;
        for (warnNumberStr = wcstok(noWarnText, delimiters); warnNumberStr; warnNumberStr = wcstok(NULL, delimiters))
        {

            warnNumber = wcstoul(warnNumberStr, &endPtr, 10);
            if ((endPtr != warnNumberStr + wcslen(warnNumberStr)) || ! IsValidWarningNumber(warnNumber)) {
                return ReportOptionError(ppError, ERR_BadWarningNumber, warnNumberStr);
            }
            warnNumberList[cWarnings] = (unsigned short) warnNumber;
            ++cWarnings;
        }

        // Now alloc the array of warning numbers, which is stored in a BSTR for convenience sake.
        m_pData->noWarnNumbers = CComBSTR(cWarnings, warnNumberList);
    }

    if (m_pData->unknownTestSwitch.Length() > 0)
    {
        return ReportOptionError(ppError, ERR_UnknownTestSwitch, (LPCWSTR) m_pData->unknownTestSwitch);
    }

    // No error -- commit the change to the controller (which will fire the
    // notifications to the host if actual changes are made).
    *ppError = NULL;
    m_pController->SetConfiguration (m_pData);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig::GetCompiler

STDMETHODIMP CCompilerConfig::GetCompiler (ICSCompiler **ppCompiler)
{
    *ppCompiler = m_pController;
    (*ppCompiler)->AddRef();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CController::SetConfiguration

void CController::SetConfiguration (COptionData *pData)
{
    BOOL    rgfChangedIndex[TOTAL_OPTIONS];

    // The configuration has already been verified.  What we need to do is
    // perform a "diff" of each option as we copy them over, sending the
    // OnConfigChange event back to the host for each one that's different.
    // First, do a comparison, setting corresponding entries in rgfChangedIndex
    // for differing option values.
    long i;
    for (i=0; i<TOTAL_OPTIONS; i++)
    {
        OPTIONDEF   *pDef = COptionData::GetOptionDef(i);
        void        *pNewMember = (void *)(((BYTE *)pData) + pDef->iOffset);
        void        *pCurMember = (void *)(((BYTE *)&m_OptionData) + pDef->iOffset);

        if (pDef->dwFlags & COF_BOOLEAN)
        {
            rgfChangedIndex[i] = (!*((BOOL *)pCurMember) != !*((BOOL *)pNewMember));
        }
        else
        {
            PCWSTR  pszNew = *((CComBSTR *)pNewMember), pszCur = *((CComBSTR *)pCurMember);

            if ((pszNew == NULL || pszNew[0] == 0) && (pszCur == NULL || pszCur[0] == 0))
                rgfChangedIndex[i] = FALSE;     // Both null/empty -- no change
            else if (pszNew != NULL && pszCur != NULL && wcscmp (pszNew, pszCur) == 0)
                rgfChangedIndex[i] = FALSE;     // They're the same
            else
                rgfChangedIndex[i] = TRUE;      // They're different
        }
    }

    // Okay, copy over the goods
    m_OptionData = *pData;

    // Now, fire the events
    for (i=0; i<TOTAL_OPTIONS; i++)
    {
        if (rgfChangedIndex[i])
        {
            VARIANT vt;
            OPTIONDEF   *pDef = COptionData::GetOptionDef(i);
            void        *pMember = (void *)(((BYTE *)&m_OptionData) + pDef->iOffset);

            if (pDef->dwFlags & COF_BOOLEAN)
            {
                V_VT(&vt) = VT_BOOL;
                V_BOOL(&vt) = !!(*((BOOL *)pMember)) ? VARIANT_TRUE : VARIANT_FALSE;
                m_spHost->OnConfigChange (pDef->iOptionID, vt);
            }
            else
            {
                V_VT(&vt) = VT_BSTR;
                V_BSTR(&vt) = SysAllocString((OLECHAR *)pMember);
                m_spHost->OnConfigChange (pDef->iOptionID, vt);
                SysFreeString(V_BSTR(&vt));
            }
        }
    }
}



////////////////////////////////////////////////////////////////////////////////
// COMPILER::AddInputSet

HRESULT COMPILER::AddInputSet (CInputSet *pSet)
{
    HRESULT     hr = S_OK;
    DWORD       dwExceptionCode;

    PAL_TRY
    {
        COMPILER_EXCEPTION_TRYEX
        {
            hr = AddInputSetWorker (pSet);
        }
        COMPILER_EXCEPTION_EXCEPT_FILTER_EX(PAL_Except1, &dwExceptionCode)
        {
            pController->GetHost()->OnCatastrophicError (TRUE, dwExceptionCode, pController->GetExceptionAddress());
            hr = E_UNEXPECTED;
        }
        COMPILER_EXCEPTION_ENDTRY
    }
    EXCEPT_FILTER_EX(PAL_Except2, &dwExceptionCode)
    {
        if (dwExceptionCode == FATAL_EXCEPTION_CODE)
            hr = E_FAIL;
        else
            hr = E_UNEXPECTED;
    }
    PAL_ENDTRY

    return hr;
}

HRESULT COMPILER::AddInputSetWorker (CInputSet *pSet)
{
    HRESULT         hr = S_OK;

    long            iSources = pSet->GetSourceTable()->Count();
    long            iResources = pSet->GetResourceTable()->Count();
    long            iMax = max (iSources, iResources);
    INFILESYM       *pInFile = NULL;
    OUTFILESYM      *pOutFile = NULL;
    CSourceFileInfo **ppSrcFiles = (CSourceFileInfo **)_alloca (iMax * sizeof (CSourceFileInfo *));


    if (iSources == 0 && pSet->GetOutputName() == NULL) {
        Error( NULL, ERR_OutputNeedsName);
        return S_FALSE;
    }

    //
    // create the output file
    //
    pOutFile = symmgr.CreateOutFile(
                    pSet->GetOutputName(),
                    !!pSet->DLL(),
                    !!pSet->WinApp(),
                    pSet->GetMainClass(),
                    NULL,
                    NULL,
                    !!options.m_fINCBUILD);

    pOutFile->imageBase = pSet->ImageBase();
    pOutFile->fileAlign = pSet->FileAlignment();
    m_bAssemble |= (pSet->MakeAssembly() == TRUE);
    if (pOutFile->isUnnamed() && options.m_fINCBUILD)
        Error( NULL, ERR_UnnamedOutputAndIncr);

    //
    // create the input files
    //
    pSet->CopySources (ppSrcFiles);
    for (long i=0; i<iSources; i++)
    {
        pInFile = symmgr.CreateSourceFile (ppSrcFiles[i]->m_pName->text, pOutFile, ppSrcFiles[i]->m_ft);
    }

    RESFILESYM      *pResFile;
    NAME            **ppNames = (NAME **)ppSrcFiles;

    pSet->GetResourceTable()->CopyContents (ppNames, NULL);
    for (long i=0; i<iResources; i++)
    {
        unsigned chName, chIdent;

        swscanf(ppNames[i]->text, L"%10u%10u", &chName, &chIdent);
        LPCWSTR p = ppNames[i]->text + 20; // running pointer

        LPWSTR szName = (LPWSTR) _alloca(sizeof(WCHAR) * (chName + 1));
        memcpy(szName, p, sizeof(WCHAR) * chName);
        p += chName;
        szName[chName] = L'\0';

        LPWSTR szIdent = (LPWSTR) _alloca(sizeof(WCHAR) * (chIdent+1));
        memcpy(szIdent, p, sizeof(WCHAR) * chIdent);
        p += chIdent;
        szIdent[chIdent] = L'\0';

        bool bEmbed = (*p++ == L'T');
        bool bVis = (*p == L'T');
        if (bEmbed)
        {
            pResFile = symmgr.CreateEmbeddedResFile (szName, szIdent, bVis);
        }
        else
        {
            OUTFILESYM *pOutFile = symmgr.CreateOutFile(
                                    szName,
                                    true,
                                    false,
                                    NULL,
                                    NULL,
                                    NULL,
                                    false);
            pOutFile->isResource = true;

            pResFile = symmgr.CreateSeperateResFile (szName, pOutFile, szIdent, bVis);
        }
        if ( pResFile == NULL)
            Error( NULL, ERR_ResourceNotUnique, szIdent);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// COMPILER::AddImport

HRESULT COMPILER::AddImport (PCWSTR pszFile, ULONG assemblyIndex)
{
    AddMetadataFile (pszFile, assemblyIndex);	// All imports are in a different assembly
    return S_OK;
}

