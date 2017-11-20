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
 ** Class: ISoapMessage
 **
 **                                               
 **
 ** Purpose: Interface For Soap Method Call
 **
 ** Date:  October 6, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters {

    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System.Runtime.Remoting.Messaging;    
    using System;
    // Used to specify a call record to either the binary or xml serializer
    // The call record can be transmitted as the SOAP Top record which contains
    // a method name instead of an object name as the Top record's element name
    /// <include file='doc\ISoapMessage.uex' path='docs/doc[@for="ISoapMessage"]/*' />
    public interface ISoapMessage
    {
        /// <include file='doc\ISoapMessage.uex' path='docs/doc[@for="ISoapMessage.ParamNames"]/*' />
        // Name of parameters, if null the default param names will be used

        String[] ParamNames {get; set;}
        /// <include file='doc\ISoapMessage.uex' path='docs/doc[@for="ISoapMessage.ParamValues"]/*' />
    
        // Parameter Values
        Object[] ParamValues {get; set;}
        /// <include file='doc\ISoapMessage.uex' path='docs/doc[@for="ISoapMessage.ParamTypes"]/*' />

        // Parameter Types
        Type[] ParamTypes {get; set;}        
        /// <include file='doc\ISoapMessage.uex' path='docs/doc[@for="ISoapMessage.MethodName"]/*' />
    
        // MethodName
        String MethodName {get; set;}
        /// <include file='doc\ISoapMessage.uex' path='docs/doc[@for="ISoapMessage.XmlNameSpace"]/*' />

        // MethodName XmlNameSpace
        String XmlNameSpace {get; set;}
        /// <include file='doc\ISoapMessage.uex' path='docs/doc[@for="ISoapMessage.Headers"]/*' />

        // Headers
        Header[] Headers {get; set;}
    }
}
