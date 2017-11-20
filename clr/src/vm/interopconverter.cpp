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
#include "common.h"

#include "vars.hpp"
#include "excep.h"
#include "interoputil.h"
#include "interopconverter.h"
#include "remoting.h"
#include "olevariant.h"
#include "comcallwrapper.h"


//--------------------------------------------------------
// Only IUnknown* is supported without FEATURE_COMINTEROP
//--------------------------------------------------------
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref)
{
    return ComCallWrapper::GetComIPFromObjectRef(poref);
}

OBJECTREF __stdcall GetObjectRefFromComIP(IUnknown* pUnk)
{
    return ComCallWrapper::GetObjectRefFromComIP(pUnk);
}

OBJECTREF __stdcall GetObjectRefFromComIP(IUnknown* pUnk, MethodTable* pMTClass)
{
    OBJECTREF oref;
    
    oref = ComCallWrapper::GetObjectRefFromComIP(pUnk);

    // make sure we can cast to the specified class
    if(oref != NULL && pMTClass != NULL)
    {
        GCPROTECT_BEGIN(oref)

        if(!ClassLoader::CanCastToClassOrInterface(oref, pMTClass->GetClass()))
        {
            CQuickBytes _qb;
			WCHAR* wszObjClsName = (WCHAR *)_qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));

            CQuickBytes _qb2;
			WCHAR* wszDestClsName = (WCHAR *)_qb2.Alloc(MAX_CLASSNAME_LENGTH * sizeof(WCHAR));

            oref->GetTrueClass()->_GetFullyQualifiedNameForClass(wszObjClsName, MAX_CLASSNAME_LENGTH);
            pMTClass->GetClass()->_GetFullyQualifiedNameForClass(wszDestClsName, MAX_CLASSNAME_LENGTH);
            COMPlusThrow(kInvalidCastException, IDS_EE_CANNOTCAST, wszObjClsName, wszDestClsName);
        }

        GCPROTECT_END();
    }

    return oref;
}

