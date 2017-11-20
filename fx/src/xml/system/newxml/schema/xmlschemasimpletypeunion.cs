//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSimpleTypeUnion.cs" company="Microsoft">
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

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaSimpleTypeUnion.uex' path='docs/doc[@for="XmlSchemaSimpleTypeUnion"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaSimpleTypeUnion : XmlSchemaSimpleTypeContent {
        XmlSchemaObjectCollection baseTypes = new XmlSchemaObjectCollection();
        XmlQualifiedName[] memberTypes;

        /// <include file='doc\XmlSchemaSimpleTypeUnion.uex' path='docs/doc[@for="XmlSchemaSimpleTypeUnion.BaseTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("simpleType", typeof(XmlSchemaSimpleType))]
        public XmlSchemaObjectCollection BaseTypes {
            get { return baseTypes; }
        }

        /// <include file='doc\XmlSchemaSimpleTypeUnion.uex' path='docs/doc[@for="XmlSchemaSimpleTypeUnion.MemberTypes"]/*' />
        [XmlAttribute("memberTypes")]
        public XmlQualifiedName[] MemberTypes {
            get { return memberTypes; }
            set { memberTypes = value; }
        }
    }
}

