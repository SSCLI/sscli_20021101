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
** Class:  IOException
**
**                                                    
**
** Purpose: Exception for a generic IO error.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System.IO {

    /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException"]/*' />
    [Serializable]
    public class IOException : SystemException
    {
        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException"]/*' />
        public IOException() 
            : base(Environment.GetResourceString("Arg_IOException")) {
    		SetErrorCode(__HResults.COR_E_IO);
        }
        
        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException1"]/*' />
        public IOException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_IO);
        }

        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException2"]/*' />
        public IOException(String message, int hresult) 
            : base(message) {
    		SetErrorCode(hresult);
        }
    	
        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException3"]/*' />
        public IOException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_IO);
        }

        /// <include file='doc\IOException.uex' path='docs/doc[@for="IOException.IOException4"]/*' />
        protected IOException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
