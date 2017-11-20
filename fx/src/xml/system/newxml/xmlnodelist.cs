//------------------------------------------------------------------------------
// <copyright file="XmlNodeList.cs" company="Microsoft">
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

namespace System.Xml 
{
    using System.Collections;

    /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an ordered collection of nodes.
    ///    </para>
    /// </devdoc>
    public abstract class XmlNodeList: IEnumerable {

        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.Item"]/*' />
        /// <devdoc>
        ///    <para>Retrieves a node at the given index.</para>
        /// </devdoc>
        public abstract XmlNode Item(int index);

        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of nodes in this XmlNodeList.</para>
        /// </devdoc>
        public abstract int Count { get;}

        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Provides a simple ForEach-style iteration over the collection of nodes in
        ///       this XmlNodeList.</para>
        /// </devdoc>
        public abstract IEnumerator GetEnumerator();
        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.this"]/*' />
        /// <devdoc>
        ///    <para>Retrieves a node at the given index.</para>
        /// </devdoc>
        [System.Runtime.CompilerServices.IndexerName ("ItemOf")]
        public virtual XmlNode this[int i] { get { return Item(i);}}
    }
}

