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
** CLASS:    XmlSyntaxException.cs
**
**                                                  
**
** PURPOSE:  Provide Syntax-error exceptions to callers
**           of XML parser. 
** 
** DATE:     25 Jun 1998
** 
===========================================================*/
namespace System.Security {
    
    
	using System;
    using System.Runtime.Serialization;

    /// <include file='doc\XMLSyntaxException.uex' path='docs/doc[@for="XmlSyntaxException"]/*' />
    [Serializable] sealed public class XmlSyntaxException : SystemException
    {
        /// <include file='doc\XMLSyntaxException.uex' path='docs/doc[@for="XmlSyntaxException.XmlSyntaxException"]/*' />
        public
        XmlSyntaxException ()
            : base (Environment.GetResourceString( "XMLSyntax_InvalidSyntax" ))
        {
    		SetErrorCode(__HResults.CORSEC_E_XMLSYNTAX);
        }
    
        /// <include file='doc\XMLSyntaxException.uex' path='docs/doc[@for="XmlSyntaxException.XmlSyntaxException1"]/*' />
        public
        XmlSyntaxException (String message)
            : base (message)
        {
    		SetErrorCode(__HResults.CORSEC_E_XMLSYNTAX);
        }
    
        /// <include file='doc\XMLSyntaxException.uex' path='docs/doc[@for="XmlSyntaxException.XmlSyntaxException2"]/*' />
        public
        XmlSyntaxException (String message, Exception inner)
            : base (message, inner)
        {
    		SetErrorCode(__HResults.CORSEC_E_XMLSYNTAX);
        }
    
        /// <include file='doc\XMLSyntaxException.uex' path='docs/doc[@for="XmlSyntaxException.XmlSyntaxException3"]/*' />
        public
        XmlSyntaxException (int lineNumber)
            : base (String.Format( Environment.GetResourceString( "XMLSyntax_SyntaxError" ), lineNumber ) )
        {
    		SetErrorCode(__HResults.CORSEC_E_XMLSYNTAX);
        }
        
        /// <include file='doc\XMLSyntaxException.uex' path='docs/doc[@for="XmlSyntaxException.XmlSyntaxException4"]/*' />
        public
        XmlSyntaxException( int lineNumber, String message )
            : base( String.Format( Environment.GetResourceString( "XMLSyntax_SyntaxErrorEx" ), lineNumber, message ) )
        {
    		SetErrorCode(__HResults.CORSEC_E_XMLSYNTAX);
        }

        internal XmlSyntaxException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
