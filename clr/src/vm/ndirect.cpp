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
// NDIRECT.CPP -
//
// N/Direct support.


#include "common.h"

#include "vars.hpp"
#include "ml.h"
#include "stublink.h"
#include "threads.h"
#include "excep.h"
#include "mlgen.h"
#include "ndirect.h"
#include "cgensys.h"
#include "method.hpp"
#include "siginfo.hpp"
#include "mlcache.h"
#include "security.h"
#include "comdelegate.h"
#include "reflectwrap.h"
#include "ceeload.h"
#include "utsem.h"
#include "mlinfo.h"
#include "eeconfig.h"
#include "cormap.hpp"
#include "eeconfig.h"
#include "cgensys.h"
#include "comutilnative.h"
#include "reflectutil.h"
#include "asmconstants.h"


VOID AllocGeneratedIL(COR_ILMETHOD_DECODER* pILHeader, size_t cbCode, size_t cbLocalSig, UINT maxStack);
VOID FreeGeneratedIL(COR_ILMETHOD_DECODER* pILHeader);

class StubState
{
public:
    virtual void SetStackPopSize(UINT cbStackPop) = 0;
    virtual void SetHas64BitReturn(BOOL fHas64BitReturn) = 0;
    virtual void SetLastError(BOOL fSetLastError) = 0;
    virtual void SetThisCall(BOOL fThisCall) = 0;
    virtual void SetDoHRESULTSwapping(BOOL fDoHRESULTSwapping) = 0;
    virtual void SetRetValIsGcRef(void) = 0;
    virtual void BeginEmit(void) = 0;
    virtual void SetHasThisCallHiddenArg(BOOL fHasHiddenArg) = 0;
    virtual void SetSrc(UINT uOffset) = 0;
    virtual void SetManagedReturnBuffSizeSmall(UINT cbSize) = 0;
    virtual void SetManagedReturnBuffSizeLarge(UINT cbSize) = 0;
    virtual void MarshalReturn(MarshalInfo* pInfo) = 0;
    virtual void MarshalArgument(MarshalInfo* pInfo) = 0;
    virtual void MarshalLCID(void) = 0;
    virtual void DoFixup(UINT cbFixup) = 0;
    virtual void DoDstFixup(UINT cbFixup) = 0;
    virtual Stub* FinishEmit(void) = 0;

};


class MLStubState : public StubState
{
public:
    MLStubState()
    {
        ZeroMemory(&m_header, sizeof(m_header));
    }

    void SetStackPopSize(UINT cbStackPop)
    {
        m_header.m_cbStackPop = cbStackPop;
    }

    void SetHas64BitReturn(BOOL fHas64BitReturn)
    {
        if (fHas64BitReturn)
        {
            m_header.m_Flags |= MLHF_64BITMANAGEDRETVAL;
        }
        else
        {
            m_header.m_Flags &= ~MLHF_64BITMANAGEDRETVAL;
        }
    }

    void SetLastError(BOOL fSetLastError)
    {
        if (fSetLastError)
        {
            m_header.m_Flags |= MLHF_SETLASTERROR;
        }
        else
        {
            m_header.m_Flags &= ~MLHF_SETLASTERROR;
        }
    }

    void SetThisCall(BOOL fThisCall)
    {
        if (fThisCall)
        {
            m_header.m_Flags |= MLHF_THISCALL;
        }
        else
        {
            m_header.m_Flags &= ~MLHF_THISCALL;
        }
    }

    void SetDoHRESULTSwapping(BOOL fDoHRESULTSwapping)
    {
        m_fDoHRESULTSwapping = fDoHRESULTSwapping;
    }

    void SetRetValIsGcRef(void)
    {
        m_header.SetManagedRetValTypeCat(MLHF_TYPECAT_GCREF);
    }

    void BeginEmit(void)
    {
        m_sl.MLEmitSpace(sizeof(m_header));
    }

    void SetHasThisCallHiddenArg(BOOL fHasHiddenArg)
    {
        if (fHasHiddenArg)
        {
            m_header.m_Flags |= MLHF_THISCALLHIDDENARG;
        }
        else
        {
            m_header.m_Flags &= ~MLHF_THISCALLHIDDENARG;
        }
    }

    void SetSrc(UINT uOffset)
    {
        _ASSERTE((INT32)uOffset == (INT32)((INT16)uOffset));

        m_sl.MLEmit(ML_CAPTURE_PSRC);
        m_sl.Emit16((UINT16)uOffset);
    }

    void SetManagedReturnBuffSizeSmall(UINT cbSize)
    {
        m_slRet.MLEmit(ML_MARSHAL_RETVAL_SMBLITTABLEVALUETYPE_C2N);
        m_slRet.Emit32(cbSize);
        m_slRet.Emit16(m_sl.MLNewLocal(sizeof(BYTE*)));
    }

    void SetManagedReturnBuffSizeLarge(UINT cbSize)
    {
        THROWSCOMPLUSEXCEPTION();

        m_sl.MLEmit(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N);
        m_sl.Emit32(cbSize);
        m_slPost.MLEmit(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_POST);
        m_slPost.Emit16(m_sl.MLNewLocal(sizeof(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_SR)));
        if (!SafeAddUINT16(&(m_header.m_cbDstBuffer), MLParmSize(sizeof(LPVOID))))
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
        }
    }

    void MarshalReturn(MarshalInfo* pInfo)
    {
        THROWSCOMPLUSEXCEPTION();

        pInfo->GenerateReturnML(&m_sl, 
                                &m_slRet,
                                TRUE,
                                m_fDoHRESULTSwapping);

        if (pInfo->IsFpu())
        {
            if (pInfo->GetMarshalType() == MarshalInfo::MARSHAL_TYPE_DOUBLE && !m_fDoHRESULTSwapping)
            {
                m_header.m_Flags |= MLHF_64BITUNMANAGEDRETVAL;
            }
                
            m_header.SetManagedRetValTypeCat(MLHF_TYPECAT_FPU);
            if (!m_fDoHRESULTSwapping) 
            {

                m_header.SetUnmanagedRetValTypeCat(MLHF_TYPECAT_FPU);
            }
        }

        if (!SafeAddUINT16(&(m_header.m_cbDstBuffer), pInfo->GetNativeArgSize()))
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
        }
    }

    void MarshalArgument(MarshalInfo* pInfo)
    {
        THROWSCOMPLUSEXCEPTION();

        pInfo->GenerateArgumentML(&m_sl, &m_slPost, TRUE);
        if (!SafeAddUINT16(&(m_header.m_cbDstBuffer), pInfo->GetNativeArgSize()))
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
        }
    }

    void MarshalLCID(void)
    {
        THROWSCOMPLUSEXCEPTION();

        m_sl.MLEmit(ML_LCID_C2N);
        if (!SafeAddUINT16(&(m_header.m_cbDstBuffer), sizeof(LCID)))
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
        }
    }

    void DoFixup(UINT cbFixup)
    {
        _ASSERTE((INT32)cbFixup == (INT32)((INT16)cbFixup));

        m_sl.Emit8(ML_BUMPSRC);
        m_sl.Emit16((INT16)cbFixup);
    }

    void DoDstFixup(UINT cbFixup)
    {
        _ASSERTE((INT32)cbFixup == (INT32)((INT16)cbFixup));

        m_sl.Emit8(ML_BUMPDST);
        m_sl.Emit16((INT16)cbFixup);
    }

    Stub* FinishEmit(void)
    {
        THROWSCOMPLUSEXCEPTION();

        // This marker separates the pre from the post work.
        m_sl.MLEmit(ML_INTERRUPT);

        // First emit code to do any backpropagation/cleanup work (this
        // was generated into a separate stublinker during the argument phase.)
        // Then emit the code to do the return value marshaling.
        m_slPost.MLEmit(ML_END);
        m_slRet.MLEmit(ML_END);

        Stub* pStubPost = NULL;
        Stub* pStubRet = NULL;

        EE_TRY_FOR_FINALLY
        {
            pStubPost = m_slPost.Link();
            pStubRet = m_slRet.Link();
            if (m_fDoHRESULTSwapping) 
            {
                m_sl.MLEmit(ML_THROWIFHRFAILED);
            }
            m_sl.EmitBytes(pStubPost->GetEntryPoint(), MLStreamLength((const UINT8 *)(pStubPost->GetEntryPoint())) - 1);
            m_sl.EmitBytes(pStubRet->GetEntryPoint(), MLStreamLength((const UINT8 *)(pStubRet->GetEntryPoint())) - 1);            
        } 
        EE_FINALLY
        {
            if (pStubRet)
                pStubRet->DecRef();
            if (pStubPost)
                pStubPost->DecRef();
        } 
        EE_END_FINALLY

        m_sl.MLEmit(ML_END);

        Stub* pStub = m_sl.Link();

        m_header.m_cbLocals = m_sl.GetLocalSize();

        *((MLHeader *)(pStub->GetEntryPoint())) = m_header;

#ifdef _DEBUG
        if (0)
        {
            MLHeader *pheader = (MLHeader*)(pStub->GetEntryPoint());
            UINT16 locals = 0;
            MLCode opcode;
            const MLCode *pMLCode = pheader->GetMLCode();


            VOID DisassembleMLStream(const MLCode *pMLCode);
            //DisassembleMLStream(pMLCode);

            while (ML_INTERRUPT != (opcode = *(pMLCode++))) {
                locals += gMLInfo[opcode].m_cbLocal;
                pMLCode += gMLInfo[opcode].m_numOperandBytes;
            }
            _ASSERTE(locals == pheader->m_cbLocals);
        }
#endif //_DEBUG

        return pStub;
    }

