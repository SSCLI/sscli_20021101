//------------------------------------------------------------------------------
// <copyright file="NavigatorQuery.cs" company="Microsoft">
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

namespace System.Xml.XPath {
    using System.Xml; 

    using System.Collections;

    internal class NavigatorQuery : IQuery {
        private XPathNavigator _e = null;
        
        internal NavigatorQuery(XPathNavigator Input) {
            _e = Input;
        }
             
        internal override XPathNavigator advance() {
            throw new XPathException(Res.Xp_NodeSetExpected);
        }

        override internal XPathNavigator peekElement() {
            throw new XPathException(Res.Xp_NodeSetExpected);

        } 
        
        internal override IQuery Clone() {
            if (_e != null)
                return new NavigatorQuery(_e);
            return null;
        }
        
        internal override object getValue(IQuery qy) {
            return _e;
        }
        
        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return _e;
        }        

        internal override XPathResultType  ReturnType() {
            return XPathResultType.Navigator;
        }

    }
}
