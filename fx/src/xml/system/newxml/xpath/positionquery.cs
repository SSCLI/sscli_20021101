//------------------------------------------------------------------------------
// <copyright file="PositionQuery.cs" company="Microsoft">
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

    internal class PositionQuery : BaseAxisQuery {

        protected ArrayList _Stk = new ArrayList();
        protected int _count = 0;
        protected bool _fillStk = true;
        private int NonWSCount = 0;
        
        internal PositionQuery() {
        }

        internal PositionQuery(IQuery qyParent) {
            m_qyInput = qyParent;
        }

        internal override void reset() {
            _Stk.Clear();
            _count = 0;
            _fillStk = true;
            base.reset();
        }
              
        
        internal virtual void FillStk(){
            while ( (m_eNext = m_qyInput.advance()) != null)
                _Stk.Add(m_eNext.Clone());
            _fillStk = false;
        }

        internal int getCount() {
            return _Stk.Count;
        }  

        internal int getNonWSCount(XsltContext context) {
            if (NonWSCount == 0) {
                for(int i=0; i < _Stk.Count; i++) {
                    XPathNavigator nav = _Stk[i] as XPathNavigator;
                    if (nav.NodeType != XPathNodeType.Whitespace || context.PreserveWhitespace(nav)) {
                        NonWSCount++;
                    }
                }
            }
            return NonWSCount;
        }  
        
        internal override XPathResultType  ReturnType() {
            return m_qyInput.ReturnType();
        }
        
        override internal XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null)
                if (m_qyInput != null)
                    return m_qyInput.MatchNode(context);
            return null;
        }
        
        internal override IQuery Clone() {
            return new PositionQuery(m_qyInput.Clone());
        }

        internal override bool Merge {
            get {
                return true;
            }
        }
    }
 
}
