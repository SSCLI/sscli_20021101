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
/*=============================================================================
**
** Class: Thread
**
**                                             
**
** Purpose: Class for creating and managing a thread.
**
** Date: April 1, 1998
**
=============================================================================*/

namespace System.Threading {
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Contexts;    
    using System.Runtime.Remoting.Messaging;
    using System;
    using System.Diagnostics;
    using System.Security.Permissions;
    using System.Security.Principal;
    using System.Globalization;
    using System.Collections;
    using System.Runtime.Serialization;
	 using System.Runtime.CompilerServices;
    using System.Security;
        
    /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread"]/*' />
    // deliberately not [serializable]
    public sealed class Thread
    {   
        /*=========================================================================
        ** Data accessed from managed code that needs to be defined in 
        ** ThreadBaseObject to maintain alignment between the two classes.
        ** DON'T CHANGE THESE UNLESS YOU MODIFY ThreadBaseObject in vm\object.h
        =========================================================================*/
        private Context            m_Context;
        private LogicalCallContext m_LogicalCallContext;      // this call context follows the logical thread
        private IllogicalCallContext m_IllogicalCallContext;  // this call context follows the physical thread
        private String m_Name;
        private Object m_ExceptionStateInfo;          // Excep. Info. latched to the thread on a thread abort
        private Delegate m_Delegate;                // Delegate
        private LocalDataStoreSlot m_PrincipalSlot;
        private Object[] m_ThreadStatics;           // Holder for thread statics
        private CultureInfo        m_CurrentCulture;
        private CultureInfo        m_CurrentUICulture;

        /*=========================================================================
        ** The base implementation of Thread is all native.  The following fields
        ** should never be used in the C# code.  They are here to define the proper
        ** space so the thread object may be allocated.  DON'T CHANGE THESE UNLESS
        ** YOU MODIFY ThreadBaseObject in vm\object.h
        **                                             
        =========================================================================*/
        private IntPtr DONT_USE_InternalThread;        // Pointer
        private int m_Priority;                     // INT32
        
        /*=========================================================================
        ** This manager is responsible for storing the global data that is 
        ** shared amongst all the thread local stores.
        =========================================================================*/
        static private LocalDataStoreMgr m_LocalDataStoreMgr = new LocalDataStoreMgr();
        private const int STATICS_BUCKET_SIZE           = 0x8;
        
        // Thread statics stuff
        private int m_ThreadStaticsCurrentBucket;
        private int m_ThreadStaticsFreeIndex;
        
