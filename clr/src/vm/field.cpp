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
// ===========================================================================
// File: Field.cpp
//
// ===========================================================================
// This file contains the implementation for FieldDesc methods.
// ===========================================================================
//

#include "common.h"

#include "encee.h"
#include "field.h"
#include "remoting.h"

// Return whether the field is a GC ref type
BOOL FieldDesc::IsObjRef()
{
    return CorTypeInfo::IsObjRef(GetFieldType());
}

// Return the type of the field, as a class.
TypeHandle FieldDesc::LoadType()
{
    PCCOR_SIGNATURE pSig;
    DWORD           cSig;

    GetSig(&pSig, &cSig);

    FieldSig        sig(pSig, GetModule());

    return sig.GetTypeHandle();
}

// Return the type of the field, as a class, but only if it's loaded.
TypeHandle FieldDesc::FindType()
{
    // Caller should have handled all the non-class cases, already.
    _ASSERTE(GetFieldType() == ELEMENT_TYPE_CLASS ||
             GetFieldType() == ELEMENT_TYPE_VALUETYPE);

    PCCOR_SIGNATURE pSig;
    DWORD           cSig;

    GetSig(&pSig, &cSig);

    FieldSig        sig(pSig, GetModule());

    // This may be the real type which includes other things
    //  beside class and value class such ass arrays
    _ASSERTE(sig.GetFieldType() == ELEMENT_TYPE_CLASS ||
             sig.GetFieldType() == ELEMENT_TYPE_VALUETYPE ||
             sig.GetFieldType() == ELEMENT_TYPE_STRING ||
             sig.GetFieldType() == ELEMENT_TYPE_VALUEARRAY ||
             sig.GetFieldType() == ELEMENT_TYPE_SZARRAY
             );

    return sig.GetTypeHandle(NULL, TRUE, TRUE);
}


void* FieldDesc::GetStaticAddress(void *base)
{

    void* ret = GetStaticAddressHandle(base);       // Get the handle

        // For value classes, the handle points at an OBJECTREF
        // which holds the boxed value class, so derefernce and unbox.  
    if (GetFieldType() == ELEMENT_TYPE_VALUETYPE && !IsRVA())
    {
        OBJECTREF obj = ObjectToOBJECTREF(*(Object**) ret);
        ret = obj->UnBox();
    }
    return ret;
}

    // static value classes are actually stored in their boxed form.  
    // this means that their address moves.  
void* FieldDesc::GetStaticAddressHandle(void *base)
{
    _ASSERTE(IsStatic());


    if (IsRVA()) 
    {
        Module* pModule = GetModule();

        void *ret;
            ret = pModule->ResolveILRVA(GetOffset(), TRUE);

        _ASSERTE(!pModule->IsPEFile() || !pModule->GetPEFile()->IsTLSAddress(ret));

        return(ret);
    }

    void *ret = ((BYTE *) base + GetOffset());

    // Since static object fields are handles, we need to dereferece to get the actual object
    // pointer, after first checking that the handle for this field exists.
    if (GetFieldType() == ELEMENT_TYPE_CLASS || GetFieldType() == ELEMENT_TYPE_VALUETYPE)
    {
        // Make sure the class's static handles & boxed structs are allocated
        GetMethodTableOfEnclosingClass()->CheckRestore();
        
        OBJECTREF *pObjRef = *((OBJECTREF**) ret);
        _ASSERTE(pObjRef);
        ret = (void*) pObjRef;
    }

    return ret;
}

