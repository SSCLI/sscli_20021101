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
** Class: CannotUnloadAppDomainException
**
**                                                
**
** Purpose: Exception class for failed attempt to unload an AppDomain.
**
** Date: September 15, 2000
**
=============================================================================*/

namespace System {

	using System.Runtime.Serialization;

    /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException"]/*' />
    [Serializable()] public class CannotUnloadAppDomainException : SystemException {
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException"]/*' />
        public CannotUnloadAppDomainException() 
            : base(Environment.GetResourceString("Arg_CannotUnloadAppDomainException")) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }
    
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException1"]/*' />
        public CannotUnloadAppDomainException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }
    
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException2"]/*' />
        public CannotUnloadAppDomainException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }

        //
        //This constructor is required for serialization.
        //
        /// <include file='doc\CannotUnloadAppDomainException.uex' path='docs/doc[@for="CannotUnloadAppDomainException.CannotUnloadAppDomainException3"]/*' />
        protected CannotUnloadAppDomainException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}







