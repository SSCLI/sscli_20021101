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
// MethodInfo is an abstract base class representing a Method.  Methods may be defined
//	either on a type or as a global.  The system provides a __RuntimeMethodInfo representing
//	the real methods of the system.
//
// Date: March 98
//
namespace System.Reflection {
	using System;
	using System.Runtime.InteropServices;
	using CultureInfo = System.Globalization.CultureInfo;

    ////////////////////////////////////////////////////////////////////////////////
    //   Method is the class which represents a Method. These are accessed from
    //   Class through getMethods() or getMethod(). This class contains information
    //   about each method and also allows the method to be dynamically invoked 
    //   on an instance.
    ////////////////////////////////////////////////////////////////////////////////
    /// <include file='doc\MethodInfo.uex' path='docs/doc[@for="MethodInfo"]/*' />
	[Serializable()] 
    abstract public class MethodInfo : MethodBase
    {
    	/// <include file='doc\MethodInfo.uex' path='docs/doc[@for="MethodInfo.MethodInfo"]/*' />
    	protected MethodInfo() {}
    
    	/////////////// MemberInfo Routines /////////////////////////////////////////////
    	
    	// The Member type Method.
    	/// <include file='doc\MethodInfo.uex' path='docs/doc[@for="MethodInfo.MemberType"]/*' />
    	public override MemberTypes MemberType {
    		get {return System.Reflection.MemberTypes.Method;}
    	}
    		
    	// Return type Type of the methods return type
    	/// <include file='doc\MethodInfo.uex' path='docs/doc[@for="MethodInfo.ReturnType"]/*' />
    	public abstract Type ReturnType {
    		get; 
    	}

		// This method will return an object that represents the 
		//	custom attributes for the return type.
		/// <include file='doc\MethodInfo.uex' path='docs/doc[@for="MethodInfo.ReturnTypeCustomAttributes"]/*' />
		public abstract ICustomAttributeProvider ReturnTypeCustomAttributes {
    		get; 
		}
    						  
    	// Return the base implementation for a method.
    	/// <include file='doc\MethodInfo.uex' path='docs/doc[@for="MethodInfo.GetBaseDefinition"]/*' />
    	abstract public MethodInfo GetBaseDefinition();

    	// Return the parents definition for a method in an inheritance chain.
    	internal virtual MethodInfo GetParentDefinition()
		{
			return null;
		}
		    	
    }
}
