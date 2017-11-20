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
** File:    ContextBoundObject.cs
**       
**
** Purpose: Defines the root type for all context bound types
**          
** Date:    Sep 30, 1999
**
===========================================================*/
namespace System {   
    
	using System;
    /// <include file='doc\ContextBoundObject.uex' path='docs/doc[@for="ContextBoundObject"]/*' />
	[Serializable()]
    public abstract class ContextBoundObject : MarshalByRefObject
    {
    }
}
