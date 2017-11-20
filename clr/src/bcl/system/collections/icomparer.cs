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
** Interface:  IComparer
**
**                                                    
**
** Purpose: Interface for comparing two Objects.
**
** Date:  September 25, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    // The IComparer interface implements a method that compares two objects. It is
    // used in conjunction with the Sort and BinarySearch methods on
    // the Array and List classes.
    // 
	// Interfaces are not serializable
    /// <include file='doc\IComparer.uex' path='docs/doc[@for="IComparer"]/*' />
    public interface IComparer {
        // Compares two objects. An implementation of this method must return a
        // value less than zero if x is less than y, zero if x is equal to y, or a
        // value greater than zero if x is greater than y.
        // 
        /// <include file='doc\IComparer.uex' path='docs/doc[@for="IComparer.Compare"]/*' />
        int Compare(Object x, Object y);
    }
}
