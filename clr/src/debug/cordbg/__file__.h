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
#ifdef _DEBUG
#define VER_FILEFLAGS		VS_FF_DEBUG
#else
#define VER_FILEFLAGS		VS_FF_SPECIALBUILD
#endif

#define VER_FILETYPE		VFT_DLL
#define VER_INTERNALNAME_STR	  "CorDbg.exe"
#define VER_INTERNALNAME_WSTR	 L"CorDbg.exe"
#define VER_FILEDESCRIPTION_STR   "Microsoft (R) Shared Source CLI Test Debugger Shell\0"
#define VER_FILEDESCRIPTION_WSTR L"Microsoft (R) Shared Source CLI Test Debugger Shell\0"
#define VER_ORIGFILENAME_STR      "cordbg.exe\0"
#define VER_ORIGFILENAME_WSTR    L"cordbg.exe\0"

