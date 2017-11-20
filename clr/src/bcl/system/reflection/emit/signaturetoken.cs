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
** Signature:  SignatureToken
**
**                                        
**
** Purpose: Represents a Signature to the ILGenerator signature.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken"]/*' />
	[Serializable()] 
    public struct SignatureToken {
    
		/// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.Empty"]/*' />
		public static readonly SignatureToken Empty = new SignatureToken();

        internal int m_signature;
        internal ModuleBuilder m_moduleBuilder;
          
        internal SignatureToken(int str, ModuleBuilder mod) {
            m_signature=str;
            m_moduleBuilder = mod;
        }
    
        /// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.Token"]/*' />
        public int Token {
            get { return m_signature; }
        }
    	
    	/// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_signature;
    	}
    
    	/// <include file='doc\SignatureToken.uex' path='docs/doc[@for="SignatureToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is SignatureToken))
    			return ((SignatureToken)obj).m_signature == m_signature;
    		else
    			return false;
    	}
    }
}
