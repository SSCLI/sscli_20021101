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

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJitCompiler.cpp                               XX
XX                                                                           XX
XX   The functionality needed for the FAST JIT DLL.                          XX
XX   Includes the DLL entry point                                            XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#include "jitpch.h"

#include "new.h"                // for placement new
#include "fjitcore.h"
#include <stdio.h>


#include "openum.h"

#ifdef LOGGING
const char *opname[] =
{
#undef OPDEF
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,flow) s,
#include "opcode.def"
#undef OPDEF
};
#endif //LOGGING

#undef DECLARE_DATA

#define USE_EH_ENCODER
#include "ehencoder.cpp"
#undef USE_EH_ENCODER

#if defined(_X86_) 
#define CODE_EXPANSION_RATIO 10
#else
#define CODE_EXPANSION_RATIO 20
#endif
#define ROUND_TO_PAGE(x) ( (((x) + PAGE_SIZE - 1)/PAGE_SIZE) * PAGE_SIZE)

/* the jit helpers that we call at runtime */
BOOL FJit_HelpersInstalled;
unsigned __int64 (__stdcall *FJit_pHlpLMulOvf) (unsigned __int64 val1, unsigned __int64 val2);
float (__stdcall *FJit_pHlpFltRem) (float divisor, float dividend);
double (__stdcall *FJit_pHlpDblRem) (double divisor, double dividend);

