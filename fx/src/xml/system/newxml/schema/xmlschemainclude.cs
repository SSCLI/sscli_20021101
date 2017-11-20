//------------------------------------------------------------------------------
// <copyright file="XmlSchemaInclude.cs" company="Microsoft">
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

    /// <include file='doc\XmlSchemaInclude.uex' path='docs/doc[@for="XmlSchemaInclude"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaInclude : XmlSchemaExternal {
        XmlSchemaAnnotation annotation;

        /// <include file='doc\XmlSchemaInclude.uex' path='docs/doc[@for="XmlSchemaInclude.Annotation"]/*' />
        [XmlElement("annotation", typeof(XmlSchemaAnnotation))]
        public XmlSchemaAnnotation Annotation {
            get { return annotation; }
            set { annotation = value; }
        }

        internal override void AddAnnotation(XmlSchemaAnnotation annotation) {
            this.annotation = annotation;
        }
    }
}
