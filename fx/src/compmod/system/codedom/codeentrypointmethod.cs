//------------------------------------------------------------------------------
// <copyright file="CodeEntryPointMethod.cs" company="Microsoft">
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

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeEntryPointMethod.uex' path='docs/doc[@for="CodeEntryPointMethod"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class method that is the entry point
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeEntryPointMethod : CodeMemberMethod {

        /// <include file='doc\CodeEntryPointMethod.uex' path='docs/doc[@for="CodeEntryPointMethod.CodeEntryPointMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeEntryPointMethod() {
        }
    }
}
