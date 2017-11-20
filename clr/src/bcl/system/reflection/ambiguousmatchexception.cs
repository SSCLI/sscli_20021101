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
// AmbiguousMatchException is thrown when binding to a method results in more
//	than one method matching the binding criteria.  This exception is thrown in
//	general when something is Ambiguous.
//
// Date: March 98
//
namespace System.Reflection {
	using System;
	using SystemException = System.SystemException;
	using System.Runtime.Serialization;
    /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException"]/*' />
	[Serializable()]    
    public sealed class AmbiguousMatchException : SystemException
	{
    	
        /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException.AmbiguousMatchException"]/*' />
        public AmbiguousMatchException() 
	        : base(Environment.GetResourceString("Arg_AmbiguousMatchException")) {
			SetErrorCode(__HResults.COR_E_AMBIGUOUSMATCH);
        }
    
        /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException.AmbiguousMatchException1"]/*' />
        public AmbiguousMatchException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_AMBIGUOUSMATCH);
        }
    	
        /// <include file='doc\AmbiguousMatchException.uex' path='docs/doc[@for="AmbiguousMatchException.AmbiguousMatchException2"]/*' />
        public AmbiguousMatchException(String message, Exception inner)  : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_AMBIGUOUSMATCH);
        }

        internal AmbiguousMatchException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
