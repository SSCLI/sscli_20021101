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
// StgTiggerStorage.cpp
//
// TiggerStorage is a stripped down version of compound doc files.  Doc files
// have some very useful and complex features to them, unfortunately nothing
// comes for free.  Given the incredibly tuned format of existing .tlb files,
// every single byte counts and 10% added by doc files is just too expensive.
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
#include "stgio.h"                      // I/O subsystem.
#include "stgtiggerstorage.h"           // Our interface.
#include "stgtiggerstream.h"            // Stream interface.
#include "corerror.h"
#include "posterror.h"
#include "mdfileformat.h"
#include "mscoree.h"


BYTE  TiggerStorage::m_Version[_MAX_PATH];
DWORD TiggerStorage::m_dwVersion = 0;
LPSTR TiggerStorage::m_szVersion = 0;
LONG TiggerStorage::m_flock = 0;

TiggerStorage::TiggerStorage() :
    m_pStgIO(0),
    m_cRef(1),
    m_pStreamList(0),
    m_pbExtra(0)
{
    memset(&m_StgHdr, 0, sizeof(STORAGEHEADER));
}


TiggerStorage::~TiggerStorage()
{
    if (m_pStgIO)
    {
        m_pStgIO->Release();
        m_pStgIO = 0;
    }
}


//*****************************************************************************
// Init this storage object on top of the given storage unit.
//*****************************************************************************
HRESULT TiggerStorage::Init(        // Return code.
    StgIO       *pStgIO,            // The I/O subsystem.
    LPSTR       pVersion)           // 'Compiled for' CLR version
{
    STORAGESIGNATURE *pSig;         // Signature data for file.
    ULONG       iOffset;            // Offset of header data.
    void        *ptr;               // Signature.
    HRESULT     hr = S_OK;

    // Make sure we always start at the beginning.
    pStgIO->Seek(0, FILE_BEGIN);

    // Save the storage unit.
    m_pStgIO = pStgIO;
    m_pStgIO->AddRef();

    // For cases where the data already exists, verify the signature.
    if ((pStgIO->GetFlags() & DBPROP_TMODEF_CREATE) == 0)
    {
        // Map the contents into memory for easy access.
        IfFailGo( pStgIO->MapFileToMem(ptr, &iOffset) );

        // Get a pointer to the signature of the file, which is the first part.
        IfFailGo( pStgIO->GetPtrForMem(0, sizeof(STORAGESIGNATURE), ptr) );

        // Finally, we can check the signature.
        pSig = (STORAGESIGNATURE *) ptr;
        IfFailGo( MDFormat::VerifySignature(pSig) );

        // Read and verify the header.
            IfFailGo( ReadHeader() );
    }
    // For write case, dump the signature into the file up front.
    else
    {
        IfFailGo( WriteSignature() );
    }

    m_szVersion = pVersion;

ErrExit:
    if (FAILED(hr) && m_pStgIO)
    {
        m_pStgIO->Release();
        m_pStgIO = 0;
    }
    return (hr);
}


//*****************************************************************************
//  Get the size of the storage signature including the version of the runtime
//  used to emit the meta-data
//*****************************************************************************
DWORD TiggerStorage::SizeOfStorageSignature()
{
    if(m_dwVersion == 0) {
        EnterStorageLock();
        if(m_dwVersion == 0) {
            DWORD dwSize = 0;
            memset(m_Version,0,sizeof(m_Version));
            if (m_szVersion) {
                dwSize = (DWORD)strlen(m_szVersion)+1; // m_dwVersion includes the 0 terminator
                memcpy(m_Version, m_szVersion, dwSize);
            }
            else {
                WCHAR version[_MAX_PATH];
                DWORD dwVersion = 0;
                GetCORRequiredVersion(version, _MAX_PATH, &dwVersion);
                if(dwVersion > 0) {
                    dwSize = WszWideCharToMultiByte(CP_UTF8, 0, version, -1, (LPSTR) m_Version, _MAX_PATH, NULL, NULL);
                    if(dwSize == 0)
                    {
                        _ASSERTE(!"WideCharToMultiByte conversion failed");
                        *((DWORD*)m_Version)=0; // DWORD to make it 4 bytes
                    }
                }
            }

            // Make sure max_path is 4 byte aligned so we never exceed it
            _ASSERTE((_MAX_PATH & 0x3) == 0);
            m_dwVersion = (dwSize + 0x3) & ~0x3;         // Make the string length four byte aligned
        }
        LeaveStorageLock();
    }

    _ASSERTE(m_dwVersion);
    return sizeof(STORAGESIGNATURE) + m_dwVersion;
}


