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
** File: DebugDebugger.cpp
**
**                                                 
**
** Purpose: Native methods on System.Debug.Debugger
**
** Date:  April 2, 1998
**
===========================================================*/

#include "common.h"

#include <object.h>
#include "ceeload.h"
#include "corpermp.h"

#include "utilcode.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "field.h"
#include "comstringcommon.h"
#include "comstring.h"
#include "gc.h"
#include "commember.h" 
#include "sigformat.h"
#include "version/__product__.ver"
#include "jitinterface.h"
#include "comsystem.h"
#include "debugdebugger.h"
#include "dbginterface.h"
#include "cordebug.h"
#include "corsym.h"


LogHashTable g_sLogHashTable;

//Note: this code is copied from VM\JIT_UserBreakpoint, so propogate changes
//back to there
FCIMPL0(void, DebugDebugger::Break)
{
#ifdef DEBUGGING_SUPPORTED
    _ASSERTE (g_pDebugInterface != NULL);

    HELPER_METHOD_FRAME_BEGIN_0();

    g_pDebugInterface->SendUserBreakpoint(GetThread());

    HELPER_METHOD_FRAME_END();
#endif // DEBUGGING_SUPPORTED
}
FCIMPLEND

FCIMPL0(INT32, DebugDebugger::Launch)
{
#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerAttached())
    {
        return TRUE;
    }
    else
    {
        _ASSERTE (g_pDebugInterface != NULL);

        HRESULT hr;

        HELPER_METHOD_FRAME_BEGIN_RET_0();

        hr = g_pDebugInterface->LaunchDebuggerForUser ();

        HELPER_METHOD_FRAME_END();

        if (SUCCEEDED (hr))
            return TRUE;
    }
#endif // DEBUGGING_SUPPORTED

    return FALSE;
}
FCIMPLEND

FCIMPL0(INT32, DebugDebugger::IsDebuggerAttached)
{
#ifdef DEBUGGING_SUPPORTED
    //verbose so that we'll return (INT32)1 or (INT32)0
    if (GetThread()->GetDomain()->IsDebuggerAttached())
        return TRUE;

#endif // DEBUGGING_SUPPORTED

    return FALSE;
}
FCIMPLEND

FCIMPL3(void, DebugDebugger::Log, INT32 Level, StringObject* strModule, StringObject* strMessage)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef DEBUGGING_SUPPORTED

    HELPER_METHOD_FRAME_BEGIN_0();

//    IsLoggingArgs IsArgs;

//    IsArgs.m_strModule = pArgs->m_strModule;
//    IsArgs.m_Level = pArgs->m_Level;

    AppDomain *pAppDomain = GetThread()->GetDomain();

    // Send message for logging only if the 
    // debugger is attached and logging is enabled
    // for the given category
    if (pAppDomain->IsDebuggerAttached() || (Level == PanicLevel))
    {
        if ((IsLogging (Level, strModule) == TRUE) || (Level == PanicLevel))
        {
            int iCategoryLength = 0;
            int iMessageLength = 0;

            WCHAR   *pstrModuleName=NULL;
            WCHAR   *pstrMessage=NULL;
            WCHAR   wszSwitchName [MAX_LOG_SWITCH_NAME_LEN+1];
            WCHAR   *pwszMessage = NULL;

            wszSwitchName [0] = L'\0';

            if (strModule != NULL)
            {
                RefInterpretGetStringValuesDangerousForGC (
                                    strModule,
                                    &pstrModuleName,
                                    &iCategoryLength);

                if (iCategoryLength > MAX_LOG_SWITCH_NAME_LEN)
                {
                    wcsncpy (wszSwitchName, pstrModuleName, MAX_LOG_SWITCH_NAME_LEN);
                    wszSwitchName [MAX_LOG_SWITCH_NAME_LEN] = L'\0';
                    iCategoryLength = MAX_LOG_SWITCH_NAME_LEN;
                }
                else
                    wcscpy (wszSwitchName, pstrModuleName);
            }

            if (strMessage != NULL)
            {   
                RefInterpretGetStringValuesDangerousForGC (
                                    strMessage,
                                    &pstrMessage,
                                    &iMessageLength);
            }

            if ((iCategoryLength || iMessageLength) || (Level == PanicLevel))
            {
                bool fMemAllocated = false;
                if ((Level == PanicLevel) && (iMessageLength == 0))
                {
                    pwszMessage = L"Panic Message received";
                    iMessageLength = (int)wcslen (pwszMessage);
                }
                else
                {
                    pwszMessage = new WCHAR [iMessageLength + 1];
                    if (pwszMessage == NULL)
                        COMPlusThrowOM();

                    wcsncpy (pwszMessage, pstrMessage, iMessageLength);
                    pwszMessage [iMessageLength] = L'\0';
                    fMemAllocated = true;
                }

                g_pDebugInterface->SendLogMessage (
                                    Level,
                                    wszSwitchName,
                                    iCategoryLength,
                                    pwszMessage,
                                    iMessageLength
                                    );

                if (fMemAllocated)
                    delete [] pwszMessage;
            }
        }
    }

    HELPER_METHOD_FRAME_END();

