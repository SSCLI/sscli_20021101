//------------------------------------------------------------------------------
// <copyright file="XPathAncestorIterator.cs" company="Microsoft">
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

    internal class XPathAncestorIterator: XPathAxisIterator {
        public XPathAncestorIterator(XPathNavigator nav, XPathNodeType type, bool matchSelf) : base(nav, type, matchSelf) {}
        public XPathAncestorIterator(XPathNavigator nav, string name, string namespaceURI, bool matchSelf) : base(nav, name, namespaceURI, matchSelf) {}
        public XPathAncestorIterator(XPathAncestorIterator it) : base(it) {}

        public override XPathNodeIterator Clone() {
            return new XPathAncestorIterator(this);
        }

        public override bool MoveNext() {
            if(first) {
                first = false;
                if(matchSelf && Matches) {
                    position = 1;
                    return true;
                }
            }

            while(nav.MoveToParent()) {
                if(Matches) {
                    position ++;
                    return true;
                }
            }
            return false;
        }
    }    
}