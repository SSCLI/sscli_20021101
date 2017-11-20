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
#include "ejitmgr.h"
#include "eetwain.h"
#include "fjit_eetwain.h"
#include "cgensys.h"
#include "dbginterface.h"


#if CHECK_APP_DOMAIN_LEAKS
#define CHECK_APP_DOMAIN    GC_CALL_CHECK_APP_DOMAIN
#else
#define CHECK_APP_DOMAIN    0
#endif

#if defined(_DEBUG)
#define REPORT_NAME(exp) ,exp
#else
#define REPORT_NAME(exp)
#endif

/*****************************************************************************
 *
 *  Decodes the methodInfoPtr and returns the decoded information
 *  in the hdrInfo struct.  The EIP parameter is the PC location
 *  within the active method.  Answer the number of bytes read.
 */
static size_t   crackMethodInfoHdr(unsigned char* compressed,
                                   SLOT    methodStart,
                                    Fjit_hdrInfo   * infoPtr)
{
    memcpy(infoPtr, compressed, sizeof(Fjit_hdrInfo));
    return sizeof(Fjit_hdrInfo);
};

/********************************************************************************
 *  Promote the object, checking interior pointers on the stack is supposed to be done
 *  in pCallback
 */
void promote(GCEnumCallback pCallback, LPVOID hCallBack, OBJECTREF* pObj, DWORD flags /* interior or pinned */
#ifdef _DEBUG
        ,char* why  = NULL     
#endif
             )
{
    LOG((LF_GC, INFO3, "    Value %x at %x being Promoted to ", OBJECTREFToObject(*pObj), pObj));
    pCallback(hCallBack, pObj, flags | CHECK_APP_DOMAIN);
    LOG((LF_GC, INFO3, "%x ", OBJECTREFToObject(*pObj)));
#ifdef _DEBUG
    LOG((LF_GC, INFO3, " because %s\n", why));
#endif 
}

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside a catch handler.
*/
void Fjit_EETwain::FixContext(
                ContextType     ctxType,
                EHContext      *ctx,
                LPVOID          methodInfoPtr,
                LPVOID          methodStart,
                DWORD           nestingLevel,
                OBJECTREF       thrownObject,
                CodeManState   *pState,
                size_t       ** ppShadowSP,
                size_t       ** ppEndRegion)
{
    assert((ctxType == FINALLY_CONTEXT) == (thrownObject == NULL));

    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)methodStart, &hdrInfo);
    
    // Get the FP for the internal frame
    PVOID* pFrameBase = getInternalFP(ctx->GetFP()); 

    // Determine the correct SP value depending on if there was space allocated inside the frame
    // via localloc and on being inside an EH subframe
    if (nestingLevel > 0)
      ctx->SetSP((PVOID)((size_t)getSavedSPForEHClause( pFrameBase, nestingLevel )-SIZE_LINKAGE_AREA));
    else 
      ctx->SetSP((PVOID)((size_t)getSPForInternalFrame( pFrameBase, ctx->GetFP(), hdrInfo.methodFrame )-SIZE_LINKAGE_AREA));

    // Set the exception object (this is where the handler expects it by convention)
    ctx->SetArg(OBJECTREFToObject(thrownObject));
}



/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext.
    Returns success of operation.