#endif // DEBUGGING_SUPPORTED
}
FCIMPLEND


FCIMPL2(INT32, DebugDebugger::IsLogging, INT32 Level, StringObject* strModule)
{
#ifdef DEBUGGING_SUPPORTED
    if (GetThread()->GetDomain()->IsDebuggerAttached())
        return (g_pDebugInterface->IsLoggingEnabled());
#endif // DEBUGGING_SUPPORTED
    return FALSE;
}
FCIMPLEND

FCIMPL3(void, DebugStackTrace::GetStackFramesInternal, StackFrameHelper* pStackFrameHelperUNSAFE, INT32 iSkip, Object* pExceptionUNSAFE)
{    
    STACKFRAMEHELPERREF pStackFrameHelper   = pStackFrameHelperUNSAFE;
    OBJECTREF           pException          = ObjectToOBJECTREF(pExceptionUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_2(pStackFrameHelper, pException);

    HRESULT hr;
    ASSERT(iSkip >= 0);

    GetStackFramesData data;

    THROWSCOMPLUSEXCEPTION();

    data.pDomain = GetAppDomain();

    data.skip = iSkip;

    data.NumFramesRequested = pStackFrameHelper->iFrameCount;

    if (pException == NULL)
    {
        data.TargetThread = pStackFrameHelper->TargetThread;
        GetStackFrames(NULL, (void*)-1, &data);
    }
    else
    {
        GetStackFramesFromException(&pException, &data);
    }

    if (data.cElements != 0)
    {
        
        COMClass::EnsureReflectionInitialized();

        // Skip any JIT helpers...
        MethodTable *pJithelperClass = NULL;

        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__METHOD_BASE);

        // Allocate memory for the MethodInfo objects
        PTRARRAYREF MethodInfoArray = (PTRARRAYREF) AllocateObjectArray(data.cElements,
                                                                        TypeHandle(pMT));

        if (!MethodInfoArray)
            COMPlusThrowOM();
        SetObjectReference( (OBJECTREF *)&(pStackFrameHelper->rgMethodInfo), (OBJECTREF)MethodInfoArray,
                            pStackFrameHelper->GetAppDomain());

        // Allocate memory for the Offsets 
        OBJECTREF Offsets = AllocatePrimitiveArray(ELEMENT_TYPE_I4, data.cElements);

        if (! Offsets)
            COMPlusThrowOM();
        SetObjectReference( (OBJECTREF *)&(pStackFrameHelper->rgiOffset), (OBJECTREF)Offsets,
                            pStackFrameHelper->GetAppDomain());

        // Allocate memory for the ILOffsets 
        OBJECTREF ILOffsets = AllocatePrimitiveArray(ELEMENT_TYPE_I4, data.cElements);

        if (! ILOffsets)
            COMPlusThrowOM();
        SetObjectReference( (OBJECTREF *)&(pStackFrameHelper->rgiILOffset), (OBJECTREF)ILOffsets,
                            pStackFrameHelper->GetAppDomain());

        // if we need Filename, linenumber, etc., then allocate memory for the same
        // Allocate memory for the Filename string objects
        PTRARRAYREF FilenameArray = (PTRARRAYREF) AllocateObjectArray(data.cElements, g_pStringClass);

        if (!FilenameArray)
            COMPlusThrowOM();
        SetObjectReference( (OBJECTREF *)&(pStackFrameHelper->rgFilename), (OBJECTREF)FilenameArray,
                            pStackFrameHelper->GetAppDomain());

        // Allocate memory for the Offsets 
        OBJECTREF LineNumbers = AllocatePrimitiveArray(ELEMENT_TYPE_I4, data.cElements);

        if (! LineNumbers)
            COMPlusThrowOM();
        SetObjectReference( (OBJECTREF *)&(pStackFrameHelper->rgiLineNumber), (OBJECTREF)LineNumbers,
                            pStackFrameHelper->GetAppDomain());

        // Allocate memory for the ILOffsets 
        OBJECTREF ColumnNumbers = AllocatePrimitiveArray(ELEMENT_TYPE_I4, data.cElements);

        if (! ColumnNumbers)
            COMPlusThrowOM();
        SetObjectReference( (OBJECTREF *)&(pStackFrameHelper->rgiColumnNumber), (OBJECTREF)ColumnNumbers,
                            pStackFrameHelper->GetAppDomain());

        int iNumValidFrames = 0;
        for (int i=0; i<data.cElements; i++)
        {
            OBJECTREF o;
            // Skip Jit Helper functions, since they can throw when you have
            // a bug in your code, such as an invalid cast. Also skip if it is 
            // a constructor 
            if (data.pElements[i].pFunc->GetMethodTable() != pJithelperClass)
            {
                if (data.pElements[i].pFunc->IsCtor())
                {
                    EEClass *pEEClass = data.pElements[i].pFunc->GetClass();

                    REFLECTCLASSBASEREF obj = (REFLECTCLASSBASEREF) pEEClass->GetExposedClassObject();
                
                    if (!obj) {
                        _ASSERTE(!"Didn't find Object");
                        FATAL_EE_ERROR();
                    }
                    
                    ReflectClass* pRC = (ReflectClass*) obj->GetData();
                    _ASSERTE(pRC);
                    ReflectMethodList* pRML = pRC->GetConstructors();
                    ReflectMethod* pRM = pRML->FindMethod(data.pElements[i].pFunc);
                    _ASSERTE(pRM);

                    o = (OBJECTREF) (pRM->GetConstructorInfo(
                                        pRC));
                }
                else if (data.pElements[i].pFunc->IsStaticInitMethod())
                {
                    o = (OBJECTREF) (COMMember::g_pInvokeUtil->GetMethodInfo(
                                        data.pElements[i].pFunc));
                }
                else
                {
                    o = (OBJECTREF) (COMMember::g_pInvokeUtil->GetMethodInfo(
                                        data.pElements[i].pFunc));
                }

                pStackFrameHelper->rgMethodInfo->SetAt(iNumValidFrames, o);

                // native offset
                I4 *pI4 = (I4 *)((I4ARRAYREF)pStackFrameHelper->rgiOffset)
                                            ->GetDirectPointerToNonObjectElements();
                pI4 [iNumValidFrames] = data.pElements[i].dwOffset; 

                // IL offset
                I4 *pILI4 = (I4 *)((I4ARRAYREF)pStackFrameHelper->rgiILOffset)
                                            ->GetDirectPointerToNonObjectElements();
                pILI4 [iNumValidFrames] = data.pElements[i].dwILOffset; 

                BOOL fFileInfoSet = FALSE;

                // check if the user wants the filenumber, linenumber info...
                if (pStackFrameHelper->fNeedFileInfo)
                {
                    // Use the MethodDesc...
                    MethodDesc *pMethod = data.pElements[i].pFunc;
                    Module *pModule = pMethod->GetModule();

                    ULONG32 sourceLine = 0;
                    ULONG32 sourceColumn = 0;
                    WCHAR wszFileName[MAX_PATH];
                    ULONG32 fileNameLength = 0;

                    // Note: we need to enable preemptive GC when accessing the unmanages symbol store.
                    BEGIN_ENSURE_PREEMPTIVE_GC();

                    ISymUnmanagedReader *pISymUnmanagedReader = pModule->GetISymUnmanagedReader();

                    if (pISymUnmanagedReader != NULL)
                    {
                        ISymUnmanagedMethod *pISymUnmanagedMethod;  
                        hr = pISymUnmanagedReader->GetMethod(pMethod->GetMemberDef(), 
                                                &pISymUnmanagedMethod);

                        if (SUCCEEDED(hr))
                        {
                            // get all the sequence points and the documents 
                            // associated with those sequence points.
                            // from the doument get the filename using GetURL()
                            ULONG32 SeqPointCount;
                            ULONG32 DummyCount;

                            hr = pISymUnmanagedMethod->GetSequencePointCount(&SeqPointCount);
                            _ASSERTE (SUCCEEDED(hr));

                            if (SUCCEEDED(hr) && SeqPointCount > 0)
                                {
                                    // allocate memory for the objects to be fetched
                                    ULONG32 *offsets = new ULONG32 [SeqPointCount];
                                    ULONG32 *lines = new ULONG32 [SeqPointCount];
                                    ULONG32 *columns = new ULONG32 [SeqPointCount];
                                    ULONG32 *endlines = new ULONG32 [SeqPointCount];
                                    ULONG32 *endcolumns = new ULONG32 [SeqPointCount];
                                    ISymUnmanagedDocument **documents = 
                                        (ISymUnmanagedDocument **)new PVOID [SeqPointCount];

                                    _ASSERTE (offsets && lines && columns 
                                              && documents && endlines && endcolumns);

                                if ((offsets && lines && columns && documents && endlines && endcolumns))
                                    {
                                    hr = pISymUnmanagedMethod->GetSequencePoints (
                                                            SeqPointCount,
                                                            &DummyCount,
                                                            offsets,
                                                            (ISymUnmanagedDocument **)documents,
                                                            lines,
                                                            columns,
                                                            endlines,
                                                            endcolumns);

                                    _ASSERTE(SUCCEEDED(hr));
                                    _ASSERTE(DummyCount == SeqPointCount);

#ifdef _DEBUG
                                    {
                                        // This is just some debugging code to help ensure that the array
                                        // returned contains valid interface pointers.
                                        for (ULONG32 i = 0; i < SeqPointCount; i++)
                                        {
                                            _ASSERTE(documents[i] != NULL);
                                            _ASSERTE(!IsBadWritePtr((LPVOID)documents[i],
                                                                    sizeof(ISymUnmanagedDocument *)));
                                            documents[i]->AddRef();
                                            documents[i]->Release();
                                        }
                                    }
#endif

                                    if (SUCCEEDED(hr))
                                    {
                                    // This is the IL offset of the current frame
                                    DWORD dwCurILOffset = data.pElements[i].dwILOffset;

                                        // search for the correct IL offset
										DWORD j;
                                        for (j=0; j<SeqPointCount; j++)
                                        {
                                            // look for the entry matching the one we're looking for
                                        if (offsets[j] >= dwCurILOffset)
                                            {
                                            // if this offset is > what we're looking for, ajdust the index
                                            if (offsets[j] > dwCurILOffset && j > 0)
                                                j--;

                                                break;
                                            }
                                        }

                                    // If we didn't find a match, default to the last sequence point
                                        if  (j == SeqPointCount)
                                            j--;

                                        while (lines[j] == 0x00feefee && j > 0)
                                            j--;

#ifdef DEBUGGING_SUPPORTED
                                        DWORD dwDebugBits = pModule->GetDebuggerInfoBits();
                                        if (CORDebuggerTrackJITInfo(dwDebugBits) && lines[j] != 0x00feefee)
                                        {
                                            sourceLine = lines [j];  
                                            sourceColumn = columns [j];  
                                        }
                                        else
#endif // DEBUGGING_SUPPORTED
                                        {
                                            sourceLine = 0;  
                                            sourceColumn = 0;  
                                        }

                                            // Also get the filename from the document...
                                            _ASSERTE (documents [j] != NULL);

                                        hr = documents [j]->GetURL (MAX_PATH, &fileNameLength, wszFileName);
                                        _ASSERTE (SUCCEEDED(hr));


                                        // indicate that the requisite information has been set!
                                        fFileInfoSet = TRUE;
                                    }                                 

                                        // free up all the allocated memory
                                        delete [] lines;
                                        delete [] columns;
                                        delete [] offsets;
                                    for (DWORD j=0; j<SeqPointCount; j++)
                                            documents [j]->Release();
                                        delete [] documents;
                                        delete [] endlines;
                                        delete [] endcolumns;
                                    }                                 
                                }

                            //
                            // touching unmanaged object...
                            //
                            pISymUnmanagedMethod->Release();
                        }
                    }

                    END_ENSURE_PREEMPTIVE_GC();
                    
                    if (fFileInfoSet == TRUE)
                    {
                        // Set the line and column numbers
                        I4 *pI4Line = (I4 *)((I4ARRAYREF)pStackFrameHelper->rgiLineNumber)
                            ->GetDirectPointerToNonObjectElements();
                        I4 *pI4Column = (I4 *)((I4ARRAYREF)pStackFrameHelper->rgiColumnNumber)
                            ->GetDirectPointerToNonObjectElements();

                        pI4Line [iNumValidFrames] = sourceLine;  
                        pI4Column [iNumValidFrames] = sourceColumn;  

                        // Set the file name
                        OBJECTREF o = (OBJECTREF) COMString::NewString(wszFileName);
                        pStackFrameHelper->rgFilename->SetAt(iNumValidFrames, o);
                    }
                }


                if (fFileInfoSet == FALSE)
                {
                    I4 *pI4Line = (I4 *)((I4ARRAYREF)pStackFrameHelper->rgiLineNumber)
                                                ->GetDirectPointerToNonObjectElements();
                    I4 *pI4Column = (I4 *)((I4ARRAYREF)pStackFrameHelper->rgiColumnNumber)
                                                ->GetDirectPointerToNonObjectElements();
                    pI4Line [iNumValidFrames] = 0;
                    pI4Column [iNumValidFrames] = 0;

                    pStackFrameHelper->rgFilename->SetAt(iNumValidFrames, NULL);
                    
                }

                iNumValidFrames++;
            }
        }

        pStackFrameHelper->iFrameCount = iNumValidFrames;

        delete [] data.pElements;
    }
    else
    {
        pStackFrameHelper->iFrameCount = 0;
    }

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND



void DebugStackTrace::GetStackFrames(Frame *pStartFrame, void* pStopStack, GetStackFramesData *pData)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT (pData != NULL);
    
    pData->cElements = 0;

    // if the caller specified (< 20) frames are required, then allocate
    // only that many
    if ((pData->NumFramesRequested != 0) && (pData->NumFramesRequested < 20))
       pData->cElementsAllocated = pData->NumFramesRequested;
    else
       pData->cElementsAllocated = 20;

    // Allocate memory for the initial 'n' frames
    pData->pElements = new (throws) StackTraceElement[pData->cElementsAllocated];
    
    bool fThreadStoreLocked = false;
    EE_TRY_FOR_FINALLY 
    {
        if (pData->TargetThread == NULL)
        {
            GetThread()->StackWalkFrames(GetStackFramesCallback, pData, FUNCTIONSONLY, pStartFrame);
        }
        else
        {
            Thread *pThread = pData->TargetThread->GetInternal();
            _ASSERTE (pThread != NULL);
            ThreadStore::LockThreadStore();
            fThreadStoreLocked = true;
            Thread::ThreadState state = pThread->GetSnapshotState();

            if (!((state & Thread::TS_Unstarted) ||
                  (state & Thread::TS_Dead) ||
                  (state & Thread::TS_Detached) ||
                  (state & Thread::TS_SyncSuspended) ||
                  (state & Thread::TS_UserSuspendPending)
                  ))
            {
                COMPlusThrow(kThreadStateException, IDS_EE_THREAD_BAD_STATE);
            }           

            pThread->StackWalkFrames(GetStackFramesCallback, pData, FUNCTIONSONLY, pStartFrame);
        }
    }
    EE_FINALLY
    {
        if (fThreadStoreLocked)
            ThreadStore::UnlockThreadStore();
        
    } EE_END_FINALLY
}

