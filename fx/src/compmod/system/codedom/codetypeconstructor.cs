//------------------------------------------------------------------------------
// <copyright file="CodeTypeConstructor.cs" company="Microsoft">
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

    /// <include file='doc\CodeTypeConstructor.uex' path='docs/doc[@for="CodeTypeConstructor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a static constructor for a class.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeTypeConstructor : CodeMemberMethod {
        /// <include file='doc\CodeTypeConstructor.uex' path='docs/doc[@for="CodeTypeConstructor.CodeTypeConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeConstructor'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeConstructor() {
            Name = ".cctor";
        }
    }
}
