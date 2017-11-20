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
** Class: PlatformNotSupportedException
**
**                                              
**
** Purpose: To handle features that don't run on particular platforms
**
** Date: September 28, 1998
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Serialization;

    /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException"]/*' />
    [Serializable()] public class PlatformNotSupportedException : NotSupportedException
    {
    	/// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException"]/*' />
    	public PlatformNotSupportedException() 
            : base(Environment.GetResourceString("Arg_PlatformNotSupported")) {
    		SetErrorCode(__HResults.COR_E_PLATFORMNOTSUPPORTED);
        }
    
        /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException1"]/*' />
        public PlatformNotSupportedException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_PLATFORMNOTSUPPORTED);
        }
    	
        /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException2"]/*' />
        public PlatformNotSupportedException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_PLATFORMNOTSUPPORTED);
        }

        /// <include file='doc\PlatformNotSupportedException.uex' path='docs/doc[@for="PlatformNotSupportedException.PlatformNotSupportedException3"]/*' />
        protected PlatformNotSupportedException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

    }
}