StackWalkAction DebugStackTrace::GetStackFramesCallback(CrawlFrame* pCf, VOID* data)
{
    GetStackFramesData* pData = (GetStackFramesData*)data;

    if (pData->pDomain != pCf->GetAppDomain())
        return SWA_CONTINUE;

    if (pData->skip > 0) {
        pData->skip--;
        return SWA_CONTINUE;
    }

    //        Can we always assume FramedMethodFrame?
    //        NOT AT ALL!!!, but we can assume it's a function
    //                       because we asked the stackwalker for it!
    MethodDesc* pFunc = pCf->GetFunction();

    if (pData->cElements >= pData->cElementsAllocated) {

        StackTraceElement* pTemp = new (nothrow) StackTraceElement[2*pData->cElementsAllocated];
        if (!pTemp)
            return SWA_ABORT;
        memcpy(pTemp, pData->pElements, pData->cElementsAllocated* sizeof(StackTraceElement));
        delete [] pData->pElements;
        pData->pElements = pTemp;
        pData->cElementsAllocated *= 2;
    }    

    pData->pElements[pData->cElements].pFunc = pFunc;

    SLOT ip;

    if (pCf->IsFrameless())
    {
        pData->pElements[pData->cElements].dwOffset = pCf->GetRelOffset();
        ip = (PBYTE)GetControlPC(pCf->GetRegisterSet());
    }
    else
    {
        ip = (SLOT)((FramedMethodFrame*)(pCf->GetFrame()))->GetIP();

        pData->pElements[pData->cElements].dwOffset = (DWORD) (size_t)ip; 
    }

    // Also get the ILOffset
#ifdef DEBUGGING_SUPPORTED
    DWORD ilOffset = (DWORD) -1;
    g_pDebugInterface->GetILOffsetFromNative(
                            pFunc,
                            (const BYTE *)ip,
                            pData->pElements[pData->cElements].dwOffset, 
                            &ilOffset);
    pData->pElements[pData->cElements].dwILOffset = (DWORD)ilOffset;
#endif // DEBUGGING_SUPPORTED

    ++pData->cElements;

    // check if we already have the number of frames that the user had asked for
    if ((pData->NumFramesRequested != 0) 
        && 
        (pData->NumFramesRequested <= pData->cElements))
        return SWA_ABORT;

    return SWA_CONTINUE;
}


