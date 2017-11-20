//------------------------------------------------------------------------------
// <copyright file="XmlNodeChangedEventArgs.cs" company="Microsoft">
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
    /// <include file='doc\XmlNodeChangedEventArgs.uex' path='docs/doc[@for="XmlNodeChangedEventArgs"]/*' />
    /// <devdoc>
    /// </devdoc>
    public class XmlNodeChangedEventArgs {
        private XmlNodeChangedAction action;
        private XmlNode     node;
        private XmlNode     oldParent;
        private XmlNode     newParent;

        /// <include file='doc\XmlNodeChangedEventArgs.uex' path='docs/doc[@for="XmlNodeChangedEventArgs.Action"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlNodeChangedAction Action { get { return action; } }
        /// <include file='doc\XmlNodeChangedEventArgs.uex' path='docs/doc[@for="XmlNodeChangedEventArgs.Node"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlNode Node { get { return node; } }
        /// <include file='doc\XmlNodeChangedEventArgs.uex' path='docs/doc[@for="XmlNodeChangedEventArgs.OldParent"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlNode OldParent { get { return oldParent; } }
        /// <include file='doc\XmlNodeChangedEventArgs.uex' path='docs/doc[@for="XmlNodeChangedEventArgs.NewParent"]/*' />
        /// <devdoc>
        /// </devdoc>
        public XmlNode NewParent { get { return newParent; } }
        

        internal XmlNodeChangedEventArgs( XmlNode node, XmlNode oldParent, XmlNode newParent, XmlNodeChangedAction action ) {
            this.node      = node;
            this.oldParent = oldParent;
            this.newParent = newParent;
            this.action    = action;
        }
    }
}
