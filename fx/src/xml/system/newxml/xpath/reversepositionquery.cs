//------------------------------------------------------------------------------
// <copyright file="ReversePositionQuery.cs" company="Microsoft">
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

    internal sealed class ReversePositionQuery : PositionQuery {
        
        internal ReversePositionQuery() {
        }

        internal ReversePositionQuery(IQuery qyParent) :base(qyParent){
        }
        
        internal override XPathNavigator advance() {
            if (_fillStk){
                FillStk();
                _position = _Stk.Count + 1;
            }
            if (_position > 1){
                _position--;
                m_eNext = (XPathNavigator)_Stk[_count++];
                return m_eNext;
            }
            return null;
        }

        internal override IQuery Clone() {
            return new ReversePositionQuery(m_qyInput.Clone());
        }

    }
}





