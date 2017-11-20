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
** Class:  SymbolToken
**
**                                               
**
** Small value class used by the SymbolStore package for passing
** around metadata tokens.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;

    /// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken"]/*' />
    public struct SymbolToken
    {
        internal int m_token;
        
        /// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.SymbolToken"]/*' />
        public SymbolToken(int val) {m_token=val;}
    
        /// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.GetToken"]/*' />
        public int GetToken() {return m_token;}
        
    	/// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.GetHashCode"]/*' />
    	public override int GetHashCode() {return m_token;}
    	
    	/// <include file='doc\Token.uex' path='docs/doc[@for="SymbolToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is SymbolToken))
    			return ((SymbolToken)obj).m_token == m_token;
    		else
    			return false;
    	}
    }
}
