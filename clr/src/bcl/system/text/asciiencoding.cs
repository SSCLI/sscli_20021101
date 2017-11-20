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
    using System.Text;

    //Marking this class serializable has no effect because it has no state, however,
    //it prevents attempts to serialize a graph containing an ASCII encoding from failing.
    using System;
    /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding"]/*' />
    [Serializable()] public class ASCIIEncoding : Encoding
    {
        private const int ASCII_CODEPAGE=20127;
        
        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.ASCIIEncoding"]/*' />
        public ASCIIEncoding() : base(ASCII_CODEPAGE) {
        }

        

        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetByteCount"]/*' />
        public override int GetByteCount(char[] chars, int index, int count) {
            if (chars == null) {
                throw new ArgumentNullException("chars", Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (index < 0 || count < 0) {
                throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"),
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - index < count) {
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }

            return (count);
        }

	    /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetByteCount1"]/*' />
	    public override int GetByteCount(String chars) {
            if (chars == null) {
                throw new ArgumentNullException("chars", Environment.GetResourceString("ArgumentNull_Array"));
            }
            return chars.Length;
        }

         /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetBytes"]/*' />
        public override int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex) {
            if (chars == null || bytes == null) {
                throw new ArgumentNullException((chars == null ? "chars" : "bytes"), 
                      Environment.GetResourceString("ArgumentNull_Array"));
            }    
            if (charIndex < 0 || charCount < 0) {
                throw new ArgumentOutOfRangeException((charIndex<0 ? "charIndex" : "charCount"), 
                      Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - charIndex < charCount) {
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            if (byteIndex < 0 || byteIndex > bytes.Length) {
                throw new ArgumentOutOfRangeException("byteIndex", 
                    Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            if (bytes.Length - byteIndex < charCount) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }
            int charEnd = charIndex + charCount;

            while (charIndex < charEnd) {
                char ch = chars[charIndex++];
                if (ch >= 0x0080) ch = '?';
                bytes[byteIndex++] = (byte)ch;
            }
            return charCount;

        }

        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetBytes1"]/*' />
        public override int GetBytes(String chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex) {
            if (chars == null || bytes == null) {
                throw new ArgumentNullException((chars == null ? "chars" : "bytes"), 
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
            if (byteIndex < 0 || byteIndex > bytes.Length) {
                throw new ArgumentOutOfRangeException("byteIndex", 
                    Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            if (bytes.Length - byteIndex < charCount) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }
            int charEnd = charIndex + charCount;

            //if (m_encodingFallback == null) {
                while (charIndex < charEnd) {
                    char ch = chars[charIndex++];
                    if (ch >= 0x0080) ch = '?';
                    bytes[byteIndex++] = (byte)ch;
                }
                return charCount;
                //} 

        }

        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetCharCount"]/*' />
        public override int GetCharCount(byte[] bytes, int index, int count) {
            if (bytes == null) {
                throw new ArgumentNullException("bytes", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (index < 0 || count < 0) {
                throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }                                                         
            if (bytes.Length - index < count) {
                throw new ArgumentOutOfRangeException("bytes",
                    Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            return (count);

        }

        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetChars"]/*' />
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex) {
            if (bytes == null || chars == null) {
                throw new ArgumentNullException((bytes == null ? "bytes" : "chars"), 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (byteIndex < 0 || byteCount < 0) {
                throw new ArgumentOutOfRangeException((byteIndex<0 ? "byteIndex" : "byteCount"), 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }    
            if ( bytes.Length - byteIndex < byteCount)
            {
                throw new ArgumentOutOfRangeException("bytes",
                    Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            if (charIndex < 0 || charIndex > chars.Length) {
                throw new ArgumentOutOfRangeException("charIndex", 
                    Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            if (chars.Length - charIndex < byteCount) {
                throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            }
            int byteEnd = byteIndex + byteCount;
            while (byteIndex < byteEnd) {
                byte b = bytes[byteIndex++];
                if (b > 0x7f) {
                    // This is an invalid byte in the ASCII encoding.
                    chars[charIndex++] = '?';
                } else {
                    chars[charIndex++] = (char)b;
                }
            }
            return (byteCount);

        }

        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetString"]/*' />
        public override String GetString(byte[] bytes)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes", Environment.GetResourceString("ArgumentNull_Array"));
            return String.CreateStringFromASCII(bytes, 0, bytes.Length);
        }

        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetString1"]/*' />
        public override String GetString(byte[] bytes, int byteIndex, int byteCount)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes", Environment.GetResourceString("ArgumentNull_Array"));                
            if (byteIndex < 0 || byteCount < 0) {
                throw new ArgumentOutOfRangeException((byteIndex<0 ? "byteIndex" : "byteCount"), 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }    
            if (bytes.Length - byteIndex < byteCount)
            {
                throw new ArgumentOutOfRangeException("bytes",
                    Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            return String.CreateStringFromASCII(bytes, byteIndex, byteCount);
        }


        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetMaxByteCount"]/*' />
        public override int GetMaxByteCount(int charCount) {
            if (charCount < 0) {
               throw new ArgumentOutOfRangeException("charCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }

            return (charCount);
        }

        /// <include file='doc\ASCIIEncoding.uex' path='docs/doc[@for="ASCIIEncoding.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount(int byteCount) {
            if (byteCount < 0) {
               throw new ArgumentOutOfRangeException("byteCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            return byteCount;
        }
    }
}
