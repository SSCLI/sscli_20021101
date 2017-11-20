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
// FRAMES.H -
//
// These C++ classes expose activation frames to the rest of the EE.
// Activation frames are actually created by JIT-generated or stub-generated
// code on the machine stack. Thus, the layout of the Frame classes and
// the JIT/Stub code generators are tightly interwined.
//
// IMPORTANT: Since frames are not actually constructed by C++,
// don't try to define constructor/destructor functions. They won't get
// called.
//
// IMPORTANT: Not all methods have full-fledged activation frames (in
// particular, the JIT may create frameless methods.) This is one reason
// why Frame doesn't expose a public "Next()" method: such a method would
// skip frameless method calls. You must instead use one of the
// StackWalk methods.
//
//
// The following is the hierarchy of frames:
//
//    Frame                   - the root class. There are no actual instances
//    |                         of Frames.
//    |
//    +-GCFrame               - this frame doesn't represent a method call.
//    |                         it's sole purpose is to let the EE gc-protect
//    |                         object references that it is manipulating.
//    |
//    +-InlinedCallFrame      - if a call to unmanaged code is hoisted into
//    |                         a JIT'ted caller, the calling method keeps
//    |                         this frame linked throughout its activation.
//    |
//    +-ResumableFrame        - this frame provides the context necessary to
//    | |                       allow garbage collection during handling of
//    | |                       a resumable exception (e.g. during edit-and-continue,
//    | |                       or under GCStress4).
//    | |
//    | +-RedirectedThreadFrame - this frame is used for redirecting threads during suspension
//    |
//    +-TransitionFrame       - this frame represents a transition from
//    | |                       one or more nested frameless method calls
//    | |                       to either a EE runtime helper function or
//    | |                       a framed method.
//    | |
//    | +-ExceptionFrame        - this frame has caused an exception
//    | | |
//    | | |
//    | | +- FaultingExceptionFrame - this frame was placed on a method which faulted
//    | |                              to save additional state information
//    | |
//    | +-FuncEvalFrame         - frame for debugger function evaluation
//    | |
//    | |
//    | +-HelperMethodFrame     - frame used allow stack crawling inside jit helpers and fcalls
//    | |
//    | |
//    | +-FramedMethodFrame   - this frame represents a call to a method
//    |   |                     that generates a full-fledged frame.
//    |   |
//    |   +-NDirectMethodFrame   - represents an N/Direct call.
//    |   | |
//    |   | +-NDirectMethodFrameEx - represents an N/Direct call w/ cleanup
//    |   |
//    |   +-PrestubMethodFrame   - represents a call to a prestub
//    |   |
//    |   +-CtxCrossingFrame     - this frame marks a call across a context
//    |   |                        boundary
//    |   |
//    |   +-MulticastFrame       - this frame protects arguments to a MulticastDelegate
//    |   |                         Invoke() call while calling each subscriber.
//    |   |
//    |   +-PInovkeCalliFrame   - represents a calli to unmanaged target
//    |  
//    |  
//    +-UnmanagedToManagedFrame - this frame represents a transition from
//    | |                         unmanaged code back to managed code. It's
//    | |                         main functions are to stop COM+ exception
//    | |                         propagation and to expose unmanaged parameters.
//    | |
//    | +-UnmanagedToManagedCallFrame - this frame is used when the target
//    |   |                             is a COM+ function or method call. it
//    |   |                             adds the capability to gc-promote callee
//    |   |                             arguments during marshaling.
//    |   |
//    |   +-UMThunkCallFrame    - this frame represents an unmanaged->managed
//    |                           transition through N/Direct
//    |
//    +-CtxMarshaledFrame  - this frame represent a cross context marshalled call
//    |                      (cross thread,inter-process, cross m/c scenarios)
//    |
//    +-CtxByValueFrame    - this frame is used to support protection of a by-
//    |                      value marshaling session, even though the thread is
//    |                      not pushing a call across a context boundary.
//    |
//    +-ContextTransitionFrame - this frame is used to mark an appdomain transition
//    |
//    +-NativeClientSecurityFrame -  this frame is used to capture the security 
//       |                           context in a Native -> Managed call. Code 
//       |                           Acess security stack walk use caller 
//       |                           information in  this frame to apply 
//       |                           security policy to the  native client.
//       |
//       +-ComClientSecurityFrame -  Security frame for Com clients. 
//                                   VBScript, JScript, IE ..

