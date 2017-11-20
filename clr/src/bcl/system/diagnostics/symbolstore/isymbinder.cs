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
** Class:  ISymbolBinder
**
**                                               
**
** Represents a symbol binder for managed code.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;
	
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymBinder.uex' path='docs/doc[@for="ISymbolBinder"]/*' />
    public interface ISymbolBinder
    {
        /// <include file='doc\ISymBinder.uex' path='docs/doc[@for="ISymbolBinder.GetReader"]/*' />
        ISymbolReader GetReader(int importer, String filename,
                                String searchPath);
    }
}
