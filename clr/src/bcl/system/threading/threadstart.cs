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
/*=============================================================================
**
** Class: ThreadStart
**
**                              
**
** Purpose: This class is a Delegate which defines the start method
**	for starting a thread.  That method must match this delegate.
**
** Date: August 1998
**
=============================================================================*/

namespace System.Threading {
	using System.Threading;

    // Define the delgate
    // NOTE: If you change the signature here, there is code in COMSynchronization
    //	that invokes this delegate in native.
    /// <include file='doc\ThreadStart.uex' path='docs/doc[@for="ThreadStart"]/*' />
    public delegate void ThreadStart();
}
