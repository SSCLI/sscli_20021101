//------------------------------------------------------------------------------
// <copyright file="XmlElementTokenInfo.cs" company="Microsoft">
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

// XmlElementTokenInfo class is for Element token when namespace support is off
// and also for token that has name/prefix/namespace
// but does not have value
//
    internal class XmlElementTokenInfo : XmlBasicTokenInfo {
        internal String          _NameWPrefix;
        internal String          _Name;                    
        internal String          _Prefix;
        internal String          _NamespaceURI;
        internal bool            _IsEmpty;
        internal int             _NameColonPos;

        internal XmlElementTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type, int depth
                                    ) : base(scanner, nsMgr, type, depth) {
        }

        internal XmlElementTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                     String name, int nameOffset, int nameLength, int nameColonPos, 
                                     int depth, bool isEmpty) : base(scanner, nsMgr, type, depth) {
            _NameColonPos = nameColonPos;
            _Name = (name == null) ? _Scanner.GetTextAtom(nameOffset, nameLength) : _Scanner.GetTextAtom(name);
        }

        internal virtual void SetName(XmlScanner scanner, int offset, int length, int colon, int depth) {
            _Scanner = scanner;
            _NameColonPos = colon;
            _Depth = depth;

            _Name = _Scanner.GetTextAtom(offset,length);
            _SchemaType = null;
            _TypedValue = null;
        }

        internal virtual void FixNames() {
        }

        internal void SetName(String nameWPrefix, String localName, String prefix,
                               String ns, int depth, XmlScanner scanner) {
            _NameWPrefix = nameWPrefix;
            _Name = localName;
            _Prefix = prefix;
            _NamespaceURI = ns;
            _Depth = depth;
            _Scanner = scanner;
        }

        internal override String Name {
            get {
                return _Name;
            }
            set {
                _Name = value;
                _SchemaType = null;
                _TypedValue = null;
            }
        }

        internal override String NameWPrefix {
            get { return this.Name; }
            set { this.Name = value; }            
        }

        internal int NameColonPos {
            get {
                return _NameColonPos;
            }
            set {
                _NameColonPos = value;
            }
        }

        internal override bool IsEmpty {
            get {
                return _IsEmpty;
            }
            set {
                _IsEmpty = value;
            }
        }

    } // XmlElementTokenInfo
} // System.Xml