*/
bool Fjit_EETwain::UnwindStackFrame(
                PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags,
                CodeManState   *pState)
{


    if (methodInfoPtr == NULL)
    { // this can only happen if we got stopped in the ejit stub and the GC info for the method has been pitched

        _ASSERTE(EconoJitManager::IsInStub( (BYTE*) (*(ctx->pPC)), FALSE));
        ctx->pPC = (SLOT*)(size_t)GetRegdisplaySP(ctx);
        SetRegdisplaySP(ctx, (LPVOID)((size_t)GetRegdisplaySP(ctx)+ sizeof(void *)));
        return true;
    }

    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)pCodeInfo, &hdrInfo);
    PVOID* pFrameBase = getInternalFP(GetRegdisplayFP(ctx));
    unsigned EHClauseNestingDepth = getEHClauseNestingDepth(pFrameBase);

    /* since Fjit methods have exactly one prolog and one epilog (at end), 
    we can determine if we are in them without needing a map of the entire generated code.
    */

        /*prolog/epilog sequence is:
            push ebp        (1byte) 
            mov  ebp,esp    (2byte)
            push esi        (1byte) //callee saved
            xor  esi,esi            //used for newobj, initialize for liveness
            push 0                  //security obj == NULL */
      /*     ... note that Fjit does not use any of the callee saved registers in prolog
            ...
            mov esi,[ebp-4]
            mov esp,ebp     
            pop ebp         
            ret [xx]        

        NOTE: This needs to match the code in fjit\x86fjit.h 
              for the emit_prolog and emit_return macros
        */


    SLOT     breakPC = (SLOT) *(ctx->pPC);
    DWORD    offset; 
    METHODTOKEN ignored;
    EconoJitManager::JitCode2MethodTokenAndOffsetStatic(breakPC,&ignored,&offset);
     _ASSERTE(offset < hdrInfo.methodSize);

    if (((unsigned)offset >= hdrInfo.prologSize) && ((unsigned)offset < (hdrInfo.methodSize-hdrInfo.epilogSize))) 
    {
        LOG((LF_CORDB, LL_INFO1000000, "Fjit_EETwain::UnwindStackFrame not in prolog\n"));

       /* check if we might be in a filter */   
       if ( (hdrInfo.methodJitGeneratedLocalsSize > sizeof(void*)) && EHClauseNestingDepth ) // are we in an EH?
       {
        LOG((LF_CORDB, LL_INFO1000000, "Fjit_EETwain::UnwindStackFrame EHClauseNestingDepth set 0x%x\n", EHClauseNestingDepth));

        /* search for clause whose esp base is greater/equal to ctx->Esp */
        while (getSavedSPForEHClause(pFrameBase, EHClauseNestingDepth) < GetRegdisplaySP(ctx))
        { 
            EHClauseNestingDepth--;
            if (EHClauseNestingDepth <= 0) 
                break;
        }
        /* are we in a filter? */
        if (EHClauseNestingDepth > 0 && isFilterEHClause(pFrameBase, EHClauseNestingDepth) ) 
        {
            // only unwind this funclet
            PVOID baseSP = getSavedSPForEHClause(pFrameBase, EHClauseNestingDepth);  
            ctx->pPC = (SLOT*)baseSP;
            SetRegdisplaySP(ctx, (BYTE*)baseSP + sizeof(void*));
            return true;
        }
       }
    }
    else
    {
        LOG((LF_CORDB, LL_ERROR, "Fjit_EETwain::UnwindStackFrame offset = 0x%x\n", offset));
        LOG((LF_CORDB, LL_ERROR, "Fjit_EETwain::UnwindStackFrame epilog = 0x%x\n", hdrInfo.methodSize-hdrInfo.epilogSize));
        LOG((LF_CORDB, LL_ERROR, "Fjit_EETwain::UnwindStackFrame prolog = 0x%x\n", hdrInfo.prologSize));
        LOG((LF_CORDB, LL_ERROR, "Fjit_EETwain::UnwindStackFrame methodSize = 0x%x\n", hdrInfo.methodSize));
    }

#if defined(_X86_)
    if (offset <= hdrInfo.prologSize) {
        if(offset == 0) goto ret_XX;
        if(offset == 1) goto pop_ebp;
        if(offset == 3) goto mov_esp;
        /* callee saved regs have been pushed, frame is complete */
        goto mov_esi;
    }
    /* if the next instr is: ret [xx] -- 0xC2 or 0xC3, we are at the end of the epilog.
       we need to call an API to examine the instruction scheme, since the debugger might
       have placed a breakpoint. For now, the ret can be determined by figuring out 
       distance from the end of the epilog. NOTE: This works because we have only one
       epilog per method. 
       if not, ebp is still valid 
       */
    
    unsigned offsetFromEnd;
    offsetFromEnd = (unsigned) (hdrInfo.methodSize-offset);
    if (offsetFromEnd <= 10) 
    {
        if (offsetFromEnd  ==   1) {
            //sanity check, we are at end of method
            goto ret_XX;
        }
        else 
        {// we are in the epilog
            goto mov_esi;
        }
    }
    /* we still have a set up frame, simulate the epilog by loading saved registers
    off of ebp. */
mov_esi:
    //restore esi, simulate mov esi,[ebp-4]
    ctx->pEsi = (DWORD *)(size_t)GetAddrOfSavedRegisterInternal(pFrameBase);