protected:
    MLStubLinker m_sl;
    MLStubLinker m_slPost;
    MLStubLinker m_slRet;

    MLHeader     m_header;
    BOOL         m_fDoHRESULTSwapping;
};


static Stub*  CreateNDirectStubWorker(StubState*        pStubState,
                                      PCCOR_SIGNATURE   szMetaSig,
                                      Module*           pModule,
                                      mdMethodDef       md,
                                      BYTE              nlType,
                                      BYTE              nlFlags,
                                      CorPinvokeMap     unmgdCallConv,
                                      BOOL              fConvertSigAsVarArg
#ifdef _DEBUG
                                      ,
                                      LPCUTF8 pDebugName,
                                      LPCUTF8 pDebugClassName,
                                      LPCUTF8 pDebugNameSpace
#endif
                                );


#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif

VOID NDirect_Prelink(MethodDesc *pMeth);

ArgBasedStubCache    *NDirect::m_pNDirectGenericStubCache = NULL;
ArgBasedStubCache    *NDirect::m_pNDirectSlimStubCache    = NULL;

// support for Pinvoke Calli instruction


NDirectMLStubCache *NDirect::m_pNDirectMLStubCache = NULL;


class NDirectMLStubCache : public MLStubCache
{
    public:
        NDirectMLStubCache(LoaderHeap *heap = 0) : MLStubCache(heap) {}

    private:
        //---------------------------------------------------------
        // Compile a native (ASM) version of the ML stub.
        //
        // This method should compile into the provided stublinker (but
        // not call the Link method.)
        //
        // It should return the chosen compilation mode.
        //
        // If the method fails for some reason, it should return
        // INTERPRETED so that the EE can fall back on the already
        // created ML code.
        //---------------------------------------------------------
        virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                                                    StubLinker *pstublinker,
                                                    void *callerContext);

        //---------------------------------------------------------
        // Tells the MLStubCache the length of an ML stub.
        //---------------------------------------------------------
        virtual UINT Length(const BYTE *pRawMLStub)
        {
            CANNOTTHROWCOMPLUSEXCEPTION();
            MLHeader *pmlstub = (MLHeader *)pRawMLStub;
            return sizeof(MLHeader) + MLStreamLength(pmlstub->GetMLCode());
        }


};


//---------------------------------------------------------
// Compile a native (ASM) version of the ML stub.
//
// This method should compile into the provided stublinker (but
// not call the Link method.)
//
// It should return the chosen compilation mode.
//
// If the method fails for some reason, it should return
// INTERPRETED so that the EE can fall back on the already
// created ML code.
//---------------------------------------------------------
MLStubCache::MLStubCompilationMode NDirectMLStubCache::CompileMLStub(const BYTE *pRawMLStub,
                     StubLinker *pstublinker, void *callerContext)
{
    MLStubCompilationMode ret = INTERPRETED;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(callerContext == 0);


    COMPLUS_TRY {
        CPUSTUBLINKER *psl = (CPUSTUBLINKER *)pstublinker;
        const MLHeader *pheader = (const MLHeader *)pRawMLStub;

#ifdef _DEBUG
        if (LoggingEnabled() && (g_pConfig->GetConfigDWORD(L"LogFacility",0) & LF_IJW)) {
            COMPLUS_LEAVE;
            }
#endif

        if (!(MonDebugHacksOn() || TueDebugHacksOn())) {
            if (NDirect::CreateStandaloneNDirectStubSys(pheader, (CPUSTUBLINKER*)psl, FALSE)) {
                ret = STANDALONE;
                COMPLUS_LEAVE;
            }
        }


    } COMPLUS_CATCH {
        ret = INTERPRETED;
    } COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

#ifdef CUSTOMER_CHECKED_BUILD
    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_StackImbalance))
    {
        // Force a GenericNDirectStub if we are checking for stack imbalance
        ret = INTERPRETED;
    }
#endif // CUSTOMER_CHECKED_BUILD

    return ret;
}


/*static*/
LPVOID NDirect::NDirectGetEntryPoint(NDirectMethodDesc *pMD, HINSTANCE hMod, UINT16 numParamBytes)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // GetProcAddress cannot be called while preemptive GC is disabled.
    // It requires the OS to take the loader lock.
    _ASSERTE(!(GetThread()->PreemptiveGCDisabled()));

#if !PLATFORM_UNIX
    // Handle ordinals on Windows.
    if (pMD->ndirect.m_szEntrypointName[0] == '#') {
        long ordinal = atol(pMD->ndirect.m_szEntrypointName+1);
        return (LPVOID) GetProcAddress(hMod, (LPCSTR)((UINT16)ordinal));
    }
#endif  // !PLATFORM_UNIX

    LPVOID pFunc, pFuncW;

    // Just look for the unmangled name.  If nlType != nltAnsi, we are going
    // to need to check for the 'W' API because it takes precedence over the
    // unmangled one (on NT some APIs have unmangled ANSI exports).
    if (((pFunc = (LPVOID) GetProcAddress(hMod, pMD->ndirect.m_szEntrypointName)) != NULL && pMD->GetNativeLinkType() == nltAnsi) ||
        (pMD->GetNativeLinkFlags() & nlfNoMangle))
        return pFunc;

    // Allocate space for a copy of the entry point name.
    int dstbufsize = (int)(sizeof(char) * (strlen(pMD->ndirect.m_szEntrypointName) + 1 + 20)); // +1 for the null terminator
                                                                         // +20 for various decorations
    LPSTR szAnsiEntrypointName = (LPSTR)_alloca(dstbufsize);

    // Make room for the preceeding '_' we might try adding.
    szAnsiEntrypointName++;

    // Copy the name so we can mangle it.
    strcpy(szAnsiEntrypointName,pMD->ndirect.m_szEntrypointName);
    DWORD nbytes = (DWORD)(strlen(pMD->ndirect.m_szEntrypointName) + 1);
    szAnsiEntrypointName[nbytes] = '\0'; // Add an extra '\0'.


    // If the program wants the ANSI api or if Unicode APIs are unavailable.
    if (pMD->GetNativeLinkType() == nltAnsi) {
        szAnsiEntrypointName[nbytes-1] = 'A';
        pFunc = (LPVOID) GetProcAddress(hMod, szAnsiEntrypointName);
    }
    else {
        szAnsiEntrypointName[nbytes-1] = 'W';
        pFuncW = (LPVOID) GetProcAddress(hMod, szAnsiEntrypointName);

        // This overrides the unmangled API. See the comment above.
        if (pFuncW != NULL)
            pFunc = pFuncW;
    }

    if (!pFunc) {


#if PLATFORM_WIN32
        /* try mangled names only for __stdcalls */

        if (!pFunc && (numParamBytes != 0xffff)) {
            szAnsiEntrypointName[-1] = '_';
            sprintf(szAnsiEntrypointName + nbytes - 1, "@%ld", (ULONG)numParamBytes);
            pFunc = GetProcAddress(hMod, szAnsiEntrypointName - 1);
        }
#endif  // PLATFORM_WIN32
    }

    return pFunc;

}

static BOOL AbsolutePath(LPCWSTR wszLibName, DWORD* pdwSize)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // check for UNC or a drive
    WCHAR* ptr = (WCHAR*) wszLibName;
    WCHAR* start = ptr;
    *pdwSize = 0;
    start = ptr;

    // Check for UNC path 
    while(*ptr) {
        if(*ptr != L'\\')
            break;
        ptr++;
    }

    if((ptr - wszLibName) == 2)
        return TRUE;
    else {
        // Check to see if there is a colon indicating a drive or protocal
        for(ptr = start; *ptr; ptr++) {
            if(*ptr == L':')
                break;
        }
        if(*ptr != NULL)
            return TRUE;
    }
    
    // We did not find a 
    *pdwSize = (DWORD)(ptr - wszLibName);
    return FALSE;
}

