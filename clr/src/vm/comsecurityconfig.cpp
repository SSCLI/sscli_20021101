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
////////////////////////////////////////////////////////////////////////////////
//
//   File:          COMSecurityConfig.cpp
//
//
//   Purpose:       Native implementation for security config access and manipulation
//
//   Date created : August 30, 2000
//
////////////////////////////////////////////////////////////////////////////////

// The security config system resides outside of the rest
// of the config system since our needs are different.  The
// unmanaged portion of the security config system is only
// concerned with data file/cache file pairs, not what they
// are used for.  It performs all the duties of reading data
// from the disk, saving data back to the disk, and maintaining
// the policy and quick cache data structures.
//
// FILE FORMAT
//
// The data file is a purely opaque blob for the unmanaged
// code; however, the cache file is constructed and maintained
// completely in the unmanaged code.  It's format is as follows:
//
// CacheHeader
//  |
//  +-- configFileTime (FILETIME, 8 bytes) = The file time of the config file associated with this cache file.
//  |
//  +-- isSecurityOn (DWORD, 4 bytes) = This is currently not used.
//  |
//  +-- quickCache (DWORD, 4 bytes) = Used as a bitfield to maintain the information for the QuickCache.  See the QuickCache section for more details.
//  |
//  +-- numEntries (DWORD, 4 bytes) = The number of policy cache entries in the latter portion of this cache file.
//  |
//  +-- sizeConfig (DWORD, 4 bytes) = The size of the config information stored in the latter portion of this cache file.
//
// Config Data (if any)
//     The cache file can include an entire copy of this
//     information in the adjoining config file.  This is
//     necessary since the cache often allows us to make
//     policy decisions without having parsed the data in 
//     the config file.  In order to guarantee that the config
//     data used by this process is not altered in the
//     meantime, we need to store the data in a readonly
//     location.  Due to the design of the caching system
//     the cache file is locked when it is opened and therefore
//     is the perfect place to store this information.  The
//     other alternative is to hold it in memory, but since
//     this can amount to many kilobytes of data we decided
//     on this design.
//
// List of CacheEntries
//  |
//  +-- CacheEntry
//  |    |
//  |    +-- numItemsInKey (DWORD, 4 bytes) = The number of evidence objects serialized in the key blob
//  |    |
//  |    +-- keySize (DWORD, 4 bytes) = The number of bytes in the key blob.
//  |    |
//  |    +-- dataSize (DWORD, 4 bytes) = The number of bytes in the data blob.
//  |    |     
//  |    +-- keyBlob (raw) = A raw blob representing the serialized evidence.
//  |    |
//  |    +-- dataBlob (raw) = A raw blob representing an XML serialized PolicyStatement
//  |
//  +-- ...
//  :
//  :
//
// QUICK CACHE
//
// The QuickCache is my name for one of the many policy resolve optimizations.  This
// particular optimization has two major steps.  First, at policy save time we perform
// analysis on the policy level and form a group of partial-evidence/partial-grant-set
// associations, the result of which are stored in the cache file (in the quickCache
// bit field in the CacheHeader).  The second step involves a set of scattered tests
// that check the QuickCache before doing a full policy resolve.  A more detailed
// explanation of the partial-evidence and partial-grant-sets that we are concerned
// about can be found in /clr/src/bcl/system/security/policymanager.cs


#include "common.h"
#include "comstring.h"
#include "comsecurityconfig.h"
#include "objecthandle.h"
#include "util.hpp"
#include "security.h"
#include "safegetfilesize.h"
#include "eeconfig.h"
#include "version/__file__.ver"

// This controls the maximum size of the cache file.

#define MAX_CACHEFILE_SIZE (1 << 20)



#define SIZE_OF_ENTRY( X )   sizeof( CacheEntryHeader ) + X->header.keySize + X->header.dataSize
#define MAX_NUM_LENGTH 16

static WCHAR* wcscatDWORD( WCHAR* dst, DWORD num )
{
    static WCHAR buffer[MAX_NUM_LENGTH];

    buffer[MAX_NUM_LENGTH-1] = L'\0';

    size_t index = MAX_NUM_LENGTH-2;

    if (num == 0)
    {
        buffer[index--] = L'0';
    }
    else
    {
        while (num != 0)
        {
            buffer[index--] = (WCHAR)(L'0' + (num % 10));
            num = num / 10;
        }
    }

    wcscat( dst, buffer + index + 1 );

    return dst;
}

#define Wszdup(_str) wcscpy(new (throws) WCHAR[wcslen(_str) + 1], (_str))

struct CacheHeader
{
    FILETIME configFileTime;
    DWORD isSecurityOn, quickCache, numEntries, sizeConfig;

    CacheHeader() : isSecurityOn( (DWORD) -1 ), quickCache( 0 ), numEntries( 0 ), sizeConfig( 0 )
    {
        memset( &this->configFileTime, 0, sizeof( configFileTime ) );
    };
};


struct CacheEntryHeader
{
    DWORD numItemsInKey;
    DWORD keySize;
    DWORD dataSize;
};

struct CacheEntry
{
    CacheEntryHeader header;
    BYTE* key;
    BYTE* data;
    DWORD cachePosition;
    BOOL used;

    CacheEntry() : key( NULL ), data( NULL ), used( FALSE ) {};

    ~CacheEntry( void )
    {
        delete [] key;
        delete [] data;
    }
};

struct Data
{
    enum State
    {
        None = 0x0,
        UsingCacheFile = 0x1,
        CopyCacheFile = 0x2,
        CacheUpdated = 0x4,
        UsingConfigFile = 0x10,
        CacheExhausted = 0x20,
        NewConfigFile = 0x40
    };

    INT32 id;
    WCHAR* configFileName;
    WCHAR* cacheFileName;
    WCHAR* cacheFileNameTemp;

    OBJECTHANDLE configData;
    FILETIME configFileTime;
    FILETIME cacheFileTime;
    CacheHeader header;
    ArrayList* oldCacheEntries;
    ArrayList* newCacheEntries;
    State state;
    DWORD cacheCurrentPosition;
    HANDLE cache;
    PBYTE configBuffer;
    DWORD  sizeConfig;
    COMSecurityConfig::ConfigRetval initRetval;
    DWORD newEntriesSize;

    Data( INT32 id )
        : id( id ),
          configFileName( NULL ),
          cacheFileName( NULL ),
          configData( NULL ),
          oldCacheEntries( new ArrayList ),
          newCacheEntries( new ArrayList ),
          state( Data::None ),
          cache( INVALID_HANDLE_VALUE ),
          configBuffer( NULL ),
          newEntriesSize( 0 )
    {
    }

    Data( INT32 id, STRINGREF* configFile )
        : id( id ),
          cacheFileName( NULL ),
          configData( SharedDomain::GetDomain()->CreateHandle( NULL ) ),
          oldCacheEntries( new ArrayList ),
          newCacheEntries( new ArrayList ),
          state( Data::None ),
          cache( INVALID_HANDLE_VALUE ),
          configBuffer( NULL ),
          newEntriesSize( 0 )
    {
        THROWSCOMPLUSEXCEPTION();

        _ASSERTE( *configFile != NULL && "A config file must be specified" );

        configFileName = Wszdup( (*configFile)->GetBuffer() );
        cacheFileName = NULL;
        cacheFileNameTemp = NULL;
    }

