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
** Interface: IRemotingFormatter;
**
**                                        
**
** Purpose: The interface for all formatters.
**
** Date:  April 22, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {

	using System;
	using System.IO;
	using System.Runtime.Serialization;
    /// <include file='doc\IRemotingFormatter.uex' path='docs/doc[@for="IRemotingFormatter"]/*' />
    public interface IRemotingFormatter : IFormatter {
        /// <include file='doc\IRemotingFormatter.uex' path='docs/doc[@for="IRemotingFormatter.Deserialize"]/*' />
    
        // Begin the process of deserialization.  For purposes of serialization,
        // this will probably rely on a stream that has been connected to the 
        // formatter through other means.  
        //
        Object Deserialize(Stream serializationStream, HeaderHandler handler);
        /// <include file='doc\IRemotingFormatter.uex' path='docs/doc[@for="IRemotingFormatter.Serialize"]/*' />
    
        // Start the process of serialization.  The object graph commencing at 
        // graph will be serialized to the appropriate backing store.
        void Serialize(Stream serializationStream, Object graph, Header[] headers);
        
    }


    
}
