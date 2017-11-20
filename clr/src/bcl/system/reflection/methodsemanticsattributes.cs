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
// This enum defines the Semantics that can be associated with Methods.  These
//	semantics are set for methods that have special meaning as part of a property
//	or event. 
//
// Date: June 99
//
namespace System.Reflection {
    
	using System;
	[Serializable()] 
    enum MethodSemanticsAttributes {
        Setter			    =   0x0001,	// 
        Getter			    =   0x0002,	// 
        Other				=   0x0004,	// 
        AddOn		        =   0x0008,	// 
        RemoveOn			=   0x0010,	// 
        Fire		        =   0x0020,	// 
    
    }
}
