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
#ifndef _APPDOMAIN_HELPER_H_
#define _APPDOMAIN_HELPER_H_

// Marshal a single object into a serialized blob.
//
//

#include "corperme.h"

class AppDomainHelper {

    friend class MarshalCache;

    // A pair of helper to move serialization info between managed byte-array and
    // unmanaged blob.
    static void AppDomainHelper::CopyEncodingToByteArray(IN PBYTE   pbData, 
                                                         IN DWORD   cbData, 
                                                         OUT OBJECTREF* pArray);

    static void AppDomainHelper::CopyByteArrayToEncoding(IN U1ARRAYREF* pArray,
                                                         OUT PBYTE*   ppbData,
                                                         OUT DWORD*   pcbData);

public:
    // Marshal a single object into a serialized blob.
    static void AppDomainHelper::MarshalObject(IN AppDomain *pDomain,
                                               IN OBJECTREF  *orObject,
                                               OUT U1ARRAYREF *porBlob);

    // Marshal one object into a seraialized blob.
    static void AppDomainHelper::MarshalObject(IN AppDomain *pDomain,
                                               IN OBJECTREF  *orObject,
                                               OUT BYTE    **ppbBlob,
                                               OUT DWORD    *pcbBlob);

    // Marshal two objects into serialized blobs.
    static void AppDomainHelper::MarshalObjects(IN AppDomain *pDomain,
                                                IN OBJECTREF  *orObject1,
                                                IN OBJECTREF  *orObject2,
                                                OUT BYTE    **ppbBlob1,
                                                OUT DWORD    *pcbBlob1,
                                                OUT BYTE    **ppbBlob2,
                                                OUT DWORD    *pcbBlob2);

    // Unmarshal a single object from a serialized blob.
    static void AppDomainHelper::UnmarshalObject(IN AppDomain   *pDomain,
                                                 IN U1ARRAYREF  *porBlob,
                                                 OUT OBJECTREF  *porObject);

    // Unmarshal a single object from a serialized blob.
    static void AppDomainHelper::UnmarshalObject(IN AppDomain   *pDomain,
                                                 IN BYTE        *pbBlob,
                                                 IN DWORD        cbBlob,
                                                 OUT OBJECTREF  *porObject);

    // Unmarshal two objects from serialized blobs.
    static void AppDomainHelper::UnmarshalObjects(IN AppDomain   *pDomain,
                                                  IN BYTE        *pbBlob1,
                                                  IN DWORD        cbBlob1,
                                                  IN BYTE        *pbBlob2,
                                                  IN DWORD        cbBlob2,
                                                  OUT OBJECTREF  *porObject1,
                                                  OUT OBJECTREF  *porObject2);

    // Copy an object from the given appdomain into the current appdomain.
    static OBJECTREF AppDomainHelper::CrossContextCopyFrom(IN AppDomain *pAppDomain,
                                                           IN OBJECTREF *orObject);
    // Copy an object to the given appdomain from the current appdomain.
    static OBJECTREF AppDomainHelper::CrossContextCopyTo(IN AppDomain *pAppDomain,
                                                         IN OBJECTREF  *orObject);
    // Copy an object from the given appdomain into the current appdomain.
    static OBJECTREF AppDomainHelper::CrossContextCopyFrom(IN DWORD dwDomainId,
                                                           IN OBJECTREF *orObject);
    // Copy an object to the given appdomain from the current appdomain.
    static OBJECTREF AppDomainHelper::CrossContextCopyTo(IN DWORD dwDomainId,
                                                         IN OBJECTREF  *orObject);

};

// Cache the bits needed to serialize/deserialize managed objects that will be
// passed across appdomain boundaries during a stackwalk. The serialization is
// performed lazily the first time it's needed and remains valid throughout the
// stackwalk. The last deserialized object is cached and tagged with its
// appdomain context. It's valid as long as we're walking frames within the same
// appdomain.
//
class MarshalCache
{
public:
    MarshalCache()
    {
        ZeroMemory(this, sizeof(*this));
    }

    ~MarshalCache()
    {
        if (m_pbObj1)
            FreeM(m_pbObj1);
        if (m_pbObj2)
            FreeM(m_pbObj2);
    }

    // Set the original value of the first cached object.
    void SetObject(OBJECTREF orObject)
    {
        m_pOriginalDomain = GetAppDomain();
        m_sGC.m_orInput1 = orObject;
    }

    // Set the original values of both cached objects.
    void SetObjects(OBJECTREF orObject1, OBJECTREF orObject2)
    {
        m_pOriginalDomain = GetAppDomain();
        m_sGC.m_orInput1 = orObject1;
        m_sGC.m_orInput2 = orObject2;
    }

