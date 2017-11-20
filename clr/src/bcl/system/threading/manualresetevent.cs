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
** Class: ManualResetEvent	
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
    /// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent"]/*' />
    public sealed class ManualResetEvent : WaitHandle
    {
        
        /// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent.ManualResetEvent"]/*' />
        public ManualResetEvent(bool initialState)
    	{
    		IntPtr eventHandle = CreateManualResetEventNative(initialState);	// throws an exception if failed to create an event
    	    SetHandleInternal(eventHandle);
    	}
    
    	/// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent.Reset"]/*' />
    	public bool Reset()
    	{
    		return ResetManualResetEventNative(Handle);
    	}
    
    	/// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent.Set"]/*' />
    	public bool Set()
    	{
    		return SetManualResetEventNative(Handle);
    	}
		
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static IntPtr CreateManualResetEventNative(bool initialState);
    
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static bool  ResetManualResetEventNative(IntPtr handle);
    
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static bool  SetManualResetEventNative(IntPtr handle);
    
    }}