    Data( INT32 id, STRINGREF* configFile, STRINGREF* cacheFile )
        : id( id ),
          configData( SharedDomain::GetDomain()->CreateHandle( NULL ) ),
          oldCacheEntries( new ArrayList ),
          newCacheEntries( new ArrayList ),
          state( Data::None ),
          cache( INVALID_HANDLE_VALUE ),
          configBuffer( NULL ),
          newEntriesSize( 0 )
    {
        THROWSCOMPLUSEXCEPTION();

        _ASSERTE( *configFile != NULL && "A config file must be specified" );

        configFileName = Wszdup( (*configFile)->GetBuffer() );

        if (cacheFile != NULL)
        {
            // Since temp cache files can stick around even after the process that
            // created them, we want to make sure they are fairly unique (if they
            // aren't, we'll just fail to save cache information, which is not good
            // but it won't cause anyone to crash or anything).  The unique name
            // algorithm used here is to append the process id and tick count to
            // the name of the cache file.

            cacheFileName = Wszdup( (*cacheFile)->GetBuffer() );
            cacheFileNameTemp = new (throws) WCHAR[wcslen( cacheFileName ) + 1 + 2 * MAX_NUM_LENGTH];
            wcscpy( cacheFileNameTemp, cacheFileName );
            wcscat( cacheFileNameTemp, L"." );
            wcscatDWORD( cacheFileNameTemp, GetCurrentProcessId() );
            wcscat( cacheFileNameTemp, L"." );
            wcscatDWORD( cacheFileNameTemp, GetTickCount() );
        }
        else
        {
            cacheFileName = NULL;
            cacheFileNameTemp = NULL;
        }
    }

    Data( INT32 id, WCHAR* configFile, WCHAR* cacheFile )
        : id( id ),
          configData( SharedDomain::GetDomain()->CreateHandle( NULL ) ),
          oldCacheEntries( new ArrayList ),
          newCacheEntries( new ArrayList ),
          state( Data::None ),
          cache( INVALID_HANDLE_VALUE ),
          configBuffer( NULL ),
          newEntriesSize( 0 )

    {
        THROWSCOMPLUSEXCEPTION();

        _ASSERTE( *configFile != NULL && "A config file must be specified" );

        configFileName = Wszdup( configFile );

        if (cacheFile != NULL)
        {
            cacheFileName = Wszdup( cacheFile );
            cacheFileNameTemp = new (throws) WCHAR[wcslen( cacheFileName ) + 1 + 2 * MAX_NUM_LENGTH];
            wcscpy( cacheFileNameTemp, cacheFileName );
            wcscat( cacheFileNameTemp, L"." );
            wcscatDWORD( cacheFileNameTemp, GetCurrentProcessId() );
            wcscat( cacheFileNameTemp, L"." );
            wcscatDWORD( cacheFileNameTemp, GetTickCount() );
        }
        else
        {
            cacheFileName = NULL;
            cacheFileNameTemp = NULL;
        }
    }

    void Cleanup( void )
    {
        delete [] configBuffer;
        configBuffer = NULL;

        if (cache != INVALID_HANDLE_VALUE)
        {
            CloseHandle( cache );
            cache = INVALID_HANDLE_VALUE;
        }

        if (cacheFileNameTemp != NULL)
        {
            WszDeleteFile( cacheFileNameTemp );
        }

        DestroyHandle( configData );
        configData = SharedDomain::GetDomain()->CreateHandle( NULL );

        DeleteAllEntries();
        header = CacheHeader();

        oldCacheEntries = new ArrayList();
        newCacheEntries = new ArrayList();
    }

    ~Data( void )
    {
        delete [] configBuffer;

        if (cache != INVALID_HANDLE_VALUE)
        {
            CloseHandle( cache );
            cache = INVALID_HANDLE_VALUE;
        }

        if (cacheFileNameTemp != NULL)
        {
            WszDeleteFile( cacheFileNameTemp );
        }

        delete [] configFileName;
        delete [] cacheFileName;
        delete [] cacheFileNameTemp;

        DestroyHandle( configData );

        DeleteAllEntries();
    }

    void DeleteAllEntries( void )
    {
        ArrayList::Iterator iter;
        
        if (oldCacheEntries != NULL)
        {
            iter = oldCacheEntries->Iterate();

            while (iter.Next())
            {
                delete (CacheEntry*) iter.GetElement();
            }

            delete oldCacheEntries;

            oldCacheEntries = NULL;
        }

        if (newCacheEntries != NULL)
        {
            iter = newCacheEntries->Iterate();

            while (iter.Next())
            {
                delete (CacheEntry*) iter.GetElement();
            }

            delete newCacheEntries;
            newCacheEntries = NULL;
        }
    }
};


void* COMSecurityConfig::GetData( INT32 id )
{
    ArrayList::Iterator iter = entries_.Iterate();

    while (iter.Next())
    {
        Data* data = (Data*)iter.GetElement();

        if (data->id == id)
        {
            return data;
        }
    }

    return NULL;
}