#ifndef _tst_frames_h
#define _tst_frames_h

#include "tst-helperframe.h"

// Forward references
class Frame;
class FieldMarshaler;
class FramedMethodFrame;
struct HijackArgs;
class UMEntryThunk;
class UMThunkMarshInfo;
class Marshaler;
class SecurityDescriptor;


typedef INT32 StackElemType;
#define STACK_ELEM_SIZE sizeof(StackElemType)

// This will take a pointer to an out-of-process Frame, resolve it to
// the proper type and create and fill that type and return a pointer
// to it.  This must be 'delete'd.  prFrame will be advanced by the
// size of the frame object.
Frame *ResolveFrame(DWORD_PTR prFrame);

//--------------------------------------------------------------------
// This represents some of the TransitionFrame fields that are
// stored at negative offsets.
//--------------------------------------------------------------------
#if defined(_X86_)
struct CalleeSavedRegisters {
    INT32       edi;
    INT32       esi;
    INT32       ebx;
    INT32       ebp;
};
#elif defined(_PPC_)
struct CalleeSavedRegisters {
    INT32   cr;                                 // cr
    INT32   r[NUM_CALLEESAVED_REGISTERS];       // r13 .. r31
    DOUBLE  f[NUM_FLOAT_CALLEESAVED_REGISTERS]; // fpr14 .. fpr31
};
#else
PORTABILITY_WARNING("struct CalleeSavedRegisters is not implemented on this platform.");
struct CalleeSavedRegisters {
};
#endif

#ifdef _PPC_
//--------------------------------------------------------------------
// PPC linkage area
//--------------------------------------------------------------------
struct LinkageArea {
    INT32 SavedSP; // stack pointer
    INT32 SavedCR; // flags
    INT32 SavedLR; // return address
    INT32 Reserved1;
    INT32 Reserved2;
    INT32 Reserved3;
};
#endif  // _PPC_

//--------------------------------------------------------------------
// This represents the arguments that are stored in volatile registers.
// This should not overlap the CalleeSavedRegisters since those are already
// saved separately and it would be wasteful to save the same register twice.
// If we do use a non-volatile register as an argument, then the ArgIterator
// will probably have to communicate this back to the PromoteCallerStack
// routine to avoid a double promotion.
//
//--------------------------------------------------------------------
struct ArgumentRegisters {

#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname)  INT32 regname;
#include "../../vm/eecallconv.h"

};

// Note: the value (-1) is used to generate the largest
// possible pointer value: this keeps frame addresses
// increasing upward.
#define FRAME_TOP ((Frame*)(-1))

#define RETURNFRAMEVPTR(classname) \
    classname boilerplate;      \
    return *((LPVOID*)&boilerplate)

