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
#ifndef _FUSION_INC_FUSIONHEAP_H_INCLUDED_
#define _FUSION_INC_FUSIONHEAP_H_INCLUDED_

#include "debmacro.h"

#define FUSION_NEW_SINGLETON(_type) new _type
#define FUSION_NEW_ARRAY(_type, _n) new _type[_n]
#define FUSION_DELETE_ARRAY(_ptr) delete [] _ptr
#define FUSION_DELETE_SINGLETON(_ptr) delete _ptr

#define NEW(_type) FUSION_NEW_SINGLETON(_type)

#endif // !defined(_FUSION_INC_FUSIONHEAP_H_INCLUDED_)
