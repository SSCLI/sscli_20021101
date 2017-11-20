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
//    |   |
//    |   +-PrestubMethodFrame   - represents a call to a prestub
//    |   |
//    |   +-CtxCrossingFrame     - this frame marks a call across a context
//    |   |                        boundary
//    |   |
//    |   +-MulticastFrame       - this frame protects arguments to a MulticastDelegate
//    |                             Invoke() call while calling each subscriber.
//    |    
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
//    |   +-UMThkCallFrame      - this frame represents an unmanaged->managed
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


#ifndef __frames_h__
#define __frames_h__

#include "util.hpp"
#include "vars.hpp"
#include "object.h"
#include "objecthandle.h"
#include "regdisp.h"
#include <stddef.h>
#include "gcscan.h"
#include "siginfo.hpp"
// context headers
#include "context.h"
#include "method.hpp"
#include "stackwalk.h"
#include "stubmgr.h"
#include "gms.h"
#include "threads.h"
// remoting headers
//#include "remoting.h"

// Forward references
class Frame;
class FieldMarshaler;
class FramedMethodFrame;
struct HijackArgs;
class UMEntryThunk;
class UMThunkMarshInfo;
class Marshaler;
class SecurityDescriptor;

// Security Frame for all IDispEx::InvokeEx calls. Enable in security.h also
// Consider post V.1
// #define _SECURITY_FRAME_FOR_DISPEX_CALLS



//------------------------------------------------------------------------
// CleanupWorkList
//
// A CleanupWorkList is a LIFO list of tasks to be carried out at a later
// time. It's designed for use in managed->unmanaged calls.
//
// NOTE: CleanupWorkList's are designed to be embedded inside method frames,
// hence they can be constructed by stubs. Thus, any changes to the layout
// or constructor will also require changing some stubs.
//
// CleanupWorkLists are not synchronized for multithreaded use.
//
// The current layout of a CleanupWorkList is simply a pointer to a linked
// list of tasks (CleanupNodes.) This makes it very easy for a stub to
// stack-allocate an empty CleanupWorkList (just push a NULL pointer)
// and equally easy for it to inline a test to see if any cleanup needs
// to be done.
//
// NOTE: CleanupTasks can execute during exception handling so they
// shouldn't be calling other managed code or throwing COM+ exceptions.
//------------------------------------------------------------------------
class CleanupWorkList
{
    public:
        //-------------------------------------------------------------------
        // Constructor.
        //-------------------------------------------------------------------
        CleanupWorkList()
        {
            // NOTE: IF YOU CHANGE THIS, YOU WILL ALSO HAVE TO CHANGE SOME
            // STUBS.
            m_pNodes = NULL;
        }

        //-------------------------------------------------------------------
        // Destructor (calls Cleanup(FALSE))
        //-------------------------------------------------------------------
        ~CleanupWorkList();


        //-------------------------------------------------------------------
        // Executes each stored cleanup task and resets the worklist back
        // to empty. Some task types are conditional based on the
        // "fBecauseOfException" flag. This flag distinguishes between
        // cleanups due to normal method termination and cleanups due to
        // an exception.
        //-------------------------------------------------------------------
        VOID __stdcall Cleanup(BOOL fBecauseOfException);


        //-------------------------------------------------------------------
        // Allocates a gc-protected handle. This handle is unconditionally
        // destroyed on a Cleanup().
        // Throws a COM+ exception if failed.
        //-------------------------------------------------------------------
        OBJECTHANDLE NewScheduledProtectedHandle(OBJECTREF oref);


        //-------------------------------------------------------------------
        // CoTaskFree memory unconditionally
        //-------------------------------------------------------------------
        VOID ScheduleCoTaskFree(LPVOID pv);

        //-------------------------------------------------------------------
        // StackingAllocator.Collapse during exceptions
        //-------------------------------------------------------------------
        VOID ScheduleFastFree(LPVOID checkpoint);



        //-------------------------------------------------------------------
        // Schedules an unconditional free of the native version
        // of an NStruct reference field. Note that pNativeData points into
        // the middle of the external part of the NStruct, so someone
        // has to hold a gc reference to the wrapping NStruct until
        // the destroy is done.
        //-------------------------------------------------------------------
        VOID ScheduleUnconditionalNStructDestroy(const FieldMarshaler *pFieldMarshaler, LPVOID pNativeData);


        //-------------------------------------------------------------------
        // CleanupWorkList::ScheduleUnconditionalCultureRestore
        // schedule restoring thread's current culture to the specified 
        // culture.
        // Throws a COM+ exception if failed.
        //-------------------------------------------------------------------
        VOID ScheduleUnconditionalCultureRestore(OBJECTREF CultureObj);

        //-------------------------------------------------------------------
        // CleanupWorkList::ScheduleLayoutDestroy
        // schedule cleanup of marshaled struct fields and of the struct itself.
        // Throws a COM+ exception if failed.
        //-------------------------------------------------------------------
        LPVOID NewScheduleLayoutDestroyNative(MethodTable *pMT);


        //-------------------------------------------------------------------
        // CleanupWorkList::NewProtectedObjRef()
        // holds a protected objref (used for creating the buffer for
        // an unmanaged->managed byref object marshal. We can't use an
        // objecthandle because modifying those without using the handle
        // api opens up writebarrier violations.
        //
        // Must have called IsVisibleToGc() first.
        //-------------------------------------------------------------------
        OBJECTREF* NewProtectedObjectRef(OBJECTREF oref);

        //-------------------------------------------------------------------
        // CleanupWorkList::NewProtectedObjRef()
        // holds a Marshaler. The cleanupworklist will own the task
        // of calling the marshaler's GcScanRoots fcn.
        //
        // It makes little architectural sense for the CleanupWorkList to
        // own this item. But it's late in the project to be adding
        // fields to frames, and it so happens everyplace we need this thing,
        // there's alreay a cleanuplist. So it's elected.
        //
        // Must have called IsVisibleToGc() first.
        //-------------------------------------------------------------------
        VOID NewProtectedMarshaler(Marshaler *pMarshaler);


        //-------------------------------------------------------------------
        // CleanupWorkList::ScheduleMarshalerCleanupOnException()
        // holds a Marshaler. The cleanupworklist will own the task
        // of calling the marshaler's DoExceptionCleanup() if an exception
        // occurs.
        //
        // The return value is a cookie thru which the marshaler can
        // cancel this item. It must do this once to avoid double
        // destruction if the marshaler cleanups normally.
        //-------------------------------------------------------------------
        class MarshalerCleanupNode;
        MarshalerCleanupNode *ScheduleMarshalerCleanupOnException(Marshaler *pMarshaler);


        //-------------------------------------------------------------------
        // CleanupWorkList::IsVisibleToGc()
        //-------------------------------------------------------------------
        VOID IsVisibleToGc()
        {
#ifdef _DEBUG
            Schedule(CL_ISVISIBLETOGC, NULL);
#endif
        }



        //-------------------------------------------------------------------
        // If you've called IsVisibleToGc(), must call this.
        //-------------------------------------------------------------------
        void GcScanRoots(promote_func *fn, ScanContext* sc);




    private:
        //-------------------------------------------------------------------
        // Cleanup task types.
        //-------------------------------------------------------------------
        enum CleanupType {
            CL_GCHANDLE,    //gc protected handle,
            CL_COTASKFREE,      // unconditional cotaskfree
            CL_FASTFREE,       // unconditionally StackingAllocator.Collapse
            CL_NSTRUCTDESTROY, // unconditionally do a DestroyNative on an NStruct ref field
            CL_RESTORECULTURE, // unconditionally restore the culture
            CL_NEWLAYOUTDESTROYNATIVE,

            CL_PROTECTEDOBJREF, // holds a GC protected OBJECTREF - similar to CL_GCHANDLE
                             // but can be safely written into without updating
                             // write barrier.
                             //
                             // Must call IsVisibleToGc() before using this nodetype.
                             //
            CL_PROTECTEDMARSHALER, // holds a GC protected marshaler
                             // Must call IsVisibleToGc() before using this nodetype.


            CL_ISVISIBLETOGC,// a special do-nothing nodetype that simply
                             // records that "IsVisibleToGc()" was called on this.


            CL_MARSHALER_EXCEP, // holds a marshaler for cleanup on exception
            CL_MARSHALERREINIT_EXCEP, // holds a marshaler for reiniting on exception
        };

        //-------------------------------------------------------------------
        // These are linked into a list.
        //-------------------------------------------------------------------
        struct CleanupNode {
            CleanupType     m_type;       // See CleanupType enumeration
            CleanupNode    *m_next;       // pointer to next task
#ifdef _DEBUG
            DWORD m_dwDomainId;           // domain Id of list. 
#endif
            union {
                OBJECTHANDLE m_oh;        // CL_GCHANDLE
                Object*     m_oref;       // CL_PROTECTEDOBJREF
                IUnknown   *m_ip;         // if CL_RELEASE
                LPVOID      m_pv;         // CleanupType-dependent contents.
                SAFEARRAY  *m_safearray;
                Marshaler  *m_pMarshaler;

                struct {                  // if CL_NSTRUCTDESTROY
                    const FieldMarshaler *m_pFieldMarshaler;
                    LPVOID                m_pNativeData;
                } nd;

                struct {
                    LPVOID  m_pnative;
                    MethodTable *m_pMT;
                } nlayout;
            };

        };