#define DEFINE_STD_FRAME_FUNCS(klass)                                   \
    virtual PWSTR GetFrameTypeName() { return L###klass; }

//------------------------------------------------------------------------
// Frame defines methods common to all frame types. There are no actual
// instances of root frames.
//------------------------------------------------------------------------


class Frame
{
public:
    DEFINE_STD_FILL_FUNCS(Frame)
    DEFINE_STD_FRAME_FUNCS(Frame)

    Frame *m_pNukeNext;

    //------------------------------------------------------------------------
    // Special characteristics of a frame
    //------------------------------------------------------------------------
    enum FrameAttribs
    {
        FRAME_ATTR_NONE = 0,
        FRAME_ATTR_EXCEPTION = 1,           // This frame caused an exception
        FRAME_ATTR_OUT_OF_LINE = 2,         // The exception out of line (IP of the frame is not correct)
        FRAME_ATTR_FAULTED = 4,             // Exception caused by Win32 fault
        FRAME_ATTR_RESUMABLE = 8,           // We may resume from this frame
        FRAME_ATTR_RETURNOBJ = 0x10,        // Frame returns an object (helperFrame only)
        FRAME_ATTR_RETURN_INTERIOR = 0x20,  // Frame returns an interior GC poitner (helperFrame only)
        FRAME_ATTR_CAPUTURE_DEPTH_2 = 0x40, // This is a helperMethodFrame and the capture occured at depth 2
        FRAME_ATTR_EXACT_DEPTH = 0x80,      // This is a helperMethodFrame and a jit helper, but only crawl to the given depth
    };

    virtual MethodDesc *GetFunction()
    {
        return (NULL);
    }

    virtual unsigned GetFrameAttribs()
    {
        return (FRAME_ATTR_NONE);
    }

    LPVOID GetReturnAddress()
    {
        LPVOID* ptr = GetReturnAddressPtr();
        return ptr ? *ptr : NULL;
    }

    virtual LPVOID* GetReturnAddressPtr()
    {
        return (NULL);
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY)
    {
        return;
    }

    //------------------------------------------------------------------------
    // Debugger support
    //------------------------------------------------------------------------

    enum
    {
        TYPE_INTERNAL,
        TYPE_ENTRY,
        TYPE_EXIT,
        TYPE_CONTEXT_CROSS,
        TYPE_INTERCEPTION,
        TYPE_SECURITY,
        TYPE_CALL,
        TYPE_FUNC_EVAL,
        TYPE_TP_METHOD_FRAME,
        TYPE_MULTICAST,

        TYPE_COUNT
    };

    virtual int GetFrameType()
    {
        return (TYPE_INTERNAL);
    };

    // When stepping into a method, various other methods may be called.
    // These are refererred to as interceptors. They are all invoked
    // with frames of various types. GetInterception() indicates whether
    // the frame was set up for execution of such interceptors

    enum Interception
    {
        INTERCEPTION_NONE,
        INTERCEPTION_CLASS_INIT,
        INTERCEPTION_EXCEPTION,
        INTERCEPTION_CONTEXT,
        INTERCEPTION_SECURITY,
        INTERCEPTION_OTHER,

        INTERCEPTION_COUNT
    };

    virtual Interception GetInterception()
    {
        return (INTERCEPTION_NONE);
    }

    // get your VTablePointer (can be used to check what type the frame is)
    LPVOID GetVTablePtr()
    {
        return(*((LPVOID*) this));
    }

    // Returns true only for frames derived from FramedMethodFrame.
    virtual BOOL IsFramedMethodFrame()
    {
        return (FALSE);
    }

    Frame     *m_This;        // This is remote pointer value of this frame
    Frame     *m_Next;        // This is the remote pointer value of the next frame

    // Because JIT-method activations cannot be expressed as Frames,
    // everyone must use the StackCrawler to walk the frame chain
    // reliably. We'll expose the Next method only to the StackCrawler
    // to prevent mistakes.
    Frame   *Next();

    // Frame is considered an abstract class: this protected constructor
    // causes any attempt to instantiate one to fail at compile-time.
    Frame()
    {
    }
};



//------------------------------------------------------------------------
// This frame represents a transition from one or more nested frameless
// method calls to either a EE runtime helper function or a framed method.
// Because most stackwalks from the EE start with a full-fledged frame,
// anything but the most trivial call into the EE has to push this
// frame in order to prevent the frameless methods inbetween from
// getting lost.
//------------------------------------------------------------------------
class TransitionFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(TransitionFrame)
    DEFINE_STD_FRAME_FUNCS(TransitionFrame)

    virtual void UpdateRegDisplay(const PREGDISPLAY) = 0;

    LPVOID  m_Datum;          // offset +8: contents depend on subclass type.
};


