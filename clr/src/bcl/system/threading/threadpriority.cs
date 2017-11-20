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
** Class: ThreadPriority
**
**                                                        
**
** Purpose: Enums for the priorities of a Thread
**
** Date: Feb 2, 2000
**
=============================================================================*/

namespace System.Threading {
	using System.Threading;

    /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority"]/*' />
	[Serializable()]
    public enum ThreadPriority
    {   
        /*=========================================================================
        ** Constants for thread priorities.
        =========================================================================*/
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.Lowest"]/*' />
        Lowest = 0,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.BelowNormal"]/*' />
        BelowNormal = 1,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.Normal"]/*' />
        Normal = 2,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.AboveNormal"]/*' />
        AboveNormal = 3,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.Highest"]/*' />
        Highest = 4
    
    }
}