void (jit_call *FJit_pHlpRngChkFail) (unsigned tryIndex);
void (jit_call *FJit_pHlpOverFlow) (unsigned tryIndex);
void (jit_call *FJit_pHlpInternalThrow) (CorInfoException throwEnum);
void (jit_call *FJit_pHlpInternalThrowFromHelper) (CorInfoException throwEnum);
CORINFO_Object (jit_call *FJit_pHlpArrAddr_St) (CORINFO_Object array, int index, CORINFO_Object value);
void (jit_call *FJit_pHlpInitClass) (CORINFO_CLASS_HANDLE cls);
CORINFO_Object (jit_call *FJit_pHlpNewObj) (CORINFO_MODULE_HANDLE scopeHnd, unsigned constrTok, int argN);
void (jit_call *FJit_pHlpThrow) (CORINFO_Object obj);
void (jit_call *FJit_pHlpRethrow) ();
void (jit_call *FJit_pHlpPoll_GC) ();
void (jit_call *FJit_pHlpMonEnter) (CORINFO_Object obj);
void (jit_call *FJit_pHlpMonExit) (CORINFO_Object obj);
void (jit_call *FJit_pHlpMonEnterStatic) (CORINFO_METHOD_HANDLE method);
void (jit_call *FJit_pHlpMonExitStatic) (CORINFO_METHOD_HANDLE method);
CORINFO_Object (jit_call *FJit_pHlpChkCast) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj);
void (jit_call *FJit_pHlpAssign_Ref)(CORINFO_Object** dst, CORINFO_Object* obj); // *dst = obj, inform GC
BOOL (jit_call *FJit_pHlpIsInstanceOf) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj);
CORINFO_Object (jit_call *FJit_pHlpNewArr_1_Direct) (CORINFO_CLASS_HANDLE cls, unsigned cElem);
CORINFO_Object (jit_call *FJit_pHlpBox) (CORINFO_CLASS_HANDLE type, void * unboxedData);
void* (jit_call *FJit_pHlpUnbox) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj);
void* (jit_call *FJit_pHlpGetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
__int64 (jit_call *FJit_pHlpGetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
void* (jit_call *FJit_pHlpGetFieldObj) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
void (jit_call *FJit_pHlpSetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int32);
void (jit_call *FJit_pHlpSetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int64 );
void (jit_call *FJit_pHlpSetFieldObj) (CORINFO_Object*, CORINFO_FIELD_HANDLE , LPVOID);
void* (jit_call *FJit_pHlpGetFieldAddress) (CORINFO_Object*, CORINFO_FIELD_HANDLE);

void (jit_call *FJit_pHlpGetRefAny) (CORINFO_CLASS_HANDLE cls, void* refany);
void (jit_call *FJit_pHlpEndCatch) ();
void (jit_call *FJit_pHlpPinvokeCalli) ();
void (jit_call *FJit_pHlpTailCall) ();
void (jit_call *FJit_pHlpBreak) ();
CORINFO_MethodPtr* (jit_call *FJit_pHlpEncResolveVirtual) (CORINFO_Object*, CORINFO_METHOD_HANDLE );
void (jit_call *FJit_pHlpVerification) (int offset);

LONG FilterAccessViolation(PEXCEPTION_POINTERS pExceptionPointers, LPVOID lpvParam)
{
    if (pExceptionPointers->ExceptionRecord->ExceptionCode == SEH_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;

    return EXCEPTION_CONTINUE_SEARCH;
}

LONG FilterJITExceptions(PEXCEPTION_POINTERS pExceptionPointers, LPVOID lpvParam)
{
    CorJitResult* pret = (CorJitResult*)lpvParam;

    switch (pExceptionPointers->ExceptionRecord->ExceptionCode)
    {
    case SEH_NO_MEMORY:
        *pret = CORJIT_OUTOFMEM;
        break;
    case SEH_JIT_REFUSED:
        *pret = CORJIT_BADCODE;
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

/*****************************************************************************
 * Disassemble and dump the Fjited code
 */


/*****************************************************************************/
extern "C"
BOOL WINAPI     DllMain(HANDLE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {

        // do any initialization here

        //allocate the code cache
        if(!FJitCompiler::Init()) {
            _ASSERTE(0);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // do any finalization here

        // free the code cache
        FJitCompiler::Terminate();
    }
    return TRUE;
}

FJitCompiler* ILJitter = NULL;     // The one and only instance of this JITer

/*****************************************************************************/
/* FIX, really the ICorJitCompiler should be done as a COM object, this is just
   something to get us going */

extern "C" 
ICorJitCompiler* __stdcall getJit()
{
    static char FJitBuff[sizeof(FJitCompiler)];
    if (ILJitter == 0)
    {
        // no need to check for out of memory, since caller checks for return value of NULL
        ILJitter = new(FJitBuff) FJitCompiler();
        _ASSERTE(ILJitter != NULL);
    }
    return(ILJitter);
}

/*                                                                                                */
FJit_Encode* __stdcall FJitCompiler::getEncoder() {
    FJit_Encode* encoder;
    New(encoder, FJit_Encode());
    return encoder;
}

/*****************************************************************************
 *  The main JIT function
 */
    //Note: this assumes that the code produced by fjit is fully relocatable, i.e. requires
    //no fixups after it is generated when it is moved.  In particular it places restrictions
    //on the code sequences used for static and non virtual calls and for helper calls among
    //other things,i.e. that pc relative instructions are not used for references to things outside of the
    //jitted method, and that pc relative instructions are used for all references to things
    //within the jitted method.  To accomplish this, the fjitted code is always reached via a level
    //of indirection.
CorJitResult __stdcall FJitCompiler::compileMethod (
            ICorJitInfo*               compHnd,            /* IN */
            CORINFO_METHOD_INFO*        info,               /* IN */
            unsigned                flags,              /* IN */
            BYTE **                 entryAddress,       /* OUT */
            ULONG  *                nativeSizeOfCode    /* OUT */
            )
{
#if defined(_DEBUG) || defined(LOGGING)
    // make a copy of the ICorJitInfo vtable so that I can log mesages later
    // this was made non-static due to a VC7 bug
    static void* ijitInfoVtable = *((void**) compHnd);
    logCallback = (ICorJitInfo*) &ijitInfoVtable;
#endif

    if(!FJitCompiler::GetJitHelpers(compHnd))
        return CORJIT_INTERNALERROR;

    // NOTE: should the properties of the FJIT change such that it
    // would have to pay attention to specific IL sequence points or
    // local variable liveness ranges for debugging purposes, we would
    // query the Runtime and Debugger for such information here,

    FJit* fjitData=NULL;
    CorJitResult ret = CORJIT_INTERNALERROR;
    unsigned char* savedCodeBuffer = NULL;
    unsigned savedCodeBufferCommittedSize = 0;
    unsigned int codeSize = 0;
    unsigned actualCodeSize;

#if defined(_DEBUG) || defined(LOGGING)
    const char *szDebugMethodName;
    const char *szDebugClassName;
    szDebugMethodName = compHnd->getMethodName(info->ftn, &szDebugClassName );
#endif

#ifdef _DEBUG
    static ConfigMethodSet fJitBreak;
    fJitBreak.ensureInit(L"JitBreak");
    if (fJitBreak.contains(szDebugMethodName, szDebugClassName, PCCOR_SIGNATURE(info->args.sig)))
        _ASSERTE(!"JITBreak");

    // Check if need to print the trace
    static ConfigDWORD fJitTrace;
    if ( fJitTrace.val(L"JitTrace") )
       printf( "Method %s Class %s \n",szDebugMethodName, szDebugClassName ); 
#endif

    PAL_TRY // for PAL_FINALLY
    PAL_TRY // for PAL_EXCEPT
    {
        fjitData = FJit::GetContext(compHnd, info, flags);

        _ASSERTE(fjitData); // if GetContext fails for any reason it throws an exception

        _ASSERTE(fjitData->opStack_len == 0);  // stack must be balanced at beginning of method

        codeSize = ROUND_TO_PAGE(info->ILCodeSize * CODE_EXPANSION_RATIO);  // HACK a large value for now
    #ifdef LOGGING
        static ConfigMethodSet fJitCodeLog;
        fJitCodeLog.ensureInit(L"JitCodeLog");
            fjitData->codeLog = fJitCodeLog.contains(szDebugMethodName, szDebugClassName, PCCOR_SIGNATURE(info->args.sig));
        if (fjitData->codeLog)
            codeSize = ROUND_TO_PAGE(info->ILCodeSize * 64);  
    #endif
        BOOL jitRetry = FALSE;  // this is set to false unless we get an exception because of underestimation of code buffer size
        do {    // the following loop is expected to execute only once, except when we underestimate the size of the code buffer,
                // in which case, we try again with a larger codeSize
            if (codeSize < MIN_CODE_BUFFER_RESERVED_SIZE)
            {
                if (codeSize > fjitData->codeBufferCommittedSize) 
                {
                    if (fjitData->codeBufferCommittedSize > 0)
                    {
                        unsigned AdditionalMemorySize = codeSize - fjitData->codeBufferCommittedSize;
                        if (AdditionalMemorySize > PAGE_SIZE) {
                            unsigned char* additionalMemory = (unsigned char*) VirtualAlloc(fjitData->codeBuffer+fjitData->codeBufferCommittedSize+PAGE_SIZE,
                                                                                            AdditionalMemorySize-PAGE_SIZE,
                                                                                            MEM_COMMIT,
                                                                                            PAGE_READWRITE);
                            if (additionalMemory == NULL)
                            {
                                ret = CORJIT_OUTOFMEM;
                                PAL_LEAVE_EX(Label2);
                            }
                            _ASSERTE(additionalMemory == fjitData->codeBuffer+fjitData->codeBufferCommittedSize+PAGE_SIZE);
                        }
                        // recommit the guard page
                        VirtualAlloc(fjitData->codeBuffer + fjitData->codeBufferCommittedSize,
                                     PAGE_SIZE,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
            
                        fjitData->codeBufferCommittedSize = codeSize;
                    }
                    else { /* first time codeBuffer being initialized */
                        savedCodeBuffer = fjitData->codeBuffer;
                        fjitData->codeBuffer = (unsigned char*)VirtualAlloc(fjitData->codeBuffer,
                                                            codeSize,
                                                            MEM_COMMIT,
                                                            PAGE_READWRITE);
                        if (fjitData->codeBuffer == NULL)
                        {
			    fjitData->codeBuffer = savedCodeBuffer;
                            ret = CORJIT_OUTOFMEM;
                            PAL_LEAVE_EX(Label2);
                        }
                        fjitData->codeBufferCommittedSize = codeSize;
                    }
                    _ASSERTE(codeSize == fjitData->codeBufferCommittedSize);
                    unsigned char* guardPage = (unsigned char*)VirtualAlloc(fjitData->codeBuffer + codeSize,
                                                            PAGE_SIZE,
                                                            MEM_COMMIT,
                                                            PAGE_READONLY); 
                    if (guardPage == NULL) 
                    {
                        ret = CORJIT_OUTOFMEM;
                        PAL_LEAVE_EX(Label2);
                    }
                }
            }
            else
            { // handle larger than MIN_CODE_BUFFER_RESERVED_SIZE methods
                savedCodeBuffer = fjitData->codeBuffer;
                savedCodeBufferCommittedSize = fjitData->codeBufferCommittedSize;
                fjitData->codeBuffer = (unsigned char*)VirtualAlloc(NULL,
                                                                    codeSize,
                                                                    MEM_RESERVE | MEM_COMMIT,
                                                                    PAGE_READWRITE);
                if (fjitData->codeBuffer == NULL)
                {
		    // Make sure that the saved buffer is freed in the destructor 
		    fjitData->codeBuffer = savedCodeBuffer;
                    ret = CORJIT_OUTOFMEM;
                    PAL_LEAVE_EX(Label2);
                }
                fjitData->codeBufferCommittedSize = codeSize;
            }



            unsigned char*  entryPoint;

            actualCodeSize = codeSize;
            PAL_TRY
            {
                FJitResult FJitRet;
                jitRetry = false; 
                FJitRet = fjitData->jitCompile(&entryPoint,&actualCodeSize); 
                // If we get a verification failed error, just map it to OK as
                // it's already been dealt with.                              
                if (FJitRet == FJIT_VERIFICATIONFAILED)
                    ret = CORJIT_OK;
                else if (FJitRet == FJIT_JITAGAIN)
		{
		    jitRetry = true;
                    ret = CORJIT_INTERNALERROR;
                }
                else // Otherwise cast it to a CorJitResult
                    ret = (CorJitResult)FJitRet;

		  if ( ret == CORJIT_OK )
                     ret = fjitData->fixupTable->resolve(fjitData->mapping, fjitData->codeBuffer, jitRetry );
                  if ( jitRetry )
		  {
                      fjitData->ReleaseContext();
                      fjitData = FJit::GetContext(compHnd, info, flags);
		      fjitData->mapInfo.savedIP = true;
                  }
            }
            PAL_EXCEPT_FILTER_EX(Label1, FilterAccessViolation, NULL)
            {  
                // we underestimated the buffer size required, so free this and allocate a larger one
                // check if this was a large method
                if (codeSize >= MIN_CODE_BUFFER_RESERVED_SIZE)
                {
                    VirtualFree(fjitData->codeBuffer,
                                fjitData->codeBufferCommittedSize,
                                MEM_DECOMMIT);
                    VirtualFree(fjitData->codeBuffer,
                                0,
                                MEM_RELEASE);
                    fjitData->codeBuffer = savedCodeBuffer;
                    fjitData->codeBufferCommittedSize = savedCodeBufferCommittedSize;
                }
                codeSize += codeSize; // try again with double the codeSize
                // The following release and get can be optimized, but since this is so rare, we don't bother doing it
                fjitData->ReleaseContext();
                fjitData = FJit::GetContext(compHnd, info, flags);
                _ASSERTE(fjitData);
                jitRetry = true;
            }
            PAL_ENDTRY

        } while (jitRetry);
    }
    PAL_EXCEPT_FILTER_EX(Label2, FilterJITExceptions, &ret)
    {
        _ASSERTE(ret != CORJIT_OK);
    }
    PAL_ENDTRY
    PAL_FINALLY_EX(Label3)
    {
        // do the cleanup
        if (ret != CORJIT_OK && fjitData)
        {
            fjitData->ReleaseContext();
        }
    }
    PAL_ENDTRY

    if (ret != CORJIT_OK)
    {
        return(ret);
    }

    *nativeSizeOfCode = actualCodeSize;

    // Report debugging information. 
    if (flags & CORJIT_FLG_DEBUG_INFO)
        fjitData->reportDebuggingData(&info->args);


#if defined(LOGGING)
    if (fjitData->codeLog)
              fjitData->displayGCMapInfo();
#endif

    /* write the EH info */
    unsigned exceptionCount = info->EHcount;
    FJit_Encode* mapping = fjitData->mapping;
    unsigned char* EHBuffer=NULL;
    unsigned EHbuffer_len=0; 
    if (exceptionCount && !fjitData->ver_failure) 
    {
        // compress the EH info, allocate space, and copy it
        // allocate space to hold the (uncompressed) exception count and all the clauses
        // this is guaranteed to hold the compressed form
        unsigned sUncompressedEHInfo = sizeof(CORINFO_EH_CLAUSE)*exceptionCount + sizeof(unsigned int);
        if (fjitData->EHBuffer_size < sUncompressedEHInfo) {
            delete [] fjitData->EHBuffer;
            fjitData->EHBuffer_size = sUncompressedEHInfo;
            fjitData->EHBuffer  = new unsigned char[sUncompressedEHInfo];
            if (fjitData->EHBuffer == NULL)
            {
                fjitData->ReleaseContext();
                return CORJIT_OUTOFMEM;
            }
            
        }
        EHBuffer = fjitData->EHBuffer;
        unsigned char* EHBufferPtr;
        // reserve space in beginning to encode size of compressed EHinfo
        EHBufferPtr = EHBuffer+2;   // enough for 16K compressed EHinfo
        EHEncoder::encode(&EHBufferPtr,exceptionCount);
        CORINFO_EH_CLAUSE clause;
        for (unsigned except = 0; except < exceptionCount; except++) 
        { 
            compHnd->getEHinfo(info->ftn, except, &clause);
            //INDEBUG(printf( "IL Try Start %x IL Try End %x  \n", clause.TryOffset, clause.TryOffset + clause.TryLength );) 
            // A try block is always jitted after its handlers
            unsigned IL_TryStart     = clause.TryOffset;
            unsigned IL_TryEnd       = clause.TryOffset + clause.TryLength;  clause.TryLength = 0;
            clause.TryOffset = mapping->pcFromIL(clause.TryOffset);
            _ASSERTE( clause.TryOffset > 0 );
            unsigned IL_offset;
            for (  IL_offset = IL_TryStart; IL_offset < IL_TryEnd; IL_offset++ )
            {
              unsigned PCoffset = mapping->pcFromIL( IL_offset );
              _ASSERTE( PCoffset > 0 );
              clause.TryOffset = clause.TryOffset  > PCoffset ?  PCoffset : clause.TryOffset;
              clause.TryLength = clause.TryLength  < PCoffset ?  PCoffset : clause.TryLength;
            }
            // We need to figure out where the last instruction of the try block ended
            unsigned EndOfLastInst = (unsigned)-1; 
            for (  IL_offset = 0; IL_offset <= fjitData->methodInfo->ILCodeSize; IL_offset++ )
            {
              unsigned PCoffset = mapping->pcFromIL( IL_offset );
              _ASSERTE( PCoffset > 0 );
              if ( clause.TryLength < PCoffset &&  EndOfLastInst > PCoffset )
                   EndOfLastInst = PCoffset;
            }
            if ( clause.TryLength  <  EndOfLastInst && EndOfLastInst != (unsigned)(-1))
                   clause.TryLength = EndOfLastInst;

            //INDEBUG(printf( "PC Try Start %x PC Try End %x  \n", clause.TryOffset,  clause.TryLength );) 
            // Since catch, finally, and filter clauses are never accessed directly from valid IL code, they always terminate
            // in a jump to the next instruction. This jump is only executed in a catch clause because finally and 
            // filter clauses return while catch clause calls JIT_EndCatch to reset the stack. 
            //INDEBUG(printf("ILHandler[Start %x End %x]\n",clause.HandlerOffset,clause.HandlerOffset+clause.HandlerLength);)
            unsigned IL_HandlerStart = clause.HandlerOffset;
            unsigned IL_HandlerEnd   = clause.HandlerOffset + clause.HandlerLength; clause.HandlerLength = 0;
            clause.HandlerOffset = mapping->pcFromIL(IL_HandlerStart);
            _ASSERTE( clause.HandlerOffset > 0 );
            for (  IL_offset = IL_HandlerStart; IL_offset < IL_HandlerEnd; IL_offset++ )
            {
              unsigned PCoffset = mapping->pcFromIL( IL_offset );
              _ASSERTE( PCoffset > 0 );
              clause.HandlerLength = clause.HandlerLength  < PCoffset ?  PCoffset : clause.HandlerLength;
              clause.HandlerOffset = clause.HandlerOffset  > PCoffset ?  PCoffset : clause.HandlerOffset;
            }  
            // We need to figure out where the last instruction of the handler block ended
            EndOfLastInst = (unsigned)-1; 
            for (  IL_offset = 0; IL_offset < fjitData->methodInfo->ILCodeSize; IL_offset++ )
            {
              unsigned PCoffset = mapping->pcFromIL( IL_offset );
              _ASSERTE( PCoffset > 0 );
              if ( clause.HandlerLength < PCoffset  &&  EndOfLastInst > PCoffset)
                   EndOfLastInst = PCoffset;
            }
            if ( clause.HandlerLength  <  EndOfLastInst && EndOfLastInst != (unsigned)-1)
                clause.HandlerLength = EndOfLastInst;
            
            //INDEBUG(printf( "PC Handler [Start %x  End %x]\n",clause.HandlerOffset, clause.HandlerLength );)
            if (clause.Flags & CORINFO_EH_CLAUSE_FILTER)
                clause.FilterOffset = mapping->pcFromIL(clause.FilterOffset);
            //INDEBUG(if (clause.Flags & CORINFO_EH_CLAUSE_FILTER) printf( "PC Filter [Start %x]\n",clause.FilterOffset );)
            EHEncoder::encode(&EHBufferPtr,clause);
        }
        EHbuffer_len = (unsigned)(EHBufferPtr - EHBuffer);
        _ASSERTE(EHbuffer_len < 0x10000);
        *(short*)EHBuffer = (short)EHbuffer_len;
    }

    /* Reset the verification failure flag it is no longer needed. We depened on the FJitInfo to be thread safe */
    fjitData->ver_failure = 0;

    /* write the header and IL/PC map (gc info) */
    /* note, we do this after the EH info since we compress the mapping and the EH info needs
       to use the mapping */
    unsigned char* hdr;
    unsigned hdr_len;

    //compute the total size needed
#ifdef _DEBUG
    //add the il to pc map at end of hdr
    hdr_len = sizeof(Fjit_hdrInfo)
        + fjitData->compressGCHdrInfo()
        + fjitData->mapping->compressedSize();
#else
    hdr_len = sizeof(Fjit_hdrInfo)
        + fjitData->compressGCHdrInfo();
#endif // _DEBUG

    unsigned char *pCodeBlock, *pEhBlock;
    pCodeBlock = fjitData->codeBuffer;

    BYTE* startAddress = (BYTE*) compHnd->alloc(actualCodeSize,
                                                &pCodeBlock,
                                                EHbuffer_len,
                                                &pEhBlock,
                                                hdr_len,
                                                &hdr);
    if ((HRESULT)startAddress == E_FAIL) {
        fjitData->ReleaseContext();
        return CORJIT_OUTOFMEM;
    }

    memcpy(pEhBlock,EHBuffer,EHbuffer_len);
    *entryAddress = startAddress; 

    hdr = pEhBlock+EHbuffer_len;


    //move the pieces into it
    _ASSERTE(hdr_len);
    memcpy(hdr, &fjitData->mapInfo, sizeof(Fjit_hdrInfo));
    hdr += sizeof(Fjit_hdrInfo);
    hdr_len -= sizeof(Fjit_hdrInfo);
    _ASSERTE(hdr_len);
    memcpy(hdr, fjitData->gcHdrInfo, fjitData->gcHdrInfo_len);
    hdr += fjitData->gcHdrInfo_len;
    hdr_len -= fjitData->gcHdrInfo_len;
#ifdef _DEBUG
    //add il to pc map at end of hdr
    _ASSERTE(hdr_len);
    if(!fjitData->mapping->compress(hdr, hdr_len)) {
        _ASSERTE(!"did't fit in buffer");
    }
    hdr_len -= fjitData->mapping->compressedSize();
#endif // _DEBUG
    _ASSERTE(!hdr_len); 


#ifdef _DEBUG
    if (codeSize < MIN_CODE_BUFFER_RESERVED_SIZE)
    {
        for (unsigned i=0; i< PAGE_SIZE; i++)
            *(fjitData->codeBuffer + i) = 0xCC;
    }
#endif

    // check if this was a large method
    if (codeSize >= MIN_CODE_BUFFER_RESERVED_SIZE)
    {
        VirtualFree(fjitData->codeBuffer,
                    fjitData->codeBufferCommittedSize,
                    MEM_DECOMMIT);
        VirtualFree(fjitData->codeBuffer,
                    0,
                    MEM_RELEASE);
        fjitData->codeBuffer = savedCodeBuffer;
        fjitData->codeBufferCommittedSize = savedCodeBufferCommittedSize;
    }

#ifdef LOGGING
    if (fjitData->codeLog) {
        LOGMSG((compHnd, LL_INFO10, "Fjitted '%s::%s' at addr %#x to %#x header %#x\n", 
            szDebugClassName,
            szDebugMethodName,
            (unsigned int) pCodeBlock,
            (unsigned int) pCodeBlock + actualCodeSize,
            hdr_len));
    }
#endif //LOGGING

    fjitData->ReleaseContext();
    return CORJIT_OK;

}


BOOL FJitCompiler::Init()
{
    FJit_HelpersInstalled = false;
    if (!FJit::Init()) 
        return FALSE;
    return TRUE;
}

void FJitCompiler::Terminate() {
    FJit::Terminate();
    if (ILJitter) ILJitter->~FJitCompiler();
    ILJitter = NULL;
}


/* grab and remember the jitInterface helper addresses that we need at runtime */
BOOL FJitCompiler::GetJitHelpers(ICorJitInfo* jitInfo) {

    if (FJit_HelpersInstalled) return true;

    FJit_pHlpLMulOvf = (unsigned __int64 (__stdcall *) (unsigned __int64 val1, unsigned __int64 val2))
        (jitInfo->getHelperFtn(CORINFO_HELP_LMUL_OVF));
    if (!FJit_pHlpLMulOvf) return false;

    FJit_pHlpFltRem = (float (__stdcall *) (float divisor, float dividend))
        (jitInfo->getHelperFtn(CORINFO_HELP_FLTREM));
    if (!FJit_pHlpFltRem) return false;

    FJit_pHlpDblRem = (double (__stdcall *) (double divisor, double dividend))
        (jitInfo->getHelperFtn(CORINFO_HELP_DBLREM));
    if (!FJit_pHlpDblRem) return false;

    FJit_pHlpRngChkFail = (void (jit_call *) (unsigned tryIndex))
        (jitInfo->getHelperFtn(CORINFO_HELP_RNGCHKFAIL));
    if (!FJit_pHlpRngChkFail) return false;

    FJit_pHlpInternalThrow = (void (jit_call *) (CorInfoException throwEnum))
        (jitInfo->getHelperFtn(CORINFO_HELP_INTERNALTHROW));
    if (!FJit_pHlpInternalThrow) return false;

    FJit_pHlpInternalThrowFromHelper = (void (jit_call *) (CorInfoException throwEnum))
        (jitInfo->getHelperFtn(CORINFO_HELP_INTERNALTHROW_FROM_HELPER));
    if (!FJit_pHlpInternalThrowFromHelper) return false;

    FJit_pHlpArrAddr_St = (CORINFO_Object (jit_call *) (CORINFO_Object arrray, int index, CORINFO_Object value))
        (jitInfo->getHelperFtn(CORINFO_HELP_ARRADDR_ST));
    if (!FJit_pHlpArrAddr_St) return false;

    FJit_pHlpOverFlow = (void (jit_call *) (unsigned tryIndex))
        (jitInfo->getHelperFtn(CORINFO_HELP_OVERFLOW));
    if (!FJit_pHlpOverFlow) return false;

    FJit_pHlpInitClass = (void (jit_call *) (CORINFO_CLASS_HANDLE cls))
        (jitInfo->getHelperFtn(CORINFO_HELP_INITCLASS));
    if (!FJit_pHlpInitClass) return false;
    
    FJit_pHlpNewObj = (CORINFO_Object (jit_call *) (CORINFO_MODULE_HANDLE scopeHnd, unsigned constrTok, int argN))
        (jitInfo->getHelperFtn(CORINFO_HELP_NEWOBJ));
    if (!FJit_pHlpNewObj) return false;

    FJit_pHlpThrow = (void (jit_call *) (CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_THROW));
    if (!FJit_pHlpThrow) return false;

    FJit_pHlpRethrow = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_RETHROW));
    if (!FJit_pHlpRethrow) return false;

    FJit_pHlpPoll_GC = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_POLL_GC));
    if (!FJit_pHlpPoll_GC) return false;

    FJit_pHlpMonEnter = (void (jit_call *) (CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_ENTER));
    if (!FJit_pHlpMonEnter) return false;

    FJit_pHlpMonExit = (void (jit_call *) (CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_EXIT));
    if (!FJit_pHlpMonExit) return false;

    FJit_pHlpMonEnterStatic = (void (jit_call *) (CORINFO_METHOD_HANDLE method))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_ENTER_STATIC));
    if (!FJit_pHlpMonEnterStatic) return false;

    FJit_pHlpMonExitStatic = (void (jit_call *) (CORINFO_METHOD_HANDLE method))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_EXIT_STATIC));
    if (!FJit_pHlpMonExitStatic) return false;
    
    FJit_pHlpChkCast = (CORINFO_Object (jit_call *) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_CHKCAST));
    if (!FJit_pHlpChkCast) return false;
    
    FJit_pHlpIsInstanceOf = (BOOL (jit_call *) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_ISINSTANCEOF));
    if (!FJit_pHlpIsInstanceOf) return false;

    FJit_pHlpNewArr_1_Direct = (CORINFO_Object (jit_call *) (CORINFO_CLASS_HANDLE cls, unsigned cElem))
        (jitInfo->getHelperFtn(CORINFO_HELP_NEWARR_1_DIRECT));
    if (!FJit_pHlpNewArr_1_Direct) return false;
   
    FJit_pHlpBox = (CORINFO_Object (jit_call *) (CORINFO_CLASS_HANDLE type, void * unboxedData))
        (jitInfo->getHelperFtn(CORINFO_HELP_BOX));
    if (!FJit_pHlpBox) return false;
    
    FJit_pHlpUnbox = (void* (jit_call *) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_UNBOX));
    if (!FJit_pHlpUnbox) return false;

    FJit_pHlpGetField32 = (void* (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELD32));
    if (!FJit_pHlpGetField32) return false;

    FJit_pHlpGetField64 = (__int64 (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELD64));
    if (!FJit_pHlpGetField64) return false;

    FJit_pHlpGetFieldObj = (void* (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELDOBJ));
    if (!FJit_pHlpGetFieldObj) return false;

    FJit_pHlpSetField32 = (void (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE,  __int32 ))
        (jitInfo->getHelperFtn(CORINFO_HELP_SETFIELD32));
    if (!FJit_pHlpSetField32) return false;

    FJit_pHlpSetField64 = (void (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int64 ))
        (jitInfo->getHelperFtn(CORINFO_HELP_SETFIELD64));
    if (!FJit_pHlpSetField64) return false;

    FJit_pHlpSetFieldObj = (void (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE, LPVOID))
        (jitInfo->getHelperFtn(CORINFO_HELP_SETFIELDOBJ));
    if (!FJit_pHlpSetFieldObj) return false;

    FJit_pHlpGetFieldAddress = (void* (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELDADDR));
    if (!FJit_pHlpGetFieldAddress) return false;

    FJit_pHlpGetRefAny = (void (jit_call *) (CORINFO_CLASS_HANDLE cls, void* refany))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETREFANY));
    if (!FJit_pHlpGetRefAny) return false;

    FJit_pHlpEndCatch = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_ENDCATCH));
    if (!FJit_pHlpEndCatch) return false;


    FJit_pHlpTailCall = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_TAILCALL));
    if (!FJit_pHlpTailCall) return false;

    FJit_pHlpBreak = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_USER_BREAKPOINT));
    if (!FJit_pHlpBreak) return false;

    FJit_pHlpEncResolveVirtual = (CORINFO_MethodPtr* (jit_call *) (CORINFO_Object*, CORINFO_METHOD_HANDLE ))
        (jitInfo->getHelperFtn(CORINFO_HELP_EnC_RESOLVEVIRTUAL));
    if (!FJit_pHlpEncResolveVirtual) return false;

    FJit_pHlpVerification = (void (jit_call *) (int offset))
        (jitInfo->getHelperFtn(CORINFO_HELP_VERIFICATION));
    if (!FJit_pHlpVerification) return false;

    FJit_pHlpAssign_Ref = (void (jit_call *)(CORINFO_Object** dst, CORINFO_Object* obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_CHECKED_ASSIGN_REF));
    if (!FJit_pHlpAssign_Ref) return false;

    FJit_HelpersInstalled = true;
    return true;
}


