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

    /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingExceptionFallback"]/*' />
    public class EncodingExceptionFallback : EncodingFallback {       
        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingExceptionFallback.Fallback"]/*' />
        public override char[] Fallback(char[] chars, int charIndex, int charCount) {
            if (chars == null) {
                throw new ArgumentNullException("chars", 
                      Environment.GetResourceString("ArgumentNull_Array"));
            }        
            if (charIndex < 0 || charCount < 0) {
                throw new ArgumentOutOfRangeException((charIndex<0 ? "charIndex" : "charCount"), 
                      Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - charIndex < charCount) {
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCount"));
            }
            throw new EncodingFallbackException(Environment.GetResourceString("Encoding_Fallback"),
                chars, charIndex, charCount);
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingExceptionFallback.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount() {
            return (0);
        }

    }

    /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException"]/*' />
    public class EncodingFallbackException : SystemException {
        char[] chars;
        int charIndex;
        int charCount;
        
        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.EncodingFallbackException"]/*' />
        public EncodingFallbackException(String message, char[] chars, int charIndex, int charCount) : base(message) {
            this.chars = chars;
            this.charIndex = charIndex;
            this.charCount = charCount;
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.Chars"]/*' />
        public char[] Chars {
            get {
                return (chars);
            }
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.Index"]/*' />
        public int Index {
            get {
                return (charIndex);
            }
        }

        /// <include file='doc\EncodingExceptionFallback.uex' path='docs/doc[@for="EncodingFallbackException.Count"]/*' />
        public int Count {
            get {
                return (charCount);
            }
        }

    }        
}
