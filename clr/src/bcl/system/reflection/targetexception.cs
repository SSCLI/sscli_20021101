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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// TargetException is thrown when the target to an Invoke is invalid.  This may
//	occur because the caller doesn't have access to the member, or the target doesn't
//	define the member, etc.
//
// Date: March 98
//
namespace System.Reflection {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException"]/*' />
	[Serializable()] 
    public class TargetException : ApplicationException {
    	
        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException"]/*' />
        public TargetException() : base() {
    		SetErrorCode(__HResults.COR_E_TARGET);
        }
    
        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException1"]/*' />
        public TargetException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_TARGET);
        }
    	
        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException2"]/*' />
        public TargetException(String message, Exception inner) : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_TARGET);
        }

        /// <include file='doc\TargetException.uex' path='docs/doc[@for="TargetException.TargetException3"]/*' />
        protected TargetException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
