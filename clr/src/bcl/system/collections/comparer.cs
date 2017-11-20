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
** Class:  Comparer
**
**                                                    
**
** Purpose: Default IComparer implementation.
**
** Date:  October 9, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer"]/*' />
    [Serializable]
    public sealed class Comparer : IComparer
    {
        /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer.Default"]/*' />
        public static readonly Comparer Default = new Comparer();
    
        private Comparer() {
        }
    
    	// Compares two Objects by calling CompareTo.  If a == 
    	// b,0 is returned.  If a implements 
    	// IComparable, a.CompareTo(b) is returned.  If a 
    	// doesn't implement IComparable and b does, 
    	// -(b.CompareTo(a)) is returned, otherwise an 
    	// exception is thrown.
    	// 
        /// <include file='doc\Comparer.uex' path='docs/doc[@for="Comparer.Compare"]/*' />
        public int Compare(Object a, Object b) {
            if (a == b) return 0;
            if (a == null) return -1;
            if (b == null) return 1;
            IComparable ia = a as IComparable;
    		if (ia != null)
    			return ia.CompareTo(b);
            IComparable ib = b as IComparable;
    		if (ib != null)
                return -ib.CompareTo(a);
    		throw new ArgumentException(Environment.GetResourceString("Argument_ImplementIComparable"));
        }
    }
}
