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
** Class: ArrayTypeMismatchException
**
** Author: 
**
** Purpose: The arrays are of different primitive types.
**
** Date: March 30, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    // The ArrayMismatchException is thrown when an attempt to store
    // an object of the wrong type within an array occurs.
    // 
    /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException"]/*' />
    [Serializable()] public class ArrayTypeMismatchException : SystemException {
    	
        // Creates a new ArrayMismatchException with its message string set to
        // the empty string, its HRESULT set to COR_E_ARRAYTYPEMISMATCH, 
        // and its ExceptionInfo reference set to null. 
        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException"]/*' />
        public ArrayTypeMismatchException() 
            : base(Environment.GetResourceString("Arg_ArrayTypeMismatchException")) {
    		SetErrorCode(__HResults.COR_E_ARRAYTYPEMISMATCH);
        }
    	
        // Creates a new ArrayMismatchException with its message string set to
        // message, its HRESULT set to COR_E_ARRAYTYPEMISMATCH, 
        // and its ExceptionInfo reference set to null. 
        // 
        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException1"]/*' />
        public ArrayTypeMismatchException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_ARRAYTYPEMISMATCH);
        }
    	
        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException2"]/*' />
        public ArrayTypeMismatchException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_ARRAYTYPEMISMATCH);
        }

        /// <include file='doc\ArrayTypeMismatchException.uex' path='docs/doc[@for="ArrayTypeMismatchException.ArrayTypeMismatchException3"]/*' />
        protected ArrayTypeMismatchException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
