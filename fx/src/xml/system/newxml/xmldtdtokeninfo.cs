//------------------------------------------------------------------------------
// <copyright file="XmlDtdTokenInfo.cs" company="Microsoft">
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


using System.Xml.Schema;

namespace System.Xml {

// XmlDtdTokenInfo class for Doctype token only.
//

    internal class XmlDtdTokenInfo : XmlNameValueTokenInfo {
        internal DtdParser _DtdParser;

        internal XmlDtdTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
            int depth, bool nor) : base(scanner, nsMgr, type, depth, nor) {
            _DtdParser = null;
        }

        internal override String GetValue() {
            if (_DtdParser != null )
                _Value = _DtdParser.InternalDTD;
            return base.GetValue();
        }
    }
 } // System.Xml
