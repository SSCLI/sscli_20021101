//------------------------------------------------------------------------------
// <copyright file="CacheQuery.cs" company="Microsoft">
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
    using System.Xml.Xsl;

    internal class CacheQuery : PositionQuery {
        
        internal CacheQuery() {
        }

        internal CacheQuery(IQuery qyParent) : base(qyParent){
        }
        
        
        internal override XPathNavigator advance() {
            if (_fillStk){
                _position = 0;
                FillStk();
            }
            if (_count < _Stk.Count ){
                _position++;
                m_eNext = (XPathNavigator)_Stk[_count++];
                return m_eNext;
            }
            return null;
        }

        internal override void FillStk() {
            while ( (m_eNext = m_qyInput.advance()) != null) {
                bool add = true;
                for (int i=0; i< _Stk.Count ; i++) {
                    XPathNavigator nav = _Stk[i] as XPathNavigator;
                    XmlNodeOrder compare = nav.ComparePosition(m_eNext) ;      
                    if (compare == XmlNodeOrder.Same ) {
                        add = false;
                        break;
                    }
                }
                if (add) {
                    _Stk.Add(m_eNext.Clone());
                }
                
            }
            _fillStk = false; 
        }
        
        internal override IQuery Clone() {
            return new CacheQuery(m_qyInput.Clone());
        }

    }
}





