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
** Class: AutoResetEvent	
**
**                                                  
**
** Purpose: An example of a WaitHandle class
**
** Date: August, 1999
**
=============================================================================*/
namespace System.Threading {
    
	using System;
	using System.Runtime.CompilerServices;
    /// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent"]/*' />
    public sealed class AutoResetEvent : WaitHandle
    {
        /// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent.AutoResetEvent"]/*' />
        public AutoResetEvent(bool initialState)
    	{
    		IntPtr eventHandle = CreateAutoResetEventNative(initialState);	// throws an exception if failed to create an event
    	    SetHandleInternal(eventHandle);
    	}
    	/// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent.Reset"]/*' />
    	public bool Reset()
    	{
    		bool res = ResetAutoResetEventNative(Handle);
    		return res;
    	}
    
    	/// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent.Set"]/*' />
    	public bool Set()
    	{
    		return SetAutoResetEventNative(Handle);
    	}
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern private static  IntPtr CreateAutoResetEventNative(bool initialState);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static   bool  ResetAutoResetEventNative(IntPtr handle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static   bool  SetAutoResetEventNative(IntPtr handle);
    }}
