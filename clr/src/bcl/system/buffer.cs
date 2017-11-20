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
    
    //Only contains static methods.  Does not require serialization
    
	using System;
	using System.Runtime.CompilerServices;
    /// <include file='doc\Buffer.uex' path='docs/doc[@for="Buffer"]/*' />
    public sealed class Buffer
    {
        private Buffer() {
        }
    
    	// Copies from one primitive array to another primitive array without
    	// respecting types.  This calls memmove internally.
        /// <include file='doc\Buffer.uex' path='docs/doc[@for="Buffer.BlockCopy"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void BlockCopy(Array src, int srcOffset,
            Array dst, int dstOffset, int count);

		// A very simple and efficient array copy that assumes all of the
		// parameter validation has already been done.  All counts here are
		// in bytes.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalBlockCopy(Array src, int srcOffset,
            Array dst, int dstOffset, int count);

    	// Gets a particular byte out of the array.  The array must be an
    	// array of primitives.  
    	//
    	// This essentially does the following: 
    	// return ((byte*)array) + index.
    	//
    	/// <include file='doc\Buffer.uex' path='docs/doc[@for="Buffer.GetByte"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern byte GetByte(Array array, int index);
    
    	// Sets a particular byte in an the array.  The array must be an
    	// array of primitives.  
    	//
    	// This essentially does the following: 
    	// *(((byte*)array) + index) = value.
    	//
    	/// <include file='doc\Buffer.uex' path='docs/doc[@for="Buffer.SetByte"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern void SetByte(Array array, int index, byte value);
    
    	// Gets a particular byte out of the array.  The array must be an
    	// array of primitives.  
    	//
    	// This essentially does the following: 
    	// return array.length * sizeof(array.UnderlyingElementType).
    	//
    	/// <include file='doc\Buffer.uex' path='docs/doc[@for="Buffer.ByteLength"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern int ByteLength(Array array);
    }
}
