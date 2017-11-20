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
** Class:  EventToken	
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
    /// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken"]/*' />
	[Serializable()] 
    public struct EventToken {

		/// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.Empty"]/*' />
		public static readonly EventToken Empty = new EventToken();
    
        internal int m_event;

        
        /// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.Token"]/*' />
        public int Token {
            get { return m_event; }
        }
        
    	/// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_event;
    	}
    	
    	/// <include file='doc\EventToken.uex' path='docs/doc[@for="EventToken.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is EventToken))
    			return ((EventToken)obj).m_event == m_event;
    		else
    			return false;
    	}
    }




}
