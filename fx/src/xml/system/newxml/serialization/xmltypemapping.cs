//------------------------------------------------------------------------------
// <copyright file="XmlTypeMapping.cs" company="Microsoft">
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

namespace System.Xml.Serialization {

    using System.Reflection;
    using System;

    /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlTypeMapping : XmlMapping {
        ElementAccessor accessor;

        internal XmlTypeMapping(TypeScope scope, ElementAccessor accessor) : base(scope) { 
            this.accessor = accessor;
        }

        internal ElementAccessor Accessor {
            get { return accessor; }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.ElementName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ElementName { 
            get { return System.Xml.Serialization.Accessor.UnescapeName(Accessor.Name); }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Namespace {
            get { return accessor.Namespace; }
        }

        internal TypeMapping Mapping {
            get { return (TypeMapping)Accessor.Mapping; }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.TypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeName {
            get { return Mapping.TypeDesc.Name; }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.TypeFullName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeFullName {
            get { return Mapping.TypeDesc.FullName; }
        }
    }
}
