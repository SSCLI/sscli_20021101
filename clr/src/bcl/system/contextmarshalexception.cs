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
** Class: ContextMarshalException
**
**                                              
**
** Purpose: Exception class for attempting to pass an instance through a context
**          boundary, when the formal type and the instance's marshal style are
**          incompatible.
**
** Date: July 6, 1998
**
=============================================================================*/

namespace System {
	using System.Runtime.InteropServices;
	using System.Runtime.Remoting;
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException"]/*' />
    [Serializable()] public class ContextMarshalException : SystemException {
        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException"]/*' />
        public ContextMarshalException() 
            : base(Environment.GetResourceString("Arg_ContextMarshalException")) {
    		SetErrorCode(__HResults.COR_E_CONTEXTMARSHAL);
        }
    
        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException1"]/*' />
        public ContextMarshalException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_CONTEXTMARSHAL);
        }
    	
        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException2"]/*' />
        public ContextMarshalException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_CONTEXTMARSHAL);
        }

        /// <include file='doc\ContextMarshalException.uex' path='docs/doc[@for="ContextMarshalException.ContextMarshalException3"]/*' />
        protected ContextMarshalException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
