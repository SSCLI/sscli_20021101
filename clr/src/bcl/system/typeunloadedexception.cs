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
** Class: TypeUnloadedException
**
**                                                
**
** Purpose: Exception class for attempt to access an unloaded class
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System.Runtime.Serialization;

    /// <include file='doc\TypeUnloadedException.uex' path='docs/doc[@for="TypeUnloadedException"]/*' />
    [Serializable()] public class TypeUnloadedException : SystemException {
        /// <include file='doc\TypeUnloadedException.uex' path='docs/doc[@for="TypeUnloadedException.TypeUnloadedException"]/*' />
        public TypeUnloadedException() 
	        : base(Environment.GetResourceString("Arg_TypeUnloadedException")) {
    		SetErrorCode(__HResults.COR_E_TYPEUNLOADED);
        }
    
        /// <include file='doc\TypeUnloadedException.uex' path='docs/doc[@for="TypeUnloadedException.TypeUnloadedException1"]/*' />
        public TypeUnloadedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_TYPEUNLOADED);
        }
    
        /// <include file='doc\TypeUnloadedException.uex' path='docs/doc[@for="TypeUnloadedException.TypeUnloadedException2"]/*' />
        public TypeUnloadedException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_TYPEUNLOADED);
        }

        //
        // This constructor is required for serialization;
        //
        /// <include file='doc\TypeUnloadedException.uex' path='docs/doc[@for="TypeUnloadedException.TypeUnloadedException3"]/*' />
        protected TypeUnloadedException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}

