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
#include "common.h"
#include "ml.h"
#include "marshaler.h"
#include "mlinfo.h"
#include "ndirect.h"
#include "commember.h"
#include "sigformat.h"
#include "eeconfig.h"
#include "utilcode.h"
#include "eehash.h"
#include "../dlls/mscorrc/resource.h"



#include "version/__file__.ver"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
    static const int I_UINT32_MAX_DIGITS = 10; // Number of digits in 2^32
#endif

#ifndef lengthof
#define lengthof(rg)    (sizeof(rg)/sizeof(rg[0]))
#endif

#define INITIAL_NUM_CMHELPER_HASHTABLE_BUCKETS 32
#define INITIAL_NUM_CMINFO_HASHTABLE_BUCKETS 32



const BYTE MarshalInfo::m_localSizes[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) sizeof(mclass),
#include "mtypes.h"
};



UINT16 MarshalInfo::comSize(MarshalType mtype)
{
    static const BYTE comSizes[]=
    {
    #define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::c_comSize,
    #include "mtypes.h"
    };

    BYTE comSize = comSizes[mtype];

    if (comSize == VARIABLESIZE)
    {
        switch (mtype)
        {

            case MARSHAL_TYPE_BLITTABLEVALUECLASS:
            case MARSHAL_TYPE_VALUECLASS:
                return (UINT16) StackElemSize( m_pClass->GetAlignedNumInstanceFieldBytes() );
                break;

            case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
                return (UINT16) StackElemSize( m_pCMHelper->GetManagedSize() );

            default:
                _ASSERTE(0);
        }
    }

    return StackElemSize((UINT16)comSize);

}


UINT16 MarshalInfo::nativeSize(MarshalType mtype)
{
    static const BYTE nativeSizes[]=
    {
    #define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::c_nativeSize,
    #include "mtypes.h"
    };

    BYTE nativeSize = nativeSizes[mtype];

    if (nativeSize == VARIABLESIZE)
    {
        switch (mtype)
        {
            case MARSHAL_TYPE_BLITTABLEVALUECLASS:
            case MARSHAL_TYPE_VALUECLASS:
            case MARSHAL_TYPE_BLITTABLEVALUECLASSWITHCOPYCTOR:
                return (UINT16) StackElemSize( m_pClass->GetMethodTable()->GetNativeSize() );
                break;

            case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
                return (UINT16) StackElemSize( m_pCMHelper->GetNativeSize() );

            default:
                _ASSERTE(0);
        }
    }

    return MLParmSize((UINT16)nativeSize);
}



const BYTE MarshalInfo::m_flags[]=
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) \
    (mclass::c_fReturnsComByref ? MarshalerFlag_ReturnsComByref : 0) | \
    (mclass::c_fReturnsNativeByref ? MarshalerFlag_ReturnsNativeByref : 0) | \
    (mclass::c_fInOnly ? MarshalerFlag_InOnly : 0) ,

#include "mtypes.h"
};



typedef Marshaler::ArgumentMLOverrideStatus (*MLOVERRIDEPROC)(MLStubLinker *psl,
                                             MLStubLinker *pslPost,
                                             BOOL        byref,
                                             BOOL        fin,
                                             BOOL        fout,
                                             BOOL        comToNative,
                                             MLOverrideArgs *pargs,
                                             UINT       *pResID);

typedef Marshaler::ArgumentMLOverrideStatus (*RETURNMLOVERRIDEPROC)(MLStubLinker *psl,
                                             MLStubLinker *pslPost,
                                             BOOL        comToNative,
                                             BOOL        fThruBuffer,
                                             MLOverrideArgs *pargs,
                                             UINT       *pResID);

static const MLOVERRIDEPROC gArgumentMLOverride[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::ArgumentMLOverride,
#include "mtypes.h"
};

static const RETURNMLOVERRIDEPROC gReturnMLOverride[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) mclass::ReturnMLOverride,
#include "mtypes.h"
};


#define UNMARSHAL_NATIVE_TO_COM_NEEDED(c)               \
    (NEEDS_UNMARSHAL_NATIVE_TO_COM_IN(c)                \
     | (NEEDS_UNMARSHAL_NATIVE_TO_COM_OUT(c) << 1)      \
     | (NEEDS_UNMARSHAL_NATIVE_TO_COM_IN_OUT(c) << 2)   \
     | (NEEDS_UNMARSHAL_NATIVE_TO_COM_BYREF_IN(c) << 3) \
     | (1 << 4)                                         \
     | (1 << 5))

const BYTE MarshalInfo::m_unmarshalN2CNeeded[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) UNMARSHAL_NATIVE_TO_COM_NEEDED(mclass),
#include "mtypes.h"
};

#define UNMARSHAL_COM_TO_NATIVE_NEEDED(c)                \
    (NEEDS_UNMARSHAL_COM_TO_NATIVE_IN(c)                 \
     | (NEEDS_UNMARSHAL_COM_TO_NATIVE_OUT(c) << 1)       \
     | (NEEDS_UNMARSHAL_COM_TO_NATIVE_IN_OUT(c) << 2)    \
     | (NEEDS_UNMARSHAL_COM_TO_NATIVE_BYREF_IN(c) << 3)  \
     | (1 << 4)                                          \
     | (1 << 5))

const BYTE MarshalInfo::m_unmarshalC2NNeeded[] =
{
#define DEFINE_MARSHALER_TYPE(mtype, mclass) UNMARSHAL_COM_TO_NATIVE_NEEDED(mclass),
#include "mtypes.h"
};



