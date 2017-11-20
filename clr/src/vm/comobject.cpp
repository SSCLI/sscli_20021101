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
** Header: COMObject.cpp
**
**                                             
**
** Purpose: Native methods on System.Object
**
** Date:  March 27, 1998
** 
===========================================================*/

#include "common.h"

#include <object.h>
#include "excep.h"
#include "vars.hpp"
#include "field.h"
#include "comobject.h"
#include "comclass.h"
#include "comsynchronizable.h"
#include "gcscan.h"
#include "remoting.h"


/********************************************************************/
/* gets an object's 'value'.  For normal classes, with reference
   based semantics, this means the object's pointer.  For boxed
   primitive types, it also means just returning the pointer (because
   they are immutable), for other value class, it means returning
   a boxed copy.  */

FCIMPL1(Object*, ObjectNative::GetObjectValue, Object* obj) 
    if (obj == 0)
        return(obj);

    MethodTable* pMT = obj->GetMethodTable();
    if (pMT->GetNormCorElementType() != ELEMENT_TYPE_VALUETYPE)
        return(obj);

    Object* retVal;
    OBJECTREF objRef(obj);
    HELPER_METHOD_FRAME_BEGIN_RET_1(objRef);    // Set up a frame
    retVal = OBJECTREFToObject(FastAllocateObject(pMT));
    CopyValueClass(retVal->GetData(), objRef->GetData(), pMT, retVal->GetAppDomain());
    HELPER_METHOD_FRAME_END();

    return(retVal);
FCIMPLEND

// Note that we obtain a sync block index without actually building a sync block.
// That's because a lot of objects are hashed, without requiring support for
FCIMPL1(INT32, ObjectNative::GetHashCode, Object* objRef) {
    if (objRef == 0)
        return 0;

    VALIDATEOBJECTREF(objRef);

    DWORD      idx = objRef->GetSyncBlockIndex();

    _ASSERTE(idx != 0);

    // If the syncblock already exists, it has now become precious.  Otherwise the
    // hash code would not be stable across GCs.
    SyncBlock *psb = objRef->PassiveGetSyncBlock();

    if (psb)
        psb->SetPrecious();

    return idx;
}
FCIMPLEND


//
// Compare by ref for normal classes, by value for value types.
//  
//

FCIMPL2(BOOL, ObjectNative::Equals, Object *pThisRef, Object *pCompareRef)
{
    if (pThisRef == pCompareRef)    
        return TRUE;

    // Since we are in FCALL, we must handle NULL specially.
    if (pThisRef == NULL || pCompareRef == NULL)
        return FALSE;

    MethodTable *pThisMT = pThisRef->GetMethodTable();

    // If it's not a value class, don't compare by value
    if (!pThisMT->IsValueClass())
        return FALSE;

    // Make sure they are the same type.
    if (pThisMT != pCompareRef->GetMethodTable())
        return FALSE;

    // Compare the contents (size - vtable - sink block index).
    BOOL ret = !memcmp((void *) (pThisRef+1), (void *) (pCompareRef+1), pThisRef->GetMethodTable()->GetBaseSize() - sizeof(Object) - sizeof(int));
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND


FCIMPL1(ReflectClassBaseObject*, ObjectNative::GetClass, Object* pThis)
{
    OBJECTREF            objRef   = NULL;
    REFLECTCLASSBASEREF  refClass = NULL;
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, objRef, refClass);
    
    objRef = ObjectToOBJECTREF(pThis);
    EEClass* pClass = objRef->GetTrueMethodTable()->GetClass();

    // Arrays of Pointers are implemented by reflection,
    //  defer to COMClass for them.
    if (pClass->IsArrayClass()) 
    {
        // This code is essentially duplicated in GetExistingClass.
        ArrayBase* array = (ArrayBase*) OBJECTREFToObject(objRef);
        TypeHandle arrayType = array->GetTypeHandle();
        refClass = (REFLECTCLASSBASEREF) arrayType.AsArray()->CreateClassObj();
    }
    else if (objRef->GetClass()->IsThunking()) 
    {
        refClass = CRemotingServices::GetClass(objRef);
    }
    else
    {
        refClass = (REFLECTCLASSBASEREF) pClass->GetExposedClassObject();
    }

    _ASSERTE(refClass != NULL);

    HELPER_METHOD_FRAME_END();

    return (ReflectClassBaseObject*)(OBJECTREFToObject(refClass));
}
FCIMPLEND