//*****************************************************************************
// Retrieves a the size and a pointer to the extra data that can optionally be
// written in the header of the storage system.  This data is not required to
// be in the file, in which case *pcbExtra will come back as 0 and pbData will
// be set to null.  You must have initialized the storage using Init() before
// calling this function.
//*****************************************************************************
HRESULT TiggerStorage::GetExtraData(    // S_OK if found, S_FALSE, or error.
    ULONG       *pcbExtra,              // Return size of extra data.
    BYTE        *&pbData)               // Return a pointer to extra data.
{
    // Assuming there is extra data, then return the size and a pointer to it.
    if (m_pbExtra)
    {
        if (!(m_StgHdr.fFlags & STGHDR_EXTRADATA))
	        return (PostError(CLDB_E_FILE_CORRUPT));
        *pcbExtra = *(ULONG *) m_pbExtra;
        pbData = (BYTE *) ((ULONG *) m_pbExtra + 1);
    }
    else
    {
        *pcbExtra = 0;
        pbData = 0;
        return (S_FALSE);
    }
    return (S_OK);
}


//*****************************************************************************
// Called when this stream is going away.
//*****************************************************************************
HRESULT TiggerStorage::WriteHeader(
    STORAGESTREAMLST *pList,            // List of streams.     
    ULONG       cbExtraData,            // Size of extra data, may be 0.
    BYTE        *pbExtraData)           // Pointer to extra data for header.
{
    ULONG       iLen;                   // For variable sized data.
    ULONG       cbWritten;              // Track write quantity.
    HRESULT     hr;
    SAVETRACE(ULONG cbDebugSize);       // Track debug size of header.

    SAVETRACE(DbgWriteEx(L"PSS:  Header:\n"));

    // Save the count and set flags.
    m_StgHdr.SetiStreams(pList->Count());
    if (cbExtraData)
        m_StgHdr.fFlags |= STGHDR_EXTRADATA;

    // Write out the header of the file.
    IfFailRet( m_pStgIO->Write(&m_StgHdr, sizeof(STORAGEHEADER), &cbWritten) );

    // Write out extra data if there is any.
    if (cbExtraData)
    {
        _ASSERTE(pbExtraData);
        _ASSERTE(cbExtraData % 4 == 0);

        // First write the length value.
        IfFailRet( m_pStgIO->Write(&cbExtraData, sizeof(ULONG), &cbWritten) );

        // And then the data.
        IfFailRet( m_pStgIO->Write(pbExtraData, cbExtraData, &cbWritten) );
        SAVETRACE(DbgWriteEx(L"PSS:    extra data size %d\n", m_pStgIO->GetCurrentOffset() - cbDebugSize);cbDebugSize=m_pStgIO->GetCurrentOffset());
    }
    
    // Save off each data stream.
    for (int i=0;  i<pList->Count();  i++)
    {
        STORAGESTREAM *pStream = pList->Get(i);

        // How big is the structure (aligned) for this struct.
        iLen = (ULONG)(sizeof(STORAGESTREAM) - MAXSTREAMNAME + strlen(pStream->rcName) + 1);

        // Write the header including the name to disk.  Does not include
        // full name buffer in struct, just string and null terminator.
        IfFailRet( m_pStgIO->Write(pStream, iLen, &cbWritten) );

        // Align the data out to 4 bytes.
        if (iLen != ALIGN4BYTE(iLen))
        {
            IfFailRet( m_pStgIO->Write(&hr, ALIGN4BYTE(iLen) - iLen, 0) );
        }
        SAVETRACE(DbgWriteEx(L"PSS:    Table %hs header size %d\n", pStream->rcName, m_pStgIO->GetCurrentOffset() - cbDebugSize);cbDebugSize=m_pStgIO->GetCurrentOffset());
    }
    SAVETRACE(DbgWriteEx(L"PSS:  Total size of header data %d\n", m_pStgIO->GetCurrentOffset()));
    // Make sure the whole thing is 4 byte aligned.
    _ASSERTE(m_pStgIO->GetCurrentOffset() % 4 == 0);
    return (S_OK);
}


