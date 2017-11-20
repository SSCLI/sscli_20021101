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
** Class:  FusionWrap
**
** Purpose: helper to call fusion without com interop
**
===========================================================*/
#include "common.h"

#include "fusionwrap.h"

#include "comstring.h"


FCIMPL4(INT32, FusionWrap::GetNextAssembly, PVOID pEnum, PVOID* ppAppCtx, PVOID* ppName, UINT32 dwFlags)
{
    HRESULT hr = S_OK;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    BEGIN_ENSURE_PREEMPTIVE_GC();

    SafeRelease((IUnknown*)*ppAppCtx);
    *ppAppCtx = NULL;

    SafeRelease((IUnknown*)*ppName);
    *ppName = NULL;

    hr = ((IAssemblyEnum*)pEnum)->GetNextAssembly(
        ppAppCtx, (IAssemblyName**)ppName, dwFlags);

    END_ENSURE_PREEMPTIVE_GC();
    HELPER_METHOD_FRAME_END();

    return hr;
}
FCIMPLEND

FCIMPL2(StringObject*, FusionWrap::GetDisplayName, PVOID* pName, UINT32 dwDisplayFlags)
{
    STRINGREF   retVal = NULL;
    DWORD       iLen = 0;
    WCHAR*      ptr = NULL;
    CQuickBytes qb;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, retVal);

    BEGIN_ENSURE_PREEMPTIVE_GC();

    ((IAssemblyName*)pName)->GetDisplayName(NULL, &iLen, dwDisplayFlags);
        
    if (iLen > 0) {
        ptr = (WCHAR*)qb.Alloc((iLen + 1)* sizeof(WCHAR));
        if (!ptr)
            COMPlusThrowOM();
        ((IAssemblyName*)pName)->GetDisplayName(ptr, &iLen, dwDisplayFlags);
    }

    END_ENSURE_PREEMPTIVE_GC();

    if (iLen > 0) {
        retVal = COMString::NewString(ptr);
    }

    HELPER_METHOD_FRAME_END();

    return (StringObject*)OBJECTREFToObject(retVal);
}
FCIMPLEND

FCIMPL1(void, FusionWrap::ReleaseFusionHandle, PVOID* pp)
{
    HELPER_METHOD_FRAME_BEGIN_0();

    SafeRelease((IUnknown*)*pp);
    *pp = NULL;

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