void DebugStackTrace::GetStackFramesFromException(OBJECTREF * e, GetStackFramesData *pData)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT (pData != NULL);

    // Reasonable default, will indicate error on failure
    pData->cElements = 0;

    // Get the class for the exception
    EEClass *pExcepClass = (*e)->GetClass();
    if (!IsException(pExcepClass))
        return;

    // Get the _stackTrace field descriptor
    FieldDesc *pStackTraceFD = g_Mscorlib.GetField(FIELD__EXCEPTION__STACK_TRACE);

    // Now get the _stackTrace reference
    I1ARRAYREF pTraceData;
    pTraceData = I1ARRAYREF(pStackTraceFD->GetRefValue(*e));

    // Get the size of the array
    unsigned cbTraceData = 0;
    if (pTraceData != NULL)
        cbTraceData = pTraceData->GetNumComponents();
    else
        cbTraceData = 0;
    _ASSERTE(cbTraceData % sizeof(SystemNative::StackTraceElement) == 0);

    // The number of frame info elements in the stack trace info
    pData->cElements = cbTraceData / sizeof(SystemNative::StackTraceElement);

    // Now we know the size, allocate the information for the data struct
    if (cbTraceData != 0)
    {
        // Allocate the memory to contain the data
        pData->pElements = new (throws) StackTraceElement[pData->cElements];

        // Get a pointer to the actuall array data
        SystemNative::StackTraceElement *arrTraceData =
            (SystemNative::StackTraceElement *)pTraceData->GetDirectPointerToNonObjectElements();
        _ASSERTE(arrTraceData);

        GCPROTECT_BEGININTERIOR(arrTraceData);
        // Fill in the data
        for (unsigned i = 0; i < (unsigned)pData->cElements; i++)
        {
            SystemNative::StackTraceElement *pCur = &arrTraceData[i];

            // Fill out the MethodDesc*
            MethodDesc *pMD = pCur->pFunc;
            _ASSERTE(pMD);
            pData->pElements[i].pFunc = pMD;

            // Calculate the native offset
            // This doesn't work for framed methods, since internal calls won't
            // push frames and the method body is therefore non-contiguous.
            // Currently such methods always return an IP of 0, so they're easy
            // to spot.
            DWORD dwNativeOffset;
            if (pCur->ip)
                dwNativeOffset = (DWORD)(pCur->ip - pMD->GetFunctionAddress());
            else
                dwNativeOffset = 0;
            pData->pElements[i].dwOffset = dwNativeOffset;

#ifdef DEBUGGING_SUPPORTED
            // Calculate the IL offset using the debugging services
            bool bRes = g_pDebugInterface->GetILOffsetFromNative(
                pMD, (const BYTE *)pCur->ip, dwNativeOffset, &pData->pElements[i].dwILOffset);
#else // !DEBUGGING_SUPPORTED
            bool bRes = false;
#endif // !DEBUGGING_SUPPORTED

            // If there was no mapping information, then set to an invalid value
            if (!bRes)
                pData->pElements[i].dwILOffset = (DWORD)-1;
        }
        GCPROTECT_END();
    }
    else
        pData->pElements = NULL;

    return;
}

