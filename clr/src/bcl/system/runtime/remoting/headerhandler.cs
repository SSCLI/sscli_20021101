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
** Class: HeaderHandler
**
**                                        
**
** Purpose: The delegate used to process headers on the stream
** during deserialization.
**
** Date:  August 9, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
	using System.Runtime.Remoting;
    //Define the required delegate
    /// <include file='doc\HeaderHandler.uex' path='docs/doc[@for="HeaderHandler"]/*' />
    public delegate Object HeaderHandler(Header[] headers);
}
