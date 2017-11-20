//------------------------------------------------------------------------------
// <copyright file="XPathEmptyIterator.cs" company="Microsoft">
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
    
    /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator"]/*' />
    internal class XPathEmptyIterator : ResetableIterator {
        /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator.XPathEmptyIterator"]/*' />
        public XPathEmptyIterator() {}
        /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator.Clone"]/*' />
        public override XPathNodeIterator Clone() {
            return new XPathEmptyIterator();
        }

        public override ResetableIterator MakeNewCopy() {
            return new XPathEmptyIterator();
        }
        
        /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator.Current"]/*' />
        public override XPathNavigator Current {
            get { return null; }
        }

        /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator.CurrentPosition"]/*' />
        public override int CurrentPosition {
            get { return 0; }
        }

        /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator.Count"]/*' />
        public override int Count {
            get { return 0; }
        }

        /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator.MoveNext"]/*' />
        public override bool MoveNext() {
            return false;
        }

        /// <include file='doc\XPathEmptyIterator.uex' path='docs/doc[@for="XPathEmptyIterator.Reset"]/*' />
        public override void Reset() {}
    }
}
