//------------------------------------------------------------------------------
// <copyright file="XmlLinkedNode.cs" company="Microsoft">
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

    /// <include file='doc\XmlLinkedNode.uex' path='docs/doc[@for="XmlLinkedNode"]/*' />
    /// <devdoc>
    ///    <para>Gets the node immediately preceeding or following this node.</para>
    /// </devdoc>
    public abstract class XmlLinkedNode: XmlNode {
        internal XmlLinkedNode next;

        internal XmlLinkedNode(): base() {
            next = null;
        }
        internal XmlLinkedNode( XmlDocument doc ): base( doc ) {
            next = null;
        }

        /// <include file='doc\XmlLinkedNode.uex' path='docs/doc[@for="XmlLinkedNode.PreviousSibling"]/*' />
        /// <devdoc>
        ///    Gets the node immediately preceding this
        ///    node.
        /// </devdoc>
        public override XmlNode PreviousSibling {
            get {
                XmlNode parent = ParentNode;
                if (parent != null) {
                    XmlNode node = parent.FirstChild;
                    while (node != null && node.NextSibling != this) {
                        node = node.NextSibling;
                    }
                    return node;
                }
                return null;
            }
        }

        /// <include file='doc\XmlLinkedNode.uex' path='docs/doc[@for="XmlLinkedNode.NextSibling"]/*' />
        /// <devdoc>
        ///    Gets the node immediately following this
        ///    node.
        /// </devdoc>
        public override XmlNode NextSibling {
            get {
                XmlNode parent = ParentNode;
                if (parent != null) {
                    if (next != parent.FirstChild)
                        return next;
                }
                return null;
            }
        }
    }
}