//*************************************************************************************
//      FixupTable methods
//*************************************************************************************
FixupTable::FixupTable()
{
    relocations_size = 0;               //let it grow as appropriate
    relocations_len = relocations_size;
    relocations = NULL;
    storedStartIP = NULL;
}

FixupTable::~FixupTable()
{
    if (relocations) delete [] relocations;
    relocations = NULL;
}

CorJitResult FixupTable::insert(void** pCodeBuffer)
{
    if (relocations_len >= relocations_size) {
        relocations_size = FJit::growBuffer((unsigned char**)&relocations, relocations_len*sizeof(void*), (relocations_len+1)*sizeof(void*));
        relocations_size = relocations_size / sizeof(void*);
    }
    relocations[relocations_len++] = (unsigned) (pCodeBuffer);
    return CORJIT_OK;
}

void  FixupTable::adjustMap(int delta) 
{
    for (unsigned i = 0; i < relocations_len; i++) {
        relocations[i] = (int) relocations[i] + delta;
    }
}

CorJitResult FixupTable::resolve(FJit_Encode* mapping, BYTE* startAddress, BOOL & JitAgain ) 
{
    for (unsigned i = 0; i < relocations_len; i++) {
        unsigned address;
        unsigned target;
        FJitResult PatchResult = FJIT_OK; 
	// Read the IL address 
        emit_read_address( (unsigned*) relocations[i], address, PatchResult );
        
        if ((unsigned) address < 0x80000000)
        {
            target = mapping->pcFromIL(address) + (unsigned) startAddress;
        }
        else
        {
            target = address - 0x80000000;
        }
        // Attempt to patch with native address
        emit_patch_address( (unsigned*) relocations[i], (unsigned)target, storedStartIP, PatchResult );
        // If the address couldn't be patched force a rejit or report internal error
        if ( PatchResult == FJIT_JITAGAIN )
	{
	   JitAgain = true;
           return CORJIT_OK;
        } 
        else if ( PatchResult != FJIT_OK )
	   return CORJIT_INTERNALERROR;
    }

    return CORJIT_OK;
}

