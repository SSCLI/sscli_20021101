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
//  IEvidenceFactory.cs
//

namespace System.Security {
    using System.Runtime.Remoting;
    using System;
    using System.Security.Policy;
    /// <include file='doc\IEvidenceFactory.uex' path='docs/doc[@for="IEvidenceFactory"]/*' />
    public interface IEvidenceFactory
    {
        /// <include file='doc\IEvidenceFactory.uex' path='docs/doc[@for="IEvidenceFactory.Evidence"]/*' />
        Evidence Evidence
        {
            get;
        }
    }

}
