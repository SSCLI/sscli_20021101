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
** Class: SynchronizationLockException
**
**                                             
**
** Purpose: Wait(), Notify() or NotifyAll() was called from an unsynchronized
**          block of code.
**
** Date: March 30, 1998
**
=============================================================================*/

namespace System.Threading {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException"]/*' />
    [Serializable()] public class SynchronizationLockException : SystemException {
        /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException"]/*' />
        public SynchronizationLockException() 
            : base(Environment.GetResourceString("Arg_SynchronizationLockException")) {
    		SetErrorCode(__HResults.COR_E_SYNCHRONIZATIONLOCK);
        }
    
        /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException1"]/*' />
        public SynchronizationLockException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_SYNCHRONIZATIONLOCK);
        }
    
    	/// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException2"]/*' />
    	public SynchronizationLockException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_SYNCHRONIZATIONLOCK);
        }

        /// <include file='doc\SynchronizationLockException.uex' path='docs/doc[@for="SynchronizationLockException.SynchronizationLockException3"]/*' />
        protected SynchronizationLockException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }

}


