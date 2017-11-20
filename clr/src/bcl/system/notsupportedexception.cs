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
** Class: NotSupportedException
**
**                                         
**
** Purpose: For methods that should be implemented on subclasses.
**
** Date: September 28, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException"]/*' />
    [Serializable()] public class NotSupportedException : SystemException
    {
    	/// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException"]/*' />
    	public NotSupportedException() 
            : base(Environment.GetResourceString("Arg_NotSupportedException")) {
    		SetErrorCode(__HResults.COR_E_NOTSUPPORTED);
        }
    
        /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException1"]/*' />
        public NotSupportedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_NOTSUPPORTED);
        }
    	
        /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException2"]/*' />
        public NotSupportedException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_NOTSUPPORTED);
        }

        /// <include file='doc\NotSupportedException.uex' path='docs/doc[@for="NotSupportedException.NotSupportedException3"]/*' />
        protected NotSupportedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
