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
// PInvokeMap is an enum that defines the PInvoke attributes.  These
//	values are defined in CorHdr.h.
//
// Date: Sep 99
//
namespace System.Runtime.InteropServices {
	using System.Runtime.InteropServices;
	using System;
	// This Enum matchs the CorPinvokeMap defined in CorHdr.h
	/// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap"]/*' />
    [Serializable()] 
	internal enum PInvokeMap
    {
    	/// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.NoMangle"]/*' />
    	NoMangle			= 0x0001,	// Pinvoke is to use the member name as specified.
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CharSetMask"]/*' />
        CharSetMask			= 0x0006,	// Heuristic used in data type & name mapping.
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CharSetNotSpec"]/*' />
        CharSetNotSpec		= 0x0000,
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CharSetAnsi"]/*' />
        CharSetAnsi			= 0x0002, 
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CharSetUnicode"]/*' />
        CharSetUnicode		= 0x0004,
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CharSetAuto"]/*' />
        CharSetAuto			= 0x0006,
    
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.PinvokeOLE"]/*' />
        PinvokeOLE			= 0x0020,	// Heuristic: pinvoke will return hresult, with return value becoming the retval param. Not relevant for fields. 
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.SupportsLastError"]/*' />
        SupportsLastError	= 0x0040,	// Information about target function. Not relevant for fields.
    
        // None of the calling convention flags is relevant for fields.
		/// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CallConvMask"]/*' />
		CallConvMask		= 0x0700,
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CallConvWinapi"]/*' />
        CallConvWinapi		= 0x0100,	// Pinvoke will use native callconv appropriate to target windows platform.
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CallConvCdecl"]/*' />
        CallConvCdecl		= 0x0200,
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CallConvStdcall"]/*' />
        CallConvStdcall		= 0x0300,
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CallConvThiscall"]/*' />
        CallConvThiscall	= 0x0400,	// In M9, pinvoke will raise exception.
        /// <include file='doc\PInvokeMap.uex' path='docs/doc[@for="PInvokeMap.CallConvFastcall"]/*' />
        CallConvFastcall	= 0x0500,
    }
}
