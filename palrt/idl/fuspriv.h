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

#ifdef _MSC_VER
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#endif

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __fuspriv_h__
#define __fuspriv_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICodebaseList_FWD_DEFINED__
#define __ICodebaseList_FWD_DEFINED__
typedef interface ICodebaseList ICodebaseList;
#endif 	/* __ICodebaseList_FWD_DEFINED__ */


#ifndef __IDownloadMgr_FWD_DEFINED__
#define __IDownloadMgr_FWD_DEFINED__
typedef interface IDownloadMgr IDownloadMgr;
#endif 	/* __IDownloadMgr_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_fuspriv_0000 */
/* [local] */ 


#ifdef _MSC_VER
#pragma comment(lib,"uuid.lib")
#endif

//---------------------------------------------------------------------------=
// Name Object Interfaces.



extern RPC_IF_HANDLE __MIDL_itf_fuspriv_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fuspriv_0000_v0_0_s_ifspec;

#ifndef __ICodebaseList_INTERFACE_DEFINED__
#define __ICodebaseList_INTERFACE_DEFINED__

/* interface ICodebaseList */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_ICodebaseList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D8FB9BD6-3969-11d3-B4AF-00C04F8ECB26")
    ICodebaseList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddCodebase( 
            /* [in] */ LPCWSTR wzCodebase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveCodebase( 
            /* [in] */ DWORD dwIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ DWORD *pdwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodebase( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR wzCodebase,
            /* [out][in] */ DWORD *pcbCodebase) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICodebaseListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICodebaseList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICodebaseList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICodebaseList * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddCodebase )( 
            ICodebaseList * This,
            /* [in] */ LPCWSTR wzCodebase);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveCodebase )( 
            ICodebaseList * This,
            /* [in] */ DWORD dwIndex);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveAll )( 
            ICodebaseList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ICodebaseList * This,
            /* [out] */ DWORD *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodebase )( 
            ICodebaseList * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR wzCodebase,
            /* [out][in] */ DWORD *pcbCodebase);
        
        END_INTERFACE
    } ICodebaseListVtbl;

    interface ICodebaseList
    {
        CONST_VTBL struct ICodebaseListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICodebaseList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICodebaseList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICodebaseList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICodebaseList_AddCodebase(This,wzCodebase)	\
    (This)->lpVtbl -> AddCodebase(This,wzCodebase)

#define ICodebaseList_RemoveCodebase(This,dwIndex)	\
    (This)->lpVtbl -> RemoveCodebase(This,dwIndex)

#define ICodebaseList_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define ICodebaseList_GetCount(This,pdwCount)	\
    (This)->lpVtbl -> GetCount(This,pdwCount)

#define ICodebaseList_GetCodebase(This,dwIndex,wzCodebase,pcbCodebase)	\
    (This)->lpVtbl -> GetCodebase(This,dwIndex,wzCodebase,pcbCodebase)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICodebaseList_AddCodebase_Proxy( 
    ICodebaseList * This,
    /* [in] */ LPCWSTR wzCodebase);


void __RPC_STUB ICodebaseList_AddCodebase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodebaseList_RemoveCodebase_Proxy( 
    ICodebaseList * This,
    /* [in] */ DWORD dwIndex);


void __RPC_STUB ICodebaseList_RemoveCodebase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodebaseList_RemoveAll_Proxy( 
    ICodebaseList * This);


void __RPC_STUB ICodebaseList_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodebaseList_GetCount_Proxy( 
    ICodebaseList * This,
    /* [out] */ DWORD *pdwCount);


void __RPC_STUB ICodebaseList_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodebaseList_GetCodebase_Proxy( 
    ICodebaseList * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ LPWSTR wzCodebase,
    /* [out][in] */ DWORD *pcbCodebase);


void __RPC_STUB ICodebaseList_GetCodebase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICodebaseList_INTERFACE_DEFINED__ */


#ifndef __IDownloadMgr_INTERFACE_DEFINED__
#define __IDownloadMgr_INTERFACE_DEFINED__

/* interface IDownloadMgr */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IDownloadMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0A6F16F8-ACD7-11d3-B4ED-00C04F8ECB26")
    IDownloadMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PreDownloadCheck( 
            /* [out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoSetup( 
            /* [in] */ LPCWSTR wzSourceUrl,
            /* [in] */ LPCWSTR wzFilePath,
            /* [in] */ const FILETIME *pftLastMod,
            /* [out] */ IUnknown **ppUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProbeFailed( 
            /* [out] */ IUnknown **ppUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDuplicate( 
            /* [out] */ IDownloadMgr *ppDLMgr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LogResult( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDownloadMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDownloadMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDownloadMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDownloadMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *PreDownloadCheck )( 
            IDownloadMgr * This,
            /* [out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *DoSetup )( 
            IDownloadMgr * This,
            /* [in] */ LPCWSTR wzSourceUrl,
            /* [in] */ LPCWSTR wzFilePath,
            /* [in] */ const FILETIME *pftLastMod,
            /* [out] */ IUnknown **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *ProbeFailed )( 
            IDownloadMgr * This,
            /* [out] */ IUnknown **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *IsDuplicate )( 
            IDownloadMgr * This,
            /* [out] */ IDownloadMgr *ppDLMgr);
        
        HRESULT ( STDMETHODCALLTYPE *LogResult )( 
            IDownloadMgr * This);
        
        END_INTERFACE
    } IDownloadMgrVtbl;

    interface IDownloadMgr
    {
        CONST_VTBL struct IDownloadMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDownloadMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDownloadMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDownloadMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDownloadMgr_PreDownloadCheck(This,ppv)	\
    (This)->lpVtbl -> PreDownloadCheck(This,ppv)

#define IDownloadMgr_DoSetup(This,wzSourceUrl,wzFilePath,pftLastMod,ppUnk)	\
    (This)->lpVtbl -> DoSetup(This,wzSourceUrl,wzFilePath,pftLastMod,ppUnk)

#define IDownloadMgr_ProbeFailed(This,ppUnk)	\
    (This)->lpVtbl -> ProbeFailed(This,ppUnk)

#define IDownloadMgr_IsDuplicate(This,ppDLMgr)	\
    (This)->lpVtbl -> IsDuplicate(This,ppDLMgr)

#define IDownloadMgr_LogResult(This)	\
    (This)->lpVtbl -> LogResult(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDownloadMgr_PreDownloadCheck_Proxy( 
    IDownloadMgr * This,
    /* [out] */ void **ppv);


void __RPC_STUB IDownloadMgr_PreDownloadCheck_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDownloadMgr_DoSetup_Proxy( 
    IDownloadMgr * This,
    /* [in] */ LPCWSTR wzSourceUrl,
    /* [in] */ LPCWSTR wzFilePath,
    /* [in] */ const FILETIME *pftLastMod,
    /* [out] */ IUnknown **ppUnk);


void __RPC_STUB IDownloadMgr_DoSetup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDownloadMgr_ProbeFailed_Proxy( 
    IDownloadMgr * This,
    /* [out] */ IUnknown **ppUnk);


void __RPC_STUB IDownloadMgr_ProbeFailed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDownloadMgr_IsDuplicate_Proxy( 
    IDownloadMgr * This,
    /* [out] */ IDownloadMgr *ppDLMgr);


void __RPC_STUB IDownloadMgr_IsDuplicate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDownloadMgr_LogResult_Proxy( 
    IDownloadMgr * This);


void __RPC_STUB IDownloadMgr_LogResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDownloadMgr_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