// These routines encapsulate the operation of getting and setting
// fields.
void    FieldDesc::GetInstanceField(LPVOID o, VOID * pOutVal)
{
    THROWSCOMPLUSEXCEPTION();

    // Check whether we are getting a field value on a proxy or a marshalbyref
    // class. If so, then ask remoting services to extract the value from the 
    //instance
    if(((Object*)o)->GetClass()->IsMarshaledByRef() ||
       ((Object*)o)->IsThunking())
    {

        if (CTPMethodTable::IsTPMethodTable(((Object*)o)->GetMethodTable()))
        {
            Object *puo = (Object *) CRemotingServices::AlwaysUnwrap((Object*) o);
            OBJECTREF unwrapped = ObjectToOBJECTREF(puo);
            
#ifdef PROFILING_SUPPORTED

         GCPROTECT_BEGIN(unwrapped); // protect from RemotingClientInvocationStarted

			BOOL fIsRemoted = FALSE;

            // If profiling is active, notify it that remoting stuff is kicking in,
            // if AlwaysUnwrap returned an identical object pointer which means that
            // we are definitely going through remoting for this access.
            if (CORProfilerTrackRemoting())
            {
                fIsRemoted = ((LPVOID)puo == o);
                if (fIsRemoted)
                {
                    g_profControlBlock.pProfInterface->RemotingClientInvocationStarted(
                        reinterpret_cast<ThreadID>(GetThread()));
                }
            }
#endif // PROFILING_SUPPORTED

            CRemotingServices::FieldAccessor(this, unwrapped, pOutVal, TRUE);

#ifdef PROFILING_SUPPORTED
            if (CORProfilerTrackRemoting() && fIsRemoted)
                g_profControlBlock.pProfInterface->RemotingClientInvocationFinished(
                    reinterpret_cast<ThreadID>(GetThread()));

         GCPROTECT_END();           // protect from RemotingClientInvocationStarted
			
#endif // PROFILING_SUPPORTED


        }
        else
        {
            CRemotingServices::FieldAccessor(this, ObjectToOBJECTREF((Object*)o),
                                             pOutVal, TRUE);
        }
         

    }
    else
    {
        _ASSERTE(GetEnclosingClass()->IsValueClass() || !((Object*) o)->IsThunking());

        // Unbox the value class
        if(GetEnclosingClass()->IsValueClass())
        {
            o = ObjectToOBJECTREF((Object *)o)->UnBox();
        }
        LPVOID pFieldAddress = GetAddress(o);
        UINT cbSize = GetSize();
           
        switch (cbSize)
        {
            case 1:
                *(INT8*)pOutVal = *(INT8*)pFieldAddress;
                break;
        
            case 2:
                *(INT16*)pOutVal = *(INT16*)pFieldAddress;
                break;
        
            case 4:
                *(INT32*)pOutVal = *(INT32*)pFieldAddress;
                break;
        
            case 8:
                *(INT64*)pOutVal = *(INT64*)pFieldAddress;
                break;
        
            default:
                CopyMemory(pOutVal, pFieldAddress, cbSize);
                break;
        }
    }
}

void    FieldDesc::SetInstanceField(LPVOID o, const VOID * pInVal)
{
    THROWSCOMPLUSEXCEPTION();

    // Check whether we are setting a field value on a proxy or a marshalbyref
    // class. If so, then ask remoting services to set the value on the 
    // instance

    if(((Object*)o)->IsThunking())
    {
        Object *puo = (Object *) CRemotingServices::AlwaysUnwrap((Object*) o);
        OBJECTREF unwrapped = ObjectToOBJECTREF(puo);

#ifdef PROFILING_SUPPORTED

        GCPROTECT_BEGIN(unwrapped);

        BOOL fIsRemoted = FALSE;

        // If profiling is active, notify it that remoting stuff is kicking in,
        // if AlwaysUnwrap returned an identical object pointer which means that
        // we are definitely going through remoting for this access.

        if (CORProfilerTrackRemoting())
        {
            fIsRemoted = ((LPVOID)puo == o);
            if (fIsRemoted)
            {
                g_profControlBlock.pProfInterface->RemotingClientInvocationStarted(
                    reinterpret_cast<ThreadID>(GetThread()));
            }
        }
#endif // PROFILING_SUPPORTED

        CRemotingServices::FieldAccessor(this, unwrapped, (void *)pInVal, FALSE);

#ifdef PROFILING_SUPPORTED
        if (CORProfilerTrackRemoting() && fIsRemoted)
            g_profControlBlock.pProfInterface->RemotingClientInvocationFinished(
                reinterpret_cast<ThreadID>(GetThread()));

		GCPROTECT_END();

#endif // PROFILING_SUPPORTED


    }
    else
    {
        _ASSERTE(GetEnclosingClass()->IsValueClass() || !((Object*) o)->IsThunking());

#ifdef _DEBUG
        //
        // assert that o is derived from MT of enclosing class
        //
        // walk up o's inheritence chain to make sure m_pMTOfEnclosingClass is along it
        //
        MethodTable* pCursor = ((Object *)o)->GetMethodTable();

        while (pCursor && (m_pMTOfEnclosingClass != pCursor))
        {
            pCursor = pCursor->GetParentMethodTable();
        }
        _ASSERTE(m_pMTOfEnclosingClass == pCursor);
#endif // _DEBUG

#if CHECK_APP_DOMAIN_LEAKS
        Object *oParmIn = (Object*)o;
#endif
        // Unbox the value class
        if(GetEnclosingClass()->IsValueClass())
        {
            o = ObjectToOBJECTREF((Object *)o)->UnBox();
        }
        LPVOID pFieldAddress = GetAddress(o);
    
    
        CorElementType fieldType = GetFieldType();
    
        if (fieldType == ELEMENT_TYPE_CLASS)
        {
            OBJECTREF ref = ObjectToOBJECTREF(*(Object**)pInVal);

            SetObjectReference((OBJECTREF*)pFieldAddress, ref, 
                               (oParmIn)->GetAppDomain());
        }
        else
        {
            UINT cbSize = GetSize();
    
            switch (cbSize)
            {
                case 1:
                    *(INT8*)pFieldAddress = *(INT8*)pInVal;
                    break;
    
                case 2:
                    *(INT16*)pFieldAddress = *(INT16*)pInVal;
                    break;
    
                case 4:
                    *(INT32*)pFieldAddress = *(INT32*)pInVal;
                    break;
    
                case 8:
                    *(INT64*)pFieldAddress = *(INT64*)pInVal;
                    break;
    
                default:
                    CopyMemory(pFieldAddress, pInVal, cbSize);
                    break;
            }
        }
    }
}

