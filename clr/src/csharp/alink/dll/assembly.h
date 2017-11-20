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
// File: assembly.h
//
// This is the real work horse
// ===========================================================================

#ifndef __assembly_h__
#define __assembly_h__

#include "array.h"
#include "msgsids.h"

#define MODULE_CA_LOCATION L"System.Runtime.CompilerServices.AssemblyAttributesGoHere"
#undef CopyFile
#define PLATFORMID_NAME L"System.PlatformID"

class CAssemblyFile;
class CFile;
class CAsmLink;

struct TypeEnumerator {
    CFile       *file;
    DWORD       TypeID;
};

struct Type {
    mdTypeDef       TypeDef;
    mdExportedType  ComType;
    static int __cdecl CompareTypeDefs(const Type* t1, const Type* t2) {
        return t1->TypeDef - t2->TypeDef;
    }
};

struct CA {
    mdCustomAttribute tkCA;
    mdToken tkType;
    BYTE *  pVal;
    DWORD   cbVal;
    bool    bAllowMulti : 1;
    bool    bSecurity   : 1;

    static ptrdiff_t __cdecl CompareToken( const void * _t1, const void * _t2) {
        mdToken t1 = ((CA*)_t1)->tkType;
        mdToken t2 = ((CA*)_t2)->tkType;
        return t1 - t2;
    }

    static int __cdecl CompareVal( const void * _t1, const void * _t2) {
        CA *t1 = (CA*)_t1;
        CA *t2 = (CA*)_t2;
        return (t1->tkType == t2->tkType && t1->cbVal == t2->cbVal) ? memcmp(t1->pVal, t2->pVal, t1->cbVal) : 1;
    }
};

struct _OptionCA {
    LPCWSTR         name;
    AssemblyOptions option;
    int             optName; // Friendly name resource ID
    VARTYPE         vt;
    BYTE            flag;
};

#define ERRORDEF(num, sev, name, id) name,
enum ERRORS {
#include "errors.h"
};

const extern _OptionCA OptionCAs[];
int FindOption(LPCWSTR CustAttrName);
LPCWSTR ErrorOptionName(AssemblyOptions opt);

typedef HRESULT (__stdcall *EnumMemberFunc)(IMetaDataImport *, HCORENUM *, mdString[], ULONG, ULONG*);
typedef HRESULT (__stdcall *EnumMemberNestedFunc)(IMetaDataImport *, HCORENUM*, mdToken, mdToken[], ULONG, ULONG*);
typedef HRESULT (__stdcall *EnumMemberNested2Func)(IMetaDataImport *, HCORENUM*, mdToken, mdToken, mdToken[], ULONG, ULONG*);

class TokenEnumFunc {
public:
    virtual HRESULT DoSomething(mdToken tkToken) { return S_OK; };
    virtual HRESULT DoSomethignNested(mdToken tkParent, mdToken tkChild) { return S_OK; }
};

class MarkToken : public TokenEnumFunc {
private:
    CComPtr<IMetaDataFilter> m_pFilter;
public:
    MarkToken(IMetaDataFilter *pFilter) : TokenEnumFunc() { m_pFilter = pFilter; }
    virtual HRESULT DoSomething(mdToken tkToken) { return m_pFilter->MarkToken(tkToken);};
    virtual HRESULT DoSomethignNested(mdToken tkParent, mdToken tkChild) { return m_pFilter->MarkToken(tkChild); }
};

class DeleteToken : public TokenEnumFunc {
private:
    CComPtr<IMetaDataEmit> m_pEmit;
public:
    DeleteToken(IMetaDataEmit *pEmit) : TokenEnumFunc() { m_pEmit = pEmit; }
    virtual HRESULT DoSomething(mdToken tkToken) { return m_pEmit->DeleteToken(tkToken);};
    virtual HRESULT DoSomethignNested(mdToken tkParent, mdToken tkChild) { return m_pEmit->DeleteToken(tkChild); }
};

class CFile {
public:
    CFile(LPCWSTR pszFilename, IMetaDataEmit* pEmitter, IMetaDataError *pError, CAsmLink* pLinker);
    CFile(LPCWSTR pszFilename, IMetaDataImport* pImporter, IMetaDataError *pError, CAsmLink* pLinker);
    CFile(IMetaDataImport* pImporter, IMetaDataError *pError, CAsmLink* pLinker);
    virtual ~CFile();

