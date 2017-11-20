//------------------------------------------------------------------------------
// <copyright file="PrefixQName.cs" company="Microsoft">
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

namespace System.Xml.Xsl {
    using System;
    using System.Xml;

    internal sealed class PrefixQName {
        public string Prefix;
        public string Name;
        public string Namespace;

        internal void ClearPrefix() {
            Prefix = string.Empty;
        }

        internal void SetQName(string qname) {
            PrefixQName.ParseQualifiedName(qname, out Prefix, out Name);
        }

        //
        // Parsing qualified names
        //

        private static string ParseNCName(string qname, ref int position) {
            int qnameLength = qname.Length;
            int nameStart = position;

            if (
                qnameLength == position ||                           // Zero length ncname
                ! XmlCharType.IsStartNCNameChar(qname[position])     // Start from invalid char
            ) {
                throw new XsltException(Res.Xslt_InvalidQName, qname);
            }

            position ++;

            while (position < qnameLength && XmlCharType.IsNCNameChar(qname[position])) {
                position ++;
            }

            return qname.Substring(nameStart, position - nameStart);
        }

        public static void ParseQualifiedName(string qname, out string prefix, out string local) {
            Debug.Assert(qname != null);
            prefix = string.Empty;
            local  = string.Empty;
            int position    = 0;

            local = ParseNCName(qname, ref position);

            if (position < qname.Length) {
                if (qname[position] == ':') {
                    position ++;
                    prefix = local;
                    local  = ParseNCName(qname, ref position);
                }

                if (position < qname.Length) {
                    throw new XsltException(Res.Xslt_InvalidQName, qname);
                }
            }
        }

        public static bool ValidatePrefix(string prefix) {
            int position = 0;
            try {
                PrefixQName.ParseNCName(prefix, ref position);
            }
            catch (Exception) {}
            return position == prefix.Length;
        }
    }
}
