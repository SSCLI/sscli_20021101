//------------------------------------------------------------------------------
// <copyright file="GroupQuery.cs" company="Microsoft">
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


    internal sealed class GroupQuery : BaseAxisQuery {

        internal GroupQuery(IQuery qy): base() {
            m_qyInput = qy;
        }

        internal override XPathNavigator advance() {
            if (m_qyInput.ReturnType() == XPathResultType.NodeSet) {
                m_eNext = m_qyInput.advance();
                if (m_eNext != null)
                    _position++;
                return m_eNext;
            }
            return null;
        }

        override internal object getValue(IQuery qyContext) {
            return m_qyInput.getValue(qyContext);
        }

        override internal object getValue(XPathNavigator qyContext, XPathNodeIterator iterator) {
            return m_qyInput.getValue(qyContext, iterator);
        }

        override internal XPathResultType ReturnType() {
            return m_qyInput.ReturnType();
        }

        internal override IQuery Clone() {
            return new GroupQuery(m_qyInput.Clone());
        }

        internal override bool Merge {
            get {
                return false;
            }
        }
    }
}
