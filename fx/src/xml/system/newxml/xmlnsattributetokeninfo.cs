//------------------------------------------------------------------------------
// <copyright file="XmlNSAttributeTokenInfo.cs" company="Microsoft">
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
using System.Globalization;
namespace System.Xml {

    using System.Diagnostics;

    // This class stores the pure data aspect of the token
    //
    //
    internal class XmlNSAttributeTokenInfo : XmlAttributeTokenInfo {
        protected String    _Name;                    // always atomalize
        protected int       _NameColonPos;            // store colon info
        protected String    _Prefix;                  // always atomalize
        protected String    _NamespaceURI;            // always atomalize

        internal  String    _XmlNs;

//        private static String   s_XmlNsNamespace = null;


        internal XmlNSAttributeTokenInfo(XmlScanner scanner, XmlNamespaceManager nsMgr, XmlNodeType type,
                                         bool nor, String xmlNs) : base(scanner, nsMgr, type, nor) {
            _XmlNs = xmlNs;
        }

        internal override void FixDefaultNSNames() {
            String value = GetValue();

            // should not allow any prefix (even empty string prefix) to be bound to the reserved namespace
            if (value == XmlReservedNs.NsXmlNs)
                throw new XmlException(Res.Xml_CanNotBindToReservedNamespace, ValueLineNum, ValueLinePos);
            
            _NamespaceURI   = XmlReservedNs.NsXmlNs;

            //Xmlns: 
            if (value == String.Empty && _Name.Length > 0)
                throw new XmlException(Res.Xml_BadAttributeChar, XmlException.BuildCharExceptionStr(' '));
			
            if ((_Name.Length > 0 && _Name[0] != '_' && XmlCharType.IsLetter(_Name[0]) == false) ||
				(_Name.Length > 2 && String.Compare(_Name, 0, "xml", 0, 3, true, CultureInfo.InvariantCulture) == 0))
                throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(_Name[0]));
			
            _NsMgr.AddNamespace(_Name, value);

        }

        internal override void FixNSNames() {
            String value = GetValue();

            // should not allow any prefix (even empty string prefix) to be bound to the reserved namespace
            if (value == XmlReservedNs.NsXmlNs)
                throw new XmlException(Res.Xml_CanNotBindToReservedNamespace, ValueLineNum, ValueLinePos);

            // This functionis only called when we encounter an attribute of the type "xmlns..."            
            Debug.Assert( 5 <= _NameWPrefix.Length);
            _NamespaceURI = XmlReservedNs.NsXmlNs;
            if ( 5 == _NameWPrefix.Length)
            {
                Debug.Assert((object)_NameWPrefix == (Object)_XmlNs);
                _Name = _NameWPrefix;
                _Prefix = String.Empty;

                // We push the prefix that the elments belonging to this namespace will use.
                // We don't have to check for value == String.Empty because it is legal to have a empty value for default namespace.
                _NsMgr.AddNamespace(String.Empty, value);
            }
            else
            {
                _Prefix = _Scanner.GetTextAtom(_NameWPrefix.Substring(0,5));
                _Name = _Scanner.GetTextAtom(_NameWPrefix.Substring(6));
                if ((object)_Name == (Object)_XmlNs) {
                    throw new XmlException(Res.Xml_ReservedNs,_Name, LineNum, LinePos);
                }

                //Xmlns: 
                if (value == String.Empty && _Name.Length > 0)  {
                    throw new XmlException(Res.Xml_BadNamespaceDecl, ValueLineNum, ValueLinePos);
                }
			
                if ((_Name.Length > 0 && _Name[0] != '_' && XmlCharType.IsLetter(_Name[0]) == false) )
                    throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(_Name[0]));

                if (_Name.Length > 2 && String.Compare(_Name, 0, "xml", 0, 3, true, CultureInfo.InvariantCulture) == 0)
                         throw new XmlException(Res.Xml_CanNotStartWithXmlInNamespace, ValueLineNum, ValueLinePos);
                
                
                _NsMgr.AddNamespace(_Name, value);
             }       
             
			
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
                _NamespaceURI = String.Empty;
            }
        }

        internal override void SetName(XmlScanner scanner, String nameWPrefix, int nameColonPos, int depth, bool isDefault) {
            _Scanner = scanner;
            _NameColonPos = nameColonPos;
            _Depth = depth;
            _IsDefault = isDefault;    
            _NameWPrefix = nameWPrefix;
            _RawValue = String.Empty;
            _Name = null;
        }

        internal override String GenerateNS() {
            if (!_NsAttribute && _Prefix != String.Empty) {
                _NamespaceURI = _NsMgr.LookupNamespace(_Prefix);
                if (_NamespaceURI == null) {
                    throw new XmlException(Res.Xml_UnknownNs,_Prefix, LineNum, LinePos);
                }
            }
            else {
                _NamespaceURI = String.Empty;
            }
            return _NamespaceURI;
        }

        internal override String Name {
            get {
                return _Name;
            }
            set {
                _Name = value;
                _NameWPrefix = _Name;                
            }
        }

        internal override int NameColonPos {
            get {
                return _NameColonPos;
            }
            set {
                _NameColonPos = value;
            }
        }

        internal override String Prefix {
            get {
                return _Prefix;
            }
            set {
                Debug.Assert(Ref.Equal(value, _Scanner.GetTextAtom(value)), "Prefix should be atomalized");
                _Prefix = value;
            }
        }

        internal override String Namespaces {
            get {
                return _NamespaceURI;
            }
            set {
                _NamespaceURI = value;                
            }
        }
    } // XmlNSAttributeTokenInfo
} // System.Xml