// *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING
// 
//   IF YOU CHANGE THIS METHOD, PLEASE ALSO MAKE CORRESPONDING CHANGES TO
//                CtxProxy::Clone() AS DESCRIBED BELOW.
//
// *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING

FCIMPL1(Object*, ObjectNative::Clone, Object* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF refClone = NULL;;
    OBJECTREF refThis  = ObjectToOBJECTREF(pThisUNSAFE);;

    if (refThis == NULL)
        FCThrow(kNullReferenceException);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refClone, refThis);

    // ObjectNative::Clone() ensures that the source and destination are always in
    // the same context.  CtxProxy::Clone() must clone an object into a different
    // context.  Leaving aside that difference, the rest of the two methods should
    // be the same and should be maintained together.


    MethodTable* pMT;
    DWORD cb;

    pMT = refThis->GetMethodTable();

    // assert that String has overloaded the Clone() method
    _ASSERTE(pMT != g_pStringClass);

    cb = pMT->GetBaseSize() - sizeof(ObjHeader);


    if (pMT->IsArray()) {

        BASEARRAYREF base = (BASEARRAYREF)refThis;
        cb += base->GetNumComponents() * pMT->GetComponentSize();

        refClone = DupArrayForCloning(base);
    } else {
        // We don't need to call the <cinit> because we know
        //  that it has been called....(It was called before this was created)
        refClone = AllocateObject(pMT);
    }

    // copy contents of "this" to the clone
    memcpyGCRefs(OBJECTREFToObject(refClone), OBJECTREFToObject(refThis), cb);

    HELPER_METHOD_FRAME_END();
        
    return OBJECTREFToObject(refClone);
}
FCIMPLEND

FCIMPL3(INT32, ObjectNative::WaitTimeout, INT32 exitContext, INT32 Timeout, Object* pThisUNSAFE)
{
    INT32 retVal;
    OBJECTREF pThis = (OBJECTREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(pThis);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if ((Timeout < 0) && (Timeout != INFINITE_TIMEOUT))
        COMPlusThrowArgumentOutOfRange(L"millisecondsTimeout", L"ArgumentOutOfRange_NeedNonNegNum");

    retVal = pThis->Wait(Timeout, exitContext);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

FCIMPL1(void, ObjectNative::Pulse, Object* pThisUNSAFE)
{
    OBJECTREF pThis = (OBJECTREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pThis);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    pThis->Pulse();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL1(void, ObjectNative::PulseAll, Object* pThisUNSAFE)
{
    OBJECTREF pThis = (OBJECTREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pThis);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    pThis->PulseAll();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// This method will return a Class object for the object
//  iff the Class object has already been created.  
//  If the Class object doesn't exist then you must call the GetClass() method.
FCIMPL1(Object*, ObjectNative::GetExistingClass, Object* thisRef) {

    if (thisRef == NULL)
        FCThrow(kNullReferenceException);

    
    EEClass* pClass = thisRef->GetTrueMethodTable()->GetClass();

    // For marshalbyref classes, let's just punt for the moment
    if (pClass->IsMarshaledByRef())
        return 0;

    OBJECTREF refClass;
    if (pClass->IsArrayClass()) {
        // This code is essentially a duplicate of the code in GetClass, done for perf reasons.
        ArrayBase* array = (ArrayBase*) thisRef;
        TypeHandle arrayType;
        // Erect a GC Frame around the call to GetTypeHandle, since on the first call,
        // it can call AppDomain::RaiseTypeResolveEvent, which allocates Strings and calls
        // a user-provided managed callback.  Yes, we have to do an allocation to do a
        // lookup, since TypeHandles are used as keys.                                                      
        HELPER_METHOD_FRAME_BEGIN_RET_1(array);
        arrayType = array->GetTypeHandle();
        refClass = COMClass::QuickLookupExistingArrayClassObj(arrayType.AsArray());
        HELPER_METHOD_FRAME_END();
    }
    else 
        refClass = pClass->GetExistingExposedClassObject();
    return OBJECTREFToObject(refClass);
}
FCIMPLEND


