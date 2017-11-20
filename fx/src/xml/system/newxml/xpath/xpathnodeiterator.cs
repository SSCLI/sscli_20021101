//------------------------------------------------------------------------------
// <copyright file="XPathNodeIterator.cs" company="Microsoft">
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

    /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator"]/*' />
    public abstract class XPathNodeIterator: ICloneable {
        private int count;
        object ICloneable.Clone() { return this.Clone(); }

        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.Clone"]/*' />
        public abstract XPathNodeIterator Clone();
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.MoveNext"]/*' />
        public abstract bool MoveNext();
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.Current"]/*' />
        public abstract XPathNavigator Current { get; }
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.CurrentPosition"]/*' />
        public abstract int CurrentPosition { get; }
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.Count"]/*' />
        public virtual int Count {
            get { 
    	        if (count == 0) {
                    XPathNodeIterator clone = this.Clone();
                    while(clone.MoveNext()) ;
                    count = clone.CurrentPosition;
                }
                return count;
            } 
        }
    }
}
