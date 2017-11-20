//------------------------------------------------------------------------------
// <copyright file="NameTable.cs" company="Microsoft">
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

    using System.Collections;

    internal class NameKey {
        string ns;
        string name;

        internal NameKey(string name, string ns) {
            this.name = name;
            this.ns = ns;
        }

        public override bool Equals(object other) {
            if (!(other is NameKey)) return false;
            NameKey key = (NameKey)other;
            return name == key.name && ns == key.ns;
        }

        public override int GetHashCode() {
            return (ns == null ? 0 : ns.GetHashCode()) ^ (name == null ? 0 : name.GetHashCode());
        }
    }

    internal class NameTable {
        Hashtable table = new Hashtable();

        public void Add(string name, string ns, object value) {
            NameKey key = new NameKey(name, ns);
            table.Add(key, value);
        }

        public object this[string name, string ns] {
            get {
                return table[new NameKey(name, ns)];
            }
            set {
                table[new NameKey(name, ns)] = value;
            }
        }

        public ICollection Values {
            get { return table.Values; }
        }

        public Array ToArray(Type type) {
            Array a = Array.CreateInstance(type, table.Count);
            table.Values.CopyTo(a, 0);
            return a;
        }
    }
}