    HRESULT ImportFile(DWORD *pdwCountOfTypes, CAssemblyFile* pAEmit);
    HRESULT ImportResources(CAssemblyFile* pAEmit);
    HRESULT ImportCAs(CAssemblyFile* pAEmit);
    HRESULT GetType(DWORD index, mdTypeDef* pType);
    HRESULT GetComType(DWORD index, mdExportedType* pType);
    HRESULT GetCA(DWORD index, mdToken filter, mdCustomAttribute* pCA);
    HRESULT GetCAName(mdToken tkCA, LPWSTR *ppszName);
    HRESULT MakeModuleRef(IMetaDataEmit *pEmit, mdModuleRef *pmr); // puts a moduleref to this file in the emit scope
    virtual HRESULT AddResource(mdToken Location, LPCWSTR pszName, DWORD offset, DWORD flags);
    virtual HRESULT AddCustomAttribute(mdToken tkType, const void* pValue, DWORD cbValue, BOOL bSecurity, BOOL bAllowMultiple, CFile *source);
    virtual void PreClose();

    IMetaDataEmit* GetEmitScope() { return m_pEmit; }
    IMetaDataImport* GetImportScope() { return m_pImport; }
    DWORD CountTypes() { return m_Types.Count(); }
    DWORD CountCAs() { return m_CAs.Count(); }
    mdFile GetFileToken() { return m_tkFile; }
    HRESULT SetSource(LPCWSTR pszSourceFile);
    HRESULT CopyFile(IMetaDataDispenserEx *pDisp, bool bCleanup);
    inline bool IsStdLib() { return m_isStdLib; }
    inline bool IsUBM() { return m_isUBM; }
    LPCWSTR GetModuleName();
    HRESULT UpdateModuleName();
    HRESULT GetStdLibRef(mdAssemblyRef * tkRef);
    HRESULT GetTypeRef(LPCWSTR wszTypeName, mdToken * tkType); // This may return a TypeDef token...

    static int __cdecl CompareFiles(const CFile** f1, const CFile** f2) {
        return (int)RidFromToken((*f1)->m_tkFile) - (int)RidFromToken((*f2)->m_tkFile);
    }

    mdToken             m_tkError;      // Token to use for error reporting

    // Error reporting
    HRESULT ReportError(HRESULT hr, mdToken tkError = mdTokenNil, bool * pbReport = NULL);
    HRESULT ReportError(ERRORS err, mdToken tkError = mdTokenNil, bool * pbReport = NULL, ...);
    LPCWSTR ErrorHR(HRESULT hr, IUnknown *pUnk, REFIID iid);
    LPCWSTR ErrorHR(HRESULT hr);
    BSTR    ErrorMessage(DWORD err_id, va_list args);
    LPWSTR  UnknownError(LPWSTR buffer, size_t cchMax, HRESULT hr);

protected:
    friend class CAssemblyFile;
    void    ZeroInitFile();
    HRESULT DupModuleRef(mdToken *tkModule, IMetaDataImport* pImport);
    HRESULT FindStdLibRef(mdAssemblyRef * tkRef);
    HRESULT ReadCAs(mdToken Parent, CAssemblyFile *pAEmit, BOOL bMulti, BOOL bSecurity);
    HRESULT EnumAllExcept(IMetaDataImport * pImport, TokenEnumFunc * pFunc, EnumMemberFunc enumfunc, mdToken *tkExcept, size_t cExceptCount, EnumMemberNestedFunc enumnestedfunc);
    HRESULT EnumAllExcept(IMetaDataImport * pImport, TokenEnumFunc * pFunc, mdToken tkParent, mdToken *tkExcept, size_t cExceptCount, EnumMemberNestedFunc enumnestedfunc, EnumMemberNested2Func enumnestedfunc2 = NULL);
    LPCWSTR             m_Name;
    LPCWSTR             m_Path;
    LPCWSTR             m_SrcFile;
    LPCWSTR             m_modName;
    mdFile              m_tkFile;
    mdTypeRef           m_tkAssemblyAttrib[2][2];
    mdAssemblyRef       m_tkMscorlibRef;
    
    // State info
    bool                m_isAssembly    : 1;
    bool                m_isStdLib      : 1;
    bool                m_isUBM         : 1;
    bool                m_isImport      : 1;
    bool                m_isDeclared    : 1;    // True when m_isImport and Embedded resources/ComTypes have been declared in the assembly
    bool                m_isCTDeclared  : 1;    // True when ComTypes are declared
    bool                m_bIsExe        : 1;

    IMetaDataEmit*      m_pEmit;
    IMetaDataImport*    m_pImport;
    IMetaDataError*     m_pError;
    CAsmLink*           m_pLinker;

    CStructArray<Type>  m_Types;
    CStructArray<CA>    m_CAs;
};

class CAssemblyFile : public CFile {
public:
    CAssemblyFile(LPCWSTR pszFilename, AssemblyFlags afFlags, IMetaDataAssemblyEmit* pAEmitter, IMetaDataEmit* pEmitter, IMetaDataError *pError, CAsmLink* pLinker);
    CAssemblyFile(LPCWSTR pszPath, IMetaDataAssemblyImport* pAImporter, IMetaDataImport* pImporter, IMetaDataError *pError, CAsmLink* pLinker);
    CAssemblyFile(IMetaDataError *pError, CAsmLink* pLinker);
    virtual ~CAssemblyFile();

