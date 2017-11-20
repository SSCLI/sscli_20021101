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
/*****************************************************************************
 **                                                                         **
 ** Xmlreader.h - general header for the shim parser                        **
 **                                                                         **
 *****************************************************************************/


#ifndef _XMLREADER_H_
#define _XMLREADER_H_

#include <corerror.h>

#define STARTUP_FOUND EMAKEHR(0xffff)           // This does not leak out of the shim so we can set it to anything
HRESULT XMLGetVersion(PCWSTR filename, LPWSTR* pVersion, LPWSTR* pImageVersion, LPWSTR* pBuildFlavor, BOOL *bSafeMode);
HRESULT XMLGetVersionFromStream(IStream* pCfgStream, DWORD dwReserved, LPWSTR* pVersion, LPWSTR* pImageVersion, LPWSTR* pBuildFlavor, BOOL *bSafeMode);

#endif