//*****************************************************************************
// Called when all data has been written.  Forces cached data to be flushed
// and stream lists to be validated.
//*****************************************************************************
HRESULT TiggerStorage::WriteFinished(   // Return code.
    STORAGESTREAMLST *pList,            // List of streams.     
    ULONG       *pcbSaveSize)           // Return size of total data.
{
    STORAGESTREAM *pEntry;              // Loop control.
    HRESULT     hr;

    // If caller wants the total size of the file, we are there right now.
    if (pcbSaveSize)
        *pcbSaveSize = m_pStgIO->GetCurrentOffset();

    // Flush our internal write cache to disk.
    IfFailRet( m_pStgIO->FlushCache() );

    // Force user's data onto disk right now so that Commit() can be
    // more accurate (although not totally up to the D in ACID).
    hr = m_pStgIO->FlushFileBuffers();
    _ASSERTE(SUCCEEDED(hr));


    // Run through all of the streams and validate them against the expected
    // list we wrote out originally.

    // Robustness check: stream counts must match what was written.
    _ASSERTE(pList->Count() == m_Streams.Count());
    if (pList->Count() != m_Streams.Count())
    {
        _ASSERTE(0 && "Mismatch in streams, save would cause corruption.");
        return (PostError(CLDB_E_FILE_CORRUPT));
        
    }

    // Sanity check each saved stream data size and offset.
    for (int i=0;  i<pList->Count();  i++)
    {
        pEntry = pList->Get(i);
        _ASSERTE(pEntry->GetOffset() == m_Streams[i].GetOffset());
        _ASSERTE(pEntry->GetSize() == m_Streams[i].GetSize());
        _ASSERTE(strcmp(pEntry->rcName, m_Streams[i].rcName) == 0);

        // For robustness, check that everything matches expected value,
        // and if it does not, refuse to save the data and force a rollback.
        // The alternative is corruption of the data file.
        if (pEntry->GetOffset() != m_Streams[i].GetOffset() ||
            pEntry->GetSize() != m_Streams[i].GetSize() ||
            strcmp(pEntry->rcName, m_Streams[i].rcName) != 0)
        {
            _ASSERTE(0 && "Mismatch in streams, save would cause corruption.");
            hr = PostError(CLDB_E_FILE_CORRUPT);
            break;
        }

    }
    return (hr);
}


//*****************************************************************************
// Tells the storage that we intend to rewrite the contents of this file.  The
// entire file will be truncated and the next write will take place at the
// beginning of the file.
//*****************************************************************************
HRESULT TiggerStorage::Rewrite(         // Return code.
    LPWSTR      szBackup)               // If non-0, backup the file.
{
    HRESULT     hr;

    // Delegate to storage.
    IfFailRet( m_pStgIO->Rewrite(szBackup) );

    // None of the old streams make any sense any more.  Delete them.
    m_Streams.Clear();

    // Write the signature out.
    if (FAILED(hr = WriteSignature()))
    {
        HRESULT hrTemp;
        hrTemp = Restore(szBackup, false);
        _ASSERTE(hrTemp == S_OK);

        return (hr);
    }

    return (S_OK);
}


