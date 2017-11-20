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
namespace System.Threading {
    
    //This class has only static members and doesn't require serialization.
    using System;
	using System.Runtime.CompilerServices;
    /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked"]/*' />
    public sealed class Interlocked
    {
        private Interlocked() {
        }
        
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.Increment"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern int Increment(ref int location);
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.Decrement"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern int Decrement(ref int location);
                /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.Increment1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern long Increment(ref long location);
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.Decrement1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern long Decrement(ref long location);

        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.Exchange"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern int Exchange(ref int location1, int value);
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.CompareExchange"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern int CompareExchange(ref int location1, int value, int comparand);
    
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.Exchange1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern float Exchange(ref float location1, float value);
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.CompareExchange1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern float CompareExchange(ref float location1, float value, float comparand);
    
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.Exchange2"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Object Exchange(ref Object location1, Object value);
        /// <include file='doc\Interlocked.uex' path='docs/doc[@for="Interlocked.CompareExchange2"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Object CompareExchange(ref Object location1, Object value, Object comparand);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern IntPtr CompareExchangePointer(ref IntPtr location1, IntPtr value, IntPtr comparand);
    }
}
