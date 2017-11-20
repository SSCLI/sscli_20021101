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
** Class:  IObjectHandle
**
** Author: 
**
** IObjectHandle defines the interface for unwrapping objects.
** Objects that are marshal by value object can be returned through 
** an indirection allowing the caller to control when the
** object is loaded into their domain. The caller can unwrap
** the object from the indirection through this interface.
**
** Date:  January 24, 2000
** 
===========================================================*/
namespace System.Runtime.Remoting {

	using System;
	using System.Runtime.InteropServices;

    /// <include file='doc\IObjectHandle.uex' path='docs/doc[@for="IObjectHandle"]/*' />
    public interface IObjectHandle {
        /// <include file='doc\IObjectHandle.uex' path='docs/doc[@for="IObjectHandle.Unwrap"]/*' />
        // Unwrap the object. Implementers of this interface
        // typically have an indirect referece to another object.
        Object Unwrap();
    }
}

