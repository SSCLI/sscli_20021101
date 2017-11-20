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
** Class: OutOfMemoryException
**
**                                             
**
** Purpose: The exception class for OOM.
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException"]/*' />
    [Serializable()] public class OutOfMemoryException : SystemException {
        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException"]/*' />
        public OutOfMemoryException() 
            : base(Environment.GetResourceString("Arg_OutOfMemoryException")) {
    		SetErrorCode(__HResults.COR_E_OUTOFMEMORY);
        }
    
        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException1"]/*' />
        public OutOfMemoryException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_OUTOFMEMORY);
        }
    	
        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException2"]/*' />
        public OutOfMemoryException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_OUTOFMEMORY);
        }

        /// <include file='doc\OutOfMemoryException.uex' path='docs/doc[@for="OutOfMemoryException.OutOfMemoryException3"]/*' />
        protected OutOfMemoryException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
