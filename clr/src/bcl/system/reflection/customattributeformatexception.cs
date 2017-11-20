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
// CustomAttributeFormatException is thrown when the binary format of a 
//	custom attribute is invalid.
//
// Date: March 98
//
namespace System.Reflection {
	using System;
	using ApplicationException = System.ApplicationException;
	using System.Runtime.Serialization;
    /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException"]/*' />
	[Serializable()] 
    public class CustomAttributeFormatException  : FormatException {
    
        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException"]/*' />
        public CustomAttributeFormatException()
	        : base(Environment.GetResourceString("Arg_CustomAttributeFormatException")) {
    		SetErrorCode(__HResults.COR_E_CUSTOMATTRIBUTEFORMAT);
        }
    
        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException1"]/*' />
        public CustomAttributeFormatException(String message) : base(message) {
    		SetErrorCode(__HResults.COR_E_CUSTOMATTRIBUTEFORMAT);
        }
    	
        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException2"]/*' />
        public CustomAttributeFormatException(String message, Exception inner) : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_CUSTOMATTRIBUTEFORMAT);
        }

        /// <include file='doc\CustomAttributeFormatException.uex' path='docs/doc[@for="CustomAttributeFormatException.CustomAttributeFormatException3"]/*' />
        protected CustomAttributeFormatException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
