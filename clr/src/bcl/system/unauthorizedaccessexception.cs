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
/*============================================================
**
** Class:  UnauthorizedAccessException
**
**                                         
**
** Purpose: An exception for OS 'access denied' types of 
**          errors, including IO and limited security types 
**          of errors.
**
** Date: August 9, 2000
** 
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System {
    // The UnauthorizedAccessException is thrown when access errors 
    // occur from IO or other OS methods.  
    /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException"]/*' />
    [Serializable()]
    public class UnauthorizedAccessException : SystemException {
    	/// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException"]/*' />
    	public UnauthorizedAccessException() 
            : base(Environment.GetResourceString("Arg_UnauthorizedAccessException")) {
    		SetErrorCode(__HResults.COR_E_UNAUTHORIZEDACCESS);
        }
    	
        /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException1"]/*' />
        public UnauthorizedAccessException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_UNAUTHORIZEDACCESS);
        }
    	
        /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException2"]/*' />
        public UnauthorizedAccessException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_UNAUTHORIZEDACCESS);
        }

        /// <include file='doc\UnauthorizedAccessException.uex' path='docs/doc[@for="UnauthorizedAccessException.UnauthorizedAccessException3"]/*' />
        protected UnauthorizedAccessException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
