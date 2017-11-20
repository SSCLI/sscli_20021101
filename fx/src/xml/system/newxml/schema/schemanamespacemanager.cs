//------------------------------------------------------------------------------
// <copyright file="SchemaNamespaceManager.cs" company="Microsoft">
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

namespace System.Xml.Schema {
    using System;
    using System.Collections;

    internal class SchemaNamespaceManager : XmlNamespaceManager {
        XmlSchemaObject node;

        public SchemaNamespaceManager(XmlSchemaObject node){
            this.node = node;
        }

        public override string LookupNamespace(string prefix) {
            for (XmlSchemaObject current = node; current != null; current = current.Parent) {
                Hashtable namespaces = current.Namespaces.Namespaces;
                if (namespaces != null && namespaces.Count > 0) {
                    object uri = namespaces[prefix];
                    if (uri != null)
                        return (string)uri;
                }
            }
            return prefix == string.Empty ? string.Empty : null;
        }
  }; //SchemaNamespaceManager

}