mov_esp:
    //simulate mov esp,ebp
    SetRegdisplaySP(ctx, (LPVOID)(size_t)GetRegdisplayFP(ctx)); 
pop_ebp:
    //simulate pop ebp
    ctx->pEbp = (DWORD*)(size_t)GetRegdisplaySP(ctx);
    SetRegdisplaySP(ctx, (LPVOID)((size_t)GetRegdisplaySP(ctx)+ sizeof(void *)));
ret_XX:
    //simulate the ret XX
    ctx->pPC = (SLOT*)(size_t)GetRegdisplaySP(ctx);
    SetRegdisplaySP(ctx, (LPVOID)((size_t)GetRegdisplaySP(ctx)+ sizeof(void *) + hdrInfo.methodArgsSize));
    return true;

#elif defined(_PPC_)
    // Check if the frame is note completely constructed
    if (offset <= hdrInfo.prologSize) {
        if (offset < 8)  goto restorePCFromLR;
        if (!hdrInfo.savedIP) offset += 12;
        if (offset < 20) goto restorePCFromStack;
        if (offset < 28) goto restoreR28;
        if (offset < 32) goto restoreFPFromStack;
        /*  frame is complete */
        goto restorePPCFrame;
    }

    // Check if the frame is partially destroyed
    unsigned offsetFromEnd;
    offsetFromEnd = (unsigned) (hdrInfo.methodSize-offset);
    if (offsetFromEnd < 24 || hdrInfo.savedIP && offsetFromEnd < 28) 
    {
        if (offsetFromEnd == 4)    goto restorePCFromLR;
        if (offsetFromEnd == 8)    goto restorePCFromStack;
        if (offsetFromEnd == 12)   goto restorePCFromStack;
        if (!hdrInfo.savedIP)      offsetFromEnd += 4;
        if (offsetFromEnd == 16 )  goto restoreR28; 
        if (offsetFromEnd == 20 )  goto restoreFPFromStack2;
        if (offsetFromEnd == 24 )  goto restoreFPFromStack;
    }

restorePPCFrame:
    // r29 = r30[-8]
    ctx->pR[29-13] = (DWORD *)(size_t)GetAddrOfSavedRegisterInternal(pFrameBase);
restoreFPFromStack:
    //r1 = r30
    SetRegdisplaySP(ctx, (LPVOID)(size_t)GetRegdisplayFP(ctx)); 
restoreFPFromStack2:
    //r30 = r1[-4]
    ctx->pR[30-13] = (DWORD *)((size_t)GetRegdisplaySP(ctx)- sizeof(void *));
restoreR28:
    // r28 = r1[+4]
    if (hdrInfo.savedIP) 
      ctx->pR[28-13] = (DWORD *)((size_t)GetRegdisplaySP(ctx)+sizeof(void *));
restorePCFromStack:
    //LR = r1[8]
    ctx->pPC = (SLOT*)((size_t)GetRegdisplaySP(ctx) + 2*sizeof(void*));
    return true;
restorePCFromLR:
    // Restore LR from the context
    ctx->pPC = (SLOT*)&(ctx->pContext->Lr);
    return true;
#else
    PORTABILITY_ASSERT("UnwindStackFrame (fjit_eetwain.cpp) is not implemented on this platform.");
    return false;
#endif
    
}

void Fjit_EETwain::HijackHandlerReturns(PREGDISPLAY     ctx,
                                        LPVOID          methodInfoPtr,
                                        ICodeInfo      *pCodeInfo,
                                        IJitManager*    jitmgr,
                                        CREATETHUNK_CALLBACK pCallBack
                                        )
{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)pCodeInfo, &hdrInfo);
 
    // If there are no exception clauses, just return
    if (hdrInfo.methodJitGeneratedLocalsSize <= sizeof(void*)) 
        return; 

    PVOID* pFrameBase = getInternalFP(GetRegdisplayFP(ctx));
    unsigned EHClauseNestingDepth = getEHClauseNestingDepth(pFrameBase);
    while (EHClauseNestingDepth) 
    {
        // only call back if not a filter, since filters are called by the EE
        if (!isFilterEHClause(pFrameBase, EHClauseNestingDepth))   
        {
            pCallBack(jitmgr, (LPVOID*)getSavedSPForEHClause(pFrameBase, EHClauseNestingDepth) , pCodeInfo );
        }
        EHClauseNestingDepth--;
    }

    return;
}

