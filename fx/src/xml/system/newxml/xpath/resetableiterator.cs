//------------------------------------------------------------------------------
// <copyright file="ResetableIterator.cs" company="Microsoft">
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
    
    internal abstract class ResetableIterator : XPathNodeIterator {
        public abstract void Reset();
        public virtual bool MoveToPosition(int pos) {
            Reset();
            for(int i = CurrentPosition; i < pos ; i ++) {
                if(!MoveNext()) {
                    return false;
                }
            }
            return true;
        }

        public abstract ResetableIterator MakeNewCopy();
    }
}
