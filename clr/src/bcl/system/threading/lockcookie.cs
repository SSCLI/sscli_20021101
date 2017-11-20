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
** Class:    LockCookie
**
**                                                 
**
** Purpose: Defines the lock that implements 
**          single-writer/multiple-reader semantics
**
** Date:    Feb 21, 1999
**
===========================================================*/

namespace System.Threading {

	using System;
	
	/// <include file='doc\LockCookie.uex' path='docs/doc[@for="LockCookie"]/*' />
    [Serializable()] 
	public struct LockCookie
    {
        private int _dwFlags;
        private int _dwWriterSeqNum;
        private int _wReaderAndWriterLevel;
        private int _dwThreadID;

 		// This method should never be called.  Its sole purpose is to shut up the compiler
		//	because it warns about private fields that are never used.  Most of these fields
		//	are used in unmanaged code.
#if _DEBUG
		internal int NeverCallThis()
		{
			BCLDebug.Assert(false,"NeverCallThis");
			int i = _dwFlags = _dwFlags;
			i = _dwWriterSeqNum = _dwWriterSeqNum;
			i = _wReaderAndWriterLevel = _wReaderAndWriterLevel;
			return _dwThreadID=_dwThreadID;
		}
#endif
    }

}
