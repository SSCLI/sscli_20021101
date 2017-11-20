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
** Class: AppDomainUnloadInProgressException
**
**                                                
**
** Purpose: Exception class for attempt unload multiple AppDomains 
**		 	simultaneously
**
** Date: March 17, 1998
**
=============================================================================*/

namespace System {

	using System.Runtime.Serialization;

    /// <include file='doc\AppDomainUnloadInProgressException.uex' path='docs/doc[@for="AppDomainUnloadInProgressException"]/*' />
    [Serializable()] public class AppDomainUnloadInProgressException : SystemException {
        /// <include file='doc\AppDomainUnloadInProgressException.uex' path='docs/doc[@for="AppDomainUnloadInProgressException.AppDomainUnloadInProgressException"]/*' />
        public AppDomainUnloadInProgressException() 
            : base(Environment.GetResourceString("Arg_CannotUnloadAppDomainException")) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }
    
        /// <include file='doc\AppDomainUnloadInProgressException.uex' path='docs/doc[@for="AppDomainUnloadInProgressException.AppDomainUnloadInProgressException1"]/*' />
        public AppDomainUnloadInProgressException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }
    
        /// <include file='doc\AppDomainUnloadInProgressException.uex' path='docs/doc[@for="AppDomainUnloadInProgressException.AppDomainUnloadInProgressException2"]/*' />
        public AppDomainUnloadInProgressException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_CANNOTUNLOADAPPDOMAIN);
        }

        //
        // This constructor is required for serialization
        //
        /// <include file='doc\AppDomainUnloadInProgressException.uex' path='docs/doc[@for="AppDomainUnloadInProgressException.AppDomainUnloadInProgressException3"]/*' />
        protected AppDomainUnloadInProgressException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}


