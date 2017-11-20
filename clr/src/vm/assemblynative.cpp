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
** Header:  AssemblyNative.cpp
**
** Purpose: Implements AssemblyNative (loader domain) architecture
**
** Date:  Dec 1, 1998
**
===========================================================*/

#include "common.h"

#include <shlwapi.h>
#include <stdlib.h>
#include "assemblynative.hpp"
#include "field.h"
#include "comstring.h"
#include "assemblyname.hpp"
#include "eeconfig.h"
#include "security.h"
#include "permset.h"
#include "comreflectioncommon.h"
#include "comclass.h"
#include "commember.h"
#include "strongname.h"
#include "interoputil.h"
#include "frames.h"


//
// NOTE: this macro must be used prior to entering a helper method frame
//

#define GET_ASSEMBLY(pThis) \
    (((ASSEMBLYREF)ObjectToOBJECTREF(pThis))->GetAssembly())

FCIMPL7(Object*, AssemblyNative::Load, AssemblyNameBaseObject* assemblyNameUNSAFE, 
                                       StringObject* codeBaseUNSAFE, 
                                       BYTE fIsStringized, 
                                       Object* securityUNSAFE, 
                                       BYTE fThrowOnFileNotFound, 
                                       AssemblyBaseObject* locationHintUNSAFE,
                                       StackCrawlMark* stackMark)
{
    CHECKGC();
    THROWSCOMPLUSEXCEPTION();

    ASSEMBLYREF     rv              = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);

    struct _gc
    {
        ASSEMBLYNAMEREF assemblyName;
        STRINGREF       codeBase;
        ASSEMBLYREF     locationHint;
        OBJECTREF       security;
    } gc;

    gc.assemblyName    = (ASSEMBLYNAMEREF) assemblyNameUNSAFE;
    gc.codeBase        = (STRINGREF)       codeBaseUNSAFE;
    gc.locationHint    = (ASSEMBLYREF)     locationHintUNSAFE;
    gc.security        = (OBJECTREF)       securityUNSAFE;

    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();

    if (gc.assemblyName == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_AssemblyName");
    
    if( (gc.assemblyName->GetSimpleName() == NULL) &&
        (gc.codeBase == NULL ) )
        COMPlusThrow(kArgumentException, L"Format_StringZeroLength");

    WCHAR* pCodeBase = NULL;
    DWORD  dwCodeBase = 0;
    Assembly *pAssembly = NULL;
    PBYTE  pStrongName = NULL;
    DWORD  dwStrongName = 0;
    IAssembly *pRefIAssembly = NULL;
    DWORD dwFlags = 0;

    AssemblyMetaDataInternal sContext;
    LPSTR psSimpleName = NULL;
    HRESULT hr = S_OK;
    CQuickBytes qb;
    CQuickBytes qb2;
    CQuickBytes qb3;
    CQuickBytes qb4;

    Thread *pThread = GetThread();
    void* checkPointMarker = pThread->m_MarshalAlloc.GetCheckpoint();

    if (!fIsStringized) {
        dwFlags = gc.assemblyName->GetFlags();

        hr = AssemblyName::ConvertToAssemblyMetaData(&(pThread->m_MarshalAlloc), 
                                                     &gc.assemblyName,
                                                     &sContext);
        if (FAILED(hr)) {
            pThread->m_MarshalAlloc.Collapse(checkPointMarker);
            COMPlusThrowHR(hr);
        }

        PBYTE  pArray = NULL;
        if (gc.assemblyName->GetPublicKeyToken() != NULL) {
            dwFlags = !afPublicKey;
            pArray = gc.assemblyName->GetPublicKeyToken()->GetDataPtr();
            dwStrongName = gc.assemblyName->GetPublicKeyToken()->GetNumComponents();
            pStrongName = (PBYTE) pThread->m_MarshalAlloc.Alloc(dwStrongName);
            memcpy(pStrongName, pArray, dwStrongName);
        }
        else if (gc.assemblyName->GetPublicKey() != NULL) {
            pArray = gc.assemblyName->GetPublicKey()->GetDataPtr();
            dwStrongName = gc.assemblyName->GetPublicKey()->GetNumComponents();
            pStrongName = (PBYTE) pThread->m_MarshalAlloc.Alloc(dwStrongName);
            memcpy(pStrongName, pArray, dwStrongName);
        }
    }

    if (gc.codeBase != NULL) {
        WCHAR* pString;
        int    iString;
        RefInterpretGetStringValuesDangerousForGC(gc.codeBase, &pString, &iString);
        dwCodeBase = (DWORD) iString;
        pCodeBase = (LPWSTR) qb2.Alloc((++dwCodeBase) * sizeof(WCHAR));
        memcpy(pCodeBase, pString, dwCodeBase*sizeof(WCHAR));
    }

    AssemblySpec spec;
    OBJECTREF Throwable = NULL;  
    GCPROTECT_BEGIN(Throwable);

    if (gc.assemblyName->GetSimpleName() != NULL) {
        STRINGREF sRef = (STRINGREF) gc.assemblyName->GetSimpleName();
        DWORD strLen;
        if((strLen = sRef->GetStringLength()) == 0)
            COMPlusThrow(kArgumentException, L"Format_StringZeroLength");

        psSimpleName = (LPUTF8) qb3.Alloc(++strLen);

        if (!COMString::TryConvertStringDataToUTF8(sRef, psSimpleName, strLen))
            psSimpleName = GetClassStringVars(sRef, &qb, &strLen);

        // Only need to get referencing IAssembly for Load(), not LoadFrom()
        Assembly *pRefAssembly;
        if (gc.locationHint == NULL)
            pRefAssembly = SystemDomain::GetCallersAssembly(stackMark);
        else
            pRefAssembly = gc.locationHint->GetAssembly();

        // Shared assemblies should not be used for the parent in the
        // late-bound case.
        if (pRefAssembly && (!pRefAssembly->IsShared()))
            pRefIAssembly = pRefAssembly->GetFusionAssembly();
        
        if (fIsStringized)
            hr = spec.Init(psSimpleName); 
        else
            hr = spec.Init(psSimpleName, &sContext, 
                           pStrongName, dwStrongName, 
                           dwFlags);

        spec.GetCodeBase()->SetParentAssembly(pRefIAssembly);
    }
    else
        spec.SetCodeBase(pCodeBase, dwCodeBase);


    if (SUCCEEDED(hr)) {
        hr = spec.LoadAssembly(&pAssembly, &Throwable,
                               &gc.security);

        // If the user specified both a simple name and a codebase, and the module
        // wasn't found by simple name, try again, this time also using the codebase
        if ((!Assembly::ModuleFound(hr)) && psSimpleName && pCodeBase) {
            AssemblySpec spec2;
            spec2.SetCodeBase(pCodeBase, dwCodeBase);
            Throwable = NULL;
            hr = spec2.LoadAssembly(&pAssembly, &Throwable,
                                   &gc.security);

            if (SUCCEEDED(hr) && pAssembly->GetFusionAssemblyName()) {
                IAssemblyName* pReqName = NULL;
                MAKE_WIDEPTR_FROMUTF8(wszAssembly, psSimpleName);

                if (fIsStringized)
                    hr = CreateAssemblyNameObject(&pReqName, wszAssembly, CANOF_PARSE_DISPLAY_NAME, NULL);
                else
                    hr = Assembly::SetFusionAssemblyName(wszAssembly,
                                                         dwFlags,
                                                         &sContext,
                                                         pStrongName,
                                                         dwStrongName,
                                                         &pReqName);

                if (SUCCEEDED(hr)) {
                    hr = pAssembly->GetFusionAssemblyName()->IsEqual(pReqName, ASM_CMPF_DEFAULT);
                    if(hr == S_FALSE) {
                        hr = FUSION_E_REF_DEF_MISMATCH;
                        PostFileLoadException(psSimpleName, FALSE, NULL, hr, &Throwable);
                    }
                }
                if (pReqName)
                    pReqName->Release();
            }
        }
    }

    // Throw special exception for display name if file not found, for clarity
    if ((Throwable != NULL) &&
        ( (hr != COR_E_FILENOTFOUND) || ( fThrowOnFileNotFound) )) {
        pThread->m_MarshalAlloc.Collapse(checkPointMarker);
        COMPlusThrow(Throwable);
    }
    GCPROTECT_END();

    if (SUCCEEDED(hr))
    {
        rv = (ASSEMBLYREF) pAssembly->GetExposedObject();
    }
    else if (fThrowOnFileNotFound) 
    {
        pThread->m_MarshalAlloc.Collapse(checkPointMarker);

        if ((hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) && psSimpleName) {
                MAKE_WIDEPTR_FROMUTF8(pSimpleName, psSimpleName);
                COMPlusThrow(kFileNotFoundException, IDS_EE_DISPLAYNAME_NOT_FOUND,
                             pSimpleName);
        }
        COMPlusThrowHR(hr);
    }
    
    pThread->m_MarshalAlloc.Collapse(checkPointMarker);

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND


FCIMPL4(Object*, AssemblyNative::LoadImage, U1Array* PEByteArrayUNSAFE, U1Array* SymByteArrayUNSAFE, Object* securityUNSAFE, StackCrawlMark* stackMark)
{
    HRESULT hr = S_OK;
    OBJECTREF   refRetVal   = NULL;
    THROWSCOMPLUSEXCEPTION();

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    struct _gc
    {
        U1ARRAYREF PEByteArray;
        U1ARRAYREF SymByteArray;
        OBJECTREF  security;
    } gc;

    gc.PEByteArray  = (U1ARRAYREF) PEByteArrayUNSAFE;
    gc.SymByteArray = (U1ARRAYREF) SymByteArrayUNSAFE;
    gc.security     = (OBJECTREF)  securityUNSAFE;

    GCPROTECT_BEGIN(gc);

    if (gc.PEByteArray == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Array");

    AppDomain* pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);

    PBYTE pbImage;
    DWORD cbImage;
    SecurityHelper::CopyByteArrayToEncoding(&gc.PEByteArray, &pbImage, &cbImage);

    // Get caller's assembly so we can extract their codebase and propagate it
    // into the new assembly (which obviously doesn't have one of its own).
    LPCWSTR pCallersFileName = NULL;
    Assembly *pCallersAssembly = SystemDomain::GetCallersAssembly(stackMark);
    if (pCallersAssembly) { // can be null if caller is interop
        PEFile *pCallersFile = pCallersAssembly->GetSecurityModule()->GetPEFile();
        pCallersFileName = pCallersFile->GetFileName();

        // The caller itself may have been loaded via byte array.
        if (pCallersFileName[0] == L'\0')
            pCallersFileName = pCallersFile->GetLoadersFileName();
    }

    // Check for the presence and validity of a strong name.
    BEGIN_ENSURE_PREEMPTIVE_GC();
    if (!StrongNameSignatureVerificationFromImage(pbImage, cbImage, SN_INFLAG_INSTALL|SN_INFLAG_ALL_ACCESS, NULL))
        hr = StrongNameErrorInfo();
    END_ENSURE_PREEMPTIVE_GC();
    if (FAILED(hr) && hr != CORSEC_E_MISSING_STRONGNAME)
        COMPlusThrowHR(hr);

    PEFile *pFile;
    hr = PEFile::Create(pbImage, cbImage, 
                        NULL, 
                        pCallersFileName, 
                        &gc.security, 
                        &pFile, 
                        FALSE);
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    Module* pModule = NULL;
    Assembly *pAssembly;

    hr = pDomain->LoadAssembly(pFile, 
                               NULL, 
                               &pModule, 
                               &pAssembly, 
                               &gc.security, 
                               FALSE,
                               NULL);
    if(FAILED(hr)) {
        FreeM(pbImage);
        if (hr == E_OUTOFMEMORY)
            COMPlusThrowOM();
        else
            COMPlusThrowHR(HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));
    }

    LOG((LF_CLASSLOADER, 
         LL_INFO100, 
         "\tLoaded in-memory module\n"));

    if (pAssembly)
    {
#ifdef DEBUGGING_SUPPORTED
        // If we were given symbols and we need to track JIT info for
        // the debugger, load them now.
        PBYTE pbSyms = NULL;
        DWORD cbSyms = 0;

        if ((gc.SymByteArray != NULL) &&
            CORDebuggerTrackJITInfo(pModule->GetDebuggerInfoBits()))
        {
            SecurityHelper::CopyByteArrayToEncoding(&gc.SymByteArray,
                                                    &pbSyms, &cbSyms);

            hr = pModule->SetSymbolBytes(pbSyms, cbSyms);

            if (FAILED(hr))
                COMPlusThrowHR(HRESULT_FROM_WIN32(hr));

            FreeM(pbSyms);
        }
#endif // DEBUGGING_SUPPORTED
        refRetVal = pAssembly->GetExposedObject();
    }

    FreeM(pbImage);

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


FCIMPL5(Object*, AssemblyNative::LoadModuleImage, Object* refThisUNSAFE, StringObject* moduleNameUNSAFE, U1Array* PEByteArrayUNSAFE, U1Array* SymByteArrayUNSAFE, Object* securityUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF   rv          = NULL;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly = GET_ASSEMBLY(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    struct _gc
    {
        STRINGREF   moduleName;
        U1ARRAYREF  PEByteArray;
        U1ARRAYREF  SymByteArray;
	OBJECTREF   security;
    } gc;

    gc.moduleName   = (STRINGREF)   moduleNameUNSAFE;
    gc.PEByteArray  = (U1ARRAYREF)  PEByteArrayUNSAFE;
    gc.SymByteArray = (U1ARRAYREF)  SymByteArrayUNSAFE;
    gc.security = (OBJECTREF)   securityUNSAFE;

    GCPROTECT_BEGIN(gc);

    if (pAssembly->IsShared())
        COMPlusThrow(kNotSupportedException, L"NotSupported_SharedAssembly");

    if (gc.moduleName == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_FileName");
    
    if (gc.PEByteArray == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Array");

    LPSTR psModuleName = NULL;
    WCHAR* pModuleName = NULL;
    WCHAR* pString;
    int    iString;
    CQuickBytes qb;

    RefInterpretGetStringValuesDangerousForGC(gc.moduleName, &pString, &iString);
    if(iString == 0)
        COMPlusThrow(kArgumentException, L"Argument_EmptyFileName");
    
    pModuleName = (LPWSTR) qb.Alloc((++iString) * sizeof(WCHAR));
    memcpy(pModuleName, pString, iString*sizeof(WCHAR));
    
    MAKE_UTF8PTR_FROMWIDE(pName, pModuleName);
    psModuleName = pName;

    HashDatum datum;
    mdFile kFile = NULL;
    if (pAssembly->m_pAllowedFiles->GetValue(psModuleName, &datum))
        kFile = (mdFile)(size_t)datum;

    // If the name doesn't match one of the File def names, don't load this module.
    // If this name matches the manifest file (datum was NULL), don't load either.
    if (!kFile)
        COMPlusThrow(kArgumentException, L"Arg_InvalidFileName");

    PBYTE pbImage;
    DWORD cbImage;
    SecurityHelper::CopyByteArrayToEncoding(&gc.PEByteArray, &pbImage, &cbImage);

    HRESULT hr;
    const BYTE* pbHash;
    DWORD cbHash;
    DWORD dwFlags;
    pAssembly->GetManifestImport()->GetFileProps(kFile,
                                                 NULL, //name
                                                 (const void**) &pbHash,
                                                 &cbHash,
                                                 &dwFlags);

    if ( pAssembly->m_cbPublicKey ||
         pAssembly->m_pManifest->GetSecurityDescriptor()->IsSigned() ) {

        if (!pbHash)
            hr = COR_E_MODULE_HASH_CHECK_FAILED;
        else
            hr = Assembly::VerifyHash(pbImage,
                                      cbImage,
                                      pAssembly->m_ulHashAlgId,
                                      pbHash,
                                      cbHash);

        if (FAILED(hr)) {
            FreeM(pbImage);
            COMPlusThrowHR(hr);
        }
    }

    BOOL fResource = IsFfContainsNoMetaData(dwFlags);
    PEFile *pFile;

    hr = PEFile::Create(pbImage, cbImage, 
                        pModuleName, 
                        NULL, 
                        &gc.security, 
                        &pFile, 
                        fResource);

    FreeM(pbImage);
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    Module* pModule = NULL;
    OBJECTREF Throwable = NULL;  
    GCPROTECT_BEGIN(Throwable);
    hr = pAssembly->LoadFoundInternalModule(pFile,
                                            kFile,
                                            fResource,
                                            &pModule,
                                            &Throwable);
    if(hr != S_OK) {
        if (Throwable != NULL)
            COMPlusThrow(Throwable);

        if (hr == S_FALSE)
            COMPlusThrow(kArgumentException, L"Argument_ModuleAlreadyLoaded");

        COMPlusThrowHR(hr);
    }
    GCPROTECT_END();

    LOG((LF_CLASSLOADER, 
         LL_INFO100, 
         "\tLoaded in-memory module\n"));

    if (pModule) {
#ifdef DEBUGGING_SUPPORTED
        if (!fResource) {
            // If we were given symbols and we need to track JIT info for
            // the debugger, load them now.
            PBYTE pbSyms = NULL;
            DWORD cbSyms = 0;
            
            if ((gc.SymByteArray != NULL) &&
                CORDebuggerTrackJITInfo(pModule->GetDebuggerInfoBits())) {
                SecurityHelper::CopyByteArrayToEncoding(&gc.SymByteArray,
                                                        &pbSyms, &cbSyms);
                
                hr = pModule->SetSymbolBytes(pbSyms, cbSyms);
                FreeM(pbSyms);
                
                if (FAILED(hr))
                    COMPlusThrowHR(hr);
            }
        }

#endif // DEBUGGING_SUPPORTED
        rv = pModule->GetExposedModuleObject();
    }

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

FCIMPL1(Object*, AssemblyNative::GetLocation, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF refRetVal = NULL;

    if (pAssembly->GetManifestFile() &&
        pAssembly->GetManifestFile()->GetFileName()) 
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
        refRetVal = COMString::NewString(pAssembly->GetManifestFile()->GetFileName()); 
        HELPER_METHOD_FRAME_END();
    }
    return OBJECTREFToObject(refRetVal);
    }
FCIMPLEND

FCIMPL2(Object*, AssemblyNative::GetType1Args, Object* refThisUNSAFE, StringObject* nameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF name      = (STRINGREF) nameUNSAFE;
    OBJECTREF refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, name);

    // Load the class from this module (fail if it is in a different one).
    refRetVal = (OBJECTREF)GetTypeInner(pAssembly, &name, FALSE, FALSE, TRUE, FALSE);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL3(Object*, AssemblyNative::GetType2Args, Object* refThisUNSAFE, StringObject* nameUNSAFE, BYTE bThrowOnError)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF name      = (STRINGREF) nameUNSAFE;
    OBJECTREF refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, name);

    // Load the class from this module (fail if it is in a different one).
    refRetVal = (OBJECTREF)GetTypeInner(pAssembly, &name, bThrowOnError, FALSE, TRUE, FALSE);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL4(Object*, AssemblyNative::GetType3Args, Object* refThisUNSAFE, StringObject* nameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF name      = (STRINGREF) nameUNSAFE;
    OBJECTREF refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, name);

    // Load the class from this module (fail if it is in a different one).
    refRetVal = (OBJECTREF)GetTypeInner(pAssembly, &name, bThrowOnError, bIgnoreCase, TRUE, FALSE);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
    }
FCIMPLEND

FCIMPL5(Object*, AssemblyNative::GetTypeInternal, Object* refThisUNSAFE, StringObject* nameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase, BYTE bPublicOnly)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF name      = (STRINGREF) nameUNSAFE;
    OBJECTREF refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, name);

    // Load the class from this module (fail if it is in a different one).
    refRetVal = (OBJECTREF)GetTypeInner(pAssembly, &name, bThrowOnError, bIgnoreCase, FALSE, bPublicOnly);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

extern EEClass *GetCallersClass(StackCrawlMark *stackMark, void *returnIP);
extern Assembly *GetCallersAssembly(StackCrawlMark *stackMark, void *returnIP);

Object* AssemblyNative::GetTypeInner(Assembly *pAssembly,
                                    STRINGREF *refClassName, 
                                    BOOL bThrowOnError, 
                                    BOOL bIgnoreCase, 
                                    BOOL bVerifyAccess,
                                    BOOL bPublicOnly)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF       sRef = *refClassName;
    if (!sRef)
        COMPlusThrowArgumentNull(L"typeName",L"ArgumentNull_String");

    OBJECTREF       rv = NULL;
    DWORD           strLen = sRef->GetStringLength() + 1;
    CQuickBytes     qb;
    LPUTF8          szClassName = (LPUTF8) qb.Alloc(strLen);
    CQuickBytes     bytes;
    DWORD           cClassName;

    // Get the class name in UTF8
    if (!COMString::TryConvertStringDataToUTF8(sRef, szClassName, strLen))
        szClassName = GetClassStringVars(sRef, &bytes, &cClassName);
    
    // Find the return address. This can be used to find caller's assembly
//    Frame* pFrame = GetThread()->GetFrame();
//    _ASSERTE(pFrame->IsFramedMethodFrame());

    void* returnIP = NULL;
    EEClass *pCallersClass = NULL;
    Assembly *pCallersAssembly = NULL;

    if(pAssembly) {
        TypeHandle typeHnd;
        BOOL fVisible = TRUE;
        OBJECTREF Throwable = NULL;

        // Look for namespace separator
        LPUTF8 szNameSpaceSep = NULL;
        LPUTF8 szWalker = szClassName;
        DWORD nameLen = 0;
        for (; *szWalker; szWalker++, nameLen++) {

            if (*szWalker == NAMESPACE_SEPARATOR_CHAR)
                szNameSpaceSep = szWalker;
        }
        
        if (nameLen >= MAX_CLASSNAME_LENGTH)
            COMPlusThrow(kArgumentException, L"Argument_TypeNameTooLong");

        GCPROTECT_BEGIN(Throwable);

        if (NormalizeArrayTypeName(szClassName, nameLen)) {

            if (!*szClassName)
              COMPlusThrow(kArgumentException, L"Format_StringZeroLength");

            NameHandle typeName;
            char noNameSpace = '\0';
            if (szNameSpaceSep) {

                *szNameSpaceSep = '\0';
                typeName.SetName(szClassName, szNameSpaceSep + 1);
            }
            else
                typeName.SetName(&noNameSpace, szClassName);

            if(bIgnoreCase)
                typeName.SetCaseInsensitive();
            else
                typeName.SetCaseSensitive();

            if (bVerifyAccess) {
                pCallersClass = GetCallersClass(NULL, returnIP);
                pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
            }

            // Returning NULL only means that the type is not in this assembly.
            typeHnd = pAssembly->FindNestedTypeHandle(&typeName, &Throwable);

            if (typeHnd.IsNull() && Throwable == NULL) 
                typeHnd = pAssembly->GetInternalType(&typeName, bThrowOnError, &Throwable);

            if (!typeHnd.IsNull() && bVerifyAccess) {
                    // verify visibility
                    EEClass *pClass = typeHnd.GetClassOrTypeParam();
                    _ASSERTE(pClass);

                if (bPublicOnly && !(IsTdPublic(pClass->GetProtection()) || IsTdNestedPublic(pClass->GetProtection())))
                    // the user is asking for a public class but the class we have is not public, discard
                    fVisible = FALSE;
                else {
                    // if the class is a top level public there is no check to perform
                    if (!IsTdPublic(pClass->GetProtection())) {
                        if (!pCallersAssembly) {
                            pCallersClass = GetCallersClass(NULL, returnIP);
                            pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
                        }

                        if (pCallersAssembly && // full trust for interop
                            !ClassLoader::CanAccess(pCallersClass,
                                                    pCallersAssembly,
                                                    pClass,
                                                    pClass->GetAssembly(),
                                                    pClass->GetAttrClass())) {
                            // This is not legal if the user doesn't have reflection permission
                            if (!AssemblyNative::HaveReflectionPermission(bThrowOnError))
                                fVisible = FALSE;
                        }
                    }
                }
            }

            if((!typeHnd.IsNull()) && fVisible)
                // There one case were this may return null, if typeHnd
                //  represents the Transparent proxy.
                rv = typeHnd.CreateClassObj();
        }

        if ((rv == NULL) && bThrowOnError) {

            if (Throwable == NULL) {
                if (szNameSpaceSep)
                    *szNameSpaceSep = NAMESPACE_SEPARATOR_CHAR;

                pAssembly->PostTypeLoadException(szClassName, IDS_CLASSLOAD_GENERIC, &Throwable);
            }

            COMPlusThrow(Throwable);
        }
        
        GCPROTECT_END();
    }
    
    return OBJECTREFToObject(rv);
}



