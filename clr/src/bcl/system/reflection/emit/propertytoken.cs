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
** Class:  PropertyToken
**
**                                     
**
** Propertybuilder is for client to define properties for a class
**
** Date: June 7, 99
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\PropertyToken.uex' path='docs/doc[@for="PropertyToken"]/*' />
	[Serializable()] 
    public struct PropertyToken {
    
		/// <include file='doc\PropertyToken.uex' path='docs/doc[@for="PropertyToken.Empty"]/*' />
		public static readonly PropertyToken Empty = new PropertyToken();

        internal int m_property;
    
        /// <include file='doc\PropertyToken.uex' path='docs/doc[@for="PropertyToken.Token"]/*' />
        public int Token {
            get { return m_property; }
        }
    	
    	// Satisfy value class requirements
    	/// <include file='doc\PropertyToken.uex' path='docs/doc[@for="PropertyToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_property;
    	}
    	
    	// Satisfy value class requirements
    	/// <include file='doc\PropertyToken.uex' path='docs/doc[@for="PropertyToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is PropertyToken))
    			return ((PropertyToken)obj).m_property == m_property;
    		else
    			return false;
    	}
    }


}
