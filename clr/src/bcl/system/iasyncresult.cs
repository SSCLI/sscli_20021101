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
** Interface: IAsyncResult
**
** Purpose: Interface to encapsulate the results of an async
**          operation
**
===========================================================*/
namespace System {
    
	using System;
	using System.Threading;
    /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult"]/*' />
    public interface IAsyncResult
    {
        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.IsCompleted"]/*' />
        bool IsCompleted { get; }

        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.AsyncWaitHandle"]/*' />
        WaitHandle AsyncWaitHandle { get; }


        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.AsyncState"]/*' />
        Object     AsyncState      { get; }

        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.CompletedSynchronously"]/*' />
        bool       CompletedSynchronously { get; }
   
    
    }

}
