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
// StgIO.h
//
// This module handles disk/memory i/o for a generic set of storage solutions,
// including:
//	* File system handle (HFILE)
//	* IStream
//	* User supplied memory buffer (non-movable)
//
// The Read, Write, Seek, ... functions are all directed to the corresponding
// method for each type of file, allowing the consumer to use one set of api's.
//
// File system data can be paged fully into memory in two scenarios:
//	read:	Normal memory mapped file is created to manage paging.
//	write:	A custom paging system provides storage for pages as required.  This
//				data is invalidated when you call Rewrite on the file.
//
// Transactions and backups are handled in the existing file case only.  The
// Rewrite function can make a backup of the current contents, and the Restore
// function can be used to recover the data into the current scope.  The backup
// file is flushed to disk (which is slower but safer) after the copy.  The
// Restore also flushed the recovered changes to disk.  Worst case scenario you
// get a crash after calling Rewrite but before Restore, in which case you will
// have a foo.clb.txn file in the same directory as the source file, foo.clb in
// this example.
//*****************************************************************************
#include "stdafx.h"						// Standard headers.
#include "stgio.h"						// Our definitions.
#include "corerror.h"
#include "posterror.h"

//********** Types. ***********************************************************
#define SMALL_ALLOC_MAP_SIZE (64 * 1024) // 64 kb is the minimum size of virtual
										// memory you can allocate, so anything
										// less is a waste of VM resources.
#define MIN_WRITE_CACHE_BYTES (16 * 1024) // 16 kb for a write back cache


//********** Locals. **********************************************************
HRESULT MapFileError(DWORD error);
static void *AllocateMemory(int iSize);
static void FreeMemory(void *pbData);
inline HRESULT MapFileError(DWORD error)
{
	return (PostError(HRESULT_FROM_WIN32(error)));
}

// Static to class.
int	StgIO::m_iPageSize=0;				// Size of an OS page.
int	StgIO::m_iCacheSize=0;				// Size for the write cache.



//********** Code. ************************************************************
StgIO::StgIO(
	bool		bAutoMap) :				// Memory map for read on open?
	m_bAutoMap(bAutoMap)
{
	CtorInit();

	// If the system page size has not been queried, do so now.
	if (m_iPageSize == 0)
	{
		SYSTEM_INFO	sInfo;				// Some O/S information.

		// Query the system page size.
		GetSystemInfo(&sInfo);
		m_iPageSize = sInfo.dwPageSize;
		m_iCacheSize = ((MIN_WRITE_CACHE_BYTES - 1) & ~(m_iPageSize - 1)) + m_iPageSize;
	}
}


void StgIO::CtorInit()
{
	m_bWriteThrough = false;
	m_bRewrite = false;
	m_bFreeMem = false;
	m_pIStream = 0;
	m_hFile = INVALID_HANDLE_VALUE;
	m_hMapping = 0;
	m_pBaseData = 0;
	m_pData = 0;
	m_cbData = 0;
	m_fFlags = 0;
	m_iType = STGIO_NODATA;
	m_cbOffset = 0;
	m_rgBuff = 0;
	m_cbBuff = 0;
	m_rgPageMap = 0;
	*m_rcFile = '\0';
	*m_rcShared = '\0';
	m_cRef = 1;
}



StgIO::~StgIO()
{
	if (m_rgBuff)
	{
		FreeMemory(m_rgBuff);
		m_rgBuff = 0;
	}

	Close();
}


