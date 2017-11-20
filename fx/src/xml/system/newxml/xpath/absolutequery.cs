//------------------------------------------------------------------------------
// <copyright file="AbsoluteQuery.cs" company="Microsoft">
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
    using System.Diagnostics;

    internal sealed class AbsoluteQuery : XPathSelfQuery {
        internal AbsoluteQuery() : base() {
        }

        internal override void setContext( XPathNavigator e) {
            Debug.Assert(e != null);
            e = e.Clone();
            e.MoveToRoot();
            base.setContext( e);
        }
        
        internal override XPathNavigator advance() {
            if (_e != null) {
                if (first) {
                    first = false;
                    return _e;
                }
                else
                    return null;
            }
            return null;
        }
        
        private AbsoluteQuery(IQuery qyInput) {
            m_qyInput = qyInput;
        }

        internal override XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null && context.NodeType == XPathNodeType.Root)
                return context;
            return null;
        }

        internal override IQuery Clone() {
            if (m_qyInput != null)
                return new AbsoluteQuery(m_qyInput.Clone());
            else{
                IQuery query = new AbsoluteQuery();
                if ( _e != null ) {
                    query.setContext(_e);
                }
                return query;
            }
        }
       internal override Querytype getName() {
            return Querytype.Root;
        }

    }
}
