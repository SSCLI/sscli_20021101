//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSequence.cs" company="Microsoft">
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

    /// <include file='doc\XmlSchemaSequence.uex' path='docs/doc[@for="XmlSchemaSequence"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaSequence : XmlSchemaGroupBase {
        XmlSchemaObjectCollection items = new XmlSchemaObjectCollection();

        /// <include file='doc\XmlSchemaSequence.uex' path='docs/doc[@for="XmlSchemaSequence.Items"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("element", typeof(XmlSchemaElement)),
         XmlElement("group", typeof(XmlSchemaGroupRef)),
         XmlElement("choice", typeof(XmlSchemaChoice)),
         XmlElement("sequence", typeof(XmlSchemaSequence)),
         XmlElement("any", typeof(XmlSchemaAny))]
        public override  XmlSchemaObjectCollection Items {
            get { return items; }
        }

        internal override bool IsEmpty {
            get { return base.IsEmpty || items.Count == 0; }
        }
    }
}