FCIMPL2(INT32, DebuggerAssert::ShowDefaultAssertDialog, StringObject* strConditionUNSAFE, StringObject* strMessageUNSAFE)
{
    int         result          = IDRETRY;
    STRINGREF   strCondition    = (STRINGREF) strConditionUNSAFE;
    STRINGREF   strMessage      = (STRINGREF) strMessageUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_2(strCondition, strMessage);

    THROWSCOMPLUSEXCEPTION();


    int iConditionLength = 0;
    int iMessageLength = 0;

    WCHAR   *pstrCondition=NULL;
    WCHAR   *pstrMessage=NULL;

    if (strCondition != NULL)
    {
        RefInterpretGetStringValuesDangerousForGC (
                            strCondition,
                            &pstrCondition,
                            &iConditionLength);
    }

    if (strMessage != NULL)
    {   
        RefInterpretGetStringValuesDangerousForGC (
                            strMessage,
                            &pstrMessage,
                            &iMessageLength);
    }
                                        
    WCHAR *pStr = new WCHAR [iConditionLength+iMessageLength+256];
    if (pStr == NULL)
        COMPlusThrowOM();

    wcscpy (pStr, L"Expression: ");
    wcscat (pStr, pstrCondition);
    wcscat (pStr, L"\n");
    wcscat (pStr, L"Description: ");
    wcscat (pStr, pstrMessage);
    wcscat (pStr, L"\n\n");
    wcscat (pStr, L"Press RETRY to attach debugger\n");

    // Give a message box to the user about the exception.
    WCHAR titleString[MAX_PATH + 64]; // extra room for title
    WCHAR fileName[MAX_PATH];
    
    if (WszGetModuleFileName(NULL, fileName, MAX_PATH))
        swprintf(titleString,
                 L"%s - Assert Failure",
                 fileName);
    else
        wcscpy(titleString, L"Assert Failure");

    // Toggle the threads's GC mode so that GC can occur
    Thread *pThread = GetThread();
    BOOL    fToggleGC = (pThread && pThread->PreemptiveGCDisabled());

    if (fToggleGC)
        pThread->EnablePreemptiveGC();

    result = WszMessageBoxInternal(NULL, pStr, titleString,
                           MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION);

    if (fToggleGC)
        pThread->DisablePreemptiveGC();

    // map the user's choice to the values recognized by 
    // the System.Diagnostics.Assert package
    if (result == IDRETRY)
        result = FailDebug;
    else if (result == IDIGNORE)
        result = FailIgnore;
    else
        result = FailTerminate;

    delete [] pStr;

    HELPER_METHOD_FRAME_END();
    return result;
}
FCIMPLEND



