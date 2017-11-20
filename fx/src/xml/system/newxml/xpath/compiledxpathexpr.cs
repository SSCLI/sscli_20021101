//------------------------------------------------------------------------------
// <copyright file="CompiledXpathExpr.cs" company="Microsoft">
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

    using System;
    using System.Globalization;
    using System.Collections;
    using System.Xml.Xsl;

    internal class CompiledXpathExpr : XPathExpression {
        IQuery _compiledQuery;
        string _expr;
        SortQuery _sortQuery;
        QueryBuilder _builder;
        bool _allowSort;
        bool _hasUnresolvedPrefix;
        
        internal CompiledXpathExpr(IQuery query, string expression, bool hasPrefix) {
            _compiledQuery = query;
            _expr = expression;
            _builder = new QueryBuilder();
            _hasUnresolvedPrefix = hasPrefix;
        }

        internal IQuery QueryTree {
            get { 
                if (_hasUnresolvedPrefix)
                    throw new XPathException(Res.Xp_NoContext);
                return _compiledQuery; 
            }
        }
        
        public override string Expression {
            get { return _expr; }
        }

        public override void AddSort(object expr, IComparer comparer) {
            // sort makes sense only when we are dealing with a query that
            // returns a nodeset.
	    IQuery evalExpr;
             _allowSort = true;
            if (expr is String) {
                evalExpr = _builder.Build((String)expr, out _hasUnresolvedPrefix); // this will throw if expr is invalid
            }
            else if (expr is CompiledXpathExpr) {
		evalExpr = ((CompiledXpathExpr)expr).QueryTree;
	    }
            else {
                throw new XPathException(Res.Xp_BadQueryObject);
            }
            if (_sortQuery == null) {
                _sortQuery = new SortQuery(_compiledQuery);
                _compiledQuery = _sortQuery;
            }
            _sortQuery.AddSort(evalExpr, comparer);
        }
        
        public override void AddSort(
            object expr,
            XmlSortOrder order,
            XmlCaseOrder caseOrder,
            string lang,
            XmlDataType dataType) {

            CultureInfo cinfo;
            if (lang == null || lang == String.Empty)
                cinfo = System.Threading.Thread.CurrentThread.CurrentCulture;
            else
                cinfo = new CultureInfo(lang);

            if (order == XmlSortOrder.Descending) {
                if (caseOrder == XmlCaseOrder.LowerFirst) {
                    caseOrder = XmlCaseOrder.UpperFirst;
                }
                else if (caseOrder == XmlCaseOrder.UpperFirst) {
                    caseOrder = XmlCaseOrder.LowerFirst;
                }
            }
            AddSort(expr, new XPathComparerHelper(order, caseOrder, cinfo, dataType));
        }
        
        public override XPathExpression Clone() {
            return new CompiledXpathExpr(_compiledQuery.Clone(), _expr, _hasUnresolvedPrefix);
        }
        
        public override void SetContext(XmlNamespaceManager nsManager) {
            XsltContext xsltContext = nsManager as XsltContext;
            if(xsltContext == null) {
                if(nsManager == null) {
                    nsManager = new XmlNamespaceManager(new NameTable());
                }
                xsltContext = new UndefinedXsltContext(nsManager);
            }
            _compiledQuery.SetXsltContext(xsltContext);
//            _compiledQuery.SetNamespaceContext((XmlNamespaceManager)context);

            if (_allowSort && (_compiledQuery.ReturnType() != XPathResultType.NodeSet)) {
               throw new XPathException(Res.Xp_NodeSetExpected);
            }
           _hasUnresolvedPrefix = false;
        }

 	    public override XPathResultType ReturnType {
            get { return _compiledQuery.ReturnType(); }
        }

        private class UndefinedXsltContext : XsltContext {
            private XmlNamespaceManager nsManager;

            public UndefinedXsltContext(XmlNamespaceManager nsManager) {
                this.nsManager = nsManager;
            }
            //----- Namespace support -----
            public override string DefaultNamespace {
                get { return string.Empty; }
            }
            public override string LookupNamespace(string prefix) {
                Debug.Assert(prefix != null);
                if(prefix == string.Empty) {
                    return string.Empty;
                }
                string ns = this.nsManager.LookupNamespace(this.nsManager.NameTable.Get(prefix));
                if(ns == null) {
                    throw new XsltException(Res.Xslt_InvalidPrefix, prefix);
                }
                Debug.Assert(ns != string.Empty, "No XPath prefix can be mapped to 'null namespace'");
                return ns;
            }
            //----- XsltContext support -----
            public override IXsltContextVariable ResolveVariable(string prefix, string name) {
                throw new XPathException(Res.Xp_UndefinedXsltContext);
            }
            public override IXsltContextFunction ResolveFunction(string prefix, string name, XPathResultType[] ArgTypes) {
                throw new XPathException(Res.Xp_UndefinedXsltContext);
            }
            public override bool Whitespace { get{ return false; } }
            public override bool PreserveWhitespace(XPathNavigator node) { return false; }
            public override int CompareDocument (string baseUri, string nextbaseUri) {
                throw new XPathException(Res.Xp_UndefinedXsltContext);
            }
        }
    }
}
