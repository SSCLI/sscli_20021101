//------------------------------------------------------------------------------
// <copyright file="XmlCDATASection.cs" company="Microsoft">
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

namespace System.Xml 
{
    using System;
    using System.Text;
    using System.Diagnostics;
    using System.Xml.XPath;

    /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Used to quote or escape blocks of text to keep that text from being
    ///       interpreted as markup language.
    ///    </para>
    /// </devdoc>
    public class XmlCDataSection : XmlCharacterData {
        /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection.XmlCDataSection"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlCDataSection( string data, XmlDocument doc ): base( data, doc ) {
        }

        /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name { 
            get {return XmlDocument.strCDataSectionName;}
        }
        /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName { 
            get {return XmlDocument.strCDataSectionName;}
        }

        /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.CDATA;}
        }

        /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateCDataSection( Data );
        }

        /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteCData( Data );
        }

        /// <include file='doc\XmlCDATASection.uex' path='docs/doc[@for="XmlCDataSection.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // Intentionally do nothing
        }

        internal override XPathNodeType XPNodeType { 
            get { return XPathNodeType.Text; }
        }
    }
}
