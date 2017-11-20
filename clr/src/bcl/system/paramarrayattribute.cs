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
/*=============================================================================
**
** Class: ParamArrayAttribute
**
**                                                          
**
** Purpose: Container for assemblies.
**
** Date: Mar 01, 2000
**
=============================================================================*/
namespace System
{
//This class contains only static members and does not need to be serializable 
   /// <include file='doc\ParamArrayAttribute.uex' path='docs/doc[@for="ParamArrayAttribute"]/*' />
   [AttributeUsage (AttributeTargets.Parameter, Inherited=true, AllowMultiple=false)]
   public sealed class ParamArrayAttribute : Attribute
   {
      /// <include file='doc\ParamArrayAttribute.uex' path='docs/doc[@for="ParamArrayAttribute.ParamArrayAttribute"]/*' />
      public ParamArrayAttribute () {}  
   }
}
