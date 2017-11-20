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
/*============================================================
**
** Header:  AssemblyName.cpp
**
** Purpose: Implements AssemblyName (loader domain) architecture
**
** Date:  August 10, 1999
**
===========================================================*/

#include "common.h"

#include <stdlib.h>
#include <shlwapi.h>

#include "assemblyname.hpp"
#include "comstring.h"
#include "permset.h"
#include "field.h"
#include "fusion.h"
#include "strongname.h"
#include "eeconfig.h"

// This uses thread storage to allocate space. Please use Checkpoint and release it.
HRESULT AssemblyName::ConvertToAssemblyMetaData(StackingAllocator* alloc, ASSEMBLYNAMEREF* pName, AssemblyMetaDataInternal* pMetaData)
{
    THROWSCOMPLUSEXCEPTION();
    HRESULT hr = S_OK;;

    if (!pMetaData)
        return E_INVALIDARG;

    ZeroMemory(pMetaData, sizeof(AssemblyMetaDataInternal));

    VERSIONREF version = (VERSIONREF) (*pName)->GetVersion();
    if(version == NULL) {
        pMetaData->usMajorVersion = (USHORT) -1;
        pMetaData->usMinorVersion = (USHORT) -1;
        pMetaData->usBuildNumber = (USHORT) -1;         
        pMetaData->usRevisionNumber = (USHORT) -1;
    }
    else {
        pMetaData->usMajorVersion = version->GetMajor();
        pMetaData->usMinorVersion = version->GetMinor();
        pMetaData->usBuildNumber = version->GetBuild();
        pMetaData->usRevisionNumber = version->GetRevision();
    }

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__CULTURE_INFO__GET_NAME);
        
    struct _gc {
        OBJECTREF   cultureinfo;
        STRINGREF   pString;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    pMetaData->szLocale = 0;

    GCPROTECT_BEGIN(gc);
    gc.cultureinfo = (*pName)->GetCultureInfo();
    if (gc.cultureinfo != NULL) {
        ARG_SLOT args[] = {
            ObjToArgSlot(gc.cultureinfo)
        };
        ARG_SLOT ret = pMD->Call(args, METHOD__CULTURE_INFO__GET_NAME);
        gc.pString = (STRINGREF)(ArgSlotToObj(ret));
        if (gc.pString != NULL) {
            DWORD lgth = WszWideCharToMultiByte(CP_UTF8, 0, gc.pString->GetBuffer(), -1, NULL, 0, NULL, NULL);
            LPSTR lpLocale = (LPSTR) alloc->Alloc(lgth + 1);
            WszWideCharToMultiByte(CP_UTF8, 0, gc.pString->GetBuffer(), -1, 
                                   lpLocale, lgth+1, NULL, NULL);
            lpLocale[lgth] = '\0';
            pMetaData->szLocale = lpLocale;
        }
    }

    GCPROTECT_END();
    if(FAILED(hr)) return hr;

    return hr;
}

