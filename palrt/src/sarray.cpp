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
// File: sarray.cpp
// 
// ===========================================================================
/*++

Abstract:

    PALRT SAFEARRAY support

Revision History:

--*/

#include "rotor_palrt.h"
#include "oautil.h"

STDAPI_(SAFEARRAY *) SafeArrayCreateVector(VARTYPE vt, LONG lLbound, ULONG cElements)
{
    SAFEARRAYBOUND sabound = {cElements, lLbound};

    SAFEARRAY FAR* psa;
    unsigned long  cbSize;
    unsigned long  cbElement;

    _ASSERTE(vt == VT_VARIANT);
    _ASSERTE(lLbound == 0);
    _ASSERTE(cElements < 10);

    cbElement = sizeof(VARIANT) * cElements;
    cbSize = cbElement + sizeof(SAFEARRAY); // add the descriptor

    if( NULL == (psa = (SAFEARRAY FAR*)malloc(cbSize)))
      return NULL;

    // zero out the allocated memory
    memset(psa, 0, cbSize);
    psa = (SAFEARRAY FAR*)((BYTE *)psa);

    psa->pvData = (LPVOID)((BYTE *)psa + sizeof(SAFEARRAY));
    psa->cDims      = 1;
    psa->cbElements = cbElement;
    psa->fFeatures  = FADF_VARIANT;
    psa->rgsabound[0] = sabound;

    return psa;
}

STDAPI SafeArrayPutElement(SAFEARRAY * psa, LONG * rgIndices, void * pv)
{
    void * pvData;
    ULONG ofs;

    _ASSERTE(psa->cDims == 1);
    ofs = rgIndices[0] * psa->cbElements;
    pvData = ((unsigned char*)psa->pvData) + ofs;

    _ASSERTE(psa->fFeatures & FADF_VARIANT);
    _ASSERTE(V_VT((VARIANT*)pv) != VT_DISPATCH);
    _ASSERTE(V_VT((VARIANT*)pv) != VT_UNKNOWN);
    _ASSERTE(V_VT((VARIANT*)pv) != VT_BSTR);
    memcpy(pvData, &pv, sizeof(VARIANT));

    return NOERROR;
}

STDAPI SafeArrayDestroy(SAFEARRAY * psa)
{
    if(psa == NULL)
      return NOERROR;

    free(psa);

    return NOERROR;
}










