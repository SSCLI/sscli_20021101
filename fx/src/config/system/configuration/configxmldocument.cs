//------------------------------------------------------------------------------
// <copyright file="ConfigXmlDocument.cs" company="Microsoft">
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

#if !LIB

namespace System.Configuration
{
    using System.Diagnostics;
    using System.IO;
    using System.Xml;
    using System.Security.Permissions;


    internal interface IConfigXmlNode {
        string Filename { get; }
        int LineNumber { get; }
    }

    /// <include file='doc\ConfigXmlDocument.uex' path='docs/doc[@for="ConfigXmlDocument"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// <para>
    /// ConfigXmlDocument - the default Xml Document doesn't track line numbers, and line
    /// numbers are necessary to display source on config errors.
    /// These classes wrap corresponding System.Xml types and also carry 
    /// the necessary information for reporting filename / line numbers.
    /// Note: these classes will go away if webdata ever decides to incorporate line numbers
    /// into the default XML classes.  This class could also go away if webdata brings back
    /// the UserData property to hang any info off of any node.
    /// </para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    public sealed class ConfigXmlDocument : XmlDocument, IConfigXmlNode {
        XmlTextReader _reader;
        int _lineOffset;
        string _filename;

        /// <internalonly/>
        int IConfigXmlNode.LineNumber {
            get { 
                if (_reader == null) 
                    return 0; 
                else {
                    if (_lineOffset > 0) {
                        return _reader.LineNumber + _lineOffset - 1;
                    }
                    else {
                        return _reader.LineNumber;
                    }
                }
            }
        }

        /// <internalonly/>
        public int LineNumber { get { return ((IConfigXmlNode)this).LineNumber; } }

        /// <internalonly/>
        string IConfigXmlNode.Filename {
            get { return _filename; }
        }

        /// <internalonly/>
        public string Filename { get { return ((IConfigXmlNode)this).Filename; } }

        /// <internalonly/>
        public override void Load(string filename) {
            _filename = filename;
            try {
                _reader = new XmlTextReader(filename);
                base.Load(_reader);
            }
            finally {
                if (_reader != null) {
                    _reader.Close();
                }
            }
        }

        /// <internalonly/>
        public void LoadSingleElement(string filename, XmlTextReader sourceReader) {
            _filename = filename;
            _lineOffset = sourceReader.LineNumber;
            string outerXml = sourceReader.ReadOuterXml();
            
            try {
                _reader = new XmlTextReader(new StringReader(outerXml), sourceReader.NameTable);
                base.Load(_reader);
            }
            finally {
                if (_reader != null) {
                    _reader.Close();
                }
            }
        }

