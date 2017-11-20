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
#ifndef ASMSTRM_H
#define ASMSTRM_H

#include <windows.h>
#include "wincrypt.h"
#include <winerror.h>
#include <objbase.h>
#include "fusionp.h"
#include "asmitem.h"

class CAssemblyCacheItem;

HRESULT DeleteAssemblyBits(LPCTSTR pszManifestPath);

HRESULT AssemblyCreateDirectory
   (OUT LPOLESTR pszDir, IN OUT LPDWORD pcwDir);

DWORD GetAssemblyStorePath (LPTSTR szPath);
HRESULT GetAssemblyStorePath(LPWSTR szPath, ULONG *pcch);
DWORD GetRandomDirName (LPTSTR szDirName);

class CAssemblyStream : public IStream
{
public:

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // *** IStream methods ***
    STDMETHOD(Read) (VOID *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write) (VOID const *pv, ULONG cb,
        ULONG *pcbWritten);
    STDMETHOD(Seek) (LARGE_INTEGER dlibMove, DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition);
    STDMETHOD (CheckHash) ();
    STDMETHOD (AddSizeToItem) ();
    STDMETHOD(SetSize) (ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo) (IStream *pStm, ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit) (DWORD dwCommitFlags);
    STDMETHOD(Revert) ();
    STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat) (STATSTG *pStatStg, DWORD grfStatFlag);
    STDMETHOD(Clone) (IStream **ppStm);

    CAssemblyStream (CAssemblyCacheItem* pParent);
    
    HRESULT Init (LPOLESTR pszPath, DWORD dwFormat);

    ~CAssemblyStream ();
    
private:

    void ReleaseParent (HRESULT hr);

    DWORD  _dwSig;
    LONG   _cRef;
    CAssemblyCacheItem* _pParent;
    HANDLE  _hf;
    TCHAR   _szPath[MAX_PATH];
    DWORD   _dwFormat;
    HCRYPTHASH      _hHash;
};
#endif
