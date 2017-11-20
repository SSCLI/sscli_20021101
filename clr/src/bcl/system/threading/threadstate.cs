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
** Class: ThreadState
**
**                                                        
**
** Purpose: Enum to represent the different thread states
**
** Date: Feb 2, 2000
**
=============================================================================*/

namespace System.Threading {

    /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState"]/*' />
	[Serializable(),Flags]
    public enum ThreadState
    {   
        /*=========================================================================
        ** Constants for thread states.
        =========================================================================*/
		/// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Running"]/*' />
		Running = 0,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.StopRequested"]/*' />
        StopRequested = 1,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.SuspendRequested"]/*' />
        SuspendRequested = 2,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Background"]/*' />
        Background = 4,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Unstarted"]/*' />
        Unstarted = 8,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Stopped"]/*' />
        Stopped = 16,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.WaitSleepJoin"]/*' />
        WaitSleepJoin = 32,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Suspended"]/*' />
        Suspended = 64,
		/// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.AbortRequested"]/*' />
		AbortRequested = 128,
		/// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Aborted"]/*' />
		Aborted = 256
    }
}