BOOL AssemblyNative::HaveReflectionPermission(BOOL ThrowOnFalse)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL haveReflectionPermission = TRUE;
    COMPLUS_TRY {
        COMMember::g_pInvokeUtil->CheckSecurity();
    }
    COMPLUS_CATCH {
        if (ThrowOnFalse)
            COMPlusRareRethrow();

        haveReflectionPermission = FALSE;
    } COMPLUS_END_CATCH

    return haveReflectionPermission;
}
 
FCIMPL5(INT32, AssemblyNative::GetVersion, Object* refThisUNSAFE, INT32* pMajorVersion, INT32* pMinorVersion, INT32*pBuildNumber, INT32* pRevisionNumber)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);

    if (pAssembly->m_Context) 
    {
        *pRevisionNumber = pAssembly->m_Context->usRevisionNumber;
        *pBuildNumber = pAssembly->m_Context->usBuildNumber;
        *pMinorVersion = pAssembly->m_Context->usMinorVersion;
        *pMajorVersion = pAssembly->m_Context->usMajorVersion;
        return 1;        
    }
    else
    {
        return 0;
}
}
FCIMPLEND

FCIMPL1(Object*, AssemblyNative::GetPublicKey, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    OBJECTREF refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    SecurityHelper::CopyEncodingToByteArray(pAssembly->m_pbPublicKey,
                                            pAssembly->m_cbPublicKey,
                                            &refRetVal);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL1(LPVOID, AssemblyNative::GetSimpleName, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly   = GET_ASSEMBLY(refThisUNSAFE);
    LPVOID      rv          = NULL;

    if (pAssembly->m_psName)
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
        *((OBJECTREF*)(&rv)) = (OBJECTREF) COMString::NewString(pAssembly->m_psName);
        HELPER_METHOD_FRAME_END();
    }

    return rv;
}
FCIMPLEND