// Checks that all IL offsets correspond to non-zero PC offsets in the mapping
int FixupTable::verifyTargets( FJit_Encode* mapping ) 
{
    for (unsigned i = 0; i < relocations_len; i++) {
        unsigned address;
        FJitResult ReadResult = FJIT_OK;
        emit_read_address( (unsigned*) relocations[i], address, ReadResult );

        //unsigned targetILoffset = *((unsigned*) relocations[i]);
       
        if ( !mapping->pcFromIL(address) || ReadResult != FJIT_OK)
        {
          INDEBUG(printf ("Not found %x from %x, val %x \n", address,(unsigned*)relocations[i],*(unsigned*)relocations[i]););
          return 0;
        }
    }

    return 1;
}

void  FixupTable::setup() 
{
#ifdef _DEBUG
    memset(relocations, 0xDD, relocations_len*sizeof(void*));
#endif
    relocations_len = 0;
}

//*************************************************************************************

#if _DEBUG
void StackEncoder::PrintStack(const char* name, unsigned char *& inPtr) {

    int stackLen = FJit_Encode::decode(&inPtr);
    LOGMSG((jitInfo, LL_INFO1000, "        %s len=%d bits:", name, stackLen));
    while(stackLen > 0) {
        --stackLen;
        LOGMSG((jitInfo, LL_INFO1000, "  "));
        unsigned bits = *inPtr++;
        for (unsigned bitPos = 0; bitPos < 8; bitPos++) {
            LOGMSG((jitInfo, LL_INFO1000, "%d ", bits & 1));
            bits >>= 1;

            if (stackLen == 0 && bits == 0)
                break;
        }
        
    }
    LOGMSG((jitInfo, LL_INFO1000, "\n"));
}