//*****************************************************************************
// Open the base file on top of: (a) file, (b) memory buffer, or (c) stream.
// If create flag is specified, then this will create a new file with the
// name supplied.  No data is read from an opened file.  You must call
// MapFileToMem before doing direct pointer access to the contents.
//*****************************************************************************
HRESULT StgIO::Open(					// Return code.
	LPCWSTR		szName,					// Name of the storage.
	long		fFlags,					// How to open the file.
	const void	*pbBuff,				// Optional buffer for memory.
	ULONG		cbBuff,					// Size of buffer.
	IStream		*pIStream,				// Stream for input.
	LPCWSTR		szSharedMem,			// Shared memory name.
	LPSECURITY_ATTRIBUTES pAttributes)	// Security token.
{
	HRESULT		hr;


	// If we were given the storage memory to begin with, then use it.
	if (pbBuff && cbBuff)
	{
		_ASSERTE((fFlags & DBPROP_TMODEF_WRITE) == 0);

		// Save the memory address and size only.  No handles.
		m_pData = (void *) pbBuff;
		m_cbData = cbBuff;

		// All access to data will be by memory provided.
		m_iType = STGIO_MEM;
		goto ErrExit;
	}
	// Check for data backed by a stream pointer.
	else if (pIStream)
	{
		// If this is for the non-create case, get the size of existing data.
		if ((fFlags & DBPROP_TMODEF_CREATE) == 0)
		{
            LARGE_INTEGER    iMove = { { 0, 0 } };
			ULARGE_INTEGER	iSize;

			// Need the size of the data so we can map it into memory.
			if (FAILED(hr = pIStream->Seek(iMove, STREAM_SEEK_END, &iSize)))
				return (hr);
			m_cbData = iSize.u.LowPart;
		}
		// Else there is nothing.
		else
			m_cbData = 0;

		// Save an addref'd copy of the stream.
		m_pIStream = pIStream;
		m_pIStream->AddRef();

		// All access to data will be by memory provided.
		m_iType = STGIO_STREAM;
		goto ErrExit;
	}

	// If not on memory, we need a file to do a create/open.
	if (!szName || !*szName)
	{
		return (PostError(E_INVALIDARG));
	}
	// Check for create of a new file.
	else if (fFlags & DBPROP_TMODEF_CREATE)
	{

		// Create the new file, overwriting only if caller allows it.
		if ((m_hFile = WszCreateFile(szName, GENERIC_READ | GENERIC_WRITE, 0, 0, 
				(fFlags & DBPROP_TMODEF_FAILIFTHERE) ? CREATE_NEW : CREATE_ALWAYS, 
				0, 0)) == INVALID_HANDLE_VALUE)
		{
			return (MapFileError(GetLastError()));
		}

		// Data will come from the file.
		m_iType = STGIO_HFILE;
	}
	// For open in read mode, need to open the file on disk.  If opening a shared
	// memory view, it has to be opened already, so no file open.
	else if ((fFlags & DBPROP_TMODEF_WRITE) == 0)
	{
		// Open the file for read.  Sharing is determined by caller, it can
		// allow other readers or be exclusive.
		if ((m_hFile = WszCreateFile(szName, GENERIC_READ, 
					(fFlags & DBPROP_TMODEF_EXCLUSIVE) ? 0 : FILE_SHARE_READ,
					0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
			return (MapFileError(GetLastError()));

		// Get size of file.
		m_cbData = ::SetFilePointer(m_hFile, 0, 0, FILE_END);

		// Can't read anything from an empty file.
		if (m_cbData == 0)
			return (PostError(CLDB_E_NO_DATA));

		// Data will come from the file.
		m_iType = STGIO_HFILE;
	}

ErrExit:

	// If we will ever write, then we need the buffer cache.
	if (fFlags & DBPROP_TMODEF_WRITE)
	{
		// Allocate a cache buffer for writing.
		if ((m_rgBuff = (BYTE *) AllocateMemory(m_iCacheSize)) == 0)
		{
			Close();
			return (PostError(OutOfMemory()));
		}
		m_cbBuff = 0;
	}


	// Save flags for later.
	m_fFlags = fFlags;
	if (szName && *szName)
		wcscpy(m_rcFile, szName);

	// For auto map case, map the view of the file as part of open.
	if (m_bAutoMap && 
		(m_iType == STGIO_HFILE || m_iType == STGIO_STREAM 
                ) &&
		!(fFlags & DBPROP_TMODEF_CREATE))
	{
		void		*ptr;
		ULONG		cb;
		
		if (FAILED(hr = MapFileToMem(ptr, &cb, pAttributes)))
		{
			Close();
			return (hr);
		}
	}
	return (S_OK);	
}


//*****************************************************************************
// Shut down the file handles and allocated objects.
//*****************************************************************************
void StgIO::Close()
{
	switch (m_iType)
	{
		// Free any allocated memory.
		case STGIO_MEM:
		case STGIO_SHAREDMEM:
		case STGIO_HFILEMEM:
		if (m_bFreeMem && m_pBaseData)
		{
			FreeMemory(m_pBaseData);
			m_pBaseData = m_pData = 0;
		}
		// Intentional fall through to file case, if we kept handle open.

		case STGIO_HFILE:
		{
			// Free the file handle.
			if (m_hFile != INVALID_HANDLE_VALUE)
				CloseHandle(m_hFile);

			// If we allocated space for in memory paging, then free it.
		}
		break;

		// Free the stream pointer.
		case STGIO_STREAM:
                {
			if (m_pIStream != NULL)
				m_pIStream->Release();
		}
		break;

		// Weird to shut down what you didn't open, isn't it?  Allow for
		// error case where dtor shuts down as an afterthought.
		case STGIO_NODATA:
		default:
		return;
	}

	// Free any page map and base data.
	FreePageMap();

	// Reset state values so we don't get confused.
	CtorInit();
}


//*****************************************************************************
// Read data from the storage source.  This will handle all types of backing
// storage from mmf, streams, and file handles.  No read ahead or MRU
// caching is done.
//*****************************************************************************
HRESULT StgIO::Read(					// Return code.
	void		*pbBuff,				// Write buffer here.
	ULONG		cbBuff,					// How much to read.
	ULONG		*pcbRead)				// How much read.
{
	ULONG		cbCopy;					// For boundary checks.
	void		*pbData;				// Data buffer for mem read.
	HRESULT		hr = S_OK;

	// Validate arguments, don't call if you don't need to.
	_ASSERTE(pbBuff != 0);
	_ASSERTE(cbBuff > 0);

	// Get the data based on type.
	switch (m_iType)
	{
		// For data on file, there are two possiblities:
		// (1) We have an in memory backing store we should use, or
		// (2) We just need to read from the file.
		case STGIO_HFILE:
		{
			_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);

			// Backing store does its own paging.
			if (IsBackingStore() || IsMemoryMapped())
			{
				// Force the data into memory.
				if (FAILED(hr = GetPtrForMem(GetCurrentOffset(), cbBuff, pbData)))
					goto ErrExit;

				// Copy it back for the user and save the size.
				memcpy(pbBuff, pbData, cbBuff);
				if (pcbRead)
					*pcbRead = cbBuff;				
			}
			// If there is no backing store, this is just a read operation.
			else
			{
				ULONG	cbTemp = 0;
				if (!pcbRead)
					pcbRead = &cbTemp;
				hr = ReadFromDisk(pbBuff, cbBuff, pcbRead);
				m_cbOffset += *pcbRead;
			}
		}
		break;

		// Data in a stream is always just read.
		case STGIO_STREAM:
		{
			_ASSERTE((IStream *) m_pIStream);
			if (!pcbRead)
				pcbRead = &cbCopy;
			hr = m_pIStream->Read(pbBuff, cbBuff, pcbRead);
			if (SUCCEEDED(hr))
				m_cbOffset += *pcbRead;
		}
		break;

		// Simply copy the data from our data.
		case STGIO_MEM:
		case STGIO_SHAREDMEM:
		case STGIO_HFILEMEM:
		{
			_ASSERTE(m_pData && m_cbData);

			// Check for read past end of buffer and adjust.
			if (GetCurrentOffset() + cbBuff > m_cbData)
				cbCopy = m_cbData - GetCurrentOffset();
			else
				cbCopy = cbBuff;
							
			// Copy the data into the callers buffer.
			memcpy(pbBuff, (void *) ((DWORD_PTR)m_pData + GetCurrentOffset()), cbCopy);
			if (pcbRead)
				*pcbRead = cbCopy;
			
			// Save a logical offset.
			m_cbOffset += cbCopy;
		}
		break;
		 
		case STGIO_NODATA:
		default:
		_ASSERTE(0);
		break;
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Write to disk.  This function will cache up to a page of data in a buffer
// and peridocially flush it on overflow and explicit request.  This makes it
// safe to do lots of small writes without too much performance overhead.
//*****************************************************************************
HRESULT StgIO::Write(					// true/false.
	const void	*pbBuff,				// Data to write.
	ULONG		cbWrite,				// How much data to write.
	ULONG		*pcbWritten)			// How much did get written.
{
	ULONG		cbWriteIn=cbWrite;		// Track amount written.
	ULONG		cbCopy;
	HRESULT		hr = S_OK;

	_ASSERTE(m_rgBuff != 0);
	_ASSERTE(cbWrite);

	while (cbWrite)
	{
		// In the case where the buffer is already huge, write the whole thing
		// and avoid the cache.
		if (m_cbBuff == 0 && cbWrite >= (ULONG) m_iPageSize)
		{
			if (SUCCEEDED(hr = WriteToDisk(pbBuff, cbWrite, pcbWritten)))
				m_cbOffset += cbWrite;
			break;
		}
		// Otherwise cache as much as we can and flush.
		else
		{
			// Determine how much data goes into the cache buffer.
			cbCopy = m_iPageSize - m_cbBuff;
			cbCopy = min(cbCopy, cbWrite);
			
			// Copy the data into the cache and adjust counts.
			memcpy(&m_rgBuff[m_cbBuff], pbBuff, cbCopy);
			pbBuff = (void *) ((DWORD_PTR)pbBuff + cbCopy);
			m_cbBuff += cbCopy;
			m_cbOffset += cbCopy;
			cbWrite -= cbCopy;

			// If there is enough data, then flush it to disk and reset count.
			if (m_cbBuff >= (ULONG) m_iPageSize)
			{
				if (FAILED(hr = FlushCache()))
					break;
			}
		}
	}

	// Return value for caller.
	if (SUCCEEDED(hr) && pcbWritten)
		*pcbWritten = cbWriteIn;
	return (hr);
}


//*****************************************************************************
// Moves the file pointer to the new location.  This handles the different
// types of storage systems.
//*****************************************************************************
HRESULT StgIO::Seek(					// New offset.
	long		lVal,					// How much to move.
	ULONG		fMoveType)				// Direction, use Win32 FILE_xxxx.
{
	ULONG		cbRtn = 0;
	HRESULT		hr = NOERROR;

	_ASSERTE(fMoveType >= FILE_BEGIN && fMoveType <= FILE_END);

	// Action taken depends on type of storage.
	switch (m_iType)
	{
		case STGIO_HFILE:
		{
			// Use the file system's move.
			_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);
			cbRtn = ::SetFilePointer(m_hFile, lVal, 0, fMoveType);
			
			// Save the location redundantly.
			if (cbRtn != 0xffffffff)
            {
                // make sure that m_cbOffset will stay within range
                if (cbRtn > m_cbData || cbRtn < 0)
                {
                    IfFailGo(STG_E_INVALIDFUNCTION);
                }
				m_cbOffset = cbRtn;
            }
		}
		break;

		case STGIO_STREAM:
		{
			LARGE_INTEGER	iMove;
			ULARGE_INTEGER	iNewLoc;

			// Need a 64-bit int.
			iMove.QuadPart = lVal;

			// The move types are named differently, but have same value.
			if (FAILED(hr = m_pIStream->Seek(iMove, fMoveType, &iNewLoc)))
				return (hr);

            // make sure that m_cbOffset will stay within range
            if (iNewLoc.u.LowPart > m_cbData || iNewLoc.u.LowPart < 0)
                IfFailGo(STG_E_INVALIDFUNCTION);

            // Save off only out location.
			m_cbOffset = iNewLoc.u.LowPart;
		}
		break;

		case STGIO_MEM:
		case STGIO_SHAREDMEM:
		case STGIO_HFILEMEM:
		{
			// We own the offset, so change our value.
			switch (fMoveType)
			{
				case FILE_BEGIN:

                // make sure that m_cbOffset will stay within range
                if ((ULONG) lVal > m_cbData || lVal < 0)
                {
                    IfFailGo(STG_E_INVALIDFUNCTION);
                }
				m_cbOffset = lVal;
				break;

				case FILE_CURRENT:
                
                // make sure that m_cbOffset will stay within range
                if (m_cbOffset + lVal > m_cbData)
                {
                    IfFailGo(STG_E_INVALIDFUNCTION);
                }
				m_cbOffset = m_cbOffset + lVal;
				break;

				case FILE_END:
				_ASSERTE(lVal < (LONG) m_cbData);
                // make sure that m_cbOffset will stay within range
                if (m_cbData + lVal > m_cbData)
                {
                    IfFailGo(STG_E_INVALIDFUNCTION);
                }
				m_cbOffset = m_cbData + lVal;
				break;
			}

			cbRtn = m_cbOffset;
		}
		break;

		// Weird to seek with no data.
		case STGIO_NODATA:
		default:
		_ASSERTE(0);
		break;
	}

ErrExit:
    return hr;
}


//*****************************************************************************
// Retrieves the current offset for the storage being used.  This value is
// tracked based on Read, Write, and Seek operations.
//*****************************************************************************
ULONG StgIO::GetCurrentOffset()			// Current offset.
{
	return (m_cbOffset);
}


//*****************************************************************************
// Map the file contents to a memory mapped file and return a pointer to the 
// data.  For read/write with a backing store, map the file using an internal
// paging system.
//*****************************************************************************
HRESULT StgIO::MapFileToMem(			// Return code.
	void		*&ptr,					// Return pointer to file data.
	ULONG		*pcbSize,				// Return size of data.
	LPSECURITY_ATTRIBUTES pAttributes)	// Security token.
{
    char		rcShared[MAXSHMEM];		// ANSI version of shared name.
	HRESULT		hr = S_OK;

	// Don't penalize for multiple calls.  Also, allow calls for mem type so
	// callers don't need to do so much checking.
	if (IsBackingStore() || IsMemoryMapped() || 
			m_iType == STGIO_MEM || m_iType == STGIO_HFILEMEM)
	{
		ptr = m_pData;
		if (pcbSize)
			*pcbSize = m_cbData;
		return (S_OK);
	}

	// Check the size of the data we want to map.  If it is small enough, then
	// simply allocate a chunk of memory from a finer grained heap.  This saves
	// virtual memory space, page table entries, and should reduce overall working set.
	// Note that any shared memory objects will require the handles to be in place
	// and are not elligible.  Also, open for read/write needs a full backing
	// store.
	if (!*m_rcShared && m_cbData <= SMALL_ALLOC_MAP_SIZE)
	{
		DWORD cbRead = m_cbData;
		_ASSERTE(m_pData == 0);

		// Just malloc a chunk of data to use.
		m_pBaseData = m_pData = AllocateMemory(m_cbData);
		if (!m_pData)
		{
			hr = OutOfMemory();
			goto ErrExit;
		}

		// Read all of the file contents into this piece of memory.
		IfFailGo( Seek(0, FILE_BEGIN) );
		if (FAILED(hr = Read(m_pData, cbRead, &cbRead)))
		{
			FreeMemory(m_pData);
			m_pData = 0;
			goto ErrExit;
		}
		_ASSERTE(cbRead == m_cbData);

		// If the file isn't being opened for exclusive mode, then free it.
		// If it is for exclusive, then we need to keep the handle open so the
		// file is locked, preventing other readers.  Also leave it open if
		// in read/write mode so we can truncate and rewrite.
		if (m_hFile == INVALID_HANDLE_VALUE ||
			((m_fFlags & DBPROP_TMODEF_EXCLUSIVE) == 0 && (m_fFlags & DBPROP_TMODEF_WRITE) == 0))
		{
			// If there was a handle open, then free it.
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
				VERIFY(CloseHandle(m_hFile));
				m_hFile = INVALID_HANDLE_VALUE;
			}
			// Free the stream pointer.
			else
			if (m_pIStream != 0)
			{
				m_pIStream->Release();
				m_pIStream = 0;
			}

			// Switch the type to memory only access.
			m_iType = STGIO_MEM;
		}
		else
			m_iType = STGIO_HFILEMEM;

		// Free the memory when we shut down.
		m_bFreeMem = true;
	}
	// Finally, a real mapping file must be created.
	else
	{
		// Now we will map, so better have it right.
		_ASSERTE(m_hFile != INVALID_HANDLE_VALUE || m_iType == STGIO_STREAM);
		_ASSERTE(m_rgPageMap == 0);

		// For read mode, use a memory mapped file since the size will never
		// change for the life of the handle.
		if ((m_fFlags & DBPROP_TMODEF_WRITE) == 0 && m_iType != STGIO_STREAM)
		{
			_ASSERTE(!*m_rcShared);

			// Create a mapping object for the file.
			_ASSERTE(m_hMapping == 0);
			if ((m_hMapping = WszCreateFileMapping(m_hFile, pAttributes, PAGE_READONLY,
					0, 0, *m_rcShared ? m_rcShared : 0)) == 0)
			{
				return (MapFileError(GetLastError()));
			}

			// Check to see if the memory already exists, in which case we have
			// no guarantees it is the right piece of data.
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				hr = PostError(CLDB_E_SMDUPLICATE, rcShared);
				goto ErrExit;
			}

			// Now map the file into memory so we can read from pointer access.
			if ((m_pBaseData = m_pData = MapViewOfFile(m_hMapping, FILE_MAP_READ,
						0, 0, 0)) == 0 ||
				IsBadReadPtr(m_pData, m_iPageSize))
			{
				hr = MapFileError(GetLastError());
				if (!FAILED(hr))
					hr = PostError(CLDB_E_FILE_CORRUPT);
				
				// In case we got back a bogus pointer.
				m_pBaseData = m_pData = 0;
				goto ErrExit;
			}
		}
		// In write mode, we need the hybrid combination of being able to back up
		// the data in memory via cache, but then later rewrite the contents and
		// throw away our cached copy.  Memory mapped files are not good for this
		// case due to poor write characteristics.
		else
		{
			ULONG		iMaxSize;			// How much memory required for file.

			// Figure out how many pages we'll require, round up actual data
			// size to page size.
			iMaxSize = (((m_cbData - 1) & ~(m_iPageSize - 1)) + m_iPageSize);

			// Allocate a bit vector to track loaded pages.
			if ((m_rgPageMap = new BYTE[iMaxSize / m_iPageSize]) == 0)
				return (PostError(OutOfMemory()));
			memset(m_rgPageMap, 0, sizeof(BYTE) * (iMaxSize / m_iPageSize));

			// Allocate space for the file contents.
			if ((m_pBaseData = m_pData = ::VirtualAlloc(0, iMaxSize, MEM_RESERVE, PAGE_NOACCESS)) == 0)
			{
				hr = PostError(OutOfMemory());
				goto ErrExit;
			}
		}
	}

	// Reset any changes made by mapping.
	IfFailGo( Seek(0, FILE_BEGIN) );

ErrExit:

	// Check for errors and clean up.
	if (FAILED(hr))
	{
		if (m_hMapping)
			CloseHandle(m_hMapping);
		m_hMapping = 0;
		m_pBaseData = m_pData = 0;
		m_cbData = 0;
	}
	ptr = m_pData;
	if (pcbSize)
		*pcbSize = m_cbData;
	return (hr);
}


//*****************************************************************************
// Free the mapping object for shared memory but keep the rest of the internal
// state intact.
//*****************************************************************************
HRESULT StgIO::ReleaseMappingObject()	// Return code.
{
	// Check type first.
	if (m_iType != STGIO_SHAREDMEM)
	{
		_ASSERTE(0);
		return (S_OK);
	}

	// Must have an allocated handle.
	_ASSERTE(m_hMapping != 0);

	// Freeing the mapping object doesn't do any good if you still have the file.
	_ASSERTE(m_hFile == INVALID_HANDLE_VALUE);

	// Unmap the memory we allocated before freeing the handle.  But keep the
	// memory address intact.
	if (m_pData)
		VERIFY(UnmapViewOfFile(m_pData));

	// Free the handle.
	if (m_hMapping != 0)
	{
		VERIFY(CloseHandle(m_hMapping));
		m_hMapping = 0;
	}
	return (S_OK);
}



//*****************************************************************************
// Resets the logical base address and size to the value given.  This is for
// cases like finding a section embedded in another format, like the .clb inside
// of an image.  GetPtrForMem, Read, and Seek will then behave as though only
// data from pbStart to cbSize is valid.
//*****************************************************************************
HRESULT StgIO::SetBaseRange(			// Return code.
	void		*pbStart,				// Start of file data.
	ULONG		cbSize)					// How big is the range.
{
	// The base range must be inside of the current range.
	_ASSERTE(m_iType != STGIO_SHAREDMEM || (m_pBaseData && m_cbData));
	_ASSERTE(m_iType != STGIO_SHAREDMEM || ((long) pbStart >= (long) m_pBaseData));
	_ASSERTE(m_iType != STGIO_SHAREDMEM || ((long) pbStart + cbSize <= (long) m_pBaseData + m_cbData));

	// Save the base range per user request.
	m_pData = pbStart;
	m_cbData = cbSize;
	return (S_OK);
}


//*****************************************************************************
// Caller wants a pointer to a chunk of the file.  This function will make sure
// that the memory for that chunk has been committed and will load from the
// file if required.  This algorithm attempts to load no more data from disk
// than is necessary.  It walks the required pages from lowest to highest,
// and for each block of unloaded pages, the memory is committed and the data
// is read from disk.  If all pages are unloaded, all of them are loaded at
// once to speed throughput from disk.
//*****************************************************************************
HRESULT StgIO::GetPtrForMem(			// Return code.
	ULONG		cbStart,				// Where to start getting memory.
	ULONG		cbSize,					// How much data.
	void		*&ptr)					// Return pointer to memory here.
{
	int			iFirst, iLast;			// First and last page required.
	ULONG		iOffset, iSize;			// For committing ranges of memory.
	int			i, j;					// Loop control.
	HRESULT		hr;

	// We need either memory (mmf or user supplied) or a backing store to
	// return a pointer.  Call Read if you don't have these.
	if (!IsBackingStore() && m_pData == 0)
		return (PostError(BadError(E_UNEXPECTED)));

	// Validate the caller isn't asking for a data value out of range.
	if (!(cbStart + cbSize <= m_cbData))
		return (PostError(E_INVALIDARG));

	// This code will check for pages that need to be paged from disk in
	// order for us to return a pointer to that memory.
	if (IsBackingStore())
	{
		// Backing store is bogus when in rewrite mode.
		if (m_bRewrite)
			return (PostError(BadError(E_UNEXPECTED)));

		// Must have the page map to continue.
		_ASSERTE(m_rgPageMap && m_iPageSize && m_pData);

		// Figure out the first and last page that are required for commit.
		iFirst = cbStart / m_iPageSize;
		iLast = (cbStart + cbSize - 1) / m_iPageSize;

		// Avoid confusion.
		ptr = 0;

		// Do a smart load of every page required.  Do not reload pages that have
		// already been brought in from disk.
		for (i=iFirst;  i<=iLast;  )
		{
			// Find the first page that hasn't already been loaded.
			while (GetBit(m_rgPageMap, i) && i<=iLast)
				++i;
			if (i > iLast)
				break;

			// Offset for first thing to load.
			iOffset = i * m_iPageSize;
			iSize = 0;

			// See how many in a row have not been loaded.
			for (j=i;  i<=iLast && !GetBit(m_rgPageMap, i);  i++)
				iSize += m_iPageSize;

			// First commit the memory for this part of the file.
			if (::VirtualAlloc((void *) ((DWORD_PTR) m_pData + iOffset), 
					iSize, MEM_COMMIT, PAGE_READWRITE) == 0)
				return (PostError(OutOfMemory()));

			// Now load that portion of the file from disk.
			if (FAILED(hr = Seek(iOffset, FILE_BEGIN)) ||
				FAILED(hr = ReadFromDisk((void *) ((DWORD_PTR) m_pData + iOffset), iSize, 0)))
			{
				return (hr);
			}

			// Change the memory to read only to avoid any modifications.  Any faults
			// that occur indicate a bug whereby the engine is trying to write to
			// protected memory.
			_ASSERTE(::VirtualAlloc((void *) ((DWORD_PTR) m_pData + iOffset), 
					iSize, MEM_COMMIT, PAGE_READONLY) != 0);
		
			// Record each new loaded page.
			for (;  j<i;  j++)
				SetBit(m_rgPageMap, j, true);
		}

		// Everything was brought into memory, so now return pointer to caller.
		ptr = (void *) ((DWORD_PTR) m_pData + cbStart);
	}
	// Memory version or memory mapped file work the same way.
	else if (IsMemoryMapped() || m_iType == STGIO_MEM || 
			m_iType == STGIO_SHAREDMEM || m_iType == STGIO_HFILEMEM)
	{	
		if (!(cbStart < m_cbData))
			return (PostError(E_INVALIDARG));

		ptr = (void *) ((DWORD_PTR) m_pData + cbStart);
	}
	// What's left?!  Add some defense.
	else
	{
		_ASSERTE(0);
		ptr = 0;
        return (PostError(BadError(E_UNEXPECTED)));
	}
	return (S_OK);
}


//*****************************************************************************
// For cached writes, flush the cache to the data store.
//*****************************************************************************
HRESULT StgIO::FlushCache()
{
	ULONG		cbWritten;
	HRESULT		hr;

	if (m_cbBuff)
	{
		if (FAILED(hr = WriteToDisk(m_rgBuff, m_cbBuff, &cbWritten)))
			return (hr);
		m_cbBuff = 0;
	}
	return (S_OK);
}

//*****************************************************************************
// Tells the file system to flush any cached data it may have.  This is
// expensive, but if successful guarantees you won't lose writes short of
// a disk failure.
//*****************************************************************************
HRESULT StgIO::FlushFileBuffers()
{
	_ASSERTE(!IsReadOnly());

	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		if (::FlushFileBuffers(m_hFile))
			return (S_OK);
		else
			return (MapFileError(GetLastError()));
	}
	return (S_OK);
}


