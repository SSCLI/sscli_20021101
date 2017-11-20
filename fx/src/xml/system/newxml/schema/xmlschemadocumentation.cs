//------------------------------------------------------------------------------
// <copyright file="XmlSchemaDocumentation.cs" company="Microsoft">
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

    /// <include file='doc\XmlSchemaDocumentation.uex' path='docs/doc[@for="XmlSchemaDocumentation"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaDocumentation : XmlSchemaObject {    
        string source;
        string language;
        XmlNode[] markup;
        static XmlSchemaDatatype languageDatatype = XmlSchemaDatatype.FromTypeName("language");

        /// <include file='doc\XmlSchemaDocumentation.uex' path='docs/doc[@for="XmlSchemaDocumentation.Source"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("source", DataType="anyURI")]
        public string Source {
            get { return source; }
            set { source = value; }
        }

        /// <include file='doc\XmlSchemaDocumentation.uex' path='docs/doc[@for="XmlSchemaDocumentation.Language"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("xml:lang")]
        public string Language {
            get { return language; }
            set { language = (string)languageDatatype.ParseValue(value, null, null); }
        }

        /// <include file='doc\XmlSchemaDocumentation.uex' path='docs/doc[@for="XmlSchemaDocumentation.Markup"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlText(), XmlAnyElement]
        public XmlNode[] Markup {
            get { return markup; }
            set { markup = value; }
        }
    }
}
