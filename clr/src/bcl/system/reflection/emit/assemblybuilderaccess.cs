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
namespace System.Reflection.Emit {
    
	using System;
    // This enumeration defines the access modes for a dynamic assembly.
    // EE uses these enum values..look for m_dwDynamicAssemblyAccess in Assembly.hpp
    /// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess"]/*' />
	[Serializable]
    public enum AssemblyBuilderAccess
    {
    	/// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess.Run"]/*' />
    	Run = 1,
    	/// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess.Save"]/*' />
    	Save = 2,
    	/// <include file='doc\AssemblyBuilderAccess.uex' path='docs/doc[@for="AssemblyBuilderAccess.RunAndSave"]/*' />
    	RunAndSave = Run | Save,
    }


}