FCIMPL1( INT32, Log::AddLogSwitch, LogSwitchObject* m_LogSwitchUNSAFE )
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);

    HRESULT hresult;
        
    struct __gc {
        OBJECTHANDLE    ObjHandle;
        LOGSWITCHREF    m_LogSwitch;
        STRINGREF       Name;
        OBJECTREF       tempObj;
        STRINGREF       strrefParentName;
    } gc;

    ZeroMemory(&gc, sizeof(gc));

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    GCPROTECT_BEGIN(gc);
        
    gc.m_LogSwitch = m_LogSwitchUNSAFE;

    // Create a strong reference handle to the LogSwitch object
    gc.ObjHandle = pThread->GetDomain()->CreateStrongHandle(NULL);
    StoreObjectInHandle( gc.ObjHandle, ObjectToOBJECTREF(gc.m_LogSwitch));
    // Use  ObjectFromHandle(ObjHandle) to get back the object. 

    // From the given args, extract the LogSwitch name
    gc.Name = ((LogSwitchObject*) OBJECTREFToObject(gc.m_LogSwitch))->GetName();

    _ASSERTE( gc.Name != NULL );
    WCHAR *pstrCategoryName = NULL;
    int iCategoryLength = 0;
    WCHAR wszParentName [MAX_LOG_SWITCH_NAME_LEN+1];
    WCHAR wszSwitchName [MAX_LOG_SWITCH_NAME_LEN+1];
    wszParentName [0] = L'\0';
    wszSwitchName [0] = L'\0';

    // extract the (WCHAR) name from the STRINGREF object
    RefInterpretGetStringValuesDangerousForGC( gc.Name, &pstrCategoryName, &iCategoryLength );

    _ASSERTE (iCategoryLength > 0);
    if (iCategoryLength > MAX_LOG_SWITCH_NAME_LEN)
    {
        wcsncpy (wszSwitchName, pstrCategoryName, MAX_LOG_SWITCH_NAME_LEN);
        wszSwitchName [MAX_LOG_SWITCH_NAME_LEN] = L'\0';
    }
    else
    {
        wcscpy (wszSwitchName, pstrCategoryName);
    }

    // check if an entry with this name already exists in the hash table.
    // Duplicates are not allowed.
    if( g_sLogHashTable.GetEntryFromHashTable( pstrCategoryName ) != NULL )
    {
        hresult = TYPE_E_DUPLICATEID;
    }
    else
    {
        hresult = g_sLogHashTable.AddEntryToHashTable( pstrCategoryName, gc.ObjHandle );

#ifdef DEBUGGING_SUPPORTED
        if (hresult == S_OK)
        {
            // tell the attached debugger about this switch
            if (GetThread()->GetDomain()->IsDebuggerAttached())
            {
                int iLevel = gc.m_LogSwitch->GetLevel();
                WCHAR *pstrParentName = NULL;
                int iParentNameLength = 0;

                gc.tempObj = gc.m_LogSwitch->GetParent();

                LogSwitchObject* pParent = (LogSwitchObject*) OBJECTREFToObject( gc.tempObj );

                if (pParent != NULL)
                {
                    // From the given args, extract the ParentLogSwitch's name
                    gc.strrefParentName = pParent->GetName();

                    // extract the (WCHAR) name from the STRINGREF object
                    RefInterpretGetStringValuesDangerousForGC( gc.strrefParentName, &pstrParentName, &iParentNameLength );

                    if (iParentNameLength > MAX_LOG_SWITCH_NAME_LEN)
                    {
                        wcsncpy (wszParentName, pstrParentName, MAX_LOG_SWITCH_NAME_LEN);
                        wszSwitchName [MAX_LOG_SWITCH_NAME_LEN] = L'\0';
                    }
                    else
                        wcscpy (wszParentName, pstrParentName);
                }

                g_pDebugInterface->SendLogSwitchSetting (iLevel, SWITCH_CREATE, wszSwitchName, wszParentName );
            }
        }   
#endif // DEBUGGING_SUPPORTED
    }

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return hresult;
}
FCIMPLEND



