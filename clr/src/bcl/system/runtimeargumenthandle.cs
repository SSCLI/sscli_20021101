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
namespace System {
    
	using System;
    //  This value type is used for constructing System.ArgIterator. 
    // 
    //  SECURITY : m_ptr cannot be set to anything other than null by untrusted
    //  code.  
    // 
    //  This corresponds to EE VARARGS cookie.

	 // Cannot be serialized
    /// <include file='doc\RuntimeArgumentHandle.uex' path='docs/doc[@for="RuntimeArgumentHandle"]/*' />
    public struct RuntimeArgumentHandle
    {
        private IntPtr m_ptr;

 		// This method should never be called.  Its sole purpose is to shut up the compiler
		//	because it warns about private fields that are never used.  Most of these fields
		//	are used in unmanaged code.
#if _DEBUG
		internal IntPtr  NeverCallThis()
		{
			m_ptr = (IntPtr)0;
			BCLDebug.Assert(false,"NeverCallThis");
			return m_ptr;
		}
#endif
    }

}
