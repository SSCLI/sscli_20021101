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
** Interface: IHashCodeProvider
**
**                                                    
**
** Purpose: A bunch of strings.
**
** Date: July 21, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    // Provides a mechanism for a hash table user to override the default
    // GetHashCode() function on Objects, providing their own hash function.
    /// <include file='doc\IHashCodeProvider.uex' path='docs/doc[@for="IHashCodeProvider"]/*' />
    public interface IHashCodeProvider 
    {
	// Interfaces are not serializable
    	// Returns a hash code for the given object.  
    	// 
    	/// <include file='doc\IHashCodeProvider.uex' path='docs/doc[@for="IHashCodeProvider.GetHashCode"]/*' />
    	int GetHashCode (Object obj);
    }
}
