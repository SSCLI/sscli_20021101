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
** Class: SerializableAttribute
**
**                                
**
** Purpose: Used to mark a class as being serializable
**
** Date: April 13, 2000
**
============================================================*/
namespace System {

    using System;

    /// <include file='doc\SerializableAttribute.uex' path='docs/doc[@for="SerializableAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Delegate, Inherited = false)]
    public sealed class SerializableAttribute : Attribute {

        /// <include file='doc\SerializableAttribute.uex' path='docs/doc[@for="SerializableAttribute.SerializableAttribute"]/*' />
        public SerializableAttribute() {
        }
    }
}
