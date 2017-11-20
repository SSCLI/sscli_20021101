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
// File: options.h
//
// ===========================================================================

#ifndef __options_h__
#define __options_h__

#include "scciface.h"

/*
////////////////////////////////////////////////////////////////////////////////
// Compiler options -- this is the structure that COMPILER and company uses.
// IT WOULD BE NICE to use the same structure that the controller and config
// objects use...

struct OPTIONS
{
    bool runInternalTests   :1; // test code only: Run the internal tests?
    bool dumpParseTrees     :1; // test code only: Dump parse trees?'
    bool dumpSymbols        :1; // test code only: dump the symbols in the symbol table?
    bool fullyImportAll     :1; // test code only: fully import all classes in imported metadata?
    bool testEmitter        :1; // test code only: run emitter test
    bool testInvalid        :1; // test code only: were all /test options recognized?
    bool noStdLibs          :1; // Don't import standard libraries.
    bool makeDLL            :1; // Create a DLL instead of an EXE.
    bool warnAsErrors       :1; // Treat first warning as an error.
    bool emitDebugInfo      :1; // emit debug information
    bool optUnusedVars      :1; // optimize away unused locals
    bool emitAssembly       :1; //                                                             

    int  warnLevel;             // Warning level (0 = no warnings).

    //                                                               
    void    ResetToDefaults () { ZeroMemory (this, sizeof (OPTIONS)); warnLevel = 4; optUnusedVars = 1; emitAssembly = 0; }
};
*/

////////////////////////////////////////////////////////////////////////////////
// Struct used for the static options table

struct OPTIONDEF
{
    PCWSTR      pszShortSwitch;     // Short Name of switch
    PCWSTR      pszLongSwitch;      // Long Name of switch
    PCWSTR      pszDescSwitch;      // Descriptive Name of switch
    long        iOptionID;          // Option ID
    size_t      iOffset;            // Offset of COptionData member holding value
    long        iDescID;            // Description string (res id)
    BSTR        bstrDesc;           // Description (loaded from resource)
    DWORD       dwFlags;            // Option flags
};

////////////////////////////////////////////////////////////////////////////////
// COptionData
//
// This is the table-driven options structure using by the controller and the
// config objects.  Each member is referred to by the option table, and changed
// and/or accessed via member pointers.
//
// Note: the data in this structure is copied and assigned around. Members should not
// point to allocated data unless appropriate copy constructors/assignment operators exist,
// such as for CComBSTR.

class COptionData
{
public:
    // This is the option table.  It's static since it doesn't change per
    // instance of configurations or options objects.
    static  OPTIONDEF   m_rgOptionTable[];
    static  long        m_rgiOptionIndexMap[];
    static  BOOL        m_fMapCreated;
    static  void        CreateIndexMap ();

    // Accessors to static option table
    static  void        InitOptionTable();
    static  OPTIONDEF  *GetOptionDef (long i) { return m_rgOptionTable + i; }
    static  long        GetOptionIndex (long iOptionID) { if (!m_fMapCreated) CreateIndexMap(); return m_rgiOptionIndexMap[iOptionID]; }

    // This defines all of the member variables that correspond to options.
    // Boolean options are named "m_f<id-base>" -- for example, m_fNOSTDLIB
    // String options are named "m_sbstr<id-base>" -- such as m_sbstrCCSYMBOLS
    #define BOOLOPTDEF(id,descid,sh,lg,des,f) BOOL m_f##id;
    #define STROPTDEF(id,descid,sh,lg,des,f) CComBSTR m_sbstr##id;
    #include "optdef.h"

    BOOL m_fNOCODEGEN;  // obsolete option (always FALSE), but code still references is.

    // Here are some mappings for some of the special-cased options
    // that are parsed upon commit (like warning level)
    int  warnLevel;
    bool pdbOnly;

    // List of warnings that are being suppressed.
    // N.B.: This is a BSTR, but is not text -- each character is a warning number!
    // We use CComBSTR because it handles memory allocation issues cleanly and robustly.
    CComBSTR noWarnNumbers;

    #define HACKBOOLOPT(var, text) bool var :1;
    #define HACKSTROPT(var, text) CComBSTR var;
    #include "hacks.h"

    CComBSTR unknownTestSwitch; // test code only: set if bogus switch given to -test:xxx

    COptionData () { InitOptionTable(); ResetToDefaults(); }

    void    ResetToDefaults ();
};


class CController;

////////////////////////////////////////////////////////////////////////////////
// CCompilerConfig
//
// Implementation of ISCCompilerConfig

class CCompilerConfig :
    public CComObjectRootMT,
    public ICSCompilerConfig
{
private:
    CController     *m_pController;         // NOTE:  Addref'd!
    COptionData     *m_pData;               // Our option data

    HRESULT __cdecl ReportOptionError(ICSError **ppError, long errorId, ...);

public:
    BEGIN_COM_MAP(CCompilerConfig)
        COM_INTERFACE_ENTRY(ICSCompilerConfig)
    END_COM_MAP()

    CCompilerConfig();
    ~CCompilerConfig();

    HRESULT     Initialize (CController *pController, COptionData *pData);

    // ICSCompilerConfig
    STDMETHOD(GetOptionCount)(long *piCount);
    STDMETHOD(GetOptionInfoAt)(long iIndex, long *piOptionID, PCWSTR *ppszSwitchName, PCWSTR *ppszDescription, DWORD *pdwFlags);
    STDMETHOD(GetOptionInfoAtEx)(long iIndex, long *piOptionID, PCWSTR *ppszShortSwitchName, PCWSTR *ppszLongSwitchName, PCWSTR *ppszDescSwitchName, PCWSTR *ppszDescription, DWORD *pdwFlags);
    STDMETHOD(ResetAllOptions)();
    STDMETHOD(SetOption)(long iOptionID, VARIANT vtValue);
    STDMETHOD(GetOption)(long iOptionID, VARIANT *pvtValue);
    STDMETHOD(CommitChanges)(ICSError **ppError);
    STDMETHOD(GetCompiler)(ICSCompiler **ppCompiler);
};

#endif //__options_h__

