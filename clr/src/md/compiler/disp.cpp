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
// Disp.cpp
//
// Implementation for the meta data dispenser code.
//
//*****************************************************************************
#include "stdafx.h"
#include "disp.h"
#include "regmeta.h"
#include "mdutil.h"
#include <corerror.h>
#include <mdlog.h>
#include <mdcommon.h>

//*****************************************************************************
// Ctor.
//*****************************************************************************
Disp::Disp() : m_cRef(0), m_Namespace(0)
{
#if defined(LOGGING)
    // InitializeLogging() calls scattered around the code.
    InitializeLogging();
#endif

    m_OptionValue.m_DupCheck = MDDupDefault;
    m_OptionValue.m_RefToDefCheck = MDRefToDefDefault;
    m_OptionValue.m_NotifyRemap = MDNotifyDefault;
    m_OptionValue.m_UpdateMode = MDUpdateFull;
    m_OptionValue.m_ErrorIfEmitOutOfOrder = MDErrorOutOfOrderDefault;
    m_OptionValue.m_ThreadSafetyOptions = MDThreadSafetyDefault;
    m_OptionValue.m_GenerateTCEAdapters = FALSE;
    m_OptionValue.m_ImportOption = MDImportOptionDefault;
    m_OptionValue.m_LinkerOption = MDAssembly;
    m_OptionValue.m_RuntimeVersion = NULL;

} // Disp::Disp()

Disp::~Disp()
{
    if (m_OptionValue.m_RuntimeVersion)
        free(m_OptionValue.m_RuntimeVersion);
    if (m_Namespace)
        free(m_Namespace);
} // Disp::~Disp()

