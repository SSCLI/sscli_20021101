//------------------------------------------------------------------------------
// <copyright file="XmlSchemaAttributeGroup.cs" company="Microsoft">
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

    /// <include file='doc\XmlSchemaAttributeGroup.uex' path='docs/doc[@for="XmlSchemaAttributeGroup"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaAttributeGroup : XmlSchemaAnnotated {
        string name;        
        XmlSchemaObjectCollection attributes = new XmlSchemaObjectCollection();
        XmlSchemaAnyAttribute anyAttribute;
        XmlQualifiedName qname = XmlQualifiedName.Empty; 
        XmlSchemaAttributeGroup redefined;
        XmlSchemaObjectTable attributeUses = new XmlSchemaObjectTable();
        XmlSchemaAnyAttribute attributeWildcard;

        bool validating;

        /// <include file='doc\XmlSchemaAttributeGroup.uex' path='docs/doc[@for="XmlSchemaAttributeGroup.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("name")]
        public string Name { 
            get { return name; }
            set { name = value; }
        }

        /// <include file='doc\XmlSchemaAttributeGroup.uex' path='docs/doc[@for="XmlSchemaAttributeGroup.Attributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("attribute", typeof(XmlSchemaAttribute)),
         XmlElement("attributeGroup", typeof(XmlSchemaAttributeGroupRef))]
        public XmlSchemaObjectCollection Attributes {
            get { return attributes; }
        }

        /// <include file='doc\XmlSchemaAttributeGroup.uex' path='docs/doc[@for="XmlSchemaAttributeGroup.AnyAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("anyAttribute")]
        public XmlSchemaAnyAttribute AnyAttribute {
            get { return anyAttribute; }
            set { anyAttribute = value; }
        }

        [XmlIgnore]
        internal XmlQualifiedName QualifiedName {
            get { return qname; }
            set { qname = value; }
        }

        [XmlIgnore]
        internal XmlSchemaObjectTable AttributeUses {
            get { return attributeUses; }
        }

        [XmlIgnore]
        internal XmlSchemaAnyAttribute AttributeWildcard {
            get { return attributeWildcard; }
            set { attributeWildcard = value; }
        }

        [XmlIgnore]
        public XmlSchemaAttributeGroup RedefinedAttributeGroup {
            get { return redefined; }
        }

        [XmlIgnore]
        internal XmlSchemaAttributeGroup Redefined {
            get { return redefined; }
            set { redefined = value; }
        }

        [XmlIgnore]
        internal bool Validating {
            get { return validating; }
            set { validating = value; }
        }
        
        [XmlIgnore]
        internal override string NameAttribute {
            get { return Name; }
            set { Name = value; }
        }
    }

}