        /// <internalonly/>
        public override XmlAttribute CreateAttribute( string prefix, string localName, string namespaceUri ) {
            return new ConfigXmlAttribute( Filename, LineNumber, prefix, localName, namespaceUri, this );
        }
        /// <internalonly/>
        public override XmlElement CreateElement( string prefix, string localName, string namespaceUri) {
            return new ConfigXmlElement( Filename, LineNumber, prefix, localName, namespaceUri, this );
        }
        /// <internalonly/>
        public override XmlText CreateTextNode(String text) {
            return new ConfigXmlText( Filename, LineNumber, text, this );
        }
        /// <internalonly/>
        public override XmlCDataSection CreateCDataSection(String data) {
            return new ConfigXmlCDataSection( Filename, LineNumber, data, this );
        }
        /// <internalonly/>
        public override XmlComment CreateComment(String data) {
            return new ConfigXmlComment( Filename, LineNumber, data, this );
        }
        /// <internalonly/>
        public override XmlSignificantWhitespace CreateSignificantWhitespace(String data) {
            return new ConfigXmlSignificantWhitespace( Filename, LineNumber, data, this );
        }
        /// <internalonly/>
        public override XmlWhitespace CreateWhitespace(String data) {
            return new ConfigXmlWhitespace( Filename, LineNumber, data, this );
        }
    }


    internal sealed class ConfigXmlElement : XmlElement, IConfigXmlNode {
        int _line;
        string _filename;

        public ConfigXmlElement( string filename, int line, string prefix, string localName, string namespaceUri, XmlDocument doc )
            : base( prefix, localName, namespaceUri, doc) {
            _line = line;
            _filename = filename;
        }
        public int LineNumber {
            get { return _line; }
        }
        public string Filename {
            get { return _filename; }
        }
        public override XmlNode CloneNode(bool deep) {
            XmlNode cloneNode = base.CloneNode(deep);
            ConfigXmlElement clone = cloneNode as ConfigXmlElement;
            if (clone != null) {
                clone._line = _line;
                clone._filename = _filename;
            }
            return cloneNode;
        }
    }

    internal sealed class ConfigXmlAttribute : XmlAttribute, IConfigXmlNode {
        int _line;
        string _filename;                

        public ConfigXmlAttribute( string filename, int line, string prefix, string localName, string namespaceUri, XmlDocument doc )
            : base( prefix, localName, namespaceUri, doc ) {
            _line = line;
            _filename = filename;
        }
        public int LineNumber {
            get { return _line; }
        }
        public string Filename {
            get { return _filename; }
        }
        public override XmlNode CloneNode(bool deep) {
            XmlNode cloneNode = base.CloneNode(deep);
            ConfigXmlAttribute clone = cloneNode as ConfigXmlAttribute;
            if (clone != null) {
                clone._line = _line;
                clone._filename = _filename;
            }
            return cloneNode;
        }
    }

    internal sealed class ConfigXmlText : XmlText, IConfigXmlNode {
        int _line;
        string _filename;

        public ConfigXmlText( string filename, int line, string strData, XmlDocument doc )
            : base( strData, doc ) {
            _line = line;
            _filename = filename;
        }
        public int LineNumber {
            get { return _line; }
        }
        public string Filename {
            get { return _filename; }
        }
        public override XmlNode CloneNode(bool deep) {
            XmlNode cloneNode = base.CloneNode(deep);
            ConfigXmlText clone = cloneNode as ConfigXmlText;
            if (clone != null) {
                clone._line = _line;
                clone._filename = _filename;
            }
            return cloneNode;
        }
    }
    internal sealed class ConfigXmlCDataSection : XmlCDataSection, IConfigXmlNode {
        int _line;
        string _filename;

        public ConfigXmlCDataSection( string filename, int line, string data, XmlDocument doc )
            : base( data, doc) {
            _line = line;
            _filename = filename;
        }
        public int LineNumber {
            get { return _line; }
        }
        public string Filename {
            get { return _filename; }
        }
        public override XmlNode CloneNode(bool deep) {
            XmlNode cloneNode = base.CloneNode(deep);
            ConfigXmlCDataSection clone = cloneNode as ConfigXmlCDataSection;
            if (clone != null) {
                clone._line = _line;
                clone._filename = _filename;
            }
            return cloneNode;
        }
    }
    internal sealed class ConfigXmlSignificantWhitespace : XmlSignificantWhitespace, IConfigXmlNode {
        public ConfigXmlSignificantWhitespace(string filename, int line, string strData, XmlDocument doc)
            : base(strData, doc) {
            _line = line;
            _filename = filename;
        }
        int _line;
        string _filename;

        public int LineNumber {
            get { return _line; }
        }
        public string Filename {
            get { return _filename; }
        }
        public override XmlNode CloneNode(bool deep) {
            XmlNode cloneNode = base.CloneNode(deep);
            ConfigXmlSignificantWhitespace clone = cloneNode as ConfigXmlSignificantWhitespace;
            if (clone != null) {
                clone._line = _line;
                clone._filename = _filename;
            }
            return cloneNode;
        }
    }
    internal sealed class ConfigXmlComment : XmlComment, IConfigXmlNode {
        int _line;
        string _filename;

        public ConfigXmlComment( string filename, int line, string comment, XmlDocument doc )
            : base( comment, doc ) {
            _line = line;
            _filename = filename;
        }
        public int LineNumber {
            get { return _line; }
        }
        public string Filename {
            get { return _filename; }
        } 
        public override XmlNode CloneNode(bool deep) {
            XmlNode cloneNode = base.CloneNode(deep);
            ConfigXmlComment clone = cloneNode as ConfigXmlComment;
            if (clone != null) {
                clone._line = _line;
                clone._filename = _filename;
            }
            return cloneNode;
        }
    }
    internal sealed class ConfigXmlWhitespace : XmlWhitespace, IConfigXmlNode {
        public ConfigXmlWhitespace( string filename, int line, string comment, XmlDocument doc )
            : base( comment, doc ) {
            _line = line;
            _filename = filename;
        }
        int _line;
        string _filename;

        public int LineNumber {
            get { return _line; }
        }
        public string Filename {
            get { return _filename; }
        }
        public override XmlNode CloneNode(bool deep) {
            XmlNode cloneNode = base.CloneNode(deep);
            ConfigXmlWhitespace clone = cloneNode as ConfigXmlWhitespace;
            if (clone != null) {
                clone._line = _line;
                clone._filename = _filename;
            }
            return cloneNode;
        }
    }
}
#endif

