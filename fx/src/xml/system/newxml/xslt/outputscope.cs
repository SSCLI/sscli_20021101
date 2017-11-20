//------------------------------------------------------------------------------
// <copyright file="OutputScope.cs" company="Microsoft">
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
    using System.Diagnostics;
    using System.Xml;

    internal class OutputScope : DocumentScope {
        private string           name;
        private string           nsUri;
        private string           prefix;
        private XmlSpace         space;
        private string           lang;
        private bool             mixed;
        private bool             toCData;
        private HtmlElementProps htmlElementProps; // in HTML output -- atomized name of element

        internal string Name {
            get { return this.name; }
        }
        internal string Namespace {
            get { return this.nsUri; }
        }
        internal string Prefix {
            get { return this.prefix; }
            set { this.prefix = value; }
        }
        internal XmlSpace Space {
            get { return this.space; }
            set { this.space = value; }
        }
        internal string Lang {
            get { return this.lang; }
            set { this.lang = value; }
        }
        internal bool Mixed {
            get { return this.mixed; }
            set { this.mixed = value; }
        }
        internal bool ToCData {
            get { return this.toCData; }
            set { this.toCData = value; }
        }
        internal HtmlElementProps HtmlElementProps {
            get { return this.htmlElementProps; }
            set { this.htmlElementProps = value; }
        }

        internal OutputScope() {
            Init(String.Empty, String.Empty, String.Empty, XmlSpace.None, String.Empty, false);
        }

        internal void Init(string name, string nspace, string prefix, XmlSpace space, string lang, bool mixed) {
            this.scopes           = null;
            this.name             = name;
            this.nsUri            = nspace;
            this.prefix           = prefix;
            this.space            = space;
            this.lang             = lang;
            this.mixed            = mixed;
            this.toCData          = false;
            this.htmlElementProps = null;
        }

        internal bool FindPrefix(string urn, out string prefix) {
            Debug.Assert(urn != null);

            for (NamespaceDecl scope = this.scopes; scope != null; scope = scope.Next) {
                if (Keywords.Equals(scope.Uri, urn) &&
                    scope.Prefix != null            &&
                    scope.Prefix.Length > 0) {
                    prefix = scope.Prefix;
                    return true;
                }
            }

            prefix = String.Empty;
            return false;
        }
    }
}
