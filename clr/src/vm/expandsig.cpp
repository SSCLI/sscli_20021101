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
#include "expandsig.h"
#include "comclass.h"

// ExpandSig
// Constructor
ExpandSig::ExpandSig(PCCOR_SIGNATURE sig, Module* pModule) : m_MetaSig(sig, pModule)
{
}

ExpandSig* ExpandSig::GetReflectSig(PCCOR_SIGNATURE sig, Module* pModule)
{
    THROWSCOMPLUSEXCEPTION();

	int nArgs;
	MetaSig msig(sig, pModule);
    BYTE callConv = msig.GetCallingConvention();

	if (callConv == IMAGE_CEE_CS_CALLCONV_FIELD)
		nArgs = 0;
	else
	    nArgs = msig.NumFixedArgs();

    TypeHandle* b = (TypeHandle*) _alloca((2 + nArgs) * sizeof(TypeHandle));
	ZeroMemory(b,(2 + nArgs) * sizeof(TypeHandle));
	*((DWORD*) b) = (DWORD) callConv;

    OBJECTREF pThrowable = NULL;
    GCPROTECT_BEGIN(pThrowable);
    {
	    b[1] = msig.GetRetTypeHandle(&pThrowable);
        if (b[1].IsNull())
            COMPlusThrow(pThrowable);
	    msig.Reset();
	    for (int i=0;i<nArgs;i++) {
            msig.NextArg();
		    b[i+ARG_OFFSET] = msig.GetTypeHandle(&pThrowable);
            if (b[i+ARG_OFFSET].IsNull())
                COMPlusThrow(pThrowable);
	    }
    }
    GCPROTECT_END();

    void *retSig = pModule->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ExpandSig) + ((1 + nArgs) * sizeof(TypeHandle)));
    if (!retSig)
        COMPlusThrowOM();

	// Set the flags...

    ExpandSig* pSig = new (retSig) ExpandSig(sig,pModule);
    _ASSERTE(pSig != NULL);

    memcpy(pSig->m_Data,b,(2 + nArgs) * sizeof(TypeHandle));
	pSig->m_flags = 0;
	if (msig.HasRetBuffArg())
		pSig->m_flags |= VALUE_RETBUF_ARG;
    return pSig;
}

// Similar to GetReflectSig, but uses the static heap for allocation and doesn't
// throw an exception on allocation failure or on type load failures.
ExpandSig* ExpandSig::GetSig(PCCOR_SIGNATURE sig, Module* pModule)
{
    int nArgs;
    MetaSig msig(sig, pModule);
    BYTE callConv = msig.GetCallingConvention();

    if (callConv == IMAGE_CEE_CS_CALLCONV_FIELD)
        nArgs = 0;
    else
        nArgs = msig.NumFixedArgs();

    TypeHandle* b = (TypeHandle*) _alloca((2 + nArgs) * sizeof(TypeHandle));
    ZeroMemory(b,(2 + nArgs) * sizeof(TypeHandle));
    *((DWORD*) b) = (DWORD) callConv;
    b[1] = msig.GetRetTypeHandle();
    msig.Reset();
    for (int i=0;i<nArgs;i++) {
        msig.NextArg();
        b[i+ARG_OFFSET] = msig.GetTypeHandle();
    }

    void *retSig = new BYTE[sizeof(ExpandSig) + ((1 + nArgs) * sizeof(TypeHandle))];
    if (!retSig)
        return NULL;

    ExpandSig* pSig = new (retSig) ExpandSig(sig,pModule);

    memcpy(pSig->m_Data,b,(2 + nArgs) * sizeof(TypeHandle));
    pSig->m_flags = HEAP_ALLOCATED;
    if (msig.HasRetBuffArg())
        pSig->m_flags |= VALUE_RETBUF_ARG;

    return pSig;
}

BOOL ExpandSig::IsEquivalent(ExpandSig *pOther)
{
    ULONG uCallConv = m_MetaSig.GetCallingConvention();
    if (uCallConv != pOther->m_MetaSig.GetCallingConvention())
        return FALSE;

    if (m_Data[RETURN_TYPE_OFFSET] != pOther->m_Data[RETURN_TYPE_OFFSET])
        return FALSE;

    if (uCallConv == IMAGE_CEE_CS_CALLCONV_FIELD)
        return TRUE;

    ULONG cArgs = m_MetaSig.NumFixedArgs();
    for (ULONG i = ARG_OFFSET; i < (ARG_OFFSET + cArgs); i++)
        if (m_Data[i] != pOther->m_Data[i])
            return FALSE;

    return TRUE;
}

EEClass* ExpandSig::GetReturnValueClass() 
{
	_ASSERTE(m_Data[RETURN_TYPE_OFFSET].IsUnsharedMT());
	return m_Data[RETURN_TYPE_OFFSET].AsClass();
}

EEClass* ExpandSig::GetReturnClass()
{
	_ASSERTE(m_Data[RETURN_TYPE_OFFSET].IsUnsharedMT());
	return m_Data[RETURN_TYPE_OFFSET].AsClass();
}

TypeHandle ExpandSig::NextArgExpanded(void** pEnum)
{
	TypeHandle t = AdvanceEnum(pEnum);
	return t;
}


TypeHandle ExpandSig::AdvanceEnum(void** pEnum)
{
    int i = *((int*) pEnum);

    if (i == 0)
        i=ARG_OFFSET;

    *((int*) pEnum) = i+1;

    return m_Data[i];
}


UINT ExpandSig::GetElemSize(TypeHandle th)
{    
    CorElementType type = th.GetNormCorElementType();
    _ASSERTE(type >= 0 && type < ELEMENT_TYPE_MAX);
    UINT ret = gElementTypeInfo[type].m_cbSize;
    if (ret != (UINT) -1)
        return ret;

    return th.GetClass()->GetNumInstanceFieldBytes();
}

DWORD ExpandSig::Hash()
{
    DWORD dwHash = m_MetaSig.GetCallingConvention();

    dwHash ^= UnsafePointerTruncate(m_Data[RETURN_TYPE_OFFSET].AsPtr());

    if (m_MetaSig.GetCallingConvention() == IMAGE_CEE_CS_CALLCONV_FIELD)
        return dwHash;

    ULONG cArgs = m_MetaSig.NumFixedArgs();
    for (ULONG i = ARG_OFFSET; i < (ARG_OFFSET + cArgs); i++)
        dwHash ^= UnsafePointerTruncate(m_Data[i].AsPtr());

    return dwHash;
}
