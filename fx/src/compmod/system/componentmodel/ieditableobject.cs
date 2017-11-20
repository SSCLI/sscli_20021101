//------------------------------------------------------------------------------
// <copyright file="IEditableObject.cs" company="Microsoft">
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

// An object that can rollback edits.
namespace System.ComponentModel {

    using System.Diagnostics;

    /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IEditableObject {
        /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject.BeginEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void BeginEdit();
        /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject.EndEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void EndEdit();
        /// <include file='doc\IEditableObject.uex' path='docs/doc[@for="IEditableObject.CancelEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void CancelEdit();
    }
}
