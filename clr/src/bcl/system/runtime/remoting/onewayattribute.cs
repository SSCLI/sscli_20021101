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
** File:    OneWayAttribute.cs
**
** Attribute for marking methods as one way
** 
**                                              
**
** Date:    Nov 8, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
	using System.Runtime.Remoting;
	using System;
    /// <include file='doc\OneWayAttribute.uex' path='docs/doc[@for="OneWayAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method)]       // bInherited
    public class OneWayAttribute : Attribute
    {
    }

}