    // Get a copy of the first object suitable for use in the given appdomain.
    OBJECTREF GetObject(AppDomain *pDomain)
    {
        THROWSCOMPLUSEXCEPTION();

        // No transition -- just return original object.
        if (pDomain == m_pOriginalDomain) {
            if (m_fObjectUpdated)
                UpdateObjectFinish();
            return m_sGC.m_orInput1;
        }

        // We've already deserialized the object into the correct context.
        if (pDomain == m_pCachedDomain)
            return m_sGC.m_orOutput1;

        // If we've updated the object in a different appdomain from the one we
        // originally started in, the cached object will be more up to date than
        // the original. Resync the objects.
        if (m_fObjectUpdated)
            UpdateObjectFinish();

        // Check whether we've serialized the original input object yet.
        if (m_pbObj1 == NULL)
            AppDomainHelper::MarshalObject(m_pOriginalDomain,
                                          &m_sGC.m_orInput1,
                                          &m_pbObj1,
                                          &m_cbObj1);

        // Deserialize into the correct context.
        AppDomainHelper::UnmarshalObject(pDomain,
                                        m_pbObj1,
                                        m_cbObj1,
                                        &m_sGC.m_orOutput1);
        m_pCachedDomain = pDomain;

        return m_sGC.m_orOutput1;
    }

    // As above, but retrieve both objects.
    OBJECTREF GetObjects(AppDomain *pDomain, OBJECTREF *porObject2)
    {
        THROWSCOMPLUSEXCEPTION();

        // No transition -- just return original objects.
        if (pDomain == m_pOriginalDomain) {
            if (m_fObjectUpdated)
                UpdateObjectFinish();
            *porObject2 = m_sGC.m_orInput2;
            return m_sGC.m_orInput1;
        }

        // We've already deserialized the objects into the correct context.
        if (pDomain == m_pCachedDomain) {
            *porObject2 = m_sGC.m_orOutput2;
            return m_sGC.m_orOutput1;
        }

        // If we've updated the object in a different appdomain from the one we
        // originally started in, the cached object will be more up to date than
        // the original. Resync the objects.
        if (m_fObjectUpdated)
            UpdateObjectFinish();

        // Check whether we've serialized the original input objects yet.
        if (m_pbObj1 == NULL)
            AppDomainHelper::MarshalObjects(m_pOriginalDomain,
                                           &m_sGC.m_orInput1,
                                           &m_sGC.m_orInput2,
                                           &m_pbObj1,
                                           &m_cbObj1,
                                           &m_pbObj2,
                                           &m_cbObj2);

        // Deserialize into the correct context.
        AppDomainHelper::UnmarshalObjects(pDomain,
                                          m_pbObj1,
                                          m_cbObj1,
                                          m_pbObj2,
                                          m_cbObj2,
                                          &m_sGC.m_orOutput1,
                                          &m_sGC.m_orOutput2);
        m_pCachedDomain = pDomain;

        *porObject2 = m_sGC.m_orOutput2;
        return m_sGC.m_orOutput1;
    }

    // Change the first object (updating the cacheing information
    // appropriately).
    void UpdateObject(AppDomain *pDomain, OBJECTREF orObject)
    {
        // The cached serialized blob is now useless.
        if (m_pbObj1)
            FreeM(m_pbObj1);
        m_pbObj1 = NULL;
        m_cbObj1 = 0;

        // The object we have now is valid in it's own appdomain, so place that
        // in the object cache.
        m_pCachedDomain = pDomain;
        m_sGC.m_orOutput1 = orObject;

        // If the object is updated in the original context, just use the new
        // value as is. In this case we have the data to re-marshal the updated
        // object as normal, so we can consider the cache fully updated and exit
        // now.
        if (pDomain == m_pOriginalDomain) {
            m_sGC.m_orInput1 = orObject;
            m_fObjectUpdated = false;
            return;
        }

        // We want to avoid re-marshaling the updated value as long as possible
        // (it might be updated again before we need its value in a different
        // context). So set a flag to indicate that the object must be
        // re-marshaled when the value is queried in a new context.
        m_fObjectUpdated = true;
    }

    // This structure is public only so that it can be GC protected. Do not
    // access the fields directly, they change in an unpredictable fashion due
    // to the lazy cacheing algorithm.
    struct _gc {
        OBJECTREF   m_orInput1;
        OBJECTREF   m_orInput2;
        OBJECTREF   m_orOutput1;
        OBJECTREF   m_orOutput2;
    }           m_sGC;

private:

    // Called after one or more calls to UpdateObject to marshal the updated
    // object back into its original context (it's assumed we're called in this
    // context).
    void UpdateObjectFinish()
    {
        _ASSERTE(m_fObjectUpdated && m_pbObj1 == NULL);
        AppDomainHelper::MarshalObject(m_pCachedDomain,
                                      &m_sGC.m_orOutput1,
                                      &m_pbObj1,
                                      &m_cbObj1);
        AppDomainHelper::UnmarshalObject(m_pOriginalDomain,
                                        m_pbObj1,
                                        m_cbObj1,
                                        &m_sGC.m_orInput1);
        m_fObjectUpdated = false;
    }

    BYTE       *m_pbObj1;
    DWORD       m_cbObj1;
    BYTE       *m_pbObj2;
    DWORD       m_cbObj2;
    AppDomain  *m_pCachedDomain;
    AppDomain  *m_pOriginalDomain;
    bool        m_fObjectUpdated;
};

#endif
