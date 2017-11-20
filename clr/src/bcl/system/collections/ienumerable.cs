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
** Interface:  IEnumerable
**
**                                                    
**
** Purpose: Interface for classes providing IEnumerators
**
** Date:  November 8, 1999
** 
===========================================================*/
namespace System.Collections {
	using System;
	using System.Runtime.InteropServices;
    // Implement this interface if you need to support VB's foreach semantics.
    // Also, COM classes that support an enumerator will also implement this interface.
    /// <include file='doc\IEnumerable.uex' path='docs/doc[@for="IEnumerable"]/*' />
    public interface IEnumerable
    {
	// Interfaces are not serializable
        // Returns an IEnumerator for this enumerable Object.  The enumerator provides
        // a simple way to access all the contents of a collection.
        /// <include file='doc\IEnumerable.uex' path='docs/doc[@for="IEnumerable.GetEnumerator"]/*' />
        IEnumerator GetEnumerator();
    }
}
