//------------------------------------------------------------------------------
// <copyright file="RegexRunnerFactory.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>                                                                
//------------------------------------------------------------------------------

// This RegexRunnerFactory class is a base class for compiled regex code.
// we need to compile a factory because Type.CreateInstance is much slower
// than calling the constructor directly.
//

namespace System.Text.RegularExpressions {

    using System.ComponentModel;
    
    /// <include file='doc\RegexRunnerFactory.uex' path='docs/doc[@for="RegexRunnerFactory"]/*' />
    /// <internalonly/>
    abstract public class RegexRunnerFactory {
        /// <include file='doc\RegexRunnerFactory.uex' path='docs/doc[@for="RegexRunnerFactory.RegexRunnerFactory"]/*' />
        protected RegexRunnerFactory() {}
        /// <include file='doc\RegexRunnerFactory.uex' path='docs/doc[@for="RegexRunnerFactory.CreateInstance"]/*' />
        abstract protected internal RegexRunner CreateInstance();
    }

}