//-------------------------------------------------------------------------------------
// Return the copy ctor for a VC class (if any exists)
//-------------------------------------------------------------------------------------
HRESULT FindCopyCtor(Module *pModule, MethodTable *pMT, MethodDesc **pMDOut)
{
    *pMDOut = NULL;

    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    HENUMInternal      hEnumMethod;
    HRESULT hr = pInternalImport->EnumGlobalFunctionsInit( &hEnumMethod );
    if (FAILED(hr)) {
        return hr;
    }

    mdMethodDef tk;
    mdTypeDef cl = pMT->GetClass()->GetCl();

    while (pInternalImport->EnumNext(&hEnumMethod, &tk)) {
        _ASSERTE(TypeFromToken(tk) == mdtMethodDef);
        DWORD dwMemberAttrs = pInternalImport->GetMethodDefProps(tk);
        if (IsMdSpecialName(dwMemberAttrs)) {
            ULONG cSig;
            PCCOR_SIGNATURE pSig;
            LPCSTR pName = pInternalImport->GetNameAndSigOfMethodDef(tk, &pSig, &cSig);     
            const char *pBaseName = ".__ctor";
            int ncBaseName = (int)strlen(pBaseName);
            int nc = (int)strlen(pName);
            if (nc >= ncBaseName && 0 == strcmp(pName + nc - ncBaseName, pBaseName)) {
                MetaSig msig(pSig, pModule);
                
                // Looking for the prototype   Ptr VC __ctor(Ptr VC, ByRef VC);
                if (msig.NumFixedArgs() == 2) {
                    if (msig.GetReturnType() == ELEMENT_TYPE_PTR) {
                        SigPointer spret = msig.GetReturnProps();
                        spret.GetElemType();
                        if (spret.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                            mdToken tk0 = spret.GetToken();
                            if (CompareTypeTokens(tk0, cl, pModule, pModule)) {
                                if (msig.NextArg() == ELEMENT_TYPE_PTR) {
                                    SigPointer sp1 = msig.GetArgProps();
                                    sp1.GetElemType();
///                                    if (sp1.GetElemType() == ELEMENT_TYPE_PTR) {
                                        if (sp1.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                                            mdToken tk1 = sp1.GetToken();
                                            if (tk1 == tk0 || CompareTypeTokens(tk1, cl, pModule, pModule)) {
                                                if (msig.NextArg() == ELEMENT_TYPE_PTR &&
                                                    msig.GetArgProps().HasCustomModifier(pModule, "Microsoft.VisualC.IsCXXReferenceModifier", ELEMENT_TYPE_CMOD_OPT)) {
                                                    SigPointer sp2 = msig.GetArgProps();
                                                    sp2.GetElemType();
                                                    if (sp2.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                                                        mdToken tk2 = sp2.GetToken();
                                                        if (tk2 == tk0 || CompareTypeTokens(tk2, cl, pModule, pModule)) {
                                                            *pMDOut = pModule->LookupMethodDef(tk);
                                                            break;                                 
                                                        }
                                                    }
                                                }
                                            }
////                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    pInternalImport->EnumClose(&hEnumMethod);
    return S_OK;
}







//-------------------------------------------------------------------------------------
// Return the destructor for a VC class (if any exists)
//-------------------------------------------------------------------------------------
HRESULT FindDtor(Module *pModule, MethodTable *pMT, MethodDesc **pMDOut)
{
    *pMDOut = NULL;

    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    HENUMInternal      hEnumMethod;
    HRESULT hr = pInternalImport->EnumGlobalFunctionsInit( &hEnumMethod );
    if (FAILED(hr)) {
        return hr;
    }

    mdMethodDef tk;
    mdTypeDef cl = pMT->GetClass()->GetCl();

    while (pInternalImport->EnumNext(&hEnumMethod, &tk)) {
        _ASSERTE(TypeFromToken(tk) == mdtMethodDef);
        ULONG cSig;
        PCCOR_SIGNATURE pSig;
        LPCSTR pName = pInternalImport->GetNameAndSigOfMethodDef(tk, &pSig, &cSig);     
        const char *pBaseName = ".__dtor";
        int ncBaseName = (int)strlen(pBaseName);
        int nc = (int)strlen(pName);
        if (nc >= ncBaseName && 0 == strcmp(pName + nc - ncBaseName, pBaseName)) {
            MetaSig msig(pSig, pModule);
            
            // Looking for the prototype   void __dtor(Ptr VC);
            if (msig.NumFixedArgs() == 1) {
                if (msig.GetReturnType() == ELEMENT_TYPE_VOID) {
                    if (msig.NextArg() == ELEMENT_TYPE_PTR) {
                        SigPointer sp1 = msig.GetArgProps();
                        sp1.GetElemType();
///                         if (sp1.GetElemType() == ELEMENT_TYPE_PTR) {
                            if (sp1.GetElemType() == ELEMENT_TYPE_VALUETYPE) {
                                mdToken tk1 = sp1.GetToken();
                                if (CompareTypeTokens(tk1, cl, pModule, pModule)) {
                                    *pMDOut = pModule->LookupMethodDef(tk);
                                    break;
                                }
                            }
///                         }
                    }
                }
            }
        }
    }
    pInternalImport->EnumClose(&hEnumMethod);
    return S_OK;
}




#ifdef _DEBUG
VOID MarshalInfo::DumpMarshalInfo(Module* pModule, SigPointer sig, mdToken token, MarshalScenario ms, BYTE nlType, BYTE nlFlags)
{
    if (LoggingOn(LF_MARSHALER, LL_INFO10))
    {
        char logbuf[3000];

        IMDInternalImport *pInternalImport = pModule->GetMDImport();

        strcpy(logbuf, "------------------------------------------------------------\n");
        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

        strcat(logbuf, "Managed type: ");

        if (m_byref) {
            strcat(logbuf, "Byref ");
        }

        SigFormat sigfmt;
        TypeHandle th;
        OBJECTREF throwable = NULL;
        GCPROTECT_BEGIN(throwable);
        th = sig.GetTypeHandle(pModule, &throwable);
        if (throwable != NULL)
        {
            strcat(logbuf, "<error>");
        }
        else
        {
            sigfmt.AddType(th);
            strcat(logbuf, sigfmt.GetCString());
        }
        GCPROTECT_END();

        strcat(logbuf, "\n");
        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

        strcat(logbuf, "NativeType  : ");
        PCCOR_SIGNATURE pvNativeType;
        ULONG           cbNativeType;
        if (token == mdParamDefNil
            || pInternalImport->GetFieldMarshal(token,
                                                 &pvNativeType,
                                                 &cbNativeType) != S_OK)
        {
            strcat(logbuf, "<absent>");
        }
        else
        {

            while (cbNativeType--)
            {
                char num[100];
                sprintf(num, "0x%lx ", (ULONG)*pvNativeType);
                strcat(logbuf, num);
                switch (*(pvNativeType++))
                {
#define XXXXX(nt) case nt: strcat(logbuf, "(" #nt ")"); break;

                    XXXXX(NATIVE_TYPE_BOOLEAN)     
                    XXXXX(NATIVE_TYPE_I1)          

                    XXXXX(NATIVE_TYPE_U1)
                    XXXXX(NATIVE_TYPE_I2)          
                    XXXXX(NATIVE_TYPE_U2)          
                    XXXXX(NATIVE_TYPE_I4)          

                    XXXXX(NATIVE_TYPE_U4)
                    XXXXX(NATIVE_TYPE_I8)          
                    XXXXX(NATIVE_TYPE_U8)          
                    XXXXX(NATIVE_TYPE_R4)          

                    XXXXX(NATIVE_TYPE_R8)

                    XXXXX(NATIVE_TYPE_LPSTR)
                    XXXXX(NATIVE_TYPE_LPWSTR)      
                    XXXXX(NATIVE_TYPE_LPTSTR)      
                    XXXXX(NATIVE_TYPE_FIXEDSYSSTRING)

                    XXXXX(NATIVE_TYPE_STRUCT)      

                    XXXXX(NATIVE_TYPE_INT)         
                    XXXXX(NATIVE_TYPE_FIXEDARRAY)

                    XXXXX(NATIVE_TYPE_UINT)

                    XXXXX(NATIVE_TYPE_FUNC)
                
                    XXXXX(NATIVE_TYPE_ASANY)

                    XXXXX(NATIVE_TYPE_ARRAY)
                    XXXXX(NATIVE_TYPE_LPSTRUCT)

                    XXXXX(NATIVE_TYPE_IUNKNOWN)


#undef XXXXX

                    
                    case NATIVE_TYPE_CUSTOMMARSHALER:
                    {
                        int strLen = 0;
                        int oldbuflen;
                        strcat(logbuf, "(NATIVE_TYPE_CUSTOMMARSHALER)");

                        // Skip the typelib guid.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;

                        // Skip the name of the native type.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;
        
                        // Extract the name of the custom marshaler.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;
        
                        // Extract the cookie string.
                        strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                        oldbuflen = (int)strlen(logbuf);
                        logbuf[oldbuflen] = ' ';
                        memcpyNoGCRefs(logbuf + oldbuflen + 1, pvNativeType, strLen);
                        logbuf[oldbuflen + 1 + strLen] = '\0';
                        pvNativeType += strLen;
                        cbNativeType -= strLen + 1;
                        break;
                    }

                    default:
                        strcat(logbuf, "(?)");
                }

                strcat(logbuf, "   ");
            }
        }
        strcat(logbuf, "\n");
        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

        strcat(logbuf, "MarshalType : ");
        {
            char num[100];
            sprintf(num, "0x%lx ", (ULONG)m_type);
            strcat(logbuf, num);
        }
        switch (m_type)
        {
#define DEFINE_MARSHALER_TYPE(mt, mc) case mt: strcat(logbuf, #mt " (" #mc ")"); break;
#include "mtypes.h"
#undef DEFINE_MARSHALER_TYPE
            case MARSHAL_TYPE_UNKNOWN:
                strcat(logbuf, "MARSHAL_TYPE_UNKNOWN (illegal combination)");
                break;
            default:
                strcat(logbuf, "MARSHAL_TYPE_???");
                break;
        }

        strcat(logbuf, "\n");


        strcat(logbuf, "Metadata In/Out     : ");
        if (TypeFromToken(token) != mdtParamDef || token == mdParamDefNil)
        {
            strcat(logbuf, "<absent>");
        }
        else
        {
            DWORD dwAttr = 0;
            USHORT usSequence;
            pInternalImport->GetParamDefProps(token, &usSequence, &dwAttr);
            if (IsPdIn(dwAttr))
            {
                strcat(logbuf, "In ");
            }
            if (IsPdOut(dwAttr))
            {
                strcat(logbuf, "Out ");
            }
        }

        strcat(logbuf, "\n");


        strcat(logbuf, "Effective In/Out     : ");
        if (m_in)
        {
            strcat(logbuf, "In ");
        }
        if (m_out)
        {
            strcat(logbuf, "Out ");
        }
        strcat(logbuf, "\n");


        LOG((LF_MARSHALER, LL_INFO10, logbuf));
        *logbuf = '\0';

    }
}
#endif




#define NATIVE_TYPE_DEFAULT NATIVE_TYPE_MAX


//==========================================================================
// Set's up the custom marshaler information.
//==========================================================================
CustomMarshalerHelper *SetupCustomMarshalerHelper(LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes, Assembly *pAssembly, TypeHandle hndManagedType)
{
    EEMarshalingData *pMarshalingData = NULL;

    // Retrieve the marshalling data for the current app domain.
    if (pAssembly->IsShared())
    {
        // If the assembly is shared, then it should only reference other shared assemblies.
        // This assumption MUST be true for the current custom marshaling scheme to work.
        // This implies that the type of the managed parameter must be a shared type.
        _ASSERTE(hndManagedType.GetAssembly()->IsShared());

        // The assembly is shared so we need to use the system domain's marshaling data.
        pMarshalingData = SystemDomain::System()->GetMarshalingData();
    }
    else
    {
        // The assembly is not shared so we use the current app domain's marshaling data.
        pMarshalingData = GetThread()->GetDomain()->GetMarshalingData();
    }

    // Retrieve the custom marshaler helper from the EE marshaling data.
    return pMarshalingData->GetCustomMarshalerHelper(pAssembly, hndManagedType, strMarshalerTypeName, cMarshalerTypeNameBytes, strCookie, cCookieStrBytes);
}


EEMarshalingData::EEMarshalingData(BaseDomain *pDomain, LoaderHeap *pHeap, Crst *pCrst)
: m_pHeap(pHeap)
, m_pDomain(pDomain)
{
    LockOwner lock = {pCrst, IsOwnerOfCrst};
    m_CMHelperHashtable.Init(INITIAL_NUM_CMHELPER_HASHTABLE_BUCKETS, &lock);
    m_SharedCMHelperToCMInfoMap.Init(INITIAL_NUM_CMINFO_HASHTABLE_BUCKETS, &lock);
}


EEMarshalingData::~EEMarshalingData()
{
    CustomMarshalerInfo *pCMInfo;


    // Walk through the linked list and delete all the custom marshaler info's.
    while ((pCMInfo = m_pCMInfoList.RemoveHead()) != NULL)
        delete pCMInfo;
}


void *EEMarshalingData::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(sizeof(EEMarshalingData));
}


void EEMarshalingData::operator delete(void *pMem)
{
    // Instances of this class are always allocated on the loader heap so
    // the delete operator has nothing to do.
}


CustomMarshalerHelper *EEMarshalingData::GetCustomMarshalerHelper(Assembly *pAssembly, TypeHandle hndManagedType, LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes)
{
    THROWSCOMPLUSEXCEPTION();

    CustomMarshalerHelper *pCMHelper = NULL;
    CustomMarshalerHelper *pNewCMHelper = NULL;
    CustomMarshalerInfo *pNewCMInfo = NULL;
    BOOL bSharedHelper = pAssembly->IsShared();
    TypeHandle hndCustomMarshalerType;
    OBJECTREF throwable = NULL;

    // Create the key that will be used to lookup in the hashtable.
    EECMHelperHashtableKey Key(cMarshalerTypeNameBytes, strMarshalerTypeName, cCookieStrBytes, strCookie, bSharedHelper);

    // Lookup the custom marshaler helper in the hashtable.
    if (m_CMHelperHashtable.GetValue(&Key, (HashDatum*)&pCMHelper))
        return pCMHelper;

    // Switch to cooperative GC mode if we are not already in cooperative.
    Thread *pThread = SetupThread();
    BOOL bToggleGC = !pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->DisablePreemptiveGC();

    GCPROTECT_BEGIN(throwable)
    {
        // Validate the arguments.
        _ASSERTE(strMarshalerTypeName && strCookie && !hndManagedType.IsNull());

        // Append a NULL terminator to the marshaler type name.
        CQuickArray<char> strCMMarshalerTypeName;
        strCMMarshalerTypeName.ReSize(cMarshalerTypeNameBytes + 1);
        memcpy(strCMMarshalerTypeName.Ptr(), strMarshalerTypeName, cMarshalerTypeNameBytes);
        strCMMarshalerTypeName[cMarshalerTypeNameBytes] = 0;

        // Load the custom marshaler class. 
        BOOL fNameIsAsmQualified = FALSE;
        hndCustomMarshalerType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strCMMarshalerTypeName.Ptr(), true, pAssembly, &fNameIsAsmQualified, &throwable);
        if (hndCustomMarshalerType.IsNull())
            COMPlusThrow(throwable);

        if (fNameIsAsmQualified)
        {
            // Set the assembly to null to indicate that the custom marshaler name is assembly
            // qualified.        
            pAssembly = NULL;
        }
    }
    GCPROTECT_END();


    if (bSharedHelper)
    {
        // Create the custom marshaler helper in the specified heap.
        pNewCMHelper = new (m_pHeap) SharedCustomMarshalerHelper(pAssembly, hndManagedType, strMarshalerTypeName, cMarshalerTypeNameBytes, strCookie, cCookieStrBytes);
    }
    else
    {
        // Create the custom marshaler info in the specified heap.
        pNewCMInfo = new (m_pHeap) CustomMarshalerInfo(m_pDomain, hndCustomMarshalerType, hndManagedType, strCookie, cCookieStrBytes);

        // Create the custom marshaler helper in the specified heap.
        pNewCMHelper = new (m_pHeap) NonSharedCustomMarshalerHelper(pNewCMInfo);
    }

    // Switch the GC mode back to the original mode.
    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    // Take the app domain lock before we insert the custom marshaler info into the hashtable.
    m_pDomain->EnterLock();

    // Verify that the custom marshaler helper has not already been added by another thread.
    if (m_CMHelperHashtable.GetValue(&Key, (HashDatum*)&pCMHelper))
    {
        m_pDomain->LeaveLock();
        if (pNewCMHelper)
            pNewCMHelper->Dispose();
        if (pNewCMInfo)
            delete pNewCMInfo;
        return pCMHelper;
    }

    // Add the custom marshaler helper to the hash table.
    m_CMHelperHashtable.InsertValue(&Key, pNewCMHelper, FALSE);

    // If we create the CM info, then add it to the linked list.
    if (pNewCMInfo)
        m_pCMInfoList.InsertHead(pNewCMInfo);

    // Release the lock and return the custom marshaler info.
    m_pDomain->LeaveLock();
    return pNewCMHelper;
}

CustomMarshalerInfo *EEMarshalingData::GetCustomMarshalerInfo(SharedCustomMarshalerHelper *pSharedCMHelper)
{
    THROWSCOMPLUSEXCEPTION();

    CustomMarshalerInfo *pCMInfo = NULL;
    CustomMarshalerInfo *pNewCMInfo = NULL;
    TypeHandle hndCustomMarshalerType;
    OBJECTREF throwable = NULL;

    // Lookup the custom marshaler helper in the hashtable.
    if (m_SharedCMHelperToCMInfoMap.GetValue(pSharedCMHelper, (HashDatum*)&pCMInfo))
        return pCMInfo;

    GCPROTECT_BEGIN(throwable)
    {
        // Append a NULL terminator to the marshaler type name.
        CQuickArray<char> strCMMarshalerTypeName;
        DWORD strLen = pSharedCMHelper->GetMarshalerTypeNameByteCount();
        strCMMarshalerTypeName.ReSize(pSharedCMHelper->GetMarshalerTypeNameByteCount() + 1);
        memcpy(strCMMarshalerTypeName.Ptr(), pSharedCMHelper->GetMarshalerTypeName(), strLen);
        strCMMarshalerTypeName[strLen] = 0;

        // Load the custom marshaler class. 
        hndCustomMarshalerType = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strCMMarshalerTypeName.Ptr(), true, pSharedCMHelper->GetAssembly(), NULL, &throwable);
        if (hndCustomMarshalerType.IsNull())
            COMPlusThrow(throwable);
    }
    GCPROTECT_END();

    // Create the custom marshaler info in the specified heap.
    pNewCMInfo = new (m_pHeap) CustomMarshalerInfo(m_pDomain, 
                                                   hndCustomMarshalerType, 
                                                   pSharedCMHelper->GetManagedType(), 
                                                   pSharedCMHelper->GetCookieString(), 
                                                   pSharedCMHelper->GetCookieStringByteCount());

    // Take the app domain lock before we insert the custom marshaler info into the hashtable.
    m_pDomain->EnterLock();

    // Verify that the custom marshaler info has not already been added by another thread.
    if (m_SharedCMHelperToCMInfoMap.GetValue(pSharedCMHelper, (HashDatum*)&pCMInfo))
    {
        m_pDomain->LeaveLock();
        delete pNewCMInfo;
        return pCMInfo;
    }

    // Add the custom marshaler helper to the hash table.
    m_SharedCMHelperToCMInfoMap.InsertValue(pSharedCMHelper, pNewCMInfo, FALSE);

    // Add the custom marshaler into the linked list.
    m_pCMInfoList.InsertHead(pNewCMInfo);

    // Release the lock and return the custom marshaler info.
    m_pDomain->LeaveLock();
    return pNewCMInfo;
}


//==========================================================================
// This structure contains the native type information for a given 
// parameter.
//==========================================================================
struct NativeTypeParamInfo
{
    NativeTypeParamInfo()
    : m_NativeType(NATIVE_TYPE_DEFAULT)
    , m_ArrayElementType(NATIVE_TYPE_DEFAULT)
    , m_SizeIsSpecified(FALSE)
    , m_CountParamIdx(0)
    , m_Multiplier(0)
    , m_Additive(1)
    , m_strCMMarshalerTypeName(NULL) 
    , m_cCMMarshalerTypeNameBytes(0)
    , m_strCMCookie(NULL)
    , m_cCMCookieStrBytes(0)
    {
    }   

    // The native type of the parameter.
    CorNativeType           m_NativeType;


    // for NT_ARRAY only
    CorNativeType           m_ArrayElementType; // The array element type.

    BOOL                    m_SizeIsSpecified;  // used to do some validation
    UINT16                  m_CountParamIdx;    // index of "sizeis" parameter
    UINT32                  m_Multiplier;       // multipler for "sizeis"
    UINT32                  m_Additive;         // additive for 'sizeis"

    // For NT_CUSTOMMARSHALER only.
    LPUTF8                  m_strCMMarshalerTypeName;
    DWORD                   m_cCMMarshalerTypeNameBytes;
    LPUTF8                  m_strCMCookie;
    DWORD                   m_cCMCookieStrBytes;
};


//==========================================================================
// Return: S_OK if there is valid data to compress
//         S_FALSE if at end of data block
//         E_FAIL if corrupt data found
//==========================================================================
HRESULT CheckForCompressedData(PCCOR_SIGNATURE pvNativeTypeStart, PCCOR_SIGNATURE pvNativeType, ULONG cbNativeType)
{
    if ( ((ULONG)(pvNativeType - pvNativeTypeStart)) == cbNativeType )
    {
        return S_FALSE;  //no more data
    }
    PCCOR_SIGNATURE pvProjectedEnd = pvNativeType + CorSigUncompressedDataSize(pvNativeType);
    if (pvProjectedEnd <= pvNativeType || ((ULONG)(pvProjectedEnd - pvNativeTypeStart)) > cbNativeType)
    {
        return E_FAIL; //corrupted data
    }
    return S_OK;
}

//==========================================================================
// Parse and validate the NATIVE_TYPE_ metadata.
// Note! NATIVE_TYPE_ metadata is optional. If it's not present, this
// routine sets NativeTypeParamInfo->m_NativeType to NATIVE_TYPE_DEFAULT. 
//==========================================================================
BOOL ParseNativeTypeInfo(mdToken                    token,
                         Module*                    pModule,
                         NativeTypeParamInfo*       pParamInfo
                         )
{
    PCCOR_SIGNATURE pvNativeType;
    ULONG           cbNativeType;
    HRESULT hr;

    if (token == mdParamDefNil || pModule->GetMDImport()->GetFieldMarshal(token, &pvNativeType, &cbNativeType) != S_OK)
    {
        return TRUE;
    }
    else
    {
        PCCOR_SIGNATURE pvNativeTypeStart = pvNativeType;

        if (cbNativeType == 0)
        {
            return FALSE;  // Zero-length NATIVE_TYPE block
        }

        pParamInfo->m_NativeType = (CorNativeType)*(pvNativeType++);

        int strLen = 0;

        // Retrieve any extra information associated with the native type.
        switch (pParamInfo->m_NativeType)
        {

            case NATIVE_TYPE_ARRAY:
                hr = CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                if (hr == S_OK)
                {
                    pParamInfo->m_ArrayElementType = (CorNativeType) (CorSigUncompressData(/*modifies*/pvNativeType));
                }

                // Check for "sizeis" param index
                hr = CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                if (hr == S_OK)
                {
                    pParamInfo->m_SizeIsSpecified = TRUE;
                    pParamInfo->m_CountParamIdx = (UINT16)(CorSigUncompressData(/*modifies*/pvNativeType));

                    // If an "sizeis" param index is present, the defaults for multiplier and additive change
                    pParamInfo->m_Multiplier = 1;
                    pParamInfo->m_Additive   = 0;

                    // Check for "sizeis" additive
                    hr = CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType);
                    if (FAILED(hr))
                    {
                        return FALSE;
                    }
                    if (hr == S_OK)
                    {
                        pParamInfo->m_Additive = CorSigUncompressData(/*modifies*/pvNativeType);
                    }    
                }

                break;

            case NATIVE_TYPE_CUSTOMMARSHALER:
                // Skip the typelib guid.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);

                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;

                pvNativeType += strLen;
                _ASSERTE((ULONG)(pvNativeType - pvNativeTypeStart) < cbNativeType);                

                // Skip the name of the native type.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;
                pvNativeType += strLen;
                _ASSERTE((ULONG)(pvNativeType - pvNativeTypeStart) < cbNativeType);

                // Extract the name of the custom marshaler.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;
                pParamInfo->m_strCMMarshalerTypeName = (LPUTF8)pvNativeType;
                pParamInfo->m_cCMMarshalerTypeNameBytes = strLen;
                pvNativeType += strLen;
                _ASSERTE((ULONG)(pvNativeType - pvNativeTypeStart) < cbNativeType);

                // Extract the cookie string.
                if (S_OK != CheckForCompressedData(pvNativeTypeStart, pvNativeType, cbNativeType))
                    return FALSE;
                strLen = CPackedLen::GetLength(pvNativeType, (void const **)&pvNativeType);
                if (pvNativeType + strLen < pvNativeType ||
                    pvNativeType + strLen > pvNativeTypeStart + cbNativeType)
                    return FALSE;
                pParamInfo->m_strCMCookie = (LPUTF8)pvNativeType;
                pParamInfo->m_cCMCookieStrBytes = strLen;
                _ASSERTE((ULONG)(pvNativeType + strLen - pvNativeTypeStart) == cbNativeType);
                break;
            default:
                break;
        }

        return TRUE;
    }
}



#ifdef _DEBUG
#define REDUNDANCYWARNING(when) if (when) LOG((LF_SLOP, LL_INFO100, "%s: Redundant nativetype metadata.\n", achDbgContext))
#else
#define REDUNDANCYWARNING(when)
#endif

//==========================================================================
// Constructs MarshalInfo. 
//==========================================================================
MarshalInfo::MarshalInfo(Module* pModule,
                         SigPointer sig,
                         mdToken token,
                         MarshalScenario ms,
                         BYTE nlType,
                         BYTE nlFlags,
                         BOOL isParam,
                         UINT paramidx   // parameter # for use in error messages (ignored if not parameter)

#ifdef CUSTOMER_CHECKED_BUILD
                         ,
                         MethodDesc* pMD
#endif
#ifdef _DEBUG
                         ,
                         LPCUTF8 pDebugName,
                         LPCUTF8 pDebugClassName,
                         LPCUTF8 pDebugNameSpace,
                         UINT    argidx  // 0 for return value, -1 for field
#endif
)
{

    m_paramidx = paramidx;
    m_resID    = IDS_EE_BADPINVOKE_GENERIC;  // if no one overwrites this with a better message, we'll still at least say something

    CorNativeType nativeType = NATIVE_TYPE_DEFAULT;
    HRESULT hr;
    NativeTypeParamInfo ParamInfo;
    Assembly *pAssembly = pModule->GetAssembly();

    BOOL fNeedsCopyCtor = FALSE;

#ifdef _DEBUG
    CHAR achDbgContext[2000] = "";
    if (!pDebugName)
    {
        strcpy(achDbgContext, "<Unknown>");
    }
    else
    {
        if (pDebugNameSpace)
        {
            strcpy(achDbgContext, pDebugNameSpace);
            strcat(achDbgContext, NAMESPACE_SEPARATOR_STR);
        }
        strcat(achDbgContext, pDebugClassName);
        strcat(achDbgContext, NAMESPACE_SEPARATOR_STR);
        strcat(achDbgContext, pDebugName);
        strcat(achDbgContext, " ");
        switch (argidx)
        {
            case -1:
                strcat(achDbgContext, "field");
                break;
            case 0:
                strcat(achDbgContext, "return value");
                break;
            default:
                {
                    char buf[30];
                    sprintf(buf, "param #%lu", (ULONG)argidx);
                    strcat(achDbgContext, buf);
                }
        }
    }

     m_strDebugMethName = pDebugName;
     m_strDebugClassName = pDebugClassName;
     m_strDebugNameSpace = pDebugNameSpace;
     m_iArg = argidx;
#else
    CHAR *achDbgContext = NULL;
#endif

#ifdef _DEBUG
    m_in = m_out = FALSE;
    m_byref = TRUE;
#endif

    CorElementType mtype = ELEMENT_TYPE_END;
    // Retrieve the native type for the current parameter.
    if (!ParseNativeTypeInfo(token, pModule, &ParamInfo))
    {
        IfFailGoto(E_FAIL, lFail);
    }
   
    nativeType = ParamInfo.m_NativeType;

    m_ms      = ms;
    m_nlType  = nlType;
    m_nlFlags = nlFlags;
    m_fAnsi   = (ms == MARSHAL_SCENARIO_NDIRECT) && (nlType == nltAnsi);

    m_comArgSize = 0;
    m_nativeArgSize = 0;
    m_pCMHelper = NULL;
    m_CMVt = VT_EMPTY;

    m_args.m_pMLInfo = this;

    mtype = (CorElementType) sig.Normalize(pModule);


    if (mtype == ELEMENT_TYPE_BYREF)
    {
        m_byref = TRUE;

        SigPointer sigtmp = sig;

        sig.GetElemType();
        mtype = (CorElementType) sig.Normalize(pModule);

        if (mtype == ELEMENT_TYPE_VALUETYPE) 
        {
            sigtmp.GetByte(); // Skip ET_BYREF;
            if (sigtmp.HasCustomModifier(pModule, "Microsoft.VisualC.NeedsCopyConstructorModifier", ELEMENT_TYPE_CMOD_REQD))
            {
                fNeedsCopyCtor = TRUE;
                m_byref = FALSE;
            }
        }
    
    }
    else
    {
        m_byref = FALSE;
    }


    if (mtype == ELEMENT_TYPE_PTR)
    {
        SigPointer sigtmp = sig;

        sigtmp.GetElemType();
        CorElementType mtype2 = (CorElementType) sigtmp.Normalize(pModule);

        if (mtype2 == ELEMENT_TYPE_VALUETYPE) 
        {
            EEClass *pClass = sigtmp.GetTypeHandle(pModule).GetClass();
            if (pClass && !pClass->IsBlittable())
            {
                m_resID = IDS_EE_BADPINVOKE_PTRNONBLITTABLE;
                IfFailGoto(E_FAIL, lFail);
            }
            if (sigtmp.HasCustomModifier(pModule, "Microsoft.VisualC.NeedsCopyConstructorModifier", ELEMENT_TYPE_CMOD_REQD))
            {
                sig.GetElemType();   // Skip ET_PTR
                mtype = (CorElementType) sig.Normalize(pModule);
                fNeedsCopyCtor = TRUE;
                m_byref = FALSE;
            }
        } else {

            if (!(mtype2 != ELEMENT_TYPE_CLASS &&
                  mtype2 != ELEMENT_TYPE_STRING &&
                  mtype2 != ELEMENT_TYPE_CLASS &&
                  mtype2 != ELEMENT_TYPE_OBJECT &&
                  mtype2 != ELEMENT_TYPE_SZARRAY))
            {
                m_resID = IDS_EE_BADPINVOKE_PTRSUBTYPE;
                IfFailGoto(E_FAIL, lFail);
            }
        }
    }


    if (m_byref && ParamInfo.m_SizeIsSpecified)
    {
        m_resID = IDS_EE_PINVOKE_NOREFFORSIZEIS;
        IfFailGoto(E_FAIL, lFail);
    }

    
    // Hack to get system primitive types (System.Int32, et.al.)
    // to marshal as expected. If those system prims were implemented as
    // enums, this wouldn't be necessary.
    if (mtype == ELEMENT_TYPE_VALUETYPE)
    {
        BEGIN_ENSURE_COOPERATIVE_GC()
        {
            OBJECTREF pException = NULL;
            GCPROTECT_BEGIN(pException)
            {    
                TypeHandle thnd = sig.GetTypeHandle(pModule, &pException);
                if (!(thnd.IsNull()))
                {  
                    CorElementType realmtype = thnd.GetNormCorElementType();

                    if (CorTypeInfo::IsPrimitiveType(realmtype))
                        mtype = realmtype;
                }
            }
            GCPROTECT_END();
        }
        END_ENSURE_COOPERATIVE_GC();
    }

    // Handle the custom marshaler native type seperately.
    if (nativeType == NATIVE_TYPE_CUSTOMMARSHALER)
    {

        switch (mtype)
        {
            case ELEMENT_TYPE_VAR:
            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_OBJECT:
                m_CMVt = VT_UNKNOWN;
                break;

            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_SZARRAY:
            case ELEMENT_TYPE_ARRAY:
                m_CMVt = VT_I4;
                break;

            default:    
                m_resID = IDS_EE_BADPINVOKE_CUSTOMMARSHALER;
                IfFailGoto(E_FAIL, lFail);
        }

        // Set up the custom marshaler info.
        m_type = MARSHAL_TYPE_UNKNOWN; // in case SetupCustomMarshalerHelper throws
        m_pCMHelper = SetupCustomMarshalerHelper(ParamInfo.m_strCMMarshalerTypeName, 
                                                 ParamInfo.m_cCMMarshalerTypeNameBytes,
                                                 ParamInfo.m_strCMCookie, 
                                                 ParamInfo.m_cCMCookieStrBytes,
                                                 pAssembly,
                                                 sig.GetTypeHandle(pModule));

        // Determine which custom marshaler to use.
        m_type = m_pCMHelper->IsDataByValue() ? MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER : 
                                              MARSHAL_TYPE_REFERENCECUSTOMMARSHALER;
        goto lExit;
    }

    m_type = MARSHAL_TYPE_UNKNOWN; // flag for uninitialized type
    switch (mtype)
    {
        case ELEMENT_TYPE_BOOLEAN:
            switch (nativeType)
            {
                case NATIVE_TYPE_BOOLEAN:
                    REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT);
                    m_type = MARSHAL_TYPE_WINBOOL;
                    break;


                case NATIVE_TYPE_U1:
                case NATIVE_TYPE_I1:
                    m_type = MARSHAL_TYPE_CBOOL;
                    break;

                case NATIVE_TYPE_DEFAULT:
                    {
                        m_type = MARSHAL_TYPE_WINBOOL;
                    }
                    break;

                default:
                    m_resID = IDS_EE_BADPINVOKE_BOOLEAN;
                    IfFailGoto(E_FAIL, lFail);
            }
            break;

        case ELEMENT_TYPE_CHAR:
            switch (nativeType)
            {
                case NATIVE_TYPE_I1: //fallthru
                case NATIVE_TYPE_U1:
                    REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi);
                    m_type = MARSHAL_TYPE_ANSICHAR;
                    break;

                case NATIVE_TYPE_I2: //fallthru
                case NATIVE_TYPE_U2:
                    REDUNDANCYWARNING(!(m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi));
                    m_type = MARSHAL_TYPE_GENERIC_U2;
                    break;

                case NATIVE_TYPE_DEFAULT:
                    m_type = ( (m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi) ? MARSHAL_TYPE_ANSICHAR : MARSHAL_TYPE_GENERIC_U2 );
                    break;

                default:
                    m_resID = IDS_EE_BADPINVOKE_CHAR;
                    IfFailGoto(E_FAIL, lFail);

            }
            break;

        case ELEMENT_TYPE_I1:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_I1 || nativeType == NATIVE_TYPE_U1 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I1;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_1;
            break;

        case ELEMENT_TYPE_U1:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_U1 || nativeType == NATIVE_TYPE_I1 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I1;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_U1;
            break;

        case ELEMENT_TYPE_I2:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_I2 || nativeType == NATIVE_TYPE_U2 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I2;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_2;
            break;

        case ELEMENT_TYPE_U2:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_U2 || nativeType == NATIVE_TYPE_I2 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I2;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_U2;
            break;

        case ELEMENT_TYPE_I4:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);

            switch (nativeType)
            {
                case NATIVE_TYPE_I4:
                case NATIVE_TYPE_U4:
                case NATIVE_TYPE_DEFAULT:
                    break;


                default:
                m_resID = IDS_EE_BADPINVOKE_I4;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_4;
            break;

        case ELEMENT_TYPE_U4:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);

            switch (nativeType)
            {
                case NATIVE_TYPE_I4:
                case NATIVE_TYPE_U4:
                case NATIVE_TYPE_DEFAULT:
                    break;


                default:
                m_resID = IDS_EE_BADPINVOKE_I4;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_4;
            break;

        case ELEMENT_TYPE_I8:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_I8 || nativeType == NATIVE_TYPE_U8 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I8;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_8;
            break;

        case ELEMENT_TYPE_U8:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_U8 || nativeType == NATIVE_TYPE_I8 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I8;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_GENERIC_8;
            break;

        case ELEMENT_TYPE_I:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_INT || nativeType == NATIVE_TYPE_UINT || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = (sizeof(LPVOID) == 4 ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8);
            break;

        case ELEMENT_TYPE_U:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_UINT || nativeType == NATIVE_TYPE_INT || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_I;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = (sizeof(LPVOID) == 4 ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8);
            break;


        case ELEMENT_TYPE_R4:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_R4 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_R4;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_FLOAT;
            break;

        case ELEMENT_TYPE_R8:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_R8 || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_R8;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_DOUBLE;
            break;

        case ELEMENT_TYPE_R:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (nativeType != NATIVE_TYPE_DEFAULT)
            {
                m_resID = IDS_EE_BADPINVOKE_R;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = MARSHAL_TYPE_DOUBLE;
            break;

        case ELEMENT_TYPE_PTR:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (nativeType != NATIVE_TYPE_DEFAULT)
            {
                m_resID = IDS_EE_BADPINVOKE_PTR;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = ( (sizeof(void*)==4) ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8 );
            break;

        case ELEMENT_TYPE_FNPTR:
            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
            if (!(nativeType == NATIVE_TYPE_FUNC || nativeType == NATIVE_TYPE_DEFAULT))
            {
                m_resID = IDS_EE_BADPINVOKE_FNPTR;
                IfFailGoto(E_FAIL, lFail);
            }
            m_type = ( (sizeof(void*)==4) ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8 );
            break;

        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VAR:
            {                

                m_pClass = sig.GetTypeHandle(pModule).GetClass();
                if (m_pClass == NULL)
                {
                    IfFailGoto(COR_E_TYPELOAD, lFail);
                }

                {
                    bool builder = false;
                    if (sig.IsStringType(pModule)
                        || ((builder = true), 0)
                        || sig.IsClass(pModule, g_StringBufferClassName)
                        || GetAppDomain()->IsSpecialStringClass(m_pClass->GetMethodTable())
                        || GetAppDomain()->IsSpecialStringBuilderClass(m_pClass->GetMethodTable())
                        )
                    {
                        StringType      stype = enum_WSTR;

                        REDUNDANCYWARNING( (m_ms == MARSHAL_SCENARIO_NDIRECT && m_fAnsi && nativeType == NATIVE_TYPE_LPSTR) ||
                                           (m_ms == MARSHAL_SCENARIO_NDIRECT && !m_fAnsi && nativeType == NATIVE_TYPE_LPWSTR) );

                        switch ( nativeType )
                        {
                            case NATIVE_TYPE_LPWSTR:
                                stype = enum_WSTR;
                                break;

                            case NATIVE_TYPE_LPSTR:
                                stype = enum_CSTR;
                                break;

                            case NATIVE_TYPE_LPTSTR:
                                {
                                    {
                                        static BOOL init = FALSE;
                                        static BOOL onUnicode;
        
                                        if (!init)
                                        {
                                            onUnicode = NDirectOnUnicodeSystem();
                                            init = TRUE;
                                        }
        
                                        if (onUnicode)
                                            stype = enum_WSTR;
                                        else
                                            stype = enum_CSTR;
                                    }

                                    break;
                                }


                            case NATIVE_TYPE_DEFAULT:
                                {
                                    {
                                        stype = m_fAnsi ? enum_CSTR : enum_WSTR;
                                    }
                                    break;
                                }
        
                            default:
                                m_resID = builder ? IDS_EE_BADPINVOKE_STRINGBUILDER : IDS_EE_BADPINVOKE_STRING;
                                IfFailGoto(E_FAIL, lFail);
                                break;
                        }
        
                        {
                            _ASSERTE(MARSHAL_TYPE_LPWSTR + enum_WSTR == (int) MARSHAL_TYPE_LPWSTR);
                            _ASSERTE(MARSHAL_TYPE_LPWSTR + enum_CSTR == (int) MARSHAL_TYPE_LPSTR);
                            _ASSERTE(MARSHAL_TYPE_LPWSTR_BUFFER + enum_WSTR == (int) MARSHAL_TYPE_LPWSTR_BUFFER);
                            _ASSERTE(MARSHAL_TYPE_LPWSTR_BUFFER + enum_CSTR == (int) MARSHAL_TYPE_LPSTR_BUFFER);

                            if (GetAppDomain()->IsSpecialStringBuilderClass(m_pClass->GetMethodTable()))
                                m_type = (MarshalType) (MARSHAL_TYPE_LPWSTR_BUFFER_X + stype);
                            else 
                            if (GetAppDomain()->IsSpecialStringClass(m_pClass->GetMethodTable()))
                                m_type = (MarshalType) (MARSHAL_TYPE_LPWSTR_X + stype);
                            else 
                            if (builder)
                                m_type = (MarshalType) (MARSHAL_TYPE_LPWSTR_BUFFER + stype);
                            else
                                m_type = (MarshalType) (MARSHAL_TYPE_LPWSTR + stype);
           
                            //if (m_byref) { m_type = MARSHAL_TYPE_VBBYVALSTR;  } 
                        }
                    }
                    else if (COMDelegate::IsDelegate(m_pClass))
                    {
                        switch (nativeType)
                        {
                            case NATIVE_TYPE_FUNC:
                                REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT);
                                m_type = MARSHAL_TYPE_DELEGATE;
                                break;

                            case NATIVE_TYPE_DEFAULT:
                                {
                                    m_type = MARSHAL_TYPE_DELEGATE;
                                }
                                break;

                            default:
                                m_resID = IDS_EE_BADPINVOKE_DELEGATE;
                                IfFailGoto(E_FAIL, lFail);
                                break;
                        }
                    }
                    else if (m_pClass->IsBlittable())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
    
                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_LPSTRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_CLASS;
                            IfFailGoto(E_FAIL, lFail);
                        }
                        m_type = MARSHAL_TYPE_BLITTABLEPTR;
                    }
                    else if (m_pClass->HasLayout())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);

                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_LPSTRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_CLASS;
                            IfFailGoto(E_FAIL, lFail);
                        }
                        m_type = MARSHAL_TYPE_LAYOUTCLASSPTR;
                    }    
    
                    else if (m_pClass->IsObjectClass()
                             || GetAppDomain()->IsSpecialObjectClass(m_pClass->GetMethodTable()))
                    {
                        switch(nativeType)
                        {
                            case NATIVE_TYPE_IUNKNOWN:
                                m_type = MARSHAL_TYPE_INTERFACE;
                                break;

                            case NATIVE_TYPE_ASANY:
                                m_type = m_fAnsi ? MARSHAL_TYPE_ASANYA : MARSHAL_TYPE_ASANYW;
                                break;

                            default:
                                m_resID = IDS_EE_BADPINVOKE_OBJECT;
                                IfFailGoto(E_FAIL, lFail);
                        }
                    }


                    else if (!m_pClass->IsValueClass())
                    {
                        m_resID = IDS_EE_BADPINVOKE_NOLAYOUT;
                        IfFailGoto(E_FAIL, lFail);
                    }

                    else
                    {
                        _ASSERTE(m_pClass->IsValueClass());
                        goto lValueClass;
                    }
                }
            }
            break;
    
        case ELEMENT_TYPE_VALUETYPE:
          lValueClass:
            {

                if (sig.IsClass(pModule, g_DecimalClassName))
                {
                    switch (nativeType)
                    {
                        case NATIVE_TYPE_DEFAULT:
                        case NATIVE_TYPE_STRUCT:
                            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                            m_type = MARSHAL_TYPE_DECIMAL;
                            break;

                        case NATIVE_TYPE_LPSTRUCT:
                            m_type = MARSHAL_TYPE_DECIMAL_PTR;
                            break;

                        case NATIVE_TYPE_CURRENCY:
                            m_type = MARSHAL_TYPE_CURRENCY;
                            break;

                        default:
                            m_resID = IDS_EE_BADPINVOKE_DECIMAL;
                            IfFailGoto(E_FAIL, lFail);
                    }
                }
                else if (sig.IsClass(pModule, g_GuidClassName))
                {
                    switch (nativeType)
                    {
                        case NATIVE_TYPE_DEFAULT:
                        case NATIVE_TYPE_STRUCT:
                            REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                            m_type = MARSHAL_TYPE_GUID;
                            break;

                        case NATIVE_TYPE_LPSTRUCT:
                            m_type = MARSHAL_TYPE_GUID_PTR;
                            break;

                        default:
                            m_resID = IDS_EE_BADPINVOKE_GUID;
                            IfFailGoto(E_FAIL, lFail);
                    }
                }
                else if (sig.IsClass(pModule, g_DateClassName))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_STRUCT))
                    {
                        m_resID = IDS_EE_BADPINVOKE_DATETIME;
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_DATE;
                }
                else if (sig.IsClass(pModule, "System.Runtime.InteropServices.ArrayWithOffset"))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT))
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_ARRAYWITHOFFSET;
                }
                else if (sig.IsClass(pModule, "System.Runtime.InteropServices.HandleRef"))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT))
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_HANDLEREF;
                }
                else if (sig.IsClass(pModule, "System.ArgIterator"))
                {
                    REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                    if (!(nativeType == NATIVE_TYPE_DEFAULT))
                    {
                        IfFailGoto(E_FAIL, lFail);
                    }
                    m_type = MARSHAL_TYPE_ARGITERATOR;
                }
                else
                {
                    m_pClass = sig.GetTypeHandle(pModule).GetClass();
                    if (m_pClass == NULL)
                        break;

                    if (m_pClass->GetMethodTable()->GetNativeSize() > 0xfff0 ||
                        m_pClass->GetAlignedNumInstanceFieldBytes() > 0xfff0)
                    {
                        m_resID = IDS_EE_STRUCTTOOCOMPLEX;
                        IfFailGoto(E_FAIL, lFail);
                    }

    
                    if (m_pClass->IsBlittable())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_STRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_VALUETYPE;
                            IfFailGoto(E_FAIL, lFail);
                        }

                        if (m_byref && !isParam)
                        {
                            // Override the prohibition on byref returns so that IJW works
                            m_byref = FALSE;
                            m_type = ( (sizeof(void*)==4) ? MARSHAL_TYPE_GENERIC_4 : MARSHAL_TYPE_GENERIC_8 );
                        }
                        else
                        {
                            if (fNeedsCopyCtor)
                            {
                                MethodDesc *pCopyCtor;
                                MethodDesc *pDtor;
                                HRESULT hr = FindCopyCtor(pModule, m_pClass->GetMethodTable(), &pCopyCtor);
                                IfFailGoto(hr, lFail);
                                hr = FindDtor(pModule, m_pClass->GetMethodTable(), &pDtor);
                                IfFailGoto(hr, lFail);
    
                                m_args.mm.m_pMT = m_pClass->GetMethodTable();
                                m_args.mm.m_pCopyCtor = pCopyCtor;
                                m_args.mm.m_pDtor = pDtor;
                                m_type = MARSHAL_TYPE_BLITTABLEVALUECLASSWITHCOPYCTOR;
                            }
                            else
                            {
                                m_args.m_pMT = m_pClass->GetMethodTable();
                                m_type = MARSHAL_TYPE_BLITTABLEVALUECLASS;
                            }
                        }
                    }
                    else if (m_pClass->HasLayout())
                    {
                        REDUNDANCYWARNING(nativeType != NATIVE_TYPE_DEFAULT);
                        if (!(nativeType == NATIVE_TYPE_DEFAULT || nativeType == NATIVE_TYPE_STRUCT))
                        {
                            m_resID = IDS_EE_BADPINVOKE_VALUETYPE;
                            IfFailGoto(E_FAIL, lFail);
                        }

                        m_args.m_pMT = m_pClass->GetMethodTable();
                        m_type = MARSHAL_TYPE_VALUECLASS;
                    }
    
                }
                break;
            }
    
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
            {
                // Get class info from array.
                TypeHandle arrayTypeHnd = sig.GetTypeHandle(pModule);
                if (arrayTypeHnd.IsNull())
                    IfFailGoto(COR_E_TYPELOAD, lFail);

                ArrayTypeDesc* asArray = arrayTypeHnd.AsArray();
                if (asArray == NULL)
                    IfFailGoto(E_FAIL, lFail);

                TypeHandle elemTypeHnd = asArray->GetTypeParam();

                unsigned ofs = 0;
                if (arrayTypeHnd.GetMethodTable())
                {
                    ofs = ArrayBase::GetDataPtrOffset(arrayTypeHnd.GetMethodTable());
                    if (ofs > 0xffff)
                    {
                        ofs = 0;   // can't represent it, so pass on magic value (which causes fallback to regular ML code)
                    }
                }

                // Handle retrieving the information for the array type.
                IfFailGoto(HandleArrayElemType(achDbgContext, &ParamInfo, (UINT16)ofs, elemTypeHnd, asArray->GetRank(), mtype == ELEMENT_TYPE_SZARRAY, isParam, FALSE, pAssembly), lFail);
            }
            break;
    
    
        default:
            m_resID = IDS_EE_BADPINVOKE_BADMANAGED;
            //     _ASSERTE(!"Unsupported type!");
    }

  lExit:

    if (m_byref && !isParam)
    {
        // byref returns don't work: the thing pointed to lives on
        // a stack that disappears!
        m_type = MARSHAL_TYPE_UNKNOWN;
        goto lReallyExit;
    }
    

    //---------------------------------------------------------------------
    // Now, figure out the IN/OUT status.
    //---------------------------------------------------------------------
    if (m_type != MARSHAL_TYPE_UNKNOWN && (m_flags[m_type] & MarshalerFlag_InOnly) && !m_byref)
    {
        // If we got here, the parameter is something like an "int" where
        // [in] is the only semantically valid choice. Since there is no
        // possible way to interpret an [out] for such a type, we will ignore
        // the metadata and force the bits to "in". We could have defined
        // it as an error instead but this is less likely to cause problems
        // with metadata autogenerated from typelibs and poorly
        // defined C headers.
        // 
        m_in = TRUE;
        m_out = FALSE;
    }
    else
    {

        // Capture and save away "In/Out" bits. If none is present, set both to FALSE (they will be properly defaulted downstream)
        if (TypeFromToken(token) != mdtParamDef || token == mdParamDefNil)
        {
            m_in = FALSE;
            m_out = FALSE;
        }
        else
        {
            IMDInternalImport *pInternalImport = pModule->GetMDImport();
            USHORT             usSequence;
            DWORD              dwAttr;
        
            pInternalImport->GetParamDefProps(token, &usSequence, &dwAttr);
            m_in = IsPdIn(dwAttr) != 0;
            m_out = IsPdOut(dwAttr) != 0;
        }
        
        // If neither IN nor OUT are true, this signals the URT to use the default
        // rules.
        if (!m_in && !m_out)
        {
            if (m_byref || (mtype == ELEMENT_TYPE_CLASS && !(sig.IsStringType(pModule)) && sig.IsClass(pModule, g_StringBufferClassName)))
            {
                m_in = TRUE;
                m_out = TRUE;
            }
            else
            {
                m_in = TRUE;
                m_out = FALSE;
            }
        
        }
    }

