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
** Interface:  IDictionary
**
**                                                    
**
** Purpose: Base interface for all dictionaries.
**
** Date:  September 21, 1999
** 
===========================================================*/
namespace System.Collections {
	using System;

    // An IDictionary is a possibly unordered set of key-value pairs.
    // Keys can be any non-null object.  Values can be any object.
    // You can look up a value in an IDictionary via the default indexed
    // property, Items.  
    /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary"]/*' />
    public interface IDictionary : ICollection
    {
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.this"]/*' />
	// Interfaces are not serializable
        // The Item property provides methods to read and edit entries 
        // in the Dictionary.
        Object this[Object key] {
            get;
            set;
        }
    
        // Returns a collections of the keys in this dictionary.
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.Keys"]/*' />
        ICollection Keys {
            get;
        }
    
        // Returns a collections of the values in this dictionary.
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.Values"]/*' />
        ICollection Values {
            get;
        }
    
        // Returns whether this dictionary contains a particular key.
        //
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.Contains"]/*' />
        bool Contains(Object key);
    
        // Adds a key-value pair to the dictionary.
        // 
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.Add"]/*' />
        void Add(Object key, Object value);
    
        // Removes all pairs from the dictionary.
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.Clear"]/*' />
        void Clear();
		/// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.IsReadOnly"]/*' />
    
		bool IsReadOnly 
        { get; }
	    /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.IsFixedSize"]/*' />

	    bool IsFixedSize
        { get; }

        // Returns an IDictionaryEnumerator for this dictionary.
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.GetEnumerator"]/*' />
        new IDictionaryEnumerator GetEnumerator();
    
        // Removes a particular key from the dictionary.
        //
        /// <include file='doc\IDictionary.uex' path='docs/doc[@for="IDictionary.Remove"]/*' />
        void Remove(Object key);
    }
}
