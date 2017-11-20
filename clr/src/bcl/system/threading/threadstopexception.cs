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
** Class: ThreadStopException
**
**                                              
**
** Purpose: An exception class which is thrown into a thread to cause it to
**          stop.  This exception is typically not caught and the thread
**          dies as a result.  This is thrown by the VM and should NOT be
**          thrown by any user thread, and subclassing this is useless.
**
** Date: June 1, 1998
**
=============================================================================*/

namespace System.Threading {
	using System.Threading;
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ThreadStopException.uex' path='docs/doc[@for="ThreadStopException"]/*' />
    /// <internalonly/>
    [Serializable()] internal sealed class ThreadStopException : SystemException {
    	private ThreadStopException() 
	        : base(Environment.GetResourceString("Arg_ThreadStopException")) {
    		SetErrorCode(__HResults.COR_E_THREADSTOP);
        }
    
        private ThreadStopException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_THREADSTOP);
        }
    
        private ThreadStopException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_THREADSTOP);
        }

        internal ThreadStopException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
