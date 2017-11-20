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
// File: controller.h
//
// ===========================================================================

#ifndef __controller_h__
#define __controller_h__

#include "scciface.h"
#include "alloc.h"
#include "options.h"
#include "namemgr.h"
#include "error.h"

class CInputSet;
class ERRLOC;
class COMPILER;

////////////////////////////////////////////////////////////////////////////////
// CController
//
// This is the "controller" for compiler objects. The controller is the object
// that exposes/implements ICSCompiler for external consumption.  Compiler
// options are configured through this object, and for an actual compilation,
// this object instanciates a COMPILER, feeds it the appropriate information,
// tells it to compile, and then destroys it.

class CController :
    public CComObjectRootMT,
    public ICSCompiler,
    public ALLOCHOST
{
private:
    NAMEMGR                     *m_pNameMgr;            // NOTE:  Referenced pointer!
    PAGEHEAP                    m_PageHeap;             // Page heap for this controller.
    MEMHEAP                     m_StdHeap;              // Standard heap for this controller (NOT used by COMPILER, it has its own)
    CComPtr<ICSCompilerHost>    m_spHost;               // Our host
    COptionData                 m_OptionData;           // Our options
    CInputSet                   *m_pInputSets;          // List of input sets
    CInputSet                   **m_ppNextInputSet;     // Next slot in input set list
    CErrorContainer             *m_pCompilerErrors;     // Container for reporting errors
    DWORD                       m_dwFlags;              // Creation flags
    void                        *m_pExceptionAddr;      // Address of exception
    long                        m_iErrorsReported;      // Number of non-warning errors reported to host

    BOOL DisplayWarning(long iErrorIndex, int warnLevel);

public:
    BEGIN_COM_MAP(CController)
        COM_INTERFACE_ENTRY(ICSCompiler)
    END_COM_MAP()

    CController ();
    ~CController ();

    HRESULT     Initialize (DWORD dwFlags, ICSCompilerHost *pHost, ICSNameTable *pNameTable);

    ICSCompilerHost *GetHost () { return m_spHost; }
    NAMEMGR         *GetNameMgr() { return m_pNameMgr; }
    void            RemoveSetFromList (CInputSet *pSet);
    COptionData     *GetOptions () { return &m_OptionData; }
    long            ErrorsReported () { return m_iErrorsReported; }
    void            SetConfiguration (COptionData *pData);
    BOOL            CheckFlags (DWORD dwFlags) { return m_dwFlags & dwFlags; }

    // The following methods comprise the error handling/COMPILER-to-CController hosting
    HRESULT         CreateError (long iErrorIndex, va_list args, CError **ppError, int warnLevelOverride = 0);
    void            SubmitError (CError *pError);
    void            ReportErrorsToHost (ICSErrorContainer *pErrors);

    void            HandleException (DWORD dwException);
    void            SetExceptionData (EXCEPTION_POINTERS *pExceptionInfo) { m_pExceptionAddr = pExceptionInfo->ExceptionRecord->ExceptionAddress; }
    void            *GetExceptionAddress () { return m_pExceptionAddr; }

    // ICSCompiler
    STDMETHOD(CreateSourceModule)(ICSSourceText *pText, ICSSourceModule **ppModule);
    STDMETHOD(GetNameTable)(ICSNameTable **ppNameTable);
    STDMETHOD(Shutdown)();
    STDMETHOD(GetConfiguration)(ICSCompilerConfig **ppConfig);
    STDMETHOD(AddInputSet)(ICSInputSet **ppInputSet);
    STDMETHOD(RemoveInputSet)(ICSInputSet *pInputSet);
    STDMETHOD(Compile)(ICSCompileProgress *pProgress);
    STDMETHOD(GetOutputFileName)(PCWSTR *ppszFileName);
    STDMETHOD(CreateParser)(ICSParser **ppParser);

    // ALLOCHOST
    void        NoMemory ();
    MEMHEAP     *GetStandardHeap ();
    PAGEHEAP    *GetPageHeap ();
};

#endif //__controller_h__

