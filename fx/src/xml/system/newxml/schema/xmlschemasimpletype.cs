//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSimpleType.cs" company="Microsoft">
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
    using System.Xml;

    /// <include file='doc\XmlSchemaSimpleType.uex' path='docs/doc[@for="XmlSchemaSimpleType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaSimpleType : XmlSchemaType {
        XmlSchemaSimpleTypeContent content;
        
        /// <include file='doc\XmlSchemaSimpleType.uex' path='docs/doc[@for="XmlSchemaSimpleType.Content"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("restriction", typeof(XmlSchemaSimpleTypeRestriction)), 
         XmlElement("list", typeof(XmlSchemaSimpleTypeList)),
         XmlElement("union", typeof(XmlSchemaSimpleTypeUnion))]
        public XmlSchemaSimpleTypeContent Content { 
            get { return content; }
            set { content = value; }
        }

        internal override XmlQualifiedName DerivedFrom {
            get {
                if (content == null) {
                    // type derived from anyType
                    return XmlQualifiedName.Empty;
                }
                if (content is XmlSchemaSimpleTypeRestriction) {
                    return ((XmlSchemaSimpleTypeRestriction)content).BaseTypeName;
                }
                return XmlQualifiedName.Empty;
            }
        }
    }
}

