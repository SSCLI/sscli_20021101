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
namespace System {
    
	using System;
	using System.Runtime.InteropServices;
	using System.Runtime.CompilerServices;

	// This class will not be marked serializable
    /// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator"]/*' />
	[StructLayout(LayoutKind.Auto)]
    public struct ArgIterator
    {
    	// create an arg iterator that points at the first argument that
    	// is not statically declared (that is the first ... arg)
    	// 'arglist' is the value returned by the ARGLIST instruction
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.ArgIterator"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern ArgIterator(RuntimeArgumentHandle arglist);
    
    	// create an arg iterator that points just past 'firstArg'.  
    	// 'arglist' is the value returned by the ARGLIST instruction
    	// This is much like the C va_start macro
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.ArgIterator1"]/*' />
		[MethodImplAttribute(MethodImplOptions.InternalCall), CLSCompliant(false)]
    	public unsafe extern ArgIterator(RuntimeArgumentHandle arglist, void* ptr);

    	// Fetch an argument as a typed referece, advance the iterator.
    	// Throws an exception if past end of argument list
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.GetNextArg"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall), CLSCompliant(false)]
    	public extern TypedReference GetNextArg();

        // Alternate version of GetNextArg() intended primarily for IJW code
        // generated by VC's "va_arg()" construct. 
        /// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.GetNextArg1"]/*' />
        [CLSCompliant(false)]
        public TypedReference GetNextArg(RuntimeTypeHandle rth)
        {
            if (SigPtr != IntPtr.Zero)
            {
                // This is an ordinary ArgIterator capable of determining
                // types from a signature. Just do a regular GetNextArg.
                return GetNextArg();
            }
            else
            {
                return InternalGetNextArg(rth);
            }
        }


        [MethodImplAttribute(MethodImplOptions.InternalCall), CLSCompliant(false)]
        private extern TypedReference InternalGetNextArg(RuntimeTypeHandle rth);

    	// Invalidate the iterator (va_end)
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.End"]/*' />
    	public void End()
    	{
    	}
    
    	// How many arguments are left in the list 
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.GetRemainingCount"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern int GetRemainingCount();
    
    	// Gets the type of the current arg, does NOT advance the iterator
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.GetNextArgType"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern RuntimeTypeHandle GetNextArgType();
    
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return unchecked((int)((long)ArgCookie));
    	}
    
    	// Inherited from object
    	/// <include file='doc\ArgIterator.uex' path='docs/doc[@for="ArgIterator.Equals"]/*' />
    	public override bool Equals(Object o)
    	{
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_NYI"));
    	}
    
    	private IntPtr ArgCookie;				// Cookie from the EE.
    	private IntPtr SigPtr;					// Pointer to remaining signature.
    	private IntPtr ArgPtr;					// Pointer to remaining args.
    	private int    RemainingArgs;			// # of remaining args.

		//
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
			ArgCookie = IntPtr.Zero;
			SigPtr = IntPtr.Zero;
			ArgPtr = IntPtr.Zero;
			RemainingArgs = 0;
		}
#endif
    }
}
