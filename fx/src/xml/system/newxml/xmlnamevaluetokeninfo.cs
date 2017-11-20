//------------------------------------------------------------------------------
// <copyright file="XmlNameValueTokenInfo.cs" company="Microsoft">
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


using System.Text;
using System.Diagnostics;
namespace System.Xml {

    // NameValueTokenInfo class for token that has simple name and value.
    // for DocType and PI node types.
    // This class has to worry about atomalize
    // name but does not have to handle prefix, namespace.
    //

    internal class XmlNameValueTokenInfo : XmlValueTokenInfo {
        private String _Name;

        internal XmlNameValueTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                        int depth, bool nor) : base(scanner, nsMgr, type, depth, nor) {
            _Name = String.Empty;
        }

        internal XmlNameValueTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                        int depth, String value, bool nor) :
                                            base(scanner, nsMgr, type, depth, nor) {
            _Name = String.Empty;
            base.SetValue(scanner, value, -1, -1);
        }

        internal override String Name {
            get {
                return _Name;
            }
            set {
                _Name = value;
                _SchemaType = null;
                _TypedValue = null;
                Debug.Assert(_Name != null, "Name should not be set to null");
            }
        }

        internal override String NameWPrefix {
            get {
                return this.Name;
            }
        }
    } // XmlNameValueTokenInfo
} // System.Xml
