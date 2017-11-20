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
** Class: FieldAccessException
**
** Purpose: The exception class for class loading failures.
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException"]/*' />
    [Serializable] public class FieldAccessException : MemberAccessException {
        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException"]/*' />
        public FieldAccessException() 
            : base(Environment.GetResourceString("Arg_FieldAccessException")) {
    		SetErrorCode(__HResults.COR_E_FIELDACCESS);
        }
    
        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException1"]/*' />
        public FieldAccessException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_FIELDACCESS);
        }
    
        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException2"]/*' />
        public FieldAccessException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_FIELDACCESS);
        }

        /// <include file='doc\FieldAccessException.uex' path='docs/doc[@for="FieldAccessException.FieldAccessException3"]/*' />
        protected FieldAccessException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
