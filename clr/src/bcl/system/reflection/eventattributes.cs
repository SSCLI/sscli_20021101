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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// EventAttributes are an enum defining the attributes associated with
//	and Event.  These are defined in CorHdr.h and are a combination of
//	bits and enums.
//
// Date: Aug 99
//
namespace System.Reflection {
    
	using System;
    /// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes"]/*' />
	[Serializable()] 
    public enum EventAttributes
    {
    	/// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.None"]/*' />
    	None			=   0x0000,

		// This Enum matchs the CorEventAttr defined in CorHdr.h
        /// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.SpecialName"]/*' />
        SpecialName       =   0x0200,     // event is special.  Name describes how.

		// Reserved flags for Runtime use only.
		/// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.ReservedMask"]/*' />
		ReservedMask          =   0x0400,
		/// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.RTSpecialName"]/*' />
		RTSpecialName         =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
    }
}