FCIMPL1(Object*, AssemblyNative::GetLocale, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF refRetVal = NULL;

    if (pAssembly->m_Context &&
        pAssembly->m_Context->szLocale &&
        *pAssembly->m_Context->szLocale)
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
        refRetVal = COMString::NewString(pAssembly->m_Context->szLocale);
        HELPER_METHOD_FRAME_END();
    }

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL3(Object*, AssemblyNative::GetCodeBase, Object* refThisUNSAFE, BYTE fCopiedName, BYTE fEscape)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF refRetVal = NULL;

    HRESULT hr;
    LPWSTR  pCodeBase = NULL;
    DWORD   dwCodeBase = 0;
    CQuickBytes qb;


    if(fCopiedName && pAssembly->GetDomain()->IsShadowCopyOn()) {
        hr = pAssembly->FindCodeBase(pCodeBase, &dwCodeBase, TRUE);

        if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            pCodeBase = (LPWSTR) qb.Alloc(dwCodeBase * sizeof(WCHAR));
            hr = pAssembly->FindCodeBase(pCodeBase, &dwCodeBase, TRUE);
        }
    }
    else
    {
        hr = pAssembly->GetCodeBase(&pCodeBase, &dwCodeBase);
    }
        

    if(SUCCEEDED(hr) && pCodeBase) {
        CQuickBytes qb2;

        if (fEscape) {
            DWORD dwEscaped = 1;
            UrlEscape(pCodeBase, (LPWSTR) qb2.Ptr(), &dwEscaped, 0);

            LPWSTR result = (LPWSTR)qb2.Alloc((++dwEscaped) * sizeof(WCHAR));
            hr = UrlEscape(pCodeBase, result, &dwEscaped, 0);
            pCodeBase = result;
        }

        if (SUCCEEDED(hr))
        {
            HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
            refRetVal = COMString::NewString(pCodeBase);
            HELPER_METHOD_FRAME_END();
        }
    }

    if (FAILED(hr))
        COMPlusThrowHR(hr);

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL1(INT32, AssemblyNative::GetHashAlgorithm, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    return pAssembly->m_ulHashAlgId;
}
FCIMPLEND


