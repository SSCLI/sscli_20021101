//------------------------------------------------------------------------------
// <copyright file="XmlWSTokenInfo.cs" company="Microsoft">
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

using System;
using System.Text;
using System.Diagnostics;
namespace System.Xml {

    using System.Diagnostics;

// XmlWSTokenInfo class is for Whitespace and Text tokens
// that may need to be normalized
//
    internal class XmlWSTokenInfo : XmlValueTokenInfo {
        private String          _Name;

        internal XmlWSTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                int depth,
                                bool nor) : base(scanner, nsMgr, type, depth, nor) {
            _Name = String.Empty;
            _RawValue = String.Empty;
        }

        internal override String Name
        {
            get
            {
                return _Name;
            }
            set
            {
                _Name = value;
                Debug.Assert(Ref.Equal(_Name, _Scanner.GetTextAtom(_Name)), "Name should be atomalized");
                Debug.Assert(_Name != null, "Name should not be set to null");
            }
        }

        internal override String NameWPrefix
        {
            get
            {
                return this._Name;
            }
            set
            {
                this.Name = value;
            }
        }

        internal override String Value
        {
            get
            {
                return base.Value;
            }

            set
            {
                _Value = value;
                _HasNormalize = !_NormalizeText;
            }
        }

        internal void SetTokenInfo(XmlNodeType nodeType, String name, int depth) {
            _NodeType = nodeType;
            //
            // this name should already be atomalized
            _Name = name;
            Debug.Assert(Ref.Equal(_Name, _Scanner.GetTextAtom(name)), "Name should be atomalized");

            _Depth = depth;
        }

    } // XmlWSTokenInfo
} // System.Xml
