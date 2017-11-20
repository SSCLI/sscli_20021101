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
#include "fusionp.h"
#include "fstream.h"

CFileStream::CFileStream()
: _cRef(1)
, _hFile(INVALID_HANDLE_VALUE)
{
}

CFileStream::~CFileStream()
{
    Close();
}

HRESULT CFileStream::OpenForRead(LPCWSTR wzFilePath)
{
    HRESULT                                    hr = S_OK;

    ASSERT(_hFile == INVALID_HANDLE_VALUE && wzFilePath);
    if (_hFile != INVALID_HANDLE_VALUE || !wzFilePath) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    _hFile = ::CreateFileW(wzFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (_hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT CFileStream::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                                    hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IStream)) {
        *ppv = static_cast<IStream *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CFileStream::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileStream::Release()
{
    ULONG                    ulRef = InterlockedDecrement(&_cRef);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

HRESULT CFileStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT                                   hr = S_OK;
    ULONG                                     cbRead = 0;

    if (pcbRead != NULL) {
        *pcbRead = 0;
    }

    ASSERT(_hFile != INVALID_HANDLE_VALUE);
    if (_hFile == INVALID_HANDLE_VALUE) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    if (!::ReadFile(_hFile, pv, cb, &cbRead, NULL)) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    if (cbRead == 0) {
        hr = S_FALSE;
    }
    else {
        hr = NOERROR;
    }

    if (pcbRead != NULL) {
        *pcbRead = cbRead;
    }

Exit:
    return hr;
}

HRESULT CFileStream::Write(void const *pv, ULONG cb, ULONG *pcbWritten)
{
    HRESULT                              hr = S_OK;
    ULONG                                cbWritten = 0;

    if (pcbWritten != NULL) {
        *pcbWritten = 0;
    }

    ASSERT(_hFile != INVALID_HANDLE_VALUE);
    if (_hFile == INVALID_HANDLE_VALUE) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    if (!::WriteFile(_hFile, pv, cb, &cbWritten, NULL)) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    if (cbWritten == 0) {
        hr = S_FALSE;
    }
    else {
        hr = S_OK;
    }

    if (pcbWritten != NULL) {
        *pcbWritten = cbWritten;
    }

Exit:
    return hr;
}

HRESULT CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    return E_NOTIMPL;
}

HRESULT CFileStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

HRESULT CFileStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    return E_NOTIMPL;
}

HRESULT CFileStream::Commit(DWORD grfCommitFlags)
{
    HRESULT                                 hr = S_OK;

    if (grfCommitFlags != 0)  {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!Close()) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

Exit:
    return hr;
}

HRESULT CFileStream::Revert()
{
    return E_NOTIMPL;
}

HRESULT CFileStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

HRESULT CFileStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

HRESULT CFileStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

HRESULT CFileStream::Clone(IStream **ppIStream)
{
    return E_NOTIMPL;
}


BOOL CFileStream::Close()
{
    BOOL                            fSuccess = FALSE;
    
    if (_hFile != INVALID_HANDLE_VALUE) {
        if (!::CloseHandle(_hFile)) {
            _hFile = INVALID_HANDLE_VALUE;
            goto Exit;
        }

        _hFile = INVALID_HANDLE_VALUE;
    }

    fSuccess = TRUE; 

Exit:
    return fSuccess;
}

