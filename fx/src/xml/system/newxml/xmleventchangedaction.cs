//------------------------------------------------------------------------------
// <copyright file="XmlEventChangedAction.cs" company="Microsoft">
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
    /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction"]/*' />    
    /// <devdoc>
    ///    <para>TODO: Specifies the type of node change </para>
    /// </devdoc>
    public enum XmlNodeChangedAction
    {
        /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction.Insert"]/*' />
        /// <devdoc>
        ///    <para>TODO: A node is beeing inserted in the tree.</para>
        /// </devdoc>
        Insert = 0,
        /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction.Remove"]/*' />
        /// <devdoc>
        ///    <para>TODO: A node is beeing removed from the tree.</para>
        /// </devdoc>
        Remove = 1,
        /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction.Change"]/*' />
        /// <devdoc>
        ///    <para>TODO: A node value is beeing changed.</para>
        /// </devdoc>
        Change = 2
    }
}
