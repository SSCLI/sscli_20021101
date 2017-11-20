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
** Enum:   SeekOrigin
**
**                                                    
**
** Purpose: Enum describing locations in a stream you could
** seek relative to.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;

namespace System.IO {
    // Provides seek reference points.  To seek to the end of a stream,
    // call stream.Seek(0, SeekOrigin.End).
    /// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin"]/*' />
	  [Serializable()]
    public enum SeekOrigin
    {
    	// These constants match Win32's FILE_BEGIN, FILE_CURRENT, and FILE_END
    	/// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin.Begin"]/*' />
    	Begin = 0,
    	/// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin.Current"]/*' />
    	Current = 1,
    	/// <include file='doc\SeekOrigin.uex' path='docs/doc[@for="SeekOrigin.End"]/*' />
    	End = 2,
    }
}