FCIMPL3(void, Log::ModifyLogSwitch, INT32 Level, StringObject* strLogSwitchNameUNSAFE, StringObject* strParentNameUNSAFE)
{
    STRINGREF strLogSwitchName = (STRINGREF) strLogSwitchNameUNSAFE;
    STRINGREF strParentName = (STRINGREF) strParentNameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_2(strLogSwitchName, strParentName);
    //-[autocvtpro]-------------------------------------------------------

    _ASSERTE (strLogSwitchName != NULL);
    
    WCHAR *pstrLogSwitchName = NULL;
    WCHAR *pstrParentName = NULL;
    int iSwitchNameLength = 0;
    int iParentNameLength = 0;
    WCHAR wszParentName [MAX_LOG_SWITCH_NAME_LEN+1];
    WCHAR wszSwitchName [MAX_LOG_SWITCH_NAME_LEN+1];
    wszParentName [0] = L'\0';
    wszSwitchName [0] = L'\0';

    // extract the (WCHAR) name from the STRINGREF object
    RefInterpretGetStringValuesDangerousForGC (
                        strLogSwitchName,
                        &pstrLogSwitchName,
                        &iSwitchNameLength);

    if (iSwitchNameLength > MAX_LOG_SWITCH_NAME_LEN)
    {
        wcsncpy (wszSwitchName, pstrLogSwitchName, MAX_LOG_SWITCH_NAME_LEN);
        wszSwitchName [MAX_LOG_SWITCH_NAME_LEN] = L'\0';
    }
    else
        wcscpy (wszSwitchName, pstrLogSwitchName);

    // extract the (WCHAR) name from the STRINGREF object
    RefInterpretGetStringValuesDangerousForGC (
                        strParentName,
                        &pstrParentName,
                        &iParentNameLength);

    if (iParentNameLength > MAX_LOG_SWITCH_NAME_LEN)
    {
        wcsncpy (wszParentName, pstrParentName, MAX_LOG_SWITCH_NAME_LEN);
        wszSwitchName [MAX_LOG_SWITCH_NAME_LEN] = L'\0';
    }
    else
        wcscpy (wszParentName, pstrParentName);

#ifdef DEBUGGING_SUPPORTED
    g_pDebugInterface->SendLogSwitchSetting (Level,
                                            SWITCH_MODIFY,
                                            wszSwitchName,
                                            wszParentName
                                            );
#endif // DEBUGGING_SUPPORTED

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


void Log::DebuggerModifyingLogSwitch (int iNewLevel, WCHAR *pLogSwitchName)
{
    // check if an entry with this name exists in the hash table.
    OBJECTHANDLE ObjHandle = g_sLogHashTable.GetEntryFromHashTable (pLogSwitchName);
    if ( ObjHandle != NULL)
    {
        OBJECTREF obj = ObjectFromHandle (ObjHandle);
        LogSwitchObject *pLogSwitch = 
                (LogSwitchObject *)(OBJECTREFToObject (obj));

        pLogSwitch->SetLevel (iNewLevel);
    }
}


// Note: Caller should ensure that it's not adding a duplicate
// entry by calling GetEntryFromHashTable before calling this
// function.
HRESULT LogHashTable::AddEntryToHashTable (WCHAR *pKey, OBJECTHANDLE pData)
{
    HashElement *pElement;

    // check that the length is non-zero
    if (pKey == NULL)
        return (E_INVALIDARG);

    int iHashKey = 0;
    int iLength = (int)wcslen (pKey);

    for (int i= 0; i<iLength; i++)
        iHashKey += pKey [i];

    iHashKey = iHashKey % MAX_HASH_BUCKETS;

    // Create a new HashElement
    if ((pElement = new HashElement) == NULL)
    {
        return (E_OUTOFMEMORY);
    }

    pElement->SetData (pData, pKey);

    if (m_Buckets [iHashKey] == NULL)
    {
        m_Buckets [iHashKey] = pElement;
    }
    else
    {
        pElement->SetNext (m_Buckets [iHashKey]);
        m_Buckets [iHashKey] = pElement;
    }

    return S_OK;
}



OBJECTHANDLE LogHashTable::GetEntryFromHashTable (WCHAR *pKey)
{

    if (pKey == NULL)
        return NULL;

    int iHashKey = 0;
    int iLength = (int)wcslen (pKey);

    // Calculate the hash value of the given key
    for (int i= 0; i<iLength; i++)
        iHashKey += pKey [i];

    iHashKey = iHashKey % MAX_HASH_BUCKETS;

    HashElement *pElement = m_Buckets [iHashKey];

    // Find and return the data
    while (pElement != NULL)
    {
        if (wcscmp(pElement->GetKey(), pKey) == 0)
            return (pElement->GetData());

        pElement = pElement->GetNext();
    }

    return NULL;
}
