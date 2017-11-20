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
** Class:  ISymbolNamespace
**
**                                               
**
** Represents a namespace within a symbol reader.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;
	
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace"]/*' />
    public interface ISymbolNamespace
    {
        /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace.Name"]/*' />
        // Get the name of this namespace
        String Name { get; }
        /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace.GetNamespaces"]/*' />
    
        // Get the children of this namespace
        ISymbolNamespace[] GetNamespaces();
        /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace.GetVariables"]/*' />
    
        // Get the variables in this namespace
        ISymbolVariable[] GetVariables();
    }
}
