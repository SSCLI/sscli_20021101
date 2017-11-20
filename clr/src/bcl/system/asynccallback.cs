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
** Interface: AsyncCallbackDelegate
**
** Purpose: Type of callback for async operations
**
===========================================================*/
namespace System {
    /// <include file='doc\AsyncCallback.uex' path='docs/doc[@for="AsyncCallback"]/*' />
	[Serializable()]    
    public delegate void AsyncCallback(IAsyncResult ar);

}
