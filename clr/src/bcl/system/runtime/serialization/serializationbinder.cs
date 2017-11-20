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
** Interface: SerializationBinder
**
**                                        
**
** Purpose: The base class of serialization binders.
**
** Date:  Jan 4, 2000
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System;

    /// <include file='doc\SerializationBinder.uex' path='docs/doc[@for="SerializationBinder"]/*' />
	[Serializable]
    public abstract class SerializationBinder {

        /// <include file='doc\SerializationBinder.uex' path='docs/doc[@for="SerializationBinder.BindToType"]/*' />
        public abstract Type BindToType(String assemblyName, String typeName);
    }
}
