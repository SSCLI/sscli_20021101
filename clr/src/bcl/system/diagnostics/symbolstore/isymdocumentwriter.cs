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
** Class:  ISymbolDocumentWriter
**
**                                               
**
** Represents a document referenced by a symbol store. A document is
** defined by a URL and a document type GUID. Document source can
** optionally be stored in the symbol store.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;
	
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymDocumentWriter.uex' path='docs/doc[@for="ISymbolDocumentWriter"]/*' />
    public interface ISymbolDocumentWriter
    {
        /// <include file='doc\ISymDocumentWriter.uex' path='docs/doc[@for="ISymbolDocumentWriter.SetSource"]/*' />
        // SetSource will store the raw source for a document into the
        // symbol store. An array of unsigned bytes is used instead of
        // character data to accomidate a wider variety of "source".
        void SetSource(byte[] source);
        /// <include file='doc\ISymDocumentWriter.uex' path='docs/doc[@for="ISymbolDocumentWriter.SetCheckSum"]/*' />
    
        // Check sum support.
        void SetCheckSum(Guid algorithmId,
                                byte[] checkSum);
    }
}
