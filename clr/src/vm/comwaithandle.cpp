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
** COMWaitHandle.cpp
**
**                                                  
**
** Purpose: Native methods on System.WaitHandle
**
** Date:  August, 1999
**
===========================================================*/
#include "common.h"
#include "object.h"
#include "field.h"
#include "reflectwrap.h"
#include "excep.h"
#include "comwaithandle.h"

FCIMPL3(BOOL, WaitHandleNative::CorWaitOneNative, LPVOID handle, INT32 timeout, BYTE exitContext)
{
    BOOL retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    _ASSERTE(handle);

    Thread* pThread = SetupThread();
    _ASSERTE(pThread != NULL);
    _ASSERTE(pThread == GetThread());

	DWORD res;

	Context* targetContext = pThread->GetContext();
	_ASSERTE(targetContext);
	Context* defaultContext = pThread->GetDomain()->GetDefaultContext();
	_ASSERTE(defaultContext);
	if (exitContext != NULL &&
		targetContext != defaultContext)
	{
		Context::WaitArgs waitOneArgs = {1, &handle, TRUE, timeout, TRUE, &res};
		Context::CallBackInfo callBackInfo = {Context::Wait_callback, (void*) &waitOneArgs};
		Context::RequestCallBack(defaultContext, &callBackInfo);
	}

	else
	{
		res = pThread->DoAppropriateWait(1,&handle,TRUE,timeout,TRUE /*alertable*/);
	}

    retVal = ((res == WAIT_OBJECT_0) || (res == WAIT_ABANDONED));


    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

FCIMPL4(INT32, WaitHandleNative::CorWaitMultipleNative, Object* waitObjectsUNSAFE, INT32 timeout, BYTE exitContext, BYTE waitForAll)
{
    INT32 retVal;
    OBJECTREF waitObjects = (OBJECTREF) waitObjectsUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(waitObjects);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(waitObjects);

    Thread* pThread = SetupThread();
    _ASSERTE(pThread != NULL);
    _ASSERTE(pThread == GetThread());

    PTRARRAYREF pWaitObjects = (PTRARRAYREF)waitObjects;  // array of objects on which to wait
    int numWaiters = pWaitObjects->GetNumComponents();


    pWaitObjects = (PTRARRAYREF)waitObjects;  // array of objects on which to wait

    HANDLE* internalHandles = (HANDLE*) _alloca(numWaiters*sizeof(HANDLE)); 

    for (int i=0;i<numWaiters;i++)
    {
        WAITHANDLEREF waitObject = (WAITHANDLEREF) (OBJECTREFToObject(pWaitObjects->m_Array[i]));
		if (waitObject == NULL)
			COMPlusThrow(kNullReferenceException);
		
		MethodTable *pMT = waitObject->GetMethodTable();
		if (pMT->IsTransparentProxyType())
			COMPlusThrow(kInvalidOperationException,L"InvalidOperation_WaitOnTransparentProxy");
        internalHandles[i] = waitObject->m_handle;
    }

    DWORD res;
	
	
	Context* targetContext = pThread->GetContext();
	_ASSERTE(targetContext);
	Context* defaultContext = pThread->GetDomain()->GetDefaultContext();
	_ASSERTE(defaultContext);
	if (exitContext != NULL &&
		targetContext != defaultContext)
	{
		Context::WaitArgs waitMultipleArgs = {numWaiters, internalHandles, waitForAll, timeout, TRUE, &res};
		Context::CallBackInfo callBackInfo = {Context::Wait_callback, (void*) &waitMultipleArgs};
		Context::RequestCallBack(defaultContext, &callBackInfo);
	}
	else
	{
		res = pThread->DoAppropriateWait(numWaiters, internalHandles, waitForAll, timeout,TRUE /*alertable*/);
	}

    retVal = res;

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


