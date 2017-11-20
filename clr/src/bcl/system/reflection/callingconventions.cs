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
// CallingConventions is a set of Bits representing the calling conventions
//	in the system.
//
// Date: Aug 99
//
namespace System.Reflection {
	using System.Runtime.InteropServices;
	using System;
    /// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions"]/*' />
    [Flags, Serializable]
    public enum CallingConventions
    {
		//NOTE: If you change this please update COMMember.cpp.  These
		//	are defined there.
    	/// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.Standard"]/*' />
    	Standard		= 0x0001,
    	/// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.VarArgs"]/*' />
    	VarArgs			= 0x0002,
    	/// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.Any"]/*' />
    	Any				= Standard | VarArgs,
        /// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.HasThis"]/*' />
        HasThis         = 0x0020,
        /// <include file='doc\CallingConventions.uex' path='docs/doc[@for="CallingConventions.ExplicitThis"]/*' />
        ExplicitThis    = 0x0040,
    }
}
