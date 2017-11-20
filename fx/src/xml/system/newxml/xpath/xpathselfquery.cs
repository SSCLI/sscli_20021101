//------------------------------------------------------------------------------
// <copyright file="XPathSelfQuery.cs" company="Microsoft">
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

    internal class XPathSelfQuery : BaseAxisQuery {
        protected bool first = true;
        protected XPathNavigator _e = null;
        private bool _Context;
        
        internal XPathSelfQuery(XPathNavigator Input) {
            _e = Input;
            first=true;
            _Context = true;
        }

        internal XPathSelfQuery(XPathNavigator Input, bool simple) {
            _e = Input;
            first=true;
            m_Type = XPathNodeType.All;
            _Context = true;

        }
        
        internal XPathSelfQuery() {
            _Context = true;
        }
        
        internal XPathSelfQuery(
                               IQuery  qyInput,
                               String Name,
                               String Prefix,
                               String URN,
                               XPathNodeType Type) : base(qyInput, Name,  Prefix, URN,  Type) {
        }
                
        internal override void setContext(XPathNavigator e) {
            if (m_qyInput != null){
                m_qyInput.setContext(e);
            }
            else
            {
                _e = e;
                first = true;
            }
        }
        
        internal override XPathNavigator advance() {
            if (_e != null) {
                if (first) {
                    first = false;
                    if (_Context || matches(_e)) {
                        _position = 1;
                        return _e;
                    }
                    return null;
                }
                else
                    return null;
            }

            while (true) {
                m_eNext = m_qyInput.advance();
                if (m_eNext != null) {
                    if (matches(m_eNext)){
                        _position++;
                        return m_eNext;
                    }
                }
                else
                    return null;
            }
        }
                
        override internal void reset() {
            first = true;
        } 

        override internal XPathNavigator peekElement() {
            if (_e != null)
                return _e;
            if (m_eNext == null)
                m_eNext = advance();
            return m_eNext;

        } 
        internal override IQuery Clone() {
            if (_e != null)
                return new XPathSelfQuery(_e);
            else
                if (m_qyInput != null)
                return new XPathSelfQuery(m_qyInput.Clone(), m_Name, 
                                          m_Prefix, m_URN, m_Type);
            else
                return new XPathSelfQuery(null, m_Name, m_Prefix, m_URN, 
                                          m_Type);
        }

        internal override XPathNavigator MatchNode(XPathNavigator navigator){
            if (_Context)
                return navigator;
            throw new XPathException(Res.Xp_InvalidPattern);
        }

        internal override Querytype getName() {
            if (m_qyInput == null) {
                return Querytype.Self;
            }
            else {
                return m_qyInput.getName();
            }
        }


    }
}
