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
** Class: MissingFieldException
**
** Purpose: The exception class for class loading failures.
**
=============================================================================*/

namespace System {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System.Runtime.CompilerServices;
    /// <include file='doc\MissingFieldException.uex' path='docs/doc[@for="MissingFieldException"]/*' />
    [Serializable()] public class MissingFieldException : MissingMemberException, ISerializable {
        /// <include file='doc\MissingFieldException.uex' path='docs/doc[@for="MissingFieldException.MissingFieldException"]/*' />
        public MissingFieldException() 
            : base(Environment.GetResourceString("Arg_MissingFieldException")) {
    		SetErrorCode(__HResults.COR_E_MISSINGFIELD);
        }
    
        /// <include file='doc\MissingFieldException.uex' path='docs/doc[@for="MissingFieldException.MissingFieldException1"]/*' />
        public MissingFieldException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_MISSINGFIELD);
        }
    
        /// <include file='doc\MissingFieldException.uex' path='docs/doc[@for="MissingFieldException.MissingFieldException2"]/*' />
        public MissingFieldException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_MISSINGFIELD);
        }

        /// <include file='doc\MissingFieldException.uex' path='docs/doc[@for="MissingFieldException.MissingFieldException3"]/*' />
        protected MissingFieldException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    
    	/// <include file='doc\MissingFieldException.uex' path='docs/doc[@for="MissingFieldException.Message"]/*' />
    	public override String Message
        {
    		get {
    	        if (ClassName == null) {
    		        return base.Message;
    			} else {
    				// do any desired fixups to classname here.
                    return String.Format(Environment.GetResourceString("MissingField_Name",
                                                                       (Signature != null ? FormatSignature(Signature) + " " : "") +
                                                                       ClassName + "." + MemberName));
    		    }
    		}
        }
    
        // Called to format signature
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String FormatSignature(byte [] signature);
    
    
    
        // Called from the EE
        private MissingFieldException(String className, String fieldName, byte[] signature)
        {
            ClassName   = className;
            MemberName  = fieldName;
            Signature   = signature;
        }
    
        /// <include file='doc\MissingFieldException.uex' path='docs/doc[@for="MissingFieldException.MissingFieldException4"]/*' />
        public MissingFieldException(String className, String fieldName)
        {
            ClassName   = className;
            MemberName  = fieldName;
        }
    
        // If ClassName != null, Message will construct on the fly using it
        // and the other variables. This allows customization of the
        // format depending on the language environment.
    }
}