/*
    Is the function currently at a "GC safe point" ?
*/

// NOP for the various platforms
#if defined(_X86_)
#define OPCODE_SEQUENCE_POINT 0x90
#elif defined(_PPC_)
#define OPCODE_SEQUENCE_POINT 0x60000000
#else
PORTABILITY_ASSERT("We need the NOP for this platform");
#endif

bool Fjit_EETwain::IsGcSafe(  PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags)

{
#ifdef DEBUGGING_SUPPORTED
    LOG((LF_CORDB, LL_INFO100000, "EJM::IsGcSafe 0x%x\n", *pContext->pPC));

    // Only the debugger cares to get a more precise answer. If there
    // is no debugger attached, just return false.
    if (!CORDebuggerAttached())
        return false;

    LOG((LF_CORDB, LL_INFO100000, "EJM::IsGcSafe Degugger attached\n"));
    BYTE* IP = (BYTE*) (*(pContext->pPC));

    // Has the method been pitched? If it has, then of course we are
    // at a GC safe point.
    if (EconoJitManager::IsCodePitched(IP))
        return true;

    // Does the IP point to the start of a thunk? If so, then the
    // method was really moved/preserved, but we still know we were at
    // a GC safe point.
    if (EconoJitManager::IsThunk(IP))
        return true;
    
    // Else see if we are at a sequence point.
    DWORD Opcode = g_pDebugInterface->GetPatchedOpcode(IP);
    LOG((LF_CORDB, LL_INFO1000000, "EJM::IsGcSafe opcode 0x%x\n", Opcode));
    return (Opcode == OPCODE_SEQUENCE_POINT);
#else // !DEBUGGING_SUPPORTED
    return false;
#endif // !DEBUGGING_SUPPORTED
}

/* report all the args (but not THIS if present), to the GC. 'framePtr' points at the 
   frame (promote doesn't assume anthing about its structure).  'msig' describes the
   arguments, and 'ctx' has the GC reporting info.  'stackArgsOffs' is the byte offset
   from 'framePtr' where the arguments start (args start at last and grow bacwards).
   Simmilarly, 'regArgsOffs' is the offset to find the register args to promote,
   This also handles the varargs case */
void promoteArgs(BYTE* framePtr, MetaSig* msig, GCCONTEXT* ctx, int stackArgsOffs, int regArgsOffs) {

    LPVOID pArgAddr;
    if (msig->GetCallingConvention() == IMAGE_CEE_CS_CALLCONV_VARARG) {
        // For varargs, look up the signature using the varArgSig token passed on the stack
        // With __cdecl calling convention the vararg_token is pushed onto the stack before the 
        // the this pointer and the return buffer pointer if these are present.
        VASigCookie* varArgSig = *((VASigCookie**) (framePtr + stackArgsOffs 
                                                    + (msig->HasThis() ? sizeof(void*) : 0 )
                                                    + (msig->HasRetBuffArg() ? sizeof(void*) : 0)
	                                            ));

        MetaSig varArgMSig(varArgSig->mdVASig, varArgSig->pModule);
        msig = &varArgMSig;

        ArgIterator argit(framePtr, msig, stackArgsOffs, regArgsOffs);
        while (NULL != (pArgAddr = argit.GetNextArgAddr()))
            msig->GcScanRoots(pArgAddr, ctx->f, ctx->sc);
        return;
    }
    
    ArgIterator argit(framePtr, msig, stackArgsOffs, regArgsOffs);
    while (NULL != (pArgAddr = argit.GetNextArgAddr()))
        msig->GcScanRoots(pArgAddr, ctx->f, ctx->sc);
}


/*
    Enumerate all live object references in that function using
    the virtual register set.
    Returns success of operation.
*/

static DWORD gcFlag[4] = {0, GC_CALL_INTERIOR, GC_CALL_PINNED, GC_CALL_INTERIOR | GC_CALL_PINNED}; 

bool Fjit_EETwain::EnumGcRefs(PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        pcOffset,
                unsigned        flags,
                GCEnumCallback  pCallback,
                LPVOID          hCallBack)

