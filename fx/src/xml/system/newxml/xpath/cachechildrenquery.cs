//------------------------------------------------------------------------------
// <copyright file="CacheChildrenQuery.cs" company="Microsoft">
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

    internal sealed class CacheChildrenQuery : ChildrenQuery {
        Stack _ElementStk = new Stack();
        XPathNavigator _NextInput = null;
        Stack _PositionStk = new Stack();
        
        internal CacheChildrenQuery(
                                   IQuery qyInput,
                                   String name,
                                   String prefix,
                                   String urn,
                                   XPathNodeType type) : base (qyInput, name, prefix, urn, type) {
        }

        internal override void reset() {
            _ElementStk.Clear();
            _NextInput = null;
            base.reset();
        }


        internal override XPathNavigator advance() {
            while (true) {
                if (_count == 0) {
                    if (DecideInput())
                        return null;
                }
                else {
                    if (! m_eNext.MoveToNext())
                        _count = 0;
                    else {
                        _count++;
                    }
                }
                if (_count > 1)
                    CompareNextInput();
                if (_count != 0) {
                    if (matches(m_eNext)) {
                        _position++;
                        return m_eNext;
                    }
                }

            } // while

        } // Advance

        private bool DecideInput() {
            if (_ElementStk.Count == 0) {
                if (_NextInput == null) {
                    m_eNext = m_qyInput.advance();
                    if (m_eNext == null)
                        return true;
                    m_eNext = m_eNext.Clone();
                }
                else {
                    m_eNext = _NextInput;
                    _NextInput = null;

                }
                if (m_eNext.MoveToFirstChild()) {

                    _count = 1;
                }
                _position = 0;
            }
            else {
                m_eNext = _ElementStk.Pop() as XPathNavigator;
                _position = (int) _PositionStk.Pop();
                _count = 2;
            }
            return false;
        }

        private void CompareNextInput() {
            XmlNodeOrder order;
            if (_NextInput == null) {
                _NextInput = m_qyInput.advance();
                if (_NextInput != null) {
                    _NextInput = _NextInput.Clone();

                }
                else
                    return;
            }
            
            order = m_eNext.ComparePosition(_NextInput);
            if (order == XmlNodeOrder.After) {
                _ElementStk.Push(m_eNext);
                _PositionStk.Push(_position);
                m_eNext = _NextInput;
                _NextInput = null;
                if (!m_eNext.MoveToFirstChild())
                    _count = 0;
                else {
                    _count = 1;
                }
                _position = 0;
            }

        }

        internal override IQuery Clone() {
            if (m_qyInput != null)
                return new 
                CacheChildrenQuery(m_qyInput.Clone(),m_Name,m_Prefix,m_URN,m_Type);
            else
                return new CacheChildrenQuery(null,m_Name,m_Prefix,m_URN,m_Type);
        }        

    } // Children Query}
}
