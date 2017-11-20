// ------------------------------------------------------------------------------
// <copyright file="CodeAttributeArgumentCollection.cs" company="Microsoft">
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
// ------------------------------------------------------------------------------
// 
namespace System.CodeDom {
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    
    
    /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection"]/*' />
    /// <devdoc>
    ///     <para>
    ///       A collection that stores <see cref='System.CodeDom.CodeAttributeArgument'/> objects.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
    ]
    public class CodeAttributeArgumentCollection : CollectionBase {
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.CodeAttributeArgumentCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeArgumentCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgumentCollection() {
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.CodeAttributeArgumentCollection1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> based on another <see cref='System.CodeDom.CodeAttributeArgumentCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgumentCollection(CodeAttributeArgumentCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.CodeAttributeArgumentCollection2"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> containing any array of <see cref='System.CodeDom.CodeAttributeArgument'/> objects.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgumentCollection(CodeAttributeArgument[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.CodeDom.CodeAttributeArgument'/>.</para>
        /// </devdoc>
        public CodeAttributeArgument this[int index] {
            get {
                return ((CodeAttributeArgument)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a <see cref='System.CodeDom.CodeAttributeArgument'/> with the specified value to the 
        ///    <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> .</para>
        /// </devdoc>
        public int Add(CodeAttributeArgument value) {
            return List.Add(value);
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of an array to the end of the <see cref='System.CodeDom.CodeAttributeArgumentCollection'/>.</para>
        /// </devdoc>
        public void AddRange(CodeAttributeArgument[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.AddRange1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Adds the contents of another <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeAttributeArgumentCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the 
        ///    <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> contains the specified <see cref='System.CodeDom.CodeAttributeArgument'/>.</para>
        /// </devdoc>
        public bool Contains(CodeAttributeArgument value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(CodeAttributeArgument[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of a <see cref='System.CodeDom.CodeAttributeArgument'/> in 
        ///       the <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(CodeAttributeArgument value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a <see cref='System.CodeDom.CodeAttributeArgument'/> into the <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, CodeAttributeArgument value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CodeAttributeArgumentCollection.uex' path='docs/doc[@for="CodeAttributeArgumentCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific <see cref='System.CodeDom.CodeAttributeArgument'/> from the 
        ///    <see cref='System.CodeDom.CodeAttributeArgumentCollection'/> .</para>
        /// </devdoc>
        public void Remove(CodeAttributeArgument value) {
            List.Remove(value);
        }
    }
}
