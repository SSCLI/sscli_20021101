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

    // TypedReference is basically only ever seen on the call stack, and in param arrays.
    //	These are blob that must be dealt with by the compiler.
    using System;
	using System.Reflection;
	using System.Runtime.CompilerServices;
    using CultureInfo = System.Globalization.CultureInfo;
	using FieldInfo = System.Reflection.FieldInfo;
    using System.Security.Permissions;

    /// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference"]/*' />
	[CLSCompliant(false)]
    public struct TypedReference
    {
    	private int Value;
    	private int Type;
    
		/// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference.MakeTypedReference"]/*' />
		[CLSCompliant(false)]	
        [ReflectionPermission(SecurityAction.LinkDemand, MemberAccess=true)]
		public static TypedReference MakeTypedReference(Object target,FieldInfo[] flds) {
			if (target == null)
				throw new ArgumentNullException("target");
			if (flds == null)
				throw new ArgumentNullException("flds");
			if (flds.Length == 0)
				throw new ArgumentException(Environment.GetResourceString("Arg_ArrayZeroError"));
            else {
                foreach (FieldInfo field in flds) {
                    if (!(field is RuntimeFieldInfo)) 
                        throw new ArgumentException(Environment.GetResourceString("Argument_MustBeRuntimeFieldInfo"));
                }
            }

			return InternalMakeTypedReferences(target,flds);
		}
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern TypedReference InternalMakeTypedReferences(Object target,FieldInfo[] flds);

    	/// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
			if (Type == 0)
				return 0;
			else
    			return __reftype(this).GetHashCode();
    	}
    
    	/// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference.Equals"]/*' />
    	public override bool Equals(Object o)
    	{
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_NYI"));
    	}

		/// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference.ToObject"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
		public extern static Object ToObject(TypedReference value);

		/// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference.GetTargetType"]/*' />
		public static Type GetTargetType (TypedReference value)
		{
			return __reftype(value);
		}

		/// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference.TargetTypeToken"]/*' />
		public static RuntimeTypeHandle TargetTypeToken (TypedReference value)
		{
			return __reftype(value).TypeHandle;
		}

		//	This may cause the type to be changed.
		/// <include file='doc\TypedReference.uex' path='docs/doc[@for="TypedReference.SetTypedReference"]/*' />
        [CLSCompliant(false)]	
		public static void SetTypedReference (TypedReference target, Object value)
    	{
			if (value == null)
				throw new ArgumentNullException("value");

			Type thisType = GetTargetType(target);
			Type thatType = value.GetType();
			if (!thisType.IsAssignableFrom(thatType))
				value = Convert.ChangeType(value, thisType, CultureInfo.InvariantCulture);

			// Directly make the assignment. Types are compatible				
    		InternalObjectToTypedReference(target,value,thisType.TypeHandle);
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern static void InternalObjectToTypedReference(TypedReference byrefValue,Object value,RuntimeTypeHandle typeHandle);
   	
	
    }

}
