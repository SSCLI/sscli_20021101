//------------------------------------------------------------------------------
// <copyright file="CodeObject.cs" company="Microsoft">
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

    using System.Collections;
    using System.Collections.Specialized;
    using System.Runtime.Serialization;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeObject.uex' path='docs/doc[@for="CodeObject"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The base class for CodeDom objects
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeObject {
        private IDictionary userData = null;

        /// <include file='doc\CodeObject.uex' path='docs/doc[@for="CodeObject.CodeObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeObject() {
        }

        /// <include file='doc\CodeObject.uex' path='docs/doc[@for="CodeObject.UserData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IDictionary UserData {
            get {
                if (userData == null) {
                    userData = new ListDictionary();
                }
                return userData;
            }
        }
    }
}
