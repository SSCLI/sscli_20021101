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
** File:    Header.cs
**
**                                              
**
** Purpose: Defines the out-of-band data for a call
**
**
** Date:    April 19, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging{
	using System.Runtime.Remoting;
	using System;
    /// <include file='doc\Header.uex' path='docs/doc[@for="Header"]/*' />
    [Serializable]
    public class Header
    {
        /// <include file='doc\Header.uex' path='docs/doc[@for="Header.Header"]/*' />
        public Header (String _Name, Object _Value)
        
            : this(_Name, _Value, true) {
        }
        /// <include file='doc\Header.uex' path='docs/doc[@for="Header.Header1"]/*' />
        public Header (String _Name, Object _Value, bool _MustUnderstand)
        {
            Name = _Name;
            Value = _Value;
            MustUnderstand = _MustUnderstand;
        }

        /// <include file='doc\Header.uex' path='docs/doc[@for="Header.Header2"]/*' />
        public Header (String _Name, Object _Value, bool _MustUnderstand, String _HeaderNamespace)
        {
            Name = _Name;
            Value = _Value;
            MustUnderstand = _MustUnderstand;
            HeaderNamespace = _HeaderNamespace;
        }

        /// <include file='doc\Header.uex' path='docs/doc[@for="Header.Name"]/*' />
        public String    Name;
        /// <include file='doc\Header.uex' path='docs/doc[@for="Header.Value"]/*' />
        public Object    Value;
        /// <include file='doc\Header.uex' path='docs/doc[@for="Header.MustUnderstand"]/*' />
        public bool   MustUnderstand;

        /// <include file='doc\Header.uex' path='docs/doc[@for="Header.HeaderNamespace"]/*' />
        public String HeaderNamespace;
    }
}
