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
** Class:  PathTooLongException
**
**                                                    
**
** Purpose: Exception for paths and/or filenames that are 
** too long.
**
** Date:   March 24, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;

namespace System.IO {

    /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException"]/*' />
    [Serializable]
    public class PathTooLongException : IOException
    {
        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException"]/*' />
        public PathTooLongException() 
            : base(Environment.GetResourceString("IO.PathTooLong")) {
            SetErrorCode(__HResults.COR_E_PATHTOOLONG);
        }
        
        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException1"]/*' />
        public PathTooLongException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_PATHTOOLONG);
        }
        
        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException2"]/*' />
        public PathTooLongException(String message, Exception innerException) 
            : base(message, innerException) {
            SetErrorCode(__HResults.COR_E_PATHTOOLONG);
        }

        /// <include file='doc\PathTooLongException.uex' path='docs/doc[@for="PathTooLongException.PathTooLongException3"]/*' />
        protected PathTooLongException(SerializationInfo info, StreamingContext context) : base (info, context) {
        }
    }
}
