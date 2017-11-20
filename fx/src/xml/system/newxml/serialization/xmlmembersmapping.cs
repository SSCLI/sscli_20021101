//------------------------------------------------------------------------------
// <copyright file="XmlMembersMapping.cs" company="Microsoft">
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

    /// <include file='doc\XmlMembersMapping.uex' path='docs/doc[@for="XmlMembersMapping"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlMembersMapping : XmlMapping {
        XmlMemberMapping[] mappings;
        ElementAccessor accessor;

        internal XmlMembersMapping(TypeScope scope, ElementAccessor accessor) : base(scope) {
            this.accessor = accessor;
            MembersMapping mapping = (MembersMapping)accessor.Mapping;
            mappings = new XmlMemberMapping[mapping.Members.Length];
            for (int i = 0; i < mappings.Length; i++)
                mappings[i] = new XmlMemberMapping(scope, mapping.Members[i]);
        }

        internal ElementAccessor Accessor {
            get { return accessor; }
        }

        /// <include file='doc\XmlMembersMapping.uex' path='docs/doc[@for="XmlMembersMapping.ElementName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ElementName { 
            get { return System.Xml.Serialization.Accessor.UnescapeName(Accessor.Name); }
        }

        /// <include file='doc\XmlMembersMapping.uex' path='docs/doc[@for="XmlMembersMapping.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Namespace {
            get { return accessor.Namespace; }
        }

        /// <include file='doc\XmlMembersMapping.uex' path='docs/doc[@for="XmlMembersMapping.TypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeName {
            get { return accessor.Mapping.TypeName; }
        }

        /// <include file='doc\XmlMembersMapping.uex' path='docs/doc[@for="XmlMembersMapping.TypeNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeNamespace {
            get { return accessor.Mapping.Namespace; }
        }

        /// <include file='doc\XmlMembersMapping.uex' path='docs/doc[@for="XmlMembersMapping.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlMemberMapping this[int index] {
            get { return mappings[index]; }
        }

        /// <include file='doc\XmlMembersMapping.uex' path='docs/doc[@for="XmlMembersMapping.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get { return mappings.Length; }
        }
    }
 
}
