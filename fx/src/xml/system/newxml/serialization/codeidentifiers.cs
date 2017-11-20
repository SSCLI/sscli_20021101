//------------------------------------------------------------------------------
// <copyright file="CodeIdentifiers.cs" company="Microsoft">
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
    using System.Collections;
    using System.IO;
    
    /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class CodeIdentifiers {
        Hashtable identifiers = new Hashtable();
        Hashtable reservedIdentifiers = new Hashtable();
        ArrayList list = new ArrayList();
        bool camelCase;

        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.Clear"]/*' />
        public void Clear(){
            identifiers.Clear();
            list.Clear();
        }

        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.UseCamelCasing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool UseCamelCasing {
            get { return camelCase; }
            set { camelCase = value; }
        }

        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.MakeRightCase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string MakeRightCase(string identifier) {
            if (camelCase)
                return CodeIdentifier.MakeCamel(identifier);
            else
                return CodeIdentifier.MakePascal(identifier);
        }
        
        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.MakeUnique"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string MakeUnique(string identifier) {
            if (IsInUse(identifier)) {
                for (int i = 1; ; i++) {
                    string newIdentifier = identifier + i.ToString();
                    if (!IsInUse(newIdentifier)) {
                        identifier = newIdentifier;
                        break;
                    }
                }
            }
            return identifier;
        }

        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.AddReserved"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddReserved(string identifier) {
            reservedIdentifiers.Add(identifier, identifier);
        }
        
        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.RemoveReserved"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RemoveReserved(string identifier) {
            reservedIdentifiers.Remove(identifier);
        }
        
        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.AddUnique"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string AddUnique(string identifier, object value) {
            identifier = MakeUnique(identifier);
            Add(identifier, value);
            return identifier;
        }
        
        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.IsInUse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsInUse(string identifier) {
            return identifiers.Contains(identifier) || reservedIdentifiers.Contains(identifier);
        }
        
        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(string identifier, object value) {
            identifiers.Add(identifier, value);
            list.Add(value);
        }
        
        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(string identifier) {
            list.Remove(identifiers[identifier]);
            identifiers.Remove(identifier);
        }
        
        /// <include file='doc\CodeIdentifiers.uex' path='docs/doc[@for="CodeIdentifiers.ToArray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object ToArray(Type type) {
            //Array array = Array.CreateInstance(type, identifiers.Values.Count);
            //identifiers.Values.CopyTo(array, 0);
            Array array = Array.CreateInstance(type, list.Count);
            list.CopyTo(array, 0);
            return array;
        }
    }
}
