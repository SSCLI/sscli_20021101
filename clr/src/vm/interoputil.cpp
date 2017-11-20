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
#include "vars.hpp"
#include "excep.h"
#include "interoputil.h"
#include "cachelinealloc.h"
#include "comutilnative.h"
#include "field.h"
#include "guidfromname.h"
#include "comvariant.h"
#include "eeconfig.h"
#include "mlinfo.h"
#include "reflectutil.h"
#include "reflectwrap.h"
#include "remoting.h"
#include "appdomain.hpp"
#include "commember.h"
#include "comreflectioncache.hpp"
#include "prettyprintsig.h"
#include "interopconverter.h"


//-------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE EEDllCanUnloadNow(void)
// DllCanUnloadNow delegates the call here
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE EEDllCanUnloadNow(void)
{
    /*if (ComCallWrapperCache::GetComCallWrapperCache()->CanShutDown())
    {
        ComCallWrapperCache::GetComCallWrapperCache()->CorShutdown();
    }*/
    //we should never unload unless the process is dying
    return S_FALSE;

}

//------------------------------------------------------------------
//  HRESULT SetupErrorInfo(OBJECTREF pThrownObject)
// setup error info for exception object
//
HRESULT SetupErrorInfo(OBJECTREF pThrownObject)
{
    HRESULT hr = E_FAIL;

    GCPROTECT_BEGIN(pThrownObject)
    {
        // Calls to COM up ahead.
        hr = QuickCOMStartup();

        if (SUCCEEDED(hr) && pThrownObject != NULL)
        {

            static int fExposeExceptionsInCOM = 0;
            static int fReadRegistry = 0;

            Thread * pThread  = GetThread();

            if (!fReadRegistry)
            {
                INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame()); //who knows..
                fExposeExceptionsInCOM = REGUTIL::GetConfigDWORD(L"ExposeExceptionsInCOM", 0);
                UNINSTALL_NESTED_EXCEPTION_HANDLER();
                fReadRegistry = 1;
            }

            if (fExposeExceptionsInCOM&3)
            {
                INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame()); //who knows..
                CQuickWSTRBase message;
                message.Init();
                GetExceptionMessage(pThrownObject, &message);

                if (fExposeExceptionsInCOM&1)
                {
                    PrintToStdOutW(L".NET exception in COM\n");
                    if (message.Size() > 0) 
                        PrintToStdOutW(message.Ptr());
                    else
                        PrintToStdOutW(L"No exception info available");
                }

                if (fExposeExceptionsInCOM&2)
                {
                    BEGIN_ENSURE_PREEMPTIVE_GC();
                    if (message.Size() > 0) 
                        WszMessageBoxInternal(NULL,message.Ptr(),L".NET exception in COM",MB_ICONSTOP|MB_OK);
                    else
                        WszMessageBoxInternal(NULL,L"No exception information available",L".NET exception in COM",MB_ICONSTOP|MB_OK);
                    END_ENSURE_PREEMPTIVE_GC();
                }


                message.Destroy();
                UNINSTALL_NESTED_EXCEPTION_HANDLER();
            }

            IErrorInfo* pErr = NULL;
            ICreateErrorInfo* pCErr = NULL;
            ExceptionData ED;
            ZeroMemory(&ED, sizeof(ED));

            COMPLUS_TRY
            {
                ExceptionNative::GetExceptionData(pThrownObject, &ED);

                hr = ED.hr;

                if (FAILED(CreateErrorInfo(&pCErr)))
                    COMPLUS_LEAVE;

                if (FAILED(pCErr->SetDescription(ED.bstrDescription)))
                    COMPLUS_LEAVE;
                if (FAILED(pCErr->SetSource(ED.bstrSource)))
                    COMPLUS_LEAVE;
                if (FAILED(pCErr->SetHelpFile(ED.bstrHelpFile)))
                    COMPLUS_LEAVE;
                if (FAILED(pCErr->SetHelpContext(ED.dwHelpContext)))
                    COMPLUS_LEAVE;

                if (FAILED(pCErr->QueryInterface(IID_IErrorInfo, (void**)&pErr)))
                    COMPLUS_LEAVE;

                SetErrorInfo(0, pErr);                
            }
            COMPLUS_CATCH
            {
            }
            COMPLUS_END_CATCH

            if (SUCCEEDED(hr))
                hr = E_FAIL;

            if (pErr)
                pErr->Release();
            if (pCErr)
                pCErr->Release();
            FreeExceptionData(&ED); // free the BStrs
        }
    }
    GCPROTECT_END();

    return hr;
}