void StackEncoder::PrintStacks(FJit_Encode* mapping)
{
    LOGMSG((jitInfo, LL_INFO1000, "Labelled Stacks\n"));
    LOGMSG((jitInfo, LL_INFO1000, "Lowest bit (first thing pushed on opcode stack) printed first\n"));
    for (unsigned i=0; i< labeled_len; i++) {
        unsigned int stackIndex = labeled[i].stackToken;
        unsigned char * inPtr = &(stacks[stackIndex]);
        unsigned ILOffset = mapping->ilFromPC(labeled[i].pcOffset,NULL);
        LOGMSG((jitInfo, LL_INFO1000, "    IL=%#x, Native=%#x\n", ILOffset,labeled[i].pcOffset));

        PrintStack("OBJREF", inPtr);
        PrintStack("BYREF ", inPtr);
    }
}
#endif // _DEBUG

LabelTable::LabelTable() {
    stacks_size = 0;
    stacks = NULL;
    stacks_len = 0;
    labels_size = 0;
    labels = NULL;
    labels_len = 0;
}

LabelTable::~LabelTable() {
    if (stacks) delete [] stacks;
    stacks = NULL;
    stacks_size = 0;
    stacks_len = 0;
    if (labels) delete [] labels;
    labels = NULL;
    labels_size = 0;
    labels_len = 0;
}

