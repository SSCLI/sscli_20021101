//------------------------------------------------------------------------------
// <copyright file="XPathArrayIterator.cs" company="Microsoft">
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
    using System.Collections;
    using System.Diagnostics;

    /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator"]/*' />
    internal class XPathArrayIterator: ResetableIterator {
        ArrayList array;
        int       index;

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.XPathArrayIterator"]/*' />
        public XPathArrayIterator(ArrayList array) {
            this.array = array;
        }

        public override ResetableIterator MakeNewCopy() {
            return new XPathArrayIterator(this.array);
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.XPathArrayIterator2"]/*' />
        public XPathArrayIterator(XPathArrayIterator it) {
            this.array = it.array;
            this.index = it.index;
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Clone"]/*' />
        public override XPathNodeIterator Clone() {
            return new XPathArrayIterator(this);
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Current"]/*' />
        public override XPathNavigator Current {
            get {
                Debug.Assert(index > 0 );
                return (XPathNavigator) array[index - 1];
            }
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.CurrentPosition"]/*' />
        public override int CurrentPosition {
            get {
                Debug.Assert(index > 0 );
                return index ;
            }
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Count"]/*' />
        public override int Count {
            get {
                return array.Count;
            }
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.MoveNext"]/*' />
        public override bool MoveNext() {
            Debug.Assert(index <= array.Count);
            if(index == array.Count) {
                return false;
            }
            index++;
            return true;
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Reset"]/*' />
        public override void Reset() {
            index = 0;
        }
    }
}