//-------------------------------------------------------------------
// void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo)
// Called from DLLMain, to initialize com specific data structures.
//-------------------------------------------------------------------
void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo)
{
    if (pErrInfo != NULL)
    {
        Thread* pThread = GetThread();
        if (pThread != NULL)
        {
            pThread->EnablePreemptiveGC();
            pErrInfo->GetSource (&pedata->bstrSource);
            pErrInfo->GetDescription (&pedata->bstrDescription);
            pErrInfo->GetHelpFile (&pedata->bstrHelpFile);
            pErrInfo->GetHelpContext (&pedata->dwHelpContext );
            pErrInfo->GetGUID(&pedata->guid);
            ULONG cbRef = SafeRelease(pErrInfo); // release the IErrorInfo interface pointer
            LogInteropRelease(pErrInfo, cbRef, "IErrorInfo");
            pThread->DisablePreemptiveGC();
        }
    }
}


//--------------------------------------------------------------------------------
// ULONG SafeRelease(IUnknown* pUnk)
// Release helper, enables and disables GC during call-outs
ULONG SafeRelease(IUnknown* pUnk)
{
    if (pUnk == NULL)
        return 0;

    ULONG res = 0;
    Thread* pThread = GetThread();

    int fGC = pThread && pThread->PreemptiveGCDisabled();
    if (fGC)
        pThread->EnablePreemptiveGC();

    res = pUnk->Release();

    if (fGC)
        pThread->DisablePreemptiveGC();

    return res;
}

//--------------------------------------------------------------------------------
//void SafeVariantClear(VARIANT* pVar)
// safe variant helpers
void SafeVariantClear(VARIANT* pVar)
{
    if (pVar)
    {
        Thread* pThread = GetThread();
        int bToggleGC = pThread->PreemptiveGCDisabled();
        if (bToggleGC)
            pThread->EnablePreemptiveGC();

        VariantClear(pVar);

        if (bToggleGC)
            pThread->DisablePreemptiveGC();
    }
}

//--------------------------------------------------------------------------------
// Determines if a COM object can be cast to the specified type.
BOOL CanCastComObject(OBJECTREF obj, TypeHandle hndType)
{
    if (!obj)
        return TRUE;

    if (hndType.GetMethodTable()->IsInterface())
    {
        return obj->GetClass()->SupportsInterface(obj, hndType.GetMethodTable());
    }
    else
    {
        return TypeHandle(obj->GetMethodTable()).CanCastTo(hndType);
    }
}

//------------------------------------------------------------------------------
// ARG_SLOT FieldAccessor(FieldDesc* pFD, OBJECTREF oref, ARG_SLOT val, BOOL isGetter, UINT8 cbSize)
// helper to access fields from an object
ARG_SLOT FieldAccessor(FieldDesc* pFD, OBJECTREF oref, ARG_SLOT val, BOOL isGetter, UINT8 cbSize)
{
    ARG_SLOT res = 0;
    _ASSERTE(pFD != NULL);
    _ASSERTE(oref != NULL);

    _ASSERTE(cbSize == 1 || cbSize == 2 || cbSize == 4 || cbSize == 8);

    switch (cbSize)
    {
        case 1: if (isGetter)
                    res = pFD->GetValue8(oref);
                else
                    pFD->SetValue8(oref,(INT8)val);
                    break;

        case 2: if (isGetter)
                    res = pFD->GetValue16(oref);
                else
                    pFD->SetValue16(oref,(INT16)val);
                    break;

        case 4: if (isGetter)
                    res = pFD->GetValue32(oref);
                else
                    pFD->SetValue32(oref,(INT32)val);
                    break;

        case 8: if (isGetter)
                    res = pFD->GetValue64(oref);
                else
                    pFD->SetValue64(oref,val);
                    break;
    };

    return res;
}


//------------------------------------------------------------------------------
// BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT)
BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT)
{
    _ASSERTE(pMT != NULL);
    _ASSERTE(pParentMT != NULL);

    while (pMT != NULL)
    {
        if (pMT == pParentMT)
            return TRUE;
        pMT = pMT->GetParentMethodTable();
    }
    return FALSE;
}