void LabelTable::reset() {
    if (stacks_len) {
#ifdef _DEBUG
        memset(stacks, 0xFF, stacks_len);
#endif
        stacks_len = 0;
    }
    if (labels_len) {
#ifdef _DEBUG
        memset(labels, 0xFF, labels_len*sizeof(label_table));
#endif
        labels_len = 0;
    }
    //add a stack of size zero at the beginning, since this is the most common stack at a label
    if (!stacks) {
        New(stacks, unsigned char[1]);
    }
    *stacks = 0;
    stacks_len = 1;
}

/* add a label with a stack signature
   note, the label_table must be kept sorted by ilOffset
   */
void LabelTable::add(unsigned int ilOffset, OpType* op_stack, unsigned int op_stack_len, bool overwrite ) {

    /* make sure there is room for the label */
    if (labels_len >= labels_size ) {
        growLabels();
    }

    /* get the probable insertion point */
    unsigned int insert = searchFor(ilOffset);

    /* at this point we either are pointing at the insertion point or this label is already here */
    if ((insert < labels_len) && (labels[insert].ilOffset == ilOffset) ) {
        // label is here already,
      if ( !overwrite )
      {
          #ifdef _DEBUG
          //lets compare the stacks
          unsigned char* inPtr = &stacks[labels[insert].stackToken];
          unsigned int num = FJit_Encode::decode(&inPtr);
          _ASSERTE(num == op_stack_len);
          OpType type;
          while (num--) {
            int enum_val = FJit_Encode::decode(&inPtr);
            int cls_val  = FJit_Encode::decode(&inPtr);
            type.init( (OpTypeEnum)enum_val, (CORINFO_CLASS_HANDLE)cls_val );

            _ASSERTE(*op_stack == type || (type.isPtr() && (*op_stack).isPtr()));
            op_stack++;
          }
          #endif
      }
      else
      {
	  // We need to overwrite a stack. We can only do it if the length of the current stack
	  // and the length of the new stack are the same. 
          unsigned char* outPtr = &stacks[labels[insert].stackToken];  //location of the compressed stack of the label
          // get the number of operands on the stored stack 
          unsigned int num = FJit_Encode::decode(&outPtr);
          // there is no way to recover from this internal error
          _ASSERTE(num == op_stack_len);
	  if ( op_stack_len != num )
              RaiseException(SEH_JIT_REFUSED,EXCEPTION_NONCONTINUABLE,0,NULL); 
          // overwrite the stack
          outPtr = &stacks[labels[insert].stackToken];
          FJit_Encode::encode(op_stack_len, &outPtr);
          while (op_stack_len--) {
            FJit_Encode::encode(op_stack->enum_(), &outPtr);
            FJit_Encode::encode((unsigned int)(op_stack->cls()), &outPtr);
            op_stack++;
          }
      }
      return;
    }

    /* make the insertion */
    memmove(&labels[insert+1], &labels[insert], (labels_len-insert)*sizeof(label_table));
    labels[insert].ilOffset = ilOffset;
    labels[insert].stackToken = compress(op_stack,op_stack_len);
    labels_len++;

}

/* find a label token from an il offset, returns LABEL_NOT_FOUND if missing */
unsigned int LabelTable::findLabel(unsigned int ilOffset) {
    /* get the probable index by search as if inserting */
    unsigned int result = searchFor(ilOffset);

    /* at this point we either are pointing at the label or we are pointing at the insertion point */
    if ((result >= labels_len) || (labels[result].ilOffset != ilOffset)) {
        return LABEL_NOT_FOUND;
    }

    return result;
}

/* set operand stack from a label token, return the size of the stack, # of operands */
unsigned int LabelTable::setStackFromLabel(unsigned int labelToken, OpType* op_stack, unsigned int op_stack_size) {
    unsigned int result;
    unsigned char* inPtr = &stacks[labels[labelToken].stackToken];  //location of the compressed stack of the label

    /* get the number of operands and make sure there is room */
    unsigned int num = FJit_Encode::decode(&inPtr);
    _ASSERTE(num <= op_stack_size);
    result = num;

    /* expand the stack back out */
    while (num--) {
        int enum_val = FJit_Encode::decode(&inPtr);
        int cls_val  = FJit_Encode::decode(&inPtr);
        op_stack->init( (OpTypeEnum)enum_val, (CORINFO_CLASS_HANDLE)cls_val );
        
        op_stack++;
    }

    return result;
}