        //-------------------------------------------------------------------
        // Inserts a new task of the given type and datum.
        // Returns non NULL on success.
        //-------------------------------------------------------------------
        CleanupNode* Schedule(CleanupType ct, LPVOID pv);

    public:
        class MarshalerCleanupNode : private CleanupNode
        {
            // DO NOT ADD ANY FIELDS!
            public:
                void CancelCleanup()
                {
                    m_type = CL_MARSHALERREINIT_EXCEP;
                }
    
        };


    private:
        // NOTE: If you change the layout of this structure, you will
        // have to change some stubs which build and manipulate
        // CleanupWorkLists.
        CleanupNode     *m_pNodes;   //Pointer to first task.
};




// Note: the value (-1) is used to generate the largest
// possible pointer value: this keeps frame addresses
// increasing upward.
#define FRAME_TOP ((Frame*)(-1))

#define RETURNFRAMEVPTR(classname) \
    classname boilerplate;      \
    return *((LPVOID*)&boilerplate)

#define DEFINE_VTABLE_GETTER(klass)             \
    public:                                     \
        friend struct MEMBER_OFFSET_INFO(klass);\
        static LPVOID GetFrameVtable() {        \
            klass boilerplate(false);           \
            return *((LPVOID*)&boilerplate);    \
        }                                       \
        klass(bool dummy) { }

#define DEFINE_VTABLE_GETTER_AND_CTOR(klass)    \
        DEFINE_VTABLE_GETTER(klass)             \
    protected:                                  \
        klass() { }

#if _DEBUG
extern "C" void __stdcall CheckExitFrameDebuggerCalls();
#endif


// Frame depends on the location of its vtable within the object. This
// superclass ensures that the vtable for Frame objects is in the same
// location under both MSVC and GCC.
class FrameBase
{
public:
    virtual void GcScanRoots(promote_func *fn, ScanContext* sc) = 0;
};

//------------------------------------------------------------------------
// Frame defines methods common to all frame types. There are no actual
// instances of root frames.
//------------------------------------------------------------------------

class Frame : public FrameBase
{
public:

    //------------------------------------------------------------------------
    // Special characteristics of a frame
    //------------------------------------------------------------------------
    enum FrameAttribs {
        FRAME_ATTR_NONE = 0,
        FRAME_ATTR_EXCEPTION = 1,           // This frame caused an exception
        FRAME_ATTR_OUT_OF_LINE = 2,         // The exception out of line (IP of the frame is not correct)
        FRAME_ATTR_FAULTED = 4,             // Exception caused by Win32 fault
        FRAME_ATTR_RESUMABLE = 8,           // We may resume from this frame
        FRAME_ATTR_RETURNOBJ = 0x10,        // Frame returns an object (helperFrame only)
        FRAME_ATTR_RETURN_INTERIOR = 0x20,  // Frame returns an interior GC poitner (helperFrame only)
        FRAME_ATTR_CAPTURE_DEPTH_2 = 0x40,  // This is a helperMethodFrame and the capture occured at depth 2
        FRAME_ATTR_EXACT_DEPTH = 0x80,      // This is a helperMethodFrame and a jit helper, but only crawl to the given depth
    };
    virtual unsigned GetFrameAttribs()
    {
        return FRAME_ATTR_NONE;
    }

    //------------------------------------------------------------------------
    // Performs cleanup on an exception unwind
    //------------------------------------------------------------------------
    virtual void ExceptionUnwind()
    {
        // Nothing to do here.
    }

    //------------------------------------------------------------------------
    // Is this a frame used on transition to native code from jitted code?
    //------------------------------------------------------------------------
    virtual BOOL IsTransitionToNativeFrame()
    {
        return FALSE;
    }

    virtual MethodDesc *GetFunction()
    {
        return NULL;
    }

    virtual MethodDesc::RETURNTYPE ReturnsObject()
    {
        MethodDesc* pMD = GetFunction();
        if (pMD == 0)
            return(MethodDesc::RETNONOBJ);
        return(pMD->ReturnsObject());
    }

    // indicate the current X86 IP address within the current method
    // return 0 if the information is not available
    virtual const BYTE* GetIP()
    {
        return NULL;
    }

    LPVOID GetReturnAddress()
    {
        LPVOID* ptr = GetReturnAddressPtr();
        return ptr ? *ptr : NULL;
    }

    void SetReturnAddress(LPVOID val)
    {
        LPVOID* ptr = GetReturnAddressPtr();
        _ASSERTE(ptr != NULL);
        *ptr = val;
    }

    virtual LPVOID* GetReturnAddressPtr()
    {
        return NULL;
    }

    virtual Context **GetReturnContextAddr()
    {
        return NULL;
    }

    virtual Object **GetReturnLogicalCallContextAddr()
    {
        return NULL;
    }

    virtual Object **GetReturnIllogicalCallContextAddr()
    {
        return NULL;
    }

    virtual ULONG_PTR* GetWin32ContextAddr()
    {
        return NULL;
    }

    void SetReturnContext(Context *pReturnContext)
    {
        Context **ppReturnContext = GetReturnContextAddr();
        _ASSERTE(ppReturnContext);
        *ppReturnContext = pReturnContext;
    }

    Context *GetReturnContext()
    {
        Context **ppReturnContext = GetReturnContextAddr();
        if (! ppReturnContext)
            return NULL;
        return *ppReturnContext;
    }

    AppDomain *GetReturnDomain()
    {
        if (! GetReturnContext())
            return NULL;
        return GetReturnContext()->GetDomain();
    }

    void SetReturnLogicalCallContext(OBJECTREF ref)
    {
        Object **pRef = GetReturnLogicalCallContextAddr();
        if (pRef != NULL)
            *pRef = OBJECTREFToObject(ref);
    }

    OBJECTREF GetReturnLogicalCallContext()
    {
        Object **pRef = GetReturnLogicalCallContextAddr();
        if (pRef == NULL)
            return NULL;
        else
            return ObjectToOBJECTREF(*pRef);
    }

    void SetReturnIllogicalCallContext(OBJECTREF ref)
    {
        Object **pRef = GetReturnIllogicalCallContextAddr();
        if (pRef != NULL)
            *pRef = OBJECTREFToObject(ref);
    }

    OBJECTREF GetReturnIllogicalCallContext()
    {
        Object **pRef = GetReturnIllogicalCallContextAddr();
        if (pRef == NULL)
            return NULL;
        else
            return ObjectToOBJECTREF(*pRef);
    }

    void SetWin32Context(ULONG_PTR cookie)
    {
        ULONG_PTR* pAddr = GetWin32ContextAddr();
        if(pAddr != NULL)
            *pAddr = cookie;
    }

    ULONG_PTR GetWin32Context()
    {
        ULONG_PTR* pAddr = GetWin32ContextAddr();
        if(pAddr == NULL)
            return NULL;
        else
            return *pAddr;
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
        return TYPE_INTERNAL;
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
        return INTERCEPTION_NONE;
    }

    // Return information about an unmanaged call the frame
    // will make.
    // ip - the unmanaged routine which will be called
    // returnIP - the address in the stub which the unmanaged routine
    //            will return to.
    // returnSP - the location returnIP is pushed onto the stack
    //            during the call.
    //
    virtual void GetUnmanagedCallSite(void **ip,
                                      void **returnIP,
                                      void **returnSP)
    {
        if (ip)
            *ip = NULL;

        if (returnIP)
            *returnIP = NULL;

        if (returnSP)
            *returnSP = NULL;
    }

    // Return where the frame will execute next - the result is filled
    // into the given "trace" structure.  The frame is responsible for
    // detecting where it is in its execution lifetime.
    virtual BOOL TraceFrame(Thread *thread, BOOL fromPatch,
                            TraceDestination *trace, REGDISPLAY *regs)
    {
        LOG((LF_CORDB, LL_INFO10000,
             "Default TraceFrame always returns false.\n"));
        return FALSE;
    }

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static BYTE GetOffsetOfNextLink()
    {
        size_t ofs = offsetof(class Frame, m_Next);
        _ASSERTE(FitsInI1(ofs));
        return (BYTE)ofs;
    }

    // get your VTablePointer (can be used to check what type the frame is)
    LPVOID GetVTablePtr()
    {
        return(*((LPVOID*) this));
    }

    // Change the type of frame (pretty dangerous),
    void SetVTablePtr(LPVOID val)
    {
        *((LPVOID*) this) = val;
    }

#ifdef _DEBUG
    virtual BOOL Protects(OBJECTREF *ppObjectRef)
    {
        return FALSE;
    }
#endif

    // Link and Unlink this frame
    VOID Push();
    VOID Pop();
    VOID Push(Thread *pThread);
    VOID Pop(Thread *pThread);

#ifdef _DEBUG
    virtual void Log();
    static void __stdcall LogTransition(Frame* frame);
#endif

