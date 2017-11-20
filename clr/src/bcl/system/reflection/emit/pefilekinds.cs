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
    // This Enum matchs the CorFieldAttr defined in CorHdr.h
    /// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds"]/*' />
	[Serializable()] 
    public enum PEFileKinds
    {
        /// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds.Dll"]/*' />
        Dll			=   0x0001,  
		/// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds.ConsoleApplication"]/*' />
		ConsoleApplication = 0x0002,
		/// <include file='doc\PEFileKinds.uex' path='docs/doc[@for="PEFileKinds.WindowApplication"]/*' />
		WindowApplication = 0x0003,
    }
}
