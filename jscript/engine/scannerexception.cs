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
namespace Microsoft.JScript
{
    
    using System;
    
    internal class ScannerException : Exception{
      internal JSError m_errorId;

      internal ScannerException(JSError errorId) : base ("Syntax Error"){
        m_errorId = errorId;
      }
    }
}

