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
namespace System {
    
	using System;
    using System.Runtime.InteropServices;

    /// <include file='doc\IServiceObjectProvider.uex' path='docs/doc[@for="IServiceProvider"]/*' />
    public interface IServiceProvider
    {
        /// <include file='doc\IServiceObjectProvider.uex' path='docs/doc[@for="IServiceProvider.GetService"]/*' />
	// Interface does not need to be marked with the serializable attribute
        Object GetService(Type serviceType);
    }
}
