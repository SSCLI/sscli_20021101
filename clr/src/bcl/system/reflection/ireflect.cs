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
// IReflect is an interface that defines a subset of the Type reflection methods.
//	This interface is used to access and invoke members of a Type.  It can be either
//	type based (like Type) or instance based (like Expando). This interface is used in 
//	combination with IExpando to model the IDispatchEx expando capibilities in
//	the interop layer. 
//
// Date: Sep 98
//
namespace System.Reflection {
    using System;
	using System.Runtime.InteropServices;
    using CultureInfo = System.Globalization.CultureInfo;
    
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect"]/*' />
    public interface IReflect
    {
    	// Return the requested method if it is implemented by the Reflection object.  The
    	// match is based upon the name and DescriptorInfo which describes the signature
    	// of the method. 
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethod"]/*' />
	    MethodInfo GetMethod(String name,BindingFlags bindingAttr,Binder binder,
    			Type[] types,ParameterModifier[] modifiers);

    	// Return the requested method if it is implemented by the Reflection object.  The
    	// match is based upon the name of the method.  If the object implementes multiple methods
    	// with the same name an AmbiguousMatchException is thrown.
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethod1"]/*' />
	    MethodInfo GetMethod(String name,BindingFlags bindingAttr);

	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethods"]/*' />
	    MethodInfo[] GetMethods(BindingFlags bindingAttr);
    	
    	// Return the requestion field if it is implemented by the Reflection object.  The
    	// match is based upon a name.  There cannot be more than a single field with
    	// a name.
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetField"]/*' />
	    FieldInfo GetField(
	            String name,
	            BindingFlags bindingAttr);

	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetFields"]/*' />
	    FieldInfo[] GetFields(
	            BindingFlags bindingAttr);

    	// Return the property based upon name.  If more than one property has the given
    	// name an AmbiguousMatchException will be thrown.  Returns null if no property
    	// is found.
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperty"]/*' />
	    PropertyInfo GetProperty(
	            String name,
	            BindingFlags bindingAttr);
    	
    	// Return the property based upon the name and Descriptor info describing the property
    	// indexing.  Return null if no property is found.
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperty1"]/*' />
	    PropertyInfo GetProperty(
				String name,
				BindingFlags bindingAttr,
				Binder binder,	            
				Type returnType,
				Type[] types,
				ParameterModifier[] modifiers);
    	
    	// Returns an array of PropertyInfos for all the properties defined on 
    	// the Reflection object.
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperties"]/*' />
	    PropertyInfo[] GetProperties(
	            BindingFlags bindingAttr);

    	// Return an array of members which match the passed in name.
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMember"]/*' />
	    MemberInfo[] GetMember(
	            String name,
	            BindingFlags bindingAttr);

    	// Return an array of all of the members defined for this object.
	    /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMembers"]/*' />
	    MemberInfo[] GetMembers(
	            BindingFlags bindingAttr);
    	
    	// Description of the Binding Process.
    	// We must invoke a method that is accessable and for which the provided
    	// parameters have the most specific match.  A method may be called if
    	// 1. The number of parameters in the method declaration equals the number of 
    	//	arguments provided to the invocation
    	// 2. The type of each argument can be converted by the binder to the
    	//	type of the type of the parameter.
    	// 
    	// The binder will find all of the matching methods.  These method are found based
    	// upon the type of binding requested (MethodInvoke, Get/Set Properties).  The set
    	// of methods is filtered by the name, number of arguments and a set of search modifiers
    	// defined in the Binder.
    	// 
    	// After the method is selected, it will be invoked.  Accessability is checked
    	// at that point.  The search may be control which set of methods are searched based
    	// upon the accessibility attribute associated with the method.
    	// 
    	// The BindToMethod method is responsible for selecting the method to be invoked.
    	// For the default binder, the most specific method will be selected.
    	// 
        // This will invoke a specific member...
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.InvokeMember"]/*' />
        Object InvokeMember(
                String name,
                BindingFlags invokeAttr,
				Binder binder,
                Object target,
                Object[] args,
                ParameterModifier[] modifiers,
                CultureInfo culture,
                String[] namedParameters);

    	// Return the underlying Type that represents the IReflect Object.  For expando object,
    	// this is the (Object) IReflectInstance.GetType().  For Type object it is this.
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.UnderlyingSystemType"]/*' />
        Type UnderlyingSystemType {
            get;
        }	
    }
}
