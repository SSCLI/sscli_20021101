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
/*  STACKWALK.CPP:
 *
 */

#include "common.h"
#include "frames.h"
#include "threads.h"
#include "stackwalk.h"
#include "excep.h"
#include "eetwain.h"
#include "codeman.h"
#include "eeconfig.h"
#include "stackprobe.h"

#ifdef _DEBUG
void* forceFrame;   // Variable used to force a local variable to the frame
#endif

MethodDesc::RETURNTYPE CrawlFrame::ReturnsObject() 
{
    if (isFrameless)
        return pFunc->ReturnsObject();
    return pFrame->ReturnsObject();
}

OBJECTREF* CrawlFrame::GetAddrOfSecurityObject()
{
    if (isFrameless)
    {
        OBJECTREF * pObj = NULL;

        _ASSERTE(pFunc);

        pObj = (static_cast <OBJECTREF*>
                    (codeMgrInstance->GetAddrOfSecurityObject(pRD,
                                                              JitManagerInstance->GetGCInfo(methodToken),
                                                              relOffset,
                                                              &codeManState)));

        return (pObj);
    }
    else
    {
        /*ISSUE: Are there any other functions holding a security desc? */
        if (pFunc && (pFunc->IsIL() || pFunc->IsNDirect()) && pFrame->IsFramedMethodFrame())
                return static_cast<FramedMethodFrame*>
                    (pFrame)->GetAddrOfSecurityDesc();
    }
    return NULL;
}

LPVOID CrawlFrame::GetInfoBlock()
{
    _ASSERTE(isFrameless);
    _ASSERTE(JitManagerInstance && methodToken);
    return JitManagerInstance->GetGCInfo(methodToken);
}

unsigned CrawlFrame::GetOffsetInFunction()
{
    _ASSERTE(!"NYI");
    return 0;
}


OBJECTREF CrawlFrame::GetObject()
{

    if (!pFunc || pFunc->IsStatic() || pFunc->GetClass()->IsValueClass())
        return NULL;

    if (isFrameless)
    {
        return codeMgrInstance->GetInstance(pRD,
                                            JitManagerInstance->GetGCInfo(methodToken),
                                            relOffset);
    }
    else
    {
        _ASSERTE(pFrame);
        _ASSERTE(pFunc);
        /*ISSUE: we already know that we have (at least) a method */
        /*       might need adjustment as soon as we solved the
                 jit-helper frame question
        */

        return ((FramedMethodFrame*)pFrame)->GetThis();
    }
}



    /* Is this frame at a safe spot for GC?
     */
bool CrawlFrame::IsGcSafe()
{
    _ASSERTE(codeMgrInstance);
    EECodeInfo codeInfo(methodToken, JitManagerInstance);
    return codeMgrInstance->IsGcSafe(pRD,
                                     JitManagerInstance->GetGCInfo(methodToken),
                                     &codeInfo,
                                     0);
}

inline void CrawlFrame::GotoNextFrame()
{
    //
    // Update app domain if this frame caused a transition
    //

    AppDomain *pRetDomain = pFrame->GetReturnDomain();
    if (pRetDomain != NULL)
        pAppDomain = pRetDomain;
    pFrame = pFrame->Next();
}



