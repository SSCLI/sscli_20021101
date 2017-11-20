//------------------------------------------------------------------------------
// <copyright file="AttributeQuery.cs" company="Microsoft">
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


    internal sealed class AttributeQuery : BaseAxisQuery {
        private int _count = 0;

        internal AttributeQuery(
                               IQuery qyParent,
                               String Name, 
                               String Prefix,
                               String URN,
                               XPathNodeType Type) : base(qyParent, Name, Prefix, URN, Type) {
        }


        internal override void reset() {
            _count = 0;
            base.reset();
        }

        internal override XPathNavigator advance() {
            while (true) {
                if (_count == 0) {
                    m_eNext = m_qyInput.advance();
                    if (m_eNext == null)
                        return null;
                    m_eNext = m_eNext.Clone();
                    if (m_eNext.MoveToFirstAttribute())
                        _count = 1;
                    _position = 0;

                }
                else
                    if (! m_eNext.MoveToNextAttribute())
                    _count = 0;
                else
                    _count++;

                if (_count != 0 && ! m_eNext.NamespaceURI.Equals("http://www.w3.org/2000/xmlns/")) {
                    if (matches(m_eNext)) {
                        _position++;
                        return m_eNext;
                    }
                }

            } // while

        } // Advance

        override internal XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null) {
                if (m_Type != XPathNodeType.All && matches(context)) {
                    XPathNavigator temp = context.Clone();
                    if (temp.MoveToParent()) {
                        if (m_qyInput != null)
                            return m_qyInput.MatchNode(temp);
                        return temp;
                    }
                    else
                        return null;
                }
            }
            return null;
        } 
        internal override Querytype getName() {
            return Querytype.Attribute;
        }  

        internal override IQuery Clone() {
            if (m_qyInput != null)
                return new 
                AttributeQuery(m_qyInput.Clone(),m_Name,m_Prefix,m_URN,m_Type);
            else
                return new AttributeQuery(null,m_Name,m_Prefix,m_URN,m_Type);
        }


    } // Attribute Query

}