#ifdef CUSTOMER_CHECKED_BUILD
    if (pMD != NULL)
        OutputCustomerCheckedBuildMarshalInfo(pMD, sig, pModule);
#endif

lReallyExit:

#ifdef _DEBUG
    DumpMarshalInfo(pModule, sig, token, ms, nlType, nlFlags); 
#endif
    return;


  lFail:
    // We got here because of an illegal ELEMENT_TYPE/NATIVE_TYPE combo.
    m_type = MARSHAL_TYPE_UNKNOWN;
    //_ASSERTE(!"Invalid ELEMENT_TYPE/NATIVE_TYPE combination");
    goto lExit;
}

HRESULT MarshalInfo::HandleArrayElemType(char *achDbgContext, NativeTypeParamInfo *pParamInfo, UINT16 optbaseoffset,  TypeHandle elemTypeHnd, int iRank, BOOL fNoLowerBounds, BOOL isParam, BOOL isSysArray, Assembly *pAssembly)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;

    _ASSERTE(pParamInfo && !elemTypeHnd.IsNull());


    //
    // Store the element type handle and determine the element type from the type handle.
    //

    m_hndArrayElemType = elemTypeHnd;
    m_iArrayRank = iRank;
    CorElementType elemType = elemTypeHnd.GetNormCorElementType();
    m_nolowerbounds = fNoLowerBounds;


    //
    // The marshaling code doesn't deal with nested array's.
    //

    if (elemTypeHnd.IsArray())
    {
        m_resID = IDS_EE_BADPINVOKE_ARRAY;
        IfFailGo(E_FAIL);
    }


    //
    // Determine which type of marshaler to use.
    //

    if (pParamInfo->m_NativeType == NATIVE_TYPE_ARRAY)
    {
        REDUNDANCYWARNING(m_ms == MARSHAL_SCENARIO_NDIRECT);
        m_type = MARSHAL_TYPE_NATIVEARRAY;
    }
    else if (pParamInfo->m_NativeType == NATIVE_TYPE_DEFAULT)
    {
        {
            m_type = MARSHAL_TYPE_NATIVEARRAY;
        }
    }
    else
    {
        m_resID = IDS_EE_BADPINVOKE_ARRAY;
        IfFailGo(E_FAIL);
    }


    //
    // Determine the VARTYPE of the elements in the array.
    //

    {
        if (elemType <= ELEMENT_TYPE_R8)
        {
            static const BYTE map [] =
            {
                VT_NULL,    // ELEMENT_TYPE_END
                VT_NULL,    // ELEMENT_TYPE_VOID
                VT_BOOL,    // ELEMENT_TYPE_BOOLEAN
                VT_UI2,     // ELEMENT_TYPE_CHAR
                VT_I1,      // ELEMENT_TYPE_I1
                VT_UI1,     // ELEMENT_TYPE_U1
                VT_I2,      // ELEMENT_TYPE_I2
                VT_UI2,     // ELEMENT_TYPE_U2
                VT_I4,      // ELEMENT_TYPE_I4
                VT_UI4,     // ELEMENT_TYPE_U4
                VT_I8,      // ELEMENT_TYPE_I8
                VT_UI8,     // ELEMENT_TYPE_U8
                VT_R4,      // ELEMENT_TYPE_R4
                VT_R8       // ELEMENT_TYPE_R8

            };
            m_arrayElementType = map[elemType];

            {
                if (m_arrayElementType == VT_BOOL)
                {
                    m_arrayElementType = VTHACK_WINBOOL;
                }
                else if (elemType == ELEMENT_TYPE_CHAR && m_fAnsi)
                {
                    m_arrayElementType = VTHACK_ANSICHAR;

                    if (!isParam)
                    {
                        // This char[]->ansichar[] thing can only work as a
                        // parameter (and in the managed->unmanaged direction at that.)
                        // This puppy really is a ill-defined pairing and we should
                        // try to shoot it if we can.
                        IfFailGo(E_FAIL);
                    }
                }
            }
        }
        else if (elemType == ELEMENT_TYPE_I)
        {
            m_arrayElementType = (sizeof(LPVOID) == 4) ? VT_I4 : VT_I8;
        }
        else if (elemType == ELEMENT_TYPE_U)
        {
            m_arrayElementType = (sizeof(LPVOID) == 4) ? VT_UI4 : VT_UI8;
        }
        else
        {
            if (elemTypeHnd == TypeHandle(g_pStringClass))
            {           
                switch (pParamInfo->m_ArrayElementType)
                {
                    case NATIVE_TYPE_DEFAULT:
                            m_arrayElementType = m_fAnsi ? VT_LPSTR : VT_LPWSTR;
                        break;
                    case NATIVE_TYPE_LPSTR:
                        m_arrayElementType = VT_LPSTR;
                        break;
                    case NATIVE_TYPE_LPWSTR:
                        m_arrayElementType = VT_LPWSTR;
                        break;
                    case NATIVE_TYPE_LPTSTR:
                        {
                            {
                                static BOOL init = FALSE;
                                static BOOL onUnicode;
                    
                                if (!init)
                                {
                                    onUnicode = NDirectOnUnicodeSystem();
                                    init = TRUE;
                                }
                    
                                if (onUnicode)
                                    m_arrayElementType = VT_LPWSTR;
                                else
                                    m_arrayElementType = VT_LPSTR;
                            }
                        }
                        break;
                    default:
                        m_arrayElementType = VT_NULL;
                }
            }
            else if (elemTypeHnd == TypeHandle(g_pObjectClass))
            {
                switch(pParamInfo->m_ArrayElementType)
                {
                    case NATIVE_TYPE_IUNKNOWN:
                        m_arrayElementType = VT_UNKNOWN;
                        break;

                    default:
                        IfFailGo(E_FAIL);
                }
            }
            else if (elemType == ELEMENT_TYPE_VALUETYPE) 
            {
                m_arrayElementType = OleVariant::GetVarTypeForTypeHandle(elemTypeHnd);
            }
            else
            {
                IfFailGo(E_FAIL);
            }
        }
    }


    //
    // Do any extra work required by the array type.
    //

    {
        // Retrieve the extra information associated with the native array marshaling.
        m_args.na.m_vt  = m_arrayElementType;
        m_args.na.m_pMT = elemTypeHnd.IsUnsharedMT() ? elemTypeHnd.AsMethodTable() : NULL;   
        m_args.na.m_optionalbaseoffset = optbaseoffset;
        m_countParamIdx = pParamInfo->m_CountParamIdx;
        m_multiplier    = pParamInfo->m_Multiplier;
        m_additive      = pParamInfo->m_Additive;
    }