//*****************************************************************************
// Tells the storage that we intend to rewrite the contents of this file.  The
// entire file will be truncated and the next write will take place at the
// beginning of the file.
//*****************************************************************************
HRESULT StgIO::Rewrite(					// Return code.
	LPWSTR		szBackup)				// If non-0, backup the file.
{
    void		*ptr;					// Working pointer.
	HRESULT		hr;

	_ASSERTE(m_iType == STGIO_HFILE || m_hFile != INVALID_HANDLE_VALUE);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return (BadError(E_UNEXPECTED));

	// Don't be calling this function for read only data.
	_ASSERTE(!IsReadOnly());

	// If there is still memory on disk, then it needs to be brought in because
	// we are about to truncate the file contents.  This state occurs when
	// data is partially faulted in, for example if there are 5 tables on disk
	// and you only opened 1 of them.  Also need to check for case where a save
	// was already done in this session.  In that case, there is no data on disk
	// that we care about and therefore we can't call GetPtrForMem.
	if (IsBackingStore())
	{
		if (FAILED(hr = GetPtrForMem(0, m_cbData, ptr)))
			return (hr);
	}

	// Caller wants a backup.  Create a temporary file copy of the user data,
	// and return the path to the caller.
	if (szBackup)
	{
		WCHAR		rcDir[_MAX_PATH];

		_ASSERTE(*m_rcFile);
		
		// Attempt to put the backup file in the user's directory so it is
		// easy for them to find on severe error.
		if (WszGetCurrentDirectory(_MAX_PATH, rcDir) &&
			(WszGetFileAttributes(rcDir) & FILE_ATTRIBUTE_READONLY) == 0)
		{
			WCHAR		rcDrive[_MAX_DRIVE];	// Volume name.
			WCHAR		rcDir2[_MAX_PATH];		// Directory.
			WCHAR		rcFile[_MAX_PATH];		// Name of file.

			SplitPath(m_rcFile, rcDrive, rcDir2, rcFile, 0);
			MakePath(szBackup, rcDrive, rcDir2, rcFile, L".clb.txn");
		}
		// Otherwise put the file in the temp directory.
		else
		{
			// Create a temporary path for backup.
			WszGetTempPath(sizeof(rcDir)/sizeof(WCHAR), rcDir);
			VERIFY(WszGetTempFileName(rcDir, L"clb", 0, szBackup));
		}

		// Copy the file to the temporary path.
		if (FAILED(hr = CopyFileInternal(szBackup, false, m_bWriteThrough)))
			return (hr);
	}

	// Set mode to rewrite.  The backing store is rendered invalid at this point,
	// unless Restore is called.  If the rewrite is successful, this state will
	// stay in affect until shutdown.
	m_bRewrite = true;

	// Verify that we don't have any hanging data from previous work.
	_ASSERTE(m_cbBuff == 0);
	m_cbBuff = 0;
	m_cbOffset = 0;

	// Truncate the file.
	_ASSERTE(!IsReadOnly() && m_hFile != INVALID_HANDLE_VALUE);
	::SetFilePointer(m_hFile, 0, 0, FILE_BEGIN);
	VERIFY(::SetEndOfFile(m_hFile));
	return (S_OK);
}