StackWalkAction Thread::StackWalkFramesEx(
                    PREGDISPLAY pRD,        // virtual register set at crawl start
                    PSTACKWALKFRAMESCALLBACK pCallback,
                    VOID *pData,
                    unsigned flags,
                    Frame *pStartFrame
                )
{
    BEGIN_FORBID_TYPELOAD();
    CrawlFrame cf;
    StackWalkAction retVal = SWA_FAILED;
    Frame * pInlinedFrame = NULL;


    if (pStartFrame)
        cf.pFrame = pStartFrame;
    else
        cf.pFrame = this->GetFrame();


    // FRAME_TOP and NULL must be distinct values. This assert
    // will fire if someone changes this.
    _ASSERTE(FRAME_TOP != NULL);

#ifdef _DEBUG
    Frame* startFrame = cf.pFrame;
    int depth = 0;
    forceFrame = &depth;
    cf.pFunc = (MethodDesc*)POISONC;
#endif
    cf.isFirst = true;
    cf.isInterrupted = false;
    cf.hasFaulted = false;
    cf.isIPadjusted = false;
    unsigned unwindFlags = (flags & QUICKUNWIND) ? 0 : UpdateAllRegs;
    ASSERT(pRD);

    IJitManager* pEEJM = ExecutionManager::FindJitMan((PBYTE)GetControlPC(pRD));
    cf.JitManagerInstance = pEEJM;
    cf.codeMgrInstance = NULL;
    if ((cf.isFrameless = (pEEJM != NULL)) == true)
        cf.codeMgrInstance = pEEJM->GetCodeManager();
    cf.pRD = pRD;
    cf.pAppDomain = GetDomain();

    // can debugger handle skipped frames?
    BOOL fHandleSkippedFrames = !(flags & HANDLESKIPPEDFRAMES);


    IJitManager::ScanFlag fJitManagerScanFlags = IJitManager::GetScanFlags();

    while (cf.isFrameless || (cf.pFrame != FRAME_TOP))
    {
        retVal = SWA_DONE;

        cf.codeManState.dwIsSet = 0;
#ifdef _DEBUG
        memset((void *)cf.codeManState.stateBuf, 0xCD,
                sizeof(cf.codeManState.stateBuf));
        depth++;
#endif

        if (cf.isFrameless)
        {
            // This must be a JITed/managed native method


            pEEJM->JitCode2MethodTokenAndOffset((PBYTE)GetControlPC(pRD),&(cf.methodToken),(DWORD*)&(cf.relOffset), fJitManagerScanFlags);
            cf.pFunc = pEEJM->JitTokenToMethodDesc(cf.methodToken, fJitManagerScanFlags);
            EECodeInfo codeInfo(cf.methodToken, pEEJM, cf.pFunc);
            //cf.methodInfo = pEEJM->GetGCInfo(&codeInfo);
            // If this is a thunk it may need to be protected against beeing freed during code-pitching
            pEEJM->ProtectThunk( *(cf.pRD->pPC), true );

            END_FORBID_TYPELOAD();
            if (SWA_ABORT == pCallback(&cf, (VOID*)pData)) 
            {
                // Try to avoid leaking a thunk, but if the callback doesn't return may leak anyways 
                pEEJM->ProtectThunk( *(cf.pRD->pPC), false );
                return SWA_ABORT;
            }
            BEGIN_FORBID_TYPELOAD();
             

            // It is fine to release a thunk that has not been protected
            pEEJM->ProtectThunk( *(cf.pRD->pPC), false );

            /* Now find out if we need to leave monitors */
            LPVOID methodInfo = pEEJM->GetGCInfo(cf.methodToken);

            if (flags & POPFRAMES)
            {
                if (cf.pFunc->IsSynchronized())
                {
                    MethodDesc    *pMD = cf.pFunc;
                    OBJECTREF      orUnwind = 0;

                    if (pMD->IsStatic())
                    {
                        EEClass    *pClass = pMD->GetClass();
                        orUnwind = pClass->GetExposedClassObject();
                    }
                    else
                    {
                        orUnwind = cf.codeMgrInstance->GetInstance(
                                                pRD,
                                                methodInfo,
                                                cf.relOffset);
                    }

                    _ASSERTE(orUnwind);
                    _ASSERTE(!orUnwind->IsThunking());
                    if (orUnwind != NULL)
                        orUnwind->LeaveObjMonitorAtException();
                }
            }
#ifdef _X86_
            // FaultingExceptionFrame is special case where it gets
            // pushed on the stack after the frame is running
            _ASSERTE((cf.pFrame == FRAME_TOP) ||
                     ((size_t)GetRegdisplaySP(cf.pRD) < (size_t)cf.pFrame) ||
                           (cf.pFrame->GetVTablePtr() == FaultingExceptionFrame::GetMethodFrameVPtr()) ||
                           (cf.pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr()));
#endif
            /* Get rid of the frame (actually, it isn't really popped) */

            LOG((LF_CORDB, LL_INFO1000000, "Thread::StackWalkFramesEx: calling UnwindStackFrame\n"));
            cf.codeMgrInstance->UnwindStackFrame(
                                    pRD,
                                    methodInfo,
                                    &codeInfo,
                                    unwindFlags | cf.GetCodeManagerFlags(),
                                    &cf.codeManState);

            LOG((LF_CORDB, LL_INFO1000000, "Thread::StackWalkFramesEx:  UnwindStackFrame returned\n"));
            cf.isFirst = FALSE;
            cf.isInterrupted = cf.hasFaulted = cf.isIPadjusted = FALSE;

            /* We might have skipped past some Frames */
            /* This happens with InlinedCallFrames and if we unwound
             * out of a finally in managed code or for ContextTransitionFrames that are
             * inserted into the managed call stack */
            while (cf.pFrame != FRAME_TOP && (size_t)cf.pFrame < (size_t)GetRegdisplaySP(cf.pRD))
            {
                if (!fHandleSkippedFrames 
                    )
                {
                    cf.GotoNextFrame();
                    if (flags & POPFRAMES)
                        this->SetFrame(cf.pFrame);
                }
                else
                {
                    cf.codeMgrInstance = NULL;
                    cf.isFrameless     = false;

                    cf.pFunc = cf.pFrame->GetFunction();

                    // process that frame
                    if (cf.pFunc || !(flags&FUNCTIONSONLY))
                    {
                        END_FORBID_TYPELOAD();
                        if (SWA_ABORT == pCallback(&cf, (VOID*)pData)) 
                            return SWA_ABORT;
                        BEGIN_FORBID_TYPELOAD();
                    }

                    if (flags & POPFRAMES)
                    {
#ifdef _DEBUG
                        if (cf.pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
                            // if it's a context transition frame that was pushed on in managed code, a managed
                            // finally may have already popped it off, so check that either have current
                            // frame or the next one down
                            _ASSERTE(cf.pFrame == GetFrame() || cf.pFrame->Next() == GetFrame());
                        else
                            _ASSERTE(cf.pFrame == GetFrame());
#endif

                        // If we got here, the current frame chose not to handle the
                        // exception. Give it a chance to do any termination work
                        // before we pop it off.
                        END_FORBID_TYPELOAD();
                        cf.pFrame->ExceptionUnwind();
                        BEGIN_FORBID_TYPELOAD();

                        // Pop off this frame and go on to the next one.
                        cf.GotoNextFrame();

                        this->SetFrame(cf.pFrame);
                    }
                    else
                    {
                        /* go to the next frame */
                        cf.GotoNextFrame();
                    }
                }
            }
            /* Now inspect caller (i.e. is it again in "native" code ?) */
            pEEJM = ExecutionManager::FindJitMan((PBYTE)GetControlPC(pRD), fJitManagerScanFlags);
            cf.JitManagerInstance = pEEJM;

            cf.codeMgrInstance = NULL;
            if ((cf.isFrameless = (pEEJM != NULL)) == true)
            {
                cf.codeMgrInstance = pEEJM->GetCodeManager(); // CHANGE, VC6.0
            }

        }
        else
        {
            pInlinedFrame = NULL;


            cf.pFunc  = cf.pFrame->GetFunction();
#ifdef _DEBUG
            cf.codeMgrInstance = NULL;
#endif

            /* Are we supposed to filter non-function frames? */

            if (cf.pFunc || !(flags&FUNCTIONSONLY))
            {
                END_FORBID_TYPELOAD();
                if (SWA_ABORT == pCallback(&cf, (VOID *)pData)) 
                    return SWA_ABORT;
                BEGIN_FORBID_TYPELOAD();
            }

            // Special resumable frames make believe they are on top of the stack
            cf.isFirst = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_RESUMABLE) != 0;

            // If the frame is a subclass of ExceptionFrame,
            // then we know this is interrupted

            cf.isInterrupted = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_EXCEPTION) != 0;

            if (cf.isInterrupted)
            {
                cf.hasFaulted   = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_FAULTED) != 0;
                cf.isIPadjusted = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_OUT_OF_LINE) != 0;
                _ASSERTE(!cf.hasFaulted || !cf.isIPadjusted); // both cant be set together
            }

            //
            // Update app domain if this frame caused a transition
            //

            AppDomain *pAppDomain = cf.pFrame->GetReturnDomain();
            if (pAppDomain != NULL)
                cf.pAppDomain = pAppDomain;

            SLOT adr = (SLOT)cf.pFrame->GetReturnAddress();
            _ASSERTE(adr != (LPVOID)POISONC);

            _ASSERTE(!pInlinedFrame || adr);

            if (adr)
            {
                /* is caller in managed code ? */
                pEEJM = ExecutionManager::FindJitMan(adr, fJitManagerScanFlags);
                cf.JitManagerInstance = pEEJM;

                _ASSERTE(pEEJM || !pInlinedFrame);

                cf.codeMgrInstance = NULL;

                if ((cf.isFrameless = (pEEJM != NULL)) == true)
                {
                    cf.pFrame->UpdateRegDisplay(pRD);
                    cf.codeMgrInstance = pEEJM->GetCodeManager(); // CHANGE, VC6.0
                }
            }

            if (!pInlinedFrame)
            {
                if (flags & POPFRAMES)
                {
                    // If we got here, the current frame chose not to handle the
                    // exception. Give it a chance to do any termination work
                    // before we pop it off.
                    cf.pFrame->ExceptionUnwind();

                    // Pop off this frame and go on to the next one.
                    cf.GotoNextFrame();

                    this->SetFrame(cf.pFrame);
                }
                else
                {
                    /* go to the next frame */
                    cf.pFrame = cf.pFrame->Next();
                }
            }
        }
    }

        // Try to insure that the frame chain did not change underneath us.
        // In particular, is thread's starting frame the same as it was when we started?
    _ASSERTE(startFrame  != 0 || startFrame == this->GetFrame());

    /* If we got here, we either couldn't even start (for whatever reason)
       or we came to the end of the stack. In the latter case we return SWA_DONE.
    */
    END_FORBID_TYPELOAD();
    LOG((LF_CORDB, LL_INFO1000000, "StackWalkFramesEx: returning 0x%x\n", retVal));

    return retVal;
}

