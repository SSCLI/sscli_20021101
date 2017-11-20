//------------------------------------------------------------------------------
// <copyright file="FieldDirection.cs" company="Microsoft">
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
    using System.Runtime.InteropServices;

    /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies values used to indicate field and parameter directions.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public enum FieldDirection {
        /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection.In"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Incoming field.
        ///    </para>
        /// </devdoc>
        In,
        /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection.Out"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outgoing field.
        ///    </para>
        /// </devdoc>
        Out,
        /// <include file='doc\FieldDirection.uex' path='docs/doc[@for="FieldDirection.Ref"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Field by reference.
        ///    </para>
        /// </devdoc>
        Ref,
    }
}
