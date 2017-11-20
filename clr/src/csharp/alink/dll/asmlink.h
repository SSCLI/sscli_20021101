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
// File: asmlink.h
//
// ===========================================================================

#ifndef __asmlink_h__
#define __asmlink_h__

#include "alink.h"
#include <ole2.h>
#include "assembly.h"

////////////////////////////////////////////////////////////////////////////////
// CAsmLink
//
// This class is the real assembly linker.

class CAsmLink :
    public CComObjectRoot,
    public ISupportErrorInfoImpl< &IID_IALink>,
    public IALink
{
public:
    BEGIN_COM_MAP(CAsmLink)
        COM_INTERFACE_ENTRY(IALink)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP()

    CAsmLink();
    ~CAsmLink();

    // Interface methods here
    STDMETHOD(Init)(IMetaDataDispenserEx *pDispenser, IMetaDataError *pError);
    STDMETHOD(SetAssemblyFile)(LPCWSTR pszFilename, IMetaDataEmit *pEmitter, AssemblyFlags afFlags, mdAssembly *pAssemblyID);
    STDMETHOD(SetNonAssemblyFlags)(AssemblyFlags afFlags); 
    

    // Files and importing
    STDMETHOD(ImportFile)(LPCWSTR pszFilename, LPCWSTR pszTargetName, BOOL fSmartImport,
        mdToken *pImportToken, IMetaDataAssemblyImport **ppAssemblyScope, DWORD *pdwCountOfScopes);
    STDMETHOD(ImportFile2)(LPCWSTR pszFilename, LPCWSTR pszTargetName, IMetaDataAssemblyImport *pAssemblyScopeIn, BOOL fSmartImport,
        mdToken *pImportToken, IMetaDataAssemblyImport **ppAssemblyScope, DWORD *pdwCountOfScopes);
    STDMETHOD(AddFile)(mdAssembly AssemblyID, LPCWSTR pszFilename, DWORD dwFlags,
        IMetaDataEmit *pEmitter, mdFile * pFileToken);
    STDMETHOD(AddImport)(mdAssembly mdAssemblyID, mdToken ImportToken, DWORD dwFlags, mdFile *pFileToken);
    STDMETHOD(GetScope)(mdAssembly AssemblyID, mdToken FileToken, DWORD dwScope, IMetaDataImport** ppImportScope);
    STDMETHOD(GetAssemblyRefHash)(mdToken FileToken, const void** ppvHash, DWORD* pcbHash);

    STDMETHOD(ImportTypes)(mdAssembly AssemblyID, mdToken FileToken, DWORD dwScope, HALINKENUM* phEnum,
        IMetaDataImport **ppImportScope, DWORD* pdwCountOfTypes);
    STDMETHOD(EnumCustomAttributes)(HALINKENUM hEnum, mdToken tkType, mdCustomAttribute rCustomValues[],
        ULONG cMax, ULONG *pcCustomValues);
    STDMETHOD(EnumImportTypes)(HALINKENUM hEnum, DWORD dwMax, mdTypeDef aTypeDefs[], DWORD* pdwCount); 
    STDMETHOD(CloseEnum)(HALINKENUM hEnum);
    
    // Exporting
    STDMETHOD(ExportType)(mdAssembly AssemblyID, mdToken FileToken, mdTypeDef TypeToken,
        LPCWSTR pszTypename, DWORD dwFlags, mdExportedType* pType); 
    STDMETHOD(ExportNestedType)(mdAssembly AssemblyID, mdToken FileToken, mdTypeDef TypeToken, 
        mdExportedType ParentType, LPCWSTR pszTypename, DWORD dwFlags, mdExportedType* pType); 
    
    // Resources
    STDMETHOD(EmbedResource)(mdAssembly AssemblyID, mdToken FileToken, LPCWSTR pszResourceName, DWORD dwOffset, DWORD dwFlags); 
    STDMETHOD(LinkResource)(mdAssembly AssemblyID, LPCWSTR pszFileName, LPCWSTR pszNewLocation, LPCWSTR pszResourceName, DWORD dwFlags); 
    
    STDMETHOD(GetResolutionScope)(mdAssembly AssemblyID, mdToken FileToken, mdToken TargetFile, mdToken* pScope); 

    
    // Custom Attributes and Assembly properties
    STDMETHOD(SetAssemblyProps)(mdAssembly AssemblyID, mdToken FileToken, AssemblyOptions Option, VARIANT Value); 
    STDMETHOD(EmitAssemblyCustomAttribute)(mdAssembly AssemblyID, mdToken FileToken, mdToken tkType, 
        void const* pCustomValue, DWORD cbCustomValue, BOOL bSecurity, BOOL bAllowMultiple); 
    STDMETHOD(EndMerge)(mdAssembly AssemblyID);
    STDMETHOD(EmitManifest)(mdAssembly AssemblyID, DWORD* pdwReserveSize, mdAssembly* ptkManifest);

    
    // Finish everything off
    STDMETHOD(PreCloseAssembly)(mdAssembly AssemblyID);
    STDMETHOD(CloseAssembly)(mdAssembly AssemblyID); 

private:
    HRESULT FileNameTooLong(LPCWSTR filename);

    // our implementation
    IMetaDataDispenserEx *m_pDisp;
    IMetaDataError *m_pError;
    CAssemblyFile *m_pAssem;
    CAssemblyFile *m_pImports;
    CAssemblyFile *m_pModules;
    CAssemblyFile *m_pStdLib;
    bool    m_bInited           : 1;
    bool    m_bPreClosed        : 1;
    bool    m_bManifestEmitted  : 1;
    bool    m_bDontDoHashes     : 1;

protected:
    friend class CFile;
    CAssemblyFile *GetStdLib();

};

#endif // __asmlink_h__
