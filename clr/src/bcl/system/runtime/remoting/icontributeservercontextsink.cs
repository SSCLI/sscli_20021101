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
//  The IContributeServerContextSink interface is implemented by 
//  context properties in a Context that wish to contribute 
//  an interception sink at the context boundary on the server end 
//  of a remoting call.
//
namespace System.Runtime.Remoting.Contexts {

    using System;
    using System.Runtime.Remoting.Messaging;    
    using System.Security.Permissions;
    /// <include file='doc\IContributeServerContextSink.uex' path='docs/doc[@for="IContributeServerContextSink"]/*' />
    /// <internalonly/>
    public interface IContributeServerContextSink
    {
        /// <include file='doc\IContributeServerContextSink.uex' path='docs/doc[@for="IContributeServerContextSink.GetServerContextSink"]/*' />
	/// <internalonly/>
        // Chain your message sink in front of the chain formed thus far and 
        // return the composite sink chain.
        // 
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        IMessageSink GetServerContextSink(IMessageSink nextSink);
    }
}