{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    IJitManager* jitMgr = ExecutionManager::GetJitForType(miManaged_IL_EJIT);
    compressed += crackMethodInfoHdr(compressed, (SLOT)pCodeInfo, &hdrInfo);

    /* if execution aborting, we don't have to report any args or temps or regs
        so we are done */
    if ((flags & ExecutionAborted) && (flags & ActiveStackFrame) ) 
        return true;

    /* if in prolog or epilog, we must be the active frame and we must be aborting execution,
       since fjit runs uninterrupted only, and we wouldn't get here */
    if (((unsigned)pcOffset < hdrInfo.prologSize) || ((unsigned)pcOffset > (hdrInfo.methodSize-hdrInfo.epilogSize))) {
        _ASSERTE(!"interrupted in prolog/epilog and not aborting");
        return false;
    }

#ifdef _DEBUG
    LPCUTF8 cls, name = pCodeInfo->getMethodName(&cls);
    LOG((LF_GC, INFO3, "GC reporting from %s.%s\n",cls,name ));
#endif

    PVOID* pFrameBase = getInternalFP(GetRegdisplayFP(ctx));

    OBJECTREF* pArgPtr;
    IFJitCompiler* fjit = (IFJitCompiler*) jitMgr->m_jit;
    FJit_Encode* encoder = fjit->getEncoder();

    /* report enregistered values, fjit only enregisters ESI */
    /* always report ESI since it is only used by new obj and is forced NULL when not in use */
    promote(pCallback,hCallBack, (OBJECTREF*)(size_t)GetCalleeSavedRegP(ctx), 0 REPORT_NAME("Callee Saved Register(x86 = ESI)"));

    /* report security object */
    OBJECTREF* pSecurityObject = GetAddrOfSecurityObjectInternal(pFrameBase);
    promote(pCallback,hCallBack, pSecurityObject, 0 REPORT_NAME("Security object"));
 
    // check if we can be in a EH clause
    unsigned EHClauseNestingDepth = getEHClauseNestingDepth(pFrameBase);
    if (hdrInfo.methodJitGeneratedLocalsSize > sizeof(void*) && EHClauseNestingDepth)
    {
        /* search for clause whose esp base is greater/equal to ctx->Esp */
        while (getSavedSPForEHClause(pFrameBase, EHClauseNestingDepth) < GetRegdisplaySP(ctx))
        { 
            EHClauseNestingDepth--;
            if (EHClauseNestingDepth <= 0) 
               break;
        }

        /* we are in the (index+1)'th handler */
        /* are we in a filter? */
        if (EHClauseNestingDepth > 0 && isFilterEHClause(pFrameBase, EHClauseNestingDepth) )
        {     
	    pArgPtr = (OBJECTREF*)getSavedSPForEHClause(pFrameBase, EHClauseNestingDepth);
            /* now report the pending operands */
            unsigned num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for non-interior locals
            compressed += num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for interior locals
            compressed += num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for non-interior pinned locals
            compressed += num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for interior pinned locals
            compressed += num;
            goto REPORT_PENDING_ARGUMENTS;
        }
    }
    else
        EHClauseNestingDepth = 0;

    /* if we come here, we are not in a filter, so report all locals, arguments, etc. */
    
    /* report <this> for non-static methods */
    if (hdrInfo.hasThis) 
    {
        /* fjit always places <this> on the stack in the prolog */
        pArgPtr = GetInstanceInternal(pFrameBase, hdrInfo.hasRetBuff);

        BOOL interior = (pCodeInfo->getClassAttribs() & CORINFO_FLG_VALUECLASS) != 0;

        promote(pCallback,hCallBack, pArgPtr, (interior ? GC_CALL_INTERIOR : 0) REPORT_NAME(" THIS ptr "));
     }
 
    { // enumerate the argument list, looking for GC references

#ifdef _DEBUG        
    // Note that I really want to say hCallBack is a GCCONTEXT, but this is pretty close
    extern void GcEnumObject(LPVOID pData, OBJECTREF *pObj, DWORD flags);
    _ASSERTE((void*) GcEnumObject == pCallback);
#endif
    GCCONTEXT   *pCtx = (GCCONTEXT *) hCallBack;

    MethodDesc * pFD = (MethodDesc *)pCodeInfo->getMethodDesc_HACK();

    MetaSig msig(pFD->GetSig(),pFD->GetModule());
    
    if (msig.HasRetBuffArg()) 
    {
        pArgPtr = (OBJECTREF*)GetReturnBufferInternal(pFrameBase, hdrInfo.hasThis );          
        promote(pCallback, hCallBack, pArgPtr, GC_CALL_INTERIOR REPORT_NAME("RetBuffArg"));
    }

    promoteArgs((BYTE*)(size_t)GetRegdisplayFP(ctx), &msig, pCtx,
                sizeof(prolog_frame),                                    // where the args start (skip varArgSig token)
	        0 );

      /*       The stack layout is: */
      /*                 Security Object
                         Saved ESI
                         EBP
                         Return Address
                         Arg3
                         ..
                         ArgN

     */
    }

    /* report locals 
       NOTE: fjit lays out the locals in the same order as specified in the il, 
              sizes of locals are (I4,I,REF = sizeof(void*); I8 = 8)
       */
    
    //point to start of the locals frame, note that the locals grow down from here on x86
    pArgPtr = (OBJECTREF*)(((size_t)GetRegdisplayFP(ctx))+prolog_bias-hdrInfo.methodJitGeneratedLocalsSize);
    
    /* at this point num_local_words is the number of void* words defined by the local tail sig
       and pArgPtr points to the start of the tail sig locals
       and compressed points to the compressed form of the localGCRef booleans
       */

    //handle gc refs and interiors in the tail sig (compressed localGCRef booleans followed by interiorGC booleans followed by pinned GC and pinned interior GC)
    bool interior;
    interior = false;
    OBJECTREF* pLocals;
    pLocals = pArgPtr;
    unsigned i;
    for (i = 0; i<4; i++) {
        unsigned num;
        unsigned char bits;
        OBJECTREF* pReport;
        pArgPtr = pLocals-1;    //-1 since the stack grows down on x86
        pReport = pLocals-1;    // ...ditto
        num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read
        while (num > 1) {
            pReport = pArgPtr;
            bits = *compressed++;
            while (bits > 0) {
                if (bits & 1) {
                    //report the gc ref
                    promote(pCallback,hCallBack, pReport, gcFlag[i] REPORT_NAME("LOCALS"));
                }
                pReport--;
                bits = bits >> 1;
            }
            num--;
            pArgPtr -=8;
        }
        bits = 0;
        if (num) bits = *compressed++;
        while (bits > 0) {
            if (bits & 1) {
                //report the gc ref
                promote(pCallback,hCallBack, pArgPtr, gcFlag[i] REPORT_NAME("LOCALS"));
            }
            pArgPtr--;
            bits = bits >> 1;
        }
    } ;
    
    // Determine the base of the evaluation stack
    if (EHClauseNestingDepth) 
    { 
      // we are in a finally/fault/catch handler.
      // NOTE: the pending operands before the handler is entered are dead and are not reported  
      pArgPtr = (OBJECTREF*)getSavedSPForEHClause(pFrameBase, EHClauseNestingDepth);
    }
    else 
    {   
      // we are not in a handler, but there might have been a localloc 
      pArgPtr = (OBJECTREF*)getSPForInternalFrame( pFrameBase, GetRegdisplayFP(ctx), hdrInfo.methodFrame );
    }

REPORT_PENDING_ARGUMENTS:

    /* report pending args on the operand stack (those not covered by the call itself)
       Note: at this point pArgPtr points to the start of the operand stack, 
             on x86 it grows down from here.
             compressed points to start of labeled stacks */
    /* read down to the nearest labeled stack that is before currect pcOffset */
    struct labeled_stacks {
        unsigned pcOffset;
        unsigned int stackToken;
    };
    //set up a zero stack as the current entry
    labeled_stacks current;
    current.pcOffset = 0;
    current.stackToken = 0;
    unsigned size = encoder->decode_unsigned(&compressed);  //#bytes in labeled stacks
    unsigned char* nextTableStart  = compressed + size;      //start of compressed stacks

    if (flags & ExecutionAborted)  // if we have been interrupted we don't have to report pending arguments
        goto INTEGRITY_CHECK;      // since we don't support resumable exceptions

    unsigned num; num= encoder->decode_unsigned(&compressed);   //#labeled entries
    while (num--) {
        labeled_stacks next;
        next.pcOffset = encoder->decode_unsigned(&compressed);
        next.stackToken = encoder->decode_unsigned(&compressed);
        if (next.pcOffset >= pcOffset) break;
        current = next;
    }
    compressed = nextTableStart;
    size = encoder->decode_unsigned(&compressed);           //#bytes in compressed stacks
#ifdef _DEBUG
    //point past the stacks, in case we have more data for debug
    nextTableStart = compressed + size; 
#endif //_DEBUG             
    if (current.stackToken) {
        //we have a non-empty stack, so we have to walk it
        compressed += current.stackToken;                           //point to the compressed stack
        //handle gc refs and interiors in the tail sig (compressed localGCRef booleans followed by interiorGC booleans)
        bool interior = false;
        DWORD gcFlag = 0;

        OBJECTREF* pLocals = pArgPtr;
        do {
            unsigned num;
            unsigned char bits;
            OBJECTREF* pReport;
            pArgPtr = pLocals-1;    //-1 since the stack grows down on x86
            pReport = pLocals-1;    // ...ditto
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read
            while (num > 1) {
                bits = *compressed++;
                while (bits > 0) {
                    if (bits & 1) {
                        //report the gc ref
		      promote(pCallback,hCallBack, pReport, gcFlag REPORT_NAME("PENDING ARGUMENT"));
                    }
                    pReport--;
                    bits = bits >> 1;
                }
                num--;
                pArgPtr -=8;
                pReport = pArgPtr;
            }
            bits = 0;
            if (num) bits = *compressed++;
            while (bits > 0) {
                if (bits & 1) {
                    //report the gc ref
		  promote(pCallback,hCallBack, pArgPtr, gcFlag REPORT_NAME("PENDING ARGUMENT"));
                }
                pArgPtr--;
                bits = bits >> 1;
            }
            interior = !interior;
            gcFlag = GC_CALL_INTERIOR;
        } while (interior);
    }

INTEGRITY_CHECK:    

#ifdef _DEBUG
    if (!(flags & ExecutionAborted))
    {
        /* for an intergrity check, we placed a map of pc offsets to an il offsets at the end,
           lets try and map the current pc of the method*/
        compressed = nextTableStart;
        encoder->decompress(compressed);
        unsigned pcInILOffset;
        // if we are not the active stack frame, we must be in a call instr, back up the pc so the we return the il of the 
        // call. However, we have to watch out for synchronized methods.  The call to synchronize
        // is the last instruction of the prolog.  The instruction we return to is legitimately
        // outside the prolog.  We don't want to back up into the prolog to sit on that call.

        //  signed ilOffset = encoder->ilFromPC((flags & ActiveStackFrame) ? pcOffset : pcOffset-1, &pcInILOffset);
        signed ilOffset = encoder->ilFromPC((unsigned)pcOffset, &pcInILOffset);

        //fix up the pcInILOffset if necessary
        pcInILOffset += (flags & ActiveStackFrame) ? 0 : 1;
        if (ilOffset < 0 ) 
        {
           _ASSERTE(!"bad il offset" );
            return false;
        }
    }
#endif //_DEBUG

    delete encoder;

    return true;

}

