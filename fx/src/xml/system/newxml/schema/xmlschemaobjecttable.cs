//------------------------------------------------------------------------------
// <copyright file="XmlSchemaObjectTable.cs" company="Microsoft">
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

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaObjectTable.uex' path='docs/doc[@for="XmlSchemaObjectTable"]/*' />
    public class XmlSchemaObjectTable {
        Hashtable table = new Hashtable();

        internal XmlSchemaObjectTable() {
        }

        internal void Add(XmlQualifiedName name,  XmlSchemaObject value) {
            table.Add(name, value);
        }

        internal void Insert(XmlQualifiedName name,  XmlSchemaObject value) {
            table[name] = value;
        }

        internal void Remove(XmlQualifiedName name) {
            table.Remove(name);
        }

        internal void Clear() {
            table.Clear();
        }

        /// <include file='doc\XmlSchemaObjectTable.uex' path='docs/doc[@for="XmlSchemaObjectTable.Count"]/*' />
        public int Count {
    		get { return table.Count; }
        }

        /// <include file='doc\XmlSchemaObjectTable.uex' path='docs/doc[@for="XmlSchemaObjectTable.Contains"]/*' />
        public bool Contains(XmlQualifiedName name) {
            return table.Contains(name);
        }

    	/// <include file='doc\XmlSchemaObjectTable.uex' path='docs/doc[@for="XmlSchemaObjectTable.this"]/*' />
    	public XmlSchemaObject this[XmlQualifiedName name] {
            get { return (XmlSchemaObject)table[name]; }
        }
        
        /// <include file='doc\XmlSchemaObjectTable.uex' path='docs/doc[@for="XmlSchemaObjectTable.Names"]/*' />
        public ICollection Names {
            get { return table.Keys; }
        }

        /// <include file='doc\XmlSchemaObjectTable.uex' path='docs/doc[@for="XmlSchemaObjectTable.Values"]/*' />
        public ICollection Values {
            get { return table.Values; }
        }
        /// <include file='doc\XmlSchemaObjectTable.uex' path='docs/doc[@for="XmlSchemaObjectTable.GetEnumerator"]/*' />
        public IDictionaryEnumerator GetEnumerator() {
            return table.GetEnumerator();
        }

    }
}
