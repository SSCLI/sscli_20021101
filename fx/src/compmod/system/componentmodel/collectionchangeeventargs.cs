//------------------------------------------------------------------------------
// <copyright file="CollectionChangeEventArgs.cs" company="Microsoft">
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

namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\CollectionChangeEventArgs.uex' path='docs/doc[@for="CollectionChangeEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see langword='CollectionChange '/> event.</para>
    /// </devdoc>
    public class CollectionChangeEventArgs : EventArgs {
        private CollectionChangeAction action;
        private object element;

        /// <include file='doc\CollectionChangeEventArgs.uex' path='docs/doc[@for="CollectionChangeEventArgs.CollectionChangeEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.CollectionChangeEventArgs'/> class.</para>
        /// </devdoc>
        public CollectionChangeEventArgs(CollectionChangeAction action, object element) {
            this.action = action;
            this.element = element;
        }

        /// <include file='doc\CollectionChangeEventArgs.uex' path='docs/doc[@for="CollectionChangeEventArgs.Action"]/*' />
        /// <devdoc>
        ///    <para>Gets an action that specifies how the collection changed.</para>
        /// </devdoc>
        public virtual CollectionChangeAction Action {
            get {
                return action;
            }
        }

        /// <include file='doc\CollectionChangeEventArgs.uex' path='docs/doc[@for="CollectionChangeEventArgs.Element"]/*' />
        /// <devdoc>
        ///    <para>Gets the instance of the collection with the change. </para>
        /// </devdoc>
        public virtual object Element {
            get {
                return element;
            }
        }
    }
}