//-----------------------------------------------------------------------
// TransitionFrames for exceptions
//-----------------------------------------------------------------------

class ExceptionFrame : public TransitionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(ExceptionFrame)
    DEFINE_STD_FRAME_FUNCS(ExceptionFrame)

    unsigned GetFrameAttribs()
    {
        return (FRAME_ATTR_EXCEPTION);
    }
};

class FaultingExceptionFrame : public ExceptionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(FaultingExceptionFrame)
    DEFINE_STD_FRAME_FUNCS(FaultingExceptionFrame)

#if defined(_X86_)
    LPVOID  m_ReturnAddress;  // return address into JIT'ted code
    DWORD m_Esp;
    CalleeSavedRegisters m_regs;
#elif defined(_PPC_)
    INT                  m_pad;
    CalleeSavedRegisters m_regs;
    LinkageArea          m_Link;
#else
    PORTABILITY_WARNING("FaultingExceptionFrame is not implemented on this platform.")
    LPVOID              m_ReturnAddress;
    CalleeSavedRegisters m_regs;
#endif

    CalleeSavedRegisters *GetCalleeSavedRegisters()
    {
        return (&m_regs);
    }

    unsigned GetFrameAttribs()
    {
        return (FRAME_ATTR_EXCEPTION | FRAME_ATTR_FAULTED);
    }

    // Retrieves the return address into the code that called the
    // helper or method.
    virtual LPVOID* GetReturnAddressPtr()
    {
    #if defined(_PPC_)
        return (LPVOID*)&(m_Link.SavedLR);
    #else
        return (&m_ReturnAddress);
    #endif
    }


    virtual void UpdateRegDisplay(const PREGDISPLAY);
};



//-----------------------------------------------------------------------
// TransitionFrame for debugger function evaluation
//
// m_Datum holds a ptr to a DebuggerEval object which contains a copy
// of the thread's context at the time it was hijacked for the func
// eval.
//
// UpdateRegDisplay updates all registers inthe REGDISPLAY, not just
// the callee saved registers, because we can hijack for a func eval
// at any point in a thread's execution.
//
// No callee saved registers are held in the negative space for this
// type of frame.
//
//-----------------------------------------------------------------------

class FuncEvalFrame : public TransitionFrame
{
    LPVOID  m_ReturnAddress;

public:
    DEFINE_STD_FILL_FUNCS(FuncEvalFrame)
    DEFINE_STD_FRAME_FUNCS(FuncEvalFrame)

    virtual BOOL IsTransitionToNativeFrame()
    {
        return (FALSE); 
    }

    virtual int GetFrameType()
    {
        return (TYPE_FUNC_EVAL);
    };

    virtual void *GetDebuggerEval()
    {
        return (void*)m_Datum;
    }
    
    virtual unsigned GetFrameAttribs();

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    virtual LPVOID* GetReturnAddressPtr(); 
};


//------------------------------------------------------------------------
// A HelperMethodFrame is created by jit helper (Modified slightly it could be used
// for native routines).   This frame just does the callee saved register
// fixup, it does NOT protect arguments (you can use GCPROTECT or the HelperMethodFrame subclases)
// see JitInterface for sample use, YOU CAN'T RETURN STATEMENT WHILE IN THE PROTECTED STATE!
//------------------------------------------------------------------------

class HelperMethodFrame : public TransitionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame)

    virtual LPVOID* GetReturnAddressPtr()
    {
        return m_MachState->_pRetAddr;
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);
    virtual unsigned GetFrameAttribs()
    {
        return(m_Attribs);
    }
    void InsureInit();
    void Init(Thread *pThread, struct MachState* ms, MethodDesc* pMD, ArgumentRegisters * regArgs);

    unsigned m_Attribs;
    MachState* m_MachState;         // pRetAddr points to the return address and the stack arguments
    ArgumentRegisters * m_RegArgs;  // if non-zero we also report these as the register arguments 
    Thread *m_pThread;
    void* m_FCallEntry;             // used to determine our identity for stack traces
};

