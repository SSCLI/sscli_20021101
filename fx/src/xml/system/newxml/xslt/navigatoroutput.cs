//------------------------------------------------------------------------------
// <copyright file="NavigatorOutput.cs" company="Microsoft">
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

namespace System.Xml.Xsl {

    using System;
    using System.Xml;
    using System.Xml.XPath;

    internal class NavigatorOutput : RecordOutput {
        protected XPathDocument doc;
        protected XPathContainer node;
        private int documentIndex;
        
        internal XPathNavigator Navigator {
            get { return ((IXPathNavigable)doc).CreateNavigator(); }
        }

        internal NavigatorOutput() {
            doc = new XPathDocument();
            node = doc.root;
        }

        public Processor.OutputResult RecordDone(RecordBuilder record) {
            Debug.Assert(record != null);

            BuilderInfo mainNode = record.MainNode;
            documentIndex++;
            mainNode.LocalName  = doc.nt.Add(mainNode.LocalName);
            mainNode.NamespaceURI   = doc.nt.Add(mainNode.NamespaceURI);
            switch(mainNode.NodeType) {
                case XmlNodeType.Element: {                    
                    XPathElement e = mainNode.IsEmptyTag ? 
                        new XPathEmptyElement(mainNode.Prefix, mainNode.LocalName, mainNode.NamespaceURI, 0, 0, node.topNamespace, documentIndex) :
                        new XPathElement(     mainNode.Prefix, mainNode.LocalName, mainNode.NamespaceURI, 0, 0, node.topNamespace, documentIndex)
                    ;
                    node.AppendChild( e );

                    XPathNamespace last = null;
                    for (int attrib = 0; attrib < record.AttributeCount; attrib ++) {
                        documentIndex++;
                        Debug.Assert(record.AttributeList[attrib] is BuilderInfo);
                        BuilderInfo attrInfo = (BuilderInfo) record.AttributeList[attrib];
                        if (attrInfo.NamespaceURI == Keywords.s_XmlnsNamespace) {
                            XPathNamespace tmp = new XPathNamespace(attrInfo.Prefix == string.Empty ? string.Empty : attrInfo.LocalName, attrInfo.Value, documentIndex);
                            tmp.next = last;
                            last = tmp;
                        }
                        else {
                            e.AppendAttribute( new XPathAttribute( attrInfo.Prefix, attrInfo.LocalName, attrInfo.NamespaceURI, attrInfo.Value, documentIndex ) );
                        }
                    }

                    if (last != null) {
                        e.AppendNamespaces(last);
                    }

                    if (!mainNode.IsEmptyTag) {
                        node = e;
                    }
                    break;
                }

                case XmlNodeType.Text:
                case XmlNodeType.Whitespace:
                case XmlNodeType.SignificantWhitespace:
                    node.AppendChild( new XPathText( mainNode.Value, 0, 0, documentIndex ) );
                    break;

                case XmlNodeType.ProcessingInstruction:
                    node.AppendChild( new XPathProcessingInstruction( mainNode.LocalName, mainNode.Value, documentIndex ) );
                    break;

                case XmlNodeType.Comment:
                    node.AppendChild( new XPathComment( mainNode.Value, documentIndex ) );
                    break;

                case XmlNodeType.Document:
                    break;

                case XmlNodeType.EndElement:
                    node = node.parent;
                    break;

                default:
                    Debug.Fail("Invalid NodeType on output: " + mainNode.NodeType.ToString());
                    break;
            }

            record.Reset();
            return Processor.OutputResult.Continue;
        }

        public void TheEnd() {
        }
    }
}