//*****************************************************************************
// Called after a successful rewrite of an existing file.  The in memory
// backing store is no longer valid because all new data is in memory and
// on disk.  This is essentially the same state as created, so free up some
// working set and remember this state.
//*****************************************************************************
HRESULT TiggerStorage::ResetBackingStore()  // Return code.
{
    return (m_pStgIO->ResetBackingStore());
}


//*****************************************************************************
// Called to restore the original file.  If this operation is successful, then
// the backup file is deleted as requested.  The restore of the file is done
// in write through mode to the disk help ensure the contents are not lost.
// This is not good enough to fulfill ACID props, but it ain't that bad.
//*****************************************************************************
HRESULT TiggerStorage::Restore(         // Return code.
    LPWSTR      szBackup,               // If non-0, backup the file.
    int         bDeleteOnSuccess)       // Delete backup file if successful.
{
    HRESULT     hr;

    // Ask file system to copy bytes from backup into master.
    IfFailRet( m_pStgIO->Restore(szBackup, bDeleteOnSuccess) );

    // Reset state.  The Init routine will re-read data as required.
    m_pStreamList = 0;
    m_StgHdr.SetiStreams(0);

    // Re-init all data structures as though we just opened.
    return (Init(m_pStgIO, m_szVersion));
}


//*****************************************************************************
// Given the name of a stream that will be persisted into a stream in this
// storage type, figure out how big that stream would be including the user's
// stream data and the header overhead the file format incurs.  The name is
// stored in ANSI and the header struct is aligned to 4 bytes.
//*****************************************************************************
HRESULT TiggerStorage::GetStreamSaveSize( // Return code.
    LPCWSTR     szStreamName,           // Name of stream.
    ULONG       cbDataSize,             // Size of data to go into stream.
    ULONG       *pcbSaveSize)           // Return data size plus stream overhead.
{
    ULONG       cbTotalSize;            // Add up each element.
    
    // Find out how large the name will be.
    cbTotalSize = ::WszWideCharToMultiByte(CP_ACP, 0, szStreamName, -1, 0, 0, 0, 0);
    _ASSERTE(cbTotalSize);

    // Add the size of the stream header minus the static name array.
    cbTotalSize += sizeof(STORAGESTREAM) - MAXSTREAMNAME;

    // Finally align the header value.
    cbTotalSize = ALIGN4BYTE(cbTotalSize);

    // Return the size of the user data and the header data.
    *pcbSaveSize = cbTotalSize + cbDataSize;
    return (S_OK);
}


//*****************************************************************************
// Return the fixed size overhead for the storage implementation.  This includes
// the signature and fixed header overhead.  The overhead in the header for each
// stream is calculated as part of GetStreamSaveSize because these structs are
// variable sized on the name.
//*****************************************************************************
HRESULT TiggerStorage::GetStorageSaveSize( // Return code.
    ULONG       *pcbSaveSize,           // [in] current size, [out] plus overhead.
    ULONG       cbExtra)                // How much extra data to store in header.
{
    *pcbSaveSize += SizeOfStorageSignature() + sizeof(STORAGEHEADER);
    if (cbExtra)
        *pcbSaveSize += sizeof(ULONG) + cbExtra;
    return (S_OK);
}


