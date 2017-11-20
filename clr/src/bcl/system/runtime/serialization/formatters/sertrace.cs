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
 ** Class: SerTrace
 **
 **                                               
 **
 ** Purpose: Routine used for Debugging
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters {
	using System;	
	using System.Runtime.Serialization;
	using System.Security.Permissions;
	using System.Reflection;
    using System.Diagnostics;

        // To turn on tracing, add the following to the per-machine
        // rotor.ini file, inside the [Rotor] section:
        //     ManagedLogFacility=0x32
        // where:
	// 0x2 is System.Runtime.Serialization
	// 0x10 is Binary Formatter
	// 0x20 is Soap Formatter
	//
	// Turn on Logging in the jitmgr


    // remoting Wsdl logging
    /// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalRM"]/*' />
    /// <internalonly/>
    [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x" + AssemblyRef.EcmaPublicKeyFull, Name="System.Runtime.Remoting" )]
    public sealed class InternalRM
    {
		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalRM.InfoSoap"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("_LOGGING")]
		public static void InfoSoap(params Object[]messages)
		{
			BCLDebug.Trace("SOAP", messages);
		}

        //[System.Diagnostics.Conditional("_LOGGING")]		
		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalRM.SoapCheckEnabled"]/*' />
        /// <internalonly/>
		public static bool SoapCheckEnabled()
        {
                return BCLDebug.CheckEnabled("SOAP");
        }
    }

	/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST"]/*' />
    /// <internalonly/>
    [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x" + AssemblyRef.MicrosoftPublicKeyFull, Name="System.Runtime.Serialization.Formatters.Soap" )]
	public sealed class InternalST
	{
		private InternalST()
		{
		}

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.InfoSoap"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("_LOGGING")]
		public static void InfoSoap(params Object[]messages)
		{
			BCLDebug.Trace("SOAP", messages);
		}

        //[System.Diagnostics.Conditional("_LOGGING")]		
		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.SoapCheckEnabled"]/*' />
        /// <internalonly/>
		public static bool SoapCheckEnabled()
        {
                return BCLDebug.CheckEnabled("Soap");
        }

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.Soap"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("SER_LOGGING")]		
		public static void Soap(params Object[]messages)
		{
			if (!(messages[0] is String))
				messages[0] = (messages[0].GetType()).Name+" ";
			else
				messages[0] = messages[0]+" ";				

			BCLDebug.Trace("SOAP",messages);								
		}

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.SoapAssert"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("_DEBUG")]		
		public static void SoapAssert(bool condition, String message)
		{
			BCLDebug.Assert(condition, message);
		}

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.SerializationSetValue"]/*' />
        /// <internalonly/>
		public static void SerializationSetValue(FieldInfo fi, Object target, Object value)
		{
                        if ( fi == null)
                            throw new ArgumentNullException("fi");
                        
                        if (target == null)
                            throw new ArgumentNullException("target");
                        
                        if (value == null)
                            throw new ArgumentNullException("value");
                        
                        FormatterServices.SerializationSetValue(fi, target, value);
                }

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.LoadAssemblyFromString"]/*' />
        /// <internalonly/>
		public static Assembly LoadAssemblyFromString(String assemblyString)
		{
			return FormatterServices.LoadAssemblyFromString(assemblyString);
		}
	}

	internal sealed class SerTrace
	{
		internal SerTrace()
		{
		}

		[Conditional("_LOGGING")]
		internal static void InfoLog(params Object[]messages)
		{
			BCLDebug.Trace("BINARY", messages);
		}

		[Conditional("SER_LOGGING")]			
		internal static void Log(params Object[]messages)
		{
			if (!(messages[0] is String))
				messages[0] = (messages[0].GetType()).Name+" ";
			else
				messages[0] = messages[0]+" ";								
			BCLDebug.Trace("BINARY",messages);
		}
	}
}


