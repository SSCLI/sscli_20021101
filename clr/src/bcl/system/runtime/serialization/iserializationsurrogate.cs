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
** Interface: ISurrogate
**
**                                        
**
** Purpose: The interface implemented by an object which
**          supports surrogates.
**
** Date:  April 23, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System.Security.Permissions;
	using System;
	using System.Reflection;
    /// <include file='doc\ISerializationSurrogate.uex' path='docs/doc[@for="ISerializationSurrogate"]/*' />
    public interface ISerializationSurrogate {
        /// <include file='doc\ISerializationSurrogate.uex' path='docs/doc[@for="ISerializationSurrogate.GetObjectData"]/*' />
    // Interface does not need to be marked with the serializable attribute
        // Returns a SerializationInfo completely populated with all of the data needed to reinstantiate the
        // the object at the other end of serialization.  
        //
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        void GetObjectData(Object obj, SerializationInfo info, StreamingContext context);
        /// <include file='doc\ISerializationSurrogate.uex' path='docs/doc[@for="ISerializationSurrogate.SetObjectData"]/*' />
    
        // Reinflate the object using all of the information in data.  The information in
        // members is used to find the particular field or property which needs to be set.
        // 
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        Object SetObjectData(Object obj, SerializationInfo info, StreamingContext context, ISurrogateSelector selector);
    }
}
