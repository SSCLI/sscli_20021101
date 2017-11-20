//------------------------------------------------------------------------------
// <copyright file="XPathSingletonIterator.cs" company="Microsoft">
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
    
    /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator"]/*' />
    internal class XPathSingletonIterator: ResetableIterator {
        private XPathNavigator nav;
        private int position;

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.XPathSingletonIterator"]/*' />
        public XPathSingletonIterator(XPathNavigator nav) {
            Debug.Assert(nav != null);
            this.nav = nav;
        }

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.XPathSingletonIterator1"]/*' />

        public override ResetableIterator MakeNewCopy(){
            return new XPathSingletonIterator(this.nav.Clone());
        }
        
        
        public XPathSingletonIterator(XPathSingletonIterator it) {
            this.nav      = it.nav.Clone();
            this.position = it.position;
        }

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.Clone"]/*' />
        public override XPathNodeIterator Clone() {
            return new XPathSingletonIterator(this);
        }

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.Current"]/*' />
        public override XPathNavigator Current {
            get { return nav; }
        }

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.CurrentPosition"]/*' />
        public override int CurrentPosition {
            get { return position; }
        }

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.Count"]/*' />
        public override int Count {
            get { return 1; }
        }

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.MoveNext"]/*' />
        public override bool MoveNext() {
            if(position == 0) {
                position = 1;
                return true;
            }
            return false;
        }

        /// <include file='doc\XPathSingletonIterator.uex' path='docs/doc[@for="XPathSingletonIterator.Reset"]/*' />
        public override void Reset() {
            position = 0;
        }
    }
}
