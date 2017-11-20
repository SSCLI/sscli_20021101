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
** File:    message.cpp
**        
** Purpose: Encapsulates a function call frame into a message 
**          object with an interface that can enumerate the 
**          arguments of the message
**
** Date:    Mar 5, 1999
**
===========================================================*/
#include "common.h"
#include "comstring.h"
#include "comreflectioncommon.h"
#include "comdelegate.h"
#include "comclass.h"
#include "excep.h"
#include "message.h"
#include "reflectwrap.h"
#include "remoting.h"
#include "field.h"
#include "eeconfig.h"
#include "invokeutil.h"

#define MESSAGEREFToMessage(m) ((MessageObject *) OBJECTREFToObject(m))

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetArgCount public
//
//  Synopsis:   Returns number of arguments in the method call
//+----------------------------------------------------------------------------
FCIMPL1(INT32, CMessage::GetArgCount, MessageObject * pMessage)
{
    LOG((LF_REMOTING, LL_INFO10, "CMessage::GetArgCount IN pMsg:0x%x\n", pMessage));

    // Get the frame pointer from the object

    MetaSig *pSig = GetMetaSig(pMessage);

    // scan the sig for the argument count
    INT32 ret = pSig->NumFixedArgs();

    if (pMessage->pDelegateMD)
    {
        ret -= 2;
    }

    LOG((LF_REMOTING, LL_INFO10, "CMessage::GetArgCount OUT ret:0x%x\n", ret));
    return ret;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetArg public
//
//  Synopsis:   Use to enumerate a call's arguments
//+----------------------------------------------------------------------------
FCIMPL2(Object*, CMessage::GetArg, MessageObject* pMessageUNSAFE, INT32 argNum)
{
    OBJECTREF refRetVal = NULL;
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pMessage);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArgCount IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    if ((UINT)argNum >= pSig->NumFixedArgs())
    {
        COMPlusThrow(kTargetParameterCountException);
    }
    pSig->Reset();
    for (INT32 i = 0; i < (argNum); i++)
    {
        pSig->NextArg();
    }

    BOOL fIsByRef = FALSE;
    CorElementType eType = pSig->NextArg();
    EEClass *      vtClass = NULL;
    if (eType == ELEMENT_TYPE_BYREF)
    {
        fIsByRef = TRUE;
        EEClass *pClass;
        eType = pSig->GetByRefType(&pClass);
        if (eType == ELEMENT_TYPE_VALUETYPE)
        {
            vtClass = pClass;
        }
    }
    else
    {
        if (eType == ELEMENT_TYPE_VALUETYPE)
        {
            vtClass = pSig->GetTypeHandle().GetClass();
        }
    }

    if (eType == ELEMENT_TYPE_PTR) 
    {
    COMPlusThrow(kRemotingException, L"Remoting_CantRemotePointerType");
    }
    pMsg->iLast = argNum;

    

    GetObjectFromStack(&refRetVal,
               GetStackPtr(argNum, pMsg->pFrame, pSig), 
               eType, 
               vtClass,
               fIsByRef);

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArg OUT\n"));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


#define RefreshMsg()   (MESSAGEREFToMessage(pMessage))

FCIMPL1(Object*, CMessage::GetArgs, MessageObject* pMessageUNSAFE)
{
    PTRARRAYREF refRetVal = NULL;
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pMessage);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArgCount IN\n"));

    MessageObject *pMsg = RefreshMsg();

    MetaSig *pSig = GetMetaSig(pMsg);
    // scan the sig for the argument count
    INT32 numArgs = pSig->NumFixedArgs();
    if (RefreshMsg()->pDelegateMD)
        numArgs -= 2;

    // Allocate an object array
    refRetVal = (PTRARRAYREF) AllocateObjectArray(
        numArgs, g_pObjectClass);

    GCPROTECT_BEGIN(refRetVal);

    pSig->Reset();
    ArgIterator iter(RefreshMsg()->pFrame, pSig);

    for (int index = 0; index < numArgs; index++)
    {

        BOOL fIsByRef = FALSE;
        CorElementType eType;
        BYTE type;
        UINT32 size;
        PVOID addr;
        eType = pSig->PeekArg();
        addr = (LPBYTE) RefreshMsg()->pFrame + iter.GetNextOffset(&type, &size);

        EEClass *      vtClass = NULL;
        if (eType == ELEMENT_TYPE_BYREF)
        {
            fIsByRef = TRUE;
            EEClass *pClass;
            // If this is a by-ref arg, GetObjectFromStack() will dereference "addr" to
            // get the real argument address. Dereferencing now will open a gc hole if "addr" 
            // points into the gc heap, and we trigger gc between here and the point where 
            // we return the arguments. 
            //addr = *((PVOID *) addr);
            eType = pSig->GetByRefType(&pClass);
            if (eType == ELEMENT_TYPE_VALUETYPE)
            {
                vtClass = pClass;
            }
        }
        else
        {
            if (eType == ELEMENT_TYPE_VALUETYPE)
            {
                vtClass = pSig->GetTypeHandle().GetClass();
            }
        }

        if (eType == ELEMENT_TYPE_PTR) 
        {
            COMPlusThrow(kRemotingException, L"Remoting_CantRemotePointerType");
        }

    
        OBJECTREF arg = NULL;

        GetObjectFromStack(&arg,
                   addr, 
                   eType, 
                   vtClass,
                   fIsByRef);

        refRetVal->SetAt(index, arg);
    }

    GCPROTECT_END();

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArgs OUT\n"));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

void GetObjectFromStack(OBJECTREF* ppDest, PVOID val, const CorElementType eType, EEClass *pCls, BOOL fIsByRef)
{
    THROWSCOMPLUSEXCEPTION();

    TRIGGERSGC();

    // WARNING *ppDest is not protected!
    switch (CorTypeInfo::GetGCType(eType))
    {
        case TYPE_GC_NONE:
        {
            if(ELEMENT_TYPE_PTR == eType)
            {
                COMPlusThrow(kNotSupportedException);
            }
            else
            {
                MethodTable *pMT = TypeHandle(g_Mscorlib.FetchElementType(eType)).AsMethodTable();
                OBJECTREF pObj = FastAllocateObject(pMT);
                if (fIsByRef)
                    val = *((PVOID *)val);
                memcpyNoGCRefs(pObj->UnBox(),val, CorTypeInfo::Size(eType));
                *ppDest = pObj;
            }
        }
        break;
        case TYPE_GC_OTHER:
        {
            if (eType == ELEMENT_TYPE_VALUETYPE) 
            {
                //
                // box the value class
                //

                _ASSERTE(CanBoxToObject(pCls->GetMethodTable()));

                _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) ppDest) ||
                     !"(pDest) can not point to GC Heap");
                OBJECTREF pObj = FastAllocateObject(pCls->GetMethodTable());
                if (fIsByRef)
                    val = *((PVOID *)val);
                CopyValueClass(pObj->UnBox(), val, pObj->GetMethodTable(), pObj->GetAppDomain());

                *ppDest = pObj;
            }
            else
            {
                _ASSERTE(!"unsupported COR element type passed to remote call");
            }
        }
        break;
        case TYPE_GC_REF:
            if (fIsByRef)
                val = *((PVOID *)val);
            *ppDest = ObjectToOBJECTREF(*(Object **)val);
            break;
        default:
            _ASSERTE(!"unsupported COR element type passed to remote call");
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::PropagateOutParameters private
//
//  Synopsis:   Copy back data for in/out parameters and the return value
//+----------------------------------------------------------------------------
FCIMPL3(void, CMessage::PropagateOutParameters, MessageObject* pMessageUNSAFE, ArrayBase* pOutPrmsUNSAFE, Object* RetValUNSAFE)
{
    struct _gc
    {
        MESSAGEREF pMessage;
        BASEARRAYREF pOutPrms;
        OBJECTREF RetVal;
    } gc;
    gc.pMessage = (MESSAGEREF) pMessageUNSAFE;
    gc.pOutPrms = (BASEARRAYREF) pOutPrmsUNSAFE;
    gc.RetVal = (OBJECTREF) RetValUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::PropogateOutParameters IN\n"));

    BEGINFORBIDGC();

    MessageObject *pMsg = MESSAGEREFToMessage(gc.pMessage);

    // Copy the metasig so that it does not move due to GC
    MetaSig *pSig = (MetaSig *)_alloca(sizeof(MetaSig)); 
    memcpy(pSig, GetMetaSig(pMsg), sizeof(MetaSig));

    _ASSERTE(pSig != NULL);
    
    ArgIterator argit(pMsg->pFrame, pSig);

    ENDFORBIDGC();

    //**************************WARNING*********************************
    // We should handle GC from now on
    //******************************************************************

    // move into object to return to client

    // Propagate the return value only if the pMsg is not a Ctor message
    // Check if the return type has a return buffer associated with it
    if ( (pMsg->iFlags & MSGFLG_CTOR) == 0  &&  
        pSig->GetReturnType() != ELEMENT_TYPE_VOID)  
    {
        if (pSig->HasRetBuffArg())
        {
            // Copy from RetVal into the retBuff.
            INT64 retVal =  CopyOBJECTREFToStack(
                                *((void**) argit.GetRetBuffArgAddr()), 
                                gc.RetVal, 
                                pSig->GetReturnType(),
                                NULL,
                                pSig,
                                TRUE);  // copy class contents

            // Refetch variables as GC could have happened after call to CopyOBJECTREFToStack
            pMsg = MESSAGEREFToMessage(gc.pMessage);
            // Copy the return value
            *(pMsg->pFrame->GetReturnValuePtr()) = retVal;            
        }
        else
        {
            // There is no separate return buffer, the retVal should fit in 
            // an INT64. 
            INT64 retVal = CopyOBJECTREFToStack(
                                NULL,                   //no return buff
                                gc.RetVal, 
                                pSig->GetReturnType(),
                                NULL,
                                pSig,
                                FALSE);                 //don't copy class contents

            // Refetch variables as GC could have happened after call to CopyOBJECTREFToStack
            pMsg = MESSAGEREFToMessage(gc.pMessage);
            // Copy the return value
            *(pMsg->pFrame->GetReturnValuePtr()) = retVal;            
        }
    }


    MetaSig *pSyncSig = NULL;
    if (pMsg->iFlags & MSGFLG_ENDINVOKE)
    {
        PCCOR_SIGNATURE pMethodSig;
        DWORD cSig;

        pMsg->pMethodDesc->GetSig(&pMethodSig, &cSig);
        _ASSERTE(pSig);

        LPVOID temp = _alloca(sizeof(MetaSig));
        pSyncSig = new (temp) MetaSig(pMethodSig, pMsg->pDelegateMD->GetModule());
    }
    else
    {
        pSyncSig = NULL;
    }

    // Refetch all the variables as GC could have happened after call to
    // CopyOBJECTREFToStack        
    pMsg = MESSAGEREFToMessage(gc.pMessage);
    OBJECTREF *pOutParams = (gc.pOutPrms != NULL) ? (OBJECTREF *) gc.pOutPrms->GetDataPtr() : NULL;
    UINT32  cOutParams = (gc.pOutPrms != NULL) ? gc.pOutPrms->GetNumComponents() : 0;
    if (cOutParams > 0)
    {
        BYTE typ;
        UINT32 structSize;
        PVOID *argAddr;
        UINT32 i = 0;
        for (i=0; i<cOutParams; i++)
        {
            if (pSyncSig)
            {
                typ = pSyncSig->NextArg();
                if (typ == ELEMENT_TYPE_BYREF)
                {
                    argAddr = (PVOID *)argit.GetNextArgAddr(&typ, &structSize);
                }
                else if (typ == 0)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                argAddr = (PVOID *)argit.GetNextArgAddr(&typ, &structSize);
                if (argAddr == NULL)
                {
                    break;
                }
                else if (typ != ELEMENT_TYPE_BYREF)
                {
                    continue;
                }
            }

            EEClass *pClass = NULL;
            CorElementType brType = pSig->GetByRefType(&pClass);

            CopyOBJECTREFToStack(
                *argAddr, 
                pOutParams[i],
                brType, 
                pClass, 
                pSig,
                pClass ? pClass->IsValueClass() : FALSE);

            // Refetch all the variables because GC could happen at the
            // end of every loop after the call to CopyOBJECTREFToStack                
            pOutParams = (OBJECTREF *) gc.pOutPrms->GetDataPtr();                
        }
    }

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

INT64 CMessage::CopyOBJECTREFToStack( 
    PVOID pvDest, OBJECTREF pSrc, CorElementType typ, EEClass *pClass, 
    MetaSig *pSig, BOOL fCopyClassContents)
{
    THROWSCOMPLUSEXCEPTION();

    INT64 ret = 0;
                             
    if (fCopyClassContents)
    {
        // We have to copy the contents of a value class to pvDest

        // write unboxed version back to memory provided by the client
        if (pvDest)
        {
            OBJECTREF obj = pSrc;
            if (obj == NULL)
            {
                COMPlusThrow(kRemotingException, L"Remoting_Message_BadRetValOrOutArg");
            }
            CopyValueClassUnchecked(pvDest, obj->UnBox(), obj->GetMethodTable());
            // return the object so it can be stored in the frame and 
            // propagated to the root set
            ret  = *((INT64*) &obj);
        }
    }
    else
    {
        // We have either a real OBJECTREF or something that does not have
        // a return buffer associated 

        // Check if it is an ObjectRef (from the GC heap)
        if (CorTypeInfo::IsObjRef(typ))
        {
            OBJECTREF obj = pSrc;
            OBJECTREF savedOr = obj;

            if ((obj!=NULL) && (obj->GetMethodTable()->IsTransparentProxyType()))
            {
                GCPROTECT_BEGIN(obj);
                if (!pClass)
                    pClass = pSig->GetRetEEClass();
                // CheckCast ensures that the returned object (proxy) gets
                // refined to the level expected by the caller of the method
                if (!CRemotingServices::CheckCast(obj, pClass))
                {
                    COMPlusThrow(kInvalidCastException, L"Arg_ObjObj");
                }
                savedOr = obj;
                GCPROTECT_END();
            }
            if (pvDest)
            {
                SetObjectReferenceUnchecked((OBJECTREF *)pvDest, savedOr);
            }
            ret = *((INT64*) &savedOr);
        }
        else
        {
            // Note: this assert includes VALUETYPE because for Enums 
            // HasRetBuffArg() returns false since the normalized type is I4
            // so we end up here ... but GetReturnType() returns VALUETYPE
            // Almost all VALUETYPEs will go through the fCopyClassContents
            // codepath instead of here.
            // Also, IsPrimitiveType() does not check for IntPtr, UIntPtr etc
            // there is a note in siginfo.hpp about that ... hence we have 
            // ELEMENT_TYPE_I, ELEMENT_TYPE_U.
            _ASSERTE(
                CorTypeInfo::IsPrimitiveType(typ) 
                || (typ == ELEMENT_TYPE_VALUETYPE)
                || (typ == ELEMENT_TYPE_I)
                || (typ == ELEMENT_TYPE_U)
                || (typ == ELEMENT_TYPE_FNPTR)
                );

            // REVIEW: For a "ref int" arg, if a nasty sink replaces the boxed
            // int with a null OBJECTREF, this is where we check. We need to be
            // uniform in our policy w.r.t. this (throw v/s ignore)
            // The 'if' block above throws, CallFieldAccessor also has this 
            // problem.
            if (pSrc != NULL)
            {
                if (pvDest)
                {
                    memcpyNoGCRefs(
                        pvDest, 
                        pSrc->GetData(), 
                        gElementTypeInfo[typ].m_cbSize);
                }
                ret = *((INT64*) pSrc->GetData());
            }
        }
    }
    return ret;
}
//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetReturnValue
//
//  Synopsis:   Pull return value off the stack
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CMessage::GetReturnValue, MessageObject* pMessageUNSAFE)
{
    OBJECTREF refRetVal = NULL;
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pMessage);
    //-[autocvtpro]-------------------------------------------------------

    MessageObject *pMsg = MESSAGEREFToMessage(pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    PVOID pvRet;
    if (pSig->HasRetBuffArg())
    {
        ArgIterator argit(pMsg->pFrame,pSig);
        pvRet = argit.GetRetBuffArgAddr();
    }
    else
    {
        pvRet = pMsg->pFrame->GetReturnValuePtr();
    }
    
    CorElementType eType = pSig->GetReturnType();
    EEClass *vtClass; 
    if (eType == ELEMENT_TYPE_VALUETYPE)
    {
        vtClass = pSig->GetRetEEClass();
    }
    else
    {
        vtClass = NULL;
    }
 
    GetObjectFromStack(&refRetVal,
               pvRet,
               eType, 
               vtClass);
               
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL3(BOOL, CMessage::Dispatch, MessageObject* pMessageUNSAFE, Object* pServerUNSAFE, BYTE fContext)
{
    BOOL retVal;
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    OBJECTREF pServer = (OBJECTREF) pServerUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(pMessage, pServer);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    MessageObject *pMsg = MESSAGEREFToMessage(pMessage);
    MetaSig *pSig = GetMetaSig(pMsg);
    
    if (/*!gfBltStack ||*/ (pMsg->iFlags & (MSGFLG_BEGININVOKE | MSGFLG_ENDINVOKE | MSGFLG_ONEWAY)))
    {
        retVal = FALSE;
        goto lExit;
    }

    GCPROTECT_BEGIN(pMsg);

    pSig = GetMetaSig(pMsg);
    BOOL fIsStatic = !pSig->HasThis();
    UINT nActualStackBytes = pSig->SizeOfActualFixedArgStack(fIsStatic);
    MethodDesc *pMD = pMsg->pMethodDesc;

    // Get the address of the code
    const BYTE *pTarget = MethodTable::GetTargetFromMethodDescAndServer(pMD, &(pServer), fContext);

#if defined(_PPC_) || defined(_SPARC_) // retbuf
    if (pSig->HasRetBuffArg()) {
        // the CallDescrWorker callsite for methods with return buffer is 
        //  different for RISC CPUs - we pass this information along by setting 
        //  the lowest bit in pTarget
        pTarget = (const BYTE *)((UINT_PTR)pTarget | 0x1);
    }
#endif

#ifdef PROFILING_SUPPORTED
    // If we're profiling, notify the profiler that we're about to invoke the remoting target
    if (CORProfilerTrackRemoting())
        g_profControlBlock.pProfInterface->RemotingServerInvocationStarted(
            reinterpret_cast<ThreadID>(GetThread()));
#endif // PROFILING_SUPPORTED
    
    // set retval

    INT64 retval = 0;
    INSTALL_COMPLUS_EXCEPTION_HANDLER();




#ifdef _PPC_
    FramedMethodFrame::Enregister((BYTE *) pMsg->pFrame, pSig, fIsStatic, nActualStackBytes);
#endif

    retval = CallDescrWorker((BYTE*)pMsg->pFrame + sizeof(FramedMethodFrame) + nActualStackBytes,
                             nActualStackBytes / STACK_ELEM_SIZE,
#if defined(_X86_) || defined(_PPC_) // argregs
                             (ArgumentRegisters*)(((BYTE *)pMsg->pFrame) + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
#endif
                             (LPVOID)pTarget);

    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();

#ifdef PROFILING_SUPPORTED
    // If we're profiling, notify the profiler that we're about to invoke the remoting target
    if (CORProfilerTrackRemoting())
        g_profControlBlock.pProfInterface->RemotingServerInvocationReturned(
            reinterpret_cast<ThreadID>(GetThread()));
#endif // PROFILING_SUPPORTED
    
    pMsg = MESSAGEREFToMessage(pMessage);
    pSig = GetMetaSig(pMsg);    
    getFPReturn(pSig->GetFPReturnSize(), &retval);
    
    if (pSig->GetReturnType() != ELEMENT_TYPE_VOID)
    {
        *((INT64 *) (MESSAGEREFToMessage(pMessage))->pFrame->GetReturnValuePtr()) = retval;
    }    

    GCPROTECT_END();
    
    retVal = TRUE;

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


FCIMPL1(Object*, CMessage::GetMethodBase, MessageObject* pMessageUNSAFE)
{
    REFLECTBASEREF refRetVal = NULL;
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pMessage);
    //-[autocvtpro
    
    // no need to GCPROTECT - gc is not happening
    MessageObject *pMsg = MESSAGEREFToMessage(pMessage);

    refRetVal = GetExposedObjectFromMethodDesc(pMsg->pMethodDesc);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

HRESULT AppendAssemblyName(CQuickBytes *out, const CHAR* str) 
{
	SIZE_T len = strlen(str) * sizeof(CHAR); 
	SIZE_T oldSize = out->Size();
	if (FAILED(out->ReSize(oldSize + len + 2)))
        return E_OUTOFMEMORY;
	CHAR * cur = (CHAR *) ((BYTE *) out->Ptr() + oldSize - 1);
    if (*cur)
        cur++;
    *cur = ASSEMBLY_SEPARATOR_CHAR;
	memcpy(cur + 1, str, len);	
    cur += (len + 1);
    *cur = '\0';
    return S_OK;
} 

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetMethodName public
//
//  Synopsis:   return the method name
//+----------------------------------------------------------------------------
FCIMPL2(Object*, CMessage::GetMethodName, ReflectBaseObject* pMethodBaseUNSAFE, STRINGREF* pTypeNAssemblyName)
{
    STRINGREF refRetVal = NULL;
    REFLECTBASEREF pMethodBase = (REFLECTBASEREF) pMethodBaseUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pMethodBase);
    //-[autocvtpro]-------------------------------------------------------

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetMethodName IN\n"));

    ReflectMethod *pRM = (ReflectMethod*) pMethodBase->GetData();
    //
    // FUTURE:: work around for formatter problem
    //
    LPCUTF8 mName = pRM->pMethod->GetName();
    STRINGREF strMethod;
    if (strcmp(mName, "<init>") == 0)
    {
        strMethod = COMString::NewString("ctor");
    }
    else
    {
        strMethod = COMString::NewString(mName);
    }

    // Now get typeNassembly name
    LPCUTF8 szAssembly = NULL;
    CQuickBytes     qb;
    GCPROTECT_BEGIN(strMethod);

    //Get class
    EEClass *pClass = pRM->pMethod->GetClass();
    //Get type
    REFLECTCLASSBASEREF objType = (REFLECTCLASSBASEREF)pClass->GetExposedClassObject();
    //Get ReflectClass
    ReflectClass *pRC = (ReflectClass *)objType->GetData();
    COMClass::GetNameInternal(pRC, COMClass::TYPE_NAME | COMClass::TYPE_NAMESPACE , &qb);

    Assembly* pAssembly = pClass->GetAssembly();
    pAssembly->GetName(&szAssembly);
    AppendAssemblyName(&qb, szAssembly);

    SetObjectReference((OBJECTREF *)pTypeNAssemblyName, COMString::NewString((LPCUTF8)qb.Ptr()), GetAppDomain());

    GCPROTECT_END();

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetMethodName OUT\n"));

    refRetVal = strMethod;

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL0(UINT32, CMessage::GetMetaSigLen)
    DWORD dwSize = sizeof(MetaSig);
    return dwSize;
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::Init
//
//  Synopsis:   Initialize internal state
//+----------------------------------------------------------------------------
FCIMPL1(void, CMessage::Init, MessageObject* pMessageUNSAFE)
{
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pMessage);
    //-[autocvtpro]-------------------------------------------------------

    // This is called from the managed world and assumed to be
    // idempotent!
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::Init IN\n"));

    BEGINFORBIDGC();

    GetMetaSig(MESSAGEREFToMessage(pMessage));

    ENDFORBIDGC();

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::Init OUT\n"));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

MetaSig * __stdcall CMessage::GetMetaSig(MessageObject* pMsg)
{
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetMetaSig IN\n"));
    MetaSig* pEmbeddedMetaSig = (MetaSig*)(pMsg->pMetaSigHolder);

    _ASSERTE(pEmbeddedMetaSig);
    return pEmbeddedMetaSig;

}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetAsyncBeginInfo
//
//  Synopsis:   Pull the AsyncBeginInfo object from an async call
//+----------------------------------------------------------------------------
FCIMPL3(void, CMessage::GetAsyncBeginInfo, MessageObject* pMessageUNSAFE, OBJECTREF* ppACBD, OBJECTREF* ppState)
{
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pMessage);
    //-[autocvtpro]-------------------------------------------------------

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetAsyncBeginInfo IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    _ASSERTE(pMsg->iFlags & MSGFLG_BEGININVOKE);

    ArgIterator argit(pMsg->pFrame, pSig);

    if ((ppACBD != NULL) || (ppState != NULL))
    {
        BYTE typ;
        UINT32 size;
        LPVOID addr;
        LPVOID last = NULL, secondtolast = NULL;
        while ((addr = argit.GetNextArgAddr(&typ, &size)) != NULL)
        {
            secondtolast = last;
            last = addr;
        }
        if (ppACBD != NULL) SetObjectReferenceUnchecked(ppACBD, ObjectToOBJECTREF(*(Object **) secondtolast));
        if (ppState != NULL) SetObjectReferenceUnchecked(ppState, ObjectToOBJECTREF(*(Object **) last));
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetAsyncResult
//
//  Synopsis:   Pull the AsyncResult from an async call
//+----------------------------------------------------------------------------
FCIMPL1(LPVOID, CMessage::GetAsyncResult, MessageObject* pMessageUNSAFE)
{
    LPVOID retVal = NULL;
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(pMessage);
    //-[autocvtpro]-------------------------------------------------------

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetAsyncResult IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pMessage);
    _ASSERTE(pMsg->iFlags & MSGFLG_ENDINVOKE);
    retVal = GetLastArgument(pMsg);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetAsyncObject
//
//  Synopsis:   Pull the AsyncObject from an async call
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CMessage::GetAsyncObject, MessageObject* pMessageUNSAFE)
{
    Object* pobjRetVal = NULL;
    MESSAGEREF pMessage = (MESSAGEREF) pMessageUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pMessage);
    //-[autocvtpro]-------------------------------------------------------

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetAsyncObject IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    ArgIterator argit(pMsg->pFrame, pSig);

    pobjRetVal = *((Object**)argit.GetThisAddr());

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return pobjRetVal;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetLastArgument private
//
//  Synopsis:   Pull the last argument of 4 bytes off the stack
//+----------------------------------------------------------------------------
LPVOID CMessage::GetLastArgument(MessageObject *pMsg)
{
    BEGINFORBIDGC();

    ArgIterator argit(pMsg->pFrame, GetMetaSig(pMsg));
    BYTE typ;
    UINT32 size;
    LPVOID addr;
    LPVOID backadder = NULL;
    while ((addr = argit.GetNextArgAddr(&typ, &size)) != NULL)
    {
        backadder = addr;
    }

    ENDFORBIDGC();

    return *((LPVOID *) backadder);
}

REFLECTBASEREF __stdcall CMessage::GetExposedObjectFromMethodDesc(MethodDesc *pMD)
{
    ReflectMethod* pRM;
    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) 
                            pMD->GetClass()->GetExposedClassObject();
    REFLECTBASEREF retVal = NULL;
    GCPROTECT_BEGIN(pRefClass);

    //NOTE: REFLECTION objects are alloced on a non-GC heap. So we don't GCProtect 
    //pRefxxx here.
    if (pMD->IsCtor())
    {
        pRM= ((ReflectClass*) pRefClass->GetData())->FindReflectConstructor(pMD);                  
        retVal = pRM->GetConstructorInfo((ReflectClass*) pRefClass->GetData());
    }
    else
    {
        pRM= ((ReflectClass*) pRefClass->GetData())->FindReflectMethod(pMD);
        retVal = pRM->GetMethodInfo((ReflectClass*) pRefClass->GetData());
    }
    GCPROTECT_END();
    return retVal;
}
//+----------------------------------------------------------------------------
//
//  Method:     CMessage::DebugOut public
//
//  Synopsis:   temp Debug out until the classlibs have one.
//+----------------------------------------------------------------------------
FCIMPL1(void, CMessage::DebugOut, StringObject* pOutUNSAFE)
{
#ifdef _DEBUG
    STRINGREF pOut = (STRINGREF) pOutUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pOut);
    //-[autocvtpro]-------------------------------------------------------
    static int fMessageDebugOut = 0;

    if (fMessageDebugOut == 0)
    {
        fMessageDebugOut = g_pConfig->GetConfigDWORD(L"MessageDebugOut", 0) ? 1 : -1;
    }

    if (fMessageDebugOut == 1)
    {
        WszOutputDebugString(pOut->GetBuffer());
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
#endif
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::HasVarArgs public
//
//  Synopsis:   Return TRUE if the method is a VarArgs Method
//+----------------------------------------------------------------------------
FCIMPL1(BOOL, CMessage::HasVarArgs, MessageObject * pMessage)
{
    if (pMessage->pMethodDesc->IsVarArg()) 
        return TRUE;
    else
        return FALSE;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetVarArgsPtr public
//
//  Synopsis:   Get internal pointer to the VarArgs array
//+----------------------------------------------------------------------------
FCIMPL1(PVOID, CMessage::GetVarArgsPtr, MessageObject * pMessage)
{
    return (PVOID) ((pMessage->pFrame) + 1);
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetStackPtr private
//
//  Synopsis:   Figure out where on the stack a parameter is stored
//
//  Parameters: ndx     - the parameter index (zero-based)
//              pFrame  - stack frame pointer (FramedMethodFrame)
//              pSig    - method signature, used to determine parameter sizes
//
//+----------------------------------------------------------------------------
PVOID CMessage::GetStackPtr(INT32 ndx, FramedMethodFrame *pFrame, MetaSig *pSig)
{
    LOG((LF_REMOTING, LL_INFO100,
         "CMessage::GetStackPtr IN ndx:0x%x, pFrame:0x%x, pSig:0x%x\n",
         ndx, pFrame, pSig));

    BEGINFORBIDGC();

    ArgIterator iter(pFrame, pSig);
    BYTE typ = 0;
    UINT32 size;
    PVOID ret = NULL;

    _ASSERTE((UINT)ndx < pSig->NumFixedArgs());    
    for (int i=0; i<=ndx; i++)
        ret = iter.GetNextArgAddr(&typ, &size);

    ENDFORBIDGC();

    // If this is a by-ref arg, GetObjectFromStack() will dereference "ret" to
    // get the real argument address. Dereferencing now will open a gc hole if "ret" 
    // points into the gc heap, and we trigger gc between here and the point where 
    // we return the arguments. 
    //if (typ == ELEMENT_TYPE_BYREF)
    //{
    //    return *((PVOID *) ret);
    //}

    return ret;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::MethodAccessCheck public
//
//  Synopsis:   Check caller's access to a method, throw security exception on
//              failure.
//
//  Parameters: method      - MethodBase to check
//              stackMark   - StackCrawlMark used to find caller on stack
//+----------------------------------------------------------------------------
FCIMPL2(void, CMessage::MethodAccessCheck, ReflectBaseObject* methodUNSAFE, StackCrawlMark* stackMark)
{
    REFLECTBASEREF method = (REFLECTBASEREF) methodUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(method);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    ReflectMethod* pRM = (ReflectMethod*)method->GetData();
    RefSecContext sCtx(stackMark);
    InvokeUtil::CheckAccess(&sCtx, pRM->attrs, pRM->pMethod->GetMethodTable(), REFSEC_THROW_SECURITY);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND
