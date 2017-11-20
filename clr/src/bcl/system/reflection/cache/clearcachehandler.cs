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
** Class: ClearCacheHandler
**
**                                
**
** Purpose: Delegate fired when clearing the cache used in
** Reflection
**
** Date: December 20, 2000
**
============================================================*/
namespace System.Reflection.Cache {
    internal delegate void ClearCacheHandler(Object sender, ClearCacheEventArgs cacheEventArgs);
}