unsigned int LabelTable::getStackSize( unsigned int labelToken )
{
    // Check that the labelToken is valid
    _ASSERTE(labelToken <= labels_len);
    if (labelToken > labels_len  )
        RaiseException(SEH_JIT_REFUSED,EXCEPTION_NONCONTINUABLE,0,NULL); 
    // Get the pointer to the compressed stack
    unsigned char* inPtr = &stacks[labels[labelToken].stackToken];  //location of the compressed stack of the label

    // Get the number of stack entries
    unsigned int num = FJit_Encode::decode(&inPtr);

    return num;
}

/* write an op stack into the stacks buffer, return the offset into the buffer where written */
unsigned int LabelTable::compress(OpType* op_stack, unsigned int op_stack_len) {

    /* check for empty stack, we have put one at the front of the stacks buffer */
    if (!op_stack_len) return 0;

    /* make sure there is enough room, note this may realloc the stacks buffer
       it is okay to overestimate the space required, but never underestimate */
    unsigned int size = stacks_len + op_stack_len * sizeof(OpType) + 2; //+2 is for op_stack_len
    if (size >= stacks_size) {
        growStacks(size);
    }

    /* we always place the new stack on the end of the buffer*/
    unsigned int result = stacks_len;
    unsigned char* outPtr = (unsigned char* ) &stacks[result];

    /* write the operands */
    FJit_Encode::encode(op_stack_len, &outPtr);
    while (op_stack_len--) {
        FJit_Encode::encode(op_stack->enum_(), &outPtr);
        FJit_Encode::encode((unsigned int)(op_stack->cls()), &outPtr);
        op_stack++;
    }

    /* compute the new length of the buffer */
    stacks_len = (unsigned)(outPtr - stacks);

    return result;
}

/* find the offset at which the label exists or should be inserted */
unsigned int LabelTable::searchFor(unsigned int ilOffset) {
    //binary search of table
    signed low, mid, high;
    low = 0;
    high = labels_len-1;
    while (low <= high) {
        mid = (low+high)/2;
        if ( labels[mid].ilOffset == ilOffset) {
            return mid;
        }
        if ( labels[mid].ilOffset < ilOffset ) {
            low = mid+1;
        }
        else {
            high = mid-1;
        }
    }
    return low;
}

/* grow the stacks buffer */
void LabelTable::growStacks(unsigned int new_size) {
    unsigned char* temp = stacks;
    unsigned allocated_size = new_size*2;   //*2 to cut down on growing
    New(stacks, unsigned char[allocated_size]);
    if (stacks_len) memcpy(stacks, temp, stacks_len);
    if (temp) delete [] temp;
    stacks_size = allocated_size;
#ifdef _DEBUG
    memset(&stacks[stacks_len], 0xEE, stacks_size-stacks_len);
#endif
}

/* grow the labels array */
void LabelTable::growLabels() {
    label_table* temp = labels;
    unsigned allocated_size = labels_size*2+20; //*2 to cut down on growing
    New(labels, label_table[allocated_size]);

    if (labels_len) memcpy(labels, temp, labels_len*sizeof(label_table));
    if (temp) delete [] temp;
#ifdef _DEBUG
    memset(&labels[labels_len], 0xEE, (labels_size-labels_len)*sizeof(label_table));
#endif
    labels_size = allocated_size;
}


StackEncoder::StackEncoder() {
    last_stack = NULL;
    last_stack_size = 0;
    last_stack_len = 0;
    stacks = NULL;
    stacks_size = 0;
    stacks_len = 0;
    gcRefs = NULL;
    gcRefs_size = 0;
    gcRefs_len = 0;
    interiorRefs = NULL;
    interiorRefs_size = 0;
    interiorRefs_len = 0;
    labeled = NULL;
    labeled_size = 0;
    labeled_len = 0;
}

StackEncoder::~StackEncoder() {
    if (last_stack) delete [] last_stack;
    last_stack = NULL;
    last_stack_size = 0;
    last_stack_len = 0;
    if (stacks) delete [] stacks;
    stacks = NULL;
    stacks_size = 0;
    if (gcRefs) delete [] gcRefs;
    gcRefs = NULL;
    gcRefs_size = 0;
    if (interiorRefs) delete [] interiorRefs;
    interiorRefs = NULL;
    interiorRefs_size = 0;
    if (labeled) delete [] labeled;
    labeled = NULL;
    labeled_size = 0;
}

/* reset so we can be reused */
void StackEncoder::reset() {
    jitInfo = NULL;
#ifdef _DEBUG
    if (last_stack_len) {
        memset(last_stack, 0xFF, last_stack_len*sizeof(OpType));
        last_stack_len = 0;
    }
    if (stacks_len) {
        memset(stacks, 0xFF, stacks_len);
        stacks_len = 0;
    }
    if (gcRefs_len) {
        memset(gcRefs, 0xFF, gcRefs_len);
        gcRefs_len = 0;
    }
    if (interiorRefs_len) {
        memset(interiorRefs, 0xFF, interiorRefs_len);
        interiorRefs_len = 0;
    }
    if (labeled_len) {
        memset(labeled, 0xFF, labeled_len*sizeof(labeled_stacks));
        labeled_len = 0;
    }
#else
    last_stack_len = 0;
    stacks_len = 0;
    gcRefs_len = 0;
    interiorRefs_len = 0;
    labeled_len = 0;
#endif
    //put an empty stack at the front of stacks
    if (stacks_size < 2) {
        if (stacks) delete [] stacks;
        stacks_size = 2;
        New(stacks,unsigned char[stacks_size]);
    }
    stacks[stacks_len++] = 0;
    stacks[stacks_len++] = 0;
    //put an empty labeled stack at pcOffset 0
    if (!labeled_size) {
        labeled_size = 1;
        if ((labeled = (labeled_stacks*) new unsigned char[labeled_size*sizeof(labeled_stacks)]) == NULL)
            RaiseException(SEH_NO_MEMORY,EXCEPTION_NONCONTINUABLE,0,NULL);
    }
    labeled[labeled_len].pcOffset = 0;
    labeled[labeled_len++].stackToken = 0;
}

/* append the stack state at pcOffset to the end */
void StackEncoder::append(unsigned pcOffset, OpType* op_stack, unsigned int op_stack_len) {

    /*check it is the same as the last stack */
    unsigned int num = op_stack_len;
    bool same = (last_stack_len == num);
    while (same && num--)
        same = same && (last_stack[num] == op_stack[num]);
    if (same) return;

    /*                                                */
    last_stack_len = op_stack_len;
    if (last_stack_len > last_stack_size) {
        last_stack_size = FJit::growBuffer((unsigned char**)&last_stack,0, last_stack_len*sizeof(OpType));
        last_stack_size /= sizeof(OpType);
    }
    for (num = 0; num < op_stack_len; num++)
    {
        last_stack[num] = op_stack[num];
    }

    /* make sure we have space for a new labeled stack */
    if (labeled_len >= labeled_size) {
        labeled_size = FJit::growBuffer((unsigned char**)&labeled, labeled_len*sizeof(labeled_stacks), (labeled_len+1)*sizeof(labeled_stacks));
        labeled_size = labeled_size / sizeof(labeled_stacks);
    }

    /* encode the stack and add to labeled stacks */
    unsigned int stackToken = encodeStackForGC(op_stack, op_stack_len);
    labeled[labeled_len].pcOffset = pcOffset;
    labeled[labeled_len++].stackToken = stackToken;
}

