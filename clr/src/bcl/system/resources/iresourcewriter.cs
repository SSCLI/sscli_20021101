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
** Class:  IResourceWriter
**
**                                                    
**
** Purpose: Default way to write strings to a COM+ resource 
** file.
**
** Date:  March 26, 1999
** 
===========================================================*/
namespace System.Resources {
	using System;
	using System.IO;
    /// <include file='doc\IResourceWriter.uex' path='docs/doc[@for="IResourceWriter"]/*' />
    public interface IResourceWriter : IDisposable
    {
	// Interface does not need to be marked with the serializable attribute
    	// Adds a string resource to the list of resources to be written to a file.
    	// They aren't written until WriteFile() is called.
    	// 
    	/// <include file='doc\IResourceWriter.uex' path='docs/doc[@for="IResourceWriter.AddResource"]/*' />
    	void AddResource(String name, String value);
    
    	// Adds a resource to the list of resources to be written to a file.
    	// They aren't written until WriteFile() is called.
    	// 
    	/// <include file='doc\IResourceWriter.uex' path='docs/doc[@for="IResourceWriter.AddResource1"]/*' />
    	void AddResource(String name, Object value);
    
    	// Adds a named byte array as a resource to the list of resources to 
    	// be written to a file. They aren't written until WriteFile() is called.
    	// 
    	/// <include file='doc\IResourceWriter.uex' path='docs/doc[@for="IResourceWriter.AddResource2"]/*' />
    	void AddResource(String name, byte[] value);
    
        // Closes the underlying resource file.
        /// <include file='doc\IResourceWriter.uex' path='docs/doc[@for="IResourceWriter.Close"]/*' />
        void Close();

    	// After calling AddResource, this writes all resources to the output
        // stream.  This does NOT close the output stream.
    	/// <include file='doc\IResourceWriter.uex' path='docs/doc[@for="IResourceWriter.Generate"]/*' />
    	void Generate();
    }
}