    // Returns true only for frames derived from FramedMethodFrame.
    virtual BOOL IsFramedMethodFrame() { return FALSE; }

private:
    // Pointer to the next frame up the stack.

protected:
    Frame   *m_Next;        // offset +4

private:
    // Because JIT-method activations cannot be expressed as Frames,
    // everyone must use the StackCrawler to walk the frame chain
    // reliably. We'll expose the Next method only to the StackCrawler
    // to prevent mistakes.
    /*                                                                   
    */
    //        friend Frame* Thread::StackWalkFrames(PSTACKWALKFRAMESCALLBACK pCallback, VOID *pData);
    friend class Thread;
    friend void CrawlFrame::GotoNextFrame();
    friend VOID RealCOMPlusThrow(OBJECTREF);
    friend FCDECL0(VOID, JIT_StressGC);
    Frame   *Next()
    {
        return m_Next;
    }


protected:
    // Frame is considered an abstract class: this protected constructor
    // causes any attempt to instantiate one to fail at compile-time.
    Frame() {}

    friend struct MEMBER_OFFSET_INFO(Frame);
};


#ifdef _DEBUG
class FCallInProgressFrame : public Frame
{
    public:
        FCallInProgressFrame()
        {
            m_Next = FRAME_TOP;
        }

        virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
        {}

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(FCallInProgressFrame)
};
#endif



#define ISREDIRECTEDTHREAD(thread)                                                      \
    FALSE

#define GETREDIRECTEDCONTEXT(thread)                                                    \
    ((CONTEXT*)NULL)



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

        virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
        {
            // Nothing to protect here.
        }

        virtual void UpdateRegDisplay(const PREGDISPLAY) = 0;

    protected:
        LPVOID  m_Datum;          // offset +8: contents depend on subclass
                                  // type.

        friend struct MEMBER_OFFSET_INFO(TransitionFrame);
};


//-----------------------------------------------------------------------
// TransitionFrames for exceptions
//-----------------------------------------------------------------------


class ExceptionFrame : public TransitionFrame
{
public:
    static LPVOID GetMethodFrameVPtr()
    {
        _ASSERTE(!"This is a pure virtual class");
        return 0;
    }

    Interception GetInterception()
    {
        return INTERCEPTION_EXCEPTION;
    }

    unsigned GetFrameAttribs()
    {
        return FRAME_ATTR_EXCEPTION;
    }

    friend struct MEMBER_OFFSET_INFO(ExceptionFrame);
};

class FaultingExceptionFrame : public ExceptionFrame
{
    friend class CheckAsmOffsets;

#if defined(_X86_)
    LPVOID  m_ReturnAddress;  // return address into JIT'ted code
    DWORD   m_Esp;
    CalleeSavedRegisters m_regs;
#elif defined(_PPC_)
    DWORD                m_pad;
    CalleeSavedRegisters m_regs;
    LinkageArea          m_Link;
#else
    PORTABILITY_WARNING("FaultingExceptionFrame is not implemented on this platform.")
    LPVOID              m_ReturnAddress;
    CalleeSavedRegisters m_regs;
#endif

public:
    FaultingExceptionFrame() { m_Next = NULL; }
    void InitAndLink(CONTEXT *pContext);

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

    CalleeSavedRegisters *GetCalleeSavedRegisters()
    {
        return &m_regs;
    }

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(FaultingExceptionFrame);
    }

    unsigned GetFrameAttribs()
    {
        return FRAME_ATTR_EXCEPTION | FRAME_ATTR_FAULTED;
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);


    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(FaultingExceptionFrame)
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
    LPVOID  m_ReturnAddress;  // return address into JIT'ted code

public:
    FuncEvalFrame(void *pDebuggerEval, LPVOID returnAddress)
    {
        m_Datum = pDebuggerEval;
        m_ReturnAddress = returnAddress;
    }

    virtual BOOL IsTransitionToNativeFrame()
    {
        return FALSE; 
    }

    virtual int GetFrameType()
    {
        return TYPE_FUNC_EVAL;
    };

    virtual unsigned GetFrameAttribs();

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    virtual void *GetDebuggerEval()
    {
        return (void*)m_Datum;
    }

    virtual LPVOID* GetReturnAddressPtr();
    
    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(FuncEvalFrame)
};


//-----------------------------------------------------------------------
// Provides access to the caller's arguments from a FramedMethodFrame.
// Does *not* include the "this" pointer.
//-----------------------------------------------------------------------
class ArgIterator
{
    public:
        //------------------------------------------------------------
        // Constructor
        //------------------------------------------------------------
        ArgIterator(FramedMethodFrame *pFrame, MetaSig* pSig);

        //------------------------------------------------------------
        // Another constructor when you dont have active frame FramedMethodFrame
        //------------------------------------------------------------
        ArgIterator(LPBYTE pFrameBase, MetaSig* pSig, BOOL fIsStatic);

        //------------------------------------------------------------
        // An even more primitive constructor when dont have have a
        // a FramedMethodFrame
        //------------------------------------------------------------
        ArgIterator(LPBYTE pFrameBase, MetaSig* pSig, int stackArgsOfs, int regArgsOfs);


        //------------------------------------------------------------
        // Each time this is called, this returns a pointer to the next
        // argument. This pointer points directly into the caller's stack.
        // Whether or not object arguments returned this way are gc-protected
        // depends on the exact type of frame.
        //
        // Returns NULL once you've hit the end of the list.
        //------------------------------------------------------------
        LPVOID GetNextArgAddr()
        {
            BYTE   typeDummy;
            UINT32 structSizeDummy;
            return GetNextArgAddr(&typeDummy, &structSizeDummy);
        }

        static int GetThisOffset(MetaSig* pSig);
        static int GetRetBuffArgOffset(MetaSig* pSig, UINT *pRegStructOfs = NULL);

        int GetThisOffset() {
            return GetThisOffset(m_pSig);
        }
        int GetRetBuffArgOffset(UINT *pRegStructOfs = NULL) {
            return GetRetBuffArgOffset(m_pSig, pRegStructOfs);
        }

        LPVOID* GetThisAddr()   {
            return((LPVOID*) (m_pFrameBase + GetThisOffset(m_pSig)));
        }
        LPVOID* GetRetBuffArgAddr() {
            return((LPVOID*) (m_pFrameBase + GetRetBuffArgOffset(m_pSig)));
        }

        //------------------------------------------------------------
        // Like GetNextArgAddr but returns information about the
        // param type (IMAGE_CEE_CS_*) and the structure size if apropos.
        //------------------------------------------------------------
        LPVOID GetNextArgAddr(BYTE *pType, UINT32 *pStructSize);

        //------------------------------------------------------------
        // Same as GetNextArgAddr() but returns a byte offset from
        // the Frame* pointer. This offset can be positive *or* negative.
        //
        // Returns 0 once you've hit the end of the list. Since the
        // the offset is relative to the Frame* pointer itself, 0 can
        // never point to a valid argument.
        //------------------------------------------------------------
        int    GetNextOffset()
        {
            BYTE   typeDummy;
            UINT32 structSizeDummy;
            return GetNextOffset(&typeDummy, &structSizeDummy);
        }

        //------------------------------------------------------------
        // Like GetNextArgOffset but returns information about the
        // param type (IMAGE_CEE_CS_*) and the structure size if apropos.
        // The optional pRegStructOfs param points to a buffer which receives
        // either the appropriate offset into the ArgumentRegisters struct or
        // -1 if the argument is on the stack.
        //------------------------------------------------------------
        int    GetNextOffset(BYTE *pType, UINT32 *pStructSize, UINT *pRegStructOfs = NULL);

        int    GetNextOffsetFaster(BYTE *pType, UINT32 *pStructSize, UINT *pRegStructOfs = NULL);

        // Must be called after all the args.  returns the offset of the
        // argument passed to methods implementing parameterized types.
        // If it is in a register pRegStructOfs is set, otherwise it is -1
        // In either case it returns off offset in the frame of the arg (assuming
        // it is framed).

        int     GetParamTypeArgOffset(INT *pRegStructOfs)
        {
            if (IsArgumentInRegister(&m_numRegistersUsed,
                                     ELEMENT_TYPE_I,
                                     sizeof(void*), FALSE,
                                     m_pSig->GetCallingConvention(),
                                     pRegStructOfs))
            {
                return m_regArgsOfs + *pRegStructOfs;
            }
            *pRegStructOfs = -1;
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
            return m_curOfs - StackElemSize(sizeof(void*));
#else
            return m_curOfs;
#endif
        }

        TypeHandle GetArgType();

    private:
        MetaSig*            m_pSig;
        int                 m_curOfs;
        LPBYTE              m_pFrameBase;
        int                 m_numRegistersUsed;
        int                 m_regArgsOfs;       // add this to pFrameBase to find the the pointer
                                                // to where the last register based argument has
                                                // been saved in the frame (again stack grows down
                                                // first arg pushed first).  0 is an illegal value
                                                // than means the register args are not saved on the
                                                // stack.
        int                 m_argNum;

        // shared init
        void Init(LPBYTE pFrameBase, MetaSig* pSig, BOOL fIsStatic, int stackArgsOfs, int regArgsOfs);
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
        // Lazy initialization of HelperMethodFrame.  Need to
        // call InsureInit to complete initialization
        // If this is an FCall, the second param is the entry point for the FCALL.
        // The MethodDesc will be looked up form this (lazily), and this method
        // will be used in stack reporting, if this is not an FCall pass a 0
    HelperMethodFrame(void* fCallFtnEntry, struct LazyMachState* ms, unsigned attribs = 0) {
         INDEBUG(memset(&m_Attribs, 0xCC, sizeof(HelperMethodFrame) - offsetof(HelperMethodFrame, m_Attribs));)
         m_Attribs = attribs;
         LazyInit(fCallFtnEntry, ms);
    }
       
