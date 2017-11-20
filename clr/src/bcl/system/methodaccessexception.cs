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
** Class: MethodAccessException
**
** Purpose: The exception class for class loading failures.
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException"]/*' />
    [Serializable] public class MethodAccessException : MemberAccessException {
        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException"]/*' />
        public MethodAccessException() 
            : base(Environment.GetResourceString("Arg_MethodAccessException")) {
    		SetErrorCode(__HResults.COR_E_METHODACCESS);
        }
    
        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException1"]/*' />
        public MethodAccessException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_METHODACCESS);
        }
    
        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException2"]/*' />
        public MethodAccessException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_METHODACCESS);
        }

        /// <include file='doc\MethodAccessException.uex' path='docs/doc[@for="MethodAccessException.MethodAccessException3"]/*' />
        protected MethodAccessException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
