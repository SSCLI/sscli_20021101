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
// Missing
//	This class represents a Missing Variant
////////////////////////////////////////////////////////////////////////////////
namespace System.Reflection {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	// This is not serializable because it is a reflection command.
    /// <include file='doc\Missing.uex' path='docs/doc[@for="Missing"]/*' />
    public sealed class Missing {
    
        //Package Private Constructor
        internal Missing(){
        }
    
    	/// <include file='doc\Missing.uex' path='docs/doc[@for="Missing.Value"]/*' />
    	public static readonly Missing Value = new Missing();
    }
}