//*****************************************************************************
// Called after a successful rewrite of an existing file.  The in memory
// backing store is no longer valid because all new data is in memory and
// on disk.  This is essentially the same state as created, so free up some
// working set and remember this state.
//*****************************************************************************
HRESULT StgIO::ResetBackingStore()		// Return code.
{
	// Don't be calling this function for read only data.
	_ASSERTE(!IsReadOnly());

	// Free up any backing store data we no longer need now that everything
	// is in memory.
	FreePageMap();
	return (S_OK);
}


//*****************************************************************************
// Called to restore the original file.  If this operation is successful, then
// the backup file is deleted as requested.  The restore of the file is done
// in write through mode to the disk help ensure the contents are not lost.
// This is not good enough to fulfill ACID props, but it ain't that bad.
//*****************************************************************************
HRESULT StgIO::Restore(					// Return code.
	LPWSTR		szBackup,				// If non-0, backup the file.
	int			bDeleteOnSuccess)		// Delete backup file if successful.
{
	BYTE		rcBuffer[4096];			// Buffer for copy.
	ULONG		cbBuff;					// Bytes read/write.
	ULONG		cbWrite;				// Check write byte count.
	HANDLE		hCopy;					// Handle for backup file.
	LONG		lRet;
	BOOL		bRet;

	// Don't be calling this function for read only data.
	_ASSERTE(!IsReadOnly());

	// Open the backup file.
	if ((hCopy = ::WszCreateFile(szBackup, GENERIC_READ, 0,
				0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
		return (MapFileError(GetLastError()));

	// Move to front of old file.
	lRet = ::SetFilePointer(m_hFile, 0, 0, FILE_BEGIN);
	_ASSERTE(lRet == 0);

	bRet = ::SetEndOfFile(m_hFile);
	_ASSERTE(bRet);

	// Copy all data from the backup into our file.
	while (::ReadFile(hCopy, rcBuffer, sizeof(rcBuffer), &cbBuff, 0) && cbBuff)
		if (!::WriteFile(m_hFile, rcBuffer, cbBuff, &cbWrite, 0) || cbWrite != cbBuff)
		{
			::CloseHandle(hCopy);
			return (MapFileError(GetLastError()));
		}

	::CloseHandle(hCopy);

	// We were successful, so if caller desires, delete the backup file.
	if (::FlushFileBuffers(m_hFile) && 
			bDeleteOnSuccess
				&& REGUTIL::GetConfigDWORD(L"AllowDeleteOnRevert", true)
			)
		VERIFY(::WszDeleteFile(szBackup));

	// Change mode back, after restore everything is back to open mode.
	m_bRewrite = false;
	return (S_OK);
}


//
// Private.
//



//*****************************************************************************
// This version will force the data in cache out to disk for real.  The code
// can handle the different types of storage we might be sitting on based on
// the open type.
//*****************************************************************************
HRESULT StgIO::WriteToDisk(				// Return code.
	const void	*pbBuff,				// Buffer to write.
	ULONG		cbWrite,				// How much.
	ULONG		*pcbWritten)			// Return how much written.
{
	ULONG		cbWritten;				// Buffer for write funcs.
	HRESULT		hr = S_OK;

	// Pretty obvious.
	_ASSERTE(!IsReadOnly());

	// Always need a buffer to write this data to.
	if (!pcbWritten)
		pcbWritten = &cbWritten;

	// Action taken depends on type of storage.
	switch (m_iType)
	{
		case STGIO_HFILE:
		case STGIO_HFILEMEM:
		{
			// Use the file system's move.
			_ASSERTE(m_hFile != INVALID_HANDLE_VALUE);

			// Do the write to disk.
			if (!::WriteFile(m_hFile, pbBuff, cbWrite, pcbWritten, 0))
				hr = MapFileError(GetLastError());
		}
		break;

		// Free the stream pointer.
		case STGIO_STREAM:
		{
			// Delegate write to stream code.
			hr = m_pIStream->Write(pbBuff, cbWrite, pcbWritten);
		}
		break;

		// We cannot write to fixed read/only memory.
		case STGIO_MEM:
		case STGIO_SHAREDMEM:
		_ASSERTE(0);
		hr = BadError(E_UNEXPECTED);
		break;

		// Weird to seek with no data.
		case STGIO_NODATA:
		default:
		_ASSERTE(0);
		break;
	}
	return (hr);
}


//*****************************************************************************
// This version only reads from disk.
//*****************************************************************************
HRESULT StgIO::ReadFromDisk(			// Return code.
	void		*pbBuff,				// Write buffer here.
	ULONG		cbBuff,					// How much to read.
	ULONG		*pcbRead)				// How much read.
{
	ULONG		cbRead;

	_ASSERTE(m_iType == STGIO_HFILE || m_iType == STGIO_STREAM);

	// Need to have a buffer.
	if (!pcbRead)
		pcbRead = &cbRead;

	// Read only from file to avoid recursive logic.
	if (m_iType == STGIO_HFILE || m_iType == STGIO_HFILEMEM)
	{
		if (::ReadFile(m_hFile, pbBuff, cbBuff, pcbRead, 0))
			return (S_OK);
		return (MapFileError(GetLastError()));
	}
	// Read directly from stream.
	else
	{
		return (m_pIStream->Read(pbBuff, cbBuff, pcbRead));
	}
}


//*****************************************************************************
// Copy the contents of the file for this storage to the target path.
//*****************************************************************************
HRESULT StgIO::CopyFileInternal(		// Return code.
	LPCWSTR		szTo,					// Target save path for file.
	int			bFailIfThere,			// true to fail if target exists.
	int			bWriteThrough)			// Should copy be written through OS cache.
{
	DWORD		iCurrent;				// Save original location.
	DWORD		cbRead;					// Byte count for buffer.
	DWORD		cbWrite;				// Check write of bytes.
	BYTE		rgBuff[4096];			// Buffer for copy.
	HANDLE		hFile;					// Target file.
	HRESULT		hr = S_OK;

	// Create target file.
	if ((hFile = ::WszCreateFile(szTo, GENERIC_WRITE, 0, 0, 
			(bFailIfThere) ? CREATE_NEW : CREATE_ALWAYS, 
			(bWriteThrough) ? FILE_FLAG_WRITE_THROUGH : 0, 
			0)) == INVALID_HANDLE_VALUE)
	{
		return (MapFileError(GetLastError()));
	}

	// Save current location and reset it later.
	iCurrent = ::SetFilePointer(m_hFile, 0, 0, FILE_CURRENT);
	::SetFilePointer(m_hFile, 0, 0, FILE_BEGIN);

	// Copy while there are bytes.
	while (::ReadFile(m_hFile, rgBuff, sizeof(rgBuff), &cbRead, 0) && cbRead)
	{
		if (!::WriteFile(hFile, rgBuff, cbRead, &cbWrite, 0) || cbWrite != cbRead)
		{
			hr = STG_E_WRITEFAULT;
			break;
		}
	}

	// Reset file offset.
	::SetFilePointer(m_hFile, iCurrent, 0, FILE_BEGIN);

	// Close target.
	if (!bWriteThrough)
		VERIFY(::FlushFileBuffers(hFile));
	::CloseHandle(hFile);
	return (hr);
}


//*****************************************************************************
// Free the data used for backing store from disk in read/write scenario.
//*****************************************************************************
void StgIO::FreePageMap()
{
	// If a small file was allocated, then free that memory.
	if (m_bFreeMem && m_pBaseData)
		FreeMemory(m_pBaseData);
	// For mmf, close handles and free resources.
	else if (m_hMapping && m_pBaseData)
	{
		VERIFY(UnmapViewOfFile(m_pBaseData));
		VERIFY(CloseHandle(m_hMapping));
	}
	// For our own system, free memory.
	else if (m_rgPageMap && m_pBaseData)
	{
		delete [] m_rgPageMap;
		m_rgPageMap = 0;
		VERIFY(::VirtualFree(m_pBaseData, (((m_cbData - 1) & ~(m_iPageSize - 1)) + m_iPageSize), MEM_DECOMMIT));
		VERIFY(::VirtualFree(m_pBaseData, 0, MEM_RELEASE));
		m_pBaseData = 0;
		m_cbData = 0;	
	}

	m_pBaseData = 0;
	m_hMapping = 0;
	m_cbData = 0;
}


//*****************************************************************************
// Check the given pointer and ensure it is aligned correct.  Return true
// if it is aligned, false if it is not.
//*****************************************************************************
int StgIO::IsAlignedPtr(ULONG_PTR Value, int iAlignment)
{
    HRESULT     hr;
	void		*ptrStart;

	if (m_iType == STGIO_STREAM || m_iType == STGIO_MEM || 
				m_iType == STGIO_SHAREDMEM)
	{
		return ((Value - (ULONG_PTR) m_pData) % iAlignment == 0);
	}
	else
	{
		hr = GetPtrForMem(0, 1, ptrStart);
		_ASSERTE(hr == S_OK && "GetPtrForMem failed");
		_ASSERTE(Value > (ULONG_PTR) ptrStart);
		return (((Value - (ULONG_PTR) ptrStart) % iAlignment) == 0);	
	}
} // int StgIO::IsAlignedPtr()





//*****************************************************************************
// These helper functions are used to allocate fairly large pieces of memory,
// more than should be taken from the runtime heap, but less that would require
// virtual memory overhead.
//*****************************************************************************
// #define _TRACE_MEM_ 1

void *AllocateMemory(int iSize)
{
	void * ptr;
	ptr = HeapAlloc(GetProcessHeap(), 0, iSize);

#if defined(_DEBUG) && defined(_TRACE_MEM_)
	static int i=0;
	DbgWriteEx(L"AllocateMemory: (%d) 0x%08x, size %d\n", ++i, ptr, iSize);
#endif
	return (ptr);
}


void FreeMemory(void *pbData)
{
#if defined(_DEBUG) && defined(_TRACE_MEM_)
	static int i=0;
	DbgWriteEx(L"FreeMemory: (%d) 0x%08x\n", ++i, pbData);
#endif

	_ASSERTE(pbData);
	VERIFY(HeapFree(GetProcessHeap(), 0, pbData));
}