        // If you give the optional MethodDesc parameter, then the frame
        // will act like like the given method for stack tracking purposes.
        // If you also give regArgs != 0, then the helper frame will
        // will also promote the arguments for you (Pretty neat, huh?)
    HelperMethodFrame(struct MachState* ms, MethodDesc* pMD, ArgumentRegisters* regArgs=0);
    
    LPVOID* GetReturnAddressPtr() {
        _ASSERTE(m_MachState->isValid());
        return m_MachState->_pRetAddr;
    }
    virtual MethodDesc* GetFunction() {
        InsureInit();
        return((MethodDesc*) m_Datum);
    }
    virtual MethodDesc::RETURNTYPE ReturnsObject();
    virtual void UpdateRegDisplay(const PREGDISPLAY);
    virtual void GcScanRoots(promote_func *fn, ScanContext* sc);
    virtual Interception GetInterception() {
        if (GetFrameAttribs() & FRAME_ATTR_EXCEPTION)
            return(INTERCEPTION_EXCEPTION);
        return(INTERCEPTION_NONE);
    }
    virtual unsigned GetFrameAttribs() {
        return(m_Attribs);
    }
    void SetFrameAttribs(unsigned attribs) {
        m_Attribs = attribs;
    }
    void Pop() {
        Frame::Pop(m_pThread);
    }
    void Poll() { 
        if (m_pThread->CatchAtSafePoint())
            CommonTripThread();
    }
    void InsureInit();
    void Init(Thread *pThread, struct MachState* ms, MethodDesc* pMD, ArgumentRegisters * regArgs);
    MachState * MachineState() { return m_MachState; }

    INDEBUG(static LPVOID GetMethodFrameVPtr() { RETURNFRAMEVPTR(HelperMethodFrame); })
protected:
    
    HelperMethodFrame::HelperMethodFrame() {
        INDEBUG(memset(&m_Attribs, 0xCC, sizeof(HelperMethodFrame) - offsetof(HelperMethodFrame, m_Attribs));)
    }
    void LazyInit(void* FcallFtnEntry, struct LazyMachState* ms);

protected:
    unsigned m_Attribs;
    MachState* m_MachState;         // pRetAddr points to the return address and the stack arguments
    ArgumentRegisters * m_RegArgs;  // if non-zero we also report these as the register arguments 
    Thread *m_pThread;
    void* m_FCallEntry;             // used to determine our identity for stack traces

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(HelperMethodFrame)
};

// Restores registers saved in m_MachState
EXTERN_C int __fastcall HelperMethodFrameRestoreState(
	INDEBUG_COMMA(HelperMethodFrame *pFrame)
        MachState *pState
	);



/* report all the args (but not THIS if present), to the GC. 'framePtr' points at the
   frame (promote doesn't assume anthing about its structure).  'msig' describes the
   arguments, and 'ctx' has the GC reporting info.  'stackArgsOffs' is the byte offset
   from 'framePtr' where the arguments start (args start at last and grow bacwards).
   Simmilarly, 'regArgsOffs' is the offset to find the register args to promote */
void promoteArgs(BYTE* framePtr, MetaSig* msig, GCCONTEXT* ctx, int stackArgsOffs, int regArgsOffs);

// workhorse for our promotion efforts
inline void DoPromote(promote_func *fn, ScanContext* sc, OBJECTREF *address, BOOL interior)
{
    LOG((LF_GC, INFO3, 
         "    Promoting pointer argument at" FMT_ADDR "from" FMT_ADDR "to ", 
         DBG_ADDR(address), DBG_ADDR(OBJECTREFToObject(*address)) ));
    if (interior)
        PromoteCarefully(fn, *((Object**)address), sc);
    else
        (*fn) (*((Object **)address), sc, 0);
    LOG((LF_GC, INFO3, "    " FMT_ADDR "\n", DBG_ADDR(OBJECTREFToObject(*address)) ));
}

/***********************************************************************************/
/* a HelplerMethodFrames that also report additional object references */

class HelperMethodFrame_1OBJ : public HelperMethodFrame {
public:
    HelperMethodFrame_1OBJ() { INDEBUG(gcPtrs[0] = (OBJECTREF*)POISONC;) }

    HelperMethodFrame_1OBJ(void* fCallFtnEntry, struct LazyMachState* ms, unsigned attribs, OBJECTREF* aGCPtr1)
        : HelperMethodFrame(fCallFtnEntry, ms, attribs) 
        {
        gcPtrs[0] = aGCPtr1; 
        INDEBUG(Thread::ObjectRefProtected(aGCPtr1);)
        }

    HelperMethodFrame_1OBJ(void* fCallFtnEntry, struct LazyMachState* ms, OBJECTREF* aGCPtr1)
        : HelperMethodFrame(fCallFtnEntry, ms) 
        { 
        gcPtrs[0] = aGCPtr1; 
        INDEBUG(Thread::ObjectRefProtected(aGCPtr1);)
        }
        
    void SetProtectedObject(OBJECTREF* objPtr) {
        gcPtrs[0] = objPtr; 
        INDEBUG(Thread::ObjectRefProtected(objPtr);)
        }

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc) {
        DoPromote(fn, sc, gcPtrs[0], FALSE);
        HelperMethodFrame::GcScanRoots(fn, sc);
    }

#ifdef _DEBUG
    void Pop() {
        HelperMethodFrame::Pop();
        Thread::ObjectRefNew(gcPtrs[0]);   
    }

    BOOL Protects(OBJECTREF *ppORef)
    {
        return (ppORef == gcPtrs[0]) ? TRUE : FALSE;
    }

#endif

private:
    OBJECTREF*  gcPtrs[1];

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(HelperMethodFrame_1OBJ)
};

class HelperMethodFrame_2OBJ : public HelperMethodFrame {
public:
    HelperMethodFrame_2OBJ(void* fCallFtnEntry, struct LazyMachState* ms, OBJECTREF* aGCPtr1, OBJECTREF* aGCPtr2)
        : HelperMethodFrame(fCallFtnEntry, ms) 
        {
        gcPtrs[0] = aGCPtr1; 
        gcPtrs[1] = aGCPtr2; 
        INDEBUG(Thread::ObjectRefProtected(aGCPtr1);)
        INDEBUG(Thread::ObjectRefProtected(aGCPtr2);)
        }
        
    HelperMethodFrame_2OBJ(void* fCallFtnEntry, struct LazyMachState* ms, unsigned attribs, OBJECTREF* aGCPtr1, OBJECTREF* aGCPtr2)
        : HelperMethodFrame(fCallFtnEntry, ms, attribs) 
        { 
        gcPtrs[0] = aGCPtr1; 
        gcPtrs[1] = aGCPtr2; 
        INDEBUG(Thread::ObjectRefProtected(aGCPtr1);)
        INDEBUG(Thread::ObjectRefProtected(aGCPtr2);)
        }

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc) {
        DoPromote(fn, sc, gcPtrs[0], FALSE);
        DoPromote(fn, sc, gcPtrs[1], FALSE);
        HelperMethodFrame::GcScanRoots(fn, sc);
    }

#ifdef _DEBUG
    void Pop() {
        HelperMethodFrame::Pop();
        Thread::ObjectRefNew(gcPtrs[0]); 
        Thread::ObjectRefNew(gcPtrs[1]); 
    }

    BOOL Protects(OBJECTREF *ppORef)
    {
        return (ppORef == gcPtrs[0] || ppORef == gcPtrs[1]) ? TRUE : FALSE;
    }
#endif

private:
    OBJECTREF*  gcPtrs[2];

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(HelperMethodFrame_2OBJ)
};

class HelperMethodFrame_4OBJ : public HelperMethodFrame {
public:
    HelperMethodFrame_4OBJ(void* fCallFtnEntry, struct LazyMachState* ms, 
        OBJECTREF* aGCPtr1, OBJECTREF* aGCPtr2, OBJECTREF* aGCPtr3, OBJECTREF* aGCPtr4 = NULL)
        : HelperMethodFrame(fCallFtnEntry, ms) 
    { 
        gcPtrs[0] = aGCPtr1; gcPtrs[1] = aGCPtr2; gcPtrs[2] = aGCPtr3; gcPtrs[3] = aGCPtr4; 
        INDEBUG(Thread::ObjectRefProtected(aGCPtr1);)
        INDEBUG(Thread::ObjectRefProtected(aGCPtr2);)
        INDEBUG(Thread::ObjectRefProtected(aGCPtr3);)
        INDEBUG(if (aGCPtr4) Thread::ObjectRefProtected(aGCPtr4);)
    }
        
    virtual void GcScanRoots(promote_func *fn, ScanContext* sc) {
        DoPromote(fn, sc, gcPtrs[0], FALSE);
        DoPromote(fn, sc, gcPtrs[1], FALSE);
        DoPromote(fn, sc, gcPtrs[2], FALSE);
        if (gcPtrs[3] != 0) DoPromote(fn, sc, gcPtrs[3], FALSE);
        HelperMethodFrame::GcScanRoots(fn, sc);
    }

#ifdef _DEBUG
    void Pop() {
        HelperMethodFrame::Pop();
        Thread::ObjectRefNew(gcPtrs[0]); 
        Thread::ObjectRefNew(gcPtrs[1]); 
        Thread::ObjectRefNew(gcPtrs[2]); 
        if (gcPtrs[3] != 0) Thread::ObjectRefNew(gcPtrs[3]); 
    }

