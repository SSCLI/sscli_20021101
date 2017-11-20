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
#ifndef __FSTREAM_H_INCLUDED__
#define __FSTREAM_H_INCLUDED__

class CFileStream : public IStream
{
    public:
        CFileStream();
        virtual ~CFileStream();

        HRESULT OpenForRead(LPCWSTR wzFilePath);

        // IUnknown methods:
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

        // ISequentialStream methods:
        STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
        STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten);
    
        // IStream methods:
        STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
        STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
        STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
        STDMETHODIMP Commit(DWORD grfCommitFlags);
        STDMETHODIMP Revert();
        STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
        STDMETHODIMP Clone(IStream **ppIStream);

    private:
        BOOL Close();

    private:
        LONG                                _cRef;
        HANDLE                              _hFile;
    
};

#endif