FCIMPL1(Object*, AssemblyNameNative::GetFileInformation, StringObject* filenameUNSAFE)
{
    ASSEMBLYNAMEREF name     = NULL;
    STRINGREF filename = (STRINGREF) filenameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, filename);

    WCHAR *FileChars;
    int fLength;

    THROWSCOMPLUSEXCEPTION();

    if (filename == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_FileName");

    //Get string data.
    RefInterpretGetStringValuesDangerousForGC(filename, &FileChars, &fLength);

    CQuickBytes qb;  
    DWORD dwFileLength = (DWORD) fLength;
    LPWSTR wszFileChars = (LPWSTR) qb.Alloc((++dwFileLength) * sizeof(WCHAR));
    memcpy(wszFileChars, FileChars, dwFileLength*sizeof(WCHAR));


    if (!fLength)
        COMPlusThrow(kArgumentException, L"Argument_EmptyFileName");

    PEFile *pFile = NULL;
    HRESULT hr = SystemDomain::LoadFile(wszFileChars, 
                                        NULL, 
                                        mdFileNil, 
                                        FALSE, 
                                        NULL, 
                                        NULL,  // Local assemblies only, therefore codebase is the same as the file
                                        NULL,  // Extra Evidence
                                        &pFile,
                                        FALSE);
    if (FAILED(hr)) {
        OBJECTREF Throwable = NULL;
        GCPROTECT_BEGIN(Throwable);

        MAKE_UTF8PTR_FROMWIDE(szName, wszFileChars);
        PostFileLoadException(szName, TRUE,NULL, hr, &Throwable);
        COMPlusThrow(Throwable);

        GCPROTECT_END();
    }

    _ASSERTE( pFile && "Unable to get PEFile" );

    if (! (pFile->GetMDImport())) {
        STRESS_ASSERT(0);
        delete pFile;
        COMPlusThrowHR(COR_E_BADIMAGEFORMAT);
    }

    GCPROTECT_BEGIN(name);
    {
        Assembly assembly;

        hr = assembly.AddManifestMetadata(pFile);
        if (SUCCEEDED(hr)) {
            assembly.m_FreeFlag |= Assembly::FREE_PEFILE;
            hr = assembly.GetManifestFile()->ReadHeaders();
        }

        if (FAILED(hr))
            COMPlusThrowHR(hr);

        assembly.m_FreeFlag |= Assembly::FREE_PEFILE;

        CQuickBytes qb2;
        DWORD dwCodeBase = 0;
        pFile->FindCodeBase(NULL, &dwCodeBase);
        LPWSTR wszCodeBase = (LPWSTR)qb2.Alloc((++dwCodeBase) * sizeof(WCHAR));

        hr = pFile->FindCodeBase(wszCodeBase, &dwCodeBase);
        if (SUCCEEDED(hr))
            assembly.m_pwCodeBase = wszCodeBase;
        else
            COMPlusThrowHR(hr);

        assembly.m_ExposedObject = SystemDomain::System()->GetCurrentDomain()->CreateHandle(NULL);

        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__ASSEMBLY__GET_NAME);

        ARG_SLOT args[1] = {
            ObjToArgSlot(assembly.GetExposedObject())
        };

        name = (ASSEMBLYNAMEREF) ArgSlotToObj(pMD->Call(args, METHOD__ASSEMBLY__GET_NAME));
        name->UnsetAssembly();

        // Don't let ~Assembly delete this.
        assembly.m_pwCodeBase = NULL;
        DestroyHandle(assembly.m_ExposedObject);

    } GCPROTECT_END();  // The ~Assembly() will toggle the GC mode that can cause a GC
    
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(name);
}
FCIMPLEND

