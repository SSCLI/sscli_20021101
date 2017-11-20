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
using System;

namespace System.Runtime.CompilerServices 
{
    /// <include file='doc\RequiredAttributeAttribute.uex' path='docs/doc[@for="RequiredAttributeAttribute"]/*' />
    [Serializable, AttributeUsage (AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface, 
                     AllowMultiple=false, Inherited=false)]
    public sealed class RequiredAttributeAttribute : Attribute 
    {
        private Type requiredContract;

        /// <include file='doc\RequiredAttributeAttribute.uex' path='docs/doc[@for="RequiredAttributeAttribute.RequiredAttributeAttribute"]/*' />
        public RequiredAttributeAttribute (Type requiredContract) 
        {
            this.requiredContract= requiredContract;
        }
        /// <include file='doc\RequiredAttributeAttribute.uex' path='docs/doc[@for="RequiredAttributeAttribute.RequiredContract"]/*' />
        public Type RequiredContract 
        {
            get { return this.requiredContract; }
        }
    }
}
