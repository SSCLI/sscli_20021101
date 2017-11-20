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
 *
 * Class:  IsolatedStorageException
 *
 *                                    
 *
 * Purpose: The exceptions in IsolatedStorage
 *
 * Date:  Feb 15, 2000
 *
 ===========================================================*/
namespace System.IO.IsolatedStorage {

	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException"]/*' />
    [Serializable()]
    public class IsolatedStorageException : Exception
    {
        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException"]/*' />
        public IsolatedStorageException()
            : base(Environment.GetResourceString("IsolatedStorage_Exception"))
        {
            SetErrorCode(__HResults.COR_E_ISOSTORE);
        }

        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException1"]/*' />
        public IsolatedStorageException(String message)
            : base(message)
        {
            SetErrorCode(__HResults.COR_E_ISOSTORE);
        }

        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException2"]/*' />
        public IsolatedStorageException(String message, Exception inner)
            : base(message, inner)
        {
            SetErrorCode(__HResults.COR_E_ISOSTORE);
        }

        /// <include file='doc\IsolatedStorageException.uex' path='docs/doc[@for="IsolatedStorageException.IsolatedStorageException3"]/*' />
        protected IsolatedStorageException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
