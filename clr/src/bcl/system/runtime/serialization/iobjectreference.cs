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
** Interface: IObjectReference
**
**                                        
**
** Purpose: Implemented by objects that are actually references
**          to a different object which can't be discovered until
**          this one is completely restored.  During the fixup stage,
**          any object implementing IObjectReference is asked for it's
**          "real" object and that object is inserted into the graph.
**
** Date:  June 16, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {

	using System;
    /// <include file='doc\IObjectReference.uex' path='docs/doc[@for="IObjectReference"]/*' />
    // Interface does not need to be marked with the serializable attribute
    public interface IObjectReference {
        /// <include file='doc\IObjectReference.uex' path='docs/doc[@for="IObjectReference.GetRealObject"]/*' />
        Object GetRealObject(StreamingContext context);
    
    }
}