/*
    Return the address of the local security object reference
*/
OBJECTREF* Fjit_EETwain::GetAddrOfSecurityObject(
                PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                unsigned        relOffset,
                CodeManState   *pState)

{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed,  (SLOT)(size_t)relOffset, &hdrInfo);

    PVOID* pFrameBase = getInternalFP(GetRegdisplayFP(ctx));
    return GetAddrOfSecurityObjectInternal(pFrameBase);
}

/*
    Returns "this" pointer if it is a non-static method AND
    the object is still alive.
    Returns NULL in all other cases.
*/
OBJECTREF Fjit_EETwain::GetInstance(
                PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                unsigned        relOffset)

{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)(size_t)relOffset, &hdrInfo);

    if(!hdrInfo.hasThis) 
        return NULL;

    // Depending on the calling convention used the 'this' pointer is either the first registered argument [ECX] or
    // the first stack argument [EBP+8] 
    PVOID* pFrameBase = getInternalFP(GetRegdisplayFP(ctx));
    OBJECTREF* tmp_this = GetInstanceInternal(pFrameBase, hdrInfo.hasRetBuff);
    return *tmp_this; 
}

/*
  Returns true if the given IP is in the given method's prolog or an epilog.
*/
bool Fjit_EETwain::IsInPrologOrEpilog(
                DWORD  relPCOffset,
                LPVOID methodInfoPtr,
                size_t* prologSize)

