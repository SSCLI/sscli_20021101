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
** Class: MarshalDirectiveException
**
** Purpose: This exception is thrown when the marshaller encounters a signature
**          that has an invalid MarshalAs CA for a given argument or is not
**          supported.
**
=============================================================================*/

namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException"]/*' />
    [Serializable()] public class MarshalDirectiveException : SystemException {
        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException"]/*' />
        public MarshalDirectiveException() 
            : base(Environment.GetResourceString("Arg_MarshalDirectiveException")) {
    		SetErrorCode(__HResults.COR_E_MARSHALDIRECTIVE);
        }
    
        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException1"]/*' />
        public MarshalDirectiveException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MARSHALDIRECTIVE);
        }
    
        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException2"]/*' />
        public MarshalDirectiveException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MARSHALDIRECTIVE);
        }

        /// <include file='doc\MarshalDirectiveException.uex' path='docs/doc[@for="MarshalDirectiveException.MarshalDirectiveException3"]/*' />
        protected MarshalDirectiveException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
