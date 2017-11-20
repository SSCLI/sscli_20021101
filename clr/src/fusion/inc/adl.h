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
#ifndef __ADL_H_
#define __ADL_H_

#include "asmcache.h"
#include "clbind.h"
#include "list.h"
#include "fuspriv.h"
#include "dbglog.h"

typedef enum tagADLSTATE {
    ADLSTATE_INITIALIZE,
    ADLSTATE_DOWNLOADING,
    ADLSTATE_ABORT,
    ADLSTATE_DOWNLOAD_COMPLETE,
    ADLSTATE_SETUP,
    ADLSTATE_COMPLETE_ALL,
    ADLSTATE_DONE
} ADLSTATE;

//
// CAssemblyDownload
//

class CAssemblyDownload {
    public:

        CAssemblyDownload(ICodebaseList *pCodebaseList, IDownloadMgr *pDLMgr,
                          CDebugLog *pdbglog, LONGLONG llFlags);
        virtual ~CAssemblyDownload();

        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        static HRESULT Create(CAssemblyDownload **ppadl,
                              IDownloadMgr *pDLMgr,
                              ICodebaseList *pCodebaseList,
                              CDebugLog *pdbglog,
                              LONGLONG llFlags);

        HRESULT AddClient(IAssemblyBindSink *pAsmBindSink, BOOL bCallStartBinding);
        HRESULT AddClient(CClientBinding *pclient, BOOL bCallStartBinding);

        HRESULT SetUrl(LPCWSTR pwzUrl);

        HRESULT KickOffDownload(BOOL bFirstDownload);
        HRESULT DownloadComplete(HRESULT hrResult, LPOLESTR pwzFileName, const FILETIME *pftLastMod,
                                 BOOL bTerminate);
        HRESULT DoSetup(LPOLESTR pwzFileName, const FILETIME *pftLastMod);
        HRESULT CompleteAll(IUnknown *pUnk);
        HRESULT ClientAbort(CClientBinding *pclient);
        HRESULT FatalAbort(HRESULT hrResult);

        HRESULT ReportProgress(ULONG ulStatusCode, ULONG ulProgress,
                               ULONG ulProgressMax, DWORD dwNotification,
                               LPCWSTR wzNotification,
                               HRESULT hrNotification);

        HRESULT PreDownload(BOOL bCallCompleteAll, void **ppv);

        HRESULT GetDownloadMgr(IDownloadMgr **ppDLMgr);
        HRESULT SetResult(HRESULT hrResult);
                               
    private:
        HRESULT RealAbort(CClientBinding *pclient);
        HRESULT PrepNextDownload(LPWSTR pwzNextCodebase);
        HRESULT DownloadNextCodebase();
        HRESULT DoSetupModule(LPOLESTR pwzFileName,
                              IAssemblyModuleImport **ppModImport);
        HRESULT DoSetupAssembly(LPOLESTR pwzFileName, IAssembly **ppAsm);
        HRESULT LookupFromGlobalCache(LPASSEMBLY *ppAsmOut);
        HRESULT CheckDuplicate();
        HRESULT GetNextCodebase(BOOL *pbIsFileUrl, LPWSTR wzFilePath,
                                DWORD cbLen);

    private:
        DWORD                                         _dwSig;
        LONG                                          _cRef;
        HRESULT                                       _hrResult;
        ADLSTATE                                      _state;
        List<CClientBinding *>                        _clientList;
        LPWSTR                                        _pwzUrl;
        IDownloadMgr                                 *_pDLMgr;
        ICodebaseList                                *_pCodebaseList;
        CDebugLog                                    *_pdbglog;
        CRITICAL_SECTION                              _cs;
        LONGLONG                                      _llFlags;
};

#endif  // __ADL_H_