static BOOL CacheOutOfDate( FILETIME* configFileTime, WCHAR* configFileName, WCHAR* cacheFileName )
{
    HANDLE config = INVALID_HANDLE_VALUE;
    BOOL retval = TRUE;
    BOOL deleteFile = FALSE;

    config = WszCreateFile( configFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    
    if (config == INVALID_HANDLE_VALUE)
    {
        goto CLEANUP;
    }

    // Get the last write time for both files.
    
    FILETIME newConfigTime;
    
    if (!GetFileTime( config, NULL, NULL, &newConfigTime ))
    {
        goto CLEANUP;
    }
    
    if (CompareFileTime( configFileTime, &newConfigTime ) != 0)
    {
        // Cache is dated.  Delete the cache.
        deleteFile = TRUE;
        goto CLEANUP;
    }

    retval = FALSE;
    
CLEANUP:
    if (config != INVALID_HANDLE_VALUE)
        CloseHandle( config );

    if (deleteFile && cacheFileName != NULL)
        WszDeleteFile( cacheFileName );
        
    return retval;
}

static BOOL CacheOutOfDate( FILETIME* cacheFileTime, HANDLE cache, WCHAR* cacheFileName )
{
    BOOL retval = TRUE;

    // Get the last write time for both files.
    
    FILETIME newCacheTime;
    
    if (!GetFileTime( cache, NULL, NULL, &newCacheTime ))
    {
        goto CLEANUP;
    }
    
    if (CompareFileTime( cacheFileTime, &newCacheTime ) != 0)
    {
        // Cache is dated.  Delete the cache.
        if (cacheFileName != NULL)
        {
            CloseHandle( cache );
            WszDeleteFile( cacheFileName );
        }
        goto CLEANUP;
    }

    retval = FALSE;
    
CLEANUP:
    return retval;
}


static BOOL CacheOutOfDate( FILETIME* configTime, FILETIME* cachedConfigTime )
{
    DWORD result = CompareFileTime( configTime, cachedConfigTime );

    return result != 0;
}

static DWORD GetShareFlags()
{
    DWORD shareFlags;

    shareFlags = FILE_SHARE_READ | FILE_SHARE_DELETE;

    return shareFlags;
}

static DWORD WriteFileData( HANDLE file, PBYTE data, DWORD size )
{
    DWORD totalBytesWritten = 0;
    DWORD bytesWritten;

    do
    {
        if (WriteFile( file, data, size - totalBytesWritten, &bytesWritten, NULL ) == 0)
        {
            return E_FAIL;
        }

        if (bytesWritten == 0)
        {
            return E_FAIL;
        }
        
        totalBytesWritten += bytesWritten;
        
    } while (totalBytesWritten < size);
    
    return S_OK;
}

static DWORD ReadFileData( HANDLE file, PBYTE data, DWORD size )
{
    DWORD totalBytesRead = 0;
    DWORD bytesRead;

    do
    {
        if (ReadFile( file, data, size - totalBytesRead, &bytesRead, NULL ) == 0)
        {
            return E_FAIL;
        }

        if (bytesRead == 0)
        {
            return E_FAIL;
        }
        
        totalBytesRead += bytesRead;
        
    } while (totalBytesRead < size);
    
    return S_OK;
}


FCIMPL2(INT32, COMSecurityConfig::EcallInitData, INT32 id, StringObject* configUNSAFE)
{
    INT32 retVal;
    STRINGREF config = (STRINGREF) configUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(config);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    retVal = InitData( id, config->GetBuffer(), NULL );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND
    
FCIMPL3(INT32, COMSecurityConfig::EcallInitDataEx, INT32 id, StringObject* configUNSAFE, StringObject* cacheUNSAFE)
{
    INT32 retVal;
    STRINGREF config = (STRINGREF) configUNSAFE;
    STRINGREF cache = (STRINGREF) cacheUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(config, cache);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    retVal = InitData( id, config->GetBuffer(), cache->GetBuffer() );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

static U1ARRAYREF AllocateByteArray(DWORD dwSize)
{
    U1ARRAYREF orRet = NULL;

    COMPLUS_TRY
    {
        orRet = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, dwSize);
    }
    COMPLUS_CATCH
    {
        orRet = NULL;
    }
    COMPLUS_END_CATCH

    return orRet;
}

COMSecurityConfig::ConfigRetval COMSecurityConfig::InitData( INT32 id, WCHAR* configFileName, WCHAR* cacheFileName )
{
    Data* data = NULL;

    data = (Data*)GetData( id );

    if (data != NULL)
    {
        return data->initRetval;
    }

    if (configFileName == NULL || wcslen( configFileName ) == 0)
    {
        return NoFile;
    }

    data = new Data( id, configFileName, cacheFileName );

    if (data == NULL)
    {
         return NoFile;
    }

    return InitData( data, TRUE );
}

COMSecurityConfig::ConfigRetval COMSecurityConfig::InitData( void* configDataParam, BOOL addToList )
{
    _ASSERTE( configDataParam != NULL );

    THROWSCOMPLUSEXCEPTION();

    HANDLE config = INVALID_HANDLE_VALUE;
    Data* data = (Data*)configDataParam;
    DWORD cacheSize;
    DWORD configSize;
    U1ARRAYREF configData;
    ConfigRetval retval = NoFile;
    DWORD shareFlags;

    shareFlags = GetShareFlags();

    // Crack open the config file.

    config = WszCreateFile( data->configFileName, GENERIC_READ, shareFlags, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        
    if (config == INVALID_HANDLE_VALUE || !GetFileTime( config, NULL, NULL, &data->configFileTime ))
    {
        memset( &data->configFileTime, 0, sizeof( data->configFileTime ) );
    }
    else
    {
        data->state = (Data::State)(Data::UsingConfigFile | data->state);
    }

    // If we want a cache file, try to open that up.

    if (data->cacheFileName != NULL)
        data->cache = WszCreateFile( data->cacheFileName, GENERIC_READ, shareFlags, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    
    if (data->cache == INVALID_HANDLE_VALUE)
    {
        goto READ_DATA;
    }

    // Validate that the cache file is in a good form by checking
    // that it is at least big enough to contain a header.

    cacheSize = SafeGetFileSize( data->cache, NULL );
    
    if (cacheSize == 0xFFFFFFFF)
    {
        goto READ_DATA;
    }
    
    if (cacheSize < sizeof( CacheHeader ))
    {
        goto READ_DATA;
    }

    // Finally read the data from the file into the buffer.
    
    if (ReadFileData( data->cache, (BYTE*)&data->header, sizeof( CacheHeader ) ) != S_OK)
    {
        goto READ_DATA;
    }

    // Check to make sure the cache file and the config file
    // match up by comparing the actual file time of the config
    // file and the config file time stored in the cache file.

    if (CacheOutOfDate( &data->configFileTime, &data->header.configFileTime ))
    {
        goto READ_DATA;
    }

    if (!GetFileTime( data->cache, NULL, NULL, &data->cacheFileTime ))
    {
        goto READ_DATA;
    }

    // Set the file pointer to after both the header and config data (if any) so
    // that we are ready to read cache entries.

    if (SetFilePointer( data->cache, sizeof( CacheHeader ) + data->header.sizeConfig, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
    {
        goto READ_DATA;
    }

    data->cacheCurrentPosition = sizeof( CacheHeader ) + data->header.sizeConfig;
    data->state = (Data::State)(Data::UsingCacheFile | Data::CopyCacheFile | data->state);

    retval = (ConfigRetval)(retval | CacheFile);

READ_DATA:
    // If we are not using the cache file but we successfully opened it, we need
    // to close it now.  In addition, we need to reset the cache information
    // stored in the Data object to make sure there is no spill over.

    if (data->cache != INVALID_HANDLE_VALUE && (data->state & Data::UsingCacheFile) == 0)
    {
        CloseHandle( data->cache );
        data->header = CacheHeader();
        data->cache = INVALID_HANDLE_VALUE;
    }

    if (config != INVALID_HANDLE_VALUE)
    {
        configSize = SafeGetFileSize( config, NULL );
    
        if (configSize == 0xFFFFFFFF)
        {
            goto ADD_DATA;
        }

        // Be paranoid and only use the cache file version if we find that it has the correct sized
        // blob in it.
        
        if ((data->state & Data::UsingCacheFile) != 0 && configSize == data->header.sizeConfig)
        {
            StoreObjectInHandle( data->configData, NULL );
            goto ADD_DATA;
        }
        else
        {
            if (data->cache != INVALID_HANDLE_VALUE)
            {
                CloseHandle( data->cache );
                data->header = CacheHeader();
                data->cache = INVALID_HANDLE_VALUE;
                data->state = (Data::State)(data->state & ~(Data::UsingCacheFile));
            }

            configData = AllocateByteArray(configSize);
            if (configData == NULL)
            {
                goto ADD_DATA;
            }
    
            if (ReadFileData( config, (PBYTE)(configData->GetDirectPointerToNonObjectElements()), configSize ) != S_OK)
            {
                goto ADD_DATA;
            } 

            StoreObjectInHandle( data->configData, (OBJECTREF)configData );

        }
        retval = (ConfigRetval)(retval | ConfigFile);
    }

ADD_DATA:
    BEGIN_ENSURE_PREEMPTIVE_GC();
    dataLock_.Enter();
    if (addToList)
        entries_.Append( data );
    dataLock_.Leave();
    END_ENSURE_PREEMPTIVE_GC();

    if (config != INVALID_HANDLE_VALUE)
        CloseHandle( config );

	_ASSERTE( data );
	data->initRetval = retval;

    return retval;

};

static CacheEntry* LoadNextEntry( HANDLE cache, Data* data )
{
    if ((data->state & Data::CacheExhausted) != 0)
        return NULL;

    CacheEntry* entry = new (nothrow) CacheEntry();

    if (entry == NULL)
        return NULL;

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (ReadFileData( cache, (BYTE*)&entry->header, sizeof( CacheEntryHeader ) ) != S_OK)
    {
        delete entry;
        entry = NULL;
        goto EXIT;
    }

    entry->cachePosition = data->cacheCurrentPosition + sizeof( entry->header );

    data->cacheCurrentPosition += sizeof( entry->header ) + entry->header.keySize + entry->header.dataSize;

    if (SetFilePointer( cache, entry->header.keySize + entry->header.dataSize, NULL, FILE_CURRENT ) == INVALID_SET_FILE_POINTER)
    {
        delete entry;
        entry = NULL;
        goto EXIT;
    }

    // We append a partially populated entry. CompareEntry is robust enough to handle this.
    data->oldCacheEntries->Append( entry );

EXIT:
    END_ENSURE_PREEMPTIVE_GC();

    return entry;
}

static BOOL WriteEntry( HANDLE cache, CacheEntry* entry, HANDLE oldCache = NULL )
{
    THROWSCOMPLUSEXCEPTION();

    if (WriteFileData( cache, (BYTE*)&entry->header, sizeof( CacheEntryHeader ) ) != S_OK)
    {
        return FALSE;
    }

    if (entry->key == NULL)
    {
        _ASSERTE (oldCache != NULL);

        // We were lazy in reading the entry. Read the key now.
        entry->key = new BYTE[entry->header.keySize];
        if (entry->key == NULL)
            COMPlusThrowOM();

        _ASSERTE (cache != INVALID_HANDLE_VALUE);

        if (SetFilePointer( oldCache, entry->cachePosition, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
            return NULL;

        if (ReadFileData( oldCache, entry->key, entry->header.keySize ) != S_OK)
        {
            return NULL;
        }

        entry->cachePosition += entry->header.keySize;
    }
        
    _ASSERTE( entry->key != NULL );
        
    if (entry->data == NULL)
    {
        _ASSERTE (oldCache != NULL);

        // We were lazy in reading the entry. Read the data also.
        entry->data = new BYTE[entry->header.dataSize];

        if (entry->data == NULL)
            COMPlusThrowOM();

        if (SetFilePointer( oldCache, entry->cachePosition, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
            return NULL;

        if (ReadFileData( oldCache, entry->data, entry->header.dataSize ) != S_OK)
            return NULL;

        entry->cachePosition += entry->header.dataSize;
    }

    _ASSERT( entry->data != NULL );

    if (WriteFileData( cache, entry->key, entry->header.keySize ) != S_OK)
    {
        return FALSE;
    }

    if (WriteFileData( cache, entry->data, entry->header.dataSize ) != S_OK)
    {
        return FALSE;
    }

    return TRUE;
}

FCIMPL1(BOOL, COMSecurityConfig::EcallSaveCacheData, INT32 id)
{
    BOOL retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    retVal = SaveCacheData( id );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

#define MAX_CACHEFILE_SIZE (1 << 20)
#define SIZE_OF_ENTRY( X )   sizeof( CacheEntryHeader ) + X->header.keySize + X->header.dataSize

BOOL COMSecurityConfig::SaveCacheData( INT32 id )
{
    // Note: this function should only be called at EEShutdown time.
    // This is because we need to close the current cache file in
    // order to delete it.  If it ever became necessary to do
    // cache saves while a process we still executing managed code
    // it should be possible to create a locking scheme for usage
    // of the cache handle with very little reordering of the below
    // (as it should always be possible for us to have a live copy of
    // the file and yet still be making the swap).

    HANDLE cache = INVALID_HANDLE_VALUE;
    HANDLE config = INVALID_HANDLE_VALUE;
    CacheHeader header;
    BOOL retval = FALSE;
    BOOL fWriteSucceeded = FALSE;
    DWORD numEntriesWritten = 0;
    DWORD amountWritten = 0;
    DWORD sizeConfig = 0;
    PBYTE configBuffer = NULL;

    Data* data = (Data*)GetData( id );

    // If there is not data by the id or there is no
    // cache file name associated with the data, then fail.

    if (data == NULL || data->cacheFileName == NULL)
        return FALSE;

    // If we haven't added anything new to the cache
    // then just return success.

    if ((data->state & Data::CacheUpdated) == 0)
        return TRUE;

    // If the config file has changed since the process started
    // then our cache data is no longer valid.  We'll just
    // return success in this case.

    if ((data->state & Data::UsingConfigFile) != 0 && CacheOutOfDate( &data->configFileTime, data->configFileName, NULL ))
        return TRUE;

    DWORD fileNameLength = (DWORD)wcslen( data->cacheFileName );

    WCHAR* newFileName = new(nothrow) WCHAR[fileNameLength + 5];

    if (!newFileName)
        return FALSE;

    wcscpy( newFileName, data->cacheFileName );
    wcscpy( &newFileName[fileNameLength], L".new" );

    cache = WszCreateFile( newFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    
    if (cache == INVALID_HANDLE_VALUE)
        goto CLEANUP;

    // This code seems complicated only because of the
    // number of cases that we are trying to handle.  All we
    // are trying to do is determine the amount of space to
    // leave for the config information.

    // If we saved out a new config file during this run, use
    // the config size stored in the Data object itself.

    if ((data->state & Data::NewConfigFile) != 0)
    {
        sizeConfig = data->sizeConfig;
    }

    // If we have a cache file, then use the size stored in the
    // cache header.

    else if ((data->state & Data::UsingCacheFile) != 0)
    {
        sizeConfig = data->header.sizeConfig;
    }

    // If we read in the config data, use the size of the
    // managed byte array that it is stored in.

    else if (ObjectFromHandle( data->configData ) != NULL)
    {
        sizeConfig = ((U1ARRAYREF)ObjectFromHandle( data->configData ))->GetNumComponents();
    }

    // Otherwise, check the config file itself to get the size.

    else
    {
        DWORD shareFlags;

        shareFlags = GetShareFlags();

        config = WszCreateFile( data->configFileName, GENERIC_READ, shareFlags, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if (config == INVALID_HANDLE_VALUE)
        {
            sizeConfig = 0;
        }
        else
        {
            sizeConfig = SafeGetFileSize( config, NULL );

            if (sizeConfig == 0xFFFFFFFF)
            {
                sizeConfig = 0;
                CloseHandle( config );
                config = INVALID_HANDLE_VALUE;
            }
        }
    }

    // First write the entries.

    if (SetFilePointer( cache, sizeof( CacheHeader ) + sizeConfig, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
    {
        goto CLEANUP;
    }

    // We're going to write out the cache entries in a modified
    // least recently used order, throwing out any that end up
    // taking us past our hardcoded max file size.

    COMPLUS_TRY
    {
        // First, write the entries from the cache file that were used.
        // We do this because presumably these are system assemblies
        // and other assemblies used by a number of applications.
    
        ArrayList::Iterator iter;

        if ((data->state & Data::UsingCacheFile) != 0)
        {
            iter = data->oldCacheEntries->Iterate();

            while (iter.Next() && amountWritten < MAX_CACHEFILE_SIZE)
            {
                CacheEntry* currentEntry = (CacheEntry*)iter.GetElement();

                if (currentEntry->used)
                {
                    if(!WriteEntry( cache, currentEntry, data->cache ))
                    {
                        COMPLUS_LEAVE;
                    }

                    amountWritten += SIZE_OF_ENTRY( currentEntry );
                    numEntriesWritten++;
                }
            }
        }

        // Second, write any new cache entries to the file.  These are
        // more likely to be assemblies specific to this app.

        iter = data->newCacheEntries->Iterate();

        while (iter.Next() && amountWritten < MAX_CACHEFILE_SIZE)
        {
            CacheEntry* currentEntry = (CacheEntry*)iter.GetElement();
    
            if (!WriteEntry( cache, currentEntry ))
            {
                COMPLUS_LEAVE;
            }
    
            amountWritten += SIZE_OF_ENTRY( currentEntry );
            numEntriesWritten++;
        }

        // Third, if we are using the cache file, write the old entries
        // that were not used this time around.

        if ((data->state & Data::UsingCacheFile) != 0)
        {
            // First, write the ones that we already have partially loaded

            iter = data->oldCacheEntries->Iterate();

            while (iter.Next() && amountWritten < MAX_CACHEFILE_SIZE)
            {
                CacheEntry* currentEntry = (CacheEntry*)iter.GetElement();

                if (!currentEntry->used)
                {
                    if(!WriteEntry( cache, currentEntry, data->cache ))
                    {
                        COMPLUS_LEAVE;
                    }

                    amountWritten += SIZE_OF_ENTRY( currentEntry );
                    numEntriesWritten++;
                }
            }

            while (amountWritten < MAX_CACHEFILE_SIZE)
            {
                CacheEntry* entry = LoadNextEntry( data->cache, data );

                if (entry == NULL)
                    break;

                if (!WriteEntry( cache, entry, data->cache ))
                {
                    COMPLUS_LEAVE;
                }
        
                amountWritten += SIZE_OF_ENTRY( entry );
                numEntriesWritten++;
            }
        }

        fWriteSucceeded = TRUE;
    }
    COMPLUS_CATCH
    {
    }
    COMPLUS_END_CATCH

    if (!fWriteSucceeded)
    {
        CloseHandle( cache );
        cache = INVALID_HANDLE_VALUE;
        WszDeleteFile( newFileName );
        goto CLEANUP;
    }

    // End with writing the header.

    header.configFileTime = data->configFileTime;
    header.isSecurityOn = 1;
    header.numEntries = numEntriesWritten;
    header.quickCache = data->header.quickCache;
    header.sizeConfig = sizeConfig;

    if (SetFilePointer( cache, 0, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
    {
        // Couldn't move to the beginning of the file
        goto CLEANUP;
    }
        
    if (WriteFileData( cache, (PBYTE)&header, sizeof( header ) ) != S_OK)
    {
        // Couldn't write header info.
        goto CLEANUP;
    }

    if (sizeConfig != 0)
    {
        if ((data->state & Data::NewConfigFile) != 0)
        {
            if (WriteFileData( cache, data->configBuffer, sizeConfig ) != S_OK)
            {
                goto CLEANUP;
            }
        }
        else
        {
            U1ARRAYREF configData = (U1ARRAYREF)ObjectFromHandle( data->configData );

            if (configData != NULL)
            {
                _ASSERTE( sizeConfig == configData->GetNumComponents() && "sizeConfig is set to the wrong value" );

                if (WriteFileData( cache, (PBYTE)(configData->GetDirectPointerToNonObjectElements()), sizeConfig ) != S_OK)
                {
                    goto CLEANUP;
                }
            }
            else if ((data->state & Data::UsingCacheFile) != 0)
            {
                configBuffer = new BYTE[sizeConfig];
    
                if (SetFilePointer( data->cache, sizeof( CacheHeader ), NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
                {
                    goto CLEANUP;
                }

                if (ReadFileData( data->cache, configBuffer, sizeConfig ) != S_OK)
                {
                    goto CLEANUP;
                }

                if (WriteFileData( cache, configBuffer, sizeConfig ) != S_OK)
                {
                    goto CLEANUP;
                }
            }
            else
            {
                configBuffer = new BYTE[sizeConfig];
    
                if (SetFilePointer( config, 0, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
                {
                    goto CLEANUP;
                }

                if (ReadFileData( config, configBuffer, sizeConfig ) != S_OK)
                {
                    goto CLEANUP;
                }

                if (WriteFileData( cache, configBuffer, sizeConfig ) != S_OK)
                {
                    goto CLEANUP;
                }
            }
        }
    }

    // Flush the file buffers to make sure
    // we get full write through.

    FlushFileBuffers( cache );

    CloseHandle( cache );
    cache = INVALID_HANDLE_VALUE;
    CloseHandle( data->cache );
    data->cache = INVALID_HANDLE_VALUE;

    // Move the existing file out of the way
    // Note: use MoveFile because we know it will never cross
    // device boundaries.

    // Note: the delete file can fail, but we can't really do anything
    // if it does so just ignore any failures.
    WszDeleteFile( data->cacheFileNameTemp );

    // Try to move the existing cache file out of the way.  However, if we can't
    // then try to delete it.  If it can't be deleted then just bail out.
    if (!WszMoveFile( data->cacheFileName, data->cacheFileNameTemp ) && (GetLastError() != ERROR_FILE_NOT_FOUND) && !WszDeleteFile( data->cacheFileNameTemp ))
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
            goto CLEANUP;
    }

    // Move the new file into position

    if (!WszMoveFile( newFileName, data->cacheFileName ))
    {
        goto CLEANUP;
    }

    retval = TRUE;

CLEANUP:
    if (newFileName != NULL)
        delete [] newFileName;

    if (config != INVALID_HANDLE_VALUE)
        CloseHandle( config );

    if (configBuffer != NULL)
        delete [] configBuffer;

    if (!retval && cache != INVALID_HANDLE_VALUE)
        CloseHandle( cache );

    return retval;

}

FCIMPL1(void, COMSecurityConfig::EcallClearCacheData, INT32 id)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    ClearCacheData( id );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

void COMSecurityConfig::ClearCacheData( INT32 id )
{
    Data* data = (Data*)GetData( id );

    if (data == NULL)
        return;

    data->DeleteAllEntries();

    data->oldCacheEntries = new ArrayList;
    data->newCacheEntries = new ArrayList;

    data->header = CacheHeader();
    data->state = (Data::State)(~(Data::CopyCacheFile | Data::UsingCacheFile) & data->state);

    if (data->cache != INVALID_HANDLE_VALUE)
    {
        CloseHandle( data->cache );
        data->cache = INVALID_HANDLE_VALUE;
    }

    DWORD shareFlags;

    shareFlags = GetShareFlags();

    HANDLE config = WszCreateFile( data->configFileName, 0, shareFlags, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    if (config == INVALID_HANDLE_VALUE)
    {
        return;
    }

    GetFileTime( config, NULL, NULL, &data->configFileTime );
    GetFileTime( config, NULL, NULL, &data->header.configFileTime );

    CloseHandle( config );
}


FCIMPL1(void, COMSecurityConfig::EcallResetCacheData, INT32 id)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    ResetCacheData( id );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

void COMSecurityConfig::ResetCacheData( INT32 id )
{
    Data* data = (Data*)GetData( id );

    if (data == NULL)
        return;

    data->DeleteAllEntries();

    data->oldCacheEntries = new ArrayList;
    data->newCacheEntries = new ArrayList;

    data->header = CacheHeader();
    data->state = (Data::State)(~(Data::CopyCacheFile | Data::UsingCacheFile) & data->state);

    DWORD shareFlags;

    shareFlags = GetShareFlags();

    HANDLE config = WszCreateFile( data->configFileName, GENERIC_READ, shareFlags, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    if (config == INVALID_HANDLE_VALUE)
    {
        _ASSERTE( FALSE && "Unable to open config file during cache reset" );
        return;
    }

    GetFileTime( config, NULL, NULL, &data->configFileTime );
    GetFileTime( config, NULL, NULL, &data->header.configFileTime );

    CloseHandle( config );
}


FCIMPL2(BOOL, COMSecurityConfig::EcallSaveDataString, INT32 id, StringObject* dataUNSAFE)
{
    BOOL retVal;
    STRINGREF data = (STRINGREF) dataUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(data);
    //-[autocvtpro]-------------------------------------------------------

    retVal = SaveData( id, data->GetBuffer(), data->GetStringLength() * sizeof( WCHAR ) );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

FCIMPL4(BOOL, COMSecurityConfig::EcallSaveDataByte, INT32 id, U1Array* dataUNSAFE, INT32 offset, INT32 length)
{
    BOOL retVal;
    U1ARRAYREF data = (U1ARRAYREF) dataUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(data);
    //-[autocvtpro]-------------------------------------------------------

    retVal = SaveData( id, data->GetDirectPointerToNonObjectElements() + offset, length );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

BOOL COMSecurityConfig::SaveData( INT32 id, void* buffer, size_t bufferSize )
{
    Data* data = (Data*)GetData( id );

    if (data == NULL)
        return FALSE;

    HANDLE newFile = INVALID_HANDLE_VALUE;

    int RetryCount;
    DWORD error = 0;
    BOOL retval = FALSE;
    DWORD fileNameLength = (DWORD)wcslen( data->configFileName );

    WCHAR* newFileName = new (nothrow) WCHAR[fileNameLength + 5];
    WCHAR* oldFileName = new (nothrow) WCHAR[fileNameLength + 5];

    if (newFileName == NULL || oldFileName == NULL)
        return FALSE;

    wcscpy( newFileName, data->configFileName );
    wcscpy( &newFileName[fileNameLength], L".new" );
    wcscpy( oldFileName, data->configFileName );
    wcscpy( &oldFileName[fileNameLength], L".old" );

    // Create the new file.
    for (RetryCount = 0; RetryCount < 5; RetryCount++) {
        newFile = WszCreateFile( newFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );        
    
        if (newFile != INVALID_HANDLE_VALUE)
	{
	    break;
	}
        else
        {
            error = GetLastError();

            if (error == ERROR_PATH_NOT_FOUND)
            {
                // The directory does not exist, iterate through and try to create it.

                WCHAR* currentChar = newFileName;

                // Skip the first backslash

                while (*currentChar != L'\0')
                {
                    if (*currentChar == L'\\' || *currentChar == L'/')
                    {
                        currentChar++;
                        break;
                    }
                    currentChar++;
                }

                // Iterate through trying to create each subdirectory.

                while (*currentChar != L'\0')
                {
                    if (*currentChar == L'\\' || *currentChar == L'/')
                    {
                        *currentChar = L'\0';

                        if (!WszCreateDirectory( newFileName, NULL ))
                        {
                            error = GetLastError();

                            if (error != ERROR_ACCESS_DENIED && error != ERROR_ALREADY_EXISTS)
                            {
                                goto CLEANUP;
                            }
                        }

                        *currentChar = L'\\';
                    }
                    currentChar++;
                }

                // Try the file creation again
                continue;
            }
        }

        // CreateFile failed.  Sleep a little and retry, in case a
        // virus scanner caused the creation to fail.
        Sleep(10);
    }
    if (newFile == INVALID_HANDLE_VALUE) {
        _ASSERTE(error==0); // Make the Win32 error code available for
	                    // debugging
        goto CLEANUP;
    }
    
    // Write the data into it.

    if (WriteFileData( newFile, (PBYTE)buffer, (DWORD)bufferSize ) != S_OK)
    {
        // Write failed, destroy the file and bail.
        CloseHandle( newFile );
        WszDeleteFile( newFileName );
        goto CLEANUP;
    }

    FlushFileBuffers( newFile );
    CloseHandle( newFile );

    // Move the existing file out of the way
    
    if (!WszMoveFileEx( data->configFileName, oldFileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ))
    {
        // If move fails for a reason other than not being able to find the file, bail out.
        // Also, if the old file didn't exist, we have no need to delete it.

        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            if (!WszDeleteFile( data->configFileName ))
                goto CLEANUP;
        }
    }

    // Move the new file into position

    if (!WszMoveFileEx( newFileName, data->configFileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ))
    {
        goto CLEANUP;
    }

    if (data->configBuffer != NULL)
    {
        delete [] data->configBuffer;
    }

    data->configBuffer = new BYTE[bufferSize];

    memcpyNoGCRefs( data->configBuffer, buffer, bufferSize );
    data->sizeConfig = (DWORD)bufferSize;

    data->state = (Data::State)(data->state | Data::NewConfigFile);

    retval = TRUE;

CLEANUP:
    if (newFileName != NULL)
        delete [] newFileName;

    if (oldFileName != NULL)
        delete [] oldFileName;
        
    return retval;
}

FCIMPL1(BOOL, COMSecurityConfig::EcallRecoverData, INT32 id)
{
    BOOL retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    retVal = RecoverData( id );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

BOOL COMSecurityConfig::RecoverData( INT32 id )
{
    Data* data = (Data*)GetData( id );

    BOOL retval = FALSE;

    if (data == NULL)
        return retval;

    DWORD fileNameLength = (DWORD)wcslen( data->configFileName );

    WCHAR* tempFileName = new (nothrow) WCHAR[fileNameLength + 10];
    WCHAR* oldFileName = new (nothrow) WCHAR[fileNameLength + 5];

    if (tempFileName == NULL || oldFileName == NULL)
        return retval;

    wcscpy( tempFileName, data->configFileName );
    wcscpy( &tempFileName[fileNameLength], L".old.temp" );
    wcscpy( oldFileName, data->configFileName );
    wcscpy( &oldFileName[fileNameLength], L".old" );

    HANDLE oldFile = WszCreateFile( oldFileName, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );        

    if (oldFile == INVALID_HANDLE_VALUE)
    {
        goto CLEANUP;
    }

    CloseHandle( oldFile );

    if (!WszMoveFile( data->configFileName, tempFileName ))
    {
        goto CLEANUP;
    }

    if (!WszMoveFile( oldFileName, data->configFileName ))
    {
        goto CLEANUP;
    }

    if (!WszMoveFile( tempFileName, oldFileName ))
    {
        goto CLEANUP;
    }

    // We need to do some work to reset the unmanaged data object
    // so that the managed side of things behaves like you'd expect.
    // This basically means cleaning up the open resources and
    // doing the work to init on a different set of files.

    data->Cleanup();
    InitData( data, FALSE );

    retval = TRUE;

CLEANUP:
    if (tempFileName != NULL)
        delete [] tempFileName;

    if (oldFileName != NULL)
        delete [] oldFileName;

    return retval;
}


FCIMPL1(Object*, COMSecurityConfig::GetRawData, INT32 id)
{
    U1ARRAYREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
    //-[autocvtpro]-------------------------------------------------------

    Data* data = (Data*)GetData( id );

    if (data == NULL)
    {
        goto lFnExit;
    }

    if (data->configData != NULL)
    {
        U1ARRAYREF configData = (U1ARRAYREF)ObjectFromHandle( data->configData );

        if (configData == NULL && ((data->state & Data::UsingCacheFile) != 0))
        {
            // Read the config data out of the place it is stored in the cache.
            // Note: we open a new handle to the file to make sure we don't 
            // move the file pointer on the existing handle.

            HANDLE cache = INVALID_HANDLE_VALUE;

            if (data->header.sizeConfig == 0)
            {
                goto EXIT;
            }

            DWORD shareFlags;

            shareFlags = GetShareFlags();

            cache = WszCreateFile( data->cacheFileName, GENERIC_READ, shareFlags, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            
            if (cache == INVALID_HANDLE_VALUE)
            {
                _ASSERTE( FALSE && "Unable to open cache file to read config info" );
                goto EXIT;
            }

            if (SetFilePointer( cache, sizeof( CacheHeader ), NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
            {
                goto EXIT;
            }

            configData = (U1ARRAYREF)AllocateByteArray(data->header.sizeConfig);
            if (configData == NULL)
            {
                goto EXIT;
            }
    
            if (ReadFileData( cache, (PBYTE)(configData->GetDirectPointerToNonObjectElements()), data->header.sizeConfig ) != S_OK)
            {
                configData = NULL;
                goto EXIT;
            }

            StoreObjectInHandle( data->configData, (OBJECTREF)configData );
EXIT:
            if (cache != INVALID_HANDLE_VALUE)
                CloseHandle( cache );
        }

        refRetVal = configData;
    }

lFnExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


FCIMPL2(DWORD, COMSecurityConfig::EcallGetQuickCacheEntry, INT32 id, QuickCacheEntryType type)
{
    DWORD retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    retVal = GetQuickCacheEntry( id, type );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


DWORD COMSecurityConfig::GetQuickCacheEntry( INT32 id, QuickCacheEntryType type )
{
    Data* data = (Data*)GetData( id );

    if (data == NULL || (data->state & Data::UsingCacheFile) == 0)
        return 0;

    return (DWORD)(data->header.quickCache & type);
}

FCIMPL2(void, COMSecurityConfig::EcallSetQuickCache, INT32 id, QuickCacheEntryType type)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    SetQuickCache( id, type );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


void COMSecurityConfig::SetQuickCache( INT32 id, QuickCacheEntryType type )
{
    Data* data = (Data*)GetData( id );

    if (data == NULL)
        return;

    if ((DWORD)type != data->header.quickCache)
    {
        BEGIN_ENSURE_PREEMPTIVE_GC();
        dataLock_.Enter();
        data->state = (Data::State)(Data::CacheUpdated | data->state);
        data->header.quickCache = type;
        dataLock_.Leave();
        END_ENSURE_PREEMPTIVE_GC();
    }
}

static HANDLE OpenCacheFile( Data* data )
{
    HANDLE retval = NULL;

    BEGIN_ENSURE_PREEMPTIVE_GC();

    COMSecurityConfig::dataLock_.Enter();

    if (data->cache != INVALID_HANDLE_VALUE)
    {
        retval = data->cache;
        goto EXIT;
    }

    _ASSERTE( FALSE && "This case should never happen" );

    data->cache = WszCreateFile( data->cacheFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    
    if (data->cache == INVALID_HANDLE_VALUE)
    {
        goto EXIT;
    }

    // Check whether the cache has changed since we first looked at it.
    // If it has but the config file hasn't, then we need to start fresh.
    // However, if the config file has changed then we have to ignore it.

    if (CacheOutOfDate( &data->cacheFileTime, data->cache, NULL ))
    {
        if (CacheOutOfDate( &data->configFileTime, data->configFileName, NULL ))
        {
            goto EXIT;
        }

        if (ReadFileData( data->cache, (BYTE*)&data->header, sizeof( CacheHeader ) ) != S_OK)
        {
            goto EXIT;
        }

        data->cacheCurrentPosition = sizeof( CacheHeader );

        if (data->oldCacheEntries != NULL)
        {
            ArrayList::Iterator iter = data->oldCacheEntries->Iterate();
    
            while (iter.Next())
            {
                delete (CacheEntry*)iter.GetElement();
            }
    
            delete data->oldCacheEntries;
            data->oldCacheEntries = new ArrayList();
        }
    }

    retval = data->cache;

EXIT:
    COMSecurityConfig::dataLock_.Leave();
    END_ENSURE_PREEMPTIVE_GC();
    return retval;
}

static BYTE* CompareEntry( CacheEntry* entry, DWORD numEvidence, DWORD evidenceSize, BYTE* evidenceBlock, HANDLE cache, DWORD* size)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE (entry);

    if (entry->header.numItemsInKey == numEvidence &&
        entry->header.keySize == evidenceSize)
    {
        if (entry->key == NULL)
        {
            // We were lazy in reading the entry. Read the key now.
            entry->key = new BYTE[entry->header.keySize];
            if (entry->key == NULL)
                COMPlusThrowOM();

            _ASSERTE (cache != INVALID_HANDLE_VALUE);

            if (SetFilePointer( cache, entry->cachePosition, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
                return NULL;

            if (ReadFileData( cache, entry->key, entry->header.keySize ) != S_OK)
                return NULL;

            entry->cachePosition += entry->header.keySize;
        }
        
        _ASSERTE (entry->key);
        
        if (memcmp( entry->key, evidenceBlock, entry->header.keySize ) == 0)
        {
            if (entry->data == NULL)
            {
                // We were lazy in reading the entry. Read the data also.
                entry->data = new BYTE[entry->header.dataSize];

                if (entry->data == NULL)
                    COMPlusThrowOM();

                if (SetFilePointer( cache, entry->cachePosition, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
                    return NULL;

                if (ReadFileData( cache, entry->data, entry->header.dataSize ) != S_OK)
                    return NULL;

                entry->cachePosition += entry->header.dataSize;
            }

            entry->used = TRUE;
            *size = entry->header.dataSize;

            return entry->data;
        }
    }
    return NULL;
}


FCIMPL4(BOOL, COMSecurityConfig::GetCacheEntry, INT32 id, DWORD numEvidence, CHARArray* evidenceUNSAFE, CHARARRAYREF* policy)
{
    BOOL retVal;
    CHARARRAYREF evidence = (CHARARRAYREF) evidenceUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(evidence);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HANDLE cache = INVALID_HANDLE_VALUE;
    BOOL success = FALSE;
    DWORD size = (DWORD) -1;

    DWORD evidenceSize = evidence->GetNumComponents() * sizeof( WCHAR );
    BYTE* evidenceBlock = new(throws) BYTE[evidenceSize];
    memcpyNoGCRefs( evidenceBlock, evidence->GetDirectPointerToNonObjectElements(), evidenceSize );

    *policy = NULL;

    BYTE* retval = NULL;

    Data* data = (Data*)GetData( id );

    BEGIN_ENSURE_PREEMPTIVE_GC();

    ArrayList::Iterator iter;

    if (data == NULL)
    {
        goto CLEANUP;
    }

    if ((data->state & Data::UsingCacheFile) == 0)
    {
        // We know we don't have anything in the config file, so
        // let's just look through the new entries to make sure we
        // aren't getting any repeats.

        // Then try the existing new entries

        iter = data->newCacheEntries->Iterate();

        while (iter.Next())
        {
            // newCacheEntries do not need the cache file so pass in NULL.
            retval = CompareEntry( (CacheEntry*)iter.GetElement(), numEvidence, evidenceSize, evidenceBlock, NULL, &size );

            if (retval != NULL)
            {
                success = TRUE;
                goto CLEANUP;
            }
        }

        goto CLEANUP;
    }

    // Its possible that the old entries were not read in completely
    // so we keep the cache file open before iterating through the
    // old entries.

    cache = OpenCacheFile( data );

    if ( cache == NULL )
    {
        goto CLEANUP;
    }

    // First, iterator over the old entries

    dataLock_.Enter();

    iter = data->oldCacheEntries->Iterate();

    while (iter.Next())
    {
        retval = CompareEntry( (CacheEntry*)iter.GetElement(), numEvidence, evidenceSize, evidenceBlock, cache, &size );

        if (retval != NULL)
        {
            success = TRUE;
            goto UNLOCKING_CLEANUP;
        }
    }

    dataLock_.Leave();

    // Then try the existing new entries

    iter = data->newCacheEntries->Iterate();

    while (iter.Next())
    {
        // newCacheEntries do not need the cache file so pass in NULL.
        retval = CompareEntry( (CacheEntry*)iter.GetElement(), numEvidence, evidenceSize, evidenceBlock, NULL, &size );

        if (retval != NULL)
        {
            success = TRUE;
            goto CLEANUP;
        }
    }

    // Finally, try loading existing entries from the file

    dataLock_.Enter();

    if (SetFilePointer( cache, data->cacheCurrentPosition, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER)
    {
        goto UNLOCKING_CLEANUP;
    }

    do
    {
        CacheEntry* entry = LoadNextEntry( cache, data );

        if (entry == NULL)
        {
            data->state = (Data::State)(Data::CacheExhausted | data->state);
            break;
        }

        retval = CompareEntry( entry, numEvidence, evidenceSize, evidenceBlock, cache, &size );

        if (retval != NULL)
        {
            success = TRUE;
            break;
        }
    } while (TRUE);

UNLOCKING_CLEANUP:
    dataLock_.Leave();

CLEANUP:
    END_ENSURE_PREEMPTIVE_GC();

    delete [] evidenceBlock;

    if (success && retval != NULL)
    {
        _ASSERTE( size != (DWORD) -1 );
        *policy = (CHARARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_CHAR, size);
        memcpyNoGCRefs( (*policy)->GetDirectPointerToNonObjectElements(), retval, size );
    }

    retVal = success;

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

FCIMPL4(void, COMSecurityConfig::AddCacheEntry, INT32 id, DWORD numEvidence, CHARArray* evidenceUNSAFE, CHARArray* policyUNSAFE)
{
    CHARARRAYREF evidence = (CHARARRAYREF) evidenceUNSAFE;
    CHARARRAYREF policy = (CHARARRAYREF) policyUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_2(evidence, policy);
    //-[autocvtpro]-------------------------------------------------------

    Data* data = (Data*)GetData( id );

    DWORD sizeOfEntry = 0;
	CacheEntry* entry = NULL;

    if (data == NULL)
    {
        goto lExit;
    }

    // In order to limit how large a long running app can become,
    // we limit the total memory held by the new cache entries list.
    // For now this limit corresponds with how large the max cache file
    // can be.

    sizeOfEntry = sizeof( WCHAR ) * evidence->GetNumComponents() +
                        sizeof( WCHAR ) * policy->GetNumComponents() +
                        sizeof( CacheEntryHeader );

    if (data->newEntriesSize + sizeOfEntry >= MAX_CACHEFILE_SIZE)
    {
        goto lExit;
    }

    entry = new(nothrow) CacheEntry();

    if (entry == NULL)
    {
        goto lExit;
    }

    entry->header.numItemsInKey = numEvidence;
    entry->header.keySize = sizeof( WCHAR ) * evidence->GetNumComponents();
    entry->header.dataSize = sizeof( WCHAR ) * policy->GetNumComponents();

    entry->key = new(nothrow) BYTE[entry->header.keySize];
    entry->data = new(nothrow) BYTE[entry->header.dataSize];

    if (entry->key == NULL || entry->data == NULL)
    {
        delete entry;
        goto lExit;
    }

    memcpyNoGCRefs( entry->key, evidence->GetDirectPointerToNonObjectElements(), entry->header.keySize );
    memcpyNoGCRefs( entry->data, policy->GetDirectPointerToNonObjectElements(), entry->header.dataSize );

    BEGIN_ENSURE_PREEMPTIVE_GC();
    dataLock_.Enter();

    // Check the size again to handle the race.

    if (data->newEntriesSize + sizeOfEntry >= MAX_CACHEFILE_SIZE)
    {
        delete entry;
    }
    else
    {
        data->state = (Data::State)(Data::CacheUpdated | data->state);
        data->newCacheEntries->Append( entry );
        data->newEntriesSize += sizeOfEntry;
    }

    dataLock_.Leave();
    END_ENSURE_PREEMPTIVE_GC();

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL1(DWORD, COMSecurityConfig::GetCacheSecurityOn, INT32 id)
{
    DWORD retVal = (DWORD) -1;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    Data* data = (Data*)GetData( id );

    if (data != NULL)
    {
        retVal = data->header.isSecurityOn;
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


FCIMPL2(void, COMSecurityConfig::SetCacheSecurityOn, INT32 id, INT32 value)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    Data* data = (Data*)GetData( id );
    
    if (data != NULL)
    {
        data->state = (Data::State)(Data::CacheUpdated | data->state);
        data->header.isSecurityOn = value;
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

ArrayListStatic COMSecurityConfig::entries_;
CrstStatic      COMSecurityConfig::dataLock_;

void COMSecurityConfig::Init( void )
{
    THROWSCOMPLUSEXCEPTION();

    dataLock_.Init("Security Policy Cache Lock", CrstSecurityPolicyCache, FALSE, FALSE );
    entries_.Init();
}

void COMSecurityConfig::Cleanup( void )
{
    ArrayList::Iterator iter = entries_.Iterate();

    while (iter.Next())
    {
        delete (Data*) iter.GetElement();
    }

    entries_.Destroy();
    dataLock_.Destroy();
}

FCIMPL0(BOOL, COMSecurityConfig::EcallIsCompilationDomain)
{
    BOOL retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    retVal = (SystemDomain::GetCurrentDomain()->IsCompilationDomain() || (g_pConfig && g_pConfig->RequireZaps()));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

FCIMPL0(Object*, COMSecurityConfig::EcallGetMachineDirectory)
{
    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
    //-[autocvtpro]-------------------------------------------------------

    WCHAR machine[MAX_PATH];
    size_t machineCount = MAX_PATH;

    BOOL result = GetMachineDirectory( machine, machineCount );

    _ASSERTE( result );
    _ASSERTE( wcslen( machine ) != 0 );

    if (result)
    {
        refRetVal = COMString::NewString( machine );
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL0(Object*, COMSecurityConfig::EcallGetUserDirectory)
{
    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
    //-[autocvtpro]-------------------------------------------------------

    WCHAR user[MAX_PATH];
    size_t userCount = MAX_PATH;

    BOOL result = GetUserDirectory( user, userCount, FALSE );

    if (result)
    {
        refRetVal = COMString::NewString( user );
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


BOOL COMSecurityConfig::GetMachineDirectory( WCHAR* buffer, size_t bufferCount )
{
    DWORD length;
    BOOL retval = FALSE;

    if (FAILED(GetCORSystemDirectory( buffer, (DWORD)bufferCount, &length )))
    {
        goto CLEANUP;
    }

    wcscat( buffer, L"config\\" );

    retval = TRUE;

CLEANUP:

    return retval;;
}


BOOL COMSecurityConfig::GetUserDirectory( WCHAR* buffer, size_t bufferCount, BOOL fTryDefault )
{
    WCHAR scratchBuffer[MAX_PATH];
    BOOL retval = FALSE;


    if (!PAL_GetUserConfigurationDirectory(buffer, (UINT)bufferCount))
        goto CLEANUP;


    wcscpy( scratchBuffer, L"\\Microsoft\\CLR Security Config\\v" );
    wcscat( scratchBuffer, VER_SBSFILEVERSION_WSTR );
    wcscat( scratchBuffer, L"\\" );

    if (bufferCount < wcslen( buffer ) + wcslen( scratchBuffer ) + 1)
    {
        goto CLEANUP;
    }

    wcscat( buffer, scratchBuffer );

    retval = TRUE;

CLEANUP:
    return retval;
}


BOOL COMSecurityConfig::WriteToEventLog( WCHAR* message )
{

    return FALSE;

}


FCIMPL1(BOOL, COMSecurityConfig::EcallWriteToEventLog, StringObject* messageUNSAFE)
{
    BOOL retVal = FALSE;
    STRINGREF message = (STRINGREF) messageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(message);
    //-[autocvtpro]-------------------------------------------------------

    WCHAR messageBuf[1024];
    WCHAR* wszMessage;

    BEGIN_ENSURE_COOPERATIVE_GC();

    if (message->GetStringLength() > 1024)
    {
        wszMessage = new (nothrow) WCHAR[message->GetStringLength() + 1];
    }
    else
    {
        wszMessage = messageBuf;
    }

    END_ENSURE_COOPERATIVE_GC();

    if (wszMessage != NULL)
    {
        wcscpy( wszMessage, message->GetBuffer() );

        retVal = WriteToEventLog( wszMessage );

        if (wszMessage != messageBuf)
        {
            delete [] wszMessage;
        }
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


FCIMPL1(Object*, COMSecurityConfig::EcallGetStoreLocation, INT32 id)
{
    STRINGREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
    //-[autocvtpro]-------------------------------------------------------

    WCHAR path[MAX_PATH];
    size_t pathCount = MAX_PATH;

    BOOL result = GetStoreLocation( id, path, pathCount );

    _ASSERTE( result );
    _ASSERTE( wcslen( path ) != 0 );

    if (result)
    {
        refRetVal = COMString::NewString( path );
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

BOOL COMSecurityConfig::GetStoreLocation( INT32 id, WCHAR* buffer, size_t bufferCount )
{
    Data* data = (Data*)GetData( id );

    if (data == NULL)
        return FALSE;

    if (wcslen( data->configFileName ) > bufferCount - 1)
        return FALSE;

    wcscpy( buffer, data->configFileName );

    return TRUE;
}


FCIMPL1(void, COMSecurityConfig::EcallTurnCacheOff, StackCrawlMark* stackmark)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    AppDomain* pDomain;
#ifdef _DEBUG
	Assembly* callerAssembly =
#endif
	SystemDomain::GetCallersAssembly( (StackCrawlMark*)stackmark, &pDomain );

    _ASSERTE( callerAssembly != NULL);

    ApplicationSecurityDescriptor* pSecDesc = pDomain->GetSecurityDescriptor();

    _ASSERTE( pSecDesc != NULL );

    pSecDesc->DisableQuickCache();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL2(HRESULT, COMSecurityConfig::DebugOut, StringObject* fileUNSAFE, StringObject* messageUNSAFE)
{
    HRESULT retVal = E_FAIL;
    STRINGREF fileName = (STRINGREF) fileUNSAFE;
    STRINGREF message = (STRINGREF) messageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(fileName, message);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    
    HANDLE file = VMWszCreateFile( fileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if (file == INVALID_HANDLE_VALUE)
    {
        goto lExit;
    }
    
    SetFilePointer( file, 0, NULL, FILE_END );
    
    DWORD bytesWritten;
    
    if (!WriteFile( file, message->GetBuffer(), message->GetStringLength() * sizeof(WCHAR), &bytesWritten, NULL ))
    {
        CloseHandle( file );
        goto lExit;
    }
    
    if (message->GetStringLength() * sizeof(WCHAR) != bytesWritten)
    {
        CloseHandle( file );
        goto lExit;
    }
    
    CloseHandle( file );
    
    retVal = S_OK;

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
} 
FCIMPLEND

