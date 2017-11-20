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
** Class:  EndOfStreamException
**
**                                                    
**
** Purpose: Exception to be thrown when reading past end-of-file.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System.IO {
    /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException"]/*' />
    [Serializable]
    public class EndOfStreamException : IOException
    {
        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException"]/*' />
        public EndOfStreamException() 
            : base(Environment.GetResourceString("Arg_EndOfStreamException")) {
    		SetErrorCode(__HResults.COR_E_ENDOFSTREAM);
        }
        
        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException1"]/*' />
        public EndOfStreamException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_ENDOFSTREAM);
        }
    	
        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException2"]/*' />
        public EndOfStreamException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_ENDOFSTREAM);
        }

        /// <include file='doc\EndOfStreamException.uex' path='docs/doc[@for="EndOfStreamException.EndOfStreamException3"]/*' />
        protected EndOfStreamException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }

}