ErrExit:
    return hr;
}



VOID ThrowInteropParamException(UINT resID, UINT paramIdx)
{
    THROWSCOMPLUSEXCEPTION();

    if (paramIdx == 0)
    {
        COMPlusThrow(kMarshalDirectiveException, resID, L"return value");
    }
    else
    {
        WCHAR buf[50];
        Wszwsprintf(buf, L"parameter #%u", paramIdx);

        COMPlusThrow(kMarshalDirectiveException, resID, buf);
    }
}


VOID MarshalInfo::EmitOrThrowInteropParamException(MLStubLinker *psl, UINT resID, UINT paramIdx)
{

    ThrowInteropParamException(resID, paramIdx);

}

void MarshalInfo::GenerateArgumentML(MLStubLinker *psl,
                                       MLStubLinker *pslPost,
                                       BOOL comToNative)
{

    THROWSCOMPLUSEXCEPTION();

    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
        EmitOrThrowInteropParamException(psl, m_resID, m_paramidx);
        return;
    }

    //
    // Use simple copy opcodes if possible.
    //

    if (!m_byref && m_type <= MARSHAL_TYPE_DOUBLE)
    {
        UINT16 size = 0;
        int opcode = 0;

        switch (m_type)
        {
        case MARSHAL_TYPE_GENERIC_1:
            if (!comToNative)
            {
                opcode = ML_COPYI1;
                size = 1;
                break;
            }
            // fall through

        case MARSHAL_TYPE_GENERIC_U1:
            if (!comToNative)
            {
                opcode = ML_COPYU1;
                size = 1;
                break;
            }
            // fall through
        case MARSHAL_TYPE_GENERIC_2:
            if (!comToNative)
            {
                opcode = ML_COPYI2;
                size = 2;
                break;
            }
            // fall through
        case MARSHAL_TYPE_GENERIC_U2:
            if (!comToNative)
            {
                opcode = ML_COPYU2;
                size = 2;
                break;
            }
            // fall through
        case MARSHAL_TYPE_GENERIC_4:
            opcode = ML_COPY4;
            size = 4;
            break;

        case MARSHAL_TYPE_GENERIC_8:
            opcode = ML_COPY8;
            size = 8;
            break;

        case MARSHAL_TYPE_FLOAT:
#ifdef _PPC_
            opcode = comToNative ? ML_COPYR4_C2N : ML_COPYR4_N2C;
#else // _PPC_
            opcode = ML_COPY4;
#endif // _PPC_
            size = 4;
            break;

        case MARSHAL_TYPE_DOUBLE:
#ifdef _PPC_
            opcode = comToNative ? ML_COPYR8_C2N : ML_COPYR8_N2C;
#else // _PPC_
            opcode = ML_COPY8;
#endif // _PPC_
            size = 8;
            break;

        case MARSHAL_TYPE_CBOOL:
            opcode = comToNative ? ML_CBOOL_C2N : ML_CBOOL_N2C;
            size = 1;
            break;

        default:
            break;
        }


        if (size != 0)
        {
            if (psl)
            {
                {
            psl->MLEmit(opcode);
                }
            }

            m_comArgSize = StackElemSize(size);
            m_nativeArgSize = MLParmSize(size);
            return;
        }
    }



    if (m_byref)
    {
        m_comArgSize = StackElemSize(sizeof(void*));
        m_nativeArgSize = MLParmSize(sizeof(void*));
    }
    else
    {
        m_comArgSize = StackElemSize(comSize(m_type));
        m_nativeArgSize = MLParmSize(nativeSize(m_type));
    }

    if (! psl)
        return;

    Marshaler::ArgumentMLOverrideStatus amostat;
    UINT resID = IDS_EE_BADPINVOKE_RESTRICTION;
    amostat = (gArgumentMLOverride[m_type]) (psl,
                                             pslPost,
                                             m_byref,
                                             m_in,
                                             m_out,
                                             comToNative,
                                             &m_args,
                                             &resID);
    
    if (amostat == Marshaler::DISALLOWED)
    {
        EmitOrThrowInteropParamException(psl, resID, m_paramidx);
        return;
    }

    if (amostat == Marshaler::HANDLEASNORMAL)
    {
        {
            //
        // Emit marshaler creation opcode
        //
    
        UINT16 local = EmitCreateOpcode(psl);
    
        //
        // Emit Marshal opcode
        //
    
        BYTE marshal = (comToNative ? ML_MARSHAL_C2N : ML_MARSHAL_N2C);
        if (m_byref)
            marshal += 2;
        if (!m_in)
            marshal++;
    
        psl->MLEmit(marshal);
    
    
        //
        // Emit Unmarshal opcode
        //
    
        int index = 0;
        if (m_byref)
            index += 3;
        if (m_out)
        {
            index++;
            if (m_in)
                index++;
        }
    
        if (comToNative
            ? ((m_unmarshalC2NNeeded[m_type])&(1<<index))
            : ((m_unmarshalN2CNeeded[m_type])&(1<<index)))
        {
            BYTE unmarshal = (comToNative ? ML_UNMARSHAL_C2N_IN : ML_UNMARSHAL_N2C_IN) + index;
    
            pslPost->MLEmit(unmarshal);
            pslPost->Emit16(local);
        }
    }
}
}