        /*=========================================================================
        ** Creates a new Thread object which will begin execution at
        ** start.ThreadStart on a new thread when the Start method is called.
        **
        ** Exceptions: ArgumentNullException if start == null.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Thread"]/*' />
        public Thread(ThreadStart start) {
            if (start == null) {
                throw new ArgumentNullException("start");
            }
            SetStart(start);
        }
    
        /*=========================================================================
        ** Spawns off a new thread which will begin executing at the ThreadStart
        ** method on the IThreadable interface passed in the constructor. Once the
        ** thread is dead, it cannot be restarted with another call to Start.
        **
        ** Exceptions: ThreadStateException if the thread has already been started.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Start"]/*' />
        public void Start()
        {
            // Attach current thread's security principal object to the new
            // thread. Be careful not to bind the current thread to a principal
            // if it's not already bound.
            IPrincipal principal = (IPrincipal) CallContext.SecurityData.Principal;
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            StartInternal(principal, ref stackMark);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void StartInternal(IPrincipal principal, ref StackCrawlMark stackMark);
    

        /*=========================================================================
        ** Raises a ThreadAbortException in the thread, which usually
        ** results in the thread's death. The ThreadAbortException is a special
        ** exception that is not catchable. The finally clauses of all try 
        ** statements will be executed before the thread dies. This includes the
        ** finally that a thread might be executing at the moment the Abort is raised.   
        ** The thread is not stopped immediately--you must Join on the
        ** thread to guarantee it has stopped. 
        ** It is possible for a thread to do an unbounded amount of computation in 
        ** the finally's and thus indefinitely delay the threads death.
        ** If Abort() is called on a thread that has not been started, the thread 
        ** will abort when Start() is called.
        ** If Abort is called twice on the same thread, a DuplicateThreadAbort 
        ** exception is thrown. 
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Abort"]/*' />

        // [SecurityPermissionAttribute(SecurityAction.Demand, ControlThread=true)]
        // This security demand is done in the call to Abort()
        public void Abort(Object stateInfo)
        {
            // if two aborts come at the same time, it is possible that the state info
            // gets set by one, and the actual abort gets delivered by another. But this
            // is not distinguishable by an application.
            if (ExceptionState == null)
                ExceptionState = stateInfo;
            Abort();
        }

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Abort1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, ControlThread=true)]
        public void Abort() { AbortInternal(); }

        // Internal helper (since we can't place security demands on
        // ecalls/fcalls).
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void AbortInternal();

        /*=========================================================================
        ** Resets a thread abort.
        ** Should be called by trusted code only
          =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.ResetAbort"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, ControlThread=true)]
        public static void ResetAbort()
        {
            Thread thread = Thread.CurrentThread;
            if ((thread.ThreadState & ThreadState.AbortRequested) == 0)
                throw new ThreadStateException(Environment.GetResourceString("ThreadState_NoAbortRequested"));
            thread.ResetAbortNative();
            thread.ExceptionState = null;
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void ResetAbortNative();

        /*=========================================================================
        ** Suspends the thread. If the thread is already suspended, this call has
        ** no effect.
        **
        ** Exceptions: ThreadStateException if the thread has not been started or
        **             it is dead.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Suspend"]/*' />
        [SecurityPermission(SecurityAction.Demand, ControlThread=true)]
        public void Suspend() { SuspendInternal(); }

        // Internal helper (since we can't place security demands on
        // ecalls/fcalls).
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void SuspendInternal();
    
        /*=========================================================================
        ** Resumes a thread that has been suspended.
        **
        ** Exceptions: ThreadStateException if the thread has not been started or
        **             it is dead or it isn't in the suspended state.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Resume"]/*' />
        [SecurityPermission(SecurityAction.Demand, ControlThread=true)]
        public void Resume() { ResumeInternal(); }
    
        // Internal helper (since we can't place security demands on
        // ecalls/fcalls).
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void ResumeInternal();
    
        /*=========================================================================
        ** Interrupts a thread that is inside a Wait(), Sleep() or Join().  If that
        ** thread is not currently blocked in that manner, it will be interrupted
        ** when it next begins to block.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Interrupt"]/*' />
        [SecurityPermission(SecurityAction.Demand, ControlThread=true)]
        public void Interrupt() { InterruptInternal(); }
    
        // Internal helper (since we can't place security demands on
        // ecalls/fcalls).
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void InterruptInternal();
    
        /*=========================================================================
        ** Returns the priority of the thread.
        **
        ** Exceptions: ThreadStateException if the thread is dead.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Priority"]/*' />
        public ThreadPriority Priority {
            get { return (ThreadPriority)GetPriorityNative(); }
            set { SetPriorityNative((int)value); }
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int GetPriorityNative();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void SetPriorityNative(int priority);
    
        /*=========================================================================
        ** Returns true if the thread has been started and is not dead.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.IsAlive"]/*' />
        public bool IsAlive {
            get { return IsAliveNative(); }
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern bool IsAliveNative();
    
        /*=========================================================================
        ** Returns true if the thread is a threadpool thread.
        =========================================================================*/
        public bool IsThreadPoolThread {
			get { return IsThreadpoolThreadNative();  }
		}
         
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern bool IsThreadpoolThreadNative();
       
