//------------------------------------------------------------------------------
// <copyright file="TheQuery.cs" company="Microsoft">
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

namespace System.Xml.Xsl {

    using System;
    using System.Xml;
    using System.Xml.XPath;

    internal sealed class TheQuery {
        internal InputScopeManager   _ScopeManager;
        private  XPathExpression   _CompiledQuery;

        internal XPathExpression CompiledQuery {
            get {
                return _CompiledQuery;
            }
            set {
                _CompiledQuery = value;
            }
        }
        

        internal InputScopeManager ScopeManager {
            get {
                return _ScopeManager;
            }
        }

        internal TheQuery( CompiledXpathExpr compiledQuery, InputScopeManager manager) {
            _CompiledQuery = compiledQuery;
            _ScopeManager = manager.Clone();
        }
    }
}