    virtual BOOL Protects(OBJECTREF *ppORef)
    {
        for (UINT i = 0; i < 4; i++) {
            if (ppORef == gcPtrs[i]) {
                return TRUE;
            }
        }
        return FALSE;
    }
#endif

private:
    OBJECTREF*  gcPtrs[4];

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(HelperMethodFrame_4OBJ)
};

//------------------------------------------------------------------------
// This frame represents a method call. No actual instances of this
// frame exist: there are subclasses for each method type.
//
// However, they all share a similar image ...
//
#if defined(_X86_)
//
// x86:
//              +...    stack-based arguments here
//              +12     return address
//              +8      datum (typically a MethodDesc*)
//              +4      m_Next
//              +0      the frame vptr
//              -...    preserved CalleeSavedRegisters
//              -...    VC5Frame (debug only)
//              -...    ArgumentRegisters
//
#elif defined(_PPC_)
//
// PPC:
//              +...    stack-based arguments here
//              +...    this
//              +...    retbugarg
//      -- FramedMethodFrame end
//              +12     LinkageArea (contains return address at offset 8)
//              +8      datum (typically a MethodDesc*)
//              +4      m_Next
//              +0      the frame vptr
//      -- FramedMethodFrame start
//              -...    CalleeSavedRegisters
//              -...    ArgumentRegisters
//
#else
PORTABILITY_WARNING("FramedMethodFrame layout not documented")
#endif
//
//------------------------------------------------------------------------
class FramedMethodFrame : public TransitionFrame
{
#if defined(_PPC_)
        DWORD           m_pad;
        LinkageArea     m_Link;
#else
        LPVOID          m_ReturnAddress;  // return address into JIT'ted code
#endif

    public:

        // Retrieves the return address into the code that called the
        // helper or method.
        virtual LPVOID* GetReturnAddressPtr();

        // FramedMethodFrame must store some fields at negative offset.
        // This method exposes the size for people needing to allocate
        // FramedMethodFrame.
        static UINT32 GetNegSpaceSize()
        {
        #if defined(_X86_)
            return PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + ARGUMENTREGISTERS_SIZE + VC5FRAME_SIZE );
        #elif defined(_PPC_)
            return PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + ARGUMENTREGISTERS_SIZE);
        #else
            PORTABILITY_ASSERT("Calling convention not specified for new platform");
            return 0;
        #endif
        }

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static int GetOffsetOfArgumentRegisters()
        {
        #if defined(_X86_)
            return -((int)(sizeof(CalleeSavedRegisters) + ARGUMENTREGISTERS_SIZE + VC5FRAME_SIZE));
        #elif defined(_PPC_)
            return -((int)(sizeof(CalleeSavedRegisters) + ARGUMENTREGISTERS_SIZE));
        #else
            PORTABILITY_ASSERT("Calling convention not specified for new platform");
            return 0;
        #endif
        }

        CalleeSavedRegisters *GetCalleeSavedRegisters()
        {
            return (CalleeSavedRegisters*)( ((BYTE*)this) - sizeof(CalleeSavedRegisters) );
        }

        virtual MethodDesc *GetFunction()
        {
            return (MethodDesc*)m_Datum;
        }

        virtual void UpdateRegDisplay(const PREGDISPLAY);

        //------------------------------------------------------------------------
        // Returns the address of a security object or
        // null if there is no space for an object on this frame.
        //------------------------------------------------------------------------
        virtual OBJECTREF *GetAddrOfSecurityDesc()
        {
            return NULL;
        }

        // Get return value address
        virtual INT64 *GetReturnValuePtr()
        {
            return NULL;
        }

        //------------------------------------------------------------------------
        // Performs cleanup on an exception unwind
        //------------------------------------------------------------------------
        virtual void ExceptionUnwind()
        {
            if (GetFunction() && GetFunction()->IsSynchronized())
                UnwindSynchronized();

            TransitionFrame::ExceptionUnwind();
        }

        IMDInternalImport *GetMDImport()
        {
            _ASSERTE(GetFunction());
            return GetFunction()->GetMDImport();
        }

        Module *GetModule()
        {
            _ASSERTE(GetFunction());
            return GetFunction()->GetModule();
        }

        //---------------------------------------------------------------
        // Get the "this" object.
        //---------------------------------------------------------------
        OBJECTREF GetThis()
        {
            return *GetAddrOfThis();
        }

        //---------------------------------------------------------------
        // Get the address of the "this" object. WARNING!!! Whether or not "this"
        // is gc-protected is depends on the frame type!!!
        //---------------------------------------------------------------
        OBJECTREF* GetAddrOfThis()
        {
            MetaSig* pSig = NULL;
#if defined(_AMD64_) || defined(_PPC_)
            // some platforms needs to know if the method has a retbuff 
            // arg or not to find where the this pointer is.
            MetaSig msig(GetFunction());
            pSig = &msig;
#endif
            return (OBJECTREF*)(ArgIterator::GetThisOffset(pSig) + (LPBYTE)this);
        }

#ifdef _X86_
        //---------------------------------------------------------------
        // Get the offset of the stored "this" pointer relative to the frame.
        // This is a special _X86_ version that does not depends on the method signature
        //---------------------------------------------------------------
        static int GetOffsetOfThis() {
            return sizeof(FramedMethodFrame);
        }
#endif // _X86_

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static BYTE GetOffsetOfMethod()
        {
            size_t ofs = offsetof(class FramedMethodFrame, m_Datum);
            _ASSERTE(FitsInI1(ofs));
            return (BYTE)ofs;
        }

        static BYTE GetOffsetOfReturnAddress()
        {
        #if defined(_PPC_)
            size_t ofs = offsetof(class FramedMethodFrame, m_Link.SavedLR);
        #else
            size_t ofs = offsetof(class FramedMethodFrame, m_ReturnAddress);
        #endif
            _ASSERTE(FitsInI1(ofs));
            return (BYTE)ofs;
        }

        static BYTE GetOffsetOfArgs()
        {
            size_t ofs = sizeof(FramedMethodFrame);
            _ASSERTE(FitsInI1(ofs));
            return (BYTE)ofs;
        }

        //---------------------------------------------------------------
        // For vararg calls, return cookie.
        //---------------------------------------------------------------
        VASigCookie *GetVASigCookie()
        {
            return *((VASigCookie**)(this + 1));
        }

        int GetFrameType()
        {
            return TYPE_CALL;
        }

        virtual BOOL IsFramedMethodFrame() { return TRUE; }

    protected:
        // For use by classes deriving from FramedMethodFrame.
        void PromoteCallerStack(promote_func* fn, ScanContext* sc)
        {
            PromoteCallerStackWorker(fn, sc, FALSE);
        }

        // For use by classes deriving from FramedMethodFrame.
        void PromoteCallerStackWithPinning(promote_func* fn, ScanContext* sc)
        {
            PromoteCallerStackWorker(fn, sc, TRUE);
        }

        void UnwindSynchronized();

                // Helper for ComPlus and NDirect method calls that are implemented via
                // compiled stubs. This function retrieves the stub (after unwrapping
                // interceptors) and asks it for the stack count computed by the stublinker.
                void AskStubForUnmanagedCallSite(void **ip,
                                         void **returnIP, void **returnSP);

#ifdef _PPC_
    public:
        // deregister the arguments
        static void Deregister(BYTE* pFrameBase, MetaSig* pSig, BOOL fIsStatic, UINT nActualStackBytes) {
            RegisterWorker(pFrameBase, pSig, fIsStatic, nActualStackBytes, FALSE);
        }

        // enregister the argument
        static void Enregister(BYTE* pFrameBase, MetaSig* pSig, BOOL fIsStatic, UINT nActualStackBytes) {
            RegisterWorker(pFrameBase, pSig, fIsStatic, nActualStackBytes, TRUE);
        }

        // this is called from the generated assembly code
        static void RegisterHelper(FramedMethodFrame* pThis, BOOL fEnregister);

        static void RegisterWorker(BYTE* pFrameBase, MetaSig* pSig, BOOL fIsStatic, 
            UINT nActualStackBytes, BOOL fEnregister);
#endif // _PPC_


    private:
        // For use by classes deriving from FramedMethodFrame.
        void PromoteCallerStackWorker(promote_func* fn, ScanContext* sc, BOOL fPinArrays);

        void PromoteCallerStackHelper(promote_func* fn, ScanContext* sc, BOOL fPinArrays,
            ArgIterator *pargit, MetaSig *pmsig);


        // Keep as last entry in class
        DEFINE_VTABLE_GETTER_AND_CTOR(FramedMethodFrame)
};

//----------------------------------------------------------------------
// The layout of the stub stackframe
//----------------------------------------------------------------------
#if defined(_PPC_)
struct StubStackFrame {
    LinkageArea             link;
    INT32                   params[8]; // max 8 local variables or parameters passed down
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
//+----------------------------------------------------------------------------
class TPMethodFrame : public FramedMethodFrame
{
    public:
        virtual int GetFrameType()
        {
            return TYPE_TP_METHOD_FRAME;
        }
        
        // GC protect arguments
        virtual void GcScanRoots(promote_func *fn, ScanContext* sc);