//---------------------------------------------------------------------------
// Returns the index of the LCID parameter if one exists and -1 otherwise.
int GetLCIDParameterIndex(IMDInternalImport *pInternalImport, mdMethodDef md)
{
    int             iLCIDParam = -1;
    HRESULT         hr;
    const BYTE *    pVal;
    ULONG           cbVal;

    // Check to see if the method has the LCIDConversionAttribute.
    hr = pInternalImport->GetCustomAttributeByName(md, INTEROP_LCIDCONVERSION_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
        iLCIDParam = *((int*)(pVal + 2));

    return iLCIDParam;
}

//---------------------------------------------------------------------------
// Transforms an LCID into a CultureInfo.
void GetCultureInfoForLCID(LCID lcid, OBJECTREF *pCultureObj)
{
    THROWSCOMPLUSEXCEPTION();

    static TypeHandle s_CultureInfoType;
    static MethodDesc *s_pCultureInfoConsMD = NULL;

    // Retrieve the CultureInfo type handle.
    if (s_CultureInfoType.IsNull())
        s_CultureInfoType = TypeHandle(g_Mscorlib.GetClass(CLASS__CULTURE_INFO));

    // Retrieve the CultureInfo(int culture) constructor.
    if (!s_pCultureInfoConsMD)
        s_pCultureInfoConsMD = g_Mscorlib.GetMethod(METHOD__CULTURE_INFO__INT_CTOR);

    OBJECTREF CultureObj = NULL;
    GCPROTECT_BEGIN(CultureObj)
    {
        // Allocate a CultureInfo with the specified LCID.
        CultureObj = AllocateObject(s_CultureInfoType.GetMethodTable());

        // Call the CultureInfo(int culture) constructor.
        ARG_SLOT pNewArgs[] = {
            ObjToArgSlot(CultureObj),
            (ARG_SLOT)lcid
        };
        s_pCultureInfoConsMD->Call(pNewArgs, METHOD__CULTURE_INFO__INT_CTOR);

        // Set the returned culture object.
        *pCultureObj = CultureObj;
    }
    GCPROTECT_END();
}

//---------------------------------------------------------------------------
// This method determines if a member is visible from COM.
BOOL IsMemberVisibleFromCom(IMDInternalImport *pInternalImport, mdToken tk, mdMethodDef mdAssociate)
{
    DWORD                   dwFlags;

    // Check to see if the member is public.
    switch (TypeFromToken(tk))
    {
        case mdtFieldDef:
            _ASSERTE(IsNilToken(mdAssociate));
            dwFlags = pInternalImport->GetFieldDefProps(tk);
            if (!IsFdPublic(dwFlags))
                return FALSE;
            break;

        case mdtMethodDef:
            _ASSERTE(IsNilToken(mdAssociate));
            dwFlags = pInternalImport->GetMethodDefProps(tk);
            if (!IsMdPublic(dwFlags))
                return FALSE;
            break;

        case mdtProperty:
            _ASSERTE(!IsNilToken(mdAssociate));
            dwFlags = pInternalImport->GetMethodDefProps(mdAssociate);
            if (!IsMdPublic(dwFlags))
                return FALSE;

            break;

        default:
            _ASSERTE(!"The type of the specified member is not handled by IsMemberVisibleFromCom");
            break;
    }


    // The member is visible.
    return TRUE;
}

ULONG GetStringizedMethodDef(IMDInternalImport *pMDImport, mdToken tkMb, CQuickArray<BYTE> &rDef, ULONG cbCur)
{
    THROWSCOMPLUSEXCEPTION();

    CQuickBytes rSig;
    HENUMInternal ePm;                  // For enumerating  params.
    mdParamDef  tkPm;                   // A param token.
    DWORD       dwFlags;                // Param flags.
    USHORT      usSeq;                  // Sequence of a parameter.
    ULONG       cPm;                    // Count of params.
    PCCOR_SIGNATURE pSig;
    ULONG       cbSig;
    HRESULT     hr = S_OK;

    // Don't count invisible members.
    if (!IsMemberVisibleFromCom(pMDImport, tkMb, mdMethodDefNil))
        return cbCur;
    
    // accumulate the signatures.
    pSig = pMDImport->GetSigOfMethodDef(tkMb, &cbSig);
    IfFailThrow(::PrettyPrintSigInternal(pSig, cbSig, "", &rSig, pMDImport));
    // Get the parameter flags.
    IfFailThrow(pMDImport->EnumInit(mdtParamDef, tkMb, &ePm));
    cPm = pMDImport->EnumGetCount(&ePm);
    // Resize for sig and params.  Just use 1 byte of param.
    IfFailThrow(rDef.ReSize(cbCur + rSig.Size() + cPm));
    memcpy(rDef.Ptr() + cbCur, rSig.Ptr(), rSig.Size());
    cbCur += (ULONG)(rSig.Size()-1);
    // Enumerate through the params and get the flags.
    while (pMDImport->EnumNext(&ePm, &tkPm))
    {
        pMDImport->GetParamDefProps(tkPm, &usSeq, &dwFlags);
        if (usSeq == 0)     // Skip return type flags.
            continue;
        rDef[cbCur++] = (BYTE)dwFlags;
    }
    pMDImport->EnumClose(&ePm);

    // Return the number of bytes.
    return cbCur;
} // void GetStringizedMethodDef()

ULONG GetStringizedFieldDef(IMDInternalImport *pMDImport, mdToken tkMb, CQuickArray<BYTE> &rDef, ULONG cbCur)
{
    THROWSCOMPLUSEXCEPTION();

    CQuickBytes rSig;
    PCCOR_SIGNATURE pSig;
    ULONG       cbSig;
    HRESULT     hr = S_OK;

    // Don't count invisible members.
    if (!IsMemberVisibleFromCom(pMDImport, tkMb, mdMethodDefNil))
        return cbCur;
    
    // accumulate the signatures.
    pSig = pMDImport->GetSigOfFieldDef(tkMb, &cbSig);
    IfFailThrow(::PrettyPrintSigInternal(pSig, cbSig, "", &rSig, pMDImport));
    IfFailThrow(rDef.ReSize(cbCur + rSig.Size()));
    memcpy(rDef.Ptr() + cbCur, rSig.Ptr(), rSig.Size());
    cbCur += (ULONG)(rSig.Size()-1);

    // Return the number of bytes.
    return cbCur;
} // void GetStringizedFieldDef()

//--------------------------------------------------------------------------------
// This method generates a stringized version of an interface that contains the
// name of the interface along with the signature of all the methods.
SIZE_T GetStringizedItfDef(TypeHandle InterfaceType, CQuickArray<BYTE> &rDef)
{
    THROWSCOMPLUSEXCEPTION();

    LPWSTR szName;                 
    ULONG cchName;
    HENUMInternal eMb;                  // For enumerating methods and fields.
    mdToken     tkMb;                   // A method or field token.
    SIZE_T       cbCur;
    HRESULT hr = S_OK;
    EEClass *pItfClass = InterfaceType.GetClass();
    _ASSERTE(pItfClass);
    IMDInternalImport *pMDImport = pItfClass->GetMDImport();
    _ASSERTE(pMDImport);

    // Make sure the specified type is an interface with a valid token.
    _ASSERTE(!IsNilToken(pItfClass->GetCl()) && pItfClass->IsInterface());

    // Get the name of the class.
    DefineFullyQualifiedNameForClassW();
    szName = GetFullyQualifiedNameForClassNestedAwareW(pItfClass);
    _ASSERTE(szName);
    cchName = (ULONG)wcslen(szName);

    // Start with the interface name.
    cbCur = cchName * sizeof(WCHAR);
    IfFailThrow(rDef.ReSize(cbCur));
    wcscpy(reinterpret_cast<LPWSTR>(rDef.Ptr()), szName);

    // Enumerate the methods...
    IfFailThrow(pMDImport->EnumInit(mdtMethodDef, pItfClass->GetCl(), &eMb));
    while(pMDImport->EnumNext(&eMb, &tkMb))
    {   // accumulate the signatures.
        cbCur = GetStringizedMethodDef(pMDImport, tkMb, rDef, (ULONG)cbCur);
    }
    pMDImport->EnumClose(&eMb);

    // Enumerate the fields...
    IfFailThrow(pMDImport->EnumInit(mdtFieldDef, pItfClass->GetCl(), &eMb));
    while(pMDImport->EnumNext(&eMb, &tkMb))
    {   // accumulate the signatures.
        cbCur = GetStringizedFieldDef(pMDImport, tkMb, rDef, (ULONG)cbCur);
    }
    pMDImport->EnumClose(&eMb);

    // Return the number of bytes.
    return cbCur;
} // ULONG GetStringizedItfDef()

//--------------------------------------------------------------------------------
// Helper to get the stringized form of assembly guid.
HRESULT GetStringizedGuidForAssembly(Assembly *pAssembly, CQuickArray<BYTE> &rDef, ULONG cbCur, ULONG *pcbFetched)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    LPCUTF8     pszName = NULL;         // Library name in UTF8.
    ULONG       cbName;                 // Length of name, UTF8 characters.
    LPWSTR      pName;                  // Pointer to library name.
    ULONG       cchName;                // Length of name, wide chars.
    LPWSTR      pch=0;                  // Pointer into lib name.
    const BYTE  *pSN=NULL;              // Pointer to public key.
    DWORD       cbSN=0;                 // Size of public key.
    static char szTypeLibKeyName[] = {"TypeLib"};
 
    // Get the name, and determine its length.
    pAssembly->GetName(&pszName);
    cbName=(ULONG)strlen(pszName);
    cchName = WszMultiByteToWideChar(CP_ACP,0, pszName,cbName+1, 0,0);
    
    // See if there is a public key.
    if (pAssembly->IsStrongNamed())
        pAssembly->GetPublicKey(&pSN, &cbSN);
    
    // Get the version information.
    struct  versioninfo
    {
        USHORT      usMajorVersion;         // Major Version.   
        USHORT      usMinorVersion;         // Minor Version.
        USHORT      usBuildNumber;          // Build Number.
        USHORT      usRevisionNumber;       // Revision Number.
    } ver;
    // There is a bug here in that usMajor is used twice and usMinor not at all.
    //  We're not fixing that because everyone has a major version, so all the
    //  generated guids would change, which is breaking.  To compensate, if 
    //  the minor is non-zero, we add it separately, below.
    ver.usMajorVersion = pAssembly->m_Context->usMajorVersion;
    ver.usMinorVersion =  pAssembly->m_Context->usMajorVersion;  // Don't fix this line!
    ver.usBuildNumber =  pAssembly->m_Context->usBuildNumber;
    ver.usRevisionNumber =  pAssembly->m_Context->usRevisionNumber;
    
    // Resize the output buffer.
    IfFailGo(rDef.ReSize(cbCur + cchName*sizeof(WCHAR) + sizeof(szTypeLibKeyName)-1 + cbSN + sizeof(ver)+sizeof(USHORT)));
                                                                                                          
    // Put it all together.  Name first.
    WszMultiByteToWideChar(CP_ACP,0, pszName,cbName+1, (LPWSTR)(&rDef[cbCur]),cchName);
    pName = (LPWSTR)(&rDef[cbCur]);
    for (pch=pName; *pch; ++pch)
    {
        if (*pch == '.' || *pch == ' ')
            *pch = '_';
        else
        if (iswupper(*pch))
            *pch = towlower(*pch);
    }
    cbCur += (cchName-1)*sizeof(WCHAR);
    memcpy(&rDef[cbCur], szTypeLibKeyName, sizeof(szTypeLibKeyName)-1);
    cbCur += sizeof(szTypeLibKeyName)-1;
        
    // Version.
    memcpy(&rDef[cbCur], &ver, sizeof(ver));
    cbCur += sizeof(ver);

    // If minor version is non-zero, add it to the hash.  It should have been in the ver struct,
    //  but due to a bug, it was omitted there, and fixing it "right" would have been
    //  breaking.  So if it isn't zero, add it; if it is zero, don't add it.  Any
    //  possible value of minor thus generates a different guid, and a value of 0 still generates
    //  the guid that the original, buggy, code generated.
    if (pAssembly->m_Context->usMinorVersion != 0)
    {
        SET_UNALIGNED_16(&rDef[cbCur], pAssembly->m_Context->usMinorVersion);
        cbCur += sizeof(USHORT);
    }

    // Public key.
    memcpy(&rDef[cbCur], pSN, cbSN);
    cbCur += cbSN;

    if (pcbFetched)
        *pcbFetched = cbCur;

ErrExit:
    return hr;
}