void MarshalInfo::GenerateReturnML(MLStubLinker *psl,
                                     MLStubLinker *pslPost,
                                     BOOL comToNative,
                                     BOOL retval)
{
    {
    THROWSCOMPLUSEXCEPTION();

    Marshaler::ArgumentMLOverrideStatus amostat;
    UINT resID = IDS_EE_BADPINVOKE_RESTRICTION;

    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
       amostat = Marshaler::HANDLEASNORMAL;
    }
    else
    {
    amostat = (gReturnMLOverride[m_type]) (psl,
                                           pslPost,
                                           comToNative,
                                           retval,
                                           &m_args,
                                           &resID);
    }
    
    if (amostat == Marshaler::DISALLOWED)
    {
        EmitOrThrowInteropParamException(psl, resID, 0);
        return;
    }
    else if (amostat == Marshaler::OVERRIDDEN)
    {
        if (retval)
            m_nativeArgSize = MLParmSize(sizeof(void *));
    
        if (m_flags[m_type] & MarshalerFlag_ReturnsComByref)
            m_comArgSize = StackElemSize(sizeof(void *));
    }
    else
    {
        _ASSERTE(amostat == Marshaler::HANDLEASNORMAL);

    
        if (m_type == MARSHAL_TYPE_UNKNOWN || m_type == MARSHAL_TYPE_NATIVEARRAY)
        {
            EmitOrThrowInteropParamException(psl, m_resID, 0);
            return;
        }
    
    
        //
        // Use simple copy opcodes if possible.
        //
    
        if (m_type <= MARSHAL_TYPE_DOUBLE)
        {
            if (retval)
            {
                if (comToNative)
                {
    
                    // Calling from COM to Native: getting returnval thru buffer.
                    _ASSERTE(comToNative && retval);
    
                    int pushOpcode = ML_END;
                    int copyOpcode = 0;
    
                    switch (m_type)
                    {
                    case MARSHAL_TYPE_GENERIC_1:
                        pushOpcode = ML_PUSHRETVALBUFFER1;
                        copyOpcode = ML_COPYI1;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_U1:
                        pushOpcode = ML_PUSHRETVALBUFFER1;
                        copyOpcode = ML_COPYU1;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_2:
                        pushOpcode = ML_PUSHRETVALBUFFER2;
                        copyOpcode = ML_COPYI2;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_U2:
                        pushOpcode = ML_PUSHRETVALBUFFER2;
                        copyOpcode = ML_COPYU2;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_4:
                        pushOpcode = ML_PUSHRETVALBUFFER4;
                        copyOpcode = ML_COPY4;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_8:
                        pushOpcode = ML_PUSHRETVALBUFFER8;
                        copyOpcode = ML_COPY8;
                        break;
    
                    case MARSHAL_TYPE_WINBOOL:
                        pushOpcode = ML_PUSHRETVALBUFFER4;
                        copyOpcode = ML_BOOL_N2C;
                        break;
    
                    case MARSHAL_TYPE_CBOOL:
                        pushOpcode = ML_PUSHRETVALBUFFER1;
                        copyOpcode = ML_CBOOL_N2C;
                        break;
    
    
                    default:
                        break;
                    }
    
                    if (pushOpcode != ML_END)
                    {
                        if (psl)
                        {
                        psl->MLEmit(pushOpcode);
                        UINT16 local = psl->MLNewLocal(sizeof(RetValBuffer));
    
                        pslPost->MLEmit(ML_SETSRCTOLOCAL);
                        pslPost->Emit16(local);
                        pslPost->MLEmit(copyOpcode);
                        }
    
                        m_nativeArgSize = MLParmSize(sizeof(void*));
    
                        return;
                    }
                }
                else
                {
    
                    // Calling from Native to COM: getting returnval thru buffer.
                    _ASSERTE(!comToNative && retval);
    
                    int copyOpcode = ML_END;
    
                    switch (m_type)
                    {
                    case MARSHAL_TYPE_GENERIC_4:
                        copyOpcode = ML_COPY4;
                        break;
    
                    case MARSHAL_TYPE_GENERIC_8:
                        copyOpcode = ML_COPY8;
                        break;
    
                    case MARSHAL_TYPE_FLOAT:
                        copyOpcode = ML_COPY4;
                        break;
    
                    case MARSHAL_TYPE_DOUBLE:
                        copyOpcode = ML_COPY8;
                        break;
    
                    case MARSHAL_TYPE_CBOOL:
                        copyOpcode = ML_CBOOL_C2N;
                        break;
    
                    default:
                        break;
                    }
    
                    if (copyOpcode != ML_END)
                    {
                        if (pslPost)
                        pslPost->MLEmit(copyOpcode);
    
                        m_nativeArgSize = MLParmSize(sizeof(void*));
    
                        return;
                    }
                }
            }
            else if (!retval)
            {
                // Getting return value thru eax:edx. This code path handles
                // both COM->Native && Native->COM.
                _ASSERTE(!retval);
    
                if (!psl)
                    return;
    
                switch (m_type)
                {
    
    #ifdef WRONGCALLINGCONVENTIONHACK
                case MARSHAL_TYPE_GENERIC_1:
                    pslPost->MLEmit(comToNative ? ML_COPYI1 : ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_U1:
                    pslPost->MLEmit(comToNative ? ML_COPYU1 : ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_2:
                    pslPost->MLEmit(comToNative ? ML_COPYI2 : ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_U2:
                    pslPost->MLEmit(comToNative ? ML_COPYU2 : ML_COPY4);
                    return;
    
    #else
                case MARSHAL_TYPE_GENERIC_1:
                case MARSHAL_TYPE_GENERIC_U1:
                case MARSHAL_TYPE_GENERIC_2:
                case MARSHAL_TYPE_GENERIC_U2:
                    pslPost->MLEmit(ML_COPY4);
                    return;
    #endif
    
                case MARSHAL_TYPE_WINBOOL:
                    pslPost->MLEmit(ML_BOOL_N2C);
                    return;
    
                case MARSHAL_TYPE_CBOOL:
                    pslPost->MLEmit(comToNative ? ML_CBOOL_N2C : ML_CBOOL_C2N);
                    return;
    
                case MARSHAL_TYPE_GENERIC_4:
                    pslPost->MLEmit(ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_GENERIC_8:
                    pslPost->MLEmit(ML_COPY8);
                    return;
    
                case MARSHAL_TYPE_FLOAT:
                    pslPost->MLEmit(ML_COPY4);
                    return;
    
                case MARSHAL_TYPE_DOUBLE:
                    pslPost->MLEmit(ML_COPY8);
                    return;

                default:
                    break;
                }
            }
        }
    
        //
        // Compute sizes
        //
    
        if (retval)
            m_nativeArgSize = MLParmSize(sizeof(void *));
    
        if (m_flags[m_type] & MarshalerFlag_ReturnsComByref)
            m_comArgSize = StackElemSize(sizeof(void *));
    
        if (!psl)
            return;
    
        //
        // Emit marshaler creation opcode
        //
    
        UINT16 local = EmitCreateOpcode(psl);
    
        //
        // Emit prereturn opcode, if necessary
        //
    
        if (retval || (m_flags[m_type] & MarshalerFlag_ReturnsComByref))
        {
            BYTE prereturn = comToNative ? ML_PRERETURN_C2N : ML_PRERETURN_N2C;
            if (retval)
                prereturn++;
    
            psl->MLEmit(prereturn);
        }
    
        //
        // Emit return opcode
        //
    
        BYTE return_ = comToNative ? ML_RETURN_C2N : ML_RETURN_N2C;
        if (retval)
            return_++;
        pslPost->MLEmit(return_);
        pslPost->Emit16(local);
    }
}
}

void MarshalInfo::GenerateSetterML(MLStubLinker *psl,
                                   MLStubLinker *pslPost)
{
    {
    THROWSCOMPLUSEXCEPTION();
    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
        COMPlusThrow(kMarshalDirectiveException, IDS_EE_COM_UNSUPPORTED_SIG);
    }

    if (psl)
    {
        EmitCreateOpcode(psl);
        psl->MLEmit(ML_SET_COM);
    }

    m_nativeArgSize = MLParmSize(nativeSize(m_type));
}
}

void MarshalInfo::GenerateGetterML(MLStubLinker *psl,
                                   MLStubLinker *pslPost,
                                   BOOL retval)
{
    {
    THROWSCOMPLUSEXCEPTION();
    if (m_type == MARSHAL_TYPE_UNKNOWN)
    {
        COMPlusThrow(kMarshalDirectiveException, IDS_EE_COM_UNSUPPORTED_SIG);
    }

    if (retval)
    {
        if (psl)
        {
            EmitCreateOpcode(psl);
            psl->MLEmit(ML_PREGET_COM_RETVAL);
        }
        m_nativeArgSize = MLParmSize(sizeof(void*));
    }
    else if (psl)
    {
        EmitCreateOpcode(pslPost);
        pslPost->MLEmit(ML_GET_COM);
    }
}
}

UINT16 MarshalInfo::EmitCreateOpcode(MLStubLinker *psl)
{
    {

    THROWSCOMPLUSEXCEPTION();

    psl->MLEmit(ML_CREATE_MARSHALER_GENERIC_1 + m_type);
    UINT16 local = psl->MLNewLocal(m_localSizes[m_type]);

    switch (m_type)
    {
    default:
        break;


    case MARSHAL_TYPE_INTERFACE:
    {
        ItfMarshalInfo itfInfo;
        GetItfMarshalInfo(&itfInfo);
        psl->EmitPtr(itfInfo.pClassMT);
        psl->EmitPtr(itfInfo.pItfMT);
        break;
    }

    case MARSHAL_TYPE_NATIVEARRAY:
        ML_CREATE_MARSHALER_CARRAY_OPERANDS mops;
        mops.methodTable = m_hndArrayElemType.AsMethodTable();
        mops.elementType = m_arrayElementType;
        mops.countParamIdx = m_countParamIdx;
        mops.countSize   = 0; //placeholder for later patching (if left unpatched, this value signals marshaler to use managed size of array)
        mops.multiplier  = m_multiplier;
        mops.additive    = m_additive;
        psl->EmitBytes((const BYTE*)&mops, sizeof(mops));
        break;

    case MARSHAL_TYPE_BLITTABLEPTR:
    case MARSHAL_TYPE_LAYOUTCLASSPTR:
    case MARSHAL_TYPE_LPWSTR_X :
    case MARSHAL_TYPE_LPSTR_X :
    case MARSHAL_TYPE_LPWSTR_BUFFER_X :
    case MARSHAL_TYPE_LPSTR_BUFFER_X :
            psl->EmitPtr(m_pClass->GetMethodTable());
        break;

    case MARSHAL_TYPE_REFERENCECUSTOMMARSHALER:
    case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
            psl->EmitPtr(m_pCMHelper);
        break;

    case MARSHAL_TYPE_UNKNOWN:
        COMPlusThrow(kMarshalDirectiveException, IDS_EE_COM_UNSUPPORTED_SIG);
    }

    return local;
}
}


void MarshalInfo::GetItfMarshalInfo(ItfMarshalInfo *pInfo)
{
    _ASSERTE(pInfo);
    _ASSERTE(m_type == MARSHAL_TYPE_INTERFACE);

    // Initialize the output parameters.
    pInfo->pItfMT = NULL;
    pInfo->pClassMT = NULL;
    if (!m_pClass->IsInterface())
    {
        // Set the class method table.
        pInfo->pClassMT = m_pClass->GetMethodTable();
    }
    else
    {
        pInfo->pItfMT = m_pClass->GetMethodTable();
    }
}

//===================================================================================
// Post-patches ML stubs for the sizeis feature.
//===================================================================================
VOID PatchMLStubForSizeIs(BYTE *pMLCode, UINT numArgs, MarshalInfo *pMLInfo)
{
    THROWSCOMPLUSEXCEPTION();

    INT16 *offsets = (INT16*)_alloca(numArgs * sizeof(INT16));

    INT16 srcofs = 0;
    UINT i;
    for (i = 0; i < numArgs; i++) 
    {
        offsets[i] = srcofs;
        srcofs += MLParmSize(pMLInfo[i].GetNativeArgSize());
    }

    BYTE *pMLWalk = pMLCode;
    for (i = 0; i < numArgs; i++) 
    {
        if (pMLInfo[i].GetMarshalType() == MarshalInfo::MARSHAL_TYPE_NATIVEARRAY)
        {
            MLCode mlcode;
            while ((mlcode = *(pMLWalk++)) != ML_CREATE_MARSHALER_CARRAY)
            {
                _ASSERTE(mlcode != ML_END && mlcode != ML_INTERRUPT);
                pMLWalk += gMLInfo[mlcode].m_numOperandBytes;
            }

            ML_CREATE_MARSHALER_CARRAY_OPERANDS *pmops = (ML_CREATE_MARSHALER_CARRAY_OPERANDS*)pMLWalk;
            pMLWalk += gMLInfo[mlcode].m_numOperandBytes;
            if (pmops->multiplier != 0) 
            {
            
                UINT16 countParamIdx = pmops->countParamIdx;
                if (countParamIdx >= numArgs)
                {
                    COMPlusThrow(kIndexOutOfRangeException, IDS_EE_SIZECONTROLOUTOFRANGE);
                }
                pmops->offsetbump = offsets[countParamIdx] - offsets[i];
                switch (pMLInfo[countParamIdx].GetMarshalType())
                {
                    case MarshalInfo::MARSHAL_TYPE_GENERIC_1:
                    case MarshalInfo::MARSHAL_TYPE_GENERIC_U1:
                        pmops->countSize = 1;
                        break;

                    case MarshalInfo::MARSHAL_TYPE_GENERIC_2:
                    case MarshalInfo::MARSHAL_TYPE_GENERIC_U2:
                        pmops->countSize = 2;
                        break;

                    case MarshalInfo::MARSHAL_TYPE_GENERIC_4:
                        pmops->countSize = 4;
                        break;

                    case MarshalInfo::MARSHAL_TYPE_GENERIC_8:
                        pmops->countSize = 8;
                        break;

                    default:
                        COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIZECONTROLBADTYPE);
                }
            }
        }
    }
}

//===================================================================================
// Support routines for storing ML stubs in prejit files
//===================================================================================

//===============================================================
// Collects paraminfo's in an indexed array so that:
//
//   aParams[0] == param token for return value
//   aParams[1] == param token for argument #1...
//   aParams[numargs] == param token for argument #n...
//
// If no param token exists, the corresponding array element
// is set to mdParamDefNil.
//
// Inputs:
//    pInternalImport  -- ifc for metadata api
//    md       -- token of method. If token is mdMethodNil,
//                all aParam elements will be set to mdParamDefNil.
//    numargs  -- # of arguments in mdMethod
//    aParams  -- uninitialized array with numargs+1 elements.
//                on exit, will be filled with param tokens.
//===============================================================
VOID CollateParamTokens(IMDInternalImport *pInternalImport, mdMethodDef md, ULONG numargs, mdParamDef *aParams)
{
    THROWSCOMPLUSEXCEPTION();

    for (ULONG i = 0; i < numargs + 1; i++)
    {
        aParams[i] = mdParamDefNil;
    }
    if (md != mdMethodDefNil)
    {
        HENUMInternal hEnumParams;
        HRESULT hr = pInternalImport->EnumInit(mdtParamDef, md, &hEnumParams);
        if (FAILED(hr))
        {
            // no param info: nothing left to do here
        }
        else
        {
            mdParamDef CurrParam = mdParamDefNil;
            while (pInternalImport->EnumNext(&hEnumParams, &CurrParam))
            {
                USHORT usSequence;
                DWORD dwAttr;
                pInternalImport->GetParamDefProps(CurrParam, &usSequence, &dwAttr);
                _ASSERTE(usSequence <= numargs);
                _ASSERTE(aParams[usSequence] == mdParamDefNil);
                aParams[usSequence] = CurrParam; 
            }
        }

    }

}



#ifdef CUSTOMER_CHECKED_BUILD

VOID MarshalInfo::OutputCustomerCheckedBuildMarshalInfo(MethodDesc* pMD, SigPointer sig, Module* pModule)
{
    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pMD != NULL)
    {
        // Get method name
        CQuickArray<WCHAR> strMethodName;
        UINT iMethodNameLength = (UINT)strlen(pMD->GetName()) + 1;
        strMethodName.Alloc(iMethodNameLength);
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pMD->GetName(),
                             -1, strMethodName.Ptr(), iMethodNameLength );

        // Get namespace.class name
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pMD->GetClass());

        static const WCHAR strNameFormat[] = {L"%s::%s"};
        CQuickArray<WCHAR> strNamespaceClassMethodName;
        strNamespaceClassMethodName.Alloc((UINT)strMethodName.Size() + (UINT)wcslen(_wszclsname_) + lengthof(strNameFormat));
        Wszwsprintf((LPWSTR)strNamespaceClassMethodName.Ptr(), strNameFormat, _wszclsname_, strMethodName.Ptr());

        if ( pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Marshaling, strNamespaceClassMethodName.Ptr()) )
        {
            // Collect information for marshal type on managed side

            CQuickArray<WCHAR> strManagedMarshalType;

            SigFormat sigfmt;
            TypeHandle th;
            OBJECTREF throwable = NULL;
            GCPROTECT_BEGIN(throwable);
            th = sig.GetTypeHandle(pModule, &throwable);
            if (throwable != NULL)
            {
                static const WCHAR strErrorMsg[] = {L"<error>"};
                strManagedMarshalType.Alloc(lengthof(strErrorMsg));
                wcscpy(strManagedMarshalType.Ptr(), strErrorMsg);
            }
            else
            {
                sigfmt.AddType(th);
                UINT iManagedMarshalTypeLength = (UINT)strlen(sigfmt.GetCString()) + 1;
                strManagedMarshalType.Alloc(iManagedMarshalTypeLength);
                MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, sigfmt.GetCString(),
                                     -1, strManagedMarshalType.Ptr(), iManagedMarshalTypeLength);
            }
            GCPROTECT_END();

            // Collect information for marshal type on native side

            CQuickArray<WCHAR> strNativeMarshalType;
            MarshalTypeToString(&strNativeMarshalType);

            static const WCHAR strMessageFormat[] = 
                {L"Marshaling from %s to %s in method %s."};

            CQuickArray<WCHAR> strMessage;
            strMessage.Alloc( lengthof(strMessageFormat) + 
                              (UINT) strManagedMarshalType.Size() + 
                              (UINT) strNativeMarshalType.Size() + 
                              strMethodName.Size() );
            Wszwsprintf( strMessage.Ptr(), strMessageFormat, strManagedMarshalType.Ptr(), 
                         strNativeMarshalType.Ptr(), strMethodName.Ptr() );
            pCdh->LogInfo(strMessage.Ptr(), CustomerCheckedBuildProbe_Marshaling);
        }
    }
}