//---------------------------------------------------------
// Loads the DLL and finds the procaddress for an N/Direct call.
//---------------------------------------------------------
/* static */
VOID NDirect::NDirectLink(NDirectMethodDesc *pMD, UINT16 numParamBytes)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();
    pThread->EnablePreemptiveGC();
    BOOL fSuccess = FALSE;
    HINSTANCE hmod = NULL;
    AppDomain* pDomain = pThread->GetDomain();
    
    MAKE_WIDEPTR_FROMUTF8(wszLibName, pMD->ndirect.m_szLibName);

    hmod = pDomain->FindUnmanagedImageInCache(wszLibName);
    if(hmod == NULL) {


        DWORD dwSize = 0;

        if(AbsolutePath(wszLibName, &dwSize)) {
            hmod = WszLoadLibrary(wszLibName);
        }
        else { 
            WCHAR buffer[_MAX_PATH];
            DWORD dwLength = _MAX_PATH;
            LPWSTR pCodeBase;
            Assembly* pAssembly = pMD->GetClass()->GetAssembly();
            
            if(SUCCEEDED(pAssembly->GetCodeBase(&pCodeBase, &dwLength)) &&
               dwSize + dwLength < _MAX_PATH) 
            {
                WCHAR* ptr;
                // Strip off the protocol
                for(ptr = pCodeBase; *ptr && *ptr != L':'; ptr++);

                // If we have a code base then prepend it to the library name
                if(*ptr) {
                    WCHAR* pBuffer = buffer;

                    // After finding the colon move forward until no more forward slashes
                    for(ptr++; *ptr && *ptr == L'/'; ptr++);
#if PLATFORM_UNIX
                    // On Unix, we need one slash to have a valid absolute
                    // path. Back up so we don't lose it.
                    if (ptr > pCodeBase && *(ptr - 1) == L'/')
                    {
                        ptr--;
                    }
#endif  // PLATFORM_UNIX
                    if(*ptr) {
                        // Calculate the number of charachters we are interested in
                        dwLength -= (DWORD)(ptr - pCodeBase);
                        if(dwLength > 0) {
                            // Back up to the last slash (forward or backwards)
                            WCHAR* tail;
                            for(tail = ptr+(dwLength-1); tail > ptr && *tail != L'/' && *tail != L'\\'; tail--);
                            if(tail > ptr) {
                                for(;ptr <= tail; ptr++, pBuffer++) {
                                    if(*ptr == L'/') 
                                        *pBuffer = L'\\';
                                    else
                                        *pBuffer = *ptr;
                                }
                            }
                        }
                    }
                    wcsncpy(pBuffer, wszLibName, dwSize+1);
                    hmod = WszLoadLibrary(buffer);
                }
            }
        }

        // Do we really need to do this. This call searches the application directory
        // instead of the location for the library.
        if(hmod == NULL)
            hmod = WszLoadLibrary(wszLibName);


        // This may be an assembly name
        if (!hmod) {
            // Format is "fileName, assemblyDisplayName"
            MAKE_UTF8PTR_FROMWIDE(szLibName, wszLibName);
            char *szComma = strchr(szLibName, ',');
            if (szComma) {
                *szComma = '\0';
                while (COMCharacter::nativeIsWhiteSpace(*(++szComma)));

                AssemblySpec spec;
                if (SUCCEEDED(spec.Init(szComma))) {
                    Assembly *pAssembly;
                    if (SUCCEEDED(spec.LoadAssembly(&pAssembly, NULL /*pThrowable*/, NULL))) {

                        HashDatum datum;
                        if (pAssembly->m_pAllowedFiles->GetValue(szLibName, &datum)) {
                            const BYTE* pHash;
                            DWORD dwFlags = 0;
                            ULONG dwHashLength = 0;
                            pAssembly->GetManifestImport()->GetFileProps((mdFile)UnsafePointerTruncate(datum),
                                                                         NULL, //&szModuleName,
                                                                         (const void**) &pHash,
                                                                         &dwHashLength,
                                                                         &dwFlags);
                            
                            WCHAR pPath[MAX_PATH];
                            Module *pModule;
                            if (SUCCEEDED(pAssembly->LoadInternalModule(szLibName,
                                                                        NULL, // mdFile
                                                                        pAssembly->m_ulHashAlgId,
                                                                        pHash,
                                                                        dwHashLength,
                                                                        dwFlags,
                                                                        pPath,
                                                                        MAX_PATH,
                                                                        &pModule,
                                                                        NULL /*pThrowable*/)))
                                hmod = WszLoadLibrary(pPath);
                        }
                    }
                }
            }
        }
    
        // After all this, if we have a handle add it to the cache.
        if(hmod) {
            HRESULT hrResult = pDomain->AddUnmanagedImageToCache(wszLibName, hmod);
            if(hrResult == S_FALSE) 
                FreeLibrary(hmod);
        }
    }

    if (hmod)
    {
        LPVOID pvTarget = NDirectGetEntryPoint(pMD, hmod, numParamBytes);
        if (pvTarget) {
            pMD->SetNDirectTarget(pvTarget);
            fSuccess = TRUE;
        }
    }

    pThread->DisablePreemptiveGC();


    if (!fSuccess) {
        if (!hmod) {
            COMPlusThrow(kDllNotFoundException, IDS_EE_NDIRECT_LOADLIB, wszLibName);
        }

        WCHAR wszEPName[50];
        if(WszMultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pMD->ndirect.m_szEntrypointName, -1, wszEPName, sizeof(wszEPName)/sizeof(WCHAR)) == 0)
        {
            wszEPName[0] = L'?';
            wszEPName[1] = L'\0';
        }

        COMPlusThrow(kEntryPointNotFoundException, IDS_EE_NDIRECT_GETPROCADDRESS, wszLibName, wszEPName);
    }


}





//---------------------------------------------------------
// One-time init
//---------------------------------------------------------
/*static*/ BOOL NDirect::Init()
{
    BOOL fSuccess = TRUE;

    if ((m_pNDirectMLStubCache = new NDirectMLStubCache(SystemDomain::System()->GetStubHeap())) == NULL) {
        return FALSE;
    }
    if ((m_pNDirectGenericStubCache = new ArgBasedStubCache()) == NULL) {
        return FALSE;
    }
    if ((m_pNDirectSlimStubCache = new ArgBasedStubCache()) == NULL) {
        return FALSE;
    }


    return fSuccess;
}

//---------------------------------------------------------
// One-time cleanup
//---------------------------------------------------------

//---------------------------------------------------------
// Computes an ML stub for the ndirect method, and fills
// in the corresponding fields of the method desc. Note that
// this does not set the m_pMLHeader field of the method desc,
// since the stub may end up being replaced by a compiled one
//---------------------------------------------------------
/* static */
Stub* NDirect::ComputeNDirectMLStub(NDirectMethodDesc *pMD) 
{
    THROWSCOMPLUSEXCEPTION();
    
    if (pMD->IsSynchronized()) {
        COMPlusThrow(kTypeLoadException, IDS_EE_NOSYNCHRONIZED);
    }

    BYTE ndirectflags = 0;
    BOOL fVarArg = pMD->MethodDesc::IsVarArg();
    NDirectMethodDesc::SetVarArgsBits(&ndirectflags, fVarArg);


    if (fVarArg && pMD->ndirect.m_szEntrypointName != NULL)
        return NULL;

    CorNativeLinkType type;
    CorNativeLinkFlags flags;
    LPCUTF8 szLibName = NULL;
    LPCUTF8 szEntrypointName = NULL;
    Stub *pMLStub = NULL;

    CorPinvokeMap unmanagedCallConv;

    CalculatePinvokeMapInfo(pMD, &type, &flags, &unmanagedCallConv, &szLibName, &szEntrypointName);

    NDirectMethodDesc::SetNativeLinkTypeBits(&ndirectflags, type);
    NDirectMethodDesc::SetNativeLinkFlagsBits(&ndirectflags, flags);
    NDirectMethodDesc::SetStdCallBits(&ndirectflags, unmanagedCallConv == pmCallConvStdcall);

    // Call this exactly ONCE per thread. Do not publish incomplete prestub flags
    // or you will introduce a race condition.
    pMD->PublishPrestubFlags(ndirectflags);


    pMD->ndirect.m_szLibName = szLibName;
    pMD->ndirect.m_szEntrypointName = szEntrypointName;

    if (pMD->IsVarArgs())
        return NULL;

    OBJECTREF pThrowable;

    PCCOR_SIGNATURE pMetaSig;
    DWORD       cbMetaSig;
    
    pMD->GetSig(&pMetaSig, &cbMetaSig);

    pMLStub = CreateNDirectMLStub(pMetaSig, pMD->GetModule(), pMD->GetMemberDef(),
                                  type, flags, unmanagedCallConv,
                                  &pThrowable, FALSE
#ifdef CUSTOMER_CHECKED_BUILD
                                  ,pMD
#endif
#ifdef _DEBUG
                                  ,
                                  pMD->m_pszDebugMethodName,
                                  pMD->m_pszDebugClassName,
                                  NULL
#endif
                                  );
    if (!pMLStub)
    {
        COMPlusThrow(pThrowable);
    }

    MLHeader *pMLHeader = (MLHeader*) pMLStub->GetEntryPoint();

    pMD->ndirect.m_cbDstBufSize = pMLHeader->m_cbDstBuffer;

    return pMLStub;
}