//*****************************************************************************
// Adjust the offset in each known stream to match where it will wind up after
// a save operation.
//*****************************************************************************
HRESULT TiggerStorage::CalcOffsets(     // Return code.
    STORAGESTREAMLST *pStreamList,      // List of streams for header.
    ULONG       cbExtra)                // Size of variable extra data in header.
{
    STORAGESTREAM *pEntry;              // Each entry in the list.
    ULONG       cbOffset=0;             // Running offset for streams.
    int         i;                      // Loop control.

    // Prime offset up front.
    GetStorageSaveSize(&cbOffset, cbExtra);

    // Add on the size of each header entry.
    for (i=0;  i<pStreamList->Count();  i++)
    {
        VERIFY(pEntry = pStreamList->Get(i));
        cbOffset += sizeof(STORAGESTREAM) - MAXSTREAMNAME;
        cbOffset += (ULONG)(strlen(pEntry->rcName) + 1);
        cbOffset = ALIGN4BYTE(cbOffset);
    }

    // Go through each stream and reset its expected offset.
    for (i=0;  i<pStreamList->Count();  i++)
    {
        VERIFY(pEntry = pStreamList->Get(i));
        pEntry->SetOffset(cbOffset);
        cbOffset += pEntry->GetSize();
    }
    return (S_OK);
}   



HRESULT STDMETHODCALLTYPE TiggerStorage::CreateStream( 
    const OLECHAR *pwcsName,
    DWORD       grfMode,
    DWORD       reserved1,
    DWORD       reserved2,
    IStream     **ppstm)
{
    char        rcStream[MAXSTREAMNAME];// For converted name.
    VERIFY(Wsz_wcstombs(rcStream, pwcsName, sizeof(rcStream)));
    return (CreateStream(rcStream, grfMode, reserved1, reserved2, ppstm));
}


