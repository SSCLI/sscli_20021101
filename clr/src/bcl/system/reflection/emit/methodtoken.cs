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
** Class:  MethodToken
**
**                                        
**
** Purpose: Represents a Method to the ILGenerator class.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken"]/*' />
	[Serializable()] 
    public struct MethodToken {
    
		/// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.Empty"]/*' />
		public static readonly MethodToken Empty = new MethodToken();
        internal int m_method;
    
        
        internal MethodToken(int str) {
            m_method=str;
        }
    
        /// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.Token"]/*' />
        public int Token {
            get { return m_method; }
        }
        
    	/// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_method;
    	}
    	
    	/// <include file='doc\MethodToken.uex' path='docs/doc[@for="MethodToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is MethodToken))
    			return ((MethodToken)obj).m_method == m_method;
    		else
    			return false;
    	}
    }
}
