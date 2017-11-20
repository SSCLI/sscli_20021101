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
** Class: InvalidProgramException
**
**                                                    
**
** Purpose: The exception class for programs with invalid IL or bad metadata.
**
** Date: November 21, 2000
**
=============================================================================*/

namespace System {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException"]/*' />
    [Serializable]
    public sealed class InvalidProgramException : SystemException {
        /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException.InvalidProgramException"]/*' />
        public InvalidProgramException() 
            : base(Environment.GetResourceString("InvalidProgram_Default")) {
    		SetErrorCode(__HResults.COR_E_INVALIDPROGRAM);
        }
    
        /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException.InvalidProgramException1"]/*' />
        public InvalidProgramException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_INVALIDPROGRAM);
        }
    
        /// <include file='doc\InvalidProgramException.uex' path='docs/doc[@for="InvalidProgramException.InvalidProgramException2"]/*' />
        public InvalidProgramException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_INVALIDPROGRAM);
        }

        internal InvalidProgramException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