//---------------------------------------------------------
// Either creates or retrieves from the cache, a stub to
// invoke NDirect methods. Each call refcounts the returned stub.
// This routines throws a COM+ exception rather than returning
// NULL.
//---------------------------------------------------------
/* static */
Stub* NDirect::GetNDirectMethodStub(StubLinker *pstublinker, NDirectMethodDesc *pMD)
{
    THROWSCOMPLUSEXCEPTION();

    MLHeader *pOldMLHeader = NULL;
    MLHeader *pMLHeader = NULL;
    Stub  *pTempMLStub = NULL;
    Stub  *pReturnStub = NULL;

    EE_TRY_FOR_FINALLY {

        // the ML header will already be set if we were prejitted
        pOldMLHeader = pMD->GetMLHeader();
        pMLHeader = pOldMLHeader;

        if (pMLHeader == NULL) {
            pTempMLStub = ComputeNDirectMLStub(pMD);
            if (pTempMLStub != NULL)
                pMLHeader = (MLHeader *) pTempMLStub->GetEntryPoint();
        }

        if (pMD->GetSubClassification() == NDirectMethodDesc::kLateBound)
        {
            NDirectLink(pMD, pMD->IsStdCall() ? pMLHeader->m_cbDstBuffer : 0xffff);
        }



        MLStubCache::MLStubCompilationMode mode;
        Stub *pCanonicalStub;

        if (pMLHeader == NULL) {
            pCanonicalStub = NULL;
            mode = MLStubCache::INTERPRETED;
        } else {
            pCanonicalStub = NDirect::m_pNDirectMLStubCache->Canonicalize((const BYTE *) pMLHeader,
                                                                          &mode);
            if (!pCanonicalStub) {
                COMPlusThrowOM();
            }
        }

#ifdef CUSTOMER_CHECKED_BUILD
        CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
#endif

        switch (mode) {

            case MLStubCache::INTERPRETED:
                if (pCanonicalStub != NULL) // it will be null for varags case
                {
                    if (!pMD->InterlockedReplaceMLHeader((MLHeader*)pCanonicalStub->GetEntryPoint(),
                                                         pOldMLHeader))
                        pCanonicalStub->DecRef();
                }

                if (pMLHeader == NULL || MonDebugHacksOn() || WedDebugHacksOn()
#ifdef _DEBUG
                    || (LoggingEnabled() && (LF_IJW & g_pConfig->GetConfigDWORD(L"LogFacility", 0)))
#endif

                    ) {
                    pReturnStub = NULL;
                } else {

#ifdef CUSTOMER_CHECKED_BUILD
                    if (!pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_StackImbalance)) {
                        pReturnStub = CreateSlimNDirectStub(pstublinker, pMD,
                                                            pMLHeader->m_cbStackPop);
                    }
                    else {
                        // Force a GenericNDirectStub if we are checking for stack imbalance
                        pReturnStub = NULL;
                    }
#else
                    pReturnStub = CreateSlimNDirectStub(pstublinker, pMD,
                                                        pMLHeader->m_cbStackPop);
#endif // CUSTOMER_CHECKED_BUILD

                }
                if (!pReturnStub) {
                    // Note that we don't want to call CbStackBytes unless
                    // strictly necessary since this will touch metadata.  
                    // Right now it will only happen for the varags case,
                    // which probably needs other tuning anyway.
                    pReturnStub = CreateGenericNDirectStub(pstublinker,
                                                           pMLHeader != NULL ? 
                                                           pMLHeader->m_cbStackPop :
                                                           pMD->CbStackPop());
                }
                break;

            case MLStubCache::SHAREDPROLOG:
                if (!pMD->InterlockedReplaceMLHeader((MLHeader*)pCanonicalStub->GetEntryPoint(),
                                                     pOldMLHeader))
                    pCanonicalStub->DecRef();
                _ASSERTE(!"NYI");
                pReturnStub = NULL;
                break;

            case MLStubCache::STANDALONE:
                pReturnStub = pCanonicalStub;
                break;

            default:
                _ASSERTE(0);
        }


    } EE_FINALLY {
        if (pTempMLStub) {
            pTempMLStub->DecRef();
        }
    } EE_END_FINALLY;

    return pReturnStub;
}


//---------------------------------------------------------
// Creates or retrives from the cache, the generic NDirect stub.
//---------------------------------------------------------
/* static */
Stub* NDirect::CreateGenericNDirectStub(StubLinker *pstublinker, UINT numStackBytes)
{
    THROWSCOMPLUSEXCEPTION();

    Stub *pStub = m_pNDirectGenericStubCache->GetStub(numStackBytes);
    if (pStub) {
        return pStub;
    } else {

        CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;

        CreateGenericNDirectStubSys(psl, numStackBytes);

        Stub *pCandidate = psl->Link(SystemDomain::System()->GetStubHeap());
        Stub *pWinner = m_pNDirectGenericStubCache->AttemptToSetStub(numStackBytes,pCandidate);
        pCandidate->DecRef();
        if (!pWinner) {
            COMPlusThrowOM();
        }
        return pWinner;
    }
}


//---------------------------------------------------------
// Call at strategic times to discard unused stubs.
//---------------------------------------------------------


//---------------------------------------------------------
// Helper function to checkpoint the thread allocator for cleanup.
//---------------------------------------------------------
VOID __stdcall DoCheckPointForCleanup(NDirectMethodFrameEx *pFrame, Thread *pThread)
{
        THROWSCOMPLUSEXCEPTION();

        CleanupWorkList *pCleanup = pFrame->GetCleanupWorkList();
        if (pCleanup) {
            // Checkpoint the current thread's fast allocator (used for temporary
            // buffers over the call) and schedule a collapse back to the checkpoint in
            // the cleanup list. Note that if we need the allocator, it is
            // guaranteed that a cleanup list has been allocated.
            void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
            pCleanup->ScheduleFastFree(pCheckpoint);
            pCleanup->IsVisibleToGc();
        }
}



//---------------------------------------------------------
// Performs an N/Direct call. This is a generic version
// that can handle any N/Direct call but is not as fast
// as more specialized versions.
//---------------------------------------------------------


#ifdef _X86_
// do not call this - it is pointer to instruction within NDirectGenericStubWorker
extern "C" void __stdcall NDirectGenericReturnFromCall(void);
#else
extern "C" PVOID NDirectGenericReturnFromCall;
#endif

#if defined(_X86_) && (defined(_DEBUG) || defined(CUSTOMER_CHECKED_BUILD))
static const int NDirectGenericWorkerStackSize = NDirectGenericWorkerFrameSize_DEBUG;
#else
static const int NDirectGenericWorkerStackSize = NDirectGenericWorkerFrameSize;
#endif



EXTERN_C
int 
__stdcall
NDirectGenericStubComputeFrameSize(
    Thread *pThread,
    NDirectMethodFrame *pFrame,
    MLHeader **ppheader
    )
{
    THROWSCOMPLUSEXCEPTION();
    NDirectMethodDesc *pMD = (NDirectMethodDesc*)(pFrame->GetFunction());
    MLHeader *pheader;

    LOG((LF_STUBS, LL_INFO1000, "Calling NDirectGenericStubWorker %s::%s \n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName));

    if (pMD->IsVarArgs()) {

        VASigCookie *pVASigCookie = pFrame->GetVASigCookie();

        Stub *pTempMLStub;

        if (pVASigCookie->pNDirectMLStub != NULL) {
            pTempMLStub = pVASigCookie->pNDirectMLStub;
        } else {
            OBJECTREF pThrowable;
            CorNativeLinkType type;
            CorNativeLinkFlags flags;
            LPCUTF8 szLibName = NULL;
            LPCUTF8 szEntrypointName = NULL;
            CorPinvokeMap unmanagedCallConv;

            CalculatePinvokeMapInfo(pMD, &type, &flags, &unmanagedCallConv, &szLibName, &szEntrypointName);

            // We are screwed here because the VASigCookie doesn't hash on or store the NATIVE_TYPE metadata.
            // Right now, we just pass the bogus value "mdMethodDefNil" which will cause CreateNDirectMLStub to use
            // the defaults.
            pTempMLStub = CreateNDirectMLStub(pVASigCookie->mdVASig, pVASigCookie->pModule, mdMethodDefNil, type, flags, unmanagedCallConv, &pThrowable, FALSE
#ifdef CUSTOMER_CHECKED_BUILD
                                              ,pMD
#endif
                                              );
            if (!pTempMLStub)
            {
                COMPlusThrow(pThrowable);
            }

            if (NULL != FastInterlockCompareExchangePointer( (void*volatile*)&(pVASigCookie->pNDirectMLStub),
                                                       pTempMLStub,
                                                       NULL )) {
                pTempMLStub->DecRef();
                pTempMLStub = pVASigCookie->pNDirectMLStub;
            }
        }
        pheader = (MLHeader*)(pTempMLStub->GetEntryPoint());

    } else {
        pheader = pMD->GetMLHeader();
    }

    // Tell our caller to allocate enough memory to store both the 
    // destination buffer and the locals.
    *ppheader = pheader;
    return pheader->m_cbDstBuffer + pheader->m_cbLocals;
}

EXTERN_C
int
__stdcall
NDirectGenericStubBuildArguments(
    Thread *pThread,
    NDirectMethodFrame *pFrame,
    MLHeader *pheader,
    MLCode const **ppMLCode,
    BYTE **pplocals,
    LPVOID *ppvFn,
    BYTE *pAlloc
    )
{
#ifdef _DEBUG
    FillMemory(pAlloc, pheader->m_cbDstBuffer + pheader->m_cbLocals, 0xcc);
#endif

    BYTE   *pdst    = pAlloc;
    BYTE   *plocals = pdst + pheader->m_cbDstBuffer;

#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
    pdst += pheader->m_cbDstBuffer;
#endif

    VOID   *psrc          = (VOID*)pFrame;

    CleanupWorkList *pCleanup = pFrame->GetCleanupWorkList();

    if (pCleanup) {

        // Checkpoint the current thread's fast allocator (used for temporary
        // buffers over the call) and schedule a collapse back to the 
        // checkpoint in the cleanup list. Note that if we need the allocator,
        // it is guaranteed that a cleanup list has been allocated.
        void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
        pCleanup->ScheduleFastFree(pCheckpoint);
        pCleanup->IsVisibleToGc();
    }

#ifdef _PPC_
    // float registers
    DOUBLE *pFloatRegs = (DOUBLE*)(pdst + 
        AlignStack(pheader->m_cbLocals + pheader->m_cbDstBuffer 
            + NUM_FLOAT_ARGUMENT_REGISTERS * sizeof(DOUBLE)) - NUM_FLOAT_ARGUMENT_REGISTERS * sizeof(DOUBLE));
    ZeroMemory(pFloatRegs, NUM_FLOAT_ARGUMENT_REGISTERS * sizeof(DOUBLE));
#endif

    // Call the ML interpreter to translate the arguments. Assuming
    // it returns, we get back a pointer to the succeeding code stream
    // which we will save away for post-call execution.
    *ppMLCode = RunML(pheader->GetMLCode(),
                      psrc,
                      pdst,
                      (UINT8*const) plocals,
                      pCleanup
#ifdef _PPC_
                      , pFloatRegs
#endif
                      );

    NDirectMethodDesc *pMD = (NDirectMethodDesc*)(pFrame->GetFunction());

    LOG((LF_IJW, LL_INFO1000, "P/Invoke call (\"%s.%s%s\")\n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName, pMD->m_pszDebugMethodSignature));

#ifdef STRESS_COMCALL
    if (g_pConfig->GetStressComCall()) {
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait();
    }
#endif

    // Call the target.
    pThread->EnablePreemptiveGC();

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of call out of the runtime
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            ManagedToUnmanagedTransition((FunctionID) pMD,
                                               COR_PRF_TRANSITION_CALL);
    }
#endif // PROFILING_SUPPORTED

    *ppvFn = pMD->GetNDirectTarget();

#if _DEBUG
    //
    // Call through debugger routines to double check their
    // implementation
    //
    *ppvFn = (void*) CheckExitFrameDebuggerCalls;
#endif

    *pplocals = plocals;
    return pheader->m_Flags;

}

EXTERN_C
INT64
__stdcall
NDirectGenericStubPostCall(
    Thread *pThread,
    NDirectMethodFrame *pFrame,
    MLHeader *pheader,
    MLCode const *pMLCode,
    BYTE *plocals,
    INT64 nativeReturnValue
#if defined(_X86_) && (defined(_DEBUG) || defined(CUSTOMER_CHECKED_BUILD))
    ,
    DWORD PreESP,
    DWORD PostESP
#endif
    )
{
    NDirectMethodDesc *pMD;

#if defined(_X86_) && defined(_DEBUG)
    _ASSERTE ((PreESP <= PostESP && PostESP <= PreESP + pheader->m_cbDstBuffer)
              ||!"esp is trashed by PInvoke call, possibly wrong signature");
#endif

    pMD = (NDirectMethodDesc*)(pFrame->GetFunction());

#if defined(_X86_) && defined(CUSTOMER_CHECKED_BUILD)

    DWORD cdh_EspBeforePushedArgs;
    DWORD cdh_PostEsp;

    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_StackImbalance))
    {
        cdh_PostEsp = PostESP;

        // Get expected calling convention

        CorNativeLinkType   type;
        CorNativeLinkFlags  flags;
        CorPinvokeMap       unmanagedCallConv;
        LPCUTF8             szLibName = NULL;
        LPCUTF8             szEntrypointName = NULL;

        CalculatePinvokeMapInfo(pMD, &type, &flags, &unmanagedCallConv, &szLibName, &szEntrypointName);

        BOOL bStackImbalance = false;

        switch( unmanagedCallConv )
        {

            // Caller cleans stack
            case pmCallConvCdecl:

                if (cdh_PostEsp != cdh_EspAfterPushedArgs)
                    bStackImbalance = true;
                break;

            // Callee cleans stack
            case pmCallConvThiscall:
                cdh_EspBeforePushedArgs = cdh_EspAfterPushedArgs + pheader->m_cbDstBuffer - sizeof(void*);
                if (cdh_PostEsp != cdh_EspBeforePushedArgs)
                    bStackImbalance = true;
                break;

            // Callee cleans stack
            case pmCallConvWinapi:
            case pmCallConvStdcall:
                cdh_EspBeforePushedArgs = cdh_EspAfterPushedArgs + pheader->m_cbDstBuffer;
                if (cdh_PostEsp != cdh_EspBeforePushedArgs)
                    bStackImbalance = true;
                break;

            // Unsupported calling convention
            case pmCallConvFastcall:
            default:
                _ASSERTE(!"Unsupported calling convention");
                break;
        }

        if (bStackImbalance)
        {
            CQuickArray<WCHAR> strEntryPointName;
            UINT32 iEntryPointNameLength = (UINT32) strlen(szEntrypointName) + 1;
            strEntryPointName.Alloc(iEntryPointNameLength);

            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szEntrypointName,
                                 -1, strEntryPointName.Ptr(), iEntryPointNameLength );

            CQuickArray<WCHAR> strLibName;
            UINT32 iLibNameLength = (UINT32) strlen(szLibName) + 1;
            strLibName.Alloc(iLibNameLength);

            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szLibName,
                                 -1, strLibName.Ptr(), iLibNameLength );

            static WCHAR strMessageFormat[] = {L"Stack imbalance may be caused by incorrect calling convention for method %s (%s)"};

            CQuickArray<WCHAR> strMessage;
            strMessage.Alloc(lengthof(strMessageFormat) + iEntryPointNameLength + iLibNameLength);

            Wszwsprintf(strMessage.Ptr(), strMessageFormat, strEntryPointName.Ptr(), strLibName.Ptr());
            pCdh->ReportError(strMessage.Ptr(), CustomerCheckedBuildProbe_StackImbalance);
        }
    }

