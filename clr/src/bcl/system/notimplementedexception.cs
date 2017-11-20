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
** Class: NotImplementedException
**
**                                       
**
** Purpose: Exception thrown when a requested method or operation is not 
**			implemented.
**
** Date: May 8, 2000
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException"]/*' />
    [Serializable()] public class NotImplementedException : SystemException
    {
    	/// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException"]/*' />
    	public NotImplementedException() 
            : base(Environment.GetResourceString("Arg_NotImplementedException")) {
    		SetErrorCode(__HResults.E_NOTIMPL);
        }
        /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException1"]/*' />
        public NotImplementedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.E_NOTIMPL);
        }
        /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException2"]/*' />
        public NotImplementedException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.E_NOTIMPL);
        }

        /// <include file='doc\NotImplementedException.uex' path='docs/doc[@for="NotImplementedException.NotImplementedException3"]/*' />
        protected NotImplementedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
