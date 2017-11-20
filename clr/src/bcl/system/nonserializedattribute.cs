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
** Class: NonSerializedAttribute
**
**                                
**
** Purpose: Used to mark a member as being not-serialized
**
** Date: April 13, 2000
**
============================================================*/
namespace System {

    /// <include file='doc\NonSerializedAttribute.uex' path='docs/doc[@for="NonSerializedAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Field, Inherited=true)]
    public sealed class NonSerializedAttribute : Attribute {

        /// <include file='doc\NonSerializedAttribute.uex' path='docs/doc[@for="NonSerializedAttribute.NonSerializedAttribute"]/*' />
        public NonSerializedAttribute() {
        }
    }
}
