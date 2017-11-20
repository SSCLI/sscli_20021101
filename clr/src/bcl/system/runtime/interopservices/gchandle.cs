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
namespace System.Runtime.InteropServices {
    
	using System;
	using System.Security.Permissions;
	using System.Runtime.CompilerServices;

	// These are the types of handles used by the EE.  IMPORTANT: These must
	// match the definitions in ObjectHandle.h in the EE.
	/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandleType"]/*' />
	[Serializable]
	public enum GCHandleType
	{
		/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandleType.Weak"]/*' />
		Weak = 0,
		/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandleType.WeakTrackResurrection"]/*' />
		WeakTrackResurrection = 1,
		/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandleType.Normal"]/*' />
		Normal = 2,
		/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandleType.Pinned"]/*' />
		Pinned = 3
	}

    // This class allows you to create an opaque, GC handle to any 
    // COM+ object. A GC handle is used when an object reference must be
	// reachable from unmanaged memory.  There are 3 kinds of roots:
	// Normal - keeps the object from being collected.
	// Weak - allows object to be collected and handle contents will be zeroed.
    //          Weak references are zeroed before the finalizer runs, so if the
    //          object is resurrected in the finalizer the weak reference is
    //          still zeroed.
	// WeakTrackResurrection - Same as weak, but stays until after object is
    //          really gone.
	// Pinned - same as normal, but allows the address of the actual object
	//          to be taken.
    //
    /// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle"]/*' />
	[System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)]
    public struct GCHandle
    {
        // Allocate a handle storing the object and the type.
        internal GCHandle(Object value, GCHandleType type)
		{
			m_handle = InternalAlloc(value, type);

			// Record if the handle is pinned.
			if (type == GCHandleType.Pinned)
                SetIsPinned();
		}  

		// Used in the conversion functions below.
		internal GCHandle(IntPtr handle)
		{
	        InternalCheckDomain(handle);
			m_handle = handle;
		}

        // Creates a new GC handle for an object.
        //
        // value - The object that the GC handle is created for.
		// type - The type of GC handle to create.
        // 
    	// returns a new GC handle that protects the object.
        /// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.Alloc"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static GCHandle Alloc(Object value)
        {
			return new GCHandle(value, GCHandleType.Normal);
        }

        /// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.Alloc1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static GCHandle Alloc(Object value, GCHandleType type)
        {
			return new GCHandle(value, type);
        }

        // Frees a GC handle.  The caller must provide synchronization to
		// prevent multiple threads from executing this simultaneously for
		// a given handle. If you modify this method please modify the 
        // __InternalFree also which is the internal without the linktime check.
        /// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.Free"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public void Free()
		{
			// Check if the handle was never initialized for was freed.
			if (m_handle == IntPtr.Zero)
				throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));

			// Free the handle.
			InternalFree(GetHandleValue());
			m_handle = IntPtr.Zero;
		}
		
		internal void __InternalFree()
		{
			// Check if the handle was never initialized for was freed.
			if (m_handle == IntPtr.Zero)
				throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));

			// Free the handle.
			InternalFree(GetHandleValue());
			m_handle = IntPtr.Zero;
		}

        // Target property - allows getting / updating of the handle's referent. If you modify this method

        // then modify the __InternalTarget too which is the internal method without the linktime check.
        /// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.Target"]/*' />
        public Object Target
        {
	        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get
            {
                // Check if the handle was never initialized or was freed.
				if (m_handle == IntPtr.Zero)
					throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));

                return InternalGet(GetHandleValue());
            }
    
	        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            set
            {
                // Check if the handle was never initialized or was freed.
				if (m_handle == IntPtr.Zero)
					throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));

                InternalSet(GetHandleValue(), value, IsPinned());
            }
        }
        
        internal Object __InternalTarget
        {
            get
            {
                // Check if the handle was never initialized or was freed.
				if (m_handle == IntPtr.Zero)
					throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));

                return InternalGet(GetHandleValue());
            }
        }

        // Retrieve the address of an object in a Pinned handle.  This throws
		// an exception if the handle is any type other than Pinned.
		/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.AddrOfPinnedObject"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
		public IntPtr AddrOfPinnedObject()
		{
			// Check if the handle was not a pinned handle.
			if (!IsPinned())
			{
				// Check if the handle was never initialized for was freed.
				if (m_handle == IntPtr.Zero)
					throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotInitialized"));

				// You can only get the address of pinned handles.
				throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HandleIsNotPinned"));
			}

			// Get the address.
			return InternalAddrOfPinnedObject(GetHandleValue());
		}

        // Determine whether this handle has been allocated or not.
		/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.IsAllocated"]/*' />
		public bool IsAllocated
		{
			get
			{
				return m_handle != IntPtr.Zero;
			}
		}

        // Used to create a GCHandle from an int.  This is intended to
		// be used with the reverse conversion.
    	/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.operatorGCHandle"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
    	public static explicit operator GCHandle(IntPtr value)
		{
            return new GCHandle(value);
        }

	// Used to get the internal integer representation of the handle out.
    	/// <include file='doc\GcHandle.uex' path='docs/doc[@for="GCHandle.operatorIntPtr"]/*' />
    	public static explicit operator IntPtr(GCHandle value)
		{
            return value.m_handle;
        }

        internal IntPtr GetHandleValue()
        {
#if WIN32
            return new IntPtr(((int)m_handle) & ~1);
#else
            return new IntPtr(((long)m_handle) & ~1L);
#endif
        }

        internal bool IsPinned()
        {
#if WIN32
            return (((int)m_handle) & 1) != 0;
#else
            return (((long)m_handle) & 1) != 0;
#endif
        }

        internal void SetIsPinned()
        {
#if WIN32
            m_handle = new IntPtr(((int)m_handle) | 1);
#else
            m_handle = new IntPtr(((long)m_handle) | 1L);
#endif
        }

		// Internal native calls that this implementation uses.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern IntPtr InternalAlloc(Object value, GCHandleType type);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalFree(IntPtr handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Object InternalGet(IntPtr handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSet(IntPtr handle, Object value, bool isPinned);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalCompareExchange(IntPtr handle, Object value, Object oldValue, bool isPinned);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern IntPtr InternalAddrOfPinnedObject(IntPtr handle);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern void InternalCheckDomain(IntPtr handle);


		// The actual integer handle value that the EE uses internally.
		private IntPtr m_handle;
    }
}
