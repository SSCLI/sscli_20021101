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
/*============================================================
**
** Class: CaseInsensitiveHashCodeProvider
**
**                                                        
**
** Purpose: Designed to support hashtables which require 
** case-insensitive behavior while still maintaining case,
** this provides an efficient mechanism for getting the 
** hashcode of the string ignoring case.
**
** Date: February 13, 2000
**
============================================================*/
namespace System.Collections {
//This class does not contain members and does not need to be serializable
    using System;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider"]/*' />
    [Serializable]
    public class CaseInsensitiveHashCodeProvider : IHashCodeProvider {
        private TextInfo m_text;
        
        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.CaseInsensitiveHashCodeProvider"]/*' />
        public CaseInsensitiveHashCodeProvider() {
            m_text = CultureInfo.CurrentCulture.TextInfo;
        }

        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.CaseInsensitiveHashCodeProvider1"]/*' />
        public CaseInsensitiveHashCodeProvider(CultureInfo culture) {
            if (culture==null) {
                throw new ArgumentNullException("culture");
            }
            m_text = culture.TextInfo;
        }

        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.Default"]/*' />
		public static CaseInsensitiveHashCodeProvider Default
		{
			get
			{
				return new CaseInsensitiveHashCodeProvider();
			}
		}

        /// <include file='doc\CaseInsensitiveHashCodeProvider.uex' path='docs/doc[@for="CaseInsensitiveHashCodeProvider.GetHashCode"]/*' />
        public int GetHashCode(Object obj) {
            if (obj==null) {
                throw new ArgumentNullException("obj");
            }

            String s = obj as String;
            if (s==null) {
                return obj.GetHashCode();
            }

            if (s.IsFastSort()) {
                return TextInfo.GetDefaultCaseInsensitiveHashCode(s);
            } else {
                return m_text.GetCaseInsensitiveHashCode(s);
            }
        }
    }
}
