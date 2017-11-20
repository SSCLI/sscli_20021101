//------------------------------------------------------------------------------
// <copyright file="XmlIteratorQuery.cs" company="Microsoft">
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
    using System.Xml.XPath;

    internal class XmlIteratorQuery: IQuery {
        ResetableIterator it;

        public XmlIteratorQuery( ResetableIterator it ) {
            this.it = it;
        }

        internal override XPathNavigator peekElement() {
            return it.Current;
        }

        internal override XPathNavigator advance() {
            if( it.MoveNext() ) {
                return it.Current;
            }
            return null;
        }

        internal override XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }

        internal override void reset() {
            it.Reset();
        }

        internal override IQuery Clone() {
            return new XmlIteratorQuery(it.MakeNewCopy());
        }
        
        internal override int Position {
            get {
                return it.CurrentPosition;
            }
        }

        internal override object getValue(IQuery qy) {
            return it;
        }
        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return it;
        }

    }
}