/* compress the labeled stacks into the buffer in gcHdrInfo format */
void StackEncoder::compress(unsigned char** buffer, unsigned int* buffer_len, unsigned int* buffer_size) {
    unsigned size;
    unsigned compressed_size;

    /* compress labeled onto itself,
       this words since the compressed form of an entry is smaller than the orig entry
       */
    unsigned char* outPtr = (unsigned char*) labeled;
    labeled_stacks entry;
    for (unsigned i = 0; i < labeled_len; i++) {
        _ASSERTE((unsigned) outPtr <= (unsigned)&labeled[i]);
        entry = labeled[i];
        FJit_Encode::encode(entry.pcOffset, &outPtr);
        FJit_Encode::encode(entry.stackToken, &outPtr);

    }
    compressed_size = ((unsigned)outPtr) - ((unsigned)labeled);
#ifdef _DEBUG
    memset(outPtr, 0xEE, labeled_len*sizeof(labeled_stacks) - compressed_size);
#endif

    /* compute size in bytes of encode form of compressed labeled and make room for it */
    size = compressed_size + FJit_Encode::encodedSize(labeled_len);

    if ((*buffer_len) + size +4 > (*buffer_size)) { // 4 bytes = overestimation of encodedSize(size)
        (*buffer_size) = FJit::growBuffer(buffer, *buffer_len, (*buffer_len)+size);
    }
    //move the compressed labeled stacks into buffer
    outPtr = &(*buffer)[*buffer_len];
    (*buffer_len) += FJit_Encode::encode(size, &outPtr);            //#bytes
    (*buffer_len) += FJit_Encode::encode(labeled_len, &outPtr);     //#labeled stacks
    memcpy(outPtr, labeled, compressed_size);                       //compressed labeled bytes
    (*buffer_len) += compressed_size;

    /* compute size needed for the compressed stacks and make room */
    size = stacks_len + FJit_Encode::encodedSize(stacks_len);
    if ((*buffer_len) + size > (*buffer_size)) {
        *buffer_size = FJit::growBuffer(buffer, *buffer_len, (*buffer_len)+size);
    }
    outPtr = &(*buffer)[*buffer_len];
    //move the encoded stacks into the buffer
    (*buffer_len) += FJit_Encode::encode(stacks_len, &outPtr);  //#bytes
    memcpy(outPtr, stacks, stacks_len);                         //compressed stacks bytes
    (*buffer_len) += stacks_len;
#ifdef _DEBUG
    outPtr = &(*buffer)[*buffer_len];
#endif
    _ASSERTE((unsigned)outPtr <= (unsigned)&(*buffer)[*buffer_size]);
}

/* encode the stack into the stacks buffer, return the index where it was placed */
unsigned int StackEncoder::encodeStackForGC(OpType* op_stack, unsigned int op_stack_len) {

    if (!op_stack_len) return 0;    //empty stack encodings is at front of stacks

    /* compute the gcRefs and interiorRefs boolean arrays for the stack */
    gcRefs_len = 0;
    interiorRefs_len = 0;
    unsigned int op_word = 0;       //logical operand word we are looking at

    for (unsigned int op = 0; op < op_stack_len; op++) {
        if (!op_stack[op].isPrimitive() && !op_stack[op].isMethod() && !op_stack[op].isNull() ) {
            unsigned int words = typeSizeInSlots(jitInfo, op_stack[op].cls());       //#of void* sized words in op
            op_word += (WORD_ALIGNED(words)-words);
            if (op_word + words > gcRefs_size) {
                gcRefs_size = FJit::growBuffer(&gcRefs, gcRefs_len, op_word + words);
            }
            memset(&gcRefs[gcRefs_len], 0, op_word-gcRefs_len);
            gcRefs_len = op_word;
            jitInfo->getClassGClayout(op_stack[op].cls(), (BYTE*)&gcRefs[gcRefs_len]);
            if (words > 1) {
                for (unsigned index = 0; index < words/2; index++){
                    unsigned char temp = gcRefs[gcRefs_len+index];
                    gcRefs[gcRefs_len+index] = gcRefs[gcRefs_len+words-1-index];
                    gcRefs[gcRefs_len+words-1-index] = temp;
                }
            }
            op_word += words;
            gcRefs_len = op_word;
        }
        else {
            switch (op_stack[op].enum_()) {
                default:
                    _ASSERTE(!"Void or Undef as a operand is not allowed");
                    break;
                case typeMethod:
                case typeI4:
                case typeR4:
                    op_word += (SIZE_STACK_SLOT / sizeof(void *));
                    break;
                case typeI8:
                case typeR8:
                    op_word += (8 / sizeof(void*));
                    break;
                case typeRef:
                    op_word += ((SIZE_STACK_SLOT - sizeof(void*)) / sizeof(void *));
                    if (op_word+1 > gcRefs_size) {
                        gcRefs_size = FJit::growBuffer(&gcRefs, gcRefs_len, op_word+1);
                    }
                    memset(&gcRefs[gcRefs_len], 0, op_word-gcRefs_len);
                    gcRefs_len = op_word;
                    gcRefs[gcRefs_len++] = true;
                    op_word += 1;
                    break;
                case typeRefAny:
                    if (op_word + 2 > interiorRefs_size) {
                        interiorRefs_size = FJit::growBuffer(&interiorRefs, interiorRefs_len, op_word+2);
                    }
                    memset(&interiorRefs[interiorRefs_len], 0, op_word-interiorRefs_len);
                    interiorRefs_len = op_word;
                    interiorRefs[interiorRefs_len++] = true;
                    interiorRefs[interiorRefs_len++] = false;
                    op_word += 2;
                    break;
                case typeByRef:
                    op_word += ((SIZE_STACK_SLOT - sizeof(void*)) / sizeof(void *));
                    if (op_word + 1 > interiorRefs_size) {
                        interiorRefs_size = FJit::growBuffer(&interiorRefs, interiorRefs_len, op_word+1);
                    }
                    memset(&interiorRefs[interiorRefs_len], 0, op_word-interiorRefs_len);
                    interiorRefs_len = op_word;
                    interiorRefs[interiorRefs_len++] = true;
                    op_word += 1;
                    break;
            }
        }
    }

    /* encode the op stack gc refs and interior refs into the stacks buffer */
    unsigned int result = stacks_len;       //where we are putting it, and what we return
    unsigned char* outPtr;
    //make sure there is space
    unsigned int size = (gcRefs_len + 7 + interiorRefs_len + 7) / 8 + 4;    //overestimate is okay, underestimate is disaster
    if (stacks_len+size > stacks_size) {
        stacks_size = FJit::growBuffer(&stacks, stacks_len, stacks_len+size);
    }
    //drop the pieces in
    size = FJit_Encode::compressBooleans(gcRefs, gcRefs_len);
    outPtr = &stacks[stacks_len];
    stacks_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, gcRefs, size);
    stacks_len += size;
    size = FJit_Encode::compressBooleans(interiorRefs, interiorRefs_len);
    outPtr = &stacks[stacks_len];
    stacks_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, interiorRefs, size);
    stacks_len += size;
    _ASSERTE(stacks_len <= stacks_size);

    return result;
}
