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
#include "stdafx.h"
#include "wsinfo.h"

WSInfo::WSInfo(IMetaDataImport *pImport)
    : m_total(0),
	m_pImport(pImport)
{
    HCORENUM hEnum = NULL;
    DWORD count;

    IfFailThrow(pImport->EnumTypeDefs(&hEnum, NULL, 0, NULL));
    IfFailThrow(pImport->CountEnum(hEnum, &m_cTypes));
    m_cTypes += 1;

    //

    m_cMethods = 0;
    m_cFields = 0;
    while (TRUE)
    {
        mdTypeDef td;
        IfFailThrow(pImport->EnumTypeDefs(&hEnum, &td, 1, &count));

        if (count == 0)
            break;

        HCORENUM hMethodEnum = NULL;
        IfFailThrow(pImport->EnumMethods(&hMethodEnum, td, NULL, 0, NULL));
        IfFailThrow(pImport->CountEnum(hMethodEnum, &count));
        pImport->CloseEnum(hMethodEnum);

        m_cMethods += count;

        HCORENUM hFieldEnum = NULL;
        IfFailThrow(pImport->EnumFields(&hFieldEnum, td, NULL, 0, NULL));
        IfFailThrow(pImport->CountEnum(hFieldEnum, &count));
        pImport->CloseEnum(hFieldEnum);

        m_cFields += count;
    }

    pImport->CloseEnum(hEnum);

    HCORENUM hMethodEnum = NULL;
    IfFailThrow(pImport->EnumMethods(&hMethodEnum, mdTypeDefNil, NULL, 0, NULL));
    IfFailThrow(pImport->CountEnum(hMethodEnum, &count));
    pImport->CloseEnum(hMethodEnum);

    m_cMethods += count;

    HCORENUM hFieldEnum = NULL;
    IfFailThrow(pImport->EnumFields(&hFieldEnum, mdTypeDefNil, NULL, 0, NULL));
    IfFailThrow(pImport->CountEnum(hFieldEnum, &count));
    pImport->CloseEnum(hFieldEnum);

    m_cFields += count;

    //
    // Now allocate arrays
    //

    m_pTypeSizes = new ULONG [ m_cTypes+1 ];
    if (m_pTypeSizes == NULL)
        ThrowHR(E_OUTOFMEMORY);
    ZeroMemory(m_pTypeSizes, sizeof(ULONG) * (m_cTypes+1));

    m_pMethodSizes = new ULONG [ m_cMethods+1 ];
    if (m_pMethodSizes == NULL)
        ThrowHR(E_OUTOFMEMORY);
    ZeroMemory(m_pMethodSizes, sizeof(ULONG) * (m_cMethods+1));

    m_pFieldSizes = new ULONG [ m_cFields+1 ];
    if (m_pFieldSizes == NULL)
        ThrowHR(E_OUTOFMEMORY);
    ZeroMemory(m_pFieldSizes, sizeof(ULONG) * (m_cFields+1));
}

WSInfo::~WSInfo()
{
    if (m_pImport != NULL)
        m_pImport->Release();

    if (m_pTypeSizes != NULL)
        delete [] m_pTypeSizes;

    if (m_pMethodSizes != NULL)
        delete [] m_pMethodSizes;

    if (m_pFieldSizes != NULL)
        delete [] m_pFieldSizes;
}

void WSInfo::AdjustAllTypeSizes(LONG size)
{
    LONG delta = size / (m_cTypes-1);
    
    ULONG *p = m_pTypeSizes;
    ULONG *pEnd = p + m_cTypes;

    while (++p < pEnd)
        *p += delta;
}

void WSInfo::AdjustTypeSize(mdTypeDef token, LONG size)
{
    if (IsNilToken(token))
        AdjustAllTypeSizes(size);
    else
	{
		_ASSERTE(m_pImport->IsValidToken(token));
		m_pTypeSizes[RidFromToken(token)] += size;
	}
}

void WSInfo::AdjustAllMethodSizes(LONG size)
{
    LONG delta = size / (m_cMethods-1);
    
    ULONG *p = m_pMethodSizes;
    ULONG *pEnd = p + m_cMethods;

    while (++p < pEnd)
        *p += delta;
}

void WSInfo::AdjustMethodSize(mdMethodDef token, LONG size)
{
    if (IsNilToken(token))
        AdjustAllMethodSizes(size);
    else
    {
        _ASSERTE(m_pImport->IsValidToken(token));
        m_pMethodSizes[RidFromToken(token)] += size;
    }
}

void WSInfo::AdjustAllFieldSizes(LONG size)
{
    LONG delta = size / (m_cFields-1);
    
    ULONG *p = m_pFieldSizes;
    ULONG *pEnd = p + m_cFields;

    while (++p < pEnd)
        *p += delta;
}

void WSInfo::AdjustFieldSize(mdFieldDef token, LONG size)
{
    if (IsNilToken(token))
        AdjustAllFieldSizes(size);
    else
    {
        _ASSERTE(m_pImport->IsValidToken(token));
        m_pFieldSizes[RidFromToken(token)] += size;
    }
}

void WSInfo::AdjustTokenSize(mdToken token, LONG size)
{
    switch (TypeFromToken(token))
    {
    case mdtTypeDef:
        AdjustTypeSize(token, size);
        return;

    case mdtFieldDef:
        AdjustFieldSize(token, size);
        return;

    case mdtMethodDef:
        AdjustMethodSize(token, size);
        return;
        
    default:
        break;
    }
}

ULONG WSInfo::GetTotalAttributedSize()
{
    ULONG size = 0;

    ULONG *p, *pEnd;

    p = m_pTypeSizes;
    pEnd = p + m_cTypes;
    while (p < pEnd)
        size += *p++;

    p = m_pMethodSizes;
    pEnd = p + m_cMethods;
    while (p < pEnd)
        size += *p++;

    p = m_pFieldSizes;
    pEnd = p + m_cFields;
    while (p < pEnd)
        size += *p++;

    return size;
}
