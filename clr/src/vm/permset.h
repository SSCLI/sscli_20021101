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
// Used for loading permissions into the runtime
//*****************************************************************************

#ifndef _PERMSET_H
#define _PERMSET_H

#include "vars.hpp"
#include "corpermp.h"
#include "eehash.h"

enum SpecialPermissionSetFlag
{
    // These also appear in clr/src/bcl/system/security/util/config.cs
    Regular = 0,
    NoSet = 1,
    EmptySet = 2,
    SkipVerification = 3
};

struct PermissionRequestSpecialFlags
{
    PermissionRequestSpecialFlags()
        : required( NoSet ),
          optional( NoSet ),
          refused( NoSet )
    {
    }

    SpecialPermissionSetFlag required;
    SpecialPermissionSetFlag optional;
    SpecialPermissionSetFlag refused;
};


// All out parameters use MallocM/FreeM macros defined in the CorPermE.h. 
// There are no serparately allocated internal pointers. All memory is 
// released with outter most pointer is freed.

// Records a serialized permission set we've seen and decoded. This entry
// exists both in a global dynamic array (which gives it an index used to
// identify the pset across appdomains) and chained into a hash (which
// allows us look for existing entries quickly when decoding new psets).
struct PsetCacheEntry
{
    EEHashEntry m_sHashEntry;
    PBYTE       m_pbPset;
    DWORD       m_cbPset;
    DWORD       m_dwIndex;
    SpecialPermissionSetFlag m_SpecialFlags;

    PsetCacheEntry(PBYTE pbPset, DWORD cbPset) :
        m_pbPset(pbPset),
        m_cbPset(cbPset)
    {}

    BOOL IsEquiv(PsetCacheEntry *pOther)
    {
        if (m_cbPset != pOther->m_cbPset)
            return FALSE;
        return memcmp(m_pbPset, pOther->m_pbPset, m_cbPset) == 0;
    }

    DWORD Hash()
    {
        DWORD dwHash = 0;
        for (DWORD i = 0; i < (m_cbPset / sizeof(DWORD)); i++)
            dwHash ^= GET_UNALIGNED_VAL32(&((DWORD*)m_pbPset)[i]);
        return dwHash;
    }
};

class SecurityHelper {
public:

    static VOID Init();
    static VOID Shutdown();

    static HRESULT MapToHR(OBJECTREF ref);

    // Loads up the permission, will throw COMPLUS exceptions
    static void LoadPermissionSet(IN PBYTE              pbRawPermissions,
                                  IN DWORD              cbRawPermissions,
                                  OUT OBJECTREF        *pRef,
                                  OUT BOOL             *pFullyTrusted,
                                  OUT DWORD            *pdwSetIndex = NULL,
                                  IN BOOL               fNoCache = FALSE,
                                  OUT SpecialPermissionSetFlag *pSpecialFlags = NULL,
                                  IN BOOL               fCreate = TRUE);

    // Retrieves a previously loaded permission set by index (this will work
    // even if the permission set was loaded in a different appdomain).
    static OBJECTREF GetPermissionSet(DWORD dwIndex, SpecialPermissionSetFlag *specialFlags = NULL);

    // Locate the index of a permission set in the cache (returns false if the
    // permission set has not yet been seen and decoded).
    static BOOL LookupPermissionSet(IN PBYTE       pbPset,
                                    IN DWORD       cbPset,
                                    OUT DWORD     *pdwSetIndex);

    // Creates a new permission vector.
    static OBJECTREF CreatePermissionSet(BOOL fTrusted);

    // Uses MallocM to create the byte array that is returned.
    static void CopyByteArrayToEncoding(IN U1ARRAYREF* pArray,
                                        OUT PBYTE* pbData,
                                        OUT DWORD* cbData);

    static void CopyStringToWCHAR(IN STRINGREF* pString,
                                  OUT WCHAR** ppwString,
                                  OUT DWORD*  pdwString);
    
    static void EncodePermissionSet(IN OBJECTREF* pRef,
                                    OUT PBYTE* ppbData,
                                    OUT DWORD* pcbData);
    
    // Generic routine, use with encoding calls that 
    // use the EncodePermission client data
    // Uses MallocM to create the byte array that is returned.
    static void CopyEncodingToByteArray(IN PBYTE   pbData,
                                        IN DWORD   cbData,
                                        IN OBJECTREF* pArray);

    static void LoadPermissionRequestsFromAssembly(IN Assembly*     pAssembly,
                                                   OUT OBJECTREF*   pReqdPermissions,
                                                   OUT OBJECTREF*   pOptPermissions,
                                                   OUT OBJECTREF*   pDenyPermissions,
                                                   OUT PermissionRequestSpecialFlags* pSpecialFlags = NULL,
                                                   BOOL             fCreate = TRUE);

    static BOOL PermissionsRequestedInAssembly(IN  Assembly* pAssembly);

    // Returns the declared permissions for the specified action type.
    static HRESULT GetDeclaredPermissions(IN IMDInternalImport *pInternalImport,
                                          IN mdToken classToken,
                                          IN CorDeclSecurity action,
                                          OUT OBJECTREF *pDeclaredPermissions,
                                          OUT SpecialPermissionSetFlag* pSpecialFlags = NULL,
                                          BOOL fCreate = TRUE);


private:
    // Insert a decoded permission set into the cache. Duplicates are discarded.
    static BOOL InsertPermissionSet(IN PBYTE       pbPset,
                                    IN DWORD       cbPset,
                                    IN OBJECTREF   orPset,
                                    OUT SpecialPermissionSetFlag *pdwSpecialFlags,
                                    OUT DWORD     *pdwSetIndex);

    friend class EEPsetHashTableHelper;

    static CQuickArrayStatic<PsetCacheEntry> s_rCachedPsets;
    static EEPsetHashTable s_sCachedPsetsHash;
    static CRITICAL_SECTION s_sCachedPsetsLock;
};

#endif
