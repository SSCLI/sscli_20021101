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
/*============================================================
**
** Class:  MethodRental
**
**                                                
**
** MethodRental class is to provide a fast way to swap method body implementation
**  given a method of a class
**
** Date:  May 1999
** 
===========================================================*/
namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    using System.Threading;
	using System.Runtime.CompilerServices;

    // MethodRental class provides the ability to insert a new method body of an 
    // existing method on a class defined in a DynamicModule.
    // Can throw OutOfMemory exception.
    // 
    //This class contains only static methods and does not require serialization.
    /// <include file='doc\MethodRental.uex' path='docs/doc[@for="MethodRental"]/*' />
    sealed public class MethodRental
    {
    	/// <include file='doc\MethodRental.uex' path='docs/doc[@for="MethodRental.JitOnDemand"]/*' />
    	public const int JitOnDemand			= 0x0000;		// jit the method body when it is necessary
    	/// <include file='doc\MethodRental.uex' path='docs/doc[@for="MethodRental.JitImmediate"]/*' />
    	public const int JitImmediate		= 0x0001;		// jit the method body now 
    
    	/// <include file='doc\MethodRental.uex' path='docs/doc[@for="MethodRental.SwapMethodBody"]/*' />
    	public static void SwapMethodBody(
    		Type		cls,				// [in] class containing the method
    		int			methodtoken,		// [in] method token
    		IntPtr		rgIL,				// [in] pointer to bytes
    		int			methodSize,			// [in] the size of the new method body in bytes
    		int			flags)				// [in] flags
		{
            if (methodSize <= 0 || methodSize >= 0x3f0000)
                throw new ArgumentException(Environment.GetResourceString("Argument_BadSizeForData"), "methodSize");

            if (cls==null)
                throw new ArgumentNullException("cls");
                
            // can only swap method body on dynamic module
			if (!(cls.Module is ModuleBuilder))
				throw new NotSupportedException(Environment.GetResourceString("NotSupported_NotDynamicModule"));

            RuntimeType rType;

            if (cls is TypeBuilder)
            {
                // If it is a TypeBuilder, make sure that TypeBuilder is already been baked.
                TypeBuilder typeBuilder = (TypeBuilder) cls;
                if (typeBuilder.m_hasBeenCreated == false)
                    throw new NotSupportedException(String.Format(Environment.GetResourceString("NotSupported_NotAllTypesAreBaked"), typeBuilder.Name)); 
                    
                // get the corresponding runtime type for the TypeBuilder.
                rType = typeBuilder.m_runtimeType as RuntimeType;
                
            }
            else
            {
                rType = cls as RuntimeType;
            }

            if (rType == null)
                throw new ArgumentException(Environment.GetResourceString("Argument_MustBeRuntimeType"), "cls");

            StackCrawlMark mark = StackCrawlMark.LookForMyCaller;

            SwapMethodBodyHelper(rType, methodtoken, rgIL, methodSize, flags, ref mark);
		}
    
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SwapMethodBodyHelper(
    		RuntimeType	cls,				// [in] class containing the method
    		int			methodtoken,		// [in] method token
    		IntPtr		rgIL,				// [in] pointer to bytes
    		int			methodSize,			// [in] the size of the new method body in bytes
            int         flags,              // [in] flags
            ref StackCrawlMark stackMark);  // [in] stack crawl mark used to find caller

    	// private constructor to prevent class to be constructed.
    	private MethodRental() {}
    }
}