VOID MarshalInfo::MarshalTypeToString(CQuickArray<WCHAR> *pStrMarshalType)
{
    LPWSTR strRetVal;

    // Some MarshalTypes have extra information and require special handling
    if (m_type == MARSHAL_TYPE_INTERFACE)
    {
        strRetVal = L"IUnknown";
    }
    else if (m_type == MARSHAL_TYPE_SAFEARRAY) {
        static const WCHAR strTemp[] = {L"SafeArray of %s"};
        CQuickArray<WCHAR> strVarType;
        VarTypeToString(m_arrayElementType, &strVarType);

        pStrMarshalType->Alloc(lengthof(strTemp) + strVarType.Size());
        Wszwsprintf(pStrMarshalType->Ptr(), strTemp, strVarType.Ptr());
        return;
    }
    else 
    if (m_type == MARSHAL_TYPE_NATIVEARRAY) {
        static const WCHAR strTemp[] = {L"native array of %s (size %i)"};
        CQuickArray<WCHAR> strVarType;
        VarTypeToString(m_arrayElementType, &strVarType);
        UINT32 iSize = m_countParamIdx * m_multiplier + m_additive;

        pStrMarshalType->Alloc(lengthof(strTemp) + strVarType.Size() + I_UINT32_MAX_DIGITS);
        Wszwsprintf(pStrMarshalType->Ptr(), strTemp, strVarType.Ptr(), iSize);
        return;
    }
    else if (m_type == MARSHAL_TYPE_REFERENCECUSTOMMARSHALER ||
             m_type == MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER ) {        
        OBJECTHANDLE objHandle = m_pCMHelper->GetCustomMarshalerInfo()->GetCustomMarshaler();
        BEGIN_ENSURE_COOPERATIVE_GC()
        {
            OBJECTREF pObjRef = ObjectFromHandle(objHandle);
            DefineFullyQualifiedNameForClassW();
            GetFullyQualifiedNameForClassW(pObjRef->GetClass());

            static const WCHAR strTemp[] = {L"custom marshaler (%s)"};
            pStrMarshalType->Alloc(lengthof(strTemp) + wcslen(_wszclsname_));
            Wszwsprintf(pStrMarshalType->Ptr(), strTemp, _wszclsname_);
        }        
        END_ENSURE_COOPERATIVE_GC();
        return;
    }
    else
    {
        // All other MarshalTypes with no special handling
        switch (m_type)
        {
            case MARSHAL_TYPE_GENERIC_1:
                strRetVal = L"BYTE";
                break;
            case MARSHAL_TYPE_GENERIC_U1:
                strRetVal = L"unsigned BYTE";
                break;
            case MARSHAL_TYPE_GENERIC_2:
                strRetVal = L"WORD";
                break;
            case MARSHAL_TYPE_GENERIC_U2:
                strRetVal = L"unsigned WORD";
                break;
            case MARSHAL_TYPE_GENERIC_4:
                strRetVal = L"DWORD";
                break;
            case MARSHAL_TYPE_GENERIC_8:
                strRetVal = L"QUADWORD";
                break;
            case MARSHAL_TYPE_WINBOOL:
                strRetVal = L"windows boolean";
                break;
            case MARSHAL_TYPE_VTBOOL:
                strRetVal = L"VariantBool";
                break;
            case MARSHAL_TYPE_ANSICHAR:
                strRetVal = L"Ansi character";
                break;
            case MARSHAL_TYPE_CBOOL:
                strRetVal = L"CBool";
                break;
            case MARSHAL_TYPE_FLOAT:
                strRetVal = L"float";
                break;
            case MARSHAL_TYPE_DOUBLE:
                strRetVal = L"double";
                break;
            case MARSHAL_TYPE_CURRENCY:
                strRetVal = L"CURRENCY";
                break;
            case MARSHAL_TYPE_DECIMAL:
                strRetVal = L"decimal";
                break;
            case MARSHAL_TYPE_DECIMAL_PTR:
                strRetVal = L"decimal pointer";
                break;
            case MARSHAL_TYPE_GUID:
                strRetVal = L"GUID";
                break;
            case MARSHAL_TYPE_GUID_PTR:
                strRetVal = L"GUID pointer";
                break;
            case MARSHAL_TYPE_DATE:
                strRetVal = L"DATE";
                break;
            case MARSHAL_TYPE_VARIANT:
                strRetVal = L"VARIANT";
                break;
            case MARSHAL_TYPE_BSTR:
                strRetVal = L"BSTR";
                break;
            case MARSHAL_TYPE_LPWSTR:
                strRetVal = L"LPWSTR";
                break;
            case MARSHAL_TYPE_LPSTR:
                strRetVal = L"LPSTR";
                break;
            case MARSHAL_TYPE_ANSIBSTR:
                strRetVal = L"AnsiBStr";
                break;
            case MARSHAL_TYPE_BSTR_BUFFER:
                strRetVal = L"BSTR buffer";
                break;
            case MARSHAL_TYPE_LPWSTR_BUFFER:
                strRetVal = L"LPWSTR buffer";
                break;
            case MARSHAL_TYPE_LPSTR_BUFFER:
                strRetVal = L"LPSTR buffer";
                break;
            case MARSHAL_TYPE_BSTR_X:
                strRetVal = L"MARSHAL_TYPE_BSTR_X";
                break;
            case MARSHAL_TYPE_LPWSTR_X:
                strRetVal = L"MARSHAL_TYPE_LPWSTR_X";
                break;
            case MARSHAL_TYPE_LPSTR_X:
                strRetVal = L"MARSHAL_TYPE_LPSTR_X";
                break;
            case MARSHAL_TYPE_BSTR_BUFFER_X:
                strRetVal = L"MARSHAL_TYPE_BSTR_BUFFER_X";
                break;
            case MARSHAL_TYPE_LPWSTR_BUFFER_X:
                strRetVal = L"MARSHAL_TYPE_LPWSTR_BUFFER_X";
                break;
            case MARSHAL_TYPE_LPSTR_BUFFER_X:
                strRetVal = L"MARSHAL_TYPE_LPSTR_BUFFER_X";
                break;

//          case MARSHAL_TYPE_INTERFACE:
//          case MARSHAL_TYPE_SAFEARRAY:
//          case MARSHAL_TYPE_NATIVEARRAY:
//              (see above)

            case MARSHAL_TYPE_ASANYA:
                strRetVal = L"AsAnyA";
                break;
            case MARSHAL_TYPE_ASANYW:
                strRetVal = L"AsAnyW";
                break;
            case MARSHAL_TYPE_DELEGATE:
                strRetVal = L"Delegate";
                break;
            case MARSHAL_TYPE_BLITTABLEPTR:
                strRetVal = L"blittable pointer";
                break;
            case MARSHAL_TYPE_VBBYVALSTR:
                strRetVal = L"VBByValStr";
                break;
            case MARSHAL_TYPE_VBBYVALSTRW:
                strRetVal = L"VBByRefStr";
                break;
            case MARSHAL_TYPE_LAYOUTCLASSPTR:
                strRetVal = L"Layout class pointer";
                break;
            case MARSHAL_TYPE_ARRAYWITHOFFSET:
                strRetVal = L"ArrayWithOffset";
                break;
            case MARSHAL_TYPE_BLITTABLEVALUECLASS:
                strRetVal = L"blittable Value class";
                break;
            case MARSHAL_TYPE_VALUECLASS:
                strRetVal = L"Value class";
                break;

//          case MARSHAL_TYPE_REFERENCECUSTOMMARSHALER:
//          case MARSHAL_TYPE_VALUECLASSCUSTOMMARSHALER:
//              (see above)

            case MARSHAL_TYPE_ARGITERATOR:
                strRetVal = L"ArgIterator";
                break;
            case MARSHAL_TYPE_BLITTABLEVALUECLASSWITHCOPYCTOR:
                strRetVal = L"blittable Value class with copy constructor";
                break;
            case MARSHAL_TYPE_OBJECT:
                strRetVal = L"Variant";
                break;
            case MARSHAL_TYPE_HANDLEREF:
                strRetVal = L"HandleRef";
                break;
            case MARSHAL_TYPE_OLECOLOR:
                strRetVal = L"OleColor";
                break;
            default:
                strRetVal = L"<UNKNOWN>";
                break;
        }
    }

    pStrMarshalType->Alloc((UINT)wcslen(strRetVal) + 1);
    wcscpy(pStrMarshalType->Ptr(), strRetVal);
    return;
}


