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
namespace System.Runtime.InteropServices {

	using System;
    /** Used for the CallingConvention named argument to the DllImport attribute
     **/
    /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention"]/*' />
	[Serializable]
	public enum CallingConvention
    {
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.Winapi"]/*' />
        Winapi          = 1,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.Cdecl"]/*' />
        Cdecl           = 2,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.StdCall"]/*' />
        StdCall         = 3,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.ThisCall"]/*' />
        ThisCall        = 4,
        /// <include file='doc\CallingConvention.uex' path='docs/doc[@for="CallingConvention.FastCall"]/*' />
        FastCall        = 5,
    }
	
}
