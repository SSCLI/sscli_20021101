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
** Class: ExecutionEngineException
**
**                                             
**
** Purpose: The exception class for misc execution engine exceptions.
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException"]/*' />
    [Serializable()] public sealed class ExecutionEngineException : SystemException {
        /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException.ExecutionEngineException"]/*' />
        public ExecutionEngineException() 
            : base(Environment.GetResourceString("Arg_ExecutionEngineException")) {
    		SetErrorCode(__HResults.COR_E_EXECUTIONENGINE);
        }
    
        /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException.ExecutionEngineException1"]/*' />
        public ExecutionEngineException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_EXECUTIONENGINE);
        }
    
        /// <include file='doc\ExecutionEngineException.uex' path='docs/doc[@for="ExecutionEngineException.ExecutionEngineException2"]/*' />
        public ExecutionEngineException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_EXECUTIONENGINE);
        }

        internal ExecutionEngineException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }

}
