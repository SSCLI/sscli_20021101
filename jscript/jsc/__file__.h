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
#define VER_FILEFLAGS            VS_FF_DEBUG
#else
#define VER_FILEFLAGS            VS_FF_SPECIALBUILD
#endif

#define VER_FILETYPE             VFT_APP
#define VER_INTERNALNAME_STR     "jsc.exe\0"
#define VER_ORIGINALFILENAME_STR "jsc.exe\0"
#define VER_FILEDESCRIPTION_STR  "Microsoft\256 JScript .NET Compiler\0"
#ifdef VER_PRODUCTNAME_STR
#undef VER_PRODUCTNAME_STR
#endif
#define VER_PRODUCTNAME_STR      "Microsoft\256 JScript .NET\0"
