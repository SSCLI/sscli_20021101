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
** Interface:  ICustomFormatter
**
**                                        
**
** Purpose: Marks a class as providing special formatting
**
** Date:  July 19, 1999
**
===========================================================*/
namespace System {
    
	using System;
	using System.Runtime.Serialization;
    /// <include file='doc\ICustomFormatter.uex' path='docs/doc[@for="ICustomFormatter"]/*' />
    public interface ICustomFormatter
    {
    	/// <include file='doc\ICustomFormatter.uex' path='docs/doc[@for="ICustomFormatter.Format"]/*' />
	// Interface does not need to be marked with the serializable attribute
    	String Format (String format, Object arg, IFormatProvider formatProvider);
    	
    }
}
