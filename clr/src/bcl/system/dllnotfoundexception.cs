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
** Class: DllNotFoundException
**
**                                                  
**
** Purpose: The exception class for some failed P/Invoke calls.
**
** Date: December 11, 2000
**
=============================================================================*/

namespace System {
    
    using System;
    using System.Runtime.Serialization;

    /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException"]/*' />
    [Serializable] public class DllNotFoundException : TypeLoadException {
        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException"]/*' />
        public DllNotFoundException() 
            : base(Environment.GetResourceString("Arg_DllNotFoundException")) {
            SetErrorCode(__HResults.COR_E_DLLNOTFOUND);
        }
    
        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException1"]/*' />
        public DllNotFoundException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_DLLNOTFOUND);
        }
    
        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException2"]/*' />
        public DllNotFoundException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_DLLNOTFOUND);
        }

        /// <include file='doc\DllNotFoundException.uex' path='docs/doc[@for="DllNotFoundException.DllNotFoundException3"]/*' />
        protected DllNotFoundException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }        
    }
}
