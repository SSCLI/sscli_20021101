//------------------------------------------------------------------------------
// <copyright file="XmlAnyElementAttribute.cs" company="Microsoft">
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

namespace System.Xml.Serialization {
    using System;
    using System.Xml.Schema;

    /// <include file='doc\XmlAnyElementAttribute.uex' path='docs/doc[@for="XmlAnyElementAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Parameter | AttributeTargets.ReturnValue, AllowMultiple=true)]
    public class XmlAnyElementAttribute : System.Attribute {
        string name;
        string ns;

        /// <include file='doc\XmlAnyElementAttribute.uex' path='docs/doc[@for="XmlAnyElementAttribute.XmlAnyElementAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAnyElementAttribute() {
        }
        
        /// <include file='doc\XmlAnyElementAttribute.uex' path='docs/doc[@for="XmlAnyElementAttribute.XmlAnyElementAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAnyElementAttribute(string name) {
            this.name = name;
        }

        /// <include file='doc\XmlAnyElementAttribute.uex' path='docs/doc[@for="XmlAnyElementAttribute.XmlAnyElementAttribute2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAnyElementAttribute(string name, string ns) {
            this.name = name;
            this.ns = ns;
        }
        
        /// <include file='doc\XmlAnyElementAttribute.uex' path='docs/doc[@for="XmlAnyElementAttribute.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Name {
            get { return name == null ? string.Empty : name; }
            set { name = value; }
        }
        
        /// <include file='doc\XmlAnyElementAttribute.uex' path='docs/doc[@for="XmlAnyElementAttribute.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Namespace {
            get { return ns; }
            set { ns = value; }
        }
    }
    
}
