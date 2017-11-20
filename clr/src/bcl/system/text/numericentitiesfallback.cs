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
    

    /// <include file='doc\NumericEntitiesFallback.uex' path='docs/doc[@for="NumericEntitiesFallback"]/*' />
    public class NumericEntitiesFallback : EncodingFallback {       

        /// <include file='doc\NumericEntitiesFallback.uex' path='docs/doc[@for="NumericEntitiesFallback.Fallback"]/*' />
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
            String str = GetNumericEntity(chars, charIndex, charCount);
            return (str.ToCharArray());
        }

        /// <include file='doc\NumericEntitiesFallback.uex' path='docs/doc[@for="NumericEntitiesFallback.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount() {
            //
            // The max fallback string has the form like "&#12345;", so
            // the max char count is 8.
            // If we support surrogate, we need to increase this.
            return (8);
        }

        internal static String GetNumericEntity(char[] chars, int charIndex, int charCount) {
            String str = "";
            //
            // 
            for (int i = 0; i < charCount; i++) {                
                str += "&#";
                str += (int)chars[charIndex + i];
                str += ";";
            }
            return (str);
        }     
    }
}
