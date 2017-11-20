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
** Header:  AppDomain.i
**
** Purpose: Implements AppDomain (loader domain) architecture
** inline functions
**
** Date:  June 27, 2000
**
===========================================================*/
#ifndef _APPDOMAIN_I
#define _APPDOMAIN_I

#include "appdomain.hpp"

inline void AppDomain::SetUnloadInProgress()
{
    SystemDomain::System()->SetUnloadInProgress(this);
}

inline void AppDomain::SetUnloadComplete()
{
    SystemDomain::System()->SetUnloadComplete();
}


#endif  // _APPDOMAIN_I




