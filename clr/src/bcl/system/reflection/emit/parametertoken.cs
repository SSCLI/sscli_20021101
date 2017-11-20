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
** Class:  ParameterToken
**
**                                
**
** Purpose: metadata tokens for a parameter
**
** Date:  Aug 99
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    // The ParameterToken class is an opaque representation of the Token returned
    // by the MetaData to represent the parameter. 
    /// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken"]/*' />
	[Serializable()]  
    public struct ParameterToken {
    
		/// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.Empty"]/*' />
		public static readonly ParameterToken Empty = new ParameterToken();
        internal int m_tkParameter;
    
        
        internal ParameterToken(int tkParam) {
            m_tkParameter = tkParam;
        }
    
        /// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.Token"]/*' />
        public int Token {
            get { return m_tkParameter; }
        }
        
    	/// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_tkParameter;
    	}
    	
    	/// <include file='doc\ParameterToken.uex' path='docs/doc[@for="ParameterToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is ParameterToken))
    			return ((ParameterToken)obj).Token == m_tkParameter;
    		else
    			return false;
    	}
    }
}