{
    // Decompress the hdrInfo    
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (BYTE *)(SIZE_T)relPCOffset, &hdrInfo);
    // Store the prolog size at the provided address  
    *prologSize = hdrInfo.prologSize;
    // Check if the offset is inside the prolog or epilog
    _ASSERTE((SIZE_T)relPCOffset < hdrInfo.methodSize);
    if (((SIZE_T)relPCOffset <= hdrInfo.prologSize) || ((SIZE_T)relPCOffset > (hdrInfo.methodSize-hdrInfo.epilogSize))) {
        return true;
    }
    return false;
}

/*
  Returns the size of a given function.
*/
size_t Fjit_EETwain::GetFunctionSize(
                LPVOID methodInfoPtr)
{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, 0, &hdrInfo);
    return hdrInfo.methodSize;
}

/*
  Returns the size of the frame of the function.
*/
unsigned int Fjit_EETwain::GetFrameSize(LPVOID methodInfoPtr)
{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, 0, &hdrInfo);
    return hdrInfo.methodFrame;
}

/**************************************************************************************/
// returns the address of the most deeply nested finally and the IP is currently in that finally.
const BYTE* Fjit_EETwain::GetFinallyReturnAddr(PREGDISPLAY pReg)
{
    return *(const BYTE**)(size_t)GetRegdisplaySP(pReg);  
}

