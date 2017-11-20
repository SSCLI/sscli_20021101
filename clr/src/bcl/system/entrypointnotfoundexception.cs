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
** Class: EntryPointNotFoundException
**
**                                                   
**
** Purpose: The exception class for some failed P/Invoke calls.
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException"]/*' />
    [Serializable] public class EntryPointNotFoundException : TypeLoadException {
        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException"]/*' />
        public EntryPointNotFoundException() 
            : base(Environment.GetResourceString("Arg_EntryPointNotFoundException")) {
            SetErrorCode(__HResults.COR_E_ENTRYPOINTNOTFOUND);
        }
    
        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException1"]/*' />
        public EntryPointNotFoundException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_ENTRYPOINTNOTFOUND);
        }
    
        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException2"]/*' />
        public EntryPointNotFoundException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_ENTRYPOINTNOTFOUND);
        }

        /// <include file='doc\EntryPointNotFoundException.uex' path='docs/doc[@for="EntryPointNotFoundException.EntryPointNotFoundException3"]/*' />
        protected EntryPointNotFoundException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    
    
    }

}
