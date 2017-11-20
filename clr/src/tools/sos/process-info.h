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

#ifndef INC_PROCESS_INFO
#define INC_PROCESS_INFO

BOOL GetExportByName (
  ULONG_PTR   BaseOfDll, 
  const char* ExportName,
  ULONG_PTR*  ExportAddress);

#endif /* ndef INC_PROCESS_INFO */

