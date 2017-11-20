//------------------------------------------------------------------------------
// <copyright file="CollectionsUtil.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>                                                                
//------------------------------------------------------------------------------

// Wrapper for a case insensitive Hashtable.

namespace System.Collections.Specialized {

    using System.Collections;

    /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class CollectionsUtil {

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveHashtable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Hashtable CreateCaseInsensitiveHashtable()  {
            return new Hashtable(CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default);
        }

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveHashtable1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Hashtable CreateCaseInsensitiveHashtable(int capacity)  {
            return new Hashtable(capacity, CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default);
        }

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveHashtable2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Hashtable CreateCaseInsensitiveHashtable(IDictionary d)  {
            return new Hashtable(d, CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default);
        }

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveSortedList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SortedList CreateCaseInsensitiveSortedList() {
            return new SortedList(CaseInsensitiveComparer.Default);
        }

    }

}
