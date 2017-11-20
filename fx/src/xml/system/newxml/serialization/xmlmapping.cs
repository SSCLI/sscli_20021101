//------------------------------------------------------------------------------
// <copyright file="XmlMapping.cs" company="Microsoft">
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

    /// <include file='doc\XmlMapping.uex' path='docs/doc[@for="XmlMapping"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class XmlMapping {
        TypeScope scope;
        bool generateSerializer = false;

        internal XmlMapping(TypeScope scope) {
            this.scope = scope;
        }

        internal TypeScope Scope {
            get { return scope; }
        }

        internal bool GenerateSerializer {
            get { return generateSerializer; }
            set { generateSerializer = value; }
        }
    }
}
