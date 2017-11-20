//------------------------------------------------------------------------------
// <copyright file="XPathAxisIterator.cs" company="Microsoft">
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

namespace System.Xml.XPath 
{
    using System;

    internal abstract class XPathAxisIterator: XPathNodeIterator {
        internal XPathNavigator nav;
        internal XPathNodeType type;
        internal string name;
        internal string uri;
        internal int    position;
        internal bool   matchSelf;
        internal bool   first = true;

        public XPathAxisIterator(XPathNavigator nav, bool matchSelf) {
            this.nav = nav;
            this.matchSelf = matchSelf;
        }

        public XPathAxisIterator(XPathNavigator nav, XPathNodeType type, bool matchSelf) : this(nav, matchSelf) {
            this.type = type;
        }

        public XPathAxisIterator(XPathNavigator nav, string name, string namespaceURI, bool matchSelf) : this(nav, matchSelf) {
            this.name      = nav.NameTable.Add(name);
            this.uri       = nav.NameTable.Add(namespaceURI);
        }

        public XPathAxisIterator(XPathAxisIterator it) {
            this.nav       = it.nav.Clone();
            this.type      = it.type;
            this.name      = it.name;
            this.uri       = it.uri;
            this.position  = it.position;
            this.matchSelf = it.matchSelf;
            this.first     = it.first;
        }

        public override XPathNavigator Current {
            get { return nav; }
        }

        public override int CurrentPosition {
            get { return position; }
        }

        // Nodetype Matching - Given nodetype matches the navigator's nodetype
        //Given nodetype is all . So it matches everything
        //Given nodetype is text - Matches text, WS, Significant WS
        protected virtual bool Matches {
            get { 
                if (name == null) {
                    return (nav.NodeType == type || type == XPathNodeType.All ||
                            ( type == XPathNodeType.Text && 
                            (nav.NodeType == XPathNodeType.Whitespace ||
                            nav.NodeType == XPathNodeType.SignificantWhitespace)));
                }
                else {
                    return(
                        nav.NodeType == XPathNodeType.Element && 
                        ((object) name == (object) nav.LocalName || name.Length == 0) && 
                        (object) uri == (object) nav.NamespaceURI
                    ); 
                }
            }
        }
    }    
}
