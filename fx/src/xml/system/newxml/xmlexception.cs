//------------------------------------------------------------------------------
// <copyright file="XmlException.cs" company="Microsoft">
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

namespace System.Xml {
    using System;
    using System.IO;
    using System.Resources;
    using System.Runtime.Serialization;
    using System.Text;

// The XmlException class contains XML parser errors.
//
    /// <include file='doc\XmlException.uex' path='docs/doc[@for="XmlException"]/*' />
    /// <devdoc>
    ///    <para>Returns detailed information about the last parse error, including the error
    ///       number, line number, character position, and a text description.</para>
    /// </devdoc>
    [Serializable]
    public class XmlException : SystemException {
        string res;
        string[] args;
        int lineNumber;
        int linePosition;
        string message;

        /// <include file='doc\XmlException.uex' path='docs/doc[@for="XmlException.XmlException5"]/*' />
        protected XmlException(SerializationInfo info, StreamingContext context) : base(info, context) {
            res                 = (string)  info.GetValue("res"  , typeof(string));
            args                = (string[])info.GetValue("args", typeof(string[]));
            lineNumber          = (int)     info.GetValue("lineNumber", typeof(int));
            linePosition        = (int)     info.GetValue("linePosition", typeof(int));
        }

        /// <include file='doc\XmlException.uex' path='docs/doc[@for="XmlException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("res",                res);
            info.AddValue("args",               args);
            info.AddValue("lineNumber",         lineNumber);
            info.AddValue("linePosition",       linePosition);
        }

        /// <include file='doc\XmlException.uex' path='docs/doc[@for="XmlException.XmlException0"]/*' />
        public XmlException(String message, Exception innerException) : base(message, innerException) {
            HResult = HResults.Xml;
            this.res = Res.Xml_UserException;
            this.args = new string[] { message };
        }

        internal XmlException(string res) :
            this(res, (string[])null, 0, 0) {}

        internal XmlException(string res, string[] args) :
            this(res, args, 0, 0) {}

        internal XmlException(string res, string arg) :
            this(res,  new string[] { arg }, 0, 0) {}


        internal XmlException(string res, String arg,  IXmlLineInfo lineInfo) :
            this(res, new string[] { arg },  lineInfo) {}

        internal XmlException(string res, string[] args,  IXmlLineInfo lineInfo) {
            HResult = HResults.Xml;
            this.res = res;
            this.args = args;
            this.lineNumber = null == lineInfo ? 0 : lineInfo.LineNumber;
            this.linePosition = null == lineInfo ? 0 : lineInfo.LinePosition;
        }

        internal XmlException(string res,  int lineNumber, int linePosition) :
            this(res, (string[])null, lineNumber, linePosition) {}

        internal XmlException(string res, string arg, int lineNumber, int linePosition) :
            this(res,  new string[] { arg }, lineNumber, linePosition) {}

        internal XmlException(string res, string[] args, int lineNumber, int linePosition) {
            HResult = HResults.Xml;
            this.res = res;
            this.args = args;
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
        }


        internal static string[] BuildCharExceptionStr(char ch) {
            string[] aStringList= new string[2];
            aStringList[0] = ch.ToString();
            aStringList[1] = "0x"+ ((int)ch).ToString("X2");
            return aStringList;
        }


        // Returns the XML exception description.
        /// <include file='doc\XmlException.uex' path='docs/doc[@for="XmlException.Message"]/*' />
        public override string Message {
            get {
                string message = this.message;
                if (message == null) {
                    try {
                        message = Res.GetString(this.res, this.args);
                    }
                    catch {
                        message = "UNKNOWN("+this.res+")";
                    }

                    if (this.lineNumber != 0) {
                        string[] msg = new string[2];
                        msg[0] = this.lineNumber.ToString();
                        msg[1] = this.linePosition.ToString();
                        message += " " + Res.GetString(Res.Xml_ErrorPosition, msg);
                    }
                    this.message = message;
                }
                return message;
            }
        }

        internal string ErrorCode {
            get { return this.res; }
        }

        /// <include file='doc\XmlException.uex' path='docs/doc[@for="XmlException.LineNumber"]/*' />
        public int LineNumber {
            get { return this.lineNumber; }
        }

        /// <include file='doc\XmlException.uex' path='docs/doc[@for="XmlException.LinePosition"]/*' />
        public int LinePosition {
            get { return this.linePosition; }
        }

        internal string[] msg {
            get { return args; }
        }
    };
} // namespace System.Xml
