//------------------------------------------------------------------------------
// <copyright file="ConfigurationException.cs" company="Microsoft">
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

namespace System.Configuration {
    using System.Xml;
    using System.Runtime.Serialization;

    /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException"]/*' />
    /// <devdoc>
    /// A config exception can contain a filename (of a config file)
    /// and a line number (of the location in the file in which a problem was
    /// encountered).
    /// 
    /// Section handlers should throw this exception (or subclasses)
    /// together with filename and line nubmer information where possible.
    /// </devdoc>
    [Serializable]
    public class ConfigurationException : SystemException {

        private String _filename;
        private int _line;


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException"]/*' />
        /// <devdoc>
        ///    <para>Default ctor is required for serialization.</para>
        /// </devdoc>
        public ConfigurationException()
        : base() {
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException7"]/*' />
        /// <devdoc>
        ///    <para>Default ctor is required for serialization.</para>
        /// </devdoc>
        protected ConfigurationException(SerializationInfo info, StreamingContext context) 
        : base(info, context) { 
            HResult = HResults.Configuration;
            _filename = info.GetString("filename");
            _line = info.GetInt32("line");
        }

        
        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message)
        : base(message) {
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, Exception inner)
        : base(message, inner) {
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, XmlNode node) 
        : this(message, GetXmlNodeFilename(node), GetXmlNodeLineNumber(node)) {
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, Exception inner, XmlNode node)
        : this(message, inner, GetXmlNodeFilename(node), GetXmlNodeLineNumber(node)) {
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, String filename, int line)
        : base(message) {
            _filename = filename;
            _line = line;
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.ConfigurationException6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ConfigurationException(String message, Exception inner, String filename, int line)
        : base(message, inner) {
            _filename = filename;
            _line = line;
            HResult = HResults.Configuration;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("filename", _filename);
            info.AddValue("line", _line);
        }

        
        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.Message"]/*' />
        /// <devdoc>
        ///    <para>The message includes the file/line number information.  
        ///       To get the message without the extra information, use BareMessage.
        ///    </para>
        /// </devdoc>
        public override String Message {
            get {
                if (_filename != null && _filename.Length > 0) {
                    if (_line != 0)
                        return base.Message + " (" + _filename + " line " + _line.ToString() + ")";
                    else
                        return base.Message + " (" + _filename + ")";
                }
                else if (_line != 0) {
                    return base.Message + " (line " + _line.ToString("G") + ")";
                }
                return base.Message;
            }
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.BareMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String BareMessage {
            get {
                return base.Message;
            }
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.Filename"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public String Filename {
            get {
                return _filename;
            }
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.Line"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Line {
            get {
                return _line;
            }
        }


        // 
        // Internal XML Error Info Helpers
        //
        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.GetXmlNodeLineNumber"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int GetXmlNodeLineNumber(XmlNode node) {
            
            IConfigXmlNode configNode = node as IConfigXmlNode;

            if (configNode != null) {
                return configNode.LineNumber;
            }
            return 0;
        }


        /// <include file='doc\ConfigurationException.uex' path='docs/doc[@for="ConfigurationException.GetXmlNodeFilename"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string GetXmlNodeFilename(XmlNode node) {
            
            IConfigXmlNode configNode = node as IConfigXmlNode;

            if (configNode != null) {
                return configNode.Filename;
            }
            return string.Empty;
        }
    }
}
#endif

