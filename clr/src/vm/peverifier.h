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
/*
 *
 * Header:  PEVerifier.h
 *        
 *
 * Purpose: Verify PE images before loading. This is to prevent
 *          Native code (other than Mscoree.DllMain() ) to execute.
 *
 *
 * The entry point should be the following instruction.
 *
 * [_X86_]
 * jmp dword ptr ds:[XXXX]
 *
 * XXXX should be the RVA of the first entry in the IAT.
 * The IAT should have only one entry, that should be MSCoree.dll:_CorMain
 *
 * Date created : 1 July 1999 
 * 
 */

#ifndef __PEVERIFIER_h__
#define __PEVERIFIER_h__


class PEVerifier
{
public:
    PEVerifier(PBYTE pBase, DWORD dwLength) : 
        m_pBase(pBase), 
        m_dwLength(dwLength), 
        m_pDOSh(NULL), 
        m_pNTh(NULL), 
        m_pFh(NULL), 
        m_pSh(NULL),
        m_dwPrefferedBase(0),
        m_nSections(0),
        m_dwIATRVA(0),
        m_dwRelocRVA(0)
    {
    }

    BOOL Check();

protected:

    PBYTE m_pBase;
    DWORD m_dwLength;

private:

    BOOL CheckDosHeader()      const;
    BOOL CheckNTHeader()       const;
    BOOL CheckFileHeader()     const;
    BOOL CheckOptionalHeader() const;
    BOOL CheckSectionHeader()  const;

    BOOL CheckSection          (DWORD *pOffsetCounter, 
                                DWORD dataOffset,
                                DWORD dataSize, 
                                DWORD *pAddressCounter,
                                DWORD virtualAddress, 
                                DWORD unalignedVirtualSize,
                                int sectionIndex) const;

    BOOL CheckDirectories()    const;
    BOOL CheckImportDlls();    // Sets m_dwIATRVA
    BOOL CheckRelocations();   // Sets m_dwRelocRVA
    BOOL CheckEntryPoint()     const;

    BOOL CheckImportByNameTable(DWORD dwRVA, BOOL fNameTable) const;
	BOOL CheckCOMHeader() const;

    BOOL CompareStringAtRVA(DWORD dwRVA, CHAR *pStr, DWORD dwSize) const;

    DWORD RVAToOffset      (DWORD dwRVA,
                            DWORD *pdwSectionOffset, 
                            DWORD *pdwSectionSize) const;

    DWORD DirectoryToOffset(DWORD dwDirectory, 
                            DWORD *pdwDirectorySize,
                            DWORD *pdwSectionOffset, 
                            DWORD *pdwSectionSize) const;

#ifdef _MODULE_VERIFY_LOG 
    static void LogError(PCHAR szField, DWORD dwActual, DWORD dwExpected);
    static void LogError(PCHAR szField, DWORD dwActual, DWORD *pdwExpected, 
        int n);
#endif

    PIMAGE_DOS_HEADER      m_pDOSh;
    PIMAGE_NT_HEADERS      m_pNTh;
    PIMAGE_FILE_HEADER     m_pFh;
    PIMAGE_OPTIONAL_HEADER m_pOPTh;
    PIMAGE_SECTION_HEADER  m_pSh;

    size_t  m_dwPrefferedBase;
    DWORD  m_nSections;
    DWORD  m_dwIATRVA;
    DWORD  m_dwRelocRVA;
};

#endif  // __PEVERIFIER_h__
