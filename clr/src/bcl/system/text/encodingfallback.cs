// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
namespace System.Text {
    using System;
    
    /// <include file='doc\EncodingFallback.uex' path='docs/doc[@for="EncodingFallback"]/*' />
    public abstract class EncodingFallback {
        /// <include file='doc\EncodingFallback.uex' path='docs/doc[@for="EncodingFallback.Fallback"]/*' />
        public abstract char[] Fallback(char[] chars, int charIndex, int charCount);

        // Note GetMaxCharCount may be 0!
        /// <include file='doc\EncodingFallback.uex' path='docs/doc[@for="EncodingFallback.GetMaxCharCount"]/*' />
        public abstract int GetMaxCharCount();
    }
}