/***********************************************************************************/
/* a HelplerMethodFrames that also report additional object references */

class HelperMethodFrame_1OBJ : public HelperMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame_1OBJ)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame_1OBJ)

    /*OBJECTREF*/ DWORD_PTR gcPtrs[1];
};

class HelperMethodFrame_2OBJ : public HelperMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame_2OBJ)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame_2OBJ)

    /*OBJECTREF*/ DWORD_PTR gcPtrs[2];
};

class HelperMethodFrame_4OBJ : public HelperMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame_4OBJ)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame_4OBJ)

    /*OBJECTREF*/ DWORD_PTR gcPtrs[4];
};

//------------------------------------------------------------------------
// This frame represents a method call. No actual instances of this
// frame exist: there are subclasses for each method type.
//
// However, they all share a similar image ...
//
//              +...    stack-based arguments here
//              +12     return address
//              +8      datum (typically a MethodDesc*)
//              +4      m_Next
//              +0      the frame vptr
//              -...    preserved CalleeSavedRegisters
//              -...    VC5Frame (debug only)
//              -...    ArgumentRegisters
//
//------------------------------------------------------------------------
class FramedMethodFrame : public TransitionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(FramedMethodFrame)
    DEFINE_STD_FRAME_FUNCS(FramedMethodFrame)

    virtual MethodDesc *GetFunction()
    {
        return(MethodDesc*)m_Datum;
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    int GetFrameType()
    {
        return (TYPE_CALL);
    }

    CalleeSavedRegisters *GetCalleeSavedRegisters();

    virtual BOOL IsFramedMethodFrame()
    {
        return (TRUE);
    }

    // Retrieves the return address into the code that called the
    // helper or method.
    virtual LPVOID* GetReturnAddressPtr();


#if defined(_PPC_)
    LinkageArea     m_Link;
#else
    LPVOID          m_ReturnAddress;    // Return address into JIT'ted code
#endif
    CalleeSavedRegisters m_savedRegs;
};


//----------------------------------------------------------------------
// The layout of the stub stackframe
//----------------------------------------------------------------------
#if defined(_PPC_)
struct StubStackFrame {
    LinkageArea             link;
    INT32                   params[4]; // max 4 local variables or parameters passed down
    ArgumentRegisters       argregs;
    CalleeSavedRegisters    savedregs;
    FramedMethodFrame       methodframe; // LinkageArea is allocated by caller!
};
#else
// define one if it makes sense for the platform 
#endif

//+----------------------------------------------------------------------------
//
//  Class:      TPMethodFrame            private
//
//  Synopsis:   This frame is pushed onto the stack for calls on transparent
//              proxy
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
class TPMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(TPMethodFrame)
    DEFINE_STD_FRAME_FUNCS(TPMethodFrame)

    virtual int GetFrameType()
    {
        return (TYPE_TP_METHOD_FRAME);
    }

    // Get offset used during stub generation
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(TPMethodFrame);
    }
};



//------------------------------------------------------------------------
// This represents a call to a NDirect method.
//------------------------------------------------------------------------
class NDirectMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrame)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------

    int GetFrameType()
    {
        return (TYPE_EXIT);
    };
};



//------------------------------------------------------------------------
// This represents a call to a NDirect method with cleanup.
//------------------------------------------------------------------------
class NDirectMethodFrameEx : public NDirectMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameEx)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameEx)
};


//------------------------------------------------------------------------
// This represents a call to a NDirect method with the generic worker
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameGeneric : public NDirectMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameGeneric)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameGeneric)
};

#ifdef _X86_
//------------------------------------------------------------------------
// This represents a call to a NDirect method with the slimstub
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameSlim : public NDirectMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameSlim)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameSlim)
};