        // Return only a valid Method Descriptor
        virtual MethodDesc *GetFunction();

        // For proxy calls m_Datum contains the number of stack bytes containing arguments.
        void SetFunction(void *pMD)
        {
            m_Datum = pMD;
        }

        // Return value is stored here
        Object *&GetReturnObject()
        {
            Object *&pReturn = *(Object **) (((BYTE *) this) - GetNegSpaceSize() - sizeof(INT64));
            // This assert is too strong, it does not work for byref returns!
            _ASSERTE(pReturn == NULL || pReturn->GetMethodTable()->GetClass());
            return(pReturn);
        }

        // Get return value address
        virtual INT64 *GetReturnValuePtr()
        {
            return (INT64*) (((BYTE *) this) - GetNegSpaceSize() - sizeof(INT64));
        }

        // Get slot number on which we were called
        INT32 GetSlotNumber()
        {
            return GetSlotNumber(m_Datum);
        }

        static INT32 GetSlotNumber(PVOID MDorSlot)
        {

            if(( ((size_t)MDorSlot) & ~0xFFFF) == 0)
            {
                // The slot number was pushed on the stack
                return (INT32)(size_t)MDorSlot;
            }
            else
            {
                // The method descriptor was pushed on the stack
                return -1;
            }
        }

        // Get offset used during stub generation
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(TPMethodFrame);
        }

        // Aid the debugger in finding the actual address of callee
        virtual BOOL TraceFrame(Thread *thread, BOOL fromPatch,
                                TraceDestination *trace, REGDISPLAY *regs);

        // Keep as last entry in class
        DEFINE_VTABLE_GETTER_AND_CTOR(TPMethodFrame)
};




//------------------------------------------------------------------------
// This represents a call to a NDirect method.
//------------------------------------------------------------------------
class NDirectMethodFrame : public FramedMethodFrame
{
    public:

        virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
        {
            FramedMethodFrame::GcScanRoots(fn, sc);
            PromoteCallerStackWithPinning(fn, sc);
            if (GetCleanupWorkList())
            {
                GetCleanupWorkList()->GcScanRoots(fn, sc);
            }

        }

        virtual BOOL IsTransitionToNativeFrame()
        {
            return TRUE;
        }

        virtual CleanupWorkList *GetCleanupWorkList()
        {
            return NULL;
        }

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------

        int GetFrameType()
        {
            return TYPE_EXIT;
        };

        void GetUnmanagedCallSite(void **ip,
                                  void **returnIP, void **returnSP) = 0;

        BOOL TraceFrame(Thread *thread, BOOL fromPatch,
                        TraceDestination *trace, REGDISPLAY *regs);

        friend struct MEMBER_OFFSET_INFO(NDirectMethodFrame);
};






//------------------------------------------------------------------------
// This represents a call to a NDirect method with cleanup.
//------------------------------------------------------------------------
class NDirectMethodFrameEx : public NDirectMethodFrame
{
    public:

        //------------------------------------------------------------------------
        // Performs cleanup on an exception unwind
        //------------------------------------------------------------------------
        virtual void ExceptionUnwind()
        {
            NDirectMethodFrame::ExceptionUnwind();
            GetCleanupWorkList()->Cleanup(TRUE);
        }


        //------------------------------------------------------------------------
        // Gets the cleanup worklist for this method call.
        //------------------------------------------------------------------------
        virtual CleanupWorkList *GetCleanupWorkList()
        {
            return (CleanupWorkList*)( ((LPBYTE)this) + GetOffsetOfCleanupWorkList() );
        }

                static INT GetOffsetOfCleanupWorkList()
                {
                        return 0 - GetNegSpaceSize();
                }

        // This frame must store some fields at negative offset.
        // This method exposes the size for people needing to allocate
        // TransitionFrames.
        static UINT32 GetNegSpaceSize()
        {
            return PLATFORM_FRAME_ALIGN(FramedMethodFrame::GetNegSpaceSize() + sizeof(CleanupWorkList));
        }

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------

        void GetUnmanagedCallSite(void **ip,
                                  void **returnIP, void **returnSP) = 0;

    friend struct MEMBER_OFFSET_INFO(NDirectMethodFrameEx);
};

//------------------------------------------------------------------------
// This represents a call to a NDirect method with the generic worker
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameGeneric : public NDirectMethodFrameEx
{
    public:
        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(NDirectMethodFrameGeneric);
        }

        void GetUnmanagedCallSite(void **ip,
                                  void **returnIP, void **returnSP);

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(NDirectMethodFrameGeneric)
};



#ifdef _X86_

//------------------------------------------------------------------------
// This represents a call to a NDirect method with the slimstub
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameSlim : public NDirectMethodFrameEx
{
    public:
        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(NDirectMethodFrameSlim);
        }

        void GetUnmanagedCallSite(void **ip,
                                  void **returnIP, void **returnSP);

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(NDirectMethodFrameSlim)
};




//------------------------------------------------------------------------
// This represents a call to a NDirect method with the standalone stub (no cleanup)
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameStandalone : public NDirectMethodFrame
{
    public:
        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(NDirectMethodFrameStandalone);
        }

        void GetUnmanagedCallSite(void **ip,
                                  void **returnIP, void **returnSP)
                {
                        AskStubForUnmanagedCallSite(ip, returnIP, returnSP);
                }

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(NDirectMethodFrameStandalone)
};



//------------------------------------------------------------------------
// This represents a call to a NDirect method with the standalone stub (with cleanup)
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameStandaloneCleanup : public NDirectMethodFrameEx
{
    public:
        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(NDirectMethodFrameStandaloneCleanup);
        }

        void GetUnmanagedCallSite(void **ip,
                                  void **returnIP, void **returnSP)
                {
                        AskStubForUnmanagedCallSite(ip, returnIP, returnSP);
                }

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(NDirectMethodFrameStandaloneCleanup)
};

#endif // _X86_


//------------------------------------------------------------------------
// This represents a call Multicast.Invoke. It's only used to gc-protect
// the arguments during the iteration.
//------------------------------------------------------------------------
class MulticastFrame : public FramedMethodFrame
{
    public:
        virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
        {
            FramedMethodFrame::GcScanRoots(fn, sc);
            PromoteCallerStack(fn, sc);
        }

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(MulticastFrame);
        }


        int GetFrameType()
        {
            return TYPE_MULTICAST;
        }

        virtual BOOL TraceFrame(Thread *thread, BOOL fromPatch,
                                TraceDestination *trace, REGDISPLAY *regs);

        Stub *AscertainMCDStubness(BYTE *pbAddr);

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(MulticastFrame)
};




//-----------------------------------------------------------------------
// Transition frame from unmanaged to managed
//-----------------------------------------------------------------------
class UnmanagedToManagedFrame : public Frame
{
public:

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

    // Retrives pointer to the lowest-addressed argument on
    // the stack. Depending on the calling convention, this
    // may or may not be the first argument.
    LPVOID GetPointerToArguments()
    {
        return (LPVOID)(this + 1);
    }

    // Exposes an offset for stub generation.
    static BYTE GetOffsetOfArgs()
    {
        size_t ofs = sizeof(UnmanagedToManagedFrame);
        _ASSERTE(FitsInI1(ofs));
        return (BYTE)ofs;
    }

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static BYTE GetOffsetOfReturnAddress()
    {
#ifdef _PPC_
        size_t ofs = offsetof(class UnmanagedToManagedFrame, m_Link.SavedLR);
#else
        size_t ofs = offsetof(class UnmanagedToManagedFrame, m_ReturnAddress);
#endif
        _ASSERTE(FitsInI1(ofs));
        return (BYTE)ofs;
    }

    // depends on the sub frames to return approp. type here
    virtual LPVOID GetDatum()
    {
        return m_pvDatum;
    }

    static UINT GetOffsetOfDatum()
    {
        return (UINT)offsetof(class UnmanagedToManagedFrame, m_pvDatum);
    }

    int GetFrameType()
    {
        return TYPE_ENTRY;
    };

    virtual const BYTE* GetManagedTarget()
    {
        return NULL;
    }

    // Return the # of stack bytes pushed by the unmanaged caller.
    virtual UINT GetNumCallerStackBytes() = 0;

     virtual void UpdateRegDisplay(const PREGDISPLAY);

protected:
    LPVOID    m_pvDatum;       // type depends on the sub class

#ifdef _PPC_
    LinkageArea m_Link;
#else
    LPVOID      m_ReturnAddress;  // return address into unmanaged code
#endif

    friend struct MEMBER_OFFSET_INFO(UnmanagedToManagedFrame);
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
    struct NegInfo
    {
        CleanupWorkList m_List;
        Context *m_pReturnContext;
        LPVOID      m_pArgs;
        ULONG       m_fGCEnabled;
        Marshaler  *m____NOLONGERUSED____; // marshaler structures that want to be notified of GC promotes
    };

    // Return the # of stack bytes pushed by the unmanaged caller.
    virtual UINT GetNumCallerStackBytes()
    {
        return 0;
    }


    // Should managed exceptions be passed thru?
    virtual BOOL CatchManagedExceptions()
    {
        return FALSE;
    }

    // Convert a thrown COM+ exception to an unmanaged result.
    virtual UINT32 ConvertComPlusException(OBJECTREF pException)
    {
        return 0;
    }