        /*=========================================================================
        ** Waits for the thread to die.
        ** 
        ** Exceptions: ThreadInterruptedException if the thread is interrupted while waiting.
        **             ThreadStateException if the thread has not been started yet.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Join"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void Join();
    
        /*=========================================================================
        ** Waits for the thread to die or for timeout milliseconds to elapse.
        ** Returns true if the thread died, or false if the wait timed out. If
        ** Timeout.Infinite is given as the parameter, no timeout will occur.
        ** 
        ** Exceptions: ArgumentException if timeout < 0.
        **             ThreadInterruptedException if the thread is interrupted while waiting.
        **             ThreadStateException if the thread has not been started yet.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Join1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern bool Join(int millisecondsTimeout);

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Join2"]/*' />
        public bool Join(TimeSpan timeout)
        {
            long tm = (long)timeout.TotalMilliseconds;
            if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));

            return Join((int)tm);
        }
    
        /*=========================================================================
        ** Suspends the current thread for timeout milliseconds. If timeout == 0,
        ** forces the thread to give up the remainer of its timeslice.  If timeout
        ** == Timeout.Infinite, no timeout will occur.
        **
        ** Exceptions: ArgumentException if timeout < 0.
        **             ThreadInterruptedException if the thread is interrupted while sleeping.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Sleep"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void Sleep(int millisecondsTimeout);

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Sleep1"]/*' />
        public static void Sleep(TimeSpan timeout)
        {
            long tm = (long)timeout.TotalMilliseconds;
            if (tm < -1 || tm > (long) Int32.MaxValue)
                throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
            Sleep((int)tm);
        }
    

		/* wait for a length of time proportial to 'iterations'.  Each iteration is should
		   only take a few machine instructions.  Calling this API is preferable to coding 
		   a explict busy loop because the hardware can be informed that it is busy waiting. */

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.SpinWait"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void SpinWait(int iterations);


        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.CurrentThread"]/*' />
        public static Thread CurrentThread {
            get {
                Thread th;
                th = GetFastCurrentThreadNative();
                if (th == null)
                    th = GetCurrentThreadNative();
                return th;
            }
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Thread GetCurrentThreadNative();
         [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Thread GetFastCurrentThreadNative();
    
        /*=========================================================================
        ** PRIVATE Sets the IThreadable interface for the thread. Assumes that
        ** start != null.
        =========================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void SetStart(ThreadStart start);
    
        /*=========================================================================
        ** Clean up the thread when it goes away.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Finalize"]/*' />
        ~Thread()
        {
            // Delegate to the unmanaged portion.
            InternalFinalize();
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void InternalFinalize();
    
    
        /*=========================================================================
        ** Return whether or not this thread is a background thread.  Background
        ** threads do not affect when the Execution Engine shuts down.
        **
        ** Exceptions: ThreadStateException if the thread is dead.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.IsBackground"]/*' />
        public bool IsBackground {
            get { return IsBackgroundNative(); }
            set { SetBackgroundNative(value); }
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern bool IsBackgroundNative();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void SetBackgroundNative(bool isBackground);
        
        
        /*=========================================================================
        ** Return the thread state as a consistent set of bits.  This is more
        ** general then IsAlive or IsBackground.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.ThreadState"]/*' />
        public ThreadState ThreadState {
            get { return (ThreadState)GetThreadStateNative(); }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int GetThreadStateNative();
    
    
        /*=========================================================================
        ** Allocates an un-named data slot. The slot is allocated on ALL the 
        ** threads.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.AllocateDataSlot"]/*' />
        public static LocalDataStoreSlot AllocateDataSlot()
        {
            return m_LocalDataStoreMgr.AllocateDataSlot();
        }
        
        /*=========================================================================
        ** Allocates a named data slot. The slot is allocated on ALL the 
        ** threads.  Named data slots are "public" and can be manipulated by
        ** anyone.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.AllocateNamedDataSlot"]/*' />
        public static LocalDataStoreSlot AllocateNamedDataSlot(String name)
        {
            return m_LocalDataStoreMgr.AllocateNamedDataSlot(name);
        }
        
        /*=========================================================================
        ** Looks up a named data slot. If the name has not been used, a new slot is
        ** allocated.  Named data slots are "public" and can be manipulated by
        ** anyone.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.GetNamedDataSlot"]/*' />
        public static LocalDataStoreSlot GetNamedDataSlot(String name)
        {
            return m_LocalDataStoreMgr.GetNamedDataSlot(name);
        }

        /*=========================================================================
        ** Frees a named data slot. The slot is allocated on ALL the 
        ** threads.  Named data slots are "public" and can be manipulated by
        ** anyone.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.FreeNamedDataSlot"]/*' />
        public static void FreeNamedDataSlot(String name)
        {
            m_LocalDataStoreMgr.FreeNamedDataSlot(name);
        }
        
        /*=========================================================================
        ** Retrieves the value from the specified slot on the current thread, for that thread's current domain.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.GetData"]/*' />
        public static Object GetData(LocalDataStoreSlot slot)
        {
            m_LocalDataStoreMgr.ValidateSlot(slot);
    
            LocalDataStore dls = GetDomainLocalStore();
            if (dls == null)
                return null;

            return dls.GetData(slot);
        }

        /*=========================================================================
        ** Sets the data in the specified slot on the currently running thread, for that thread's current domain.
        =========================================================================*/
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.SetData"]/*' />
        public static void SetData(LocalDataStoreSlot slot, Object data)
        {   
            LocalDataStore dls = GetDomainLocalStore();

            // Create new DLS if one hasn't been created for this domain for this thread
            if (dls == null) {
                dls = m_LocalDataStoreMgr.CreateLocalDataStore();
                SetDomainLocalStore(dls);
            }

            dls.SetData(slot, data);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern private LocalDataStore GetDomainLocalStore();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern private void SetDomainLocalStore(LocalDataStore dls);
       
        /***
         * An appdomain has been unloaded - remove its DLS from the manager
         */
        static private void RemoveDomainLocalStore(LocalDataStore dls)
        {
            if (dls != null)
                m_LocalDataStoreMgr.DeleteLocalDataStore(dls);
        }

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.CurrentUICulture"]/*' />
        public CultureInfo CurrentUICulture {
            get {
                if (null==m_CurrentUICulture) {
                    return CultureInfo.UserDefaultUICulture;
                }
                return m_CurrentUICulture;
            }

            set {
                if (null==value) {
                    throw new ArgumentNullException("value");
                }

                //If they're trying to use a Culture with a name that we can't use in resource lookup,
                //don't even let them set it on the thread.
                CultureInfo.VerifyCultureName(value, true);

                m_CurrentUICulture = value;
            }
        }

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.CurrentCulture"]/*' />
        public CultureInfo CurrentCulture {
            get {
                if (null==m_CurrentCulture) {
                    return CultureInfo.UserDefaultCulture;
                }
                return m_CurrentCulture;
            }

            [SecurityPermission(SecurityAction.Demand, ControlThread=true)]
            set {
                if (null==value) {
                    throw new ArgumentNullException("value");
                }
                CultureInfo.CheckNeutral(value);
                
                //If we can't set the nativeThreadLocale, we'll just let it stay
                //at whatever value it had before.  This allows people who use
                //just managed code not to be limited by the underlying OS.
                CultureInfo.nativeSetThreadLocale(value.LCID);
                m_CurrentCulture = value;
            }
        }



        /*===============================================================
        ====================== Thread Statics ===========================
        ===============================================================*/
        private int ReserveSlot()
        {
            // This is called by the thread on itself so no need for locks
            if (m_ThreadStatics == null)
            {
                // allocate the first bucket
                m_ThreadStatics = new Object[STATICS_BUCKET_SIZE];
                // set next bucket field to null
                m_ThreadStatics[0] = null;
                m_ThreadStaticsFreeIndex = 1;
                m_ThreadStaticsCurrentBucket = 0;
            }

            // See if we have to allocate a new bucket
            if (m_ThreadStaticsFreeIndex == STATICS_BUCKET_SIZE)
            {
                Object[] newBucket = new Object[STATICS_BUCKET_SIZE];

                // walk the chain to locate the last bucket
                Object[] bucket = m_ThreadStatics;
                while (bucket[0] != null)
                {
                    bucket = (Object[]) bucket[0];
                }
                // chain in the new bucket
                bucket[0] = newBucket;
                m_ThreadStaticsFreeIndex = 1;
                m_ThreadStaticsCurrentBucket++;
            }
            // bucket# in highWord, index in lowWord
            return m_ThreadStaticsFreeIndex++|m_ThreadStaticsCurrentBucket<<16;
        }
        /*=============================================================*/

        /*======================================================================
        **  Current thread context is stored in a slot in the thread local store
        **  CurrentContext gets the Context from the slot.
        ======================================================================*/

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.CurrentContext"]/*' />
        public static Context CurrentContext
        {
            get
            {
                return CurrentThread.GetCurrentContextInternal();
            }
        }

        internal Context GetCurrentContextInternal()
        {
            if (m_Context == null)
            {
                m_Context = Context.DefaultContext;
            }
            return m_Context;
        }
    
        internal LogicalCallContext GetLogicalCallContext()
        {
            if (m_LogicalCallContext == null)
            {
                m_LogicalCallContext = new LogicalCallContext();
            }
            return m_LogicalCallContext;
        }

        internal LogicalCallContext SetLogicalCallContext(
            LogicalCallContext callCtx)
        {
            LogicalCallContext oldCtx = m_LogicalCallContext;
            m_LogicalCallContext = callCtx;
            return oldCtx;
        }

        internal IllogicalCallContext GetIllogicalCallContext()
        {
            if (m_IllogicalCallContext == null)
            {
                m_IllogicalCallContext = new IllogicalCallContext();
            }
            return m_IllogicalCallContext;
        }
    
        // Get and set thread's current principal (for role based security).
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.CurrentPrincipal"]/*' />
        public static IPrincipal CurrentPrincipal
        {
            get
            {
                lock (CurrentThread)
                {
                    IPrincipal principal = (IPrincipal)
                        CallContext.SecurityData.Principal;
                    if (principal == null)
                    {
                        principal = GetDomain().GetThreadPrincipal();
                        CallContext.SecurityData.Principal = principal;
                    }
                    return principal;
                }
            }

            [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPrincipal)]
            set
            {
                CallContext.SecurityData.Principal = value;
            }       
        }

        // Private routine called from unmanaged code to set an initial
        // principal for a newly created thread.
        private void SetPrincipalInternal(IPrincipal principal)
        {
            GetLogicalCallContext().SecurityData.Principal = principal;
        }

        // This returns the exposed context for a given context ID.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Context GetContextInternal(IntPtr id);

        internal void EnterContext(Context newContext, ref ContextTransitionFrame frame)
        {
            EnterContextInternal(newContext, newContext.InternalContextID, 0, ref frame);
            // The newContext parameter is passed in just to keep GC from
            // collecting a new context when we are trying to enter it.
            // By the time this returns, the m_Context field in the managed
            // thread is already set to the newContext ... so from there on
            // the newContext is rooted.
            
            // EnterContextInternal is directly called by the X-AppDomain
            // channel since it only has the target context-id and not the
            // context object itself (since the latter is in another domain)
            // It is also called directly when we are bootstrapping a new
            // AppDomain w.r.t. remoting (in CreateProxyForDomain). In both
            // these cases, the newContext is null.
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool ReturnToContext(ref ContextTransitionFrame frame);

        // This does the real work of switching contexts
        // It will also switch AppDomains/push crossing frames if needed.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool EnterContextInternal(Context ctx, IntPtr id, Int32 appDomainID, ref ContextTransitionFrame frame);

        /*======================================================================
        ** Returns the current domain in which current thread is running.
        **                                                   
        **
        **                                                                    
        ======================================================================*/

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern AppDomain GetDomainInternal();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern AppDomain GetFastDomainInternal();
        
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern bool IsRunningInDomain(Int32 domainId);

        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.GetDomain"]/*' />
        public static AppDomain GetDomain()
        {
            if (CurrentThread.m_Context==null)
            {
                AppDomain ad;
                ad = GetFastDomainInternal();
                if (ad == null)
                    ad = GetDomainInternal();
                return ad;
            }
            else
            {
                BCLDebug.Assert( GetDomainInternal() == CurrentThread.m_Context.AppDomain, "AppDomains on the managed & unmanaged threads should match");
                return CurrentThread.m_Context.AppDomain;
            }
        }


        /*  
         *  This returns a unique id to identify an appdomain.
         */
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.GetDomainID"]/*' />
        public static int GetDomainID()
        {
            return GetDomainIDInternal();
        }

        internal static int GetDomainIDInternal()
        {
            return GetDomain().GetId();
        }
      
    
        // Retrieves the name of the thread.
        //
        /// <include file='doc\Thread.uex' path='docs/doc[@for="Thread.Name"]/*' />
        public  String Name {
            get {
                return m_Name;
                
            }set {
                lock(this) {
                    if (m_Name != null)
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_WriteOnce"));
                    m_Name = value;

                    if (Debugger.IsAttached)
                        InformThreadNameChange(this);
                }            
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InformThreadNameChange(Thread t);

        internal Object ExceptionState {
            get { return m_ExceptionStateInfo;}
            set { m_ExceptionStateInfo = value;}
        }

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            m_Delegate = null;  
            m_PrincipalSlot = null;
            m_Priority = 0;
            DONT_USE_InternalThread = IntPtr.Zero;
        }
#endif
    }
 
    // declaring a local var of this enum type and passing it by ref into a function that needs to do a 
    // stack crawl will both prevent inlining of the calle and pass an ESP point to stack crawl to
	[Serializable]
    internal enum StackCrawlMark
    {
        LookForMe = 0,
        LookForMyCaller = 1,
                LookForMyCallersCaller = 2
    }

        }