//------------------------------------------------------------------------
// This represents a call to a NDirect method with the standalone stub (no cleanup)
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameStandalone : public NDirectMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameStandalone)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameStandalone)
};


//------------------------------------------------------------------------
// This represents a call to a NDirect method with the standalone stub (with cleanup)
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameStandaloneCleanup : public NDirectMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameStandaloneCleanup)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameStandaloneCleanup)
};
#endif // _X86_


//------------------------------------------------------------------------
// This represents a call Multicast.Invoke. It's only used to gc-protect
// the arguments during the iteration.
//------------------------------------------------------------------------
class MulticastFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(MulticastFrame)
    DEFINE_STD_FRAME_FUNCS(MulticastFrame)

    int GetFrameType()
    {
        return (TYPE_MULTICAST);
    }
};


//-----------------------------------------------------------------------
// Transition frame from unmanaged to managed
//-----------------------------------------------------------------------
class UnmanagedToManagedFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(UnmanagedToManagedFrame)
    DEFINE_STD_FRAME_FUNCS(UnmanagedToManagedFrame)

    virtual LPVOID* GetReturnAddressPtr()
    {
#ifdef _PPC_
        return (LPVOID*)&(m_Link.SavedLR);
#else
        return (&m_ReturnAddress);
#endif
    }

    // depends on the sub frames to return approp. type here
    virtual LPVOID GetDatum()
    {
        return (m_pvDatum);
    }

    int GetFrameType()
    {
        return (TYPE_ENTRY);
    };

    virtual const BYTE* GetManagedTarget()
    {
        return (NULL);
    }

    // Return the # of stack bytes pushed by the unmanaged caller.
    virtual UINT GetNumCallerStackBytes() = 0;

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    LPVOID    m_pvDatum;       // type depends on the sub class
#ifdef _PPC_
    LinkageArea m_Link;
#else
    LPVOID    m_ReturnAddress;  // return address into unmanaged code
#endif
};


//-----------------------------------------------------------------------
// Transition frame from unmanaged to managed
//
// this frame contains some object reference at negative
// offset which need to be promoted, the references could be [in] args during
// in the middle of marshalling or [out], [in,out] args that need to be tracked
//------------------------------------------------------------------------
class UnmanagedToManagedCallFrame : public UnmanagedToManagedFrame
{
public:
    DEFINE_STD_FILL_FUNCS(UnmanagedToManagedCallFrame)
    DEFINE_STD_FRAME_FUNCS(UnmanagedToManagedCallFrame)

    // Return the # of stack bytes pushed by the unmanaged caller.
    virtual UINT GetNumCallerStackBytes()
    {
        return (0);
    }
};





// Some context-related forwards.


//------------------------------------------------------------------------
// This represents a declarative secuirty check. This frame is inserted
// prior to calls on methods that have declarative security defined for
// the class or the specific method that is being called. This frame
// is only created when the prestubworker creates a real stub.
//------------------------------------------------------------------------
class SecurityFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(SecurityFrame)
    DEFINE_STD_FRAME_FUNCS(SecurityFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(SecurityFrame);
    }

    int GetFrameType()
    {
        return (TYPE_SECURITY);
    }
};


//------------------------------------------------------------------------
// This represents a call to a method prestub. Because the prestub
// can do gc and throw exceptions while building the replacement
// stub, we need this frame to keep things straight.
//------------------------------------------------------------------------
class PrestubMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(PrestubMethodFrame)
    DEFINE_STD_FRAME_FUNCS(PrestubMethodFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(PrestubMethodFrame);
    }

    int GetFrameType()
    {
        return (TYPE_INTERCEPTION);
    }
};