FCIMPL1(INT32,  AssemblyNative::GetFlags, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    return pAssembly->m_dwFlags;
}
FCIMPLEND

FCIMPL5(BYTE*, AssemblyNative::GetResource, Object* refThisUNSAFE, StringObject* nameUNSAFE, UINT64* length, StackCrawlMark* stackMark, bool skipSecurityCheck)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly           = GET_ASSEMBLY(refThisUNSAFE);
    PBYTE       pbInMemoryResource  = NULL;
    STRINGREF   name                = ObjectToSTRINGREF(nameUNSAFE);

    _ASSERTE(length != NULL);


    if (name == NULL)
    {
        HELPER_METHOD_FRAME_BEGIN_RET_0();
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
        HELPER_METHOD_FRAME_END();
    }
        
    // Get the name in UTF8
    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    HELPER_METHOD_FRAME_BEGIN_RET_1(name);
    szName = GetClassStringVars((STRINGREF)name, &bytes, &cName);
    if (!cName)
    {
        COMPlusThrow(kArgumentException, L"Format_StringZeroLength");
    }

    DWORD  cbResource;

    if (SUCCEEDED(pAssembly->GetResource(szName, NULL, &cbResource,
                                         &pbInMemoryResource, NULL, NULL,
                                         NULL, stackMark, skipSecurityCheck))) {
        _ASSERTE(pbInMemoryResource);
        *length = cbResource;
    }
    HELPER_METHOD_FRAME_END();

    return pbInMemoryResource;
}
FCIMPLEND