BOOL Fjit_EETwain::IsInFilter(   void *methodInfoPtr,
                                 unsigned offset,    
                                 PCONTEXT pCtx,
                                 DWORD nestingLevel)
{
    // Must be inside at least one internal EH subframe
    _ASSERTE(nestingLevel > 0);

    // Get a base pointer to the internal frame 
    PVOID* pFrameBase = getInternalFP(GetFP(pCtx));

#ifdef _DEBUG
    if ( isFilterEHClause(pFrameBase, nestingLevel) )  
        LOG((LF_CORDB, LL_INFO10000, "EJM::IsInFilter")); 
#endif 

    return (isFilterEHClause(pFrameBase, nestingLevel));                
}        

// Only called in normal path, not during exception
BOOL Fjit_EETwain::LeaveFinally(   void *methodInfoPtr,
                                   unsigned offset,    
                                   PCONTEXT pCtx,
                                   DWORD nestingLevel    // = current nesting level = most deeply nested finally
                                   )
{
    // Must be inside at least one internal EH subframe
    _ASSERTE(nestingLevel > 0);
    PVOID* pFrameBase = getInternalFP(GetFP(pCtx));
   
    // Decrease the counter of nested internal frames && zero out the base esp slot
    unsigned ctr = getEHClauseNestingDepth( pFrameBase );
    setSavedSPForEHClause( pFrameBase, ctr, 0 );
    setEHClauseNestingDepth(  pFrameBase, --ctr );
   
    // remove the return address from the stack 
    SetSP(pCtx, (LPVOID)(size_t)((size_t)GetSP(pCtx) + sizeof(void*)));

    LOG((LF_CORDB, LL_INFO10000, "EJM::LeaveFinally: fjit sets esp to "
        "0x%x, was 0x%x\n", (DWORD)(size_t)GetSP(pCtx), (DWORD)(size_t)GetSP(pCtx) - sizeof(void*)));
    return TRUE;   
}

void Fjit_EETwain::LeaveCatch(    void *methodInfoPtr,
                                  unsigned offset,    
                                  PCONTEXT pCtx)
{    
    // Get a base pointer for the internal frame 
    PVOID* pFrameBase = getInternalFP(GetFP(pCtx));
   
    // Decrease the counter of nested internal frames && zero out the base esp slot
    unsigned ctr = getEHClauseNestingDepth( pFrameBase );
    setSavedSPForEHClause( pFrameBase, ctr, 0 );
    setEHClauseNestingDepth(  pFrameBase, --ctr );

    // set esp to right value 
    if (ctr) 
      SetSP( pCtx, getSavedSPForEHClause( pFrameBase, ctr ));

    return;
}


