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
** Class:  LocalToken
**
**                                        
**
** Purpose: Represents a Local to the ILGenerator class.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\LocalToken.uex' path='docs/doc[@for="LocalToken"]/*' />
	[Serializable()] 
    internal struct LocalToken {

		public static readonly LocalToken Empty = new LocalToken();
    
        internal int m_local;
        internal Object m_class;

        
        internal LocalToken(int local, Type cls) {
            m_local=local;
            m_class = cls;
        }
    
        internal LocalToken(int local, TypeBuilder cls) {
            m_local=local;
            m_class = (Object)cls;
        }
    
        internal int GetLocalValue() {
            return m_local;
        }
    
        internal Object GetClassType() {
            return m_class;
        }
    
    	/// <include file='doc\LocalToken.uex' path='docs/doc[@for="LocalToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_local;
    	}
    	
    	/// <include file='doc\LocalToken.uex' path='docs/doc[@for="LocalToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is LocalToken)) {
    			LocalToken that = (LocalToken) obj;
    			return (that.m_class == m_class && that.m_local == m_local);
    		}
    		else
    			return false;
    	}
    }
}