//*****************************************************************************
// Create a brand new scope.  This is based on the CLSID that was used to get
// the dispenser.
//*****************************************************************************
HRESULT Disp::DefineScope(              // Return code.
    REFCLSID    rclsid,                 // [in] What version to create.
    DWORD       dwCreateFlags,          // [in] Flags on the create.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk)               // [out] Return interface on success.
{
    RegMeta     *pMeta = 0;
    HRESULT     hr = NOERROR;

    LOG((LF_METADATA, LL_INFO10, "Disp::DefineScope(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", rclsid, dwCreateFlags, riid, ppIUnk));

    // If it is a version we don't understand, then we cannot continue.
    if (rclsid != CLSID_CorMetaDataRuntime)
        return (CLDB_E_FILE_OLDVER);
        
    if (dwCreateFlags)
        return E_INVALIDARG;

// Testers need this flag for their tests.

    // Create a new coclass for this guy.
    pMeta = new RegMeta(&m_OptionValue);
    IfNullGo( pMeta );

    // Remember the open type.
    pMeta->SetScopeType(DefineForWrite);

    IfFailGo(pMeta->Init());

    // Get the requested interface.
    IfFailGo(pMeta->QueryInterface(riid, (void **) ppIUnk));
    
    // Create the MiniMd-style scope.
    IfFailGo(pMeta->PostInitForWrite());

    // Add the new RegMeta to the cache.
    IfFailGo(pMeta->AddToCache());
    
    LOG((LOGMD, "{%08x} Created new emit scope\n", pMeta));

ErrExit:
    if (FAILED(hr))
    {
        if (pMeta) delete pMeta;
    }
    return (hr);
} // HRESULT Disp::DefineScope()


//*****************************************************************************
// Open an existing scope.
//*****************************************************************************
HRESULT Disp::OpenScope(                // Return code.
    LPCWSTR     szFileName,             // [in] The scope to open.
    DWORD       dwOpenFlags,            // [in] Open mode flags.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk)               // [out] Return interface on success.
{
    RegMeta     *pMeta = 0;
    HRESULT     hr;
    bool        bCompressed = false;    // If true, opened a MiniMd.
    bool        bWriteable;             // If true, open for write.
    HRESULT     hrOld = NOERROR;        // Saved failure code from attempted open.

    LOG((LF_METADATA, LL_INFO10, "Disp::OpenScope(%S, 0x%08x, 0x%08x, 0x%08x)\n", MDSTR(szFileName), dwOpenFlags, riid, ppIUnk));

    // Validate that there is some sort of file name.
    if (!szFileName || !szFileName[0] || !ppIUnk)
        return E_INVALIDARG;

    // Create a new coclass for this guy.
    pMeta = new RegMeta(&m_OptionValue);
    IfNullGo( pMeta );

    bWriteable = (dwOpenFlags & ofWrite) != 0;
    pMeta->SetScopeType(bWriteable ? OpenForWrite : OpenForRead);

    // Always initialize the RegMeta's stgdb. 
    if (memcmp(szFileName, L"file:", 10) == 0)
	{
        szFileName = &szFileName[5];
	}

    // Try to open the MiniMd-style scope.
    hr = pMeta->PostInitForRead(szFileName, 0,0,0, false);

    bCompressed = (hr == S_OK);


    // Check the return code from most recent operation.
    IfFailGo( hr );

    // Let the RegMeta cache the scope.
    pMeta->Init();

    // call PostOpen
    IfFailGo( pMeta->PostOpen() );

    // Return the requested interface.
    IfFailGo( pMeta->QueryInterface(riid, (void **) ppIUnk) );

    // Add the new RegMeta to the cache.
    IfFailGo(pMeta->AddToCache());
    
    LOG((LOGMD, "{%08x} Successfully opened '%S'\n", pMeta, MDSTR(szFileName)));

ErrExit:
    if (FAILED(hr))
    {
        if (pMeta) delete pMeta;
    }
    if (FAILED(hr) && FAILED(hrOld))
        return hrOld;
    return (hr);
} // HRESULT Disp::OpenScope()


//*****************************************************************************
// Open an existing scope.
//*****************************************************************************
HRESULT Disp::OpenScopeOnMemory(        // Return code.
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData,                 // [in] Size of the data pointed to by pData.
    DWORD       dwOpenFlags,            // [in] Open mode flags.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk)               // [out] Return interface on success.
{
    RegMeta     *pMeta = 0;
    HRESULT     hr;
    bool        bCompressed = false;    // If true, opened a MiniMd.
    bool        bWriteable;             // If true, open for write.
    bool        fFreeMemory = false;    // set to true if we make a copy of the memory that host passes in.
    void        *pbData = const_cast<void*>(pData);

    LOG((LF_METADATA, LL_INFO10, "Disp::OpenScopeOnMemory(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", pData, cbData, dwOpenFlags, riid, ppIUnk));

    // Create a new coclass for this guy.
    pMeta = new RegMeta(&m_OptionValue);
    if (!pMeta)
        return (OutOfMemory());

    bWriteable = (dwOpenFlags & ofWrite) != 0;
    pMeta->SetScopeType(bWriteable ? OpenForWrite : OpenForRead);

    // Always initialize the RegMeta's stgdb. 
    if (dwOpenFlags &  ofCopyMemory)
    {
        fFreeMemory = true;
        pbData = malloc(cbData);
        IfNullGo(pbData);
        memcpy(pbData, pData, cbData);
    }
    hr = pMeta->PostInitForRead(0, pbData, cbData, 0, fFreeMemory);
    bCompressed = (hr == S_OK);
    IfFailGo( hr );

    // Let the RegMeta cache the scope.
    pMeta->Init();

    // call PostOpen to cache the global typedef.
    IfFailGo( pMeta->PostOpen() );

    // Return the requested interface.
    IfFailGo( pMeta->QueryInterface(riid, (void **) ppIUnk) );

    // Add the new RegMeta to the cache.
    IfFailGo(pMeta->AddToCache());
    
    LOG((LOGMD, "{%08x} Opened new scope on memory, pData: %08x    cbData: %08x\n", pMeta, pData, cbData));

ErrExit:
    if (FAILED(hr))
    {
        if (pMeta) delete pMeta;
    }
    return (hr);
} // HRESULT Disp::OpenScopeOnMemory()


//*****************************************************************************
// Get the directory where the CLR system resides.
//*****************************************************************************
HRESULT Disp::GetCORSystemDirectory(    // Return code.
     LPWSTR     szBuffer,               // [out] Buffer for the directory name
     DWORD      cchBuffer,              // [in] Size of the buffer
     DWORD      *pchBuffer)             // [OUT] Number of characters returned
{
     HRESULT    hr;                     // A result.
     if (!pchBuffer)
         return E_INVALIDARG;
    
     IfFailRet(SetInternalSystemDirectory());

     DWORD lgth = cchBuffer;
     hr = ::GetInternalSystemDirectory(szBuffer, &lgth);
     *pchBuffer = lgth;
     return hr;
} // HRESULT Disp::GetCORSystemDirectory()


HRESULT Disp::FindAssembly(             // S_OK or error
    LPCWSTR     szAppBase,              // [IN] optional - can be NULL
    LPCWSTR     szPrivateBin,           // [IN] optional - can be NULL
    LPCWSTR     szGlobalBin,            // [IN] optional - can be NULL
    LPCWSTR     szAssemblyName,         // [IN] required - this is the assembly you are requesting
    LPCWSTR     szName,                 // [OUT] buffer - to hold name 
    ULONG       cchName,                // [IN] the name buffer's size
    ULONG       *pcName)                // [OUT] the number of characters returend in the buffer
{
    return E_NOTIMPL;
} // HRESULT Disp::FindAssembly()

HRESULT Disp::FindAssemblyModule(       // S_OK or error
    LPCWSTR     szAppBase,              // [IN] optional - can be NULL
    LPCWSTR     szPrivateBin,           // [IN] optional - can be NULL
    LPCWSTR     szGlobalBin,            // [IN] optional - can be NULL
    LPCWSTR     szAssemblyName,         // [IN] The assembly name or code base of the assembly
    LPCWSTR     szModuleName,           // [IN] required - the name of the module
    LPWSTR      szName,                 // [OUT] buffer - to hold name 
    ULONG       cchName,                // [IN]  the name buffer's size
    ULONG       *pcName)                // [OUT] the number of characters returend in the buffer
{
    return E_NOTIMPL;
} // HRESULT Disp::FindAssemblyModule()

//*****************************************************************************
// Open a scope on an ITypeInfo
//*****************************************************************************
HRESULT Disp::OpenScopeOnITypeInfo(     // Return code.
    ITypeInfo   *pITI,                  // [in] ITypeInfo to open.
    DWORD       dwOpenFlags,            // [in] Open mode flags.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk)               // [out] Return interface on success.
{

    _ASSERTE(false);
    return E_NOTIMPL;

} // HRESULT Disp::OpenScopeOnITypeInfo()


//*****************************************************************************
// IUnknown
//*****************************************************************************

ULONG Disp::AddRef()
{
    return InterlockedIncrement(&m_cRef);
} // ULONG Disp::AddRef()

ULONG Disp::Release()
{
    ULONG   cRef = InterlockedDecrement(&m_cRef);
    if (!cRef)
        delete this;
    return (cRef);
} // ULONG Disp::Release()

HRESULT Disp::QueryInterface(REFIID riid, void **ppUnk)
{
    *ppUnk = 0;

    if (riid == IID_IUnknown)
        *ppUnk = (IUnknown *) (IMetaDataDispenser *) this;
    else if (riid == IID_IMetaDataDispenser)
        *ppUnk = (IMetaDataDispenser *) this;
    else if (riid == IID_IMetaDataDispenserEx)
        *ppUnk = (IMetaDataDispenserEx *) this;
    else
        return (E_NOINTERFACE);
    AddRef();
    return (S_OK);
} // HRESULT Disp::QueryInterface()


//*****************************************************************************
// Called by the class factory template to create a new instance of this object.
//*****************************************************************************
HRESULT Disp::CreateObject(REFIID riid, void **ppUnk)
{ 
    HRESULT     hr;
    Disp *pDisp = new Disp();

    if (pDisp == 0)
        return (E_OUTOFMEMORY);

    hr = pDisp->QueryInterface(riid, ppUnk);
    if (FAILED(hr))
        delete pDisp;
    return (hr);
} // HRESULT Disp::CreateObject()

//*****************************************************************************
// This routine provides the user a way to set certain properties on the
// Dispenser.
//*****************************************************************************
HRESULT Disp::SetOption(                // Return code.
    REFGUID     optionid,               // [in] GUID for the option to be set.
    const VARIANT *pvalue)              // [in] Value to which the option is to be set.
{
    LOG((LF_METADATA, LL_INFO10, "Disp::SetOption(0x%08x, 0x%08x)\n", optionid, pvalue));

    if (optionid == MetaDataCheckDuplicatesFor)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_DupCheck = (CorCheckDuplicatesFor) V_UI4(pvalue);
    }
    else if (optionid == MetaDataRefToDefCheck)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_RefToDefCheck = (CorRefToDefCheck) V_UI4(pvalue);
    }
    else if (optionid == MetaDataNotificationForTokenMovement)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_NotifyRemap = (CorNotificationForTokenMovement)V_UI4(pvalue);
    }
    else if (optionid == MetaDataSetENC)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_UpdateMode = V_UI4(pvalue);
    }
    else if (optionid == MetaDataErrorIfEmitOutOfOrder)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_ErrorIfEmitOutOfOrder = (CorErrorIfEmitOutOfOrder) V_UI4(pvalue);
    }
    else if (optionid == MetaDataImportOption)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_ImportOption = (CorImportOptions) V_UI4(pvalue);
    }
    else if (optionid == MetaDataThreadSafetyOptions)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_ThreadSafetyOptions = (CorThreadSafetyOptions) V_UI4(pvalue);
    }
    else if (optionid == MetaDataGenerateTCEAdapters)
    {
        if (V_VT(pvalue) != VT_BOOL)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_GenerateTCEAdapters = V_BOOL(pvalue);
    }
    else if (optionid == MetaDataTypeLibImportNamespace)
    {
        if (V_VT(pvalue) != VT_BSTR && V_VT(pvalue) != VT_EMPTY && V_VT(pvalue) != VT_NULL)
        {
            _ASSERTE(!"Invalid Variant Type value for namespace.");
            return E_INVALIDARG;
        }
        if (m_Namespace)
            free(m_Namespace);
        if (V_VT(pvalue) == VT_EMPTY || V_VT(pvalue) == VT_NULL || V_BSTR(pvalue) == 0 || *V_BSTR(pvalue) == 0)
            m_Namespace = 0;
        else
        {
            m_Namespace = reinterpret_cast<WCHAR*>(malloc( sizeof(WCHAR) * (1 + wcslen(V_BSTR(pvalue))) ) );
            if (m_Namespace == 0)
                return E_OUTOFMEMORY;
            wcscpy(m_Namespace, V_BSTR(pvalue));
        }
    }
    else if (optionid == MetaDataLinkerOptions)
    {
        if (V_VT(pvalue) != VT_UI4)
        {
            _ASSERTE(!"Invalid Variant Type value!");
            return E_INVALIDARG;
        }
        m_OptionValue.m_LinkerOption = (CorLinkerOptions) V_UI4(pvalue);
    }
    else if (optionid == MetaDataRuntimeVersion)
    {
        if (V_VT(pvalue) != VT_BSTR && V_VT(pvalue) != VT_EMPTY && V_VT(pvalue) != VT_NULL)
        {
            _ASSERTE(!"Invalid Variant Type value for version.");
            return E_INVALIDARG;
        }
        if (m_OptionValue.m_RuntimeVersion)
            free(m_OptionValue.m_RuntimeVersion);

        if (V_VT(pvalue) == VT_EMPTY || V_VT(pvalue) == VT_NULL || V_BSTR(pvalue) == 0 || *V_BSTR(pvalue) == 0)
            m_RuntimeVersion = 0;
        else {
            INT32 len = WszWideCharToMultiByte(CP_UTF8, 0, V_BSTR(pvalue), -1, NULL, 0, NULL, NULL);
            m_OptionValue.m_RuntimeVersion = (LPSTR) malloc(len);
            if (m_OptionValue.m_RuntimeVersion == NULL)
                return E_OUTOFMEMORY;
            WszWideCharToMultiByte(CP_UTF8, 0, V_BSTR(pvalue), -1, m_OptionValue.m_RuntimeVersion, len, NULL, NULL);
        }
    }
    else
    {
        _ASSERTE(!"Invalid GUID");
        return E_INVALIDARG;
    }
    return S_OK;
} // HRESULT Disp::SetOption()