    HRESULT AddFile(CFile *file, DWORD dwFlags, mdFile * tkFile);
    HRESULT AddFile(CAssemblyFile *file, DWORD dwFlags, mdFile * tkFile);
    HRESULT GetFile(DWORD index, CFile** file);
    HRESULT RemoveFile(DWORD index);
    HRESULT ImportAssembly(DWORD *pdwCountOfScopes, BOOL fImportTypes, IMetaDataDispenserEx* pDisp);
    HRESULT AddExportType(mdToken tkParent, mdTypeDef tkTypeDef, LPCWSTR pszTypeName, DWORD flags, mdExportedType *pType);
    virtual HRESULT AddResource(mdToken Location, LPCWSTR pszName, DWORD offset, DWORD flags);
    virtual HRESULT AddCustomAttribute(mdToken tkType, const void* pValue, DWORD cbValue, BOOL bSecurity, BOOL bAllowMultiple, CFile *source);
    HRESULT MakeRes(CFile *file, BOOL fDll, LPCWSTR pszIconFile, const void **ppBlob, DWORD *pcbBlob); // Makes a Win32 resource
    HRESULT SetOption(AssemblyOptions option, VARIANT *pvalue);
    HRESULT EmitManifest(DWORD *pdwReserveSize, mdAssembly *ptkAssembly);
    HRESULT EmitAssembly(IMetaDataDispenserEx *pDisp);
    HRESULT SignAssembly();
    virtual void PreClose();

    HRESULT MakeAssemblyRef(IMetaDataAssemblyEmit *pEmit, mdAssemblyRef *par); // puts an assemblyref to this file in the emit scope
    HRESULT MakeAssemblyRef(IMetaDataEmit *pEmit, mdAssemblyRef *par); // puts an assemblyref to this file in the emit scope
    HRESULT GetHash(const void ** ppvHash, DWORD *pcbHash);

    bool    IsInMemory() { return m_bIsInMemory; }
    DWORD   CountFiles() { return m_Files.Count(); }
    void    DontDoHash() { m_bDoHash = false; }
    IMetaDataAssemblyImport* GetImporter() { return m_pAImport; };
    IMetaDataAssemblyEmit* GetEmitter() { return m_pAEmit; };

protected:
    void ZeroInitAssembly();
    HRESULT ReadCryptoKey(PublicKeyBlob **ppPubKey, DWORD &dwLen);
    HRESULT ReadCryptoFile();
    HRESULT CloseCryptoFile();
    HRESULT MakeVersion(WORD *rev, WORD *build); // NOTE: rev is optional (may be NULL), but build it NOT
    HRESULT ConvertCAToken(mdToken *tkCA, CFile *src);
    HRESULT CopyType(mdToken *tkType, CFile *src);
    HRESULT DupAssemblyRef(mdToken *tkAR, IMetaDataAssemblyImport* pImport);
    LPCWSTR GetAsmName();
    BSTR    ExtractString(const void *pvData, DWORD *cbData);

    // Keep all the assembly info
    LPCWSTR             m_szAsmName;
    ASSEMBLYMETADATA    m_adata;
    LPCWSTR             m_Title;
    LPCWSTR             m_Description;
    LPCWSTR             m_Company;
    LPCWSTR             m_Product;
    LPCWSTR             m_ProductVersion;
    LPCWSTR             m_Copyright;
    LPCWSTR             m_Trademark;
    LPCWSTR             m_KeyName;
    LPCWSTR             m_KeyFile;
    ALG_ID              m_iAlgID;
    DWORD               m_dwFlags;

    const BYTE *        m_pbHash;
    DWORD               m_cbHash;

    bool                m_doHalfSign    : 1;
    
    // State info
    bool                m_isVersionSet  : 1;
    bool                m_isAlgIdSet    : 1;
    bool                m_isFlagsSet    : 1;
    bool                m_isHalfSignSet : 1;
    bool                m_isAutoKeyName : 1;
    bool                m_bAllowExes    : 1;
    bool                m_bIsInMemory   : 1;
    bool                m_bClean        : 1;
    bool                m_bDoHash       : 1;

    IMetaDataAssemblyEmit*
                    m_pAEmit;
    IMetaDataAssemblyImport*
                    m_pAImport;

    CSimpleArray<CFile> m_Files;

    PBYTE               m_pbKeyPair;
    DWORD               m_cbKeyPair;
    PublicKeyBlob *     m_pPubKey;
    DWORD               m_cbPubKey;
    HANDLE              m_hKeyFile;
    HANDLE              m_hKeyMap;
    mdAssembly          m_tkAssembly;

};
#endif // __assembly_h__

