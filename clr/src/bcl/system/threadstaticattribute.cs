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
** File:        ThreadStaticAttribute.cs
**
**                                                    
**
** Purpose:     Custom attribute to indicate that the field should be treated 
**              as a static relative to a thread.
**          
**
** Date:        Jan 18, 2000
**
===========================================================*/
namespace System {
    
    using System;

    /// <include file='doc\ThreadStaticAttribute.uex' path='docs/doc[@for="ThreadStaticAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Field, Inherited = false),Serializable()] 
    public class  ThreadStaticAttribute : Attribute
    {
        /// <include file='doc\ThreadStaticAttribute.uex' path='docs/doc[@for="ThreadStaticAttribute.ThreadStaticAttribute"]/*' />
        public ThreadStaticAttribute()
        {
        }
    }
}
