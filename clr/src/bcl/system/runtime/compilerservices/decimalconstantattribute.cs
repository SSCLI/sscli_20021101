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
namespace System.Runtime.CompilerServices
{
    /// <include file='doc\DecimalConstantAttribute.uex' path='docs/doc[@for="DecimalConstantAttribute"]/*' />
    [Serializable, AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter, Inherited=false),CLSCompliant(false)]
    public sealed class DecimalConstantAttribute : Attribute
    {
        /// <include file='doc\DecimalConstantAttribute.uex' path='docs/doc[@for="DecimalConstantAttribute.DecimalConstantAttribute"]/*' />
        public DecimalConstantAttribute(
            byte scale,
            byte sign,
            uint hi,
            uint mid,
            uint low
        )
        {
            dec = new System.Decimal((int) low, (int)mid, (int)hi, (sign != 0), scale);
        }

        /// <include file='doc\DecimalConstantAttribute.uex' path='docs/doc[@for="DecimalConstantAttribute.Value"]/*' />
        public System.Decimal Value
        {
            get {
                return dec;
            }
        }

        private System.Decimal dec;
    }
}

