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
// File: inputset.h
//
// ===========================================================================

#ifndef __inputset_h__
#define __inputset_h__

#include "scciface.h"
#include "table.h"
#include "rotor_palrt.h"

class CController;

////////////////////////////////////////////////////////////////////////////////
// CSourceFileInfo

class CSourceFileInfo
{
public:
    NAME                *m_pName;               // Name of source file (case preserved)
    FILETIME            m_ft;                   // Last-write time
    CSourceFileInfo     *m_pNext;               // Next SourceFile (preserves ordering of files)
};

typedef CAutoDelTable<CSourceFileInfo> CSourceTable;


////////////////////////////////////////////////////////////////////////////////
// CInputSet
//
// Represents an input set for adding sources/resources to compile into a single
// output file

class CInputSet :
    public CComObjectRootMT,
    public ICSInputSet
{
private:
    CController         *m_pController;             // Owning compiler controller
    CComBSTR            m_sbstrOutputName;          // Output file name
    CComBSTR            m_sbstrMainClass;           // Fully Qualified class name to use for Main()
    CSourceTable        m_tableSources;             // Source files
    CSourceFileInfo     *m_pSrcHead;                // Head of Source list, but in reverse order
    CTable<NAME>        m_tableResources;           // Resource files
    unsigned            m_fDLL:1;                   // TRUE if creating a DLL
    unsigned            m_fNoOutput:1;              // TRUE if creating nothing
    unsigned            m_fWinApp:1;                // TRUE if creating a Windows exe
    unsigned            m_fAssemble:1;              // TRUE if creating an assembly
    DWORD               m_dwImageBase;              // Image Base
    DWORD               m_dwFileAlign;              // File Alignment

public:
    BEGIN_COM_MAP(CInputSet)
        COM_INTERFACE_ENTRY(ICSInputSet)
    END_COM_MAP()

    CInputSet ();
    ~CInputSet ();

    HRESULT     Initialize (CController *pController);
    HRESULT     IsSourceFileInInputSet (NAME *pName);

    CInputSet   *m_pNext;

    CSourceTable        *GetSourceTable() { return &m_tableSources; }
    void                CopySources(CSourceFileInfo **ppSrcFiles);
    CTable<NAME>        *GetResourceTable() { return &m_tableResources; }
    PCWSTR              GetOutputName () { return m_fNoOutput ? NULL : (PCWSTR)m_sbstrOutputName; }
    BOOL                DLL () { return m_fDLL; }
    DWORD               ImageBase () { return m_dwImageBase; }
    DWORD               FileAlignment () { return m_dwFileAlign; }
    PCWSTR              GetMainClass () { return m_fDLL ? NULL : (PCWSTR)m_sbstrMainClass; }
    BOOL                WinApp () { return m_fWinApp; }
    BOOL                MakeAssembly() { return m_fAssemble; }

    // ICSInputSet
    STDMETHOD(GetCompiler)(ICSCompiler **ppCompiler);
    STDMETHOD(AddSourceFile)(PCWSTR pszFileName, FILETIME *pFT);
    STDMETHOD(RemoveSourceFile)(PCWSTR pszFileName);
    STDMETHOD(RemoveAllSourceFiles)();
    STDMETHOD(AddResourceFile)(PCWSTR pszFileName, PCWSTR pszIdent, BOOL bEmbed, BOOL bVis);
    STDMETHOD(RemoveResourceFile)(PCWSTR pszFileName, PCWSTR pszIdent, BOOL bEmbed, BOOL bVis);
    STDMETHOD(SetOutputFileName)(PCWSTR pszFileName);
    STDMETHOD(SetOutputFileType)(DWORD dwFileType);
    STDMETHOD(SetImageBase)(DWORD dwImageBase);
    STDMETHOD(SetMainClass)(PCWSTR pszFQClassName);
    STDMETHOD(SetFileAlignment)(DWORD dwAlign);
};

#endif //__inputset_h__

