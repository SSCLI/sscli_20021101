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

#ifndef BEGIN_CLASS_DUMP_INFO
#define BEGIN_CLASS_DUMP_INFO(klass)
#endif

#ifndef END_CLASS_DUMP_INFO
#define END_CLASS_DUMP_INFO(klass)
#endif

#ifndef BEGIN_CLASS_DUMP_INFO_DERIVED
#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent)
#endif

#ifndef END_CLASS_DUMP_INFO_DERIVED
#define END_CLASS_DUMP_INFO_DERIVED(klass, parent)
#endif

#ifndef BEGIN_ABSTRACT_CLASS_DUMP_INFO
#define BEGIN_ABSTRACT_CLASS_DUMP_INFO(klass)
#endif

#ifndef END_ABSTRACT_CLASS_DUMP_INFO
#define END_ABSTRACT_CLASS_DUMP_INFO(klass)
#endif

#ifndef BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED
#define BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)
#endif

#ifndef END_ABSTRACT_CLASS_DUMP_INFO_DERIVED
#define END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)
#endif

#ifndef CDI_CLASS_MEMBER_OFFSET
#define CDI_CLASS_MEMBER_OFFSET(member)
#endif


BEGIN_ABSTRACT_CLASS_DUMP_INFO(Frame)
    CDI_CLASS_MEMBER_OFFSET(m_Next)
END_ABSTRACT_CLASS_DUMP_INFO(Frame)


BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(TransitionFrame, Frame)
    CDI_CLASS_MEMBER_OFFSET(m_Datum)
END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(TransitionFrame, Frame)

BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(ExceptionFrame, Frame)
END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(ExceptionFrame, Frame)

BEGIN_CLASS_DUMP_INFO_DERIVED(FaultingExceptionFrame, ExceptionFrame)
#if defined(_X86_)
    CDI_CLASS_MEMBER_OFFSET(m_ReturnAddress)
    CDI_CLASS_MEMBER_OFFSET(m_Esp)
    CDI_CLASS_MEMBER_OFFSET(m_regs)
#elif defined(_PPC_)
    CDI_CLASS_MEMBER_OFFSET(m_regs)
    CDI_CLASS_MEMBER_OFFSET(m_Link)
#else
    PORTABILITY_WARNING("FaultingExceptionFrame is not implemented on this platform.")
    CDI_CLASS_MEMBER_OFFSET(m_ReturnAddress)
    CDI_CLASS_MEMBER_OFFSET(m_regs)
#endif
END_CLASS_DUMP_INFO_DERIVED(FaultingExceptionFrame, ExceptionFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(FuncEvalFrame, TransitionFrame)
    CDI_CLASS_MEMBER_OFFSET(m_ReturnAddress)
END_CLASS_DUMP_INFO_DERIVED(FuncEvalFrame, TransitionFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame, TransitionFrame)
    CDI_CLASS_MEMBER_OFFSET(m_Attribs)
    CDI_CLASS_MEMBER_OFFSET(m_MachState)
    CDI_CLASS_MEMBER_OFFSET(m_RegArgs)
    CDI_CLASS_MEMBER_OFFSET(m_pThread)
    CDI_CLASS_MEMBER_OFFSET(m_FCallEntry)
END_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame, TransitionFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame_1OBJ, HelperMethodFrame)
    CDI_CLASS_MEMBER_OFFSET(gcPtrs)
END_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame_1OBJ, HelperMethodFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame_2OBJ, HelperMethodFrame)
    CDI_CLASS_MEMBER_OFFSET(gcPtrs)
END_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame_2OBJ, HelperMethodFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame_4OBJ, HelperMethodFrame)
    CDI_CLASS_MEMBER_OFFSET(gcPtrs)
END_CLASS_DUMP_INFO_DERIVED(HelperMethodFrame_4OBJ, HelperMethodFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(FramedMethodFrame, TransitionFrame)
#if defined(_PPC_)
    CDI_CLASS_MEMBER_OFFSET(m_Link)
#else
    CDI_CLASS_MEMBER_OFFSET(m_ReturnAddress)
#endif
END_CLASS_DUMP_INFO_DERIVED(FramedMethodFrame, TransitionFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(TPMethodFrame, FramedMethodFrame)
END_CLASS_DUMP_INFO_DERIVED(TPMethodFrame, FramedMethodFrame)


BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrame, FramedMethodFrame)
END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrame, FramedMethodFrame)

BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameEx, NDirectMethodFrame)
END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameEx, NDirectMethodFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameGeneric, NDirectMethodFrameEx)
END_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameGeneric, NDirectMethodFrameEx)

#ifdef _X86_
BEGIN_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameSlim, NDirectMethodFrameEx)
END_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameSlim, NDirectMethodFrameEx)

BEGIN_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameStandalone, NDirectMethodFrame)
END_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameStandalone, NDirectMethodFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameStandaloneCleanup, NDirectMethodFrameEx)
END_CLASS_DUMP_INFO_DERIVED(NDirectMethodFrameStandaloneCleanup, NDirectMethodFrameEx)
#endif

BEGIN_CLASS_DUMP_INFO_DERIVED(MulticastFrame, FramedMethodFrame)
END_CLASS_DUMP_INFO_DERIVED(MulticastFrame, FramedMethodFrame)

BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(UnmanagedToManagedFrame, Frame)
    CDI_CLASS_MEMBER_OFFSET(m_pvDatum)
#ifdef _PPC_
    CDI_CLASS_MEMBER_OFFSET(m_Link)
#else
    CDI_CLASS_MEMBER_OFFSET(m_ReturnAddress)
#endif
END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(UnmanagedToManagedFrame, Frame)

BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(UnmanagedToManagedCallFrame, UnmanagedToManagedFrame)
END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(UnmanagedToManagedCallFrame, UnmanagedToManagedFrame)




BEGIN_CLASS_DUMP_INFO_DERIVED(SecurityFrame, FramedMethodFrame)
END_CLASS_DUMP_INFO_DERIVED(SecurityFrame, FramedMethodFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(PrestubMethodFrame, FramedMethodFrame)
END_CLASS_DUMP_INFO_DERIVED(PrestubMethodFrame, FramedMethodFrame)

BEGIN_CLASS_DUMP_INFO_DERIVED(InterceptorFrame, SecurityFrame)
END_CLASS_DUMP_INFO_DERIVED(InterceptorFrame, SecurityFrame)


BEGIN_CLASS_DUMP_INFO_DERIVED(GCFrame, Frame)
    CDI_CLASS_MEMBER_OFFSET(m_pObjRefs)
    CDI_CLASS_MEMBER_OFFSET(m_numObjRefs)
    CDI_CLASS_MEMBER_OFFSET(m_pCurThread)
    CDI_CLASS_MEMBER_OFFSET(m_MaybeInterior)
END_CLASS_DUMP_INFO_DERIVED(GCFrame, Frame)

BEGIN_CLASS_DUMP_INFO_DERIVED(ProtectByRefsFrame, Frame)
    CDI_CLASS_MEMBER_OFFSET(m_brInfo)
    CDI_CLASS_MEMBER_OFFSET(m_pThread)
END_CLASS_DUMP_INFO_DERIVED(ProtectByRefsFrame, Frame)

BEGIN_CLASS_DUMP_INFO_DERIVED(ProtectValueClassFrame, Frame)
    CDI_CLASS_MEMBER_OFFSET(m_pVCInfo)
    CDI_CLASS_MEMBER_OFFSET(m_pThread)
END_CLASS_DUMP_INFO_DERIVED(ProtectValueClassFrame, Frame)

BEGIN_CLASS_DUMP_INFO_DERIVED(DebuggerClassInitMarkFrame, Frame)
END_CLASS_DUMP_INFO_DERIVED(DebuggerClassInitMarkFrame, Frame)

BEGIN_CLASS_DUMP_INFO_DERIVED(DebuggerSecurityCodeMarkFrame, Frame)
END_CLASS_DUMP_INFO_DERIVED(DebuggerSecurityCodeMarkFrame, Frame)

BEGIN_CLASS_DUMP_INFO_DERIVED(DebuggerExitFrame, Frame)
END_CLASS_DUMP_INFO_DERIVED(DebuggerExitFrame, Frame)

BEGIN_CLASS_DUMP_INFO_DERIVED(UMThkCallFrame, UnmanagedToManagedCallFrame)
END_CLASS_DUMP_INFO_DERIVED(UMThkCallFrame, UnmanagedToManagedCallFrame)


BEGIN_CLASS_DUMP_INFO_DERIVED(ContextTransitionFrame, Frame)
END_CLASS_DUMP_INFO_DERIVED(ContextTransitionFrame, Frame)