VOID MarshalInfo::VarTypeToString(VARTYPE vt, CQuickArray<WCHAR> *pStrVarType)
{
    LPWSTR strRetVal;
    
    switch(vt)
    {
        case VT_I2:
            strRetVal = L"2-byte signed int";
            break;
        case VT_I4:
            strRetVal = L"4-byte signed int";
            break;
        case VT_R4:
            strRetVal = L"4-byte real";
            break;
        case VT_R8:
            strRetVal = L"8-byte real";
            break;
        case VT_CY:
            strRetVal = L"currency";
            break;
        case VT_DATE:
            strRetVal = L"date";
            break;
        case VT_BSTR:
            strRetVal = L"binary string";
            break;
        case VT_DISPATCH:
            strRetVal = L"IDispatch *";
            break;
        case VT_ERROR:
            strRetVal = L"Scode";
            break;
        case VT_BOOL:
            strRetVal = L"boolean";
            break;
        case VT_VARIANT:
            strRetVal = L"VARIANT *";
            break;
        case VT_UNKNOWN:
            strRetVal = L"IUnknown *";
            break;
        case VT_DECIMAL:
            strRetVal = L"16-byte fixed point";
            break;
        case VT_RECORD:
            strRetVal = L"user defined type";
            break;
        case VT_I1:
            strRetVal = L"signed char";
            break;
        case VT_UI1:
            strRetVal = L"unsigned char";
            break;
        case VT_UI2:
            strRetVal = L"unsigned short";
            break;
        case VT_UI4:
            strRetVal = L"unsigned short";
            break;
        case VT_INT:
            strRetVal = L"signed int";
            break;
        case VT_UINT:
            strRetVal = L"unsigned int";
            break;
        case VTHACK_WINBOOL:
            strRetVal = L"boolean";
            break;
        case VTHACK_ANSICHAR:
            strRetVal = L"char";
            break;
        default:
            strRetVal = L"unknown";
            break;
    }

    pStrVarType->Alloc((UINT)wcslen(strRetVal) + 1);
    wcscpy(pStrVarType->Ptr(), strRetVal);
    return;
}

#endif // CUSTOMER_CHECKED_BUILD
