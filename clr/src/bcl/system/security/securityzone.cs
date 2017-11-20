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
//  SecurityZone.cs
//
//  Enumeration of the zones code can come from
//

namespace System.Security {
	using System;
    /// <include file='doc\SecurityZone.uex' path='docs/doc[@for="SecurityZone"]/*' />
	[Serializable]
    public enum SecurityZone
    {
        // Note: this information is referenced in $/Com99/src/vm/security.cpp
        // in ApplicationSecurityDescriptor.GetEvidence().
    
        /// <include file='doc\SecurityZone.uex' path='docs/doc[@for="SecurityZone.MyComputer"]/*' />
        MyComputer = 0,
        /// <include file='doc\SecurityZone.uex' path='docs/doc[@for="SecurityZone.Intranet"]/*' />
        Intranet     = 1,
        /// <include file='doc\SecurityZone.uex' path='docs/doc[@for="SecurityZone.Trusted"]/*' />
        Trusted      = 2,
        /// <include file='doc\SecurityZone.uex' path='docs/doc[@for="SecurityZone.Internet"]/*' />
        Internet     = 3,
        /// <include file='doc\SecurityZone.uex' path='docs/doc[@for="SecurityZone.Untrusted"]/*' />
        Untrusted    = 4,
    
        /// <include file='doc\SecurityZone.uex' path='docs/doc[@for="SecurityZone.NoZone"]/*' />
        NoZone       = -1,  // No Zone Information
    }
}
