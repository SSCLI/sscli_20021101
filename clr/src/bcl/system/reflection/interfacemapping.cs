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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Interface Map.  This struct returns the mapping of an interface into the actual methods on a class
//	that implement that interface.
//
// Date: March 2000
//
namespace System.Reflection {
    using System;

	/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping"]/*' />
	public struct InterfaceMapping {
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.TargetType"]/*' />
		public Type				TargetType;			// The type implementing the interface
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.InterfaceType"]/*' />
		public Type				InterfaceType;		// The type representing the interface
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.TargetMethods"]/*' />
		public MethodInfo[]		TargetMethods;		// The methods implementing the interface
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.InterfaceMethods"]/*' />
		public MethodInfo[]		InterfaceMethods;	// The methods defined on the interface
	}
}
