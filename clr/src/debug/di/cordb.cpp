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
// CorDB.cpp
//
// Dll* routines for entry points, and support for COM framework.  The class
// factory and other routines live in this module.
//
//*****************************************************************************
#include "stdafx.h"
#include "classfactory.h"
#include "corsym.h"

// Helper function returns the instance handle of this module.
HINSTANCE GetModuleInst();

//********** Globals. *********************************************************
HINSTANCE       g_hInst;                // Instance handle to this piece of code.

// This map contains the list of coclasses which are exported from this module.
const COCLASS_REGISTER g_CoClasses[] =
{
//  pClsid              szProgID            pfnCreateObject
    { &CLSID_CorDebug,    L"CorDebug",        Cordb::CreateObject },
    { &CLSID_CorpubPublish,  L"CorpubPublish",  CorpubPublish::CreateObject },
    { NULL,               NULL,               NULL }
};


//********** Locals. **********************************************************


//********** Code. ************************************************************


//*****************************************************************************
// The main dll entry point for this module.  This routine is called by the
// OS when the dll gets loaded.  Control is simply deferred to the main code.
//*****************************************************************************
BOOL WINAPI DbgDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    // Save off the instance handle for later use.
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;

        // Init the Win32 wrappers.
        OnUnicodeSystem();
    
    }

    return TRUE;
}




//*****************************************************************************
// Called by COM to get a class factory for a given CLSID.  If it is one we
// support, instantiate a class factory object and prepare for create instance.
//*****************************************************************************
STDAPI DllGetClassObjectInternal(               // Return code.
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv)                   // Return interface pointer here.
{
    CClassFactory *pClassFactory;       // To create class factory object.
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    HRESULT     hr = CLASS_E_CLASSNOTAVAILABLE;

    // Scan for the right one.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        if (*pCoClass->pClsid == rclsid)
        {
            // Allocate the new factory object.
            pClassFactory = new CClassFactory(pCoClass);
            if (!pClassFactory)
                return (E_OUTOFMEMORY);

            // Pick the v-table based on the caller's request.
            hr = pClassFactory->QueryInterface(riid, ppv);

            // Always release the local reference, if QI failed it will be
            // the only one and the object gets freed.
            pClassFactory->Release();
            break;
        }
    }
    return (hr);
}



//*****************************************************************************
//
//********** Class factory code.
//
//*****************************************************************************


//*****************************************************************************
// QueryInterface is called to pick a v-table on the co-class.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE CClassFactory::QueryInterface( 
    REFIID      riid,
    void        **ppvObject)
{
    HRESULT     hr;

    // Avoid confusion.
    *ppvObject = NULL;

    // Pick the right v-table based on the IID passed in.
    if (riid == IID_IUnknown)
        *ppvObject = (IUnknown *) this;
    else if (riid == IID_IClassFactory)
        *ppvObject = (IClassFactory *) this;

    // If successful, add a reference for out pointer and return.
    if (*ppvObject)
    {
        hr = S_OK;
        AddRef();
    }
    else
        hr = E_NOINTERFACE;
    return (hr);
}


//*****************************************************************************
// CreateInstance is called to create a new instance of the coclass for which
// this class was created in the first place.  The returned pointer is the
// v-table matching the IID if there.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE CClassFactory::CreateInstance( 
    IUnknown    *pUnkOuter,
    REFIID      riid,
    void        **ppvObject)
{
    HRESULT     hr;

    // Avoid confusion.
    *ppvObject = NULL;
    _ASSERTE(m_pCoClass);

    // Aggregation is not supported by these objects.
    if (pUnkOuter)
        return (CLASS_E_NOAGGREGATION);

    // Ask the object to create an instance of itself, and check the iid.
    hr = (*m_pCoClass->pfnCreateObject)(riid, ppvObject);
    return (hr);
}


HRESULT STDMETHODCALLTYPE CClassFactory::LockServer( 
    BOOL        fLock)
{
    return (S_OK);
}





//*****************************************************************************
// This helper provides access to the instance handle of the loaded image.
//*****************************************************************************
HINSTANCE GetModuleInst()
{
    return g_hInst;
}