#endif // _X86_ && CUSTOMER_CHECKED_BUILD

#ifdef BIGENDIAN
    // returnValue and nativeReturnValue are treated as 8 bytes buffers. The 32 bit values
    // will be in the high dword!
#endif

    if (pheader->GetUnmanagedRetValTypeCat() == MLHF_TYPECAT_FPU) {
        int fpNativeSize;
        if (pheader->m_Flags & MLHF_64BITUNMANAGEDRETVAL) {
            fpNativeSize = 8;
        } else {
            fpNativeSize = 4;
        }
        getFPReturn(fpNativeSize, &nativeReturnValue);
    }

    if (pheader->m_Flags & MLHF_SETLASTERROR) {
        pThread->m_dwLastError = GetLastError();
    }

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of return from call out of the runtime
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            UnmanagedToManagedTransition((FunctionID) pMD,
                                         COR_PRF_TRANSITION_RETURN);
    }
#endif // PROFILING_SUPPORTED

    pThread->DisablePreemptiveGC();
    if (g_TrapReturningThreads)
        pThread->HandleThreadAbort();

#ifdef STRESS_COMCALL
    if (g_pConfig->GetStressComCall()) {
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait();
    }
#endif

    INT64 returnValue = 0;
    CleanupWorkList *pCleanup = pFrame->GetCleanupWorkList();

    // Marshal the return value and propagate any [out] parameters back.
    RunML(pMLCode,
          &nativeReturnValue,
          ((BYTE*)&returnValue)
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
          + ((pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) ? 8 : 4)
#endif
          ,
          (UINT8*const)plocals,
          pCleanup);

    int managedRetValTypeCat = pheader->GetManagedRetValTypeCat();

    if (pCleanup) {
        if (managedRetValTypeCat == MLHF_TYPECAT_GCREF) {
            GCPROTECT_BEGIN(*(VOID**)&returnValue);
            pCleanup->Cleanup(FALSE);
            GCPROTECT_END();

        } else {
            pCleanup->Cleanup(FALSE);
        }
    }

    if (managedRetValTypeCat == MLHF_TYPECAT_FPU) {
        int fpComPlusSize;
        if (pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) {
            fpComPlusSize = 8;
        } else {
            fpComPlusSize = 4;
        }
        setFPReturn(fpComPlusSize, returnValue);
    }

    return returnValue;
}


