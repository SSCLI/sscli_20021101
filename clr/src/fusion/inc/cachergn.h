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
#include "asmstrm.h"
#include "transprt.h"
#include <windows.h>
#include "dbglog.h"

#define TRANSPORT_CACHE_FLAGS_REGENERATION                  0x100
#define TRANSPORT_CACHE_REGENERATION_IDX_OFFSET             0x100
class CAssemblyCacheRegenerator
{
    protected:
        DWORD               _dwSig;
        CDebugLog           *_pdbglog;

        // for cross process locking
        HANDLE              _hRegeneratorMutex;
        
        // storage for lock that CDatabase::Lock returns
        HLOCK               _hlTransCacheLock[TRANSPORT_CACHE_IDX_TOTAL];
        HLOCK               _hlNameResLock;
        HLOCK               _hlNewGlobalCacheLock;
        
        // interface to temporary cache index files
        static IDatabase    *g_pDBNewCache[TRANSPORT_CACHE_IDX_TOTAL];
        static IDatabase    *g_pDBNewNameRes;

        // reentrancy protection flags
        BOOL                _fThisInstanceIsRegenerating;
        static DWORD         g_dwRegeneratorRunningInThisProcess;
        
        // which database are we regenerating
        DWORD               _dwCacheId;

        // we are regenerating NameRes (TRUE) TransCache (FALSE)
        BOOL                _fIsNameRes;
        
    public:        
        
        CAssemblyCacheRegenerator(CDebugLog *pdbglog, DWORD dwCacheId, BOOL fIsNameRes);
        ~CAssemblyCacheRegenerator();

        HRESULT Init();
        HRESULT Regenerate();
        static HRESULT SetSchemaVersion(DWORD dwNewMinorVersion, DWORD dwCacheId, BOOL fIsNameRes);
        
    private:
        static HRESULT CreateRegenerationTransCache(DWORD dwCacheId, CTransCache **CTransCache);                 
        HRESULT ProcessStoreDir();
        HRESULT RegenerateGlobalCache();
        HRESULT CreateEmptyCache();
        HRESULT ProcessSubDir(LPTSTR szCurrentDir, LPTSTR szSubDir);
        HRESULT LockFusionCache();
        HRESULT UnlockFusionCache();
        HRESULT CloseCacheRegeneratedDatabase();
        HRESULT DeleteFilesInDirectory(LPTSTR szDirectory);


    // CCache::InsertTransCacheEntry needs access to CreateRegenerationTransCache
    friend class CCache;
};

HRESULT RegenerateCache(CDebugLog *pdbg, DWORD dwCacheId, BOOL fIsNameRes);