//------------------------------------------------------------------------
// This represents a call to a method prestub. Because the prestub
// can do gc and throw exceptions while building the replacement
// stub, we need this frame to keep things straight.
//------------------------------------------------------------------------
class InterceptorFrame : public SecurityFrame
{
public:
    DEFINE_STD_FILL_FUNCS(InterceptorFrame)
    DEFINE_STD_FRAME_FUNCS(InterceptorFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(InterceptorFrame);
    }
};



//------------------------------------------------------------------------
// This frame protects object references for the EE's convenience.
// This frame type actually is created from C++.
//------------------------------------------------------------------------
class GCFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(GCFrame)
    DEFINE_STD_FRAME_FUNCS(GCFrame)

    //--------------------------------------------------------------------
    // This constructor pushes a new GCFrame on the frame chain.
    //--------------------------------------------------------------------
    GCFrame()
    {
    }; 

    /*OBJECTREF*/

void *m_pObjRefs;
    UINT       m_numObjRefs;
    Thread    *m_pCurThread;
    BOOL       m_MaybeInterior;
};

struct ByRefInfo;

class ProtectByRefsFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ProtectByRefsFrame)
    DEFINE_STD_FRAME_FUNCS(ProtectByRefsFrame)

    ByRefInfo *m_brInfo;
    Thread    *m_pThread;
};

struct ValueClassInfo;

class ProtectValueClassFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ProtectValueClassFrame)
    DEFINE_STD_FRAME_FUNCS(ProtectValueClassFrame)

    ValueClassInfo *m_pVCInfo;
    Thread    *m_pThread;
};

//------------------------------------------------------------------------
// DebuggerClassInitMarkFrame is a small frame whose only purpose in
// life is to mark for the debugger that "class initialization code" is
// being run. It does nothing useful except return good values from
// GetFrameType and GetInterception.
//------------------------------------------------------------------------
class DebuggerClassInitMarkFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(DebuggerClassInitMarkFrame)
    DEFINE_STD_FRAME_FUNCS(DebuggerClassInitMarkFrame)

    virtual int GetFrameType()
    {
        return (TYPE_INTERCEPTION);
    }
};

//------------------------------------------------------------------------
// DebuggerSecurityCodeMarkFrame is a small frame whose only purpose in
// life is to mark for the debugger that "security code" is
// being run. It does nothing useful except return good values from
// GetFrameType and GetInterception.
//------------------------------------------------------------------------
class DebuggerSecurityCodeMarkFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(DebuggerSecurityCodeMarkFrame)
    DEFINE_STD_FRAME_FUNCS(DebuggerSecurityCodeMarkFrame)

    virtual int GetFrameType()
    {
        return (TYPE_INTERCEPTION);
    }
};

//------------------------------------------------------------------------
// DebuggerExitFrame is a small frame whose only purpose in
// life is to mark for the debugger that there is an exit transiton on
// the stack.  This is special cased for the "break" IL instruction since
// it is an fcall using a helper frame which returns TYPE_CALL instead of
// an ecall (as in System.Diagnostics.Debugger.Break()) which returns
// TYPE_EXIT.  This just makes the two consistent for debugging services.
//------------------------------------------------------------------------
class DebuggerExitFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(DebuggerExitFrame)
    DEFINE_STD_FRAME_FUNCS(DebuggerExitFrame)

    virtual int GetFrameType()
    {
        return (TYPE_EXIT);
    }
};




//------------------------------------------------------------------------
// This frame guards an unmanaged->managed transition thru a UMThk
//------------------------------------------------------------------------
class UMThkCallFrame : public UnmanagedToManagedCallFrame
{
public:
    DEFINE_STD_FILL_FUNCS(UMThkCallFrame)
    DEFINE_STD_FRAME_FUNCS(UMThkCallFrame)
};




//------------------------------------------------------------------------
// This frame is used to mark a Context/AppDomain Transition
//------------------------------------------------------------------------
class ContextTransitionFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ContextTransitionFrame)
    DEFINE_STD_FRAME_FUNCS(ContextTransitionFrame)

    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ContextTransitionFrame);
    }
};

#endif //_tst_frames_h
