//------------------------------------------------------------------------------
// <copyright file="XmlSchemaAny.cs" company="Microsoft">
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

    /// <include file='doc\XmlSchemaAny.uex' path='docs/doc[@for="XmlSchemaAny"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaAny : XmlSchemaParticle {
        string ns;
        XmlSchemaContentProcessing processContents = XmlSchemaContentProcessing.None;
        NamespaceList namespaceList;
        
        /// <include file='doc\XmlSchemaAny.uex' path='docs/doc[@for="XmlSchemaAny.Namespaces"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("namespace")]
        public string Namespace {
            get { return ns; }
            set { ns = value; }
        }

        /// <include file='doc\XmlSchemaAny.uex' path='docs/doc[@for="XmlSchemaAny.ProcessContents"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("processContents"), DefaultValue(XmlSchemaContentProcessing.None)]
        public XmlSchemaContentProcessing ProcessContents {
            get { return processContents; }
            set { processContents = value; }
        }

        [XmlIgnore]
        internal NamespaceList NamespaceList {
            get { return namespaceList; }
        }

        [XmlIgnore]
        internal XmlSchemaContentProcessing ProcessContentsCorrect {
            get { return processContents == XmlSchemaContentProcessing.None ? XmlSchemaContentProcessing.Strict : processContents; }
        }

        internal void BuildNamespaceList(string targetNamespace) {
            if (ns != null) {
                namespaceList = new NamespaceList(ns, targetNamespace);
            }
            else {
                namespaceList = new NamespaceList();
            }
        }

        internal bool Allows(XmlQualifiedName qname) {
            return namespaceList.Allows(qname.Namespace);
        }

    }

}
