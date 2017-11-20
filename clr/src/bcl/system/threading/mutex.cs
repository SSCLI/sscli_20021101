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
** Class: Mutex	
**
**                                                  
**
** Purpose: synchronization primitive that can also be used for interprocess synchronization
**
** Date: February, 2000
**
=============================================================================*/
namespace System.Threading 
{  
	using System;
	using System.Threading;
	using System.Runtime.CompilerServices;
    /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex"]/*' />
    public sealed class Mutex : WaitHandle
    {
        
        /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex"]/*' />
        public Mutex(bool initiallyOwned, String name, out bool createdNew)
    	{
            IntPtr mutexHandle = CreateMutexNative(initiallyOwned, name, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
        /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex1"]/*' />
        public Mutex(bool initiallyOwned, String name)
    	{
    	    bool createdNew;
            IntPtr mutexHandle = CreateMutexNative(initiallyOwned, name, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
		/// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex2"]/*' />
		public Mutex(bool initiallyOwned)
    	{
    	    bool createdNew;
    	    IntPtr mutexHandle = CreateMutexNative(initiallyOwned, null, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
        /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex3"]/*' />
        public Mutex()
    	{
    	    bool createdNew;
    	    IntPtr mutexHandle = CreateMutexNative(false, null, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
    
    	/// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.ReleaseMutex"]/*' />
    	public void ReleaseMutex()
    	{
    		ReleaseMutexNative(Handle);
    	}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static IntPtr CreateMutexNative(bool initialState, String name, out bool createdNew);
    
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static void  ReleaseMutexNative(IntPtr handle);
       
    }
}
