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
#ifndef _FUSIONWRAP_H_
#define _FUSIONWRAP_H_


class FusionWrap
{
public:
    static FCDECL4(INT32, GetNextAssembly, PVOID pEnum, PVOID* ppAppCtx, PVOID* ppName, UINT32 dwFlags);
    static FCDECL2(StringObject*, GetDisplayName, PVOID* pName, UINT32 dwDisplayFlags);
    static FCDECL1(void, ReleaseFusionHandle, PVOID* pp);
};


#endif // _FUSIONWRAP_H_
