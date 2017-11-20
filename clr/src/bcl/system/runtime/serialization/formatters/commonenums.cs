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
 ** Class: FormatterEnums
 **
 **                                               
 **
 ** Purpose: Soap XML Formatter Enums
 **
 ** Date:  August 23, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters {
	using System.Threading;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System;
    // Enums which specify options to the XML and Binary formatters
    // These will be public so that applications can use them
    /// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle"]/*' />
	[Serializable]
    public enum FormatterTypeStyle
    {
    	/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle.TypesWhenNeeded"]/*' />
    	TypesWhenNeeded = 0, // Types are outputted only for Arrays of Objects, Object Members of type Object, and ISerializable non-primitive value types
    	/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle.TypesAlways"]/*' />
    	TypesAlways = 0x1, // Types are outputted for all Object members and ISerialiable object members.
		/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterTypeStyle.XsdString"]/*' />
		XsdString = 0x2     // Strings are outputed as xsd rather then SOAP-ENC strings. No string ID's are transmitted
    }

	/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterAssemblyStyle"]/*' />
	[Serializable]
	public enum FormatterAssemblyStyle
	{
		/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterAssemblyStyle.Simple"]/*' />
		Simple = 0,
		/// <include file='doc\CommonEnums.uex' path='docs/doc[@for="FormatterAssemblyStyle.Full"]/*' />
		Full = 1,
	}
    
}
