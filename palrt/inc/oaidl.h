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
// File: oaidl.h
// 
// ===========================================================================

#ifndef __OAIDL_H__
#define __OAIDL_H__

#include "rpc.h"
#include "rpcndr.h"

#include "unknwn.h"

typedef interface IErrorInfo IErrorInfo;
typedef /* [unique] */ IErrorInfo *LPERRORINFO;

EXTERN_C const IID IID_IErrorInfo;

    interface
    IErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetGUID( 
            /* [out] */ GUID *pGUID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSource( 
            /* [out] */ BSTR *pBstrSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [out] */ BSTR *pBstrDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHelpFile( 
            /* [out] */ BSTR *pBstrHelpFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHelpContext( 
            /* [out] */ DWORD *pdwHelpContext) = 0;
        
    };
    
typedef interface ICreateErrorInfo ICreateErrorInfo;

EXTERN_C const IID IID_ICreateErrorInfo;

typedef /* [unique] */ ICreateErrorInfo *LPCREATEERRORINFO;

    interface
    ICreateErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGUID( 
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSource( 
            /* [in] */ LPOLESTR szSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDescription( 
            /* [in] */ LPOLESTR szDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpFile( 
            /* [in] */ LPOLESTR szHelpFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpContext( 
            /* [in] */ DWORD dwHelpContext) = 0;
        
    };
    
STDAPI
SetErrorInfo(ULONG dwReserved, IErrorInfo FAR* perrinfo);

STDAPI
GetErrorInfo(ULONG dwReserved, IErrorInfo FAR* FAR* pperrinfo);

STDAPI
CreateErrorInfo(ICreateErrorInfo FAR* FAR* pperrinfo);


typedef interface ISupportErrorInfo ISupportErrorInfo;

typedef /* [unique] */ ISupportErrorInfo *LPSUPPORTERRORINFO;

EXTERN_C const IID IID_ISupportErrorInfo;

    
    interface
    ISupportErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo( 
            /* [in] */ REFIID riid) = 0;
        
    };

#endif //__OAIDL_H__
