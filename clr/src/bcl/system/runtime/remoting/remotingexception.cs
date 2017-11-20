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
** Class: RemotingException
**
**                                  
**
** Purpose: Exception class for remoting 
**
** Date: July 15, 1999
**
=============================================================================*/

namespace System.Runtime.Remoting
{
	using System.Runtime.Remoting;
	using System;
	using System.Runtime.Serialization;
	// The Exception thrown when something has gone
	// wrong during remoting
	// 
	/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingException"]/*' />
	[Serializable()]
	public class RemotingException : SystemException {

		private static String _nullMessage = Environment.GetResourceString("Remoting_Default");

		// Creates a new RemotingException with its message 
		// string set to a default message.
		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingException.RemotingException"]/*' />
		public RemotingException() 
				: base(_nullMessage) {
			SetErrorCode(__HResults.COR_E_REMOTING);
		}

		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingException.RemotingException1"]/*' />
		public RemotingException(String message) 
				: base(message) {
			SetErrorCode(__HResults.COR_E_REMOTING);
		}

		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingException.RemotingException2"]/*' />

		public RemotingException(String message, Exception InnerException) 
				: base(message, InnerException) {
			SetErrorCode(__HResults.COR_E_REMOTING);
		}	

        /// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingException.RemotingException3"]/*' />
        protected RemotingException(SerializationInfo info, StreamingContext context) : base(info, context) {}
	}




	// The Exception thrown when something has gone
	// wrong on the server during remoting. This exception is thrown
	// on the client.
	// 
	/// <include file='doc\RemotingException.uex' path='docs/doc[@for="ServerException"]/*' />
	[Serializable()]
	public class ServerException : SystemException {
		private static String _nullMessage = Environment.GetResourceString("Remoting_Default");
		// Creates a new ServerException with its message 
		// string set to a default message.
		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="ServerException.ServerException"]/*' />
		public ServerException()
				: base(_nullMessage) {
			SetErrorCode(__HResults.COR_E_SERVER);			
		}

		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="ServerException.ServerException1"]/*' />
		public ServerException(String message) 
				: base(message) {
			SetErrorCode(__HResults.COR_E_SERVER);
		}

		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="ServerException.ServerException2"]/*' />

		public ServerException(String message, Exception InnerException) 
				: base(message, InnerException) {
			SetErrorCode(__HResults.COR_E_SERVER);
		}	

		internal ServerException(SerializationInfo info, StreamingContext context) : base(info, context) {}
	}


	/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingTimeoutException"]/*' />
	[Serializable()]
	public class RemotingTimeoutException : RemotingException {

		private static String _nullMessage = Environment.GetResourceString("Remoting_Default");

		// Creates a new RemotingException with its message 
		// string set to a default message.
		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingTimeoutException.RemotingTimeoutException"]/*' />
		public RemotingTimeoutException() : base(_nullMessage) 
		{
		}

		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingTimeoutException.RemotingTimeoutException1"]/*' />
		public RemotingTimeoutException(String message) : base(message) {
			SetErrorCode(__HResults.COR_E_REMOTING);
		}

		/// <include file='doc\RemotingException.uex' path='docs/doc[@for="RemotingTimeoutException.RemotingTimeoutException2"]/*' />

		public RemotingTimeoutException(String message, Exception InnerException) 
            : base(message, InnerException) 
        {
            SetErrorCode(__HResults.COR_E_REMOTING);
        }

        internal RemotingTimeoutException(SerializationInfo info, StreamingContext context) : base(info, context) {}
        
    } // RemotingTimeoutException

}
