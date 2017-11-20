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
    /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback"]/*' />
    public class NoBestFitFallback : EncodingFallback {
        
        String defaultStr;
        char[] defaultChars;
        int defaultCharsCount;


        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.NoBestFitFallback"]/*' />
        public NoBestFitFallback() : this("?") {
        }

        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.NoBestFitFallback1"]/*' />
        public NoBestFitFallback(String defaultStr) {
            if (defaultStr == null) {
                throw new ArgumentNullException("defaultStr");
            }

            this.defaultStr = defaultStr;
            defaultChars = defaultStr.ToCharArray();
            defaultCharsCount = defaultChars.Length;
        }
        
        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.Fallback"]/*' />
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

            if (charCount == 0) return (null);

            char[] result = new char[charCount * defaultCharsCount];
            for (int i = 0; i < charCount; i++) {
                Array.Copy(defaultChars, 0, result, i * defaultStr.Length, defaultCharsCount);
            }
            return (result);
        }

        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount() {
            return (defaultStr.Length);
        }

        
        /// <include file='doc\NoBestFitFallback.uex' path='docs/doc[@for="NoBestFitFallback.DefaultString"]/*' />
        public String DefaultString {
            get {
                return (defaultStr);
            }
        }
    }
}