HRESULT STDMETHODCALLTYPE TiggerStorage::CreateStream( 
    LPCSTR      szName,
    DWORD       grfMode,
    DWORD       reserved1,
    DWORD       reserved2,
    IStream     **ppstm)
{
    STORAGESTREAM *pStream;             // For lookup.
    HRESULT     hr;

    _ASSERTE(szName && *szName);

    // Check for existing stream, which might be an error or more likely
    // a rewrite of a file.
    if ((pStream = FindStream(szName)) != 0)
    {
        if (pStream->GetOffset() != 0xffffffff && (grfMode & STGM_FAILIFTHERE))
            return (PostError(STG_E_FILEALREADYEXISTS));
    }
    // Add a control to track this stream.
    else if (!pStream && (pStream = m_Streams.Append()) == 0)
        return (PostError(OutOfMemory()));
    pStream->SetOffset(0xffffffff);
    pStream->SetSize(0);
    strcpy(pStream->rcName, szName);

    // Now create a stream object to allow reading and writing.
    TiggerStream *pNew = new TiggerStream;
    if (!pNew)
        return (PostError(OutOfMemory()));
    *ppstm = (IStream *) pNew;

    // Init the new object.
    if (FAILED(hr = pNew->Init(this, pStream->rcName)))
    {
        delete pNew;
        return (hr);
    }
    return (S_OK);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::OpenStream( 
    const OLECHAR *pwcsName,
    void        *reserved1,
    DWORD       grfMode,
    DWORD       reserved2,
    IStream     **ppstm)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::CreateStorage( 
    const OLECHAR *pwcsName,
    DWORD       grfMode,
    DWORD       dwStgFmt,
    DWORD       reserved2,
    IStorage    **ppstg)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::OpenStorage( 
    const OLECHAR *pwcsName,
    IStorage    *pstgPriority,
    DWORD       grfMode,
    SNB         snbExclude,
    DWORD       reserved,
    IStorage    **ppstg)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::CopyTo( 
    DWORD       ciidExclude,
    const IID   *rgiidExclude,
    SNB         snbExclude,
    IStorage    *pstgDest)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::MoveElementTo( 
    const OLECHAR *pwcsName,
    IStorage    *pstgDest,
    const OLECHAR *pwcsNewName,
    DWORD       grfFlags)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::Commit( 
    DWORD       grfCommitFlags)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::Revert()
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::EnumElements( 
    DWORD       reserved1,
    void        *reserved2,
    DWORD       reserved3,
    IEnumSTATSTG **ppenum)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::DestroyElement( 
    const OLECHAR *pwcsName)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::RenameElement( 
    const OLECHAR *pwcsOldName,
    const OLECHAR *pwcsNewName)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::SetElementTimes( 
    const OLECHAR *pwcsName,
    const FILETIME *pctime,
    const FILETIME *patime,
    const FILETIME *pmtime)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::SetClass( 
    REFCLSID    clsid)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::SetStateBits( 
    DWORD       grfStateBits,
    DWORD       grfMask)
{
    return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::Stat( 
    STATSTG     *pstatstg,
    DWORD       grfStatFlag)
{
    return (E_NOTIMPL);
}



HRESULT STDMETHODCALLTYPE TiggerStorage::OpenStream( 
    LPCWSTR     szStream,
    ULONG       *pcbData,
    void        **ppAddress)
{
    STORAGESTREAM *pStream;             // For lookup.
    char        rcName[MAXSTREAMNAME];  // For conversion.
    HRESULT     hr;

    // Convert the name for internal use.
    VERIFY(::WszWideCharToMultiByte(CP_ACP, 0, szStream, -1, rcName, sizeof(rcName), 0, 0));

    // Look for the stream which must be found for this to work.  Note that
    // this error is explcitly not posted as an error object since unfound streams
    // are a common occurence and do not warrant a resource file load.
    if ((pStream = FindStream(rcName)) == 0)
        IfFailRet(STG_E_FILENOTFOUND);

    // Get the memory for the stream.
    IfFailRet( m_pStgIO->GetPtrForMem(pStream->GetOffset(), pStream->GetSize(), *ppAddress) );
    *pcbData = pStream->GetSize();
    return (S_OK);
}



//
// Protected.
//


//*****************************************************************************
// Called by the stream implementation to write data out to disk.
//*****************************************************************************
HRESULT TiggerStorage::Write(       // Return code.
    LPCSTR      szName,             // Name of stream we're writing.
    const void *pData,              // Data to write.
    ULONG       cbData,             // Size of data.
    ULONG       *pcbWritten)        // How much did we write.
{
    STORAGESTREAM *pStream;         // Update size data.
    ULONG       iOffset = 0;        // Offset for write.
    ULONG       cbWritten;          // Handle null case.
    HRESULT     hr;

    // Get the stream descriptor.
    pStream = FindStream(szName);
    if (!pStream)
        return CLDB_E_FILE_BADWRITE;

    // If we need to know the offset, keep it now.
    if (pStream->GetOffset() == 0xffffffff)
    {
        iOffset = m_pStgIO->GetCurrentOffset();

        // Align the storage on a 4 byte boundary.
        if (iOffset % 4 != 0)
        {
            ULONG       cb;
            ULONG       pad = 0;
            HRESULT     hr;

            if (FAILED(hr = m_pStgIO->Write(&pad, ALIGN4BYTE(iOffset) - iOffset, &cb)))
                return (hr);
            iOffset = m_pStgIO->GetCurrentOffset();
            
            _ASSERTE(iOffset % 4 == 0);
        }
    }

    // Avoid confusion.
    if (pcbWritten == 0)
        pcbWritten = &cbWritten;
    *pcbWritten = 0;

    // Let OS do the write.
    if (SUCCEEDED(hr = m_pStgIO->Write(pData, cbData, pcbWritten)))
    {
        // On success, record the new data.
        if (pStream->GetOffset() == 0xffffffff)
            pStream->SetOffset(iOffset);
        pStream->SetSize(pStream->GetSize() + *pcbWritten);  
        return (S_OK);
    }
    else
        return (hr);
}


//
// Private
//

STORAGESTREAM *TiggerStorage::FindStream(LPCSTR szName)
{
    int         i;

    // In read mode, just walk the list and return one.
    if (m_pStreamList)
    {
        STORAGESTREAM *p;
        for (i=0, p=m_pStreamList;  i<m_StgHdr.GetiStreams();  i++)
        {
            if (!_stricmp(p->rcName, szName))
                return (p);
            p = p->NextStream();
        }
    }
    // In write mode, walk the array which is not on disk yet.
    else
    {
        for (int i=0;  i<m_Streams.Count();  i++)
        {
            if (!_stricmp(m_Streams[i].rcName, szName))
                return (&m_Streams[i]);
        }
    }
    return (0);
}


//*****************************************************************************
// Write the signature area of the file format to disk.  This includes the
// "magic" identifier and the version information.
//*****************************************************************************
HRESULT TiggerStorage::WriteSignature()
{
    STORAGESIGNATURE sSig;
    ULONG       cbWritten;
    HRESULT     hr;

    // Signature belongs at the start of the file.
    _ASSERTE(m_pStgIO->GetCurrentOffset() == 0);

    // SizeOfStorageSignature must be called before m_dwVersion is used.
#ifdef _DEBUG
    ULONG storageSize =
#endif
    SizeOfStorageSignature();

    sSig.SetSignature(STORAGE_MAGIC_SIG);
    sSig.SetMajorVer(FILE_VER_MAJOR);
    sSig.SetMinorVer(FILE_VER_MINOR);
    sSig.SetExtraDataOffset(0); // We have no extra inforation

    sSig.SetVersionStringLength(m_dwVersion);  // We gain one character thus including the null terminator
    _ASSERTE(storageSize == sizeof(STORAGESIGNATURE) + m_dwVersion);
    IfFailRet(m_pStgIO->Write(&sSig, sizeof(STORAGESIGNATURE), &cbWritten));
    IfFailRet(m_pStgIO->Write(m_Version, m_dwVersion, &cbWritten));

    return hr;
}


//*****************************************************************************
// Read the header from disk.  This reads the header for the most recent version
// of the file format which has the header at the front of the data file.
//*****************************************************************************
HRESULT TiggerStorage::ReadHeader() // Return code.
{
    STORAGESTREAM *pAppend, *pStream;// For copy of array.
    void        *ptr;               // Working pointer.
    ULONG       iOffset;            // Offset of header data.
    ULONG       cbExtra;            // Size of extra data.
    ULONG       cbRead;             // For calc of read sizes.
    HRESULT     hr;

    // Read the signature
    if (FAILED(hr = m_pStgIO->GetPtrForMem(0, sizeof(STORAGESIGNATURE), ptr)))
        return hr;

    STORAGESIGNATURE* pStorage = (STORAGESIGNATURE*) ptr;

    // Make sure we have paged in enough data                                                  
    //  if (FAILED(hr = m_pStgIO->GetPtrForMem(sizeof(STORAGESIGNATURE), VAL32(pStorage->iVersionString), ptr)))
    //      return hr;

    // Header data starts after signature.
    iOffset = sizeof(STORAGESIGNATURE) + pStorage->VersionStringLength();
        
    // Read the storage header which has the stream counts.  Throw in the extra
    // count which might not exist, but saves us down stream.
    if (FAILED(hr = m_pStgIO->GetPtrForMem(iOffset, sizeof(STORAGEHEADER) + sizeof(ULONG), ptr)))
        return (hr);
    _ASSERTE(m_pStgIO->IsAlignedPtr((ULONG_PTR) ptr, 4));

    // Read the storage header which has the stream counts.  Throw in the extra
    // count which might not exist, but saves us down stream.
    if (FAILED(hr = m_pStgIO->GetPtrForMem(iOffset, sizeof(STORAGEHEADER) + sizeof(ULONG), ptr)))
        return (hr);
    if (!m_pStgIO->IsAlignedPtr((ULONG_PTR) ptr, 4))
        return (PostError(CLDB_E_FILE_CORRUPT));

    // Copy the header into memory and check it.
    memcpy(&m_StgHdr, ptr, sizeof(STORAGEHEADER));
    IfFailRet( VerifyHeader() );
    ptr = (void *) ((STORAGEHEADER *) ptr + 1);
    iOffset += sizeof(STORAGEHEADER);

    // Save off a pointer to the extra data.
    if (m_StgHdr.fFlags & STGHDR_EXTRADATA)
    {
        m_pbExtra = ptr;
        cbExtra = sizeof(ULONG) + *(ULONG *) ptr;

        // Force the extra data to get faulted in.
        IfFailRet( m_pStgIO->GetPtrForMem(iOffset, cbExtra, ptr) );
		if (!m_pStgIO->IsAlignedPtr((ULONG_PTR) ptr, 4))
			return (PostError(CLDB_E_FILE_CORRUPT));
    }
    else
    {
        m_pbExtra = 0;
        cbExtra = 0;
    }
    iOffset += cbExtra;

    // Force the worst case scenario of bytes to get faulted in for the
    // streams.  This makes the rest of this code very simple.
    cbRead = sizeof(STORAGESTREAM) * m_StgHdr.GetiStreams();
    if (cbRead)
    {
        cbRead = min(cbRead, m_pStgIO->GetDataSize() - iOffset);
        if (FAILED(hr = m_pStgIO->GetPtrForMem(iOffset, cbRead, ptr)))
            return (hr);
		if (!m_pStgIO->IsAlignedPtr((ULONG_PTR) ptr, 4))
			return (PostError(CLDB_E_FILE_CORRUPT));

        // For read only, just access the header data.
        if (m_pStgIO->IsReadOnly())
        {
            // Save a pointer to the current list of streams.
            m_pStreamList = (STORAGESTREAM *) ptr;
        }
        // For writeable, need a copy we can modify.
        else
        {
            pStream = (STORAGESTREAM *) ptr;

            // Copy each of the stream headers.
            for (int i=0;  i<m_StgHdr.GetiStreams();  i++)
            {
                if ((pAppend = m_Streams.Append()) == 0)
                    return (PostError(OutOfMemory()));
                // Validate that the stream header is not too big.
                ULONG sz = pStream->GetStreamSize();
                if (sz > sizeof(STORAGESTREAM))
					return (PostError(CLDB_E_FILE_CORRUPT));
                memcpy (pAppend, pStream, sz);
                pStream = pStream->NextStream();
                if (!m_pStgIO->IsAlignedPtr((ULONG_PTR) pStream, 4))
					return (PostError(CLDB_E_FILE_CORRUPT));
            }

            // All must be loaded and accounted for.
            _ASSERTE(m_StgHdr.GetiStreams() == m_Streams.Count());
        }
    }
    return (S_OK);
}


//*****************************************************************************
// Verify the header is something this version of the code can support.
//*****************************************************************************
HRESULT TiggerStorage::VerifyHeader()
{
    return (S_OK);
}

//*****************************************************************************
// Print the sizes of the various streams.
//*****************************************************************************
#if defined(_DEBUG)
ULONG TiggerStorage::PrintSizeInfo(bool verbose)
{
    ULONG total = 0;

    printf("Storage Header:  %d\n",sizeof(STORAGEHEADER));
    if (m_pStreamList)
    {
        STORAGESTREAM *storStream = m_pStreamList;
        STORAGESTREAM *pNext;
        for (ULONG i = 0; i<m_StgHdr.GetiStreams(); i++)
        {
            pNext = storStream->NextStream();
            printf("Stream #%d (%s) Header: %d, Data: %d\n",i,storStream->rcName, (BYTE*)pNext - (BYTE*)storStream, storStream->GetSize());
            total += storStream->GetSize(); 
            storStream = pNext;
        }
    }
    else
    {
    }

    if (m_pbExtra)
    {
        printf("Extra bytes: %d\n",*(ULONG*)m_pbExtra);
        total += *(ULONG*)m_pbExtra;
    }
    return total;
}
#endif // _DEBUG