// factorization of CreateNDirectStubWorker
static MarshalInfo::MarshalType DoMarshalReturnValue(MetaSig&       msig, 
                                                    ArgIterator&   ai,
                                                    Module*        pModule,
                                                    mdParamDef*    params,
                                                    BYTE           nlType,
                                                    BYTE           nlFlags,
                                                    BOOL           fDoHRESULTSwapping,
                                                    StubState*     pss,
                                                    BOOL           fThisCall,
                                                    int&           lastArgSize
#ifdef CUSTOMER_CHECKED_BUILD
                                                    ,
                                                    MethodDesc     pMD
#endif
#ifdef _DEBUG
                                                    ,
                                                    LPCUTF8 pDebugName,
                                                    LPCUTF8 pDebugClassName,
                                                    LPCUTF8 pDebugNameSpace
#endif
                                                    )
{
    THROWSCOMPLUSEXCEPTION();

    MarshalInfo::MarshalType marshalType = (MarshalInfo::MarshalType) 0xcccccccc;

    if (msig.GetReturnType() != ELEMENT_TYPE_VOID)
    {
        SigPointer pSig;
        pSig = msig.GetReturnProps();

        MarshalInfo returnInfo(pModule,
                                pSig,
                                params[0],
                                MarshalInfo::MARSHAL_SCENARIO_NDIRECT,
                                nlType,
                                nlFlags,
                                FALSE,
                                0
#ifdef _DEBUG
                                ,
                                pDebugName,
                                pDebugClassName,
                                pDebugNameSpace,
                                0
#endif

                                );

        marshalType = returnInfo.GetMarshalType();

        if (msig.HasRetBuffArg())
        {
            if (marshalType == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS)
            {
                if (fDoHRESULTSwapping)
                {
                    // V1 restriction: we could implement this but it's late in the game to do so.
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }

                MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                UINT         managedSize = msig.GetRetTypeHandle().GetSize();
                UINT         unmanagedSize = pMT->GetNativeSize();

                if (fThisCall)
                {
                    pss->SetHasThisCallHiddenArg(true);
                }

                if (IsManagedValueTypeReturnedByRef(managedSize) && 
                    !(fThisCall || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                {
                    pss->SetSrc(ai.GetRetBuffArgOffset());
                    pss->SetManagedReturnBuffSizeSmall(managedSize);
                }
            }   
            else if (marshalType == MarshalInfo::MARSHAL_TYPE_CURRENCY)
            {
                if (fDoHRESULTSwapping)
                {
                    // V1 restriction: we could implement this but it's late in the game to do so.
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
            }
            else
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
            }
        }   
        else
        {

            pss->MarshalReturn(&returnInfo);

            lastArgSize = returnInfo.GetComArgSize();
        }
    }

    return marshalType;
}


//---------------------------------------------------------
// Creates a new stub for a N/Direct call. Return refcount is 1.
// This Worker() routine is broken out as a separate function
// for purely logistical reasons: our COMPLUS exception mechanism
// can't handle the destructor call for StubLinker so this routine
// has to return the exception as an outparam.
//---------------------------------------------------------
static Stub*  CreateNDirectStubWorker(StubState*        pss,
                                        PCCOR_SIGNATURE szMetaSig,
                                        Module*    pModule,
                                        mdMethodDef md,
                                        BYTE       nlType,
                                        BYTE       nlFlags,
                                                                                CorPinvokeMap unmgdCallConv,
                                        BOOL    fConvertSigAsVarArg
#ifdef CUSTOMER_CHECKED_BUILD
                                        ,MethodDesc* pMD
#endif
#ifdef _DEBUG
                                        ,
                                        LPCUTF8 pDebugName,
                                        LPCUTF8 pDebugClassName,
                                        LPCUTF8 pDebugNameSpace
#endif
                                        )
{
    THROWSCOMPLUSEXCEPTION();


    Stub*           pstub           = NULL;

    _ASSERTE(nlType == nltAnsi || nlType == nltUnicode);

        if (unmgdCallConv != pmCallConvStdcall &&
                unmgdCallConv != pmCallConvCdecl &&
        unmgdCallConv != pmCallConvThiscall) 
    {
                COMPlusThrow(kTypeLoadException, IDS_INVALID_PINVOKE_CALLCONV);
        }

    IMDInternalImport *pInternalImport = pModule->GetMDImport();

    BOOL fDoHRESULTSwapping = FALSE;

    if (md != mdMethodDefNil) 
    {
        DWORD           dwDescrOffset;
        DWORD           dwImplFlags;
        pInternalImport->GetMethodImplProps(md,
                                            &dwDescrOffset,
                                            &dwImplFlags
                                           );
        fDoHRESULTSwapping = !IsMiPreserveSig(dwImplFlags);
    }

    //
    // Set up signature walking objects.
    //

    MetaSig msig(szMetaSig, pModule, fConvertSigAsVarArg);
    MetaSig sig = msig;
    ArgIterator ai( NULL, &sig, TRUE);

    if (sig.HasThis())
    {                
        COMPlusThrow(kInvalidProgramException, VLDTR_E_FMD_PINVOKENOTSTATIC);
    }

    bool fThisCall = (unmgdCallConv == pmCallConvThiscall);

    pss->SetStackPopSize(msig.CbStackPop(TRUE));
    pss->SetHas64BitReturn(msig.Is64BitReturn());
    pss->SetLastError(nlFlags & nlfLastError);
    pss->SetThisCall(fThisCall);
    pss->SetDoHRESULTSwapping(fDoHRESULTSwapping);

    switch (msig.GetReturnType())
    {
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_VAR:
        // case ELEMENT_TYPE_PTR:  -- cwb. PTR is unmanaged & should not be GC promoted
            pss->SetRetValIsGcRef();
            break;
        default:
            break;
    }

    pss->BeginEmit();


    //
    // Get a list of the COM+ argument offsets.  We
    // need this since we have to iterate the arguments
    // backwards.
    // Note that the first argument listed may
    // be a byref for a value class return value
    //

    int numArgs = msig.NumFixedArgs();

    int *offsets = (int*)_alloca(numArgs * sizeof(int));
    int *o = offsets;
    int *oEnd = o + numArgs;
    while (o < oEnd)
    {
        *o++ = ai.GetNextOffset();
    }

#ifdef STACK_GROWS_UP_ON_ARGS_WALK
    o = offsets;
#endif // STACK_GROWS_UP_ON_ARGS_WALK


    mdParamDef *params = (mdParamDef*)_alloca((numArgs+1) * sizeof(mdParamDef));
    CollateParamTokens(pInternalImport, md, numArgs, params);


    //
    // Now, emit the ML.
    //

    int argOffset = 0;
    int lastArgSize = 0;

    MarshalInfo::MarshalType marshalType = (MarshalInfo::MarshalType) 0xcccccccc;

    //
    // Marshal the return value.
    //

#ifdef STACK_GROWS_UP_ON_ARGS_WALK
    int nativeStackSize = 0;
    msig.Reset();
    if (msig.HasRetBuffArg())
        nativeStackSize += sizeof(LPVOID);
#else
    marshalType = DoMarshalReturnValue(msig, 
                                ai,
                                pModule,
                                params,
                                nlType,
                                nlFlags,
                                fDoHRESULTSwapping,
                                pss,
                                fThisCall,
                                lastArgSize
#ifdef CUSTOMER_CHECKED_BUILD
                                ,pMD
#endif
#ifdef _DEBUG
                                ,
                                pDebugName,
                                pDebugClassName,
                                pDebugNameSpace
#endif
                                );

    msig.GotoEnd();
#endif  // STACK_GROWS_UP_ON_ARGS_WALK

#ifdef _PPC_
    // We marshal arguments first, then return value
    // so skip the return buffer if there is one
    if (msig.HasRetBuffArg())
        pss->DoDstFixup(sizeof(LPVOID));
#endif

    //
    // Marshal the arguments
    //

    // Check to see if we need to do LCID conversion.
    int iLCIDArg = GetLCIDParameterIndex(pInternalImport, md);
    if (iLCIDArg != -1 && iLCIDArg > numArgs)
        COMPlusThrow(kIndexOutOfRangeException, IDS_EE_INVALIDLCIDPARAM);

    int argidx;
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
    argidx = 1;
    while (o < oEnd)
#else // STACK_GROWS_UP_ON_ARGS_WALK
    argidx = msig.NumFixedArgs();
    while (o > offsets)
#endif // STACK_GROWS_UP_ON_ARGS_WALK
    {
#ifndef STACK_GROWS_UP_ON_ARGS_WALK
        --o;
#endif // STACK_GROWS_UP_ON_ARGS_WALK

        //
        // Check to see if this is the parameter after which we need to insert the LCID.
        //

        if (argidx == iLCIDArg)
        {
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
            _ASSERTE(!"NYI -- LCID marshaling");
#endif 
            pss->MarshalLCID();
        }

        //
        // Adjust src pointer if necessary (for register params or
        // for return value order differences)
        //

        int fixup = *o - (argOffset + lastArgSize);
        argOffset = *o;

        if (!FitsInI2(fixup))
        {
            COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
        }
        if (fixup != 0)
        {
            pss->DoFixup(fixup);
        }

#ifdef STACK_GROWS_UP_ON_ARGS_WALK
        msig.NextArg();
#else
        msig.PrevArg();
#endif

        MarshalInfo info(pModule,
                            msig.GetArgProps(),
                            params[argidx],
                            MarshalInfo::MARSHAL_SCENARIO_NDIRECT,
                            nlType,
                            nlFlags,
                            TRUE,
                            argidx
#ifdef CUSTOMER_CHECKED_BUILD
                            ,pMD
#endif
#ifdef _DEBUG
                            ,
                            pDebugName,
                            pDebugClassName,
                            pDebugNameSpace,
                            argidx
#endif
);

        pss->MarshalArgument(&info);

        lastArgSize = info.GetComArgSize();

#ifdef STACK_GROWS_UP_ON_ARGS_WALK
        nativeStackSize += info.GetNativeArgSize();
#endif

#ifdef STACK_GROWS_UP_ON_ARGS_WALK
        o++;
        argidx++;
#else  // STACK_GROWS_UP_ON_ARGS_WALK
        --argidx;
#endif  // STACK_GROWS_UP_ON_ARGS_WALK
    }

    // Check to see if this is the parameter after which we need to insert the LCID.
    if (argidx == iLCIDArg)
    {
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
        _ASSERTE(!"NYI -- LCID marshaling");
#endif 
        pss->MarshalLCID();
        }

#ifdef STACK_GROWS_UP_ON_ARGS_WALK

    marshalType = DoMarshalReturnValue(msig, 
                            ai,
                            pModule,
                            params,
                            nlType,
                            nlFlags,
                            fDoHRESULTSwapping,
                            pss,
                            fThisCall,
                            lastArgSize
#ifdef CUSTOMER_CHECKED_BUILD
                            ,
                            pMD
#endif
#ifdef _DEBUG
                            ,
                            pDebugName,
                            pDebugClassName,
                            pDebugNameSpace
#endif
                            );


#endif // STACK_GROWS_UP_ON_ARGS_WALK


    if (msig.GetReturnType() != ELEMENT_TYPE_VOID)
    {
        if (msig.HasRetBuffArg())
        {
            if (marshalType == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS)
            {

                // Checked for this above.
                _ASSERTE(!fDoHRESULTSwapping);

                MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                UINT         managedSize = msig.GetRetTypeHandle().GetSize();
                UINT         unmanagedSize = pMT->GetNativeSize();
    
                if (IsManagedValueTypeReturnedByRef(managedSize) && 
                    (fThisCall || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                {
                    int fixup = ai.GetRetBuffArgOffset() - (argOffset + lastArgSize);
                    argOffset = ai.GetRetBuffArgOffset();
    
                    if (!FitsInI2(fixup))
                    {
                        COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
                    }
                    if (fixup != 0)
                    {   
                        pss->DoFixup(fixup);
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                        pss->DoDstFixup(-nativeStackSize);
#endif
                    }
                    pss->SetManagedReturnBuffSizeLarge(managedSize);
                    }
                else if (IsManagedValueTypeReturnedByRef(managedSize) && !(fThisCall || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                {
                    // nothing to do here: already taken care of above
                }
                else
                {
                    COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }
            }
        }
    }
    pstub = pss->FinishEmit();


    return pstub; // CHANGE, VC6.0
}


//---------------------------------------------------------
// Creates a new stub for a N/Direct call. Return refcount is 1.
// If failed, returns NULL and sets *ppException to an exception
// object.
//---------------------------------------------------------
Stub * CreateNDirectMLStub(PCCOR_SIGNATURE szMetaSig,
                           Module*    pModule,
                           mdMethodDef md,
                           CorNativeLinkType       nlType,
                           CorNativeLinkFlags       nlFlags,
                           CorPinvokeMap unmgdCallConv,
                           OBJECTREF *ppException,
                           BOOL fConvertSigAsVarArg
#ifdef CUSTOMER_CHECKED_BUILD
                           ,MethodDesc* pMD
#endif
#ifdef _DEBUG
                           ,
                           LPCUTF8 pDebugName,
                           LPCUTF8 pDebugClassName,
                           LPCUTF8 pDebugNameSpace
#endif
                           )
{
    THROWSCOMPLUSEXCEPTION();
    
    MLStubState MLState;

    return CreateNDirectStubWorker(&MLState,
                                     szMetaSig,
                                     pModule,
                                     md,
                                     nlType,
                                     nlFlags,
                                     unmgdCallConv,
                                     fConvertSigAsVarArg
#ifdef CUSTOMER_CHECKED_BUILD
                                     ,pMD
#endif
#ifdef _DEBUG
                                     ,
                                     pDebugName,
                                     pDebugClassName,
                                     pDebugNameSpace
#endif
                                     );
};




//---------------------------------------------------------
// Extracts the effective NAT_L CustomAttribute for a method.
//---------------------------------------------------------
VOID CalculatePinvokeMapInfo(MethodDesc *pMD,
                             /*out*/ CorNativeLinkType  *pLinkType,
                             /*out*/ CorNativeLinkFlags *pLinkFlags,
                             /*out*/ CorPinvokeMap      *pUnmgdCallConv,
                             /*out*/ LPCUTF8             *pLibName,
                             /*out*/ LPCUTF8             *pEntryPointName)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL fHasNatL = NDirect::ReadCombinedNAT_LAttribute(pMD, pLinkType, pLinkFlags, pUnmgdCallConv, pLibName, pEntryPointName);
    if (!fHasNatL)
    {
        *pLinkType = nltAnsi;
        *pLinkFlags = nlfNone;

        
        PCCOR_SIGNATURE pSig;
        DWORD           cSig;
        pMD->GetSig(&pSig, &cSig);
        CorPinvokeMap unmanagedCallConv = MetaSig::GetUnmanagedCallingConvention(pMD->GetModule(), pSig, cSig);
        if (unmanagedCallConv == (CorPinvokeMap)0 || unmanagedCallConv == (CorPinvokeMap)pmCallConvWinapi)
        {
            unmanagedCallConv = pMD->IsVarArg() ? pmCallConvCdecl : pmCallConvStdcall;
        }
        *pUnmgdCallConv = unmanagedCallConv;
        *pLibName = NULL;
        *pEntryPointName = NULL;
    }
}



//---------------------------------------------------------
// Extracts the effective NAT_L CustomAttribute for a method,
// taking into account default values and inheritance from
// the global NAT_L CustomAttribute.
//
// Returns TRUE if a NAT_L CustomAttribute exists and is valid.
// Returns FALSE if no NAT_L CustomAttribute exists.
// Throws an exception otherwise.
//---------------------------------------------------------
/*static*/
BOOL NDirect::ReadCombinedNAT_LAttribute(MethodDesc       *pMD,
                                 CorNativeLinkType        *pLinkType,
                                 CorNativeLinkFlags       *pLinkFlags,
                                 CorPinvokeMap            *pUnmgdCallConv,
                                 LPCUTF8                   *pLibName,
                                 LPCUTF8                   *pEntrypointName)
{
    THROWSCOMPLUSEXCEPTION();

    IMDInternalImport  *pInternalImport = pMD->GetMDImport();
    mdToken             token           = pMD->GetMemberDef();


    BOOL fVarArg = pMD->IsVarArg();

    *pLibName        = NULL;
    *pEntrypointName = NULL;

    DWORD   mappingFlags = 0xcccccccc;
    LPCSTR  pszImportName = (LPCSTR)POISONC;
    mdModuleRef modref = 0xcccccccc;

    if (SUCCEEDED(pInternalImport->GetPinvokeMap(token, &mappingFlags, &pszImportName, &modref)))
    {
        *pLibName = (LPCUTF8)POISONC;
        pInternalImport->GetModuleRefProps(modref, pLibName);

        *pLinkFlags = nlfNone;

        if (mappingFlags & pmSupportsLastError)
        {
            (*pLinkFlags) = (CorNativeLinkFlags) ((*pLinkFlags) | nlfLastError);
        }

        if (mappingFlags & pmNoMangle)
        {
            (*pLinkFlags) = (CorNativeLinkFlags) ((*pLinkFlags) | nlfNoMangle);
        }

        switch (mappingFlags & (pmCharSetNotSpec|pmCharSetAnsi|pmCharSetUnicode|pmCharSetAuto))
        {
        case pmCharSetNotSpec: //fallthru to Ansi
        case pmCharSetAnsi:
            *pLinkType = nltAnsi;
            break;
        case pmCharSetUnicode:
            *pLinkType = nltUnicode;
            break;
        case pmCharSetAuto:
            *pLinkType = (NDirectOnUnicodeSystem() ? nltUnicode : nltAnsi);
            break;
        default:
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL); // charset illegal

        }


        *pUnmgdCallConv = (CorPinvokeMap)(mappingFlags & pmCallConvMask);
        PCCOR_SIGNATURE pSig;
        DWORD           cSig;
        pMD->GetSig(&pSig, &cSig);
        CorPinvokeMap sigCallConv = MetaSig::GetUnmanagedCallingConvention(pMD->GetModule(), pSig, cSig);

        if (sigCallConv != 0 &&
            *pUnmgdCallConv != 0 &&
            sigCallConv != *pUnmgdCallConv)
        {
            // non-matching calling conventions
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL_CALLCONV);
        }
        if (*pUnmgdCallConv == 0)
        {
            *pUnmgdCallConv = sigCallConv;
        }

        if (*pUnmgdCallConv == pmCallConvWinapi ||
            *pUnmgdCallConv == 0
            ) {


            if (*pUnmgdCallConv == 0 && fVarArg)
            {
                *pUnmgdCallConv = pmCallConvCdecl;
            }
            else
            {
                *pUnmgdCallConv = pmCallConvStdcall;
            }
        }

        if (fVarArg && *pUnmgdCallConv != pmCallConvCdecl)
        {
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL_CALLCONV);
        }

        if (mappingFlags & ~((DWORD)(pmCharSetNotSpec |
                                     pmCharSetAnsi |
                                     pmCharSetUnicode |
                                     pmCharSetAuto |
                                     pmSupportsLastError |
                                     pmCallConvWinapi |
                                     pmCallConvCdecl |
                                     pmCallConvStdcall |
                                     pmCallConvThiscall |
                                     pmCallConvFastcall |
                                     pmNoMangle
                                     )))
        {
            COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_BADNATL); // mapping flags illegal
        }


        if (pszImportName == NULL)
            *pEntrypointName = pMD->GetName();
        else
            *pEntrypointName = pszImportName;

        return TRUE;
    }

    return FALSE;
}




//---------------------------------------------------------
// Does a class or method have a NAT_L CustomAttribute?
//
// S_OK    = yes
// S_FALSE = no
// FAILED  = unknown because something failed.
//---------------------------------------------------------
/*static*/
HRESULT NDirect::HasNAT_LAttribute(IMDInternalImport *pInternalImport, mdToken token)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(TypeFromToken(token) == mdtMethodDef);

    // Check method flags first before trying to find the custom value
    DWORD           dwAttr;
    dwAttr = pInternalImport->GetMethodDefProps(token);
    if (!IsReallyMdPinvokeImpl(dwAttr))
    {
        return S_FALSE;
    }

    DWORD   mappingFlags = 0xcccccccc;
    LPCSTR  pszImportName = (LPCSTR)POISONC;
    mdModuleRef modref = 0xcccccccc;


    if (SUCCEEDED(pInternalImport->GetPinvokeMap(token, &mappingFlags, &pszImportName, &modref)))
    {
        return S_OK;
    }

    return S_FALSE;
}