// This function is used for BYREF support of fields.  Since it generates
// interior pointers, you really have to watch the lifetime of the pointer
// so that GCs don't happen while you have the reference active
void *FieldDesc::GetAddress( void *o)
{
    if (GetEnclosingClass()->IsValueClass())
        return ((*((BYTE**) &o)) + GetOffset());
    else
        return ((*((BYTE**) &o)) + GetOffset() + sizeof(Object));
}

// And here's the equivalent, when you are guaranteed that the enclosing instance of
// the field is in the GC Heap.  So if the enclosing instance is a value type, it had
// better be boxed.  We ASSERT this.
void *FieldDesc::GetAddressGuaranteedInHeap(void *o, BOOL doValidate)
{
#ifdef _DEBUG
    Object *pObj = (Object *)o;
    if (doValidate)
        pObj->Validate();
#endif
    return ((*((BYTE**) &o)) + GetOffset() + sizeof(Object));
}


DWORD   FieldDesc::GetValue32(OBJECTREF o)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD val;
    GetInstanceField(o, (LPVOID)&val);
    return val;
}

VOID    FieldDesc::SetValue32(OBJECTREF o, DWORD dwValue)
{
    THROWSCOMPLUSEXCEPTION();

    SetInstanceField(o, (LPVOID)&dwValue);
}

void*   FieldDesc::GetValuePtr(OBJECTREF o)
{
    THROWSCOMPLUSEXCEPTION();

    void* val;
    GetInstanceField(o, (LPVOID)&val);
    return val;
}

VOID    FieldDesc::SetValuePtr(OBJECTREF o, void* pValue)
{
    THROWSCOMPLUSEXCEPTION();

    SetInstanceField(o, (LPVOID)&pValue);
}

OBJECTREF FieldDesc::GetRefValue(OBJECTREF o)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF val = NULL;

#ifdef PROFILING_SUPPORTED
    GCPROTECT_BEGIN(val);
#endif

    GetInstanceField(o, (LPVOID)&val);

#ifdef PROFILING_SUPPORTED
    GCPROTECT_END();
#endif

    return val;
}

VOID    FieldDesc::SetRefValue(OBJECTREF o, OBJECTREF orValue)
{
    THROWSCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(o);
    VALIDATEOBJECTREF(orValue);

    SetInstanceField(o, (LPVOID)&orValue);
}

USHORT  FieldDesc::GetValue16(OBJECTREF o)
{
    THROWSCOMPLUSEXCEPTION();

    USHORT val;
    GetInstanceField(o, (LPVOID)&val);
    return val;
}

VOID    FieldDesc::SetValue16(OBJECTREF o, DWORD dwValue)
{
    THROWSCOMPLUSEXCEPTION();

    USHORT val = (USHORT)dwValue;
    SetInstanceField(o, (LPVOID)&val);
}

BYTE    FieldDesc::GetValue8(OBJECTREF o)
{
    THROWSCOMPLUSEXCEPTION();

    BYTE val;
    GetInstanceField(o, (LPVOID)&val);
    return val;

}

VOID    FieldDesc::SetValue8(OBJECTREF o, DWORD dwValue)
{
    THROWSCOMPLUSEXCEPTION();

    BYTE val = (BYTE)dwValue;
    SetInstanceField(o, (LPVOID)&val);
}

__int64 FieldDesc::GetValue64(OBJECTREF o)
{
    THROWSCOMPLUSEXCEPTION();
    __int64 val;
    GetInstanceField(o, (LPVOID)&val);
    return val;

}

VOID    FieldDesc::SetValue64(OBJECTREF o, __int64 value)
{
    THROWSCOMPLUSEXCEPTION();
    SetInstanceField(o, (LPVOID)&value);
}


UINT FieldDesc::GetSize()
{
    CorElementType type = GetFieldType();
    UINT size = GetSizeForCorElementType(type);
    if (size == (UINT) -1)
    {
        _ASSERTE(GetFieldType() == ELEMENT_TYPE_VALUETYPE);
        size = GetTypeOfField()->GetNumInstanceFieldBytes();
    }

    return size;
}
