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
    /// <include file='doc\DateTimeConstantAttribute.uex' path='docs/doc[@for="DateTimeConstantAttribute"]/*' />
    [Serializable, AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter, Inherited=false)]
    public sealed class DateTimeConstantAttribute : CustomConstantAttribute
    {
        /// <include file='doc\DateTimeConstantAttribute.uex' path='docs/doc[@for="DateTimeConstantAttribute.DateTimeConstantAttribute"]/*' />
        public DateTimeConstantAttribute(long ticks)
        {
            date = new System.DateTime(ticks);
        }

        /// <include file='doc\DateTimeConstantAttribute.uex' path='docs/doc[@for="DateTimeConstantAttribute.Value"]/*' />
        public override Object Value
        {
            get {
                return date;
            }
        }

        private System.DateTime date;
    }
}

