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
** File:    AssemblyNameProxy
**
** Author:  
**
** Purpose: Remotable version the AssemblyName
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System.Reflection {
    using System;

    /// <include file='doc\AssemblyNameProxy.uex' path='docs/doc[@for="AssemblyNameProxy"]/*' />
    public class AssemblyNameProxy : MarshalByRefObject 
    {
        /// <include file='doc\AssemblyNameProxy.uex' path='docs/doc[@for="AssemblyNameProxy.GetAssemblyName"]/*' />
        public AssemblyName GetAssemblyName(String assemblyFile)
        {
            return AssemblyName.nGetFileInformation(assemblyFile);
        }
    }
    
}
