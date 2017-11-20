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
//
// OBJECT.INL
//
// Definitions inline functions of a Com+ Object
//

#ifndef _OBJECT_INL_
#define _OBJECT_INL_

#include "object.h"

inline DWORD Object::GetAppDomainIndex()
{
#ifndef _DEBUG
    // ok to cast to AppDomain because we know it's a real AppDomain if it's not shared
    if (!GetMethodTable()->IsShared())
        return ((AppDomain*)GetMethodTable()->GetModule()->GetDomain())->GetIndex();
#endif
        return GetHeader()->GetAppDomainIndex();
}

#endif  // _OBJECT_INL_
