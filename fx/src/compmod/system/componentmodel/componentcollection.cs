//------------------------------------------------------------------------------
// <copyright file="ComponentCollection.cs" company="Microsoft">
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
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    
    /** The component in the container identified by name. */
    /// <include file='doc\ComponentCollection.uex' path='docs/doc[@for="ComponentCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Gets a specific <see cref='System.ComponentModel.Component'/> in the <see cref='System.ComponentModel.Container'/>
    ///       .
    ///    </para>
    /// </devdoc>
    public class ComponentCollection : ReadOnlyCollectionBase {
        /// <include file='doc\ComponentCollection.uex' path='docs/doc[@for="ComponentCollection.ComponentCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ComponentCollection(IComponent[] components) {
            InnerList.AddRange(components);
        }

        /** The component in the container identified by name. */
        /// <include file='doc\ComponentCollection.uex' path='docs/doc[@for="ComponentCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a specific <see cref='System.ComponentModel.Component'/> in the <see cref='System.ComponentModel.Container'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public virtual IComponent this[string name] {
            get {
                if (name != null) {
                    IList list = InnerList;
                    foreach(IComponent comp in list) {
                        if (comp != null && comp.Site != null && comp.Site.Name != null && string.Compare(comp.Site.Name, name, true, CultureInfo.InvariantCulture) == 0) {
                            return comp;
                        }
                    }
                }
                return null;
            }
        }
        
        /** The component in the container identified by index. */
        /// <include file='doc\ComponentCollection.uex' path='docs/doc[@for="ComponentCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a specific <see cref='System.ComponentModel.Component'/> in the <see cref='System.ComponentModel.Container'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public virtual IComponent this[int index] {
            get {
                return (IComponent)InnerList[index];
            }
        }
        
        /// <include file='doc\ComponentCollection.uex' path='docs/doc[@for="ComponentCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(IComponent[] array, int index) {
            InnerList.CopyTo(array, index);
        }
    }
}

