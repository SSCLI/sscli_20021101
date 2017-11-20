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
// IExpando is an interface which allows Objects implemeningt this interface 
//	support the ability to modify the object by adding and removing members, 
//	represented by MemberInfo objects.
//
// Date: March 98
//
// The IExpando Interface.
namespace System.Runtime.InteropServices.Expando {
    
	using System;
	using System.Reflection;

    /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando"]/*' />
    public interface IExpando : IReflect
    {
    	// Add a new Field to the reflection object.  The field has
    	// name as its name.
        /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.AddField"]/*' />
        FieldInfo AddField(String name);

    	// Add a new Property to the reflection object.  The property has
    	// name as its name.
	    /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.AddProperty"]/*' />
	    PropertyInfo AddProperty(String name);

    	// Add a new Method to the reflection object.  The method has 
    	// name as its name and method is a delegate
    	// to the method.  
        /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.AddMethod"]/*' />
        MethodInfo AddMethod(String name, Delegate method);

    	// Removes the specified member.
        /// <include file='doc\IExpando.uex' path='docs/doc[@for="IExpando.RemoveMember"]/*' />
        void RemoveMember(MemberInfo m);
    }
}
