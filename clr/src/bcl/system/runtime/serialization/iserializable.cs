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
** Interface: ISerializable
**
**                                        
**
** Purpose: Implemented by any object that needs to control its
**          own serialization.
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
    /// <include file='doc\ISerializable.uex' path='docs/doc[@for="ISerializable"]/*' />
    public interface ISerializable {
        /// <include file='doc\ISerializable.uex' path='docs/doc[@for="ISerializable.GetObjectData"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        void GetObjectData(SerializationInfo info, StreamingContext context);
    }

}