FCIMPL1(Object*, AssemblyNameNative::ToString, Object* refThisUNSAFE);
{
    OBJECTREF pObj    = NULL;
    OBJECTREF refThis = (OBJECTREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    THROWSCOMPLUSEXCEPTION();
    if (refThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    HRESULT hr;
    ASSEMBLYNAMEREF pThis = (ASSEMBLYNAMEREF) refThis;
    GCPROTECT_BEGIN(pThis);

    DWORD dwFlags = pThis->GetFlags();

    AssemblyMetaDataInternal sContext;
    Thread *pThread = GetThread();

    void* checkPointMarker = pThread->m_MarshalAlloc.GetCheckpoint();

    hr = AssemblyName::ConvertToAssemblyMetaData(&(pThread->m_MarshalAlloc), 
                                                 (ASSEMBLYNAMEREF*) &pThis,
                                                 &sContext);
    if (SUCCEEDED(hr)) {

        WCHAR* pSimpleName = NULL;

        if (pThis->GetSimpleName() != NULL) {
            WCHAR* pString;
            int    iString;
            RefInterpretGetStringValuesDangerousForGC((STRINGREF) pThis->GetSimpleName(), &pString, &iString);
            pSimpleName = (WCHAR*) pThread->m_MarshalAlloc.Alloc((++iString) * sizeof(WCHAR));
            memcpy(pSimpleName, pString, iString*sizeof(WCHAR));
        }

        PBYTE  pbPublicKeyOrToken = NULL;
        DWORD  cbPublicKeyOrToken = 0;
        if (IsAfPublicKey(pThis->GetFlags()) && pThis->GetPublicKey() != NULL) {
            PBYTE  pArray = NULL;
            pArray = pThis->GetPublicKey()->GetDataPtr();
            cbPublicKeyOrToken = pThis->GetPublicKey()->GetNumComponents();
            pbPublicKeyOrToken = (PBYTE) pThread->m_MarshalAlloc.Alloc(cbPublicKeyOrToken);
            memcpy(pbPublicKeyOrToken, pArray, cbPublicKeyOrToken);
        }
        else if (IsAfPublicKeyToken(pThis->GetFlags()) && pThis->GetPublicKeyToken() != NULL) {
            PBYTE  pArray = NULL;
            pArray = pThis->GetPublicKeyToken()->GetDataPtr();
            cbPublicKeyOrToken = pThis->GetPublicKeyToken()->GetNumComponents();
            pbPublicKeyOrToken = (PBYTE) pThread->m_MarshalAlloc.Alloc(cbPublicKeyOrToken);
            memcpy(pbPublicKeyOrToken, pArray, cbPublicKeyOrToken);
            }

        IAssemblyName* pFusionAssemblyName = NULL;
        if (SUCCEEDED(hr = Assembly::SetFusionAssemblyName(pSimpleName,
                                                           dwFlags,
                                                           &sContext,
                                                           pbPublicKeyOrToken,
                                                           cbPublicKeyOrToken,
                                                           &pFusionAssemblyName))) {
            CQuickBytes qb;
            DWORD cb = 0;
            LPWSTR wszDisplayName = NULL;

            pFusionAssemblyName->GetDisplayName(NULL, &cb, 0);
            if(cb) {
                wszDisplayName = (LPWSTR) qb.Alloc(cb * sizeof(WCHAR));
                hr = pFusionAssemblyName->GetDisplayName(wszDisplayName, &cb, 0);
            }
            pFusionAssemblyName->Release();

            if (SUCCEEDED(hr) && cb) {
                    pObj = (OBJECTREF) COMString::NewString(wszDisplayName);
                }
            }
    }

    pThread->m_MarshalAlloc.Collapse(checkPointMarker);

    GCPROTECT_END();

    if (FAILED(hr))
        COMPlusThrowHR(hr);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pObj);
}
FCIMPLEND

FCIMPL1(Object*, AssemblyNameNative::GetPublicKeyToken, Object* refThisUNSAFE)
{
    OBJECTREF orOutputArray = NULL;
    OBJECTREF refThis       = (OBJECTREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    THROWSCOMPLUSEXCEPTION();
    if (refThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ASSEMBLYNAMEREF orThis = (ASSEMBLYNAMEREF)refThis;
    U1ARRAYREF orPublicKey = orThis->GetPublicKey();

    if ((orPublicKey != NULL) && (orPublicKey->GetNumComponents() > 0)) {
        DWORD   cbKey = orPublicKey->GetNumComponents();
        CQuickBytes qb;
        BYTE   *pbKey = (BYTE*) qb.Alloc(cbKey);
        DWORD   cbToken;
        BYTE   *pbToken;

        memcpy(pbKey, orPublicKey->GetDataPtr(), cbKey);

        Thread *pThread = GetThread();
        pThread->EnablePreemptiveGC();
        BOOL fResult = StrongNameTokenFromPublicKey(pbKey, cbKey, &pbToken, &cbToken);
        pThread->DisablePreemptiveGC();

        if (!fResult)
            COMPlusThrowHR(StrongNameErrorInfo());

        SecurityHelper::CopyEncodingToByteArray(pbToken, cbToken, &orOutputArray);

        StrongNameFreeBuffer(pbToken);
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(orOutputArray);
}
FCIMPLEND


FCIMPL1(Object*, AssemblyNameNative::EscapeCodeBase, StringObject* filenameUNSAFE)
{
    STRINGREF rv        = NULL;
    STRINGREF filename  = (STRINGREF) filenameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, filename);

    THROWSCOMPLUSEXCEPTION();

    LPWSTR pCodeBase = NULL;
    DWORD  dwCodeBase = 0;
    CQuickBytes qb;

    if (filename != NULL) {
        WCHAR* pString;
        int    iString;
        RefInterpretGetStringValuesDangerousForGC(filename, &pString, &iString);
        dwCodeBase = (DWORD) iString;
        pCodeBase = (LPWSTR) qb.Alloc((++dwCodeBase) * sizeof(WCHAR));
        memcpy(pCodeBase, pString, dwCodeBase*sizeof(WCHAR));
    }

    if(pCodeBase) {
        CQuickBytes qb2;
        DWORD dwEscaped = 1;
        UrlEscape(pCodeBase, (LPWSTR) qb2.Ptr(), &dwEscaped, 0);

        LPWSTR result = (LPWSTR)qb2.Alloc((++dwEscaped) * sizeof(WCHAR));
        HRESULT hr = UrlEscape(pCodeBase, result, &dwEscaped, 0);

        if (SUCCEEDED(hr))
            rv = COMString::NewString(result);
        else
            COMPlusThrowHR(hr);
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(rv);
}
FCIMPLEND
