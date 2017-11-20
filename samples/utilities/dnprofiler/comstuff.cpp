#include <windows.h>
#include <stdio.h>
#include "profilercallback.h"

//=============================================================
#define ARRAY_SIZE( s ) (sizeof( s ) / sizeof( s[0] ))
#define MAX_LENGTH 256

#define PROFILER_GUID "{9AB84088-18E7-42F0-8F8D-E022AE3C4517}"


#define COM_METHOD( TYPE ) TYPE STDMETHODCALLTYPE

HINSTANCE g_hInst;        // instance handle to this piece of code

//==========================================================

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{    
    // save off the instance handle for later use
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hInstance );
            g_hInst = hInstance;
            break;        
    } 
        
    return TRUE;
}

//================================================================

class CClassFactory : public IClassFactory
{
    public:
        CClassFactory( ){ m_refCount = 1; }
    
        COM_METHOD( ULONG ) AddRef()
                            {
                                return InterlockedIncrement( &m_refCount );
                            }
        COM_METHOD( ULONG ) Release()
                            {
                                return InterlockedDecrement( &m_refCount );
                            }

        COM_METHOD( HRESULT ) QueryInterface(REFIID riid, void **ppInterface );         
        
        // IClassFactory methods
        COM_METHOD( HRESULT ) LockServer( BOOL fLock ) { return S_OK; }
        COM_METHOD( HRESULT ) CreateInstance( IUnknown *pUnkOuter,
                                              REFIID riid,
                                              void **ppInterface );
        
    private:
    
        LONG m_refCount;                        
};

CClassFactory *g_pProfilerClassFactory = NULL;


//================================================================
STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv )                  
{    
    HRESULT hr = E_OUTOFMEMORY;

const GUID CLSID_PROFILER = 
        { 0x9AB84088, 0x18E7, 0x42F0,
        { 0x8F, 0x8D, 0xE0, 0x22, 0xAE, 0x3C, 0x45, 0x17 } };

    if ( rclsid == CLSID_PROFILER )
    {
        if (g_pProfilerClassFactory == NULL)
        {
            g_pProfilerClassFactory = new CClassFactory();
        }        
        hr = g_pProfilerClassFactory->QueryInterface( riid, ppv );
    }

    return hr;   
}

//===========================================================

HRESULT CClassFactory::QueryInterface( REFIID riid, void **ppInterface )
{    
    if ( riid == IID_IUnknown )
        *ppInterface = static_cast<IUnknown *>( this ); 
    else if ( riid == IID_IClassFactory )
        *ppInterface = static_cast<IClassFactory *>( this );
    else
    {
        *ppInterface = NULL;                                  
        return E_NOINTERFACE;
    }
    
    reinterpret_cast<IUnknown *>( *ppInterface )->AddRef();
    
    return S_OK;
}

//===========================================================
HRESULT CClassFactory::CreateInstance( IUnknown *pUnkOuter, REFIID riid,
                                        void **ppInstance )
{       
    // aggregation is not supported by these objects
    if ( pUnkOuter != NULL )
        return CLASS_E_NOAGGREGATION;

    CProfilerCallback * pProfilerCallback = new CProfilerCallback;

    *ppInstance = (void *)pProfilerCallback;

    return S_OK;
}

