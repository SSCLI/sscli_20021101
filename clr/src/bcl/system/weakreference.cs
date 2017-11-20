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
/*============================================================
**
** Class: WeakReference
**
**                                        
**
** Purpose: A wrapper for establishing a WeakReference to an Object.
**
** Date:  November 4, 1998
** 
===========================================================*/
namespace System {
    
    using System;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System.Runtime.InteropServices;
    using System.Threading;
    /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference"]/*' />
    [Serializable()] public class WeakReference : ISerializable {
    
        internal IntPtr m_handle;
        internal bool m_IsLongReference;
    
        // Creates a new WeakReference that keeps track of target.
        // Assumes a Short Weak Reference (ie TrackResurrection is false.)
        //
        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.WeakReference"]/*' />
        public WeakReference(Object target) 
            : this(target, false) {
        }
    
        //Creates a new WeakReference that keeps track of target.
        //
        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.WeakReference1"]/*' />
        public WeakReference(Object target, bool trackResurrection) {
            m_IsLongReference=trackResurrection;
            m_handle = GCHandle.InternalAlloc(target,
                            trackResurrection ? GCHandleType.WeakTrackResurrection : GCHandleType.Weak);
        }


        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.WeakReference2"]/*' />
        protected WeakReference(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            Object temp = info.GetValue("TrackedObject",typeof(Object));
            m_IsLongReference = info.GetBoolean("TrackResurrection");
            m_handle = GCHandle.InternalAlloc(temp,
                                              m_IsLongReference ? GCHandleType.WeakTrackResurrection : GCHandleType.Weak);
        }
    
        //Determines whether or not this instance of WeakReference still refers to an object
        //that has not been collected.
        //
        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.IsAlive"]/*' />
        public virtual bool IsAlive {
            get {
                IntPtr h = m_handle;
                if (IntPtr.Zero == h)
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));
                return (GCHandle.InternalGet(h)!=null);
            }
        }
    
        //Returns a boolean indicating whether or not we're tracking objects until they're collected (true)
        //or just until they're finalized (false).
        //
        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.TrackResurrection"]/*' />
        public virtual bool TrackResurrection {
            get { return m_IsLongReference; }
        }
    
        //Gets the Object stored in the handle if it's accessible.
        // Or sets it.
        //
        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.Target"]/*' />
        public virtual Object Target {
            get {
                IntPtr h = m_handle;
                if (IntPtr.Zero == h)
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));
                Object o = GCHandle.InternalGet(h);

                // Check m_handle once more.  There is a race when this object is
                // finalized.  If the handle has gone to null, we have to fail
                // this call.
                if (m_handle == IntPtr.Zero) {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));
                } else {
                    return o;
                }
            }
            set {
                IntPtr h = m_handle;
                if (h == IntPtr.Zero)
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));

                // There is a race w/ finalization where m_handle gets set to
                // NULL and the WeakReference becomes invalid.  Here we have to 
                // do the following in order:
                //
                // 1.  Get the old object value
                // 2.  Get m_handle
                // 3.  HndInterlockedCompareExcahnge(m_handle, newValue, oldValue);
                //
                // If the interlocked-cmp-exchange fails, then either we lost a race
                // with another updater, or we lost a race w/ the finalizer.  In
                // either case, we can just let the other guy win.
                //
                Object oldValue = GCHandle.InternalGet(h);
                h = m_handle;  
                if (h == IntPtr.Zero)
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));
                GCHandle.InternalCompareExchange(h, value, oldValue, false /* isPinned */);
            }
        }
    
        // Free all system resources associated with this reference.
        //
        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.Finalize"]/*' />
        ~WeakReference() {
            IntPtr old_handle = m_handle;
            if (old_handle != IntPtr.Zero) {
                if (old_handle == Interlocked.CompareExchangePointer(ref m_handle, IntPtr.Zero, old_handle)) 
                    GCHandle.InternalFree(old_handle);
            }
        }
    
        /// <include file='doc\WeakReference.uex' path='docs/doc[@for="WeakReference.GetObjectData"]/*' />
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            info.AddValue("TrackedObject", Target, typeof(Object));
            info.AddValue("TrackResurrection", m_IsLongReference);
        }
    
    }        

}