//struct _NDirect_NumParamBytes_Args {
//    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refMethod);
//};


//
// NumParamBytes
// Counts # of parameter bytes
FCIMPL1(INT32, NDirect_NumParamBytes, ReflectBaseObject* refMethodUNSAFE)
{
    INT32 cbParamBytes;
    REFLECTBASEREF refMethod = (REFLECTBASEREF) refMethodUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(refMethod);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (!(refMethod)) 
        COMPlusThrowArgumentNull(L"m");
    if (refMethod->GetMethodTable() != g_pRefUtil->GetClass(RC_Method))
        COMPlusThrowArgumentException(L"m", L"Argument_MustBeRuntimeMethodInfo");

    ReflectMethod* pRM = (ReflectMethod*) refMethod->GetData();
    MethodDesc* pMD = pRM->pMethod;

    if (!(pMD->IsNDirect()))
        COMPlusThrow(kArgumentException, IDS_EE_NOTNDIRECT);

    // This is revolting but the alternative is even worse. The unmanaged param count
    // isn't stored anywhere permanent so the only way to retrieve this value is
    // to recreate the MLStream. So this api will have horrible perf but almost
    // nobody uses it anyway.

    CorNativeLinkType type;
    CorNativeLinkFlags flags;
    LPCUTF8 szLibName = NULL;
    LPCUTF8 szEntrypointName = NULL;
    Stub  *pTempMLStub = NULL;
    CorPinvokeMap unmgdCallConv;

    CalculatePinvokeMapInfo(pMD, &type, &flags, &unmgdCallConv, &szLibName, &szEntrypointName);

    OBJECTREF pThrowable;

    PCCOR_SIGNATURE pMetaSig;
    DWORD       cbMetaSig;
    pMD->GetSig(&pMetaSig, &cbMetaSig);


    pTempMLStub = CreateNDirectMLStub(pMetaSig, pMD->GetModule(), pMD->GetMemberDef(), type, flags, unmgdCallConv, &pThrowable, FALSE
#ifdef CUSTOMER_CHECKED_BUILD
                                      ,pMD
#endif
                                      );
    if (!pTempMLStub)
    {
        COMPlusThrow(pThrowable);
    }
    cbParamBytes = ((MLHeader*)(pTempMLStub->GetEntryPoint()))->m_cbDstBuffer;
    pTempMLStub->DecRef();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return cbParamBytes;
}
FCIMPLEND