    //------------------------------------------------------------------------
    // Performs cleanup on an exception unwind
    //------------------------------------------------------------------------
    virtual void ExceptionUnwind();

    // ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING
    //
    // !! We shave some cycles in ComToComPlusWorker() & ComToComPlusSimpleWorker()
    // !! by asserting that we are operating on a ComMethodFrame and then we
    // !! bypass virtualization.  Regrettably, this means that there are some
    // !! non-virtual implementations of the following three methods in ComMethodFrame.
    // !!
    // !! If you edit the following 3 methods, please propagate your changes to
    // !! those implementations, also.
    //
    // ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING

    virtual CleanupWorkList *GetCleanupWorkList()
    {
        return &GetNegInfo()->m_List;
    }
    virtual NegInfo* GetNegInfo()
    {
        return (NegInfo*)( ((LPBYTE)this) - GetNegSpaceSize());
    }
    virtual ULONG* GetGCInfoFlagPtr()
    {
        return &GetNegInfo()->m_fGCEnabled;
    }

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc);

    virtual Context **GetReturnContextAddr()
    {
        return &(GetNegInfo()->m_pReturnContext);
    }

    // ********************** END OF WARNING *************************



    virtual LPVOID GetPointerToDstArgs()
    {
        return GetNegInfo()->m_pArgs;
    }

    virtual void SetDstArgsPointer(LPVOID pv)
    {
        _ASSERTE(pv != NULL);
        GetNegInfo()->m_pArgs = pv;
    }


    // UnmanagedToManagedCallFrames must store some fields at negative offset.
    // This method exposes the size
    static UINT32 GetNegSpaceSize()
    {
        UINT32 temp = sizeof (NegInfo)
#if defined(_X86_)
            + sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE
#endif
            ;
        return PLATFORM_FRAME_ALIGN(temp);
    }

    friend struct MEMBER_OFFSET_INFO(UnmanagedToManagedCallFrame);
};

#if defined(_X86_)
struct UMStubStackFrame {
    EXCEPTION_REGISTRATION_RECORD   exReg;

    // scratch area - corresponds to FrameHandlerExRecord::m_tct
    ThrowCallbackType      tct;

    UnmanagedToManagedCallFrame::NegInfo info;

#ifdef _DEBUG
    // VC trace info
    //  push eax        ;; push return address
    //  push ebp        ;; push previous ebp
    INT32                   vcJunk1;
    INT32                   vcJunk2;
#endif

    CalleeSavedRegisters    savedregs;

    UnmanagedToManagedCallFrame umframe; // LinkageArea is allocated by caller!
};
#elif defined(_PPC_)
struct UMStubStackFrame {
    LinkageArea             link;
    INT32                   params[2]; // max 2 parameters passed down

    EXCEPTION_REGISTRATION_RECORD exReg;

    // scratch area - corresponds to FrameHandlerExRecord::m_tct
    ThrowCallbackType       tct;

    ArgumentRegisters       argregs;
    CalleeSavedRegisters    savedregs;
    UnmanagedToManagedCallFrame::NegInfo info;
    UnmanagedToManagedCallFrame umframe; // LinkageArea is allocated by caller!
};
#else
// define one if it makes sense for the platform 
#endif




//------------------------------------------------------------------------
// This represents a declarative secuirty check. This frame is inserted
// prior to calls on methods that have declarative security defined for
// the class or the specific method that is being called. This frame
// is only created when the prestubworker creates a real stub.
//------------------------------------------------------------------------
class SecurityFrame : public FramedMethodFrame
{
public:
        struct ISecurityState {
            // Additional field for referencing per-frame security information.
            // This field is not necessary for most frames, so it is a costly
            // addition for all frames. Leave it here for M3 after which we can
            // be smarter about when to insert this extra field. This field should
            // not always be added to the negative offset
            OBJECTREF   m_securityData;
        };

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(SecurityFrame);
        }

        //-----------------------------------------------------------
        // Returns the address of the frame security descriptor ref
        //-----------------------------------------------------------

        virtual OBJECTREF *GetAddrOfSecurityDesc()
        {
            return & GetISecurityState()->m_securityData;
        }

        static UINT32 GetNegSpaceSize()
        {
            return PLATFORM_FRAME_ALIGN(FramedMethodFrame::GetNegSpaceSize() + sizeof(ISecurityState));
        }

        VOID GcScanRoots(promote_func *fn, ScanContext* sc);

        BOOL TraceFrame(Thread *thread, BOOL fromPatch,
                        TraceDestination *trace, REGDISPLAY *regs);

        int GetFrameType()
        {
            return TYPE_SECURITY;
        }

    private:
        ISecurityState *GetISecurityState()
        {
            return (ISecurityState*)( ((BYTE*)this) - GetNegSpaceSize() );
        }

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(SecurityFrame)
};


//------------------------------------------------------------------------
// This represents a call to a method prestub. Because the prestub
// can do gc and throw exceptions while building the replacement
// stub, we need this frame to keep things straight.
//------------------------------------------------------------------------
class PrestubMethodFrame : public FramedMethodFrame
{
    public:
        virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
        {
            FramedMethodFrame::GcScanRoots(fn, sc);
            PromoteCallerStack(fn, sc);
        }


        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(PrestubMethodFrame);
        }

        BOOL TraceFrame(Thread *thread, BOOL fromPatch,
                        TraceDestination *trace, REGDISPLAY *regs);

        int GetFrameType()
        {
            return TYPE_INTERCEPTION;
        }

        Interception GetInterception();

        // Link this frame, setting the vptr
        VOID Push();

    private:
    friend const BYTE * __stdcall PreStubWorker(PrestubMethodFrame *pPFrame);

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(PrestubMethodFrame)
};

//------------------------------------------------------------------------
// This represents a call to a method prestub. Because the prestub
// can do gc and throw exceptions while building the replacement
// stub, we need this frame to keep things straight.
//------------------------------------------------------------------------
class InterceptorFrame : public SecurityFrame
{
    public:
        virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
        {
            SecurityFrame::GcScanRoots(fn, sc);
            PromoteCallerStack(fn, sc);
        }


        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetMethodFrameVPtr()
        {
            RETURNFRAMEVPTR(InterceptorFrame);
        }

        Interception GetInterception();

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(InterceptorFrame)
};




//------------------------------------------------------------------------
// These macros GC-protect OBJECTREF pointers on the EE's behalf.
// In between these macros, the GC can move but not discard the protected
// objects. If the GC moves an object, it will update the guarded OBJECTREF's.
// Typical usage:
//
//   OBJECTREF or = <some valid objectref>;
//   GCPROTECT_BEGIN(or);
//
//   ...<do work that can trigger GC>...
//
//   GCPROTECT_END();
//
//
// These macros can also protect multiple OBJECTREF's if they're packaged
// into a structure:
//
//   struct xx {
//      OBJECTREF o1;
//      OBJECTREF o2;
//   } gc;
//
//   GCPROTECT_BEGIN(gc);
//   ....
//   GCPROTECT_END();
//
//
// Notes:
//
//   - GCPROTECT_BEGININTERIOR() can be used in place of GCPROTECT_BEGIN()
//     to handle the case where one or more of the OBJECTREFs is potentially
//     an interior pointer.  This is a rare situation, because boxing would
//     normally prevent us from encountering it.  Be aware that the OBJECTREFs
//     we protect are not validated in this situation.
//
//   - GCPROTECT_ARRAY_BEGIN() can be used when an array of object references
//     is allocated on the stack.  The pointer to the first element is passed
//     along with the number of elements in the array.
//
//   - The argument to GCPROTECT_BEGIN should be an lvalue because it
//     uses "sizeof" to count the OBJECTREF's.
//
//   - GCPROTECT_BEGIN spiritually violates our normal convention of not passing
//     non-const refernce arguments. Unfortunately, this is necessary in
//     order for the sizeof thing to work.
//
//   - GCPROTECT_BEGIN does _not_ zero out the OBJECTREF's. You must have
//     valid OBJECTREF's when you invoke this macro.
//
//   - GCPROTECT_BEGIN begins a new C nesting block. Besides allowing
//     GCPROTECT_BEGIN's to nest, it also has the advantage of causing
//     a compiler error if you forget to code a maching GCPROTECT_END.
//
//   - If you are GCPROTECTing something, it means you are expecting a GC to occur.
//     So we assert that GC is not forbidden. If you hit this assert, you probably need
//     a HELPER_METHOD_FRAME to protect the region that can cause the GC.
//------------------------------------------------------------------------
#define GCPROTECT_BEGIN(ObjRefStruct)           do {            \
                GCFrame __gcframe((OBJECTREF*)&(ObjRefStruct),  \
                sizeof(ObjRefStruct)/sizeof(OBJECTREF),         \
                FALSE);                                         \
                _ASSERTE(!GetThread()->GCForbidden());          \
                DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK

#define GCPROTECT_ARRAY_BEGIN(ObjRefArray,cnt) do {             \
                GCFrame __gcframe((OBJECTREF*)&(ObjRefArray),   \
                cnt,                                            \
                FALSE);                                         \
                _ASSERTE(!GetThread()->GCForbidden());          \
                DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK

#define GCPROTECT_BEGININTERIOR(ObjRefStruct)           do {    \
                GCFrame __gcframe((OBJECTREF*)&(ObjRefStruct),  \
                sizeof(ObjRefStruct)/sizeof(OBJECTREF),         \
                TRUE);                                          \
                _ASSERTE(!GetThread()->GCForbidden());          \
                DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK

#define GCPROTECT_END()                     __gcframe.Pop(); } while(0)

//------------------------------------------------------------------------
// This frame protects object references for the EE's convenience.
// This frame type actually is created from C++.
//------------------------------------------------------------------------
class GCFrame : public Frame
{
    public:


        //--------------------------------------------------------------------
        // This constructor pushes a new GCFrame on the frame chain.
        //--------------------------------------------------------------------
        GCFrame() { };
        GCFrame(OBJECTREF *pObjRefs, UINT numObjRefs, BOOL maybeInterior);
        void Init(Thread *pThread, OBJECTREF *pObjRefs, UINT numObjRefs, BOOL maybeInterior);


        //--------------------------------------------------------------------
        // Pops the GCFrame and cancels the GC protection. Also
        // trashes the contents of pObjRef's in _DEBUG.
        //--------------------------------------------------------------------
        VOID Pop();

        virtual void GcScanRoots(promote_func *fn, ScanContext* sc);

#ifdef _DEBUG
        virtual BOOL Protects(OBJECTREF *ppORef)
        {
            for (UINT i = 0; i < m_numObjRefs; i++) {
                if (ppORef == m_pObjRefs + i) {
                    return TRUE;
                }
            }
            return FALSE;
        }
#endif

    private:
        OBJECTREF *m_pObjRefs;
        UINT       m_numObjRefs;
        Thread    *m_pCurThread;
        BOOL       m_MaybeInterior;

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(GCFrame)
};

struct ByRefInfo
{
    ByRefInfo *pNext;
    INT32      argIndex;
    CorElementType typ;
    EEClass   *pClass;
    char       data[1];
};

class ProtectByRefsFrame : public Frame
{
public:
    ProtectByRefsFrame(Thread *pThread, ByRefInfo *brInfo);
    void Pop();

    virtual void GcScanRoots(promote_func *fn, ScanContext *sc);

private:
    ByRefInfo *m_brInfo;
    Thread    *m_pThread;

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(ProtectByRefsFrame)
};

struct ValueClassInfo
{
    ValueClassInfo  *pNext;
    INT32           argIndex;
    CorElementType  typ;
    EEClass         *pClass;
    LPVOID          pData;
};

class ProtectValueClassFrame : public Frame
{
public:
    ProtectValueClassFrame(Thread *pThread, ValueClassInfo *vcInfo);
    void Pop();

    static void PromoteValueClassEmbeddedObjects(promote_func *fn, ScanContext *sc,
                                          EEClass *pClass, PVOID pvObject);    
    virtual void GcScanRoots(promote_func *fn, ScanContext *sc);

private:

    ValueClassInfo *m_pVCInfo;
    Thread    *m_pThread;

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(ProtectValueClassFrame)
};


#ifdef _DEBUG
BOOL IsProtectedByGCFrame(OBJECTREF *ppObjectRef);
#endif













void DoPromote(promote_func *fn, ScanContext* sc, OBJECTREF *address, BOOL interior);





//------------------------------------------------------------------------
// DebuggerClassInitMarkFrame is a small frame whose only purpose in
// life is to mark for the debugger that "class initialization code" is
// being run. It does nothing useful except return good values from
// GetFrameType and GetInterception.
//------------------------------------------------------------------------
class DebuggerClassInitMarkFrame : public Frame
{
public:
    DebuggerClassInitMarkFrame()
    {
        Push();
    };

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
    {
        // Nothing to do here.
    }

    virtual int GetFrameType()
    {
        return TYPE_INTERCEPTION;
    }

    virtual Interception GetInterception()
    {
        return INTERCEPTION_CLASS_INIT;
    }

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(DebuggerClassInitMarkFrame)
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
    DebuggerSecurityCodeMarkFrame()
    {
        Push();
    };

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
    {
        // Nothing to do here.
    }

    virtual int GetFrameType()
    {
        return TYPE_INTERCEPTION;
    }

    virtual Interception GetInterception()
    {
        return INTERCEPTION_SECURITY;
    }

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(DebuggerSecurityCodeMarkFrame)
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
    DebuggerExitFrame()
    {
        Push();
    };

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
    {
        // Nothing to do here.
    }

    virtual int GetFrameType()
    {
        return TYPE_EXIT;
    }

    // Return information about an unmanaged call the frame
    // will make.
    // ip - the unmanaged routine which will be called
    // returnIP - the address in the stub which the unmanaged routine
    //            will return to.
    // returnSP - the location returnIP is pushed onto the stack
    //            during the call.
    //
    virtual void GetUnmanagedCallSite(void **ip,
                                      void **returnIP,
                                      void **returnSP)
    {
        if (ip)
            *ip = NULL;

        if (returnIP)
            *returnIP = NULL;

        if (returnSP)
            *returnSP = NULL;
    }

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(DebuggerExitFrame)
};




//------------------------------------------------------------------------
// This frame guards an unmanaged->managed transition thru a UMThk
//------------------------------------------------------------------------
class UMThkCallFrame : public UnmanagedToManagedCallFrame
{
    friend class UMThunkStubCache;

    public:

        virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
        {
            PromoteCalleeStack(fn, sc);
            UnmanagedToManagedCallFrame::GcScanRoots(fn,sc);
        }

        // promote callee stack, if we are the topmost frame
        void PromoteCalleeStack(promote_func *fn, ScanContext* sc);


        // Manipulate the GCArgsprotection enable bit. For us, it's just a simple boolean!
        BOOL GCArgsProtectionOn()
        {
            return !!*(GetGCInfoFlagPtr());
        }

        VOID SetGCArgsProtectionState(BOOL fEnable)
        {
            *(GetGCInfoFlagPtr()) = !!fEnable;
        }

        // used by PromoteCalleeStack to get the destination function sig and module
        // NOTE: PromoteCalleeStack only promotes bona-fide arguments, and not
        // the "this" reference. The whole purpose of PromoteCalleeStack is
        // to protect the partially constructed argument array during
        // the actual process of argument marshaling.
        virtual PCCOR_SIGNATURE GetTargetCallSig();
        virtual Module *GetTargetModule();

        // Return the # of stack bytes pushed by the unmanaged caller.
        UINT GetNumCallerStackBytes();


        // Should managed exceptions be passed thru?
        virtual BOOL CatchManagedExceptions()
        {
            return FALSE;
        }
    

        // Convert a thrown COM+ exception to an unmanaged result.
        virtual UINT32 ConvertComPlusException(OBJECTREF pException);


        UMEntryThunk *GetUMEntryThunk()
        {
            return (UMEntryThunk*)GetDatum();
        }


        static UINT GetOffsetOfUMEntryThunk()
        {
            return GetOffsetOfDatum();
        }

        const BYTE* GetManagedTarget();

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        static LPVOID GetUMThkCallFrameVPtr()
        {
            RETURNFRAMEVPTR(UMThkCallFrame);
        }

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER_AND_CTOR(UMThkCallFrame)
};





//------------------------------------------------------------------------
// This frame is used to mark a Context/AppDomain Transition
//------------------------------------------------------------------------
class ContextTransitionFrame : public Frame
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // WARNING: If you change this structure, you must also change
    // System.Runtime.Remoting.ContextTransitionFrame to match it.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // exRecord field must always go first
    EXCEPTION_REGISTRATION_RECORD exRecord;

    // to preserve the state between CPFH_RealFirstPassHandler and COMPlusAfterUnwind    
    ThrowCallbackType m_tct;

    Context *m_pReturnContext;
    Object *m_ReturnLogicalCallContext;
    Object *m_ReturnIllogicalCallContext;
    ULONG_PTR m_ReturnWin32Context;

public:

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc);

    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ContextTransitionFrame);
    }

    virtual Context **GetReturnContextAddr()
    {
        return &m_pReturnContext;
    }

    virtual Object **GetReturnLogicalCallContextAddr()
    {
        return (Object **) &m_ReturnLogicalCallContext;
    }

    virtual Object **GetReturnIllogicalCallContextAddr()
    {
        return (Object **) &m_ReturnIllogicalCallContext;
    }

    virtual ULONG_PTR* GetWin32ContextAddr()
    {
        return &m_ReturnWin32Context;
    }

    virtual void ExceptionUnwind();

        // Install an EH handler so we can unwind properly
    void InstallExceptionHandler();

    void UninstallExceptionHandler();

    static inline ContextTransitionFrame* CurrFrame(EXCEPTION_REGISTRATION_RECORD *pRec) {
        return (ContextTransitionFrame*)((char *)pRec - offsetof(ContextTransitionFrame, exRecord));
    }

    ContextTransitionFrame() {}

    // Keep as last entry in class
    DEFINE_VTABLE_GETTER(ContextTransitionFrame)
};

#define DECLARE_ALLOCA_CONTEXT_TRANSITION_FRAME(pFrame) \
    void *_pBlob = _alloca(sizeof(ContextTransitionFrame)); \
    ContextTransitionFrame *pFrame = new (_pBlob) ContextTransitionFrame();

INDEBUG(bool isLegalManagedCodeCaller(void* retAddr));
bool isRetAddr(size_t retAddr, size_t* whereCalled);


#endif  //__frames_h__
