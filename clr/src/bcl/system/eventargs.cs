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
    // The base class for all event classes.
    /// <include file='doc\EventArgs.uex' path='docs/doc[@for="EventArgs"]/*' />
	[Serializable]
    public class EventArgs {
        /// <include file='doc\EventArgs.uex' path='docs/doc[@for="EventArgs.Empty"]/*' />
        public static readonly EventArgs Empty = new EventArgs();
    
        /// <include file='doc\EventArgs.uex' path='docs/doc[@for="EventArgs.EventArgs"]/*' />
        public EventArgs() 
        {
        }
    }
}
