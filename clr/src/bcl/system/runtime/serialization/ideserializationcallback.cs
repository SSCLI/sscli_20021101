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
** Interface: IDeserializationEventListener
**
**                                        
**
** Purpose: Implemented by any class that wants to indicate that
**          it wishes to receive deserialization events.
**
** Date:  August 16, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System;
    /// <include file='doc\IDeserializationCallback.uex' path='docs/doc[@for="IDeserializationCallback"]/*' />

    // Interface does not need to be marked with the serializable attribute
    public interface IDeserializationCallback {
        /// <include file='doc\IDeserializationCallback.uex' path='docs/doc[@for="IDeserializationCallback.OnDeserialization"]/*' />
        void OnDeserialization(Object sender);
    
    }
}
