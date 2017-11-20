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
////////////////////////////////////////////////////////////////////////////////
// This module contains the implementation of the native methods for the
//  varargs class(es)..
//
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "comvariant.h"
#include "comvarargs.h"

static void InitCommon(VARARGS *data, VASigCookie** cookie);
static void AdvanceArgPtr(VARARGS *data);

////////////////////////////////////////////////////////////////////////////////
// ArgIterator constructor that initializes the state to support iteration
// of the args starting at the first optional argument.
////////////////////////////////////////////////////////////////////////////////
FCIMPL2(void, COMVarArgs::Init, VARARGS* _this, LPVOID cookie)
{
    HELPER_METHOD_FRAME_BEGIN_0();

    THROWSCOMPLUSEXCEPTION();

    VARARGS* data = _this;
    if (cookie == 0)
        COMPlusThrow(kArgumentException, L"InvalidOperation_HandleIsNotInitialized");

    VASigCookie* pCookie = *(VASigCookie**)(cookie); 

    if (pCookie->mdVASig == NULL)
    {
        data->SigPtr = NULL;
        data->ArgCookie = NULL;
        data->ArgPtr = (BYTE*)((VASigCookieEx*)pCookie)->m_pArgs;
    }
    else
    {
        // Use common code to pick the cookie apart and advance to the ...
        InitCommon(data, (VASigCookie**)cookie);
        AdvanceArgPtr(data);
    }

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////////
// ArgIterator constructor that initializes the state to support iteration
// of the args starting at the argument following the supplied argument pointer.
// Specifying NULL as the firstArg parameter causes it to start at the first
// argument to the call.
////////////////////////////////////////////////////////////////////////////////
FCIMPL3(void, COMVarArgs::Init2, VARARGS* _this, LPVOID cookie, LPVOID firstArg)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    THROWSCOMPLUSEXCEPTION();

    VARARGS* data = _this;
    if (cookie == 0)
        COMPlusThrow(kArgumentException, L"InvalidOperation_HandleIsNotInitialized");

    // Init most of the structure.
    InitCommon(data, (VASigCookie**)cookie);

    // If it is NULL, start at the first arg.
    if (firstArg != NULL)
    {
        // Advance to the specified arg.
        while (data->RemainingArgs > 0)
        {
            if (data->SigPtr.PeekElemType() == ELEMENT_TYPE_SENTINEL)
                COMPlusThrow(kArgumentException);

            // Adjust the frame pointer and the signature info.
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
            data->ArgPtr -= StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
#else  // STACK_GROWS_UP_ON_ARGS_WALK
            data->ArgPtr += StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
#endif // STACK_GROWS_**_ON_ARGS_WALK

            data->SigPtr.SkipExactlyOne();
            --data->RemainingArgs;

            // Stop when we get to where the user wants to be.
            if (data->ArgPtr == firstArg)
                break;
        }
    }
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////////
// Initialize the basic info for processing a varargs parameter list.
////////////////////////////////////////////////////////////////////////////////
static void InitCommon(VARARGS *data, VASigCookie** cookie)
{
    // Save the cookie and a copy of the signature.
    data->ArgCookie = *cookie;
    data->SigPtr.SetSig(data->ArgCookie->mdVASig);

    // Skip the calling convention, get the # of args and skip the return type.
    ULONG callConv;
    callConv = data->SigPtr.GetCallingConvInfo();
    data->RemainingArgs = data->SigPtr.GetData();
    CorElementType retType;
    retType = data->SigPtr.PeekElemTypeNormalized(data->ArgCookie->pModule);
    data->SigPtr.SkipExactlyOne();

    // Get a pointer to the cookie arg.
    data->ArgPtr = (BYTE *) cookie;

#if defined(_X86_)
    //  STACK_GROWS_DOWN_ON_ARGS_WALK

    //   <return address>  ;; lower memory                  
    //   <varargs_cookie>  \
    //   <argN>             \
    //                       += sizeOfArgs     
    //                      /
    //   <arg1>            /
    // * <this>            ;; if an instance method (note: <this> is usally passed in 
    //                     ;; a register and wouldn't appear on the stack frame)
    //                     ;; higher memory
    //
    // When the stack grows down, ArgPtr always points to the end of the next
    // argument passed. So we initialize it to the address that is the
    // end of the first fixed arg (arg1) (marked with a '*').

    data->ArgPtr += data->ArgCookie->sizeOfArgs;

    // When we calculated the address of the last argument above, we 
    // assumed that the cookie was the first argument on the stack. However
    // for current implementation of vararg calling convention in FJIT that
    // is not true because the "this pointer" and the "return buffer" are
    // in front of the cookie
    if ( callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS )  data->ArgPtr -= sizeof(void *); // this pointer
    if ( retType == ELEMENT_TYPE_VALUETYPE || retType == ELEMENT_TYPE_TYPEDBYREF ) data->ArgPtr -= sizeof(void *); 

#elif defined(_IA64_) || defined(_PPC_) || defined (_SPARC_)
    //  STACK_GROWS_UP_ON_ARGS_WALK

    //   <this>	           ;; lower memory
    //   <varargs_cookie>  ;; if an instance method
    // * <arg1>
    //                          
    //   <argN>		   ;; higher memory
    //
    // When the stack grows up, ArgPtr always points to the start of the next
    // argument passed. So we initialize it to the address marked with a '*',
    // which is the start of the first fixed arg (arg1).

    // Always skip over the varargs_cookie.
    data->ArgPtr += StackElemSize(sizeof(LPVOID));

#else
    PORTABILITY_ASSERT("Platform vararg cookie calling convention not defined!");
#endif

    COMVariant::EnsureVariantInitialized();
}

////////////////////////////////////////////////////////////////////////////////
// After initialization advance the next argument pointer to the first optional
// argument.
////////////////////////////////////////////////////////////////////////////////
void AdvanceArgPtr(VARARGS *data)
{
    // Advance to the first optional arg.
    while (data->RemainingArgs > 0)
    {
        if (data->SigPtr.PeekElemType() == ELEMENT_TYPE_SENTINEL)
        {
            data->SigPtr.SkipExactlyOne();
            break;
        }

        // Adjust the frame pointer and the signature info.
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
        data->ArgPtr -= StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
#else  // STACK_GROWS_UP_ON_ARGS_WALK
        data->ArgPtr += StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
#endif // STACK_GROWS_**_ON_ARGS_WALK

        data->SigPtr.SkipExactlyOne();
        --data->RemainingArgs;
    }
}




////////////////////////////////////////////////////////////////////////////////
// Return the number of unprocessed args in the argument iterator.
////////////////////////////////////////////////////////////////////////////////
FCIMPL1(int, COMVarArgs::GetRemainingCount, VARARGS* _this)
{
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    THROWSCOMPLUSEXCEPTION();

    if (!(_this->ArgCookie))
    {
        // this argiterator was created by marshaling from an unmanaged va_list -
        // can't do this operation
        COMPlusThrow(kNotSupportedException); 
    }
    HELPER_METHOD_FRAME_END();
    return (_this->RemainingArgs);
}
FCIMPLEND


////////////////////////////////////////////////////////////////////////////////
// Retrieve the type of the next argument without consuming it.
////////////////////////////////////////////////////////////////////////////////
FCIMPL1(void*, COMVarArgs::GetNextArgType, VARARGS* _this)
{
    TypedByRef  value;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    THROWSCOMPLUSEXCEPTION();

    VARARGS     data = *_this;

    if (!(_this->ArgCookie))
    {
        // this argiterator was created by marshaling from an unmanaged va_list -
        // can't do this operation
        COMPlusThrow(kNotSupportedException);
    }


    // Make sure there are remaining args.
    if (data.RemainingArgs == 0)
        COMPlusThrow(kInvalidOperationException, L"InvalidOperation_EnumEnded");

    GetNextArgHelper(&data, &value, FALSE);
    HELPER_METHOD_FRAME_END();
    return value.type.AsPtr();
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////////
// Retrieve the next argument and return it in a TypedByRef and advance the
// next argument pointer.
////////////////////////////////////////////////////////////////////////////////
FCIMPL0_INST_RET_TR(TypedByRef, value, COMVarArgs::GetNextArg, VARARGS* _this)
{
    HELPER_METHOD_FRAME_BEGIN_RET_VC_0();

    THROWSCOMPLUSEXCEPTION();

    if (!(_this->ArgCookie))
    {
        // this argiterator was created by marshaling from an unmanaged va_list -
        // can't do this operation
        COMPlusThrow(kInvalidOperationException);
    }

    // Make sure there are remaining args.
    if (_this->RemainingArgs == 0)
        COMPlusThrow(kInvalidOperationException, L"InvalidOperation_EnumEnded");

    GetNextArgHelper(_this, value, TRUE);
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND_RET_TR



////////////////////////////////////////////////////////////////////////////////
// Retrieve the next argument and return it in a TypedByRef and advance the
// next argument pointer.
////////////////////////////////////////////////////////////////////////////////
FCIMPL1_INST_RET_TR(TypedByRef, value, COMVarArgs::GetNextArg2, VARARGS* _this, TypeHandle typehandle)
{
    HELPER_METHOD_FRAME_BEGIN_RET_VC_0();

    THROWSCOMPLUSEXCEPTION(); 

    value->data = NULL;
    COMPlusThrow(kNotSupportedException);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND_RET_TR



////////////////////////////////////////////////////////////////////////////////
// This is a helper that uses a VARARGS tracking data structure to retrieve
// the next argument out of a varargs function call.  This does not check if
// there are any args remaining (it assumes it has been checked).
////////////////////////////////////////////////////////////////////////////////
void  COMVarArgs::GetNextArgHelper(VARARGS *data, TypedByRef *value, BOOL fData)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pThrowable = NULL;
    GCPROTECT_BEGIN(pThrowable);

    unsigned __int8 elemType;

    _ASSERTE(data->RemainingArgs != 0);

    if (data->SigPtr.PeekElemType() == ELEMENT_TYPE_SENTINEL)
        data->SigPtr.GetElemType();

    // Get a pointer to the beginning of the argument. 
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
    data->ArgPtr -= StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
#endif

    // Assume the ref pointer points directly at the arg on the stack.
    void* origArgPtr = data->ArgPtr;
    value->data = origArgPtr;

#ifndef STACK_GROWS_DOWN_ON_ARGS_WALK
    data->ArgPtr += StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
#endif // STACK_GROWS_**_ON_ARGS_WALK


TryAgain:
    switch (elemType = data->SigPtr.PeekElemType())
    {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
#ifdef BIGENDIAN
        if ( origArgPtr == value->data ) {
            value->data = (BYTE*)origArgPtr + (sizeof(void*)-1);
        }
#endif
        value->type = GetTypeHandleForCVType(elemType);
        break;

        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_CHAR:
#ifdef BIGENDIAN
        if ( origArgPtr == value->data ) {
            value->data = (BYTE*)origArgPtr + (sizeof(void*)-2);
        }
#endif
        value->type = GetTypeHandleForCVType(elemType);
        break;

        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_STRING:
        value->type = GetTypeHandleForCVType(elemType);
        break;

        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R8:
        value->type = GetTypeHandleForCVType(elemType);
#if !defined(_WIN64) && (DATA_ALIGNMENT > 4)
        if ( fData && origArgPtr == value->data ) {
            // allocate an aligned copy of the value
            value->data = value->type.GetMethodTable()->Box(origArgPtr, FALSE)->UnBox();
        }
#endif
        break;


        case ELEMENT_TYPE_I:
        value->type = ElementTypeToTypeHandle(ELEMENT_TYPE_I);
        break;

        case ELEMENT_TYPE_U:
        value->type = ElementTypeToTypeHandle(ELEMENT_TYPE_U);
        break;

            // Fix if R and R8 diverge
        case ELEMENT_TYPE_R:
        value->type = ElementTypeToTypeHandle(ELEMENT_TYPE_R8);
#if !defined(_WIN64) && (DATA_ALIGNMENT > 4)
        if ( fData && origArgPtr == value->data ) {
            // allocate an aligned copy of the value
            value->data = value->type.GetMethodTable()->Box(origArgPtr, FALSE)->UnBox();
        }
#endif
        break;

        case ELEMENT_TYPE_PTR:
        value->type = data->SigPtr.GetTypeHandle(data->ArgCookie->pModule, &pThrowable);
        if (value->type.IsNull()) {
            _ASSERTE(pThrowable != NULL);
            COMPlusThrow(pThrowable);
        }
        break;

        case ELEMENT_TYPE_BYREF:
        // Check if we have already processed a by-ref.
        if (value->data != origArgPtr)
        {
            _ASSERTE(!"Can't have a ByRef of a ByRef");
            COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
        }

        // Dereference the argument to remove the indirection of the ByRef.
        value->data = *((void **) value->data);

        // Consume and discard the element type.
        data->SigPtr.GetElemType();
        goto TryAgain;

        case ELEMENT_TYPE_VALUETYPE:
        // fall through
        case ELEMENT_TYPE_CLASS: {
        value->type = data->SigPtr.GetTypeHandle(data->ArgCookie->pModule, &pThrowable);
        if (value->type.IsNull()) {
            _ASSERTE(pThrowable != NULL);
            COMPlusThrow(pThrowable);
        }

#if defined(BIGENDIAN) && !defined(_SPARC_) // valbyref
       // Adjust the pointer for small valuetypes
       if (elemType == ELEMENT_TYPE_VALUETYPE && origArgPtr == value->data) {
            switch (value->type.GetSize()) {
            case 1:
                value->data = (BYTE*)origArgPtr + sizeof(void*)-1;
                break;
            case 2:
                value->data = (BYTE*)origArgPtr + sizeof(void*)-2;
                break;
            default:
                // nothing to do
                break;
            }
       }
#endif
        if (elemType == ELEMENT_TYPE_CLASS && value->type.GetClass()->IsValueClass())
            value->type = g_pObjectClass;
        } break;

        case ELEMENT_TYPE_TYPEDBYREF:
        if (value->data != origArgPtr)
        {
            _ASSERTE(!"Can't have a ByRef of a TypedByRef");
            COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
        }
        // Load the TypedByRef
        value->type = ((TypedByRef*)value->data)->type;
        value->data = ((TypedByRef*)value->data)->data;
        break;

        default:
        case ELEMENT_TYPE_SENTINEL:
            _ASSERTE(!"Unrecognized element type");
            COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
        break;

        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_VALUEARRAY:
        {
            value->type = data->SigPtr.GetTypeHandle(data->ArgCookie->pModule, &pThrowable);
            if (value->type.IsNull()) {
                _ASSERTE(pThrowable != NULL);
                COMPlusThrow(pThrowable);
            }

            break;
        }

        case ELEMENT_TYPE_FNPTR:
        case ELEMENT_TYPE_OBJECT:
        _ASSERTE(!"Not implemented");
        COMPlusThrow(kNotSupportedException);
        break;
    }

    // Update the tracking stuff to move past the argument.
    --data->RemainingArgs;
    data->SigPtr.SkipExactlyOne();

    GCPROTECT_END();
}


/*static*/ void COMVarArgs::MarshalToManagedVaList(va_list va, VARARGS *dataout)
{
    dataout->SigPtr = NULL;
    dataout->ArgCookie = NULL;
    dataout->ArgPtr = (BYTE*)va;
}

////////////////////////////////////////////////////////////////////////////////
// Creates an unmanaged va_list equivalent. (WARNING: Allocated from the
// LIFO memory manager so this va_list is only good while that memory is in "scope".) 
////////////////////////////////////////////////////////////////////////////////
/*static*/ va_list COMVarArgs::MarshalToUnmanagedVaList(const VARARGS *data)
{
    THROWSCOMPLUSEXCEPTION();


    // Must make temporary copy so we don't alter the original
    SigPointer sp = data->SigPtr;

    // Calculate how much space we need for the marshaled stack. This actually overestimates
    // the value since it counts the fixed args as well as the varargs. But that's harmless.
    DWORD      cbAlloc = MetaSig::SizeOfActualFixedArgStack(data->ArgCookie->pModule , data->ArgCookie->mdVASig, FALSE);

    BYTE*      pdstbuffer = (BYTE*)(GetThread()->m_MarshalAlloc.Alloc(cbAlloc));

    int        remainingArgs = data->RemainingArgs;
    BYTE*      psrc = (BYTE*)(data->ArgPtr);

    if (sp.PeekElemType() == ELEMENT_TYPE_SENTINEL) 
    {
        sp.GetElemType();
    }

    BYTE*      pdst = pdstbuffer;
    while (remainingArgs--) 
    {
        CorElementType elemType = sp.PeekElemType();
        switch (elemType)
        {
            case ELEMENT_TYPE_I1:
            case ELEMENT_TYPE_U1:
            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_I4:
            case ELEMENT_TYPE_U4:
            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
            case ELEMENT_TYPE_R4:
            case ELEMENT_TYPE_R8:
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_R:
            case ELEMENT_TYPE_PTR:
                {
                    DWORD cbSize = StackElemSize(sp.SizeOf(data->ArgCookie->pModule));
                    
                    #ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                    psrc -= cbSize;
                    #endif // STACK_GROWS_DOWN_ON_ARGS_WALK
                    
                    CopyMemory(pdst, psrc, cbSize);
                    
                    #ifdef STACK_GROWS_UP_ON_ARGS_WALK
                    psrc += cbSize;
                    #endif // STACK_GROWS_UP_ON_ARGS_WALK

                    pdst += cbSize;
                    sp.SkipExactlyOne();
                }
                break;

            default:
                // non-IJW data type - we don't support marshaling these inside a va_list.
                COMPlusThrow(kNotSupportedException);


        }
    }

    return (va_list)pdstbuffer;
}

