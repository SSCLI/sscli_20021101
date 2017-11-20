//------------------------------------------------------------------------------
// <copyright file="XmlDocumentFragment.cs" company="Microsoft">
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

// DocumentFragment is a "lightweight" or "minimal" 
// Document object. It is very common to want to be able to 
// extract a portion of a document's tree or to create a new fragment of a 
// document. Imagine implementing a user command like cut or rearranging a 
// document by moving fragments around. It is desirable to have an object 
// which can hold such fragments and it is quite natural to use a Node for 
// this purpose. While it is true that a Document object could 
// fulfil this role,  a Document object can potentially be a 
// heavyweight  object, depending on the underlying implementation. What is 
// really needed for this is a very lightweight object.  
// DocumentFragment is such an object.
// Furthermore, various operations -- such as inserting nodes as children 
// of another Node -- may take DocumentFragment 
// objects as arguments;  this results in all the child nodes of the 
// DocumentFragment  being moved to the child list of this node.
// The children of a DocumentFragment node are zero or more 
// nodes representing the tops of any sub-trees defining the structure of the 
// document. DocumentFragment nodes do not need to be 
// well-formed XML documents (although they do need to follow the rules 
// imposed upon well-formed XML parsed entities, which can have multiple top 
// nodes).  For example, a DocumentFragment might have only one 
// child and that child node could be a Text node. Such a 
// structure model  represents neither an HTML document nor a well-formed XML 
// document.  
// When a DocumentFragment is inserted into a  
// Document (or indeed any other Node that may take 
// children) the children of the DocumentFragment and not the 
// DocumentFragment  itself are inserted into the 
// Node. This makes the DocumentFragment very 
// useful when the user wishes to create nodes that are siblings; the 
// DocumentFragment acts as the parent of these nodes so that the
//  user can use the standard methods from the Node  interface, 
// such as insertBefore() and  appendChild().  

namespace System.Xml 
{

    using System.Diagnostics;
    using System.Xml.XPath;

    /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a lightweight object that is useful for tree insert
    ///       operations.
    ///    </para>
    /// </devdoc>
    public class XmlDocumentFragment : XmlNode {
        XmlLinkedNode lastChild;

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.XmlDocumentFragment"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlDocumentFragment( XmlDocument ownerDocument ): base( ) {
            if ( ownerDocument == null )
                throw new ArgumentException(Res.GetString(Res.Xdom_Node_Null_Doc));
            parentNode= ownerDocument;
        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name { 
            get { return XmlDocument.strDocumentFragmentName;}
        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName { 
            get { return XmlDocument.strDocumentFragmentName;}
        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.DocumentFragment;}
        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.ParentNode"]/*' />
        /// <devdoc>
        ///    <para>Gets the parent of this node (for nodes that can have
        ///       parents).</para>
        /// </devdoc>
        public override XmlNode ParentNode {
            get { return null;}
        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.OwnerDocument"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Xml.XmlDocument'/> that contains this node.
        ///    </para>
        /// </devdoc>
        public override XmlDocument OwnerDocument {
            get { 
                return (XmlDocument)parentNode;
            }

        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.InnerXml"]/*' />
        /// <devdoc>
        ///    Gets or sets the markup representing just
        ///    the children of this node.
        /// </devdoc>
        public override string InnerXml {
            get {
                return base.InnerXml;
            }
            set {
                RemoveAll();
                XmlLoader loader = new XmlLoader();
                //Hack that the content is the same element
                loader.ParsePartialContent( this, value, XmlNodeType.Element );
            }
        }
        
        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>Creates a duplicate of this node.</para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            XmlDocumentFragment clone = OwnerDocument.CreateDocumentFragment();
            if(deep)
                clone.CopyChildren( this, deep );
            return clone;                
        }

        internal override bool IsContainer {
            get { return true;}
        }

        internal override XmlLinkedNode LastNode {
            get { return lastChild;}
            set { lastChild = value;}
        }

        internal override bool IsValidChildType( XmlNodeType type ) {
            switch (type) {
                case XmlNodeType.Element:
                case XmlNodeType.Text:
                case XmlNodeType.EntityReference:
                case XmlNodeType.Comment:
                case XmlNodeType.Whitespace:
                case XmlNodeType.SignificantWhitespace:
                case XmlNodeType.ProcessingInstruction:
                case XmlNodeType.CDATA:
                    return true;

                default:
                    return false;
            }
        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>Saves the node to the specified XmlWriter.</para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            WriteContentTo( w );
        }

        /// <include file='doc\XmlDocumentFragment.uex' path='docs/doc[@for="XmlDocumentFragment.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>Saves all the children of the node to the specified XmlWriter.</para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            foreach( XmlNode n in this ) {
                n.WriteTo( w );
            }
        }

        internal override XPathNodeType XPNodeType { get { return XPathNodeType.Root; } }
    }
}