FCIMPL5(INT32, AssemblyNative::GetManifestResourceInfo, Object* refThisUNSAFE, StringObject* nameUNSAFE, OBJECTREF* pAssemblyRef, STRINGREF* pFileNameOUT, StackCrawlMark* stackMark)
{
    THROWSCOMPLUSEXCEPTION();

    INT32 rv = -1;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF name      = (STRINGREF) nameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(name);

    if (name == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
        
    // Get the name in UTF8
    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    szName = GetClassStringVars(name, &bytes, &cName);
    if (!cName)
        COMPlusThrow(kArgumentException, L"Format_StringZeroLength");

    *pFileNameOUT   = NULL;
    *pAssemblyRef   = NULL;

    LPCSTR pFileName = NULL;
    DWORD dwLocation = 0;
    Assembly *pReferencedAssembly = NULL;

    if (SUCCEEDED(pAssembly->GetResource(szName, NULL, NULL, NULL,
                                      &pReferencedAssembly, &pFileName,
                                      &dwLocation, stackMark)))
    {
        if (pFileName) {
            *((STRINGREF*) (&(*pFileNameOUT))) = COMString::NewString(pFileName);
        }
        if (pReferencedAssembly) {
            *((OBJECTREF*) (&(*pAssemblyRef))) = pReferencedAssembly->GetExposedObject();
        }

        rv = dwLocation;
}

    HELPER_METHOD_FRAME_END();

    return rv;
}
FCIMPLEND

FCIMPL3(Object*, AssemblyNative::GetModules, Object* refThisUNSAFE, INT32 fLoadIfNotFound, INT32 fGetResourceModules)
{
    THROWSCOMPLUSEXCEPTION();

    PTRARRAYREF     rv              = NULL;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*       pAssembly       = GET_ASSEMBLY(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    MethodTable *pModuleClass = g_Mscorlib.GetClass(CLASS__MODULE);

    HENUMInternal phEnum;

    HRESULT hr;
    if (FAILED(hr = pAssembly->GetManifestImport()->EnumInit(mdtFile,
                                                             mdTokenNil,
                                                             &phEnum)))
        COMPlusThrowHR(hr);

    DWORD dwElements = pAssembly->GetManifestImport()->EnumGetCount(&phEnum) + 1;

    struct _gc {
        PTRARRAYREF ModArray;
        PTRARRAYREF nArray;
        OBJECTREF Throwable;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    
    
    GCPROTECT_BEGIN(gc);
    gc.ModArray = (PTRARRAYREF) AllocateObjectArray(dwElements,pModuleClass);
    if (!gc.ModArray)
        COMPlusThrowOM();
    
    int iCount = 0;

    mdFile  mdFile;
    OBJECTREF o = pAssembly->GetSecurityModule()->GetExposedModuleObject();
    gc.ModArray->SetAt(0, o);

    for(int i = 1;
        pAssembly->GetManifestImport()->EnumNext(&phEnum, &mdFile);
        i++) {
            
        Module  *pModule = pAssembly->GetSecurityModule()->LookupFile(mdFile);

        if (pModule) {
            if (pModule->IsResource() &&
                (!fGetResourceModules))
                pModule = NULL;
        }
        else if (fLoadIfNotFound) {
            // Module isn't loaded yet

            LPCSTR szModuleName;
            const BYTE* pHash;
            DWORD dwFlags;
            ULONG dwHashLength;
            pAssembly->GetManifestImport()->GetFileProps(mdFile,
                                                         &szModuleName,
                                                         (const void**) &pHash,
                                                         &dwHashLength,
                                                         &dwFlags);

            if (IsFfContainsMetaData(dwFlags) ||
                fGetResourceModules) {
                WCHAR pPath[MAX_PATH];
                hr = pAssembly->LoadInternalModule(szModuleName,
                                                   mdFile,
                                                   pAssembly->m_ulHashAlgId,
                                                   pHash,
                                                   dwHashLength,
                                                   dwFlags,
                                                   pPath,
                                                   MAX_PATH,
                                                   &pModule, 
                                                   &gc.Throwable);
                if(FAILED(hr)) {
                    pAssembly->GetManifestImport()->EnumClose(&phEnum);
                    
                    if (gc.Throwable != NULL)
                        COMPlusThrow(gc.Throwable);
                    COMPlusThrowHR(hr);
                }
            }
        }

        if(pModule) {
            OBJECTREF o = (OBJECTREF) pModule->GetExposedModuleObject();
            gc.ModArray->SetAt(i, o);
        }
        else
            iCount++;
    }
    
    pAssembly->GetManifestImport()->EnumClose(&phEnum);        
    
    if(iCount) {
        gc.nArray = (PTRARRAYREF) AllocateObjectArray(dwElements - iCount, pModuleClass);
        DWORD index = 0;
        for(DWORD ii = 0; ii < dwElements; ii++) {
            if(gc.ModArray->GetAt(ii) != NULL) {
                _ASSERTE(index < dwElements - iCount);
                gc.nArray->SetAt(index, gc.ModArray->GetAt(ii));
                index++;
            }
        }
        
        rv = gc.nArray;
    }
    else 
    {
        rv = gc.ModArray;
    }
    
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND


FCIMPL2(Object*, AssemblyNative::GetModule, Object* refThisUNSAFE, StringObject* strFileNameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly     = GET_ASSEMBLY(refThisUNSAFE);
    OBJECTREF rv            = NULL;
    STRINGREF strFileName   = (STRINGREF) strFileNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, strFileName);

    if (strFileName == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_FileName");
    if(*(strFileName->GetBuffer()) == '\0')
        COMPlusThrow(kArgumentException, L"Argument_EmptyFileName");
    
    Module *pModule;
    MAKE_UTF8PTR_FROMWIDE(szModuleName, strFileName->GetBuffer());

    HashDatum datum;
    if (pAssembly->m_pAllowedFiles->GetValue(szModuleName, &datum)) 
    {
        if (datum) 
        { // internal module
            pModule = pAssembly->GetSecurityModule()->LookupFile((mdFile)UnsafePointerTruncate(datum));
            if (!pModule) {
                const BYTE* pHash;
                DWORD dwFlags = 0;
                ULONG dwHashLength = 0;
                HRESULT hr;
                pAssembly->GetManifestImport()->GetFileProps((mdFile)UnsafePointerTruncate(datum),
                                                             NULL, //&szModuleName,
                                                             (const void**) &pHash,
                                                             &dwHashLength,
                                                             &dwFlags);
                
                OBJECTREF Throwable = NULL;
                GCPROTECT_BEGIN(Throwable);
                WCHAR pPath[MAX_PATH];
                hr = pAssembly->LoadInternalModule(szModuleName,
                                                   (mdFile)UnsafePointerTruncate(datum),
                                                   pAssembly->m_ulHashAlgId,
                                                   pHash,
                                                   dwHashLength,
                                                   dwFlags,
                                                   pPath,
                                                   MAX_PATH,
                                                   &pModule,
                                                   &Throwable);

                if (Throwable != NULL)
                    COMPlusThrow(Throwable);
                GCPROTECT_END();

                if (FAILED(hr))
                    COMPlusThrowHR(hr);
            }
        }
        else // manifest module
        {
            pModule = pAssembly->GetSecurityModule();
        }
            
        rv = pModule->GetExposedModuleObject();
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

static Object* __fastcall GetExportedTypesInternal(Assembly* pAssembly);

FCIMPL1(Object*, AssemblyNative::GetExportedTypes, Object* refThisUNSAFE)
{
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    OBJECTREF   rv        = NULL;   

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    rv = ObjectToOBJECTREF(GetExportedTypesInternal(pAssembly));

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

static Object* __fastcall GetExportedTypesInternal(Assembly* pAssembly)
{
    THROWSCOMPLUSEXCEPTION();

    PTRARRAYREF  rv         = NULL;
    MethodTable *pTypeClass = g_Mscorlib.GetClass(CLASS__TYPE);
    
    _ASSERTE(pAssembly->GetManifestImport());

    HENUMInternal phCTEnum;
    HENUMInternal phTDEnum;
    DWORD dwElements;

    if (pAssembly->GetManifestImport()->EnumInit(mdtExportedType,
                                                 mdTokenNil,
                                                 &phCTEnum) == S_OK)
        dwElements = pAssembly->GetManifestImport()->EnumGetCount(&phCTEnum);
    else
        dwElements = 0;

    if (pAssembly->GetSecurityModule()->GetMDImport()->EnumTypeDefInit(&phTDEnum) == S_OK)
        dwElements += pAssembly->GetSecurityModule()->GetMDImport()->EnumGetCount(&phTDEnum);


    mdExportedType mdCT;
    mdTypeDef mdTD;
    LPCSTR pszNameSpace;
    LPCSTR pszClassName;
    DWORD dwFlags;
    int iCount = 0;
    
    struct _gc {
        PTRARRAYREF TypeArray;
        PTRARRAYREF nArray;
        OBJECTREF Throwable;  
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    COMPLUS_TRY {
        gc.TypeArray = (PTRARRAYREF) AllocateObjectArray(dwElements, pTypeClass);
        if (gc.TypeArray == NULL)
            COMPlusThrowOM();
        
        while(pAssembly->GetSecurityModule()->GetMDImport()->EnumNext(&phTDEnum, &mdTD)) {
            mdTypeDef mdEncloser;
            TypeHandle typeHnd;
            
            pAssembly->GetSecurityModule()->GetMDImport()->GetNameOfTypeDef(mdTD,
                                                                            &pszClassName,
                                                                            &pszNameSpace);
            pAssembly->GetSecurityModule()->GetMDImport()->GetTypeDefProps(mdTD,
                                                                           &dwFlags,
                                                                           NULL);
            mdEncloser = mdTD;
            
            // nested type
            while (SUCCEEDED(pAssembly->GetSecurityModule()->GetMDImport()->GetNestedClassProps(mdEncloser, &mdEncloser)) &&
                   IsTdNestedPublic(dwFlags)) {
                pAssembly->GetSecurityModule()->GetMDImport()->GetTypeDefProps(mdEncloser,
                                                                               &dwFlags,
                                                                               NULL);
            }
            
            if (IsTdPublic(dwFlags)) {
                NameHandle typeName(pAssembly->GetSecurityModule(), mdTD);
                typeName.SetName(pszNameSpace, pszClassName);
                typeHnd = pAssembly->LoadTypeHandle(&typeName, &gc.Throwable);
                if (!typeHnd.IsNull()) {
                    OBJECTREF o = typeHnd.CreateClassObj();
                    gc.TypeArray->SetAt(iCount, o);
                    iCount++;
                }
                else if (gc.Throwable != NULL)
                    COMPlusThrow(gc.Throwable);
            }
        }
        
        
        // Now get the ExportedTypes that don't have TD's in the manifest file
        while(pAssembly->GetManifestImport()->EnumNext(&phCTEnum, &mdCT)) {
            mdToken mdImpl;
            TypeHandle typeHnd;
            pAssembly->GetManifestImport()->GetExportedTypeProps(mdCT,
                                                                 &pszNameSpace,
                                                                 &pszClassName,
                                                                 &mdImpl,
                                                                 NULL, //binding
                                                                 &dwFlags);
            
            // nested type
            while ((TypeFromToken(mdImpl) == mdtExportedType) &&
                   (mdImpl != mdExportedTypeNil) &&
                   IsTdNestedPublic(dwFlags)) {
                
                pAssembly->GetManifestImport()->GetExportedTypeProps(mdImpl,
                                                                     NULL, //namespace
                                                                     NULL, //name
                                                                     &mdImpl,
                                                                     NULL, //binding
                                                                     &dwFlags);
            }
            
            if ((TypeFromToken(mdImpl) == mdtFile) &&
                (mdImpl != mdFileNil) &&
                IsTdPublic(dwFlags)) {
                
                NameHandle typeName(pszNameSpace, pszClassName);
                typeName.SetTypeToken(pAssembly->GetSecurityModule(), mdCT);
                typeHnd = pAssembly->LookupTypeHandle(&typeName, &gc.Throwable);
                    
                if (typeHnd.IsNull() || (gc.Throwable != NULL)) {
                    if (gc.Throwable == NULL) {
                        pAssembly->PostTypeLoadException(pszNameSpace, pszClassName, IDS_CLASSLOAD_GENERIC, &gc.Throwable);
                    }

                    COMPlusThrow(gc.Throwable);                    
                }
                else {
                    OBJECTREF o = typeHnd.CreateClassObj();
                    gc.TypeArray->SetAt(iCount, o);
                    iCount++;
                }
            }
        }

        gc.nArray = (PTRARRAYREF) AllocateObjectArray(iCount, pTypeClass);
        for(int i = 0; i < iCount; i++)
            gc.nArray->SetAt(i, gc.TypeArray->GetAt(i));
        
        rv = gc.nArray;
    }
    COMPLUS_FINALLY 
    {
        pAssembly->GetManifestImport()->EnumClose(&phCTEnum);
        pAssembly->GetSecurityModule()->GetMDImport()->EnumTypeDefClose(&phTDEnum);
    } 
    COMPLUS_END_FINALLY
        
    GCPROTECT_END();
    
    return OBJECTREFToObject(rv);
}

FCIMPL1(Object*, AssemblyNative::GetResourceNames, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    PTRARRAYREF rv        = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    _ASSERTE(pAssembly->GetManifestImport());

    HENUMInternal phEnum;
    DWORD dwCount;

    if (pAssembly->GetManifestImport()->EnumInit(mdtManifestResource,
                                                 mdTokenNil,
                                                 &phEnum) == S_OK)
        dwCount = pAssembly->GetManifestImport()->EnumGetCount(&phEnum);
    else
        dwCount = 0;

    PTRARRAYREF ItemArray = (PTRARRAYREF) AllocateObjectArray(dwCount, g_pStringClass);
        
    if (ItemArray == NULL)
        COMPlusThrowOM();

    mdManifestResource mdResource;

    GCPROTECT_BEGIN(ItemArray);
    for(DWORD i = 0;  i < dwCount; i++) {
        pAssembly->GetManifestImport()->EnumNext(&phEnum, &mdResource);
        LPCSTR pszName = NULL;
        
        pAssembly->GetManifestImport()->GetManifestResourceProps(mdResource,
                                                                 &pszName, // name
                                                                 NULL, // linkref
                                                                 NULL, // offset
                                                                 NULL); //flags
           
        OBJECTREF o = (OBJECTREF) COMString::NewString(pszName);
        ItemArray->SetAt(i, o);
    }
     
    rv = ItemArray;
    GCPROTECT_END();
    
    pAssembly->GetManifestImport()->EnumClose(&phEnum);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

FCIMPL1(Object*, AssemblyNative::GetReferencedAssemblies, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    PTRARRAYREF rv        = NULL;   

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    _ASSERTE(pAssembly->GetManifestImport());

    HENUMInternal phEnum;
    DWORD dwCount;

    if (pAssembly->GetManifestImport()->EnumInit(mdtAssemblyRef,
                                                 mdTokenNil,
                                                 &phEnum) == S_OK)
        dwCount = pAssembly->GetManifestImport()->EnumGetCount(&phEnum);
    else
        dwCount = 0;

    MethodTable* pClass = g_Mscorlib.GetClass(CLASS__ASSEMBLY_NAME);
    MethodTable* pVersion = g_Mscorlib.GetClass(CLASS__VERSION);
    MethodTable* pCI = g_Mscorlib.GetClass(CLASS__CULTURE_INFO);
    
    mdAssemblyRef mdAssemblyRef;

    struct _gc {
        PTRARRAYREF ItemArray;
        OBJECTREF pObj;
        OBJECTREF CultureInfo;
        STRINGREF Locale;
        OBJECTREF Version;
        U1ARRAYREF PublicKeyOrToken;
        STRINGREF Name;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    
    GCPROTECT_BEGIN(gc);
 
    gc.ItemArray = (PTRARRAYREF) AllocateObjectArray(dwCount, pClass);        
    if (gc.ItemArray == NULL)
        goto ErrExit;

    for(DWORD i = 0; i < dwCount; i++) {

        pAssembly->GetManifestImport()->EnumNext(&phEnum, &mdAssemblyRef);

        LPCSTR pszName;
        const void *pbPublicKeyOrToken;
        DWORD cbPublicKeyOrToken;
        DWORD dwAssemblyRefFlags;
        AssemblyMetaDataInternal context;
        const void *pbHashValue;
        DWORD cbHashValue;

        ZeroMemory(&context, sizeof(context));
        pAssembly->GetManifestImport()->GetAssemblyRefProps(mdAssemblyRef,        // [IN] The AssemblyRef for which to get the properties.        
                                                            &pbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.
                                                            &cbPublicKeyOrToken,  // [OUT] Count of bytes in the public key or token.
                                                            &pszName,             // [OUT] Buffer to fill with name.                              
                                                            &context,             // [OUT] Assembly MetaData.                                     
                                                            &pbHashValue,         // [OUT] Hash blob.                                             
                                                            &cbHashValue,         // [OUT] Count of bytes in the hash blob.                       
                                                            &dwAssemblyRefFlags); // [OUT] Flags.                                             

        MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__VERSION__CTOR);

        // version
        gc.Version = AllocateObject(pVersion);
        
        ARG_SLOT VersionArgs[5] =
        {
            ObjToArgSlot(gc.Version),
            (ARG_SLOT) context.usMajorVersion,      
            (ARG_SLOT) context.usMinorVersion,
            (ARG_SLOT) context.usBuildNumber,
            (ARG_SLOT) context.usRevisionNumber,
        };
        pCtor->Call(VersionArgs, METHOD__VERSION__CTOR);


        
        // cultureinfo
        if (context.szLocale) {

            MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__CULTURE_INFO__STR_CTOR);

            gc.CultureInfo = AllocateObject(pCI);

            gc.Locale = COMString::NewString(context.szLocale);
            
            ARG_SLOT args[2] = 
            {
                ObjToArgSlot(gc.CultureInfo),
                ObjToArgSlot(gc.Locale)
            };

            pCtor->Call(args, METHOD__CULTURE_INFO__STR_CTOR);
        }
        
        // public key or token byte array
        SecurityHelper::CopyEncodingToByteArray((BYTE*) pbPublicKeyOrToken,
                                                cbPublicKeyOrToken,
                                                (OBJECTREF*) &gc.PublicKeyOrToken);

        // simple name
        if(pszName && *pszName) {
            if(strchr(pszName,'\\') || strchr(pszName,':')) {
                _ASSERTE(!"Assembly Name With Path");
                goto ErrExit;
            }
            gc.Name = COMString::NewString(pszName);
        }
        else {
            _ASSERTE(!"No Assembly Name");
            goto ErrExit;
        }

        pCtor = g_Mscorlib.GetMethod(METHOD__ASSEMBLY_NAME__CTOR);

        gc.pObj = AllocateObject(pClass);
        ARG_SLOT MethodArgs[] = { ObjToArgSlot(gc.pObj),
                               ObjToArgSlot(gc.Name),
                               ObjToArgSlot(gc.PublicKeyOrToken),
                               (ARG_SLOT) NULL, // codebase
                               (ARG_SLOT) pAssembly->m_ulHashAlgId,
                               ObjToArgSlot(gc.Version),
                               ObjToArgSlot(gc.CultureInfo),
                               (ARG_SLOT) dwAssemblyRefFlags,
        };

        pCtor->Call(MethodArgs, METHOD__ASSEMBLY_NAME__CTOR);

        gc.ItemArray->SetAt(i, gc.pObj);
    }
     
    rv = gc.ItemArray;

 ErrExit:
    GCPROTECT_END();
    pAssembly->GetManifestImport()->EnumClose(&phEnum);
    
    if (rv == NULL) COMPlusThrowOM();

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

FCIMPL1(Object*, AssemblyNative::GetEntryPoint, Object* refThisUNSAFE)
{
    mdToken     tkEntry;
    mdToken     tkParent;
    MethodDesc* pMeth;
    Module*     pModule = 0;
    HRESULT     hr;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly   = GET_ASSEMBLY(refThisUNSAFE);
    OBJECTREF   rv          = NULL; 
    OBJECTREF   o           = NULL;
    
    THROWSCOMPLUSEXCEPTION();
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

	hr = pAssembly->GetEntryPoint(&pModule);
    if (FAILED(hr))
    {
        if (hr == E_FAIL) // no entrypoint
        {
            goto lExit;
        }

        COMPlusThrowHR(hr);
    }

    _ASSERTE(pModule);

    tkEntry = VAL32(pModule->GetCORHeader()->EntryPointToken);

    hr = pModule->GetMDImport()->GetParentToken(tkEntry,&tkParent);
    if (FAILED(hr))
    {
        _ASSERTE(!"Unable to find ParentToken");
        goto lExit;
    }

    if (tkParent != COR_GLOBAL_PARENT_TOKEN) 
    {
        EEClass* InitialClass;
        OBJECTREF Throwable = NULL;
        GCPROTECT_BEGIN(Throwable);

        NameHandle name;
        name.SetTypeToken(pModule, tkParent);
        InitialClass = pAssembly->GetLoader()->LoadTypeHandle(&name, &Throwable).GetClass();
        if (!InitialClass)
            COMPlusThrow(Throwable);


        GCPROTECT_END();

        pMeth = InitialClass->FindMethod((mdMethodDef)tkEntry);  
        o = COMMember::g_pInvokeUtil->GetMethodInfo(pMeth);
    }   
    else 
    { 
        pMeth = pModule->FindFunction((mdToken)tkEntry);
        o = COMMember::g_pInvokeUtil->GetGlobalMethodInfo(pMeth,pModule);
    }

    rv = o;

lExit: ;
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

// prepare saving manifest to disk
FCIMPL2(void,  AssemblyNative::PrepareSavingManifest, Object* refThisUNSAFE, ReflectModuleBaseObject* moduleUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly*               pAssembly   = GET_ASSEMBLY(refThisUNSAFE);
    REFLECTMODULEBASEREF    pReflect    = (REFLECTMODULEBASEREF) moduleUNSAFE;
    ReflectionModule    *pModule = NULL;

    HELPER_METHOD_FRAME_BEGIN_1(pReflect);

    if (pReflect != NULL)
    {
        pModule = (ReflectionModule*) pReflect->GetData();
        _ASSERTE(pModule);
    }

    pAssembly->PrepareSavingManifest(pModule);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// add a file name to the file list of this assembly. On disk only.
FCIMPL2(mdFile, AssemblyNative::AddFileList, Object* refThisUNSAFE, StringObject* strFileNameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly   = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF   strFileName = (STRINGREF) strFileNameUNSAFE;
    mdFile      retVal;

    HELPER_METHOD_FRAME_BEGIN_RET_1(strFileName);
    retVal = pAssembly->AddFileList(strFileName->GetBuffer());
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

// set the hash value on a file.
FCIMPL3(void, AssemblyNative::SetHashValue, Object* refThisUNSAFE, INT32 tkFile, StringObject* strFullFileNameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(strFullFileNameUNSAFE != NULL);

    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly         = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF strFullFileName   = (STRINGREF) strFullFileNameUNSAFE;


    HELPER_METHOD_FRAME_BEGIN_1(strFullFileName);
    pAssembly->SetHashValue(tkFile, strFullFileName->GetBuffer());
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// add a Type name to COMType table. On disk only.
FCIMPL5(mdExportedType,  AssemblyNative::AddExportedType, Object* refThisUNSAFE, StringObject* strCOMTypeNameUNSAFE, INT32 ar, INT32 tkTypeDef, INT32 flags)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*       pAssembly       = GET_ASSEMBLY(refThisUNSAFE);
    STRINGREF       strCOMTypeName  = (STRINGREF) strCOMTypeNameUNSAFE;
    mdExportedType  retVal;

    HELPER_METHOD_FRAME_BEGIN_RET_1(strCOMTypeName);
    retVal = pAssembly->AddExportedType(strCOMTypeName->GetBuffer(), ar, tkTypeDef, (CorTypeAttr)flags);
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

// add a Stand alone resource to ManifestResource table
FCIMPL5(void, AssemblyNative::AddStandAloneResource, Object* refThisUNSAFE, StringObject* strNameUNSAFE, StringObject* strFileNameUNSAFE, StringObject* strFullFileNameUNSAFE, INT32 iAttribute)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);

    struct _gc
    {
        STRINGREF strName;
        STRINGREF strFileName;
        STRINGREF strFullFileName;
    } gc;

    gc.strName          = (STRINGREF) strNameUNSAFE;
    gc.strFileName      = (STRINGREF) strFileNameUNSAFE;
    gc.strFullFileName  = (STRINGREF) strFullFileNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);

    pAssembly->AddStandAloneResource(
        gc.strName->GetBuffer(), 
        NULL,
        NULL,
        gc.strFileName->GetBuffer(),
        gc.strFullFileName->GetBuffer(),
        iAttribute); 

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// Save security permission requests.
FCIMPL4(void, AssemblyNative::SavePermissionRequests, Object* refThisUNSAFE, U1Array* requiredUNSAFE, U1Array* optionalUNSAFE, U1Array* refusedUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);

    struct _gc
    {
        U1ARRAYREF required;
        U1ARRAYREF optional;
        U1ARRAYREF refused;
    } gc;

    gc.required = (U1ARRAYREF) requiredUNSAFE;
    gc.optional = (U1ARRAYREF) optionalUNSAFE;
    gc.refused  = (U1ARRAYREF) refusedUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);

    pAssembly->SavePermissionRequests(gc.required, gc.optional, gc.refused);

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// save the manifest to disk!
FCIMPL4(void, AssemblyNative::SaveManifestToDisk, Object* refThisUNSAFE, StringObject* strManifestFileNameUNSAFE, INT32 entrypoint, INT32 fileKind)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    // Make a copy of the file name, GC could move strManifestFileName

    CQuickBytes qb;
    LPWSTR      pwszFileName = (LPWSTR) qb.Alloc(
            (strManifestFileNameUNSAFE->GetStringLength() + 1) * sizeof(WCHAR));

    memcpyNoGCRefs(pwszFileName, strManifestFileNameUNSAFE->GetBuffer(),
            (strManifestFileNameUNSAFE->GetStringLength() + 1) * sizeof(WCHAR));

    pAssembly->SaveManifestToDisk(pwszFileName, entrypoint, fileKind);

    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// Add a file entry into the in memory file list of this manifest
FCIMPL3(void, AssemblyNative::AddFileToInMemoryFileList, Object* refThisUNSAFE, StringObject* strModuleFileNameUNSAFE, Object* refModuleUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    REFLECTMODULEBASEREF    pReflect;
    Module                  *pModule;
    pReflect = (REFLECTMODULEBASEREF) ObjectToOBJECTREF(refModuleUNSAFE);
    _ASSERTE(pReflect);

    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);
    
    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    pAssembly->AddFileToInMemoryFileList(strModuleFileNameUNSAFE->GetBuffer(), pModule);

    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND


FCIMPL1(Object*, AssemblyNative::GetStringizedName, Object* refThisUNSAFE)
{
    STRINGREF   refRetVal   = NULL;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly   = GET_ASSEMBLY(refThisUNSAFE);

    THROWSCOMPLUSEXCEPTION();

    // If called by Object.ToString(), pAssembly may be NULL.
    if (pAssembly)
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

        LPCWSTR wsFullName;
        HRESULT hr;
        if (FAILED(hr = pAssembly->GetFullName(&wsFullName)))
            COMPlusThrowHR(hr);
    
        refRetVal = COMString::NewString(wsFullName);
    
        HELPER_METHOD_FRAME_END();
    }

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


FCIMPL1(Object*, AssemblyNative::GetExecutingAssembly, StackCrawlMark* stackMark)
{
    THROWSCOMPLUSEXCEPTION();
    ASSEMBLYREF refRetVal = NULL;
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    Assembly* pAssembly = SystemDomain::GetCallersAssembly(stackMark);

    if(pAssembly)
    {
        refRetVal = (ASSEMBLYREF) pAssembly->GetExposedObject();
    }

    HELPER_METHOD_FRAME_END();
    
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


FCIMPL0(Object*, AssemblyNative::GetEntryAssembly)
{
    THROWSCOMPLUSEXCEPTION();
    OBJECTREF   rv      = NULL;
    BaseDomain *pDomain = SystemDomain::GetCurrentDomain();

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    if (!(pDomain == SystemDomain::System())) 
    {
        PEFile *pFile = ((AppDomain*) pDomain)->m_pRootFile;
        if(pFile) 
        {
            Module *pModule = pDomain->FindModule(pFile->GetBase());
            _ASSERTE(pModule);
            rv = pModule->GetAssembly()->GetExposedObject();
        }
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

FCIMPL2(Object*, AssemblyNative::CreateQualifiedName, StringObject* strAssemblyNameUNSAFE, StringObject* strTypeNameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    OBJECTREF   rv              = NULL;
    LPWSTR pTypeName = NULL;
    DWORD  dwTypeName = 0;
    LPWSTR pAssemblyName = NULL;
    DWORD  dwAssemblyName = 0;
    CQuickBytes qb;

    STRINGREF   strAssemblyName = (STRINGREF) strAssemblyNameUNSAFE;
    STRINGREF   strTypeName     = (STRINGREF) strTypeNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, strAssemblyName, strTypeName);
    
    if(strTypeName != NULL) {
        pTypeName = strTypeName->GetBuffer();
        dwTypeName = strTypeName->GetStringLength();
    }

    if(strAssemblyName != NULL) {
        pAssemblyName = strAssemblyName->GetBuffer();
        dwAssemblyName = strAssemblyName->GetStringLength();
    }

    DWORD length = dwTypeName + dwAssemblyName + ASSEMBLY_SEPARATOR_LEN + 1;
    LPWSTR result = (LPWSTR) qb.Alloc(length * sizeof(WCHAR));
    if(result == NULL) COMPlusThrowOM();

    if(ns::MakeAssemblyQualifiedName(result,
                                     length,
                                     pTypeName,
                                     dwTypeName,
                                     pAssemblyName,
                                     dwAssemblyName)) 
    {
        rv = (OBJECTREF) COMString::NewString(result);
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

FCIMPL1(void, AssemblyNative::ForceResolve, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    
    HELPER_METHOD_FRAME_BEGIN_0();
    
    // We get the evidence so that even if security is off
    // we generate the evidence properly.
    Security::InitSecurity();
    pAssembly->GetSecurityDescriptor()->GetEvidence();
        
    pAssembly->GetSecurityDescriptor()->Resolve();

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(void, AssemblyNative::GetGrantSet, Object* refThisUNSAFE, OBJECTREF* ppGranted, OBJECTREF* ppDenied)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_0();

    AssemblySecurityDescriptor *pSecDesc = pAssembly->GetSecurityDescriptor();

    pSecDesc->Resolve();

    OBJECTREF granted = pSecDesc->GetGrantedPermissionSet(ppDenied);
    *ppGranted = granted;

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// return the on disk assembly module for reflection emit. This only works for dynamic assembly.
FCIMPL1(Object*, AssemblyNative::GetOnDiskAssemblyModule, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE); 
    OBJECTREF rv        = NULL; 

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    ReflectionModule *mod;

    mod = pAssembly->GetOnDiskManifestModule();
    _ASSERTE(mod);

    // Assign the return value  
    rv = (OBJECTREF) mod->GetExposedModuleBuilderObject();     

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

// return the in memory assembly module for reflection emit. This only works for dynamic assembly.
FCIMPL1(Object*, AssemblyNative::GetInMemoryAssemblyModule, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly*   pAssembly   = GET_ASSEMBLY(refThisUNSAFE);
    Module *mod; 
    OBJECTREF   rv          = NULL; 

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    mod = pAssembly->GetSecurityModule();
    _ASSERTE(mod);

    // get the corresponding managed ModuleBuilder class
    rv = (OBJECTREF) mod->GetExposedModuleBuilderObject();

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND



FCIMPL1(INT32, AssemblyNative::GlobalAssemblyCache, Object* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    BOOL cache = FALSE;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Assembly* pAssembly = GET_ASSEMBLY(refThisUNSAFE);
    if(pAssembly->GetFusionAssembly()) {
        DWORD location;
        if(SUCCEEDED(pAssembly->GetFusionAssembly()->GetAssemblyLocation(&location)))
            if((location & ASMLOC_LOCATION_MASK) == ASMLOC_GAC)
                cache = TRUE;

    }
    return (INT32) cache;
}
FCIMPLEND

FCIMPL1(Object*, AssemblyNative::ParseTypeName, StringObject* typeNameUNSAFE)
{
    PTRARRAYREF refRetVal = NULL;
    STRINGREF typeName = (STRINGREF) typeNameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, typeName);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    
    if(typeName == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_AssemblyName");

    DWORD           strLen = typeName->GetStringLength() + 1;
    LPUTF8          szFulltypeName = (LPUTF8)_alloca(strLen);
    CQuickBytes     bytes;
    DWORD           ctypeName;
    LPUTF8          szAssemblyName = NULL;
    PTRARRAYREF        ItemArray = NULL;

    // Get the class name in UTF8
    if (!COMString::TryConvertStringDataToUTF8(typeName, szFulltypeName, strLen))
        szFulltypeName = GetClassStringVars(typeName, &bytes, &ctypeName);


    GCPROTECT_BEGIN(ItemArray);
    ItemArray = (PTRARRAYREF) AllocateObjectArray(2, g_pStringClass);
    if (ItemArray == NULL) COMPlusThrowOM();

    if(SUCCEEDED(FindAssemblyName(szFulltypeName,
                                  &szAssemblyName))) {
        OBJECTREF o = (OBJECTREF) COMString::NewString(szFulltypeName);
        ItemArray->SetAt(1, o);
        if(szAssemblyName != NULL) {
            o = (OBJECTREF) COMString::NewString(szAssemblyName);
            ItemArray->SetAt(0, o);
        }
        refRetVal = ItemArray;
    }
    GCPROTECT_END();
    
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

HRESULT AssemblyNative::FindAssemblyName(LPUTF8 szFullTypeName,
                                         LPUTF8* pszAssemblyName)
{
    _ASSERTE(pszAssemblyName);
    _ASSERTE(szFullTypeName);
    char* szAssembly = szFullTypeName;
    *pszAssemblyName = NULL;
    while(1) {
        szAssembly = strchr(szAssembly, ASSEMBLY_SEPARATOR_CHAR);
        if(szAssembly) {
            if (szAssembly - szFullTypeName >= MAX_CLASSNAME_LENGTH)
                return E_FAIL;

            for(; COMCharacter::nativeIsWhiteSpace(*szAssembly) ||
                    (*szAssembly == ASSEMBLY_SEPARATOR_CHAR); szAssembly++);
            if(*szAssembly != ']') {
                *pszAssemblyName = szAssembly;
                break;
            }
        }
        else {
            if (strlen(szFullTypeName) >= MAX_CLASSNAME_LENGTH)
                return E_FAIL;
            break;
        }
    }

    if(*pszAssemblyName != NULL) {
        char* ptr = (*pszAssemblyName)-1;
        for(; *ptr != ASSEMBLY_SEPARATOR_CHAR && ptr > szFullTypeName; ptr--);
        *(ptr) = NULL;
    }
    
    return S_OK;
}

