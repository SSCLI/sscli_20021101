// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
/*============================================================
**
** Class:  BadImageFormatException
**
**                                                  
**
** Purpose: Exception to an invalid dll or executable format.
**
** Date:  May 22, 2000
** 
===========================================================*/
namespace System {
    
    using System;
    using System.Runtime.Serialization;
    using FileLoadException = System.IO.FileLoadException;
    using System.Security.Permissions;
    using SecurityException = System.Security.SecurityException;

    /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException"]/*' />
    [Serializable()] public class BadImageFormatException : SystemException {

        private String _fileName;  // The name of the corrupt PE file.
        private String _fusionLog;  // fusion log (when applicable)

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.BadImageFormatException"]/*' />
        public BadImageFormatException() 
            : base(Environment.GetResourceString("Arg_BadImageFormatException")) {
            SetErrorCode(__HResults.COR_E_BADIMAGEFORMAT);
        }
    
        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.BadImageFormatException1"]/*' />
        public BadImageFormatException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_BADIMAGEFORMAT);
        }
        
        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.BadImageFormatException2"]/*' />
        public BadImageFormatException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_BADIMAGEFORMAT);
        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.BadImageFormatException3"]/*' />
        public BadImageFormatException(String message, String fileName) : base(message)
        {
            SetErrorCode(__HResults.COR_E_BADIMAGEFORMAT);
            _fileName = fileName;
        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.BadImageFormatException4"]/*' />
        public BadImageFormatException(String message, String fileName, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_BADIMAGEFORMAT);
            _fileName = fileName;
        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.Message"]/*' />
        public override String Message
        {
            get {
                SetMessageField();
                return _message;
            }
        }

        private void SetMessageField()
        {
            if (_message == null) {
                if (_fileName == null)
                    _message = Environment.GetResourceString("Arg_BadImageFormatException");

                else
                    _message = FileLoadException.FormatFileLoadExceptionMessage(_fileName, HResult);
            }

        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.FileName"]/*' />
        public String FileName {
            get { return _fileName; }
        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.ToString"]/*' />
        public override String ToString()
        {
            String s = GetType().FullName + ": " + Message;

            if (_fileName != null && _fileName.Length != 0)
                s += Environment.NewLine + String.Format(Environment.GetResourceString("IO.FileName_Name"), _fileName);
            
            if (InnerException != null)
                s = s + " ---> " + InnerException.ToString();

            if (StackTrace != null)
                s += Environment.NewLine + StackTrace;

			try
			{
                if(FusionLog!=null)
                {
                    if (s==null)
                        s=" ";
                    s+=Environment.NewLine;
                    s+=Environment.NewLine;
                    s+="Fusion log follows: ";
                    s+=Environment.NewLine;
                    s+=FusionLog;
                }
			}
			catch(SecurityException)
			{
			
			}

            return s;
        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.BadImageFormatException5"]/*' />
        protected BadImageFormatException(SerializationInfo info, StreamingContext context) : base(info, context) {
            // Base class constructor will check info != null.

            _fileName = info.GetString("BadImageFormat_FileName");
			_fusionLog = info.GetString("BadImageFormat_FusionLog");
        }

        private BadImageFormatException(String fileName, String fusionLog,int hResult)
            : base(null)
        {
            SetErrorCode(hResult);
            _fileName = fileName;
            _fusionLog=fusionLog;
            SetMessageField();
        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.FusionLog"]/*' />
        public String FusionLog {
            [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
            get { return _fusionLog; }
        }

        /// <include file='doc\BadImageFormatException.uex' path='docs/doc[@for="BadImageFormatException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            // Serialize data for our base classes.  base will verify info != null.
            base.GetObjectData(info, context);

            // Serialize data for this class
            info.AddValue("BadImageFormat_FileName", _fileName, typeof(String));
			info.AddValue("BadImageFormat_FusionLog", _fusionLog, typeof(String));
        }
    }
}