//*****************************************************************************
// This routine provides the user a way to set certain properties on the
// Dispenser.
//*****************************************************************************
HRESULT Disp::GetOption(                // Return code.
    REFGUID     optionid,               // [in] GUID for the option to be set.
    VARIANT *pvalue)                    // [out] Value to which the option is currently set.
{
    LOG((LF_METADATA, LL_INFO10, "Disp::GetOption(0x%08x, 0x%08x)\n", optionid, pvalue));

    _ASSERTE(pvalue);
    if (optionid == MetaDataCheckDuplicatesFor)
    {
        V_VT(pvalue) = VT_UI4;
        V_UI4(pvalue) = m_OptionValue.m_DupCheck;
    }
    else if (optionid == MetaDataRefToDefCheck)
    {
        V_VT(pvalue) = VT_UI4;
        V_UI4(pvalue) = m_OptionValue.m_RefToDefCheck;
    }
    else if (optionid == MetaDataNotificationForTokenMovement)
    {
        V_VT(pvalue) = VT_UI4;
        V_UI4(pvalue) = m_OptionValue.m_NotifyRemap;
    }
    else if (optionid == MetaDataSetENC)
    {
        V_VT(pvalue) = VT_UI4;
        V_UI4(pvalue) = m_OptionValue.m_UpdateMode;
    }
    else if (optionid == MetaDataErrorIfEmitOutOfOrder)
    {
        V_VT(pvalue) = VT_UI4;
        V_UI4(pvalue) = m_OptionValue.m_ErrorIfEmitOutOfOrder;
    }
    else if (optionid == MetaDataGenerateTCEAdapters)
    {
        V_VT(pvalue) = VT_BOOL;
        V_BOOL(pvalue) = m_OptionValue.m_GenerateTCEAdapters;
    }
    else if (optionid == MetaDataLinkerOptions)
    {
        V_VT(pvalue) = VT_BOOL;
        V_UI4(pvalue) = m_OptionValue.m_LinkerOption;
    }
    else
    {
        _ASSERTE(!"Invalid GUID");
        return E_INVALIDARG;
    }
    return S_OK;
} // HRESULT Disp::GetOption()


//*****************************************************************************
// Process attach initialization.
//*****************************************************************************
static DWORD LoadedModulesReadWriteLock[sizeof(UTSemReadWrite)/sizeof(DWORD) + sizeof(DWORD)];
void InitMd()
{
    LOADEDMODULES::m_pSemReadWrite = new((void*)LoadedModulesReadWriteLock) UTSemReadWrite;
} // void InitMd()

//*****************************************************************************
// Process attach cleanup.
//*****************************************************************************
void UninitMd()
{
    if (LOADEDMODULES::m_pSemReadWrite)
    {
        LOADEDMODULES::m_pSemReadWrite->~UTSemReadWrite();
        LOADEDMODULES::m_pSemReadWrite = 0;
    }
} // void UninitMd()


// EOF

