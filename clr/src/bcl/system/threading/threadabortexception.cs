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
** Class: ThreadAbortException
**
**                                                  
**
** Purpose: An exception class which is thrown into a thread to cause it to
**          abort. This is a special non-catchable exception and results in
**			the thread's death.  This is thrown by the VM only and can NOT be
**          thrown by any user thread, and subclassing this is useless.
**
** Date: February, 2000
**
=============================================================================*/

namespace System.Threading {
    
    
    using System;
    using System.Runtime.Serialization;

    /// <include file='doc\ThreadAbortException.uex' path='docs/doc[@for="ThreadAbortException"]/*' />
    [Serializable()] public sealed class ThreadAbortException : SystemException {
    	private ThreadAbortException() 
	        : base(Environment.GetResourceString("Arg_ThreadAbortException")) {
    		SetErrorCode(__HResults.COR_E_THREADABORTED);
        }

        //required for serialization
        internal ThreadAbortException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
   
        private ThreadAbortException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_THREADABORTED);
        }
    
        private ThreadAbortException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_THREADABORTED);
        }

		/// <include file='doc\ThreadAbortException.uex' path='docs/doc[@for="ThreadAbortException.ExceptionState"]/*' />
		public Object ExceptionState {
			get {return Thread.CurrentThread.ExceptionState;}
		}
    }
}


