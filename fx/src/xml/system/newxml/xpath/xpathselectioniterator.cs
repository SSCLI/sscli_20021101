//------------------------------------------------------------------------------
// <copyright file="XPathSelectionIterator.cs" company="Microsoft">
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
namespace System.Xml.XPath 
{
    using System;
    using System.Diagnostics;

    internal class XPathSelectionIterator : ResetableIterator {
        private XPathNavigator nav;
        private IQuery         query;
        private int            position;
        
        public XPathSelectionIterator(XPathNavigator nav, XPathExpression expr) {
            this.nav = nav.Clone();
            query = ((CompiledXpathExpr) expr).QueryTree;
            if (query.ReturnType() != XPathResultType.NodeSet) {
                throw new XPathException(Res.Xp_NodeSetExpected);
            }
            query.setContext(nav.Clone());
        }

        public XPathSelectionIterator(XPathNavigator nav, string xpath) {
            this.nav = nav;
            query = new QueryBuilder().Build( xpath, /*allowVar:*/true, /*allowKey:*/true );
            if (query.ReturnType() != XPathResultType.NodeSet) {
                throw new XPathException(Res.Xp_NodeSetExpected);
            }
            query.setContext(nav.Clone());
        }

        private XPathSelectionIterator(XPathNavigator nav, IQuery query) {
            this.nav = nav;
            this.query = query;
        }

        public override ResetableIterator MakeNewCopy() {
           return new XPathSelectionIterator(nav.Clone(),query.Clone());
        }
        
        private XPathSelectionIterator(XPathSelectionIterator it) {
            this.nav   = it.nav.Clone();
            this.query = it.query.Clone();
            for(position = 0; position < it.position; position ++) {
                this.query.advance();
            }
        }

        public override bool MoveNext() {
            XPathNavigator n = query.advance();
	        if( n != null ) {
                position++;
                if (!nav.MoveTo(n)) {                   
		            nav = n;
                }
                return true;
            }
            return false;
        }

        public override XPathNavigator Current {
            get { return nav; }
        }

        public override int CurrentPosition { 
       	    get { return position;  }
        }
	    
        public override void Reset() {
            this.query.reset();
        }

        public override XPathNodeIterator Clone() {
            return new XPathSelectionIterator(this);
        }

       
    }     
}