StackWalkAction Thread::StackWalkFrames(PSTACKWALKFRAMESCALLBACK pCallback,
                               VOID *pData,
                               unsigned flags,
                               Frame *pStartFrame)
{
    SAFE_REQUIRES_N4K_STACK(3); // shared between exceptions & normal codepath
    
    /*                                                              
           */

    REGDISPLAY rd;
    BOOL fInitialized;
    fInitialized = FALSE;

#ifdef _X86_

    CONTEXT ctx;
    {
        ctx.ContextFlags = 0;
        ctx.Eip = 0;
        rd.pPC = (SLOT*)&(ctx.Eip);
        fInitialized = TRUE;
    }

#elif defined(_PPC_) || defined(_SPARC_)

    CONTEXT ctx;

    if (this == GetThread())
    {
        ctx.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(GetCurrentThread(), &ctx)) {
            _ASSERTE(!"GetThreadContext failed!");
            return SWA_FAILED;
        }

        FillRegDisplay(&rd, &ctx);
        fInitialized = TRUE;
    }
    else
    {
        ctx.ContextFlags = 0;
#if defined(_PPC_)
        ctx.Iar = 0;
        rd.pPC = (SLOT*)&(ctx.Iar);
#else
        ctx.pc = 0;
        rd.pPC = (SLOT*)&(ctx.pc);
#endif
        fInitialized = TRUE;
    }

#else
    PORTABILITY_ASSERT("StackWalk not yet implemented");
#endif

    if (!fInitialized)
    {
        memset(&rd, 0, sizeof(rd));
    }

    return StackWalkFramesEx(&rd, pCallback, pData, flags, pStartFrame);
}

StackWalkAction StackWalkFunctions(Thread * thread,
                                   PSTACKWALKFRAMESCALLBACK pCallback,
                                   VOID * pData)
{
    return thread->StackWalkFrames(pCallback, pData, FUNCTIONSONLY);
}