//struct _NDirect_Prelink_Args {
//    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refMethod);
//};

// Prelink
// Does advance loading of an N/Direct library
FCIMPL1(VOID, NDirect_Prelink_Wrapper, ReflectBaseObject* refMethodUNSAFE)
{
    REFLECTBASEREF refMethod = (REFLECTBASEREF) refMethodUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(refMethod);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (!(refMethod)) 
        COMPlusThrowArgumentNull(L"m");
    if (refMethod->GetMethodTable() != g_pRefUtil->GetClass(RC_Method))
        COMPlusThrowArgumentException(L"m", L"Argument_MustBeRuntimeMethodInfo");

    ReflectMethod* pRM = (ReflectMethod*) refMethod->GetData();
    MethodDesc* pMeth = pRM->pMethod;

    NDirect_Prelink(pMeth);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


VOID NDirect_Prelink(MethodDesc *pMeth)
{
    THROWSCOMPLUSEXCEPTION();

    // If the prestub has already been executed, no need to do it again.
    // This is a perf thing since it's safe to execute the prestub twice.
    if (!(pMeth->PointAtPreStub())) {
        return;
    }

    // Silently ignore if not N/Direct and not ECall.
    if (!(pMeth->IsNDirect()) && !(pMeth->IsECall())) {
        return;
    }

    pMeth->DoPrestub(NULL);
}




//==========================================================================
// NDirect debugger support
//==========================================================================

void NDirectMethodFrameGeneric::GetUnmanagedCallSite(void **ip,
                                                     void **returnIP,
                                                     void **returnSP)
{

    if (ip != NULL)
        AskStubForUnmanagedCallSite(ip, NULL, NULL);

    NDirectMethodDesc *pMD = (NDirectMethodDesc*)GetFunction();

    //
    // Get ML header
    //

    MLHeader *pheader;

    DWORD cbLocals;
    DWORD cbDstBuffer;
    DWORD cbThisCallInfo;

    if (pMD->IsVarArgs())
    {
        VASigCookie *pVASigCookie = GetVASigCookie();

        Stub *pTempMLStub;

        if (pVASigCookie->pNDirectMLStub != NULL)
        {
            pTempMLStub = pVASigCookie->pNDirectMLStub;

            pheader = (MLHeader*)(pTempMLStub->GetEntryPoint());

            cbLocals = pheader->m_cbLocals;
            cbDstBuffer = pheader->m_cbDstBuffer;
            cbThisCallInfo = (pheader->m_Flags & MLHF_THISCALL) ? 4 : 0;
        }
        else
        {
            // If we don't have a stub when we get here then we simply can't compute the return SP below. However, we
            // never actually ask for the return IP or SP when we don't have a stub yet anyway, so this is just fine.
            pheader = NULL;
            cbLocals = 0;
            cbDstBuffer = 0;
            cbThisCallInfo = 0;
        }
    }
    else
    {
        pheader = pMD->GetMLHeader();

        cbLocals = pheader->m_cbLocals;
        cbDstBuffer = pheader->m_cbDstBuffer;
        cbThisCallInfo = (pheader->m_Flags & MLHF_THISCALL) ? 4 : 0;
    }

    //
    // Compute call site info for NDirectGenericStubWorker
    //

    if (returnIP != NULL)
        *returnIP = (LPVOID) NDirectGenericReturnFromCall;

    //
    // !!! yow, this is a bit fragile...
    //

    if (returnSP != NULL)
#ifdef _X86_
        *returnSP = (void*) (((BYTE*) this)
                            - GetNegSpaceSize()
                            - NDirectGenericWorkerStackSize
                            - cbLocals
                            - cbDstBuffer
                            + cbThisCallInfo
                            - sizeof(void *)
                            );
#elif defined(_PPC_)
        *returnSP = (void*) (((BYTE*) this)
                            - offsetof(StubStackFrame, methodframe)
                            - NDirectGenericWorkerStackSize
                            - AlignStack(cbLocals + cbDstBuffer + NUM_FLOAT_ARGUMENT_REGISTERS * sizeof(DOUBLE))
                            );
#else
        PORTABILITY_WARNING("NDirectMethodFrameGeneric::GetUnmanagedCallSite");
        *returnSP = 0;
#endif
}


#ifdef _X86_

void NDirectMethodFrameSlim::GetUnmanagedCallSite(void **ip,
                                                  void **returnIP,
                                                  void **returnSP)
{

    AskStubForUnmanagedCallSite(ip, returnIP, returnSP);

    if (returnSP != NULL)
    {
        //
        // The slim worker pushes extra stack space, so
        // adjust our result obtained directly from the stub.
        //

        //
        // Get ML header
        //

        NDirectMethodDesc *pMD = (NDirectMethodDesc*)GetFunction();

        MLHeader *pheader;
        if (pMD->IsVarArgs())
        {
            VASigCookie *pVASigCookie = GetVASigCookie();

            Stub *pTempMLStub = NULL;

            if (pVASigCookie->pNDirectMLStub != NULL) {
                pTempMLStub = pVASigCookie->pNDirectMLStub;
            } else {
                //
                // We don't support generating an ML stub inside the debugger
                // callback right now.
                //
                _ASSERTE(!"NYI");
            }

            pheader = (MLHeader*)(pTempMLStub->GetEntryPoint());
            *(BYTE**)returnSP -= pheader->m_cbLocals;
        }
        else
        {
            pheader = pMD->GetMLHeader();
           *(BYTE**)returnSP -= pheader->m_cbLocals;
        }
    }
}

#endif // _X86_




void FramedMethodFrame::AskStubForUnmanagedCallSite(void **ip,
                                                     void **returnIP,
                                                     void **returnSP)
{

    MethodDesc *pMD = GetFunction();

    // If we were purists, we'd create a new subclass that parents NDirect and ComPlus method frames
    // so we don't have these funky methods that only work for a subset. 
    _ASSERTE(pMD->IsNDirect() || pMD->IsComPlusCall());

    if (returnIP != NULL || returnSP != NULL)
    {

        //
        // We need to get a pointer to the NDirect stub.
        // Unfortunately this is a little more difficult than
        // it might be...
        //

        //
        // Read destination out of prestub
        //

        BYTE *prestub = (BYTE*) pMD->GetPreStubAddr();
        INT32 stubOffset = *((UINT32*)(prestub+1));
        const BYTE *code = prestub + METHOD_CALL_PRESTUB_SIZE + stubOffset;

        //
        // Recover stub from code address
        //

        Stub *stub = Stub::RecoverStub(code);

        //
        // NDirect stub may have interceptors - skip them
        //

        while (stub->IsIntercept())
            stub = *((InterceptStub*)stub)->GetInterceptedStub();

        //
        // This should be the NDirect stub.
        //

        code = stub->GetEntryPoint();
        _ASSERTE(StubManager::IsStub(code));

        //
        // Compute the pointers from the call site info in the stub
        //

        if (returnIP != NULL)
            *returnIP = (void*) (code + stub->GetCallSiteReturnOffset());

        if (returnSP != NULL)
            *returnSP = (void*)
              (((BYTE*)this)+GetOffsetOfNextLink()+sizeof(Frame*)
               - stub->GetCallSiteStackSize()
               - sizeof(void*));
    }

    if (ip != NULL)
    {
        if (pMD->IsNDirect())
        {
            NDirectMethodDesc *pNMD = (NDirectMethodDesc *)pMD;
    
            *ip = pNMD->GetNDirectTarget();

    
            _ASSERTE(*ip != pNMD->ndirect.m_ImportThunkGlue);
        }
        else
            _ASSERTE(0);
    }
}


BOOL NDirectMethodFrame::TraceFrame(Thread *thread, BOOL fromPatch,
                                    TraceDestination *trace, REGDISPLAY *regs)
{

    //
    // Get the call site info
    //

    void *ip, *returnIP, *returnSP;
    GetUnmanagedCallSite(&ip, &returnIP, &returnSP);

    //
    // If we've already made the call, we can't trace any more.
    //
    // !!! Note that this test isn't exact.
    //

    if (!fromPatch
        && (thread->GetFrame() != this
            || !thread->m_fPreemptiveGCDisabled
            || *(void**)returnSP == returnIP))
    {
        LOG((LF_CORDB, LL_INFO10000,
             "NDirectMethodFrame::TraceFrame: can't trace...\n"));
        return FALSE;
    }

    //
    // Otherwise, return the unmanaged destination.
    //

    trace->type = TRACE_UNMANAGED;
    trace->address = (const BYTE *) ip;

    LOG((LF_CORDB, LL_INFO10000,
         "NDirectMethodFrame::TraceFrame: ip=0x%08x\n", ip));

    return TRUE;
}


