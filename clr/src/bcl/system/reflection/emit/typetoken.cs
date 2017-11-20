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
** Class:  TypeToken
**
**                                        
**
** Purpose: Represents a Class to the ILGenerator class.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
	using System.Threading;
    /// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken"]/*' />
	[Serializable()] 
    public struct TypeToken {
    
		/// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.Empty"]/*' />
		public static readonly TypeToken Empty = new TypeToken();

        internal int m_class;
    
        
        internal TypeToken(int str) {
            m_class=str;
        }
    
        /// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.Token"]/*' />
        public int Token {
            get { return m_class; }
        }
        
    	/// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_class;
    	}
    	
    	/// <include file='doc\TypeToken.uex' path='docs/doc[@for="TypeToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is TypeToken))
    			return ((TypeToken)obj).m_class == m_class;
    		else
    			return false;
    	}
    }
}
