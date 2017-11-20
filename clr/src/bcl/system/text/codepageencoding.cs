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
	using System.Globalization;
	using System.Runtime.InteropServices;
	using System;
	using System.Security;
    using System.Collections;
	using System.Runtime.CompilerServices;

    //
    // These are the delegates used by the internal GetByteCountFallback() and GetBytesFallback() to provide
    // the fallback support.
    //

    /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding"]/*' />
    [Serializable()] internal class CodePageEncoding : Encoding
    {
        private int maxCharSize;
    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.CodePageEncoding"]/*' />
        public CodePageEncoding(int codepage) : base(codepage == 0? Microsoft.Win32.Win32Native.GetACP(): codepage) {
            maxCharSize = GetCPMaxCharSizeNative(codepage);
        }

    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetByteCount"]/*' />
        public override int GetByteCount(char[] chars, int index, int count) {
            if (chars == null) {
                throw new ArgumentNullException("chars", 
                    Environment.GetResourceString("ArgumentNull_Array"));
            }
            if (index < 0 || count < 0) {
                throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"),
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            if (chars.Length - index < count) {
                throw new ArgumentOutOfRangeException("chars",
                      Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }

            if (maxCharSize == 1) return count;
            //bool useDefaultChar;
            int byteCount = UnicodeToBytesNative(m_codePage, chars, index, count, null, 0, 0);

            // Check for overflows.
            if (byteCount < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return byteCount;
        }

        
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetBytes"]/*' />
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
            if (charCount == 0) return 0;
            if (byteIndex == bytes.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));


            //bool usedDefaultChar;
            int result = UnicodeToBytesNative(m_codePage, chars, charIndex, charCount,
                                              bytes, byteIndex, bytes.Length - byteIndex);
                
            if (result == 0) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            
            return (result);
        }


        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetCharCount"]/*' />
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
            if (maxCharSize == 1) return count;
            return BytesToUnicodeNative(m_codePage, bytes, index, count, null, 0, 0);
        }
        
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetChars"]/*' />
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
            if (byteCount == 0) return 0;
            if (charIndex == chars.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            int result = BytesToUnicodeNative(m_codePage, bytes, byteIndex, byteCount,
                chars, charIndex, chars.Length - charIndex);
            if (result == 0) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
            return result;
        }
    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetDecoder"]/*' />
        public override System.Text.Decoder GetDecoder() {
            return new Decoder(this);
        }
        
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetMaxByteCount"]/*' />
        public override int GetMaxByteCount(int charCount) {
            if (charCount < 0) {
               throw new ArgumentOutOfRangeException("charCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            long byteCount = charCount * maxCharSize;
            // Check for overflows.
            if (byteCount < 0 || byteCount > Int32.MaxValue)
                throw new ArgumentOutOfRangeException("charCount", Environment.GetResourceString("ArgumentOutOfRange_GetByteCountOverflow"));
            return (int)byteCount;

        }
    
        /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.GetMaxCharCount"]/*' />
        public override int GetMaxCharCount(int byteCount) {
            if (byteCount < 0) {
               throw new ArgumentOutOfRangeException("byteCount", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum")); 
            }
            return byteCount;
        }

            
        [Serializable]
        private class Decoder : System.Text.Decoder
        {
            private CodePageEncoding encoding;
            private int lastByte;
            private byte[] temp;
    
            public Decoder(CodePageEncoding encoding) {
                if (encoding.CodePage==50229) {
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_CodePage50229"));
                }

                this.encoding = encoding;
                lastByte = -1;
                temp = new byte[2];
            }
    
            /// <include file='doc\CodePageEncoding.uex' path='docs/doc[@for="CodePageEncoding.Decoder.GetCharCount"]/*' />
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
                if (encoding.maxCharSize == 1 || count == 0) return count;
                int result = 0;
                if (lastByte >= 0) {
                    index++;
                    count--;
                    result = 1;
                    if (count == 0) return result;
                }
                if ((GetEndLeadByteCount(bytes, index, count) & 1) != 0) {
                    count--;
                    if (count == 0) return result;
                }
                return result + BytesToUnicodeNative(encoding.m_codePage,
                    bytes, index, count, null, 0, 0);
            }
    
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
                int result = 0;
                if (byteCount == 0) return result;
                if (lastByte >= 0) {
                    if (charIndex == chars.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                    temp[0] = (byte)lastByte;
                    temp[1] = bytes[byteIndex];
                    BytesToUnicodeNative(encoding.m_codePage, temp, 0, 2, chars, charIndex, 1);
                    byteIndex++;
                    byteCount--;
                    charIndex++;
                    lastByte = -1;
                    result = 1;
                    if (byteCount == 0) return result;
                }
                if (encoding.maxCharSize > 1) {
                    if ((GetEndLeadByteCount(bytes, byteIndex, byteCount) & 1) != 0) {
                        lastByte = bytes[byteIndex + --byteCount] & 0xFF;
                        if (byteCount == 0) return result;
                    }
                }
                if (charIndex == chars.Length) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                int n = BytesToUnicodeNative(encoding.m_codePage, bytes, byteIndex, byteCount,
                    chars, charIndex, chars.Length - charIndex);
                if (n == 0) throw new ArgumentException(Environment.GetResourceString("Argument_ConversionOverflow"));
                return result + n;
            }
    
            private int GetEndLeadByteCount(byte[] bytes, int index, int count) {
                int end = index + count;
                int i = end;
                while (i > index && IsDBCSLeadByteEx(encoding.m_codePage, bytes[i - 1])) i--;
                return end - i;
            }
        }
    
        [DllImport(Microsoft.Win32.Win32Native.KERNEL32, CharSet=CharSet.Auto),
         SuppressUnmanagedCodeSecurityAttribute()]
         private static extern bool IsDBCSLeadByteEx(int codePage, byte testChar);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int BytesToUnicodeNative(int codePage,
            byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex, int charCount);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int UnicodeToBytesNative(int codePage,
            char[] chars, int charIndex, int charCount,
                                                       byte[] bytes, int byteIndex, int byteCount/*, ref bool usedDefaultChar*/);
    	
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	internal static extern int GetCPMaxCharSizeNative(int codePage);
    }
}
