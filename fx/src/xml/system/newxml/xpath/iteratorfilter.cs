//------------------------------------------------------------------------------
// <copyright file="IteratorFilter.cs" company="Microsoft">
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

    internal class IteratorFilter : XPathNodeIterator {
        private XPathNodeIterator innerIterator;
        private string            name;
        private int               position = 0;

        internal IteratorFilter(XPathNodeIterator innerIterator, string name) {
            this.innerIterator = innerIterator;
            this.name          = name;
        }

        public IteratorFilter(IteratorFilter it) {
            this.innerIterator = it.innerIterator.Clone();
            this.name          = name;
            this.position      = position;
        }

        public override XPathNodeIterator Clone()         { return new IteratorFilter(this); }
        public override XPathNavigator    Current         { get { return innerIterator.Current;} }        
        public override int               CurrentPosition { get { return this.position; } }

        public override bool MoveNext() {
            while(innerIterator.MoveNext()) {
                if(innerIterator.Current.LocalName == this.name) {
                    this.position ++;
                    return true;
                }
            }
            return false;
        }
    }
}
