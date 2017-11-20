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
** File:    StackBuilderSink.cpp
**
**                                              
**
** Purpose: Native implementaion for Microsoft.Runtime.StackBuilderSink
**
** Date:    Mar 24, 1999
**
===========================================================*/
#include "common.h"
#include "comstring.h"
#include "comreflectioncommon.h"
#include "excep.h"
#include "message.h"
#include "reflectwrap.h"
#include "stackbuildersink.h"
#include "dbginterface.h"
#include "remoting.h"
#include "profilepriv.h"
#include "class.h"

//+----------------------------------------------------------------------------
//
//  Method:     CStackBuilderSink::PrivateProcessMessage, public
//
//  Synopsis:   Builds the stack and calls an object
//
//+----------------------------------------------------------------------------
FCIMPL7(Object*, CStackBuilderSink::PrivateProcessMessage, Object* pSBSinkUNSAFE, ReflectBaseObject* pMethodBaseUNSAFE, PTRArray* pArgsUNSAFE, Object* pServerUNSAFE, void* iMethodPtr, BOOL fContext, PTRARRAYREF* ppVarOutParams)
{
    OBJECTREF ret = NULL;
    struct _gc
    {
        REFLECTBASEREF pMethodBase;
        PTRARRAYREF pArgs;
        OBJECTREF pServer;
        OBJECTREF pSBSink;
    } gc;
    gc.pMethodBase = (REFLECTBASEREF) pMethodBaseUNSAFE;
    gc.pArgs = (PTRARRAYREF) pArgsUNSAFE;
    gc.pServer = (OBJECTREF) pServerUNSAFE;
    gc.pSBSink = (OBJECTREF) pSBSinkUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    TRIGGERSGC();

    LOG((LF_REMOTING, LL_INFO10,
         "CStackBuilderSink::PrivateProcessMessage\n"));
    
    _ASSERTE(gc.pMethodBase != NULL);
    ReflectMethod *pRM = (ReflectMethod *)gc.pMethodBase->GetData();
    MethodDesc *pMD = pRM->pMethod;    

	// Either pServer is non-null or the method is static (but not both)
    _ASSERTE((gc.pServer!=NULL) == !(pMD->IsStatic()));

    // Check if this is an interface invoke, if yes, then we have to find the
    // real method descriptor on the class of the server object.
    if(pMD->GetMethodTable()->IsInterface())
    {
        _ASSERTE(gc.pServer != NULL);

        MethodDesc* pTemp = pMD;
        // NOTE: This method can trigger GC
        pMD = gc.pServer->GetMethodTable()->GetMethodDescForInterfaceMethod(pMD, gc.pServer);
        if(NULL == pMD)
        {
            MAKE_WIDEPTR_FROMUTF8(wName, pTemp->GetName())
            COMPlusThrow(kMissingMethodException, IDS_EE_MISSING_METHOD, NULL, wName);
        }
    }

    MetaSig mSig(pMD->GetSig(), pMD->GetModule());
    
    // get the target depending on whether the method is virtual or non-virtual
    // like a constructor, private or final method
    const BYTE* pTarget = NULL;
     
    if (iMethodPtr) 
    {
        pTarget = (const BYTE*) iMethodPtr;
    }
    else
    {
        // Get the address of the code
        pTarget = MethodTable::GetTargetFromMethodDescAndServer(pMD, &(gc.pServer), fContext);    
    }
    

    VASigCookie *pCookie = NULL;
    _ASSERTE(NULL != pTarget);
    GCPROTECT_BEGIN (ret);
            // this function does the work
    ::CallDescrWithObjectArray(
            gc.pServer, 
            pRM, 
            pTarget, 
            &mSig, 
            pCookie, 
            gc.pServer==NULL?TRUE:FALSE, //fIsStatic
            gc.pArgs, 
            &ret,
            ppVarOutParams);
    GCPROTECT_END ();

    LOG((LF_REMOTING, LL_INFO10,
         "CStackBuilderSink::PrivateProcessMessage OUT\n"));

        //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND


struct ArgInfo
{
    PBYTE             dataLocation;
    INT32             dataSize;
    EEClass          *dataClass;
    BYTE              dataType;
    BYTE              byref;
};

//+----------------------------------------------------------------------------
//
//  Function:   CallDescrWithObjectArray, private
//
//  Synopsis:   Builds the stack from a object array and call the object
//
// Note this function triggers GC and assumes that pServer, pArguments, pVarRet, and ppVarOutParams are
// all already protected!!
//+----------------------------------------------------------------------------
void CallDescrWithObjectArray(OBJECTREF& pServer, 
                  ReflectMethod *pRM, 
                  const BYTE *pTarget, 
                  MetaSig* sig, 
                  VASigCookie *pCookie,
                  BOOL fIsStatic,  
                  PTRARRAYREF& pArgArray,
                  OBJECTREF *pVarRet,
                  PTRARRAYREF *ppVarOutParams) 
{
    THROWSCOMPLUSEXCEPTION();
    TRIGGERSGC();       // the debugger, profiler code triggers a GC

    LOG((LF_REMOTING, LL_INFO10,
         "CallDescrWithObjectArray IN\n"));

    ByRefInfo *pByRefs = NULL;
    INT64 retval = 0;
    UINT  nActualStackBytes = 0;
    LPBYTE pAlloc = 0;
    LPBYTE pFrameBase = 0;
    UINT32 numByRef = 0;
    DWORD attr = pRM->dwFlags;
#ifdef _DEBUG
    MethodDesc *pMD = pRM->pMethod;
#endif

    // check the calling convention

    BYTE callingconvention = sig->GetCallingConvention();
    if (!isCallConv(callingconvention, IMAGE_CEE_CS_CALLCONV_DEFAULT))
    {
        _ASSERTE(!"This calling convention is not supported.");
        COMPlusThrow(kInvalidProgramException);
    }

#ifdef DEBUGGING_SUPPORTED
    // debugger goo What does this do? can someone put a comment here?
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall(pTarget);
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    // If we're profiling, notify the profiler that we're about to invoke the remoting target
    if (CORProfilerTrackRemoting())
        g_profControlBlock.pProfInterface->RemotingServerInvocationStarted(
            reinterpret_cast<ThreadID>(GetThread()));
#endif // PROFILING_SUPPORTED
        
    // Create a fake FramedMethodFrame on the stack.
    nActualStackBytes = sig->SizeOfActualFixedArgStack(fIsStatic);
    pAlloc = (LPBYTE)_alloca(FramedMethodFrame::GetNegSpaceSize() + sizeof(FramedMethodFrame) + nActualStackBytes);
    pFrameBase = pAlloc + FramedMethodFrame::GetNegSpaceSize();


    // cycle through the parameters and see if there are byrefs

    BYTE typ = 0;
    BOOL   fHasByRefs = FALSE;

    if (attr & RM_ATTR_BYREF_FLAG_SET)
        fHasByRefs = attr & RM_ATTR_HAS_BYREF_ARG;
    else
    {
        sig->Reset();
        while ((typ = sig->NextArg()) != ELEMENT_TYPE_END)
        {
            if (typ == ELEMENT_TYPE_BYREF)
            {
                fHasByRefs = TRUE;
                attr |= RM_ATTR_HAS_BYREF_ARG;
                break;
            }
        }
        attr |= RM_ATTR_BYREF_FLAG_SET;
        pRM->dwFlags = attr;
        sig->Reset();
    }

    int nFixedArgs = sig->NumFixedArgs();
    // if there are byrefs allocate and array for the out parameters

    if (fHasByRefs)
    {
        *ppVarOutParams = PTRARRAYREF(AllocateObjectArray(sig->NumFixedArgs(), g_pObjectClass));

        // Null out the array
        memset(&(*ppVarOutParams)->m_Array, 0, sizeof(OBJECTREF) * sig->NumFixedArgs());
    }

    ArgIterator argit(pFrameBase, sig, fIsStatic);

    // set the this pointer
    OBJECTREF *ppThis = (OBJECTREF*)argit.GetThisAddr();
    *ppThis = NULL;

    // if there is a return buffer, allocate it
    if (sig->HasRetBuffArg()) 
    {
        EEClass *pEECValue = sig->GetRetEEClass();
        _ASSERTE(pEECValue->IsValueClass());
        MethodTable * mt = pEECValue->GetMethodTable();
        *pVarRet = AllocateObject(mt);

        *(argit.GetRetBuffArgAddr()) = (*pVarRet)->UnBox();

#if defined(_PPC_) || defined(_SPARC_) // retbuf
        // the CallDescrWorker callsite for methods with return buffer is 
        //  different for RISC CPUs - we pass this information along by setting 
        //  the lowest bit in pTarget
        pTarget = (const BYTE *)((UINT_PTR)pTarget | 0x1);
#endif
    }



    // gather data about the parameters by iterating over the sig:
    UINT32 arg = 0;    
    UINT32 structSize = 0;
    int    ofs = 0;
    // REVIEW: need to use actual arg count if VarArgs are supported
    ArgInfo* pArgInfo = (ArgInfo*) _alloca(nFixedArgs*sizeof(ArgInfo));
#ifdef _DEBUG
    // We expect to write useful data over every part of this so need
    // not do this in retail!
    memset((void *)pArgInfo, 0, sizeof(ArgInfo)*nFixedArgs);
#endif
    for( ; 0 != (ofs = argit.GetNextOffset(&typ, &structSize)); arg++, pArgInfo++ )
    {


        if (typ == ELEMENT_TYPE_BYREF)
        {
            EEClass *pClass = NULL;
            CorElementType brType = sig->GetByRefType(&pClass);
            if (CorIsPrimitiveType(brType))
            {
                pArgInfo->dataSize = gElementTypeInfo[brType].m_cbSize;
            }
            else if (pClass->IsValueClass())
            {
                pArgInfo->dataSize = pClass->GetAlignedNumInstanceFieldBytes();
                numByRef ++;
            }
            else
            {

                pArgInfo->dataSize = sizeof(Object *);
                numByRef ++;
            }
            ByRefInfo *brInfo = (ByRefInfo *) _alloca(offsetof(ByRefInfo,data) + pArgInfo->dataSize);
            brInfo->argIndex = arg;
            brInfo->typ = brType;
            brInfo->pClass = pClass;
            brInfo->pNext = pByRefs;
            pByRefs = brInfo;
            pArgInfo->dataLocation = (BYTE*)brInfo->data;
            *((void**)(pFrameBase + ofs)) = (void*)pArgInfo->dataLocation;
            pArgInfo->dataClass = pClass;
            pArgInfo->dataType = brType;
            pArgInfo->byref = TRUE;
        }
        else
        {
            pArgInfo->dataLocation = pFrameBase + ofs;
            pArgInfo->dataSize = StackElemSize(structSize);
            pArgInfo->dataClass = sig->GetTypeHandle().GetClass(); // this may cause GC!
            pArgInfo->dataType = typ;
            pArgInfo->byref = FALSE;
        }  
    }

    if (!fIsStatic) {
        // If this isn't a value class, verify the objectref
#ifdef _DEBUG
        if (pMD->GetClass()->IsValueClass() == FALSE)
        {
            VALIDATEOBJECTREF(pServer);
        }
#endif //_DEBUG
        *ppThis = pServer;
     }

    // There should be no GC when we fill up the stack with parameters, as we don't protect them
    // Assignment of "*ppThis" above triggers the point where we become unprotected.
    BEGINFORBIDGC();


    // reset pArgInfo to point to the start of the block we _alloca-ed
    pArgInfo = pArgInfo-nFixedArgs;

    PBYTE             dataLocation;
    INT32             dataSize;
    EEClass          *dataClass;
    BYTE              dataType;

    OBJECTREF* pArguments = pArgArray->m_Array;
    UINT32 i, j = arg;
    for (i=0; i<j; i++)
    {
        dataSize = pArgInfo->dataSize;
        dataLocation = pArgInfo->dataLocation;
        dataClass = pArgInfo->dataClass;
        dataType = pArgInfo->dataType;

        switch (dataSize) 
        {
            case 1:
                // This "if" statement is necessary to make the assignement big-endian aware
                if (pArgInfo->byref)
                    *((INT8*)dataLocation) = *((INT8*)pArguments[i]->GetData());
                else
                    *(StackElemType*)dataLocation = (StackElemType)*((INT8*)pArguments[i]->GetData());
                break;
            case 2:
                // This "if" statement is necessary to make the assignement big-endian aware
                if (pArgInfo->byref)
                    *((INT16*)dataLocation) = *((INT16*)pArguments[i]->GetData());
                else
                    *(StackElemType*)dataLocation = (StackElemType)*((INT16*)pArguments[i]->GetData());
                break;
            case 4:
                if ((dataType == ELEMENT_TYPE_STRING)  ||
                    (dataType == ELEMENT_TYPE_OBJECT)  ||
                    (dataType == ELEMENT_TYPE_CLASS)   ||
                    (dataType == ELEMENT_TYPE_SZARRAY) ||
                    (dataType == ELEMENT_TYPE_ARRAY))
                {
                    *(OBJECTREF *)dataLocation = pArguments[i];
                }
                else
                {
                    *(StackElemType*)dataLocation = (StackElemType)*((INT32*)pArguments[i]->GetData());
                }
                break;

            case 8:
                *((INT64*)dataLocation) = *((INT64*)pArguments[i]->GetData());
                break;

            default:
            {
                memcpy(dataLocation, pArguments[i]->UnBox(), dataSize);
            }
        }

        pArgInfo++;
    }
#ifdef _DEBUG
    // Should not be using this any more
    pArgInfo = pArgInfo - nFixedArgs;
    memset((void *)pArgInfo, 0, sizeof(ArgInfo)*nFixedArgs);
#endif

    // if there were byrefs, push a protection frame

    ProtectByRefsFrame *pProtectionFrame = NULL;    
    if (pByRefs && numByRef > 0)
    {
        char *pBuffer = (char*)_alloca (sizeof (ProtectByRefsFrame));
        pProtectionFrame = new (pBuffer) ProtectByRefsFrame(GetThread(), pByRefs);
    }

    // call the correct worker function depending of if the method
    // is varargs or not

    ENDFORBIDGC();

#ifdef _PPC_
    FramedMethodFrame::Enregister(pFrameBase, sig, fIsStatic, nActualStackBytes);
#endif

    INSTALL_COMPLUS_EXCEPTION_HANDLER();

    retval = CallDescrWorker(
                pFrameBase + sizeof(FramedMethodFrame) + nActualStackBytes,
                nActualStackBytes / STACK_ELEM_SIZE,
#if defined(_X86_) || defined(_PPC_) // argregs
                (ArgumentRegisters*)(pFrameBase + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
#endif
                (LPVOID)pTarget);

    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();

    // set floating point return values

    getFPReturn(sig->GetFPReturnSize(), &retval);

    // need to build a object based on the return type.
    
    if (!sig->HasRetBuffArg()) 
    {
        BYTE *pRetVal = (BYTE*)&retval;

#ifdef BIGENDIAN
        switch (sig->GetReturnTypeSize())
        {
        case 1:
            pRetVal += sizeof(void*)-1;
            break;
        case 2:
            pRetVal += sizeof(void*)-2;
            break;
        default:
            // nothing to do
            break;
        }
#endif

        GetObjectFromStack(pVarRet, pRetVal, sig->GetReturnType(), sig->GetRetEEClass());
    }

    // extract the out args from the byrefs

    if (pByRefs)
    {     
        do
        {
            // Always extract the data ptr every time we enter this loop because
            // calls to GetObjectFromStack below can cause a GC.
            // Even this is not enough, because that we are passing a pointer to GC heap
            // to GetObjectFromStack .  If GC happens, nobody is protecting the passed in pointer.

            OBJECTREF pTmp = NULL;
            GetObjectFromStack(&pTmp, pByRefs->data, pByRefs->typ, pByRefs->pClass);
            (*ppVarOutParams)->SetAt(pByRefs->argIndex, pTmp);
            pByRefs = pByRefs->pNext;
        }
        while (pByRefs);
        if (pProtectionFrame) pProtectionFrame->Pop();
    }

#ifdef PROFILING_SUPPORTED
    // If we're profiling, notify the profiler that we're about to invoke the remoting target
    if (CORProfilerTrackRemoting())
        g_profControlBlock.pProfInterface->RemotingServerInvocationReturned(
            reinterpret_cast<ThreadID>(GetThread()));
#endif // PROFILING_SUPPORTED

    LOG((LF_REMOTING, LL_INFO10, "CallDescrWithObjectArray OUT\n"));
}
