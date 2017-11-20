//------------------------------------------------------------------------------
// <copyright file="XmlIdentity.cs" company="Microsoft">
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

    internal class XmlIdentity {
        internal XmlIdentityTable table;
        string localName;
        string namespaceURI;
        XmlName firstName;

        internal XmlIdentity( XmlIdentityTable table, string localName, string namespaceURI ) {
            this.table = table;
            this.localName = table.NameTable.Add( localName );
            this.namespaceURI = table.NameTable.Add( namespaceURI );
        }

        public String LocalName {
            get { return localName;}
        }

        public String NamespaceURI {
            get { return namespaceURI;}
        }

        public XmlIdentityTable IdentityTable {
            get { return table;}
        }

        public XmlName GetNameForPrefix( string prefix ) {
            prefix = table.NameTable.Add( prefix );

            XmlName n = firstName;
            for (; n != null; n = n.next) {
                if ((object)n.Prefix == (object)prefix)
                    return n;
            }

            n = new XmlName( this, prefix );
            n.next = firstName;
            firstName = n;

            return n;
        }
    }
}