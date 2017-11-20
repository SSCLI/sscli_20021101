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
// Date: May, 1999
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "commethodrental.h"
#include "corerror.h"

Stub *MakeJitWorker(MethodDesc *pMD, COR_ILMETHOD_DECODER* ILHeader, BOOL fSecurity, BOOL fGenerateUpdateableStub, MethodTable *pDispatchingMT, OBJECTREF *pThrowable);
void InterLockedReplacePrestub(MethodDesc* pMD, Stub* pStub);


// SwapMethodBody
// This method will take the rgMethod as the new function body for a given method. 
//
FCIMPL6(void, COMMethodRental::SwapMethodBody, ReflectClassBaseObject* clsUNSAFE, INT32 tkMethod, LPVOID rgMethod, INT32 iSize, INT32 flags, StackCrawlMark* stackMark)
{
    REFLECTCLASSBASEREF cls = (REFLECTCLASSBASEREF) clsUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(cls);
    //-[autocvtpro]-------------------------------------------------------

    EEClass*    eeClass;
	BYTE		*pNewCode		= NULL;
	MethodDesc	*pMethodDesc;
	InMemoryModule *module;
    ICeeGen*	pGen;
    ULONG		methodRVA;
	MethodTable	*pMethodTable;
    HRESULT     hr;

    THROWSCOMPLUSEXCEPTION();

	if ( cls == NULL)
    {
        COMPlusThrowArgumentNull(L"cls");
    }

	eeClass	= ((ReflectClass *) cls->GetData())->GetClass();
	module = (InMemoryModule *) eeClass->GetModule();
    pGen = module->GetCeeGen();

	Assembly* caller = SystemDomain::GetCallersAssembly( stackMark );

	_ASSERTE( caller != NULL && "Unable to get calling assembly" );
	_ASSERTE( module->GetCreatingAssembly() != NULL && "InMemoryModule must have a creating assembly to be used with method rental" );

	if (module->GetCreatingAssembly() != caller)
	{
		COMPlusThrow(kSecurityException);
	}

	// Find the methoddesc given the method token
	pMethodDesc = eeClass->FindMethod(tkMethod);
	if (pMethodDesc == NULL)
	{
        COMPlusThrowArgumentException(L"methodtoken", NULL);
	}
    if (pMethodDesc->GetMethodTable()->GetClass() != eeClass)
    {
        COMPlusThrowArgumentException(L"methodtoken", L"Argument_TypeDoesNotContainMethod");
    }
    hr = pGen->AllocateMethodBuffer(iSize, &pNewCode, &methodRVA);    
    if (FAILED(hr))
        COMPlusThrowHR(hr);

	if (pNewCode == NULL)
	{
        COMPlusThrowOM();
	}





	// copy the new function body to the buffer
    memcpy(pNewCode, (void *) rgMethod, iSize);

	// make the descr to point to the new code
	// For in-memory module, it is the blob offset that is stored in the method desc
	pMethodDesc->SetRVA(methodRVA);

    DWORD attrs = pMethodDesc->GetAttrs();
	// do the back patching if it is on vtable
    if (
        (!IsMdRTSpecialName(attrs)) &&
        (!IsMdStatic(attrs)) &&
        (!IsMdPrivate(attrs)) &&
        (!IsMdFinal(attrs)) &&
        !pMethodDesc->GetClass()->IsValueClass())
	{
		pMethodTable = eeClass->GetMethodTable();
		(pMethodTable->GetVtable())[(pMethodDesc)->GetSlot()] = (SLOT)pMethodDesc->GetPreStubAddr();
	}


    if (flags)
    {
        // JITImmediate
        OBJECTREF     pThrowable = NULL;
        Stub *pStub = NULL;
#if _DEBUG
    	COR_ILMETHOD* ilHeader = pMethodDesc->GetILHeader();
        _ASSERTE(((BYTE *)ilHeader) == pNewCode);
#endif
    	COR_ILMETHOD_DECODER header((COR_ILMETHOD *)pNewCode, pMethodDesc->GetMDImport()); 

        // minimum validation on the correctness of method header
        if (header.GetCode() == NULL)
            COMPlusThrowHR(VLDTR_E_MD_BADHEADER);

        GCPROTECT_BEGIN(pThrowable);
        pStub = MakeJitWorker(pMethodDesc, &header, FALSE, FALSE, NULL, &pThrowable);
        if (!pStub)
            COMPlusThrow(pThrowable);

        GCPROTECT_END();
    }

    // add feature::
	// If SQL is generating class with inheritance hierarchy, we may need to
	// check the whole vtable to find duplicate entries.


    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}	// COMMethodRental::SwapMethodBody
FCIMPLEND

