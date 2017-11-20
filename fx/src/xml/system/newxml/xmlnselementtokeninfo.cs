//------------------------------------------------------------------------------
// <copyright file="XmlNSElementTokenInfo.cs" company="Microsoft">
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
namespace System.Xml {

    using System.Diagnostics;

    // XmlNSElementTokenInfo class is for Element token when Namespace support is on
    // and also for token that has name/prefix/namespace
    // but does not have value
    //
    internal class XmlNSElementTokenInfo : XmlElementTokenInfo {
        internal XmlNSElementTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type, String name, int nameOffset, int nameLength, int nameColonPos,
                                       int depth, bool isEmpty) : base(scanner, nsMgr, type, depth) {
            _NsMgr = nsMgr;
            _Scanner = scanner;
            _NameColonPos = nameColonPos;
            _NameWPrefix = String.Empty;
            _Name = String.Empty;
            _Prefix = String.Empty;
            _NamespaceURI = String.Empty;
        }

        internal override void SetName(XmlScanner scanner, int offset, int length, int colon, int depth) {
            _Scanner = scanner;
            _NameColonPos = colon;
            _Depth = depth;

            _NameWPrefix = _Scanner.GetTextAtom(offset,length);
            _SchemaType = null;
            _TypedValue = null;
        }

        internal override void FixNames() {
            if (_NameColonPos > 0) {
                _Prefix = _Scanner.GetTextAtom(_NameWPrefix.Substring(0, _NameColonPos));
                _Name = _Scanner.GetTextAtom(_NameWPrefix.Substring(_NameColonPos+1));
                _NamespaceURI = _NsMgr.LookupNamespace(_Prefix);
                if (_NamespaceURI == null) {
                    throw new XmlException(Res.Xml_UnknownNs,_Prefix, LineNum, LinePos);
                }
            }
            else {
                _Prefix = String.Empty;
                _Name = _NameWPrefix;
                _NamespaceURI = _NsMgr.DefaultNamespace;
                
            }
        }

        internal override String NameWPrefix {
            get {
                return _NameWPrefix;
            }
            set {            
                Debug.Assert(_Scanner == null || Ref.Equal(value, _Scanner.GetTextAtom(value)), "FullName should be atomalized");
                _NameWPrefix = value;
                _SchemaType = null;
                _TypedValue = null;
            }
        }

        internal override String Name {
            get { return _Name;}
        }

        internal override String Prefix {
            get { return _Prefix;}
        }

        internal override String Namespaces {
            get { return _NamespaceURI;}
        }

    } // XmlNSElementTokenInfo
} // System.Xml
