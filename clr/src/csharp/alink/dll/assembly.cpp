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
// File: assembly.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "alink.h"
#include "asmlink.h"
#include "assembly.h"
#include "msgsids.h"
#include "common.h"
#include "merge.h"
#include "oleauto.h"

#define OPTION(name, type, flags, id, shortname, longname, CA) {CA, name, id, type, flags},
const _OptionCA OptionCAs [] = {
#include "options.h"
    {L"", optLastAssemOption, IDS_InternalError, VT_EMPTY, 0x02}  // 2 = Custom Attribute Only
};

#define ERRORDEF(num, sev, name, id) {num, sev, id},
const int ERROR_DEFS[][3] = {
#include "errors.h"
};

#define SAFEFREE(var) if ((var) != NULL) { VSFree((void*)(var)); (var) = NULL; }

__forceinline bool IsValidPublicKeyBlob(const PublicKeyBlob *p, const size_t len) {
    return ((VAL32(p->cbPublicKey) + (sizeof(ULONG) * 3)) == len &&        // do the lengths match?
            GET_ALG_CLASS(VAL32(p->SigAlgID)) == ALG_CLASS_SIGNATURE &&    // is it a valid signature alg?
            GET_ALG_CLASS(VAL32(p->HashAlgID)) == ALG_CLASS_HASH);         // is it a valid hash alg?
}

// Helper functions that wrap calls to IMetaDataImport methods.
static HRESULT __stdcall CallEnumCustomAttributes(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdToken tk, mdToken tkType,
                                mdCustomAttribute rCustomValues[], ULONG cMax,
                                ULONG *pcCustomValues)
{
    return pImport->EnumCustomAttributes(phEnum, tk, tkType, rCustomValues, cMax, pcCustomValues);
}

static HRESULT __stdcall CallEnumUserStrings(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdString rStrings[],
                                ULONG cmax, ULONG *pcStrings)
{
    return pImport->EnumUserStrings(phEnum, rStrings, cmax, pcStrings);
}

static HRESULT __stdcall CallEnumSignatures(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdSignature rSignatures[],
                                ULONG cmax, ULONG *pcSignatures)
{
    return pImport->EnumSignatures(phEnum, rSignatures, cmax, pcSignatures);
}

static HRESULT __stdcall CallEnumTypeDefs(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdTypeDef rTypeDefs[],
                                ULONG cMax, ULONG *pcTypeDefs)
{
    return pImport->EnumTypeDefs(phEnum, rTypeDefs, cMax, pcTypeDefs);
}

static HRESULT __stdcall CallEnumMembers(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdTypeDef cl,
                                mdToken rMembers[], ULONG cMax,
                                ULONG *pcTokens)
{
    return pImport->EnumMembers(phEnum, cl, rMembers, cMax, pcTokens);
}

static HRESULT __stdcall CallEnumTypeRefs(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdTypeRef rTypeRefs[],
                                ULONG cMax, ULONG* pcTypeRefs)
{
    return pImport->EnumTypeRefs(phEnum, rTypeRefs, cMax, pcTypeRefs);
}

static HRESULT __stdcall CallEnumMemberRefs(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdToken tkParent,
                                mdMemberRef rMemberRefs[], ULONG cMax,
                                ULONG *pcTokens)
{
    return pImport->EnumMemberRefs(phEnum, tkParent, rMemberRefs, cMax, pcTokens);
}

static HRESULT __stdcall CallEnumTypeSpecs(IMetaDataImport *pImport,
                                HCORENUM *phEnum, mdTypeSpec rTypeSpecs[],
                                ULONG cmax, ULONG *pcTypeSpecs)
{
    return pImport->EnumTypeSpecs(phEnum, rTypeSpecs, cmax, pcTypeSpecs);
}

/*
 * Helper function and load and format a message. Uses Unicode
 * APIs, so it won't work on Win95/98.
 */
static bool LoadAndFormatW(HINSTANCE hModule, int resid, va_list args, LPWSTR buffer, int cchMax)
{
    LPWSTR formatString = (LPWSTR)_alloca(cchMax * sizeof(WCHAR));

    return (LoadStringW (hModule, resid, formatString, cchMax) != 0) &&
           (FormatMessageW (FORMAT_MESSAGE_FROM_STRING, formatString, 0, 0, buffer, cchMax, &args) != 0);
}

/*
 * Helper function to load and format a message using ANSI functions.
 * Used as a backup when Unicode ones are not available.
 */
static bool LoadAndFormatA(HINSTANCE hModule, int resid, va_list args, LPWSTR buffer, int cchMax)
{
    LPSTR pszFormat = (LPSTR)_alloca((cchMax * 2) * sizeof(WCHAR));
    LPSTR pszResult = (LPSTR)_alloca((cchMax * 2) * sizeof(WCHAR));

    return (LoadStringA (hModule, resid, pszFormat, cchMax * 2) != 0) &&
           (FormatMessageA (FORMAT_MESSAGE_FROM_STRING, pszFormat, 0, 0, pszResult, cchMax * 2, &args) != 0) &&
           (MultiByteToWideChar (CP_ACP, 0, pszResult, -1, buffer, cchMax) != 0);
}

/*
 * Helper function and load and format a message. Uses Unicode
 *  and ANSI APIs, so it works on all platforms.
 */
static bool W_LoadAndFormat(HINSTANCE hModule, int resid, va_list args, LPWSTR buffer, int cchMax)
{
    return LoadAndFormatW (hModule, resid, args, buffer, cchMax) ||
           LoadAndFormatA (hModule, resid, args, buffer, cchMax);
}

inline int FindOption(LPCWSTR CustAttrName)
{
    for (int opt = 0; opt < optLastAssemOption; opt++) {
        if (wcscmp(CustAttrName, OptionCAs[opt].name) == 0)
            return opt;
    }
    return optLastAssemOption;
}

LPCWSTR ErrorOptionName(AssemblyOptions opt) {
    static WCHAR buffer[1024];
    HINSTANCE hRes = GetALinkMessageDll();

    // Load the message and fill in arguments. Try using Unicode function first,
    // then back off to ANSI.
    if (!W_LoadAndFormat (hRes, OptionCAs[opt].optName, NULL, buffer, 1024))
    {
        // Not a lot we can do if we can't report an error. Assert in debug so we know
        // what is going on.
        VSFAIL("Failed to load error message");
        return NULL;
    }

    // This will probably fail if we're in low-memory conditions.
    return buffer;
}

void CFile::ZeroInitFile()
{
    m_isAssembly = false;
    m_isStdLib = false;
    m_isCTDeclared = false;
    m_isDeclared = false;
    m_isImport = false;
    m_isUBM = false;
    m_bIsExe = false;
    m_Name = NULL;
    m_Path = NULL;
    m_modName = NULL;
    m_pEmit = NULL;
    m_pImport = NULL;
    m_pError = NULL;
    m_SrcFile = NULL;
    m_tkAssemblyAttrib[0][0] = m_tkAssemblyAttrib[0][1] =
        m_tkAssemblyAttrib[1][0] = m_tkAssemblyAttrib[1][1] = mdTokenNil;
    m_tkFile = mdTokenNil;
    m_tkError = mdTokenNil;
    m_tkMscorlibRef = mdAssemblyRefNil;
    m_pLinker = NULL;
    m_Types.ClearAll();
}

CFile::CFile(LPCWSTR pszFilename, IMetaDataEmit* pEmitter, IMetaDataError *pError, CAsmLink* pLinker)
{
    ZeroInitFile();
    m_pError = pError;
    m_pEmit = pEmitter;
    m_pLinker = pLinker;
    if (m_pEmit) {
        m_pEmit->AddRef();
#ifdef _DEBUG
        HRESULT hr =
#endif  // _DEBUG
        m_pEmit->QueryInterface(IID_IMetaDataImport, (void**)&m_pImport);
        ASSERT(SUCCEEDED(hr));
        m_isDeclared = true;
    } else {
        m_pImport = NULL;
        m_isDeclared = false;
        m_isCTDeclared = true;
        m_isUBM = true;
    }

    // The name is everything after the last '/' or '\'
    m_Path = VSAllocStr(pszFilename);
    LPCWSTR temp;
    temp = wcsrchr( m_Path, L'\\');
    if (temp == NULL)
        temp = m_Path;
    else
        temp++;
    m_Name = wcsrchr( temp, L'/');
    if (m_Name == NULL)
        m_Name = temp;
    else 
        m_Name++;

    size_t len = wcslen(m_Name);
    m_bIsExe = ((len > 4) && (_wcsicmp(m_Name + (len - 4), L".exe") == 0));
}

CFile::CFile(IMetaDataImport* pImporter, IMetaDataError *pError, CAsmLink* pLinker)
{
    ZeroInitFile();
    m_isImport = true;
    m_pError = pError;
    m_pLinker = pLinker;
    m_pImport = pImporter;
    m_pImport->AddRef();
    m_pEmit = NULL;

    m_Name = (LPWSTR)VSAlloc(sizeof(WCHAR)*MAX_IDENT_LEN);
    ULONG chName = 0;
    HRESULT hr = m_pImport->GetScopeProps( (LPWSTR)m_Name, MAX_IDENT_LEN, &chName, NULL);
    if (FAILED(hr)) {
        VSFree((LPWSTR)m_Name);
        m_Name = NULL;
    }
    m_Path = m_Name;

    m_bIsExe = ((chName > 4) && (m_Name != NULL && _wcsicmp(m_Name + (chName - 4), L".exe") == 0));

    m_isUBM = true;
    m_isDeclared = false;
}

CFile::CFile(LPCWSTR pszFilename, IMetaDataImport* pImporter, IMetaDataError *pError, CAsmLink* pLinker)
{
    ZeroInitFile();
    m_isImport = true;
    m_pError = pError;
    m_pLinker = pLinker;
    m_pImport = pImporter;
    m_pImport->AddRef();
    m_pEmit = NULL;
    m_isDeclared = false;

    // The name is everything after the last '/' or '\'
    m_Path = VSAllocStr(pszFilename);
    LPCWSTR temp;
    temp = wcsrchr( m_Path, L'\\');
    if (temp == NULL)
        temp = m_Path;
    else
        temp++;
    m_Name = wcsrchr( temp, L'/');
    if (m_Name == NULL)
        m_Name = temp;
    else
        m_Name++;

    size_t len = wcslen(m_Name);
    m_bIsExe = ((len > 4) && (_wcsicmp(m_Name + (len - 4), L".exe") == 0));
}

CFile::~CFile()
{
    if (m_pEmit) {
        m_pEmit->Release();
        m_pEmit = NULL;
    }
    if (m_pImport) {
        m_pImport->Release();
        m_pImport = NULL;
    }
    if (m_CAs.Count()) {
        for (DWORD l = 0; l < m_CAs.Count(); l++) {
            if (m_CAs.GetAt(l))
                delete [] m_CAs.GetAt(l)->pVal;
        }
    }
    SAFEFREE(m_Path);
    m_Name = NULL;
    SAFEFREE(m_SrcFile);
    SAFEFREE(m_modName);
    m_pError = NULL;
}

void CFile::PreClose()
{
    if (m_pEmit) {
        m_pEmit->Release();
        m_pEmit = NULL;
    }
    if (m_pImport) {
        m_pImport->Release();
        m_pImport = NULL;
    }
}

HRESULT CFile::ImportFile(DWORD *pdwCountOfTypes, CAssemblyFile* pAEmit)
{
    ASSERT(m_pImport != NULL || (m_isDeclared && m_isCTDeclared));
    HRESULT hr = S_OK;

    if ( !m_isDeclared) {
        HCORENUM enumTypes;
        mdTypeDef typedefs[32];
        ULONG cTypedefs, iTypedef, cchName;
        Type *newType = NULL;
        WCHAR name[MAX_IDENT_LEN];
        DWORD dwFlags = 0;
        mdToken tkParent = mdTokenNil;
    
        m_Types.ClearAll();

        // Enumeration all the types in the scope.
        enumTypes = 0;
        do {
            // Get next batch of types.
            hr = m_pImport->EnumTypeDefs(&enumTypes, typedefs, lengthof(typedefs), &cTypedefs);
            if (FAILED(hr))
                break;

            // Process each type.
            for (iTypedef = 0; iTypedef < cTypedefs; ++iTypedef) {
                hr = m_pImport->GetTypeDefProps( typedefs[iTypedef], name, MAX_IDENT_LEN, &cchName, &dwFlags, NULL);
                if (FAILED(hr)) // Don't ReportError because this is a COM+ error!
                    break;
                if (pAEmit && IsTdNested(dwFlags)) {
                    Type t;
                    if (FAILED(hr = m_pImport->GetNestedClassProps( typedefs[iTypedef], &tkParent)))
                        // Don't ReportError because this is a COM+ error!
                        break;
                    t.TypeDef = tkParent;
                    newType = (Type*)bsearch( &t, m_Types.Base(), m_Types.Count(), sizeof(t), (int (__cdecl*)(const void*,const void*))Type::CompareTypeDefs);
                    // Else assume that if the parent doesn't exist, it's because it's non-public
                    // so this must be non-public
                    if (newType != NULL) {
                        tkParent = newType->ComType;
                        newType = NULL;
                    } else {
                        dwFlags = ((dwFlags & (~tdVisibilityMask)) | tdNestedPrivate);
                    }
                } else {
                    tkParent = m_tkFile;
                }
                if (IsTdPublic(dwFlags) || IsTdNestedPublic(dwFlags) ||
                    IsTdNestedFamily(dwFlags) || IsTdNestedFamORAssem(dwFlags)) {
                    // Only Export public types
                    hr = m_Types.Add( NULL, &newType);
                    if (FAILED(hr)) {
                        hr = ReportError(hr); // This always E_OUTOFMEMORY?
                        break;
                    }
                    newType->TypeDef = typedefs[iTypedef];
                    if (pAEmit != NULL) {
                        hr = pAEmit->AddExportType( tkParent, typedefs[iTypedef], name, dwFlags, &newType->ComType);
                        // Don't ReportError because AddExportType did!
                    } else
                        newType->ComType = mdExportedTypeNil;
                } else if (m_isUBM && (IsTdNestedAssembly(dwFlags) || IsTdNestedFamANDAssem(dwFlags) || IsTdNotPublic(dwFlags))) {
                    hr = m_Types.Add( NULL, &newType);
                    if (FAILED(hr)) {
                        hr = ReportError(hr); // This always E_OUTOFMEMORY?
                        break;
                    }
                    newType->TypeDef = typedefs[iTypedef];
                    newType->ComType = mdExportedTypeNil;
                }
#ifdef _DEBUG
                else if (m_isUBM)
                    ASSERT(IsTdNestedPrivate(dwFlags));
#endif
            }
        } while (cTypedefs > 0 && SUCCEEDED(hr));
    
        m_pImport->CloseEnum(enumTypes);
    } // if (!m_isDeclared)

    if (SUCCEEDED(hr)) {
        m_isDeclared = true;
        if (!m_isCTDeclared || pAEmit)
            hr = ImportCAs(pAEmit);
        if (SUCCEEDED(hr)) {
            if (pdwCountOfTypes)
                *pdwCountOfTypes = m_Types.Count();
            // ImportCAs can return S_FALSE indicating there are no
            // Custom Attributes, but we only want to return S_FALSE
            // if there are no Types!
            if (pAEmit && m_Types.Count() > 0) {
                ULONG iTypedef, cchName;
                Type *newType = NULL;
                WCHAR name[MAX_IDENT_LEN];
                DWORD dwFlags = 0;
                mdToken tkParent = mdTokenNil;
                int Skipped = 0, LastSkipped;

                do {
                    LastSkipped = Skipped;
                    for (iTypedef = 0; iTypedef < m_Types.Count(); iTypedef++) {
                        if (!IsNilToken(m_Types.GetAt(iTypedef)->ComType))
                            continue;
                        hr = m_pImport->GetTypeDefProps( m_Types.GetAt(iTypedef)->TypeDef, name, MAX_IDENT_LEN, &cchName, &dwFlags, NULL);
                        if (FAILED(hr)) // Don't ReportError because this is a COM+ error!
                            break;
                        if (IsTdNested(dwFlags)) {
                            Type t;
                            if (FAILED(hr = m_pImport->GetNestedClassProps( m_Types.GetAt(iTypedef)->TypeDef, &tkParent)))
                                // Don't ReportError because this is a COM+ error!
                                break;
                            t.TypeDef = tkParent;
                            newType = (Type*)bsearch( &t, m_Types.Base(), m_Types.Count(), sizeof(t), (int (__cdecl*)(const void*,const void*))Type::CompareTypeDefs);
                            // Else assume that if the parent doesn't exist, it's because it's non-public
                            // so this must be non-public
                            if (newType != NULL) {
                                if (IsNilToken(newType->ComType)) {
                                    Skipped = LastSkipped + 1;
                                    continue;
                                }
                                tkParent = newType->ComType;
                                newType = NULL;
                            } else {
                                continue;
                            }
                        } else {
                            tkParent = m_tkFile;
                        }
                        if (IsTdPublic(dwFlags) || IsTdNestedPublic(dwFlags) ||
                            IsTdNestedFamily(dwFlags) || IsTdNestedFamORAssem(dwFlags)) {
                            hr = pAEmit->AddExportType( tkParent, m_Types.GetAt(iTypedef)->TypeDef, name, dwFlags, &m_Types.GetAt(iTypedef)->ComType);
                            // Don't ReportError because AddExportType did!
                        }
                    }
                    if (Skipped > 10 && SUCCEEDED(hr))  // Never do more than 10 iterations
                        hr = E_PENDING;

                } while (Skipped > LastSkipped && Skipped < 10 && SUCCEEDED(hr));
            }
            if (SUCCEEDED(hr))
                hr = m_Types.Count() > 0 ? S_OK : S_FALSE;
        } else if (pdwCountOfTypes) {
            // Set this to 0 for the error case
            *pdwCountOfTypes = 0;
        }
    }
    
    return hr;
}

HRESULT CFile::ImportResources(CAssemblyFile* pAEmit)
{
    ASSERT(m_pImport != NULL && pAEmit != NULL);

    CComPtr<IMetaDataAssemblyImport> pAImport = NULL;
    HRESULT hr = S_OK;
    HCORENUM enumRes;
    mdManifestResource res[32];
    ULONG cRes, iRes;
    DWORD dwFlags = 0, dwOffset = 0;
    mdToken tkLoc;
    mdManifestResource mr = mdTokenNil;

    if (FAILED(hr = m_pImport->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&pAImport)))
        // Don't ReportError because this is a COM+ error!
        return hr;
    
    // Enumeration all the resources in the scope.
    enumRes = 0;
    do {
        // Get next batch of resources.
        hr = pAImport->EnumManifestResources(&enumRes, res, lengthof(res), &cRes);
        if (FAILED(hr))
            // Don't ReportError because this is a COM+ error!
            break;

        LPWSTR name;
        DWORD cchName;

        // Process each type.
        for (iRes = 0; iRes < cRes; ++iRes) {
            hr = pAImport->GetManifestResourceProps( res[iRes], NULL, 0, &cchName,
                &tkLoc, &dwOffset, &dwFlags);
            if (FAILED(hr))
                // Don't ReportError because this is a COM+ error!
                break;
            ASSERT (IsNilToken(tkLoc));
            if ((name = new WCHAR[++cchName]) == NULL)
                break;

            hr = pAImport->GetManifestResourceProps( res[iRes], name, cchName, &cchName,
                NULL, NULL, NULL);
            if (FAILED(hr)) {
                // Don't ReportError because this is a COM+ error!
                delete [] name;
                break;
            }

            hr = pAEmit->GetEmitter()->DefineManifestResource( name, m_tkFile, dwOffset, dwFlags, &mr);
            delete [] name;
            if (FAILED(hr))
                // Don't ReportError because this is a COM+ error!
                break;
        }
    } while (cRes > 0 && SUCCEEDED(hr));
    
    pAImport->CloseEnum(enumRes);

    return hr;
}

HRESULT CFile::ImportCAs(CAssemblyFile* pAEmit)
{
    HRESULT hr = S_OK;
    bool bReport = false;
    ASSERT(m_pImport != NULL || m_isCTDeclared);

    if (m_isCTDeclared) {
        if (pAEmit == NULL)
            return m_CAs.Count() == 0 ? S_FALSE : S_OK;
        else {
            CA *ca;
            DWORD i, j;
            
            for (i = 0, j = m_CAs.Count(); i < j && SUCCEEDED(hr); i++) {
                ca = m_CAs.GetAt(i);
                hr = pAEmit->AddCustomAttribute(ca->tkType, ca->pVal, ca->cbVal, ca->bSecurity, ca->bAllowMulti, this);
                if (hr == S_FALSE) {
                    // This means the CA over-wrote an assembly bit previously set
                    // we must report the warning
                    LPWSTR name;
                    int opt;
                    GetCAName(ca->tkType, &name);
                    opt = FindOption(name);
                    if (opt != optLastAssemOption)
                        ReportError(WRN_OptionConflicts, opt, &bReport, ErrorOptionName((AssemblyOptions)opt));
                    delete [] name;
                }
            }
            return hr;
        }
    }

    mdToken tkAssem, tkAR;
    const static WCHAR names [4][3] = {  {0,0,0}, {'S', 0, 0}, {'M', 0, 0}, {'S', 'M', 0} };
    const static BOOL vals[4][2] = { {FALSE, FALSE}, {TRUE, FALSE}, {FALSE, TRUE}, {TRUE, TRUE} };

    hr = FindStdLibRef(&tkAR);

    for (int i = 0; i < 4 && SUCCEEDED(hr); i++) {
        WCHAR name[1024];
        wcscpy(name, MODULE_CA_LOCATION);
        wcscat(name, names[i]);
        // Try both a scoped and unscoped reference
        if (FAILED(hr = m_pImport->FindTypeRef(tkAR, name, &tkAssem)) &&
            (hr != CLDB_E_RECORD_NOTFOUND || tkAR == mdTokenNil || FAILED(hr = m_pImport->FindTypeRef(mdTokenNil, name, &tkAssem)))) {

            if (hr == CLDB_E_RECORD_NOTFOUND) {
                hr = S_FALSE;
            }
            continue;
        }

        if (IsNilToken(tkAssem)) {
            hr = S_OK;
            continue;
        }

        hr = ReadCAs( tkAssem, pAEmit, vals[i][1], vals[i][0]);
    }

    if (SUCCEEDED(hr))
        m_isCTDeclared = true;

    return hr;
}

HRESULT CFile::ReadCAs(mdToken tkParent, CAssemblyFile* pAEmit, BOOL bMulti, BOOL bSecurity)
{
    HRESULT hr = S_OK;

    HCORENUM enumCAs = NULL;
    mdCustomAttribute CAs[32];
    ULONG cCAs, iCAs;
    mdToken tkType;
    bool bReport = false;

    // Enumeration all the CAs in the scope.
    do {
        // Get next batch of CAs.
        hr = m_pImport->EnumCustomAttributes(&enumCAs, tkParent, 0, CAs, lengthof(CAs), &cCAs);
        if (FAILED(hr))
            // Don't ReportError because this is a COM+ error!
            break;

        LPVOID pVal;
        DWORD cbVal;

        // Process each CA.
        for (iCAs = 0; iCAs < cCAs; ++iCAs) {
            hr = m_pImport->GetCustomAttributeProps(CAs[iCAs], NULL, &tkType, (const void**)&pVal, &cbVal);
            if (FAILED(hr))
                // Don't ReportError because this is a COM+ error!
                break;
            if (pAEmit) {
                hr = pAEmit->AddCustomAttribute( tkType, pVal, cbVal, bSecurity, bMulti, this);
                if (hr == S_FALSE) {
                    // This means the CA over-wrote an assembly bit previously set
                    // we must report the warning
                    LPWSTR name;
                    int opt;
                    GetCAName(tkType, &name);
                    opt = FindOption(name);
                    if (opt != optLastAssemOption)
                        ReportError(WRN_OptionConflicts, opt, &bReport, ErrorOptionName((AssemblyOptions)opt));
                    delete [] name;
                }
            } else {
                CA *ca;
                hr = m_CAs.Add( NULL, &ca);
                if (SUCCEEDED(hr)) {
                    ca->tkCA = CAs[iCAs];
                    ca->cbVal = cbVal;
                    ca->tkType = tkType;
                    ca->bSecurity = (bSecurity != FALSE);
                    ca->bAllowMulti = (bMulti != FALSE);
                    ca->pVal = new BYTE[cbVal];
                    memcpy(ca->pVal, pVal, cbVal);
                } else {
                    // Report this one
                    hr = ReportError(hr);
                }
            }
            if (FAILED(hr))
                // Don't ReportError because this is a COM+ error!
                break;
        }
    } while (cCAs > 0 && SUCCEEDED(hr));

    m_pImport->CloseEnum(enumCAs);
    return hr;
}

HRESULT CFile::GetType(DWORD index, mdTypeDef* pType)
{
    if (index >= m_Types.Count())
        return E_INVALIDARG;
    *pType = m_Types.GetAt(index)->TypeDef;
    return S_OK;
}

HRESULT CFile::GetComType(DWORD index, mdExportedType* pType)
{
    if (index >= m_Types.Count())
        return E_INVALIDARG;
    *pType = m_Types.GetAt(index)->ComType;
    return S_OK;
}

HRESULT CFile::GetCA(DWORD index, mdToken filter, mdCustomAttribute* pCA)
{
    if (index >= m_CAs.Count())
        return E_INVALIDARG;
    CA *ca = m_CAs.GetAt(index);
    if (filter == 0 || ca->tkType == filter) {
        *pCA = m_CAs.GetAt(index)->tkCA;
        return S_OK;
    } else {
        *pCA = NULL;
        return S_FALSE;
    }

}

HRESULT CFile::GetStdLibRef(mdAssemblyRef *tkRef)
{
    if (!tkRef)
        return E_POINTER;
    if (!IsNilToken(m_tkMscorlibRef)) {
        *tkRef = m_tkMscorlibRef;
        return S_OK;
    }

    CAssemblyFile *pAsm = m_pLinker->GetStdLib();
    if (pAsm)
        return pAsm->MakeAssemblyRef(m_pEmit, tkRef);

    HRESULT hr = FindStdLibRef(tkRef);
    if (hr == S_FALSE) {
        ASSERT(*tkRef == mdTokenNil);
        return E_FAIL;
    }
    return hr;
}

HRESULT CFile::FindStdLibRef(mdAssemblyRef *tkRef)
{
    if (!tkRef)
        return E_POINTER;
    if (!IsNilToken(m_tkMscorlibRef)) {
        *tkRef = m_tkMscorlibRef;
        return S_OK;
    }

    HRESULT hr;
    CComPtr<IMetaDataAssemblyImport> pAImport;
    
    if (m_pEmit) {
        if (FAILED(hr = m_pEmit->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&pAImport)))
            return hr;
    } else if (m_pImport) {
        if (FAILED(hr = m_pImport->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&pAImport)))
            return hr;
    }

    ULONG count;
    mdAssemblyRef ar[4];
    HCORENUM hEnum = NULL;

    while (SUCCEEDED(pAImport->EnumAssemblyRefs( &hEnum, ar, lengthof(ar), &count)) && count > 0) {
        ULONG len = 0;
        WCHAR buffer[32];
        for (ULONG i = 0; i < count; i++) {
            if (FAILED(hr = pAImport->GetAssemblyRefProps( ar[i], NULL, NULL, buffer, lengthof(buffer), &len, NULL, NULL, NULL, NULL))) {
                pAImport->CloseEnum(hEnum);
                return hr;
            }
            if (len == 9 && wcscmp(buffer, L"mscorlib") == 0) {
                *tkRef = ar[i];
                pAImport->CloseEnum(hEnum);
                return S_OK;
            }
        }
    }

    pAImport->CloseEnum(hEnum);
    *tkRef = mdTokenNil;
    return S_FALSE;
}

// Returns a TypeRef or TypeDef token for the given typename
// If needed, it will create a TypeRef
// TypeRefs will always be scoped by an AssemblyRef to MSCORLIB
HRESULT CFile::GetTypeRef(LPCWSTR wszTypeName, mdToken * tkType)
{
    HRESULT hr;
    mdAssemblyRef tkAR = m_tkMscorlibRef;

    if (IsNilToken(tkAR) && FAILED(hr = GetStdLibRef(&tkAR)))
        return hr;
    else
        hr = S_OK;

    if (!m_pImport && !m_pEmit)
        return E_FAIL;
    if (!m_pImport && FAILED(hr = m_pEmit->QueryInterface(IID_IMetaDataImport, (void**)&m_pImport)))
        return hr;

    // Find or Make the TypeRef
    hr = m_pImport->FindTypeRef(tkAR, wszTypeName, tkType);
    if (hr == CLDB_E_RECORD_NOTFOUND)
         hr = m_pImport->FindTypeDefByName( wszTypeName, mdTokenNil, tkType);
    if (hr == CLDB_E_RECORD_NOTFOUND && m_pEmit)
         hr = m_pEmit->DefineTypeRefByName( tkAR, wszTypeName, tkType);

    return hr;
}

HRESULT CFile::AddResource(mdToken Location, LPCWSTR pszName, DWORD offset, DWORD flags)
{
    HRESULT hr;
    IMetaDataAssemblyEmit* pAEmit = NULL;
    mdManifestResource mr = mdTokenNil;

    hr = m_pEmit->QueryInterface( IID_IMetaDataAssemblyEmit, (void**)&pAEmit);
    if (SUCCEEDED(hr))
        hr = pAEmit->DefineManifestResource( pszName, Location, offset, flags, &mr);
    // Don't ReportError because this is a COM+ error!

    return hr;
}

HRESULT CFile::AddCustomAttribute(mdToken tkType, const void* pValue, DWORD cbValue, BOOL bSecurity, BOOL bAllowMultiple, CFile *source)
{
    ASSERT(m_pEmit != NULL && (source == this || source == NULL));
    if (IsNilToken(m_tkAssemblyAttrib[bSecurity][bAllowMultiple])) {
        ASSERT(m_isUBM);
        HRESULT hr;
        WCHAR name[1024];
        wcscpy(name, MODULE_CA_LOCATION);
        if (IsNilToken(m_tkMscorlibRef) && FAILED(hr = GetStdLibRef(&m_tkMscorlibRef)))
            return hr;

        if (bSecurity != FALSE)
            wcscat(name, L"S");
        if (bAllowMultiple != FALSE)
            wcscat(name, L"M");
        if (FAILED(hr = GetTypeRef(name, &m_tkAssemblyAttrib[bSecurity][bAllowMultiple])))
            // Don't ReportError because this is a COM+ error!
            return hr;
    }
    return m_pEmit->DefineCustomAttribute( m_tkAssemblyAttrib[bSecurity][bAllowMultiple], tkType, pValue, cbValue, NULL);
}

HRESULT CFile::MakeModuleRef(IMetaDataEmit *pEmit, mdModuleRef *pmr)
{
    return pEmit->DefineModuleRef(GetModuleName(), pmr);
}

HRESULT CFile::ReportError(HRESULT hr, mdToken tkError, bool * pbReport)
{
    if (hr == S_OK || hr == S_FALSE)
        return hr;

    HRESULT hr2;
    CComPtr<ICreateErrorInfo>   cerr;
    CComPtr<IErrorInfo>         err;

    if (tkError == mdTokenNil)
        tkError = m_tkError;

    VERIFY(SUCCEEDED(hr2 = CreateErrorInfo( &cerr)));
    VERIFY(SUCCEEDED(hr2 = cerr->SetGUID(IID_IALink)));
    ASSERT(HIWORD(hr) != 0);
    // this is a normal HRESULT
    VERIFY(SUCCEEDED(hr2 = cerr->SetDescription((LPWSTR)ErrorHR(hr))));

    if (SUCCEEDED(hr2 = cerr->QueryInterface(IID_IErrorInfo, (void**)&err)))
        SetErrorInfo( 0, err);

    if (pbReport != NULL) {
        if (m_pError != NULL)
            *pbReport = (S_OK == m_pError->OnError(hr, tkError));
        else
            *pbReport = false;
    }

    return hr;
}

HRESULT CFile::ReportError(ERRORS err, mdToken tkError, bool * pbReport, ...)
{
    ASSERT(HIWORD(err) == 0);

    HRESULT hr2, hr = E_FAIL;
    CComPtr<ICreateErrorInfo>   cerr;
    CComPtr<IErrorInfo>         error;
    CComBSTR                    msg;

    if (tkError == mdTokenNil)
        tkError = m_tkError;

    if (SUCCEEDED(hr2 = CreateErrorInfo( &cerr)) &&
        SUCCEEDED(hr2 = cerr->SetGUID(IID_IALink))) {

        // This one of our errors
        va_list args;
        va_start( args, pbReport);
        msg.Attach(ErrorMessage(ERROR_DEFS[err][2], args));
        hr2 = cerr->SetDescription(msg);
        va_end(args);
        hr = MAKE_HRESULT(((ERROR_DEFS[err][1] < 1) ? SEVERITY_ERROR : SEVERITY_SUCCESS), FACILITY_ITF, ERROR_DEFS[err][0] & 0x0000FFFF);
    }
    if (SUCCEEDED(hr2) &&
        SUCCEEDED(hr2 = cerr->QueryInterface(IID_IErrorInfo, (void**)&error)))
        SetErrorInfo( 0, error);

    if (pbReport != NULL) {
        if (SUCCEEDED(hr2) && m_pError != NULL)
            *pbReport = (S_OK == m_pError->OnError(hr, tkError));
        else
            *pbReport = false;
    }

    return hr;
}


/*
 * Get a string (PWSTR) for an HRESULT (Attempt to use GetErrorInfo)
 */
LPCWSTR CFile::ErrorHR(HRESULT hr, IUnknown *pUnk, REFIID iid)
{
    CComPtr<ISupportErrorInfo> pErrInfo;
    static WCHAR buffer[2048];
    bool UseErrorInfo = false;

    if (pUnk && SUCCEEDED(pUnk->QueryInterface(IID_ISupportErrorInfo, (void**)&pErrInfo)) &&
        pErrInfo &&
        S_OK == pErrInfo->InterfaceSupportsErrorInfo( iid))
        UseErrorInfo = true;

    // HACK: IMetaDataEmit supports the ErrorInfo
    if (!UseErrorInfo && iid == IID_IMetaDataEmit)
        UseErrorInfo = true;

    // See if there is more detailed error message available via GetErrorInfo.
    CComPtr<IErrorInfo> err;
    CComBSTR str;
    GUID guid;

    if (S_OK == GetErrorInfo( 0, &err) && err != NULL &&
        (UseErrorInfo || (SUCCEEDED(err->GetGUID( &guid)) && guid == iid)) &&
        SUCCEEDED(err->GetDescription( &str)) &&
        SysStringLen(str) < lengthof(buffer)) {
            memcpy(buffer, str, SysStringLen(str) * sizeof(WCHAR));
            return buffer;
    }
    return ErrorHR(hr);
}
        

/*
 * Get a string (PWSTR) for an HRESULT
 */
LPCWSTR CFile::ErrorHR(HRESULT hr)
{
    static WCHAR buffer[2048];
    int r;

    // Use FormatMessage to get the error string for the HRESULT from the system.
    r = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, hr, 0,
                        buffer, lengthof(buffer),
                        NULL);


    // Check for errors, and possibly give an 'UnknownError
    if (r == 0)
        return UnknownError(buffer, 2048, hr);

    return buffer;
}

LPWSTR UnknownErrorHelper(LPWSTR buffer, size_t cchMax, ...)
{
    HINSTANCE hRes = GetALinkMessageDll();
    ASSERT(cchMax <= ULONG_MAX);
    ULONG chMax = (ULONG)cchMax;

    // Load the message and fill in arguments. Try using Unicode function first,
    // then back off to ANSI.
    va_list args;
    va_start(args, cchMax); // We assume that the 1 and only argument is an HRESULT
    if (!W_LoadAndFormat (hRes, IDS_UnknownError, args, buffer, chMax))
    {
        // Not a lot we can do if we can't report an error. Assert in debug so we know
        // what is going on.
        VSFAIL("FormatMessage failed to load error message");
        buffer[0] = L'\0';
    }
    va_end(args);

    // This will probably fail if we're in low-memory conditions.
    return buffer;
}

LPWSTR CFile::UnknownError(LPWSTR buffer, size_t cchMax, HRESULT hr)
{
    return UnknownErrorHelper(buffer, cchMax, hr);
}

BSTR CFile::ErrorMessage(DWORD err_id, va_list args)
{
    WCHAR pszBuffer[2048];
    HINSTANCE hRes = GetALinkMessageDll();

    // Load the message and fill in arguments. Try using Unicode function first,
    // then back off to ANSI.
    if (!W_LoadAndFormat (hRes, err_id, args, pszBuffer, 2048))
    {
        // Not a lot we can do if we can't report an error. Assert in debug so we know
        // what is going on.
        VSFAIL("FormatMessage failed to load error message");
        pszBuffer[0] = L'\0';
    }

    // This will probably fail if we're in low-memory conditions.
    return SysAllocString(pszBuffer);
}


HRESULT CFile::SetSource(LPCWSTR pszSourceFile)
{
    if (m_SrcFile)
        VSFree((LPWSTR)m_SrcFile);

    return (m_SrcFile = VSAllocStr(pszSourceFile)) == NULL ? E_OUTOFMEMORY : S_OK;
}

HRESULT CFile::CopyFile(IMetaDataDispenserEx *pDisp, bool bCleanup)
{
    HRESULT hr = S_OK;
    if (m_SrcFile != NULL && _wcsicmp(m_SrcFile, m_Path) != 0) {
        if (W_CopyFile(m_SrcFile, m_Path, FALSE) == FALSE)
            return ReportError(HRESULT_FROM_WIN32(GetLastError()));
        bCleanup = true;
    } else
        hr = S_FALSE; // FLASE because we did nothing

    if (bCleanup) {
        mdToken tkAR, tkAttrib[4] = {mdTokenNil, mdTokenNil, mdTokenNil, mdTokenNil };
        const static WCHAR names [4][2] = {  {0,0}, {'S', 0}, {'M', 0}, {'S', 'M'} };
        CTokenMap Map;
        ALPEFile module;
        CComPtr<IMetaDataEmit> pNewEmit, pOldEmit;
        CComPtr<IMetaDataAssemblyImport> pAImport;
        CComPtr<IMetaDataFilter> pFilter;
        ULONG count;
        mdAssemblyRef ar[4];
        HCORENUM hEnum = NULL;

        if (FAILED(hr = module.OpenFileAs( pDisp, m_Path, true, true)))
            // A COM+ or WIN32 error
            goto CLEANUP;

        if (FAILED(hr = pDisp->DefineScope(CLSID_CorMetaDataRuntime,   // Format of metadata
                                           0,                           // flags
                                           IID_IMetaDataEmit,           // Emitting interface
                                           (IUnknown **) &pNewEmit)))
            goto CLEANUP;

        if (FAILED(hr = module.pImport->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&pAImport))||
            FAILED(hr = module.pImport->QueryInterface(IID_IMetaDataEmit, (void**)&pOldEmit)) ||
            FAILED(hr = module.pImport->QueryInterface(IID_IMetaDataFilter, (void**)&pFilter)))
            goto CLEANUP;


        while (SUCCEEDED(pAImport->EnumAssemblyRefs( &hEnum, ar, lengthof(ar), &count)) && count > 0) {
            ULONG len = 0;
            WCHAR buffer[32];
            for (ULONG i = 0; i < count; i++) {
                if (FAILED(hr = pAImport->GetAssemblyRefProps( ar[i], NULL, NULL, buffer, lengthof(buffer), &len, NULL, NULL, NULL, NULL))) {
                    pAImport->CloseEnum(hEnum);
                    goto CLEANUP;
                }
                if (len == 9 && wcscmp(buffer, L"mscorlib") == 0) {
                    tkAR = ar[i];
                    pAImport->CloseEnum(hEnum);
                    goto FoundMscorLib;
                }
            }
        }
        pAImport->CloseEnum(hEnum);
        tkAR = mdTokenNil;

FoundMscorLib:
        pAImport = NULL; // Release it

        for (int i = 0; i < 4 && SUCCEEDED(hr); i++) {
            WCHAR name[1024];
            wcscpy(name, MODULE_CA_LOCATION);
            mdToken tkAssem = mdTokenNil;
            wcscat(name, names[i]);
            // Try both a scoped and unscoped reference
            if (FAILED(hr = module.pImport->FindTypeRef(tkAR, name, &tkAssem)) &&
                (hr != CLDB_E_RECORD_NOTFOUND || tkAR == mdTokenNil || FAILED(hr = module.pImport->FindTypeRef(mdTokenNil, name, &tkAssem)))) {
                if (hr == CLDB_E_RECORD_NOTFOUND) {
                    hr = S_FALSE;
                    continue;
                } else
                    break;
            }

            tkAttrib[i] = tkAssem;
            if (!IsNilToken(tkAssem)) {
                DeleteToken func(pOldEmit);
                hr = EnumAllExcept( module.pImport, &func, tkAssem, NULL, 0, NULL, &CallEnumCustomAttributes);
            }
        }
        pOldEmit = NULL;
        if (FAILED(hr) || FAILED(hr = pFilter->UnmarkAll()))
            goto CLEANUP;

        {
            MarkToken func(pFilter);

            if (FAILED(hr = EnumAllExcept( module.pImport, &func, &CallEnumUserStrings, NULL, 0, NULL)) ||
                FAILED(hr = EnumAllExcept( module.pImport, &func, &CallEnumSignatures, NULL, 0, NULL)) ||
                FAILED(hr = EnumAllExcept( module.pImport, &func, &CallEnumTypeDefs, NULL, 0, &CallEnumMembers)) ||
                // Get Global functions/methods
                FAILED(hr = EnumAllExcept( module.pImport, &func, mdTokenNil, NULL, 0, &CallEnumMembers)) ||
                FAILED(hr = EnumAllExcept( module.pImport, &func, &CallEnumTypeRefs, tkAttrib, lengthof(tkAttrib), &CallEnumMemberRefs)) ||
                // Get Global functions/methods
                FAILED(hr = EnumAllExcept( module.pImport, &func, mdTokenNil, tkAttrib, lengthof(tkAttrib), &CallEnumMemberRefs)) ||
                FAILED(hr = EnumAllExcept( module.pImport, &func, &CallEnumTypeSpecs, NULL, 0, NULL)))
                goto CLEANUP;
        }

        if (FAILED(hr = pNewEmit->SetModuleProps(GetModuleName())))
            goto CLEANUP;


        if (FAILED(hr = pNewEmit->Merge( module.pImport, &Map, NULL)) ||
            FAILED(hr = pNewEmit->MergeEnd()) ||
            FAILED(hr = pNewEmit->GetSaveSize((CorSaveSize)(cssAccurate | cssDiscardTransientCAs), &count)))
            goto CLEANUP;

        if (FAILED(hr = module.EmitMembers( NULL, NULL, NULL, NULL, NULL, &Map, pNewEmit)) ||
            FAILED(hr = module.WriteNewMetaData(pNewEmit, count)) ||
            FAILED(hr = module.Close()))
            hr = ReportError(hr);

CLEANUP:;
    }

    return hr;
}

HRESULT CFile::EnumAllExcept(IMetaDataImport * pImport, TokenEnumFunc * pFunc, EnumMemberFunc enumfunc, mdToken *tkExcept, size_t cExceptCount, EnumMemberNestedFunc enumnestedfunc)
{
    HRESULT     hr;
    mdToken     tkParent[16];
    ULONG       cParent;
    HCORENUM    hParent = NULL;

    do {
        if (SUCCEEDED(hr = (*enumfunc)(pImport, &hParent, tkParent, lengthof(tkParent), &cParent))) {
            for (ULONG i = 0; i < cParent; i++) {
                bool bMark = true;
                if (cExceptCount > 0) {
                    ASSERT(tkExcept != NULL);
                    for (size_t e = 0; e < cExceptCount; e++) {
                        if (tkParent[i] == tkExcept[e]) {
                            bMark = false;
                            break;
                        }
                    }
                }

                if (bMark && SUCCEEDED(hr = pFunc->DoSomething(tkParent[i])) && enumnestedfunc != NULL) {
                    hr = EnumAllExcept(pImport, pFunc, tkParent[i], tkExcept, cExceptCount, enumnestedfunc);
                }
                if (FAILED(hr))
                    break;
            }
        }
    } while (hr == S_OK && cParent == lengthof(tkParent));

    return hr;
}

HRESULT CFile::EnumAllExcept(IMetaDataImport * pImport, TokenEnumFunc * pFunc, mdToken tkParent, mdToken *tkExcept, size_t cExceptCount, EnumMemberNestedFunc enumnestedfunc, EnumMemberNested2Func enumnestedfunc2)
{
    HRESULT     hr = E_FAIL;
    mdToken     tkNested[16];
    ULONG       cNested = 0;
    HCORENUM    hNested = NULL;

    do {
        if ((enumnestedfunc && SUCCEEDED(hr = (*enumnestedfunc)(pImport, &hNested, tkParent, tkNested, lengthof(tkNested), &cNested))) ||
            (enumnestedfunc2 && SUCCEEDED(hr = (*enumnestedfunc2)(pImport, &hNested, tkParent, mdTokenNil, tkNested, lengthof(tkNested), &cNested)))) {
            for (ULONG i = 0; i < cNested; i++) {
                bool bMark = true;
                if (cExceptCount > 0) {
                    ASSERT(tkExcept != NULL);
                    for (size_t e = 0; e < cExceptCount; e++) {
                        if (tkNested[i] == tkExcept[e]) {
                            bMark = false;
                            break;
                        }
                    }
                }

                if (bMark)
                    hr = pFunc->DoSomethignNested(tkParent, tkNested[i]);
                if (FAILED(hr))
                    break;
            }
        }
    } while (hr == S_OK && cNested == lengthof(tkNested));

    return hr;
}

LPCWSTR CFile::GetModuleName()
{
    DWORD   chName;

    if (m_modName != NULL)
        return m_modName;

    // If we don't already have the name, get it from the MetaData
    // But if it isn't set in MD, then fall back on the filename
    // Unless we're renaming the file, in which case, get it from the new filename
    if (m_SrcFile == NULL || _wcsicmp(m_SrcFile, m_Path) == 0) {
        if (m_pImport == NULL) {
            if (m_pEmit == NULL || FAILED(m_pEmit->QueryInterface(IID_IMetaDataImport, (void**)&m_pImport)))
                goto FAIL;
        }
        if (FAILED(m_pImport->GetScopeProps( NULL, 0, &chName, NULL)) || chName == 0)
            goto FAIL;
        if ((m_modName = (LPWSTR)VSAlloc(sizeof(WCHAR)*chName)) == NULL)
            goto FAIL;
        if (SUCCEEDED(m_pImport->GetScopeProps( (LPWSTR)m_modName, chName, &chName, NULL)) &&
            chName > 0) 
            return m_modName;
    }

FAIL:
    if ((m_modName = (LPWSTR)VSAlloc(sizeof(WCHAR)*(wcslen(m_Name)+1)))) {
        return wcscpy((LPWSTR)m_modName, m_Name);
    }
    return m_Name;
}

HRESULT CFile::UpdateModuleName()
{
    HRESULT hr = E_FAIL;
    if (m_pEmit == NULL) {
        if (m_pImport == NULL || FAILED(hr = m_pImport->QueryInterface(IID_IMetaDataEmit, (void**)&m_pEmit)))
            return hr;
    }

    return m_pEmit->SetModuleProps(GetModuleName());
}

HRESULT CFile::DupModuleRef(mdToken *tkModule, IMetaDataImport* pImport)
{
    HRESULT hr = S_OK;
    LPWSTR  name;
    DWORD   chName;
    if (FAILED(hr = pImport->GetModuleRefProps( *tkModule, NULL, 0, &chName)))
        return hr;
    if ((name = (LPWSTR)VSAlloc(sizeof(WCHAR)*chName)) == NULL)
        return ReportError(E_OUTOFMEMORY);
    if (SUCCEEDED(hr = pImport->GetModuleRefProps( *tkModule, name, chName, &chName))) {
        if (wcscmp(name, GetModuleName()) != 0) {
            hr = m_pEmit->DefineModuleRef( name, tkModule);
        } else {
            hr = S_OK;
            *tkModule = TokenFromRid(1, mdtModule);
        }
    }
    return hr;
}

CAssemblyFile::CAssemblyFile(LPCWSTR pszFilename, AssemblyFlags afFlags, IMetaDataAssemblyEmit* pAEmitter, IMetaDataEmit* pEmitter, IMetaDataError *pError, CAsmLink* pLinker) : CFile( pszFilename, pEmitter, pError, pLinker)
{
    ZeroInitAssembly();
    m_pAEmit = pAEmitter;
    m_pAEmit->AddRef();
#ifdef _DEBUG
    HRESULT hr =
#endif  // _DEBUG
    m_pAEmit->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&m_pAImport);
    ASSERT(SUCCEEDED(hr));
    if (m_bIsInMemory = (afFlags & afInMemory))
        m_bIsExe = false;
    m_bClean = !!(afFlags & afCleanModules);
    m_isStdLib = (m_Name != NULL && _wcsicmp( m_Name, L"mscorlib.dll") == 0);
    m_bDoHash = !(afFlags & afNoRefHash);
}

void CAssemblyFile::ZeroInitAssembly()
{
    memset(&m_adata, 0, sizeof(m_adata));
    m_pAEmit = NULL;
    m_pAImport = NULL;
    m_hKeyFile = INVALID_HANDLE_VALUE;
    m_hKeyMap = NULL;
    m_Title = NULL;
    m_Description = NULL;
    m_Company = NULL;
    m_Product = NULL;
    m_ProductVersion = NULL;
    m_Copyright = NULL;
    m_Trademark = NULL;
    m_KeyName = NULL;
    m_KeyFile = NULL;
    m_pPubKey = NULL;
    m_cbPubKey = 0;
    m_szAsmName = NULL;
    m_iAlgID = 0;
    m_doHalfSign = false;
    m_isVersionSet = false;
    m_isAlgIdSet = false;
    m_isFlagsSet = false;
    m_isHalfSignSet = false;
    m_isAutoKeyName = false;
    m_isAssembly = true;
    m_bIsInMemory = false;
    m_bAllowExes = false;
    m_bClean = false;
    m_bDoHash = true;
    m_dwFlags = 0;
    m_pbKeyPair = NULL;
    m_cbKeyPair = 0;
    m_pbHash = NULL;
    m_cbHash = 0;
    m_tkAssembly = 0;
}

CAssemblyFile::CAssemblyFile(IMetaDataError *pError, CAsmLink* pLinker) : CFile( L"", (IMetaDataEmit*)NULL, pError, pLinker)
{
    ZeroInitAssembly();
    m_pAEmit = NULL;
    m_pAImport = NULL;
    m_isUBM = true;
    m_bAllowExes = true;
}

CAssemblyFile::CAssemblyFile(LPCWSTR pszPath, IMetaDataAssemblyImport* pAImporter, IMetaDataImport* pImporter, IMetaDataError *pError, CAsmLink* pLinker) : CFile(pszPath, pImporter, pError, pLinker)
{
    ZeroInitAssembly();
    pAImporter->AddRef();
    m_pAImport = pAImporter;
    m_pAEmit = NULL;
    m_isUBM = false;
    m_isStdLib = (m_Name != NULL && _wcsicmp( m_Name, L"mscorlib.dll") == 0);
}

CAssemblyFile::~CAssemblyFile()
{
    if (m_pAEmit) {
        m_pAEmit->Release();
        m_pAEmit = NULL;
    }
    if (m_pAImport) {
        m_pAImport->Release();
        m_pAImport = NULL;
    }

    if (m_Files.Count()) {
        for (DWORD l = 0; l < m_Files.Count(); l++) {
            if (m_Files[l])
                delete m_Files[l];
        }
    }
    CloseCryptoFile();

    SAFEFREE(m_szAsmName);
    SAFEFREE(m_pPubKey);
    m_cbPubKey = 0;
    SAFEFREE(m_adata.szLocale);
    SAFEFREE(m_adata.rOS);
    SAFEFREE(m_adata.rProcessor);
    memset(&m_adata, 0, sizeof(m_adata));
    SAFEFREE(m_Title);
    SAFEFREE(m_Description);
    SAFEFREE(m_Company);
    SAFEFREE(m_Product);
    SAFEFREE(m_ProductVersion);
    SAFEFREE(m_Copyright);
    SAFEFREE(m_Trademark);
    SAFEFREE(m_KeyName);
    SAFEFREE(m_KeyFile);
    SAFEFREE(m_pbHash);
    m_cbHash = 0;

}

void CAssemblyFile::PreClose()
{
    if (m_pAEmit) {
        m_pAEmit->Release();
        m_pAEmit = NULL;
    }
    if (m_pAImport) {
        m_pAImport->Release();
        m_pAImport = NULL;
    }

    if (m_Files.Count()) {
        for (DWORD l = 0; l < m_Files.Count(); l++) {
            if (m_Files[l])
                m_Files[l]->PreClose();
        }
    }

    CFile::PreClose();
}

HRESULT CAssemblyFile::AddFile(CFile *file, DWORD dwFlags, mdFile * tkFile)
{   
    HRESULT hr;

    if (!m_bAllowExes && file->m_bIsExe)
        return ReportError(ERR_CantAddExes, mdTokenNil, NULL, file->m_Path);
    if (!m_isUBM && file->m_isAssembly)
        return ReportError(ERR_CantAddAssembly, mdTokenNil, NULL, file->m_Path);

    file->m_isUBM |= m_isUBM; 
    if (FAILED(hr = m_Files.Add( file)))
        return ReportError(hr); // Out of memory

    // Don't emit the hash now
    if (m_pAEmit)
        hr = m_pAEmit->DefineFile(file->GetModuleName(), NULL, 0, dwFlags, &file->m_tkFile);
    if (tkFile)
        *tkFile = TokenFromRid( mdtFile, m_Files.Count() - 1);

    // Make sure everything stays sorted
    ASSERT(!m_pAEmit || m_Files.Count() == 1 || file->m_tkFile > m_Files.GetAt(m_Files.Count() - 2)->m_tkFile);
    return hr;
}

HRESULT CAssemblyFile::AddFile(CAssemblyFile *file, DWORD dwFlags, mdFile * tkFile)
{
    HRESULT hr;
    if (SUCCEEDED(hr = AddFile((CFile*)file, dwFlags, tkFile)) && tkFile)
        *tkFile = TokenFromRid( mdtAssemblyRef, m_Files.Count() - 1);
    return hr;
}

HRESULT CAssemblyFile::GetHash(const void ** ppvHash, DWORD *pcbHash)
{
    ASSERT( ppvHash && pcbHash);
    if (IsInMemory()) {
        // We can't hash an InMemory file
        *ppvHash = NULL;
        *pcbHash = 0;
        return S_FALSE;
    }

    if (!m_bDoHash || (m_cbHash && m_pbHash != NULL)) {
        *ppvHash = m_pbHash;
        *pcbHash = m_cbHash;
        return S_OK;
    }

    DWORD cchSize = 0, result;
    
    // AssemblyRefs ALWAYS use CALG_SHA1
    ALG_ID alg = CALG_SHA1;
    if (StrongNameHashSize( alg, &cchSize) == FALSE)
        return ReportError(HRESULT_FROM_WIN32(StrongNameErrorInfo()));

    if ((m_pbHash = (const BYTE *)VSAlloc(cchSize)) == NULL)
        return ReportError(E_OUTOFMEMORY);
    m_cbHash = cchSize;
    
    if ((result = GetHashFromAssemblyFileW(m_Path, &alg, (BYTE*)m_pbHash, cchSize, &m_cbHash)) != 0) {
        VSFree((void*)m_pbHash);
        m_pbHash = 0;
        m_cbHash = 0;
    }
    *ppvHash = m_pbHash;
    *pcbHash = m_cbHash;
    
    return result == 0 ? S_OK : ReportError(HRESULT_FROM_WIN32(result));
}

HRESULT CAssemblyFile::ImportAssembly(DWORD *pdwCountOfScopes, BOOL fImportTypes, IMetaDataDispenserEx *pDisp)
{
    ASSERT(m_pAImport != NULL);

    HRESULT hr = S_OK;
    DWORD chName = 0;
    void *pbOrig = NULL;
    DWORD cbOrig = 0;

    mdAssembly tkAsm;
    if (FAILED(hr = m_pAImport->GetAssemblyFromScope( &tkAsm)))
        // Don't ReportError because this is a COM+ error!
        return hr;
    if (FAILED(hr = m_pAImport->GetAssemblyProps( tkAsm, NULL, NULL, NULL, NULL, 0, &chName, &m_adata, NULL)))
        // Don't ReportError because this is a COM+ error!
        return hr;

    if ((m_szAsmName = (LPWSTR)VSAlloc( chName * sizeof(WCHAR))) == NULL)
        return ReportError(E_OUTOFMEMORY);
    if (m_adata.cbLocale) 
        m_adata.szLocale = (LPWSTR)VSAlloc(m_adata.cbLocale * sizeof(WCHAR));
    else
        m_adata.szLocale = NULL;
    if (m_adata.ulOS)
        m_adata.rOS = (OSINFO*)VSAlloc( m_adata.ulOS * sizeof(OSINFO));
    else
        m_adata.ulOS = NULL;
    if (m_adata.ulProcessor)
        m_adata.rProcessor = (ULONG*)VSAlloc( m_adata.ulProcessor * sizeof(ULONG));
    else
        m_adata.rProcessor = NULL;

    if (FAILED(hr = m_pAImport->GetAssemblyProps( tkAsm, (const void**)&pbOrig, &cbOrig, NULL, (LPWSTR)m_szAsmName, chName, NULL, &m_adata, NULL)))
        // Don't ReportError because this is a COM+ error!
        return hr;

    if (cbOrig > 0) {
        if (!StrongNameTokenFromPublicKey( (PBYTE)pbOrig, cbOrig, (PBYTE*)&pbOrig, &cbOrig))
            return ReportError(HRESULT_FROM_WIN32(StrongNameErrorInfo()));
    }
    if ((m_pPubKey = (PublicKeyBlob*)VSAlloc(cbOrig)) == NULL)
        return ReportError(E_OUTOFMEMORY);
    memcpy(m_pPubKey, pbOrig, cbOrig);
    m_cbPubKey = cbOrig;
    if (cbOrig > 0)
        StrongNameFreeBuffer( (BYTE*)pbOrig);


    // Get the CAs
    if (FAILED(hr = ReadCAs(tkAsm, NULL, FALSE, FALSE))) // We don't know and we don't care for the import case.
        return hr;
    // The manifest file always imports it's types implicitly
    if (fImportTypes) {
        if (FAILED(hr = ImportFile(NULL, NULL)))
            return hr;
    }

    HCORENUM enumFiles;
    mdFile filedefs[32];
    ULONG cFiledefs, iFiledef;
    WCHAR *FileName = NULL, *filepart = NULL;
    DWORD len, cchName, dwFlags;

    cchName = (DWORD)wcslen(m_Path) + MAX_PATH;
    FileName = (LPWSTR)_alloca(sizeof(WCHAR) * cchName);
    wcscpy(FileName, m_Path);
    filepart = wcsrchr(FileName, L'\\');
    if (filepart)
        filepart++;
    else
        filepart = FileName;

    len = cchName - (DWORD)(filepart - FileName);
    
    ASSERT(m_Files.Count() == 0);

    // Enumeration all the Files in this assembly.
    enumFiles= 0;
    do {
        // Get next batch of files.
        hr = m_pAImport->EnumFiles(&enumFiles, filedefs, lengthof(filedefs), &cFiledefs);
        if (FAILED(hr))
            // Don't ReportError because this is a COM+ error!
            break;

        // Process each file.
        for (iFiledef = 0; iFiledef < cFiledefs && SUCCEEDED(hr); ++iFiledef) {
            CFile *file = NULL;
            hr = m_pAImport->GetFileProps( filedefs[iFiledef], filepart, len, &cchName, NULL, NULL, &dwFlags);
            if (FAILED(hr))
                // Don't ReportError because this is a COM+ error!
                break;

            if (IsFfContainsMetaData(dwFlags)) {
                IMetaDataImport* pImport = NULL;
                hr = pDisp->OpenScope( FileName, ofRead | ofNoTypeLib, IID_IMetaDataImport, (IUnknown**)&pImport);
                if (SUCCEEDED(hr)) {
                    file = new CFile( FileName, pImport, m_pError, m_pLinker);
                    pImport->Release();
                    if (FAILED(hr = m_Files.Add(file)))
                        hr = ReportError(hr);
                } else {
                    hr = ReportError(ERR_AssemblyModuleImportError, m_tkError, NULL, m_Path, filepart, ErrorHR(hr, pDisp, IID_IMetaDataDispenserEx));
                }
            } else {
                file = new CFile( FileName, (IMetaDataEmit*)NULL, m_pError, m_pLinker);
                file->m_isDeclared = file->m_isCTDeclared = true;
                if (FAILED(hr = m_Files.Add(file)))
                    hr = ReportError(hr);
            }
            if (FAILED(hr) && file) {
                delete file;
                file = NULL;
            } else if (file) {
                file->m_tkError = file->m_tkFile = filedefs[iFiledef];
            }
        }
    } while (cFiledefs > 0 && SUCCEEDED(hr));
    
    m_pImport->CloseEnum(enumFiles);
    if (pdwCountOfScopes) *pdwCountOfScopes = (DWORD)m_Files.Count() + 1;
    if (FAILED(hr) || fImportTypes == FALSE)
        return hr;

    HCORENUM enumTypes;
    mdExportedType typedefs[32];
    ULONG cTypedefs, iTypedef;
    mdToken tkImpl;
    mdTypeDef tkTypeDef;
    Type *newType;

    // Enumeration all the ComTypes in this assembly.
    enumTypes= 0;
    do {
        // Get next batch of ComTypes.
        hr = m_pAImport->EnumExportedTypes(&enumTypes, typedefs, lengthof(typedefs), &cTypedefs);
        if (FAILED(hr))
            // Don't ReportError because this is a COM+ error!
            break;

        CFile *KeyFile = new CFile( L"$$LookUpFile$$", (IMetaDataEmit*)NULL, m_pError, m_pLinker);
        // Process each Type.
        for (iTypedef = 0; iTypedef < cTypedefs; ++iTypedef) {
            hr = m_pAImport->GetExportedTypeProps( typedefs[iTypedef], NULL, 0, NULL,
                &tkImpl, &tkTypeDef, &dwFlags);
            if ((IsTdPublic(dwFlags) || IsTdNestedPublic(dwFlags)) &&
                TypeFromToken(tkImpl) == mdtFile && !IsNilToken(tkImpl)) {
                KeyFile->m_tkFile = tkImpl;
                CFile **file = (CFile**)bsearch(&KeyFile, m_Files.Base(), m_Files.Count(), sizeof(CFile*), (int (__cdecl*)(const void*,const void*))CFile::CompareFiles);
                if (file == NULL || *file == NULL || (*file)->m_pImport == NULL) {
                    hr = ReportError(ERR_InvalidFileDefInComType, mdTokenNil, NULL, typedefs[iTypedef], tkImpl);
                } else {
                    if (FAILED(hr = (*file)->m_Types.Add( NULL, &newType)))
                        ReportError(hr); // Out of memory
                }
                if (SUCCEEDED(hr)) {
                    newType->TypeDef = tkTypeDef;
                    newType->ComType = typedefs[iTypedef];
                }
            }
        }
        delete KeyFile;
    } while (cTypedefs > 0 && SUCCEEDED(hr));
    
    m_pImport->CloseEnum(enumTypes);

    return hr;
}

HRESULT CAssemblyFile::GetFile(DWORD index, CFile** file)
{
    if (!file)
        return E_POINTER;

    if (RidFromToken(index) < m_Files.Count()) {
        if ((*file = m_Files.GetAt(RidFromToken(index))))
            return S_OK;
    }
    return ReportError(E_INVALIDARG);
}

HRESULT CAssemblyFile::RemoveFile(DWORD index)
{
    if (RidFromToken(index) < m_Files.Count()) {
        m_Files.Base()[RidFromToken(index)] = NULL;
        return S_OK;
    }
    return ReportError(E_INVALIDARG);
}

HRESULT CAssemblyFile::AddExportType(mdToken tkParent, mdTypeDef tkTypeDef, LPCWSTR pszTypeName, DWORD flags, mdExportedType *pType)
{
    return m_pAEmit->DefineExportedType(pszTypeName, tkParent, tkTypeDef, flags, pType);
}

HRESULT CAssemblyFile::AddResource(mdToken Location, LPCWSTR pszName, DWORD offset, DWORD flags)
{
    mdManifestResource mr = mdTokenNil;
    return m_pAEmit->DefineManifestResource( pszName, Location, offset, flags, &mr);
}

BSTR CAssemblyFile::ExtractString(const void * pvData, DWORD * cbData)
{
    // String is stored as compressed length, UTF8.
    if (*(const BYTE*)pvData == 0xFF) {
        pvData = (const BYTE*)pvData + 1;
        return SysAllocStringLen(NULL, 0);
    } else {
        PCCOR_SIGNATURE sig = (PCCOR_SIGNATURE)pvData;
        *cbData -= CorSigUncompressedDataSize(sig);
        ASSERT(*cbData >= 0);
        DWORD len = CorSigUncompressData(sig);
        pvData = (const void *)sig;
        ASSERT(*cbData >= len);
        DWORD cchLen = UnicodeLengthOfUTF8((PCSTR)pvData, len);
        LPWSTR str = (LPWSTR)_alloca(sizeof(WCHAR)*cchLen);
        UTF8ToUnicode((const char*)pvData, len, str, cchLen);
        pvData = (const BYTE*)pvData + len;
        *cbData -= len;
        return SysAllocStringLen(str, cchLen);
    }
}

HRESULT CAssemblyFile::AddCustomAttribute(mdToken tkType, const void* pValue, DWORD cbValue, BOOL bSecurity, BOOL bAllowMultiple, CFile *source)
{
    ASSERT(m_pEmit != NULL);
    ASSERT(lengthof(OptionCAs) == optLastAssemOption + 1);
    CA ca;
    CA *pca = &ca;
    HRESULT hr = S_OK;
    int opt;

    if (m_isUBM)
        return CFile::AddCustomAttribute(tkType, pValue, cbValue, bSecurity, bAllowMultiple, source);

    if (source == NULL)
        source = this;
    
    // check to see if this is a 'special' CA
    LPWSTR name = NULL;
    if (FAILED(hr = source->GetCAName(tkType, &name)))
        return hr;
    
    opt = FindOption(name);

#ifdef _DEBUG
    if (opt != optLastAssemOption) {
        ASSERT(bAllowMultiple == ((OptionCAs[opt].flag & 0x04) == 0x04));
        ASSERT(bSecurity == ((OptionCAs[opt].flag & 0x08) == 0x08));
    }
#endif

    if (OptionCAs[opt].flag & 0x02) {
        // This attribute is a real attribute
        if (source != this) {
            if (FAILED(hr = ConvertCAToken( &tkType, source))) {
                delete [] name;
                return hr;
            }
        }
        // Just a normal CA
        ca.tkCA = mdTokenNil;
        ca.cbVal = cbValue;
        ca.tkType = tkType;
        ca.pVal = static_cast<BYTE *>(const_cast<void *>(pValue));
        ca.bSecurity = (bSecurity != FALSE);
        ca.bAllowMulti = (bAllowMultiple != FALSE);

        UINT count = m_CAs.Count();
        CA *found = NULL;
        if (ca.bAllowMulti) {
            // Check for an exact duplicate
            found = (CA*)_lfind(&ca, m_CAs.Base(), &count, sizeof(CA), CA::CompareVal);
            ASSERT(count == m_CAs.Count());

            if (found == NULL) {
                // It allows multiples and an identical one was not found, so add it
                hr = m_CAs.Add( NULL, &pca);
                if (SUCCEEDED(hr)) {
                    pca->tkCA = mdTokenNil;
                    pca->cbVal = cbValue;
                    pca->tkType = tkType;
                    pca->bSecurity = (bSecurity != FALSE);
                    pca->bAllowMulti = (bAllowMultiple != FALSE);
                    pca->pVal = new BYTE[pca->cbVal];
                    memcpy(pca->pVal, pValue, pca->cbVal);
                } else {
                    hr = ReportError(hr);
                }
            } else
                hr = META_S_DUPLICATE;
        } else {
            // No duplicates allowed
            found = (CA*)_lfind(&ca, m_CAs.Base(), &count, sizeof(CA), CA::CompareToken);
            ASSERT(count == m_CAs.Count());
            if (found == NULL) {
                // Add it
                hr = m_CAs.Add( NULL, &pca);
                if (SUCCEEDED(hr)) {
SETVALUE:
                    pca->tkCA = mdTokenNil;
                    pca->cbVal = cbValue;
                    pca->tkType = tkType;
                    pca->bSecurity = (bSecurity != FALSE);
                    pca->bAllowMulti = (bAllowMultiple != FALSE);
                    pca->pVal = new BYTE[pca->cbVal];
                    memcpy(pca->pVal, pValue, pca->cbVal);
                } else {
                    hr = ReportError(hr);
                }
            } else {
                // If the one found is identical, then ignore it, otherwise give an error
                if (CA::CompareVal(&ca, found) != 0) {
                    if (opt < optLastAssemOption) {
                        // It's a known CA, so replace it
                        hr = S_FALSE;
                        VSFree(found->pVal);
                        pca = found;
                        goto SETVALUE;
                    } else
                        hr = ReportError(ERR_DuplicateCA, tkType, NULL, name);
                } else
                    hr = META_S_DUPLICATE;
            }
        }
    }
    delete [] name;

    if (SUCCEEDED(hr) && (OptionCAs[opt].flag & 0x01)) {
        // This is a 'known' CA
        const BYTE * pvData = (const BYTE *)pValue;
        DWORD  cbSize = cbValue;
        VARIANT var;
        
        // Make sure that data is in the correct format. Check the prolog, then
        // move beyond it.
        if (cbSize < sizeof(WORD) || GET_UNALIGNED_VAL16(pvData) != 1)
            return ReportError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
        pvData += sizeof(WORD);
        cbSize -= sizeof(WORD);
        VariantInit(&var);

        switch(OptionCAs[opt].vt) {
        case VT_BSTR:
            if (opt == optAssemOS) {
                ULONG temp = 0;
                if (cbSize < sizeof(ULONG)) {
                    hr = ReportError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
                    break;
                }
                temp = VAL32((*(ULONG*)pvData));
                cbSize -= sizeof(ULONG);
                pvData += sizeof(ULONG);
                CComBSTR str;
                str.Attach(ExtractString(pvData, &cbSize));
                LPWSTR os = (LPWSTR)_alloca(sizeof(WCHAR)*(str.Length() + 12));
                int len = swprintf(os, L"%d.", temp);
                memcpy(os + len, str, str.Length()*sizeof(WCHAR));
                V_VT(&var) = VT_BSTR;
                V_BSTR(&var) = SysAllocStringLen(os, len + SysStringLen(str));
                SysFreeString(str);
            } else {
                V_VT(&var) = VT_BSTR;
                V_BSTR(&var) = ExtractString(pvData, &cbSize);
            }
            hr = SetOption((AssemblyOptions)opt, &var);
            break;
        case VT_BOOL:
            if (cbSize < sizeof(BYTE)) {
                hr = ReportError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
                break;
            }
            V_VT(&var) = VT_BOOL;
            V_BOOL(&var) = ((*(BYTE*)pvData) ? VARIANT_TRUE : VARIANT_FALSE);
            cbSize -= sizeof(BYTE);
            pvData += sizeof(BYTE);
            hr = SetOption((AssemblyOptions)opt, &var);
            break;

        case VT_UI4:
            if (cbSize < sizeof(ULONG)) {
                hr = ReportError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
                break;
            }
            V_VT(&var) = VT_UI4;
            V_UI4(&var) = GET_UNALIGNED_VAL32(pvData);
            cbSize -= sizeof(ULONG);
            pvData += sizeof(ULONG);
            hr = SetOption((AssemblyOptions)opt, &var);
            break;

        default:
            hr = ReportError(E_INVALIDARG);
        }
        VariantClear(&var);
    }

    if (bAllowMultiple && hr == S_FALSE)
        hr = S_OK;

    return hr;
}

HRESULT CAssemblyFile::SetOption(AssemblyOptions option, VARIANT *pvalue)
{
    HRESULT hr = S_OK;
    LPWSTR *target = NULL;
    LPWSTR str = NULL;
    size_t len = 0;
    bool bCheckVersionString = false;
    bool bRequireVersionString = false;
    if (option >= optLastAssemOption || OptionCAs[option].flag & 0x40)
        return E_INVALIDARG;

    switch(option) {
    case optAssemTitle:
        target = (LPWSTR*)&m_Title;
        break;
    case optAssemDescription:
        target = (LPWSTR*)&m_Description;
        break;
    case optAssemConfig:
        VSFAIL("We should never be setting optAssemConfig!");
        return E_INVALIDARG;
    case optAssemLocale:
        target = (LPWSTR*)&m_adata.szLocale;
        break;
    case optAssemCompany:
        target = (LPWSTR*)&m_Company;
        break;
    case optAssemProduct:
        target = (LPWSTR*)&m_Product;
        break;
    case optAssemProductVersion:
        target = (LPWSTR*)&m_ProductVersion;
        bCheckVersionString = true;
        break;
    case optAssemSatelliteVer:
        target = &str;
        bCheckVersionString = true;
        bRequireVersionString = true;
        break;
    case optAssemCopyright:
        target = (LPWSTR*)&m_Copyright;
        break;
    case optAssemTrademark:
        target = (LPWSTR*)&m_Trademark;
        break;
    case optAssemKeyFile:
        target = (LPWSTR*)&m_KeyFile;
        break;
    case optAssemOS:
    case optAssemVersion:
        target = &str;
        break;
    case optAssemProcessor:      // ULONG
        hr = VariantChangeType( pvalue, pvalue, 0, VT_UI4);
        if (FAILED(hr))
            return ReportError(hr);
        if (m_adata.ulProcessor > 0) {
            hr = S_FALSE;
            // Don't add duplicates
            for (ULONG u = 0; u < m_adata.ulProcessor; u++) {
                if (m_adata.rProcessor[u] == V_UI4(pvalue))
                    return S_FALSE;
            }
            m_adata.rProcessor = (ULONG*)VSRealloc( m_adata.rProcessor, (m_adata.ulProcessor + 1) * sizeof(ULONG));
        } else {
            m_adata.rProcessor = (ULONG*)VSAlloc( sizeof(ULONG));
        }
        m_adata.rProcessor[m_adata.ulProcessor++] = V_UI4(pvalue);
        return hr;
    case optAssemAlgID:       // ULONG
        hr = VariantChangeType( pvalue, pvalue, 0, VT_UI4);
        if (FAILED(hr))
            return ReportError(hr);
        if (SUCCEEDED(hr) && m_isAlgIdSet && m_iAlgID != V_UI4(pvalue))
            hr = S_FALSE;
        m_iAlgID = V_UI4(pvalue);
        m_isAlgIdSet = true;
        return hr;
    case optAssemFlags:       // ULONG
        hr = VariantChangeType( pvalue, pvalue, 0, VT_UI4);
        if (FAILED(hr))
            return ReportError(hr);
        if (SUCCEEDED(hr) && m_isFlagsSet && m_dwFlags != V_UI4(pvalue))
            hr = S_FALSE;
        m_dwFlags = V_UI4(pvalue);
        m_isFlagsSet = true;
        return hr;
    case optAssemHalfSign:       // Bool
        hr = VariantChangeType( pvalue, pvalue, 0, VT_BOOL);
        if (FAILED(hr))
            return ReportError(hr);
        if (SUCCEEDED(hr) && m_isHalfSignSet && m_doHalfSign != (V_BOOL(pvalue) != FALSE))
            hr = S_FALSE;
        m_doHalfSign = (V_BOOL(pvalue) != FALSE);
        m_isHalfSignSet = true;
        return hr;
    default:
        return E_INVALIDARG;
    }
    if (target != NULL) {
        hr = VariantChangeType(pvalue, pvalue, 0, VT_BSTR);
        if (FAILED(hr))
            return ReportError(hr);
        // Need to support embedded NULLs
        len = SysStringLen(V_BSTR(pvalue));
        if ((OptionCAs[option].flag & 0x10) && len > MAX_PATH)
            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        if (*target != NULL) {
            if (memcmp(*target, V_BSTR(pvalue), len * sizeof(WCHAR)) == 0 && len == wcslen(*target))
                // It's a dupe, but it has the same value, so it's OK
                return S_OK;
            hr = S_FALSE;
            VSFree(*target);
        }
        *target = (LPWSTR)memcpy(VSAlloc((len+1)*sizeof(WCHAR)), V_BSTR(pvalue), sizeof(WCHAR)*len);
        (*target)[len] = L'\0';
        if (option == optAssemLocale) {
            m_adata.cbLocale = (ULONG)len;
            if (m_bIsExe && m_adata.cbLocale > 0)
                hr = ReportError(ERR_ExeHasCulture);
        }
    }

    // String encoded as: "Major.Minor.Build.Revision"
    ULONG val[4] = {0,0,0,0};
    LPWSTR internal = NULL;
    LPWSTR end = NULL;

    if (option == optAssemVersion) {
        // String encoded as: "Major[.Minor[.Build[.Revision]]]"
        bool BadTime = false;
        internal = str;
        hr = ERR_InvalidVersionFormat;

        for (int i = 0; i < 4; i++) {
            if (i > 1 && *internal == L'*' && internal + 1 == str + len) {
                // Auto-gen
                WORD rev, bld;
                hr = MakeVersion( i == 2 ? &rev : NULL, &bld);
                if (hr == E_FAIL)
                    BadTime = true;
                else if (FAILED(hr))
                    goto SUCCEED;
                // all done
                val[3] = bld;
                if (i == 2)
                    val[2] = rev;
                break;
            }
            if (iswspace(*internal) || *internal == L'-' || *internal == L'+')
                // we don't allow these
                goto FAIL;
            val[i] = wcstoul( internal, &end, 10); // Allow only decimal
            if (end == internal || val[i] >= USHRT_MAX)  // 65535 is not valid (because of metadata restrictions)
                // We didn't parse anything, or it wasn't valid
                goto FAIL;
            if (end == str + len)
                // we're done
                break;
            if (*end != L'.' || i == 3)
                // Need a dot to continue (or this should have been the end)
                goto FAIL;
            internal = end + 1;
        }

        VSFree(str);
        if (m_isVersionSet &&
            (m_adata.usMajorVersion != (unsigned short)val[0] ||
             m_adata.usMinorVersion != (unsigned short)val[1] ||
             m_adata.usRevisionNumber != (unsigned short)val[3] ||
             m_adata.usBuildNumber != (unsigned short)val[2]))
            hr = BadTime ? ReportError(WRN_InvalidTime, NULL, NULL) : S_FALSE;
        else
            hr = BadTime ? ReportError(WRN_InvalidTime, NULL, NULL) : S_OK;
    
        m_adata.usMajorVersion = (unsigned short)val[0];
        m_adata.usMinorVersion = (unsigned short)val[1];
        m_adata.usRevisionNumber = (unsigned short)val[3];
        m_adata.usBuildNumber = (unsigned short)val[2];
        m_isVersionSet = true;
    } else if (option == optAssemOS) {
        // String encoded as: "dwOSPlatformId.dwOSMajorVersion.dwOSMinorVersion"
        internal = str;

        hr = ERR_InvalidOSString;
        for (int i = 0; i < 3; i++) {
            if (iswspace(*internal) || *internal == L'-' || *internal == L'+')
                // we don't allow these
                goto FAIL;
            val[i] = wcstoul( internal, &end, 10); // Allow only decimal
            if (end == internal || (val[i] == ULONG_MAX && errno == ERANGE)) 
                // We didn't parse anything, or it wasn't valid
                goto FAIL;
            if (end == str + len)
                // we're done
                break;
            if (*end != L'.' || i == 2)
                // Need a dot to continue (or this should have been the end)
                goto FAIL;
            internal = end + 1;
        }

        if (m_adata.ulOS > 0) {
            hr = S_FALSE;
            for (ULONG u = 0; u < m_adata.ulOS; u++) {
                if (m_adata.rOS[u].dwOSPlatformId == val[0] &&
                    m_adata.rOS[u].dwOSMajorVersion == val[1] &&
                    m_adata.rOS[u].dwOSMinorVersion == val[2])
                    goto SUCCEED;
            }
            m_adata.rOS = (OSINFO*)VSRealloc( m_adata.rOS, (m_adata.ulOS + 1) * sizeof(OSINFO));
        } else {
            hr = S_OK;
            m_adata.rOS = (OSINFO*)VSAlloc( sizeof(OSINFO));
        }
        m_adata.rOS[m_adata.ulOS].dwOSPlatformId = val[0];
        m_adata.rOS[m_adata.ulOS].dwOSMajorVersion = val[1];
        m_adata.rOS[m_adata.ulOS++].dwOSMinorVersion = val[2];
        goto SUCCEED;
FAIL:
        if (hr != S_OK)
            hr = ReportError((ERRORS)hr, mdTokenNil, NULL, V_BSTR(pvalue));
SUCCEED:
        VSFree(str);
    } else if (bCheckVersionString) {
        // String encoded as: "Major.Minor.Build.Revision"
        internal = *target;

        for (int i = 0; i < 4; i++) {
            if (iswspace(*internal) || *internal == L'-' || *internal == L'+')
                // we don't allow these
                break;
            val[i] = wcstoul( internal, &end, 10); // Allow only decimal
            if (end == internal || (bRequireVersionString ? (val[i] >= USHRT_MAX) : (val[i] > USHRT_MAX)))  // 65535 is not valid (because of metadata restrictions)
                // We didn't parse anything, or it wasn't valid
                break;
            if (end == *target + len) {
                // we're done
                goto OK_VERSION;
            }
            if (*end != L'.' || i == 3)
                // Need a dot to continue (or this should have been the end)
                break;
            internal = end + 1;
        }
        if (bRequireVersionString)
            hr = ReportError(ERR_InvalidVersionString, mdTokenNil, NULL, ErrorOptionName(option), *target);
        else {
            bool bReported = false;
            HRESULT hr2 = ReportError(WRN_InvalidVersionString, mdTokenNil, &bReported, ErrorOptionName(option), *target);
            if (!bReported)
                hr = hr2;
        }
OK_VERSION:

        if (option == optAssemSatelliteVer)
            VSFree(str);
    }

    return hr;
}

HRESULT CAssemblyFile::MakeRes(CFile *file, BOOL fDll, LPCWSTR pszIconFile, const void **ppBlob, DWORD *pcbBlob)
{
    WORD version[4];
    const void * pBlob = NULL;
    DWORD cbBlob = 0;

    version[0] = m_adata.usMajorVersion;
    version[1] = m_adata.usMinorVersion;
    version[2] = m_adata.usBuildNumber;
    version[3] = m_adata.usRevisionNumber;

    *ppBlob = NULL;
    *pcbBlob = 0;

    if (pszIconFile && wcslen(pszIconFile) > MAX_PATH) {
        WCHAR shortName[1024];
        size_t len = wcslen(pszIconFile);
        if (len >= lengthof(shortName)) {
            shortName[0] = shortName[1] = shortName[2] = L'.';
            len -= (lengthof(shortName) - 4);
            ASSERT( lengthof(shortName) > wcslen(pszIconFile) - len);
            wcscpy( shortName + 3, pszIconFile + len);
            pszIconFile = shortName;
        }
        return ReportError( ERR_FileNameTooLong, 0, NULL, pszIconFile);
    }


    *pcbBlob = cbBlob = 0;
    *ppBlob = pBlob = NULL;


    return S_OK;
}

HRESULT CAssemblyFile::EmitManifest(DWORD *pdwReserveSize, mdAssembly *ptkAssembly)
{
    ASSERT(pdwReserveSize);
    // Emit the Manifest

    BOOLEAN b = FALSE;
    HRESULT hr;
    
    // Check for more CAs that might have gotten merged in
    hr = ImportCAs(this);
    if (FAILED(hr))
        return hr;

    *pdwReserveSize = 0;
    if (FAILED(hr = ReadCryptoKey(&m_pPubKey, m_cbPubKey)))
        return hr;

    if (!m_isAlgIdSet) {
        m_iAlgID = CALG_SHA1;
        m_isAlgIdSet = true;
    }

    // Make sure we have 'created' the assembly name
    GetAsmName();
    hr = m_pAEmit->DefineAssembly( m_pPubKey, m_cbPubKey, m_iAlgID, m_szAsmName,
        &m_adata, m_dwFlags, &m_tkAssembly);

    if (hr == META_S_DUPLICATE) {
        // Incremental Update and they want to change the Assembly Info
        void * PK = m_pPubKey;
        if (m_cbPubKey == 0) {
            // if PK is NULL, the metadata will leave the old value there!
            ASSERT(PK == NULL);
            PK = &m_cbPubKey;
        }
        hr = m_pAEmit->SetAssemblyProps( m_tkAssembly, PK, m_cbPubKey, m_iAlgID,
            m_szAsmName, &m_adata, m_dwFlags);
    }

    // NOTE: we can't put the digest at the begining of the file because 
    // the APIs don't like a RVA of 0.
    // Just get the size of the digest
    if (SUCCEEDED(hr) && m_pPubKey) {

        if (pdwReserveSize) {
            b = StrongNameSignatureSize( (PBYTE)m_pPubKey, m_cbPubKey, pdwReserveSize);
            if (b == FALSE) {
                *pdwReserveSize = 0;
                hr = ReportError(HRESULT_FROM_WIN32(StrongNameErrorInfo()));
            }
        }
    } else if (pdwReserveSize) {
        *pdwReserveSize = 0;
    }

    if (SUCCEEDED(hr) && ptkAssembly)
        *ptkAssembly = m_tkAssembly;

    return hr;
}

HRESULT CAssemblyFile::EmitAssembly(IMetaDataDispenserEx *pDisp)
{
    // Emit all the cached CAs, the imported CAs, and the hashes of files, and the global Refs
    if (m_tkAssembly == 0)
        // Somehow an error prevented us from emitting an assembly,
        // so don't compound the problem further, just quit.
        return S_FALSE;

    DWORD i, l;
    BOOLEAN b = FALSE;
    PBYTE pHash;
    DWORD cbHash, ccbHash;
    HRESULT hr = S_OK;
    DWORD SecurityCAs = 0;

    for (i = 0, l = m_CAs.Count(); i < l; i++) {
        CA *temp = m_CAs.GetAt(i);
        if (temp->bSecurity) {
            SecurityCAs++;
        }
        else {
            hr = m_pEmit->DefineCustomAttribute( m_tkAssembly, temp->tkType, temp->pVal, temp->cbVal, &temp->tkCA);
            if (FAILED(hr)) {
                LPWSTR buffer = NULL;
                if (SUCCEEDED(GetCAName(temp->tkType, &buffer)))
                    hr = ReportError(ERR_EmitCAFailed, temp->tkCA, NULL, buffer, ErrorHR(hr, m_pEmit, IID_IMetaDataEmit));
                delete [] buffer;
                return hr;
            }
        }
    }

    if (SecurityCAs > 0) {
        // Emit the security CAs
        DWORD j, Err;
        COR_SECATTR *attrs = (COR_SECATTR*)VSAlloc(sizeof(COR_SECATTR) * SecurityCAs);
        if (attrs == NULL)
            return ReportError(E_OUTOFMEMORY);
        for (i = j = 0, l = m_CAs.Count(); i < l && j < SecurityCAs; i++) {
            CA *temp = m_CAs.GetAt(i);
            if (temp->bSecurity) {
                attrs[j].tkCtor = temp->tkType;
                attrs[j].pCustomAttribute = temp->pVal;
                attrs[j].cbCustomAttribute = temp->cbVal;
                j++;
            }
        }
        hr = m_pEmit->DefineSecurityAttributeSet( m_tkAssembly, attrs, j, &Err);
        VSFree(attrs);
        if (FAILED(hr))
            // Don't Report the error (COM+ already did)
            return hr;
    }

    if (m_Files.Count() > 0) {
        b = StrongNameHashSize( m_iAlgID, &cbHash);
        if (b == FALSE)
            return ReportError(ERR_CryptoHashFailed, mdTokenNil, NULL, ErrorHR(HRESULT_FROM_WIN32(StrongNameErrorInfo())));
    
        pHash = (PBYTE)VSAlloc(cbHash);
        if (pHash == NULL)
            return E_OUTOFMEMORY;
        for (i = 0, l = m_Files.Count(); i < l; i++) {
            CFile *temp = m_Files.GetAt(i);
    
            // Copy all the AssemblyRefs
            if (temp->GetImportScope()) {
                IMetaDataAssemblyImport *asrc = NULL;
                // But only if we have metadata
                if (SUCCEEDED(hr = temp->GetImportScope()->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&asrc))) {
                    HCORENUM hEnum = 0;
                    mdAssemblyRef mdAR[8];
                    DWORD cAR = 0;
    
                    do {
                        if (SUCCEEDED(hr = asrc->EnumAssemblyRefs( &hEnum, mdAR, lengthof(mdAR), &cAR))) {
                            for (DWORD j = 0; j < cAR && SUCCEEDED(hr); j++) {
                                hr = DupAssemblyRef( &mdAR[j], asrc);
                            }
                        }
                    } while (SUCCEEDED(hr) && cAR != 0);
                    asrc->CloseEnum(hEnum);
                    asrc->Release();
                    if (FAILED(hr))
                        break;
                } else {
                    hr = temp->ReportError(hr);
                    break;
                }
            }
    
            // Finish the files
            if (FAILED(hr = temp->CopyFile(pDisp, m_bClean)))
                break;
            HANDLE hFile = OpenFileEx(temp->m_Path, NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                hr = ReportError(HRESULT_FROM_WIN32(GetLastError()), temp->m_tkError);
                break;
            }
            DWORD result = GetHashFromHandle(hFile, &m_iAlgID, pHash, cbHash, &ccbHash);
            CloseHandle(hFile);
            if (FAILED(hr = HRESULT_FROM_WIN32(result))) {
                hr = ReportError(ERR_CryptoHashFailed, temp->m_tkError, NULL, ErrorHR(hr));
                break;
            }
            ASSERT(cbHash == ccbHash);
            if (FAILED(hr = m_pAEmit->SetFileProps(temp->m_tkFile, pHash, ccbHash, ULONG_MAX))) // ULONG_MAX = Don't change
                break;
        }
        VSFree(pHash);
    }

    // Check references
    if (SUCCEEDED(hr)) {
        if (!m_pAImport)
            hr = m_pAEmit->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&m_pAImport);

        if (m_pAImport) {
            HCORENUM hEnum = 0;
            mdAssemblyRef mdAR[8];
            DWORD cAR = 0, j;

            do {
                if (SUCCEEDED(hr = m_pAImport->EnumAssemblyRefs( &hEnum, mdAR, lengthof(mdAR), &cAR))) {
                    for (j = 0; j < cAR && SUCCEEDED(hr); j++) {
                        const void * pbPubKey = NULL;
                        ULONG cbPubKey = 0, chName = 0;
                        ASSEMBLYMETADATA data;
                        memset(&data, 0, sizeof(data));
                        if (SUCCEEDED(hr = m_pAImport->GetAssemblyRefProps( mdAR[j], &pbPubKey, &cbPubKey,
                            NULL, 0, &chName, &data, NULL, NULL, NULL))) {

                            ASSERT(data.cbLocale != 1); // This would just be a "\0" right?
                            bool bRefNotStrong = (m_pPubKey && (pbPubKey == NULL || cbPubKey == 0)); // This dependant is not strongly-named, but we are?
                            bool bRefHasCulture = (data.cbLocale > 1); // This dependant has a non-empty Culture setting

                            if (bRefNotStrong || bRefHasCulture) {
                                bool bReported = false;
                                LPWSTR wszName = (LPWSTR)_alloca(sizeof(WCHAR)*(chName+1));
                                if (FAILED(hr = m_pAImport->GetAssemblyRefProps( mdAR[j], NULL, NULL,
                                    wszName, chName, NULL, NULL, NULL, NULL, NULL))) {
                                    wszName[0] = L'\0';
                                }
                                
                                if (bRefNotStrong) {
                                    HRESULT hr2 = ReportError( ERR_RefNotStrong, mdAR[j], &bReported, wszName);
                                    if (!bReported)
                                        hr = hr2;
                                }
                                if (SUCCEEDED(hr) && bRefHasCulture) {
                                    HRESULT hr2 = ReportError( WRN_RefHasCulture, mdAR[j], &bReported, wszName);
                                    if (!bReported)
                                        hr = hr2;
                                }
                            }
                        }
                    }
                }
            } while (SUCCEEDED(hr) && cAR != 0);
            m_pAImport->CloseEnum(hEnum);
        }
    }

    return hr;
}

HRESULT CAssemblyFile::ConvertCAToken(mdToken *tkCA, CFile *src)
{
    HRESULT hr = S_OK;
    mdToken parent;

    if (TypeFromToken(*tkCA) == mdtMemberRef) {
        DWORD cchMember;
        LPWSTR member;
        PCCOR_SIGNATURE sig = NULL;
        ULONG cbSig = 0;

        if (FAILED(hr = src->GetImportScope()->GetMemberRefProps( *tkCA, &parent, NULL, 0, &cchMember, NULL, NULL)))
            return hr;
        member = (LPWSTR)_alloca(sizeof(WCHAR)*cchMember);
        if (FAILED(hr = src->GetImportScope()->GetMemberRefProps( *tkCA, NULL, member, cchMember, &cchMember, &sig, &cbSig)))
            return hr;
        
        if (FAILED(hr = CopyType(&parent, src)))
            return hr;

        ULONG cbNewSig = cbSig * 2; // Can't be more than twice original size right?
        PCOR_SIGNATURE newSig = (PCOR_SIGNATURE)_alloca(cbNewSig);
        if (FAILED(hr = m_pEmit->TranslateSigWithScope(NULL, NULL, 0, src->GetImportScope(), sig, cbSig,
            m_pAEmit, m_pEmit, newSig, cbNewSig, &cbNewSig)))
            return hr;

        if (TypeFromToken(parent) == mdtTypeDef) {
            // It's defined in this file!?!?
            hr = m_pImport->FindMember(parent, member, newSig, cbNewSig, tkCA);
        } else {
            hr = m_pImport->FindMemberRef(parent, member, newSig, cbNewSig, tkCA);
            if (hr == CLDB_E_RECORD_NOTFOUND) {
                // Make a memberref
                hr = m_pEmit->DefineMemberRef(parent, member, newSig, cbNewSig, tkCA);
            }
        }
    } else if (TypeFromToken(*tkCA) == mdtTypeRef) {
        // A TypeRef
        hr = CopyType(tkCA, src);
    } else {
        // This must be a Def
        ASSERT(TypeFromToken(*tkCA) == mdtMethodDef);
        if (FAILED(hr = src->GetImportScope()->GetMemberProps( *tkCA, &parent, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL)))
            return hr;

        ASSERT(TypeFromToken(parent) == mdtTypeDef);
        if (FAILED(hr = CopyType(&parent, src)))
            return hr;
        ASSERT(TypeFromToken(parent) == mdtTypeRef);

        if (src->m_isAssembly) {
            CAssemblyFile *asrc = (CAssemblyFile*)src;
            const void * pvHash = NULL;
            DWORD pbHash = 0;
            if (FAILED(hr = asrc->GetHash( &pvHash, &pbHash)))
                return hr;
            hr = m_pEmit->DefineImportMember(asrc->GetImporter(), pvHash, pbHash, src->GetImportScope(),
                *tkCA, m_pAEmit, parent, tkCA);
        } else {
            hr = m_pEmit->DefineImportMember( NULL, NULL, 0, src->GetImportScope(), *tkCA, m_pAEmit, parent, tkCA);
        }
    }
    return hr;
}

LPCWSTR CAssemblyFile::GetAsmName()
{
    if (m_szAsmName)
        return m_szAsmName;

    if (IsInMemory()) {
        m_szAsmName = VSAllocStr(m_Path);
    } else {
        // strip the extension from the name for the manifest
        m_szAsmName = VSAllocStr(m_Name);
        LPWSTR pExt = (LPWSTR)wcsrchr(m_szAsmName, L'.');
        if (pExt) *pExt = L'\0';
    }

    return m_szAsmName;
}

HRESULT CAssemblyFile::MakeAssemblyRef(IMetaDataAssemblyEmit *pEmit, mdAssemblyRef *par)
{
    const void * pTemp;
    DWORD cTemp;
    HRESULT hr;

    // Call GetHash to make sure we have a valid hash of the assembly
    if (FAILED(hr = GetHash( &pTemp, &cTemp)))
        return hr;

    return pEmit->DefineAssemblyRef( m_pPubKey, m_cbPubKey, GetAsmName(), &m_adata, m_pbHash, m_cbHash, 0, par);
}

HRESULT CAssemblyFile::MakeAssemblyRef(IMetaDataEmit *pEmit, mdAssemblyRef *par)
{
    HRESULT hr;
    CComPtr<IMetaDataAssemblyEmit> pAEmit;

    // Get the right interface, then call the real function
    if (FAILED(hr = pEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void**)&pAEmit)))
        return hr;

    return MakeAssemblyRef( pAEmit, par);
}

HRESULT CAssemblyFile::CopyType(mdToken *tkType, CFile *src)
{
    HRESULT hr;

    if (TypeFromToken(*tkType) == mdtTypeDef) {
        // Use the standard APIs
        if (src->m_isAssembly) {
            CAssemblyFile *asrc = (CAssemblyFile*)src;
            const void * pvHash = NULL;
            DWORD pbHash = 0;
            if (FAILED(hr = asrc->GetHash( &pvHash, &pbHash)))
                return hr;
            hr = m_pEmit->DefineImportType(asrc->GetImporter(), pvHash, pbHash, src->GetImportScope(),
                *tkType, m_pAEmit, tkType);
        } else {
            hr = m_pEmit->DefineImportType( NULL, NULL, 0, src->GetImportScope(), *tkType, m_pAEmit, tkType);
        }
    } else {
        mdToken tkScope;
        DWORD cchName;
        LPWSTR name;
        IMetaDataImport* imp = src->GetImportScope();

        ASSERT(TypeFromToken(*tkType) == mdtTypeRef);
        if (FAILED(hr = imp->GetTypeRefProps(*tkType, &tkScope, NULL, 0, &cchName)))
            return hr;

        name = (LPWSTR)_alloca(cchName * sizeof(WCHAR));
        if (FAILED(hr = imp->GetTypeRefProps(*tkType, NULL, name, cchName, &cchName)))
            return hr;
        else {
            if (SUCCEEDED(hr = m_pImport->FindTypeDefByName(name, mdTokenNil, tkType)))
                return hr;
        }

        switch(TypeFromToken(tkScope)) {
        case mdtModule:
            hr = src->MakeModuleRef( m_pEmit, &tkScope);
            break;
        case mdtModuleRef:
            hr = DupModuleRef(&tkScope, src->GetImportScope());
            break;
        case mdtAssemblyRef:
            if (src->m_isAssembly)
                hr = DupAssemblyRef( &tkScope, ((CAssemblyFile*)src)->GetImporter());
            else {
                IMetaDataAssemblyImport *asrc = NULL;
                if (SUCCEEDED(hr = imp->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&asrc))) {
                    hr = DupAssemblyRef( &tkScope, asrc);
                    asrc->Release();
                } else {
                    hr = ReportError(hr, tkScope);
                }
            }
            break;
        default:
            return ReportError(E_INVALIDARG, tkScope);
        }
        if (FAILED(hr))
            return hr;

        if (FAILED(hr = m_pImport->FindTypeRef(tkScope, name, tkType)))
            hr = m_pEmit->DefineTypeRefByName(tkScope, name, tkType);
    }
    return hr;
}

HRESULT CAssemblyFile::DupAssemblyRef(mdToken *tkAR, IMetaDataAssemblyImport* pImport)
{
    // ASSUME: the Import scope will NEVER have an AssemblyRef to the assembly we are building

    ASSEMBLYMETADATA data;
    DWORD chName, cbOrig, cbHash, dwFlags;
    const void *pOrig, *pHash;
    LPWSTR name = NULL;
    HRESULT hr = S_OK;

    memset(&data, 0, sizeof(data));
    chName = cbOrig = cbHash = dwFlags = 0;
    pOrig = pHash = NULL;

    hr = pImport->GetAssemblyRefProps(*tkAR,
        NULL, NULL,                     // Originator
        NULL, 0, &chName,               // Name
        &data,                          // MetaData Struct
        NULL, NULL,                     // Hash
        NULL);                          // Flags
    if (FAILED(hr))
        return hr;

#define AllocArray(var, ch, datum) if ((ch) > 0) { if ((var = (datum*)(VSAlloc(sizeof(datum)*(ch)))) == NULL) { hr = ReportError(E_OUTOFMEMORY); goto FAIL;}}
    AllocArray(name, chName, WCHAR);
    AllocArray(data.szLocale, data.cbLocale, WCHAR);
    AllocArray(data.rOS, data.ulOS, OSINFO);
    AllocArray(data.rProcessor, data.ulProcessor, ULONG);
#undef AllocArray

    hr = pImport->GetAssemblyRefProps(*tkAR,
        &pOrig, &cbOrig,                     // Originator
        name, chName, &chName,               // Name
        &data,                               // MetaData Struct
        &pHash, &cbHash,                     // Hash
        &dwFlags);                           // Flags
    if (FAILED(hr))
        goto FAIL;

    hr = m_pAEmit->DefineAssemblyRef(
        pOrig, cbOrig,
        name,
        &data,
        pHash, cbHash,
        dwFlags,
        tkAR);

FAIL:
    SAFEFREE(name);
    SAFEFREE(data.szLocale);
    SAFEFREE(data.rOS);
    SAFEFREE(data.rProcessor);

    if (FAILED(hr))
        *tkAR = mdTokenNil;

    return hr;
}

HRESULT CFile::GetCAName(mdToken tkCA, LPWSTR *ppszName)
{
    HRESULT hr = S_OK;
    DWORD cchName;
    LPWSTR name;

    *ppszName = NULL;
    if (m_pImport == NULL &&
        FAILED(hr = m_pEmit->QueryInterface(IID_IMetaDataImport, (void**)&m_pImport)))
        return hr;
    
    if (TypeFromToken(tkCA) == mdtMemberRef) {
        mdToken parent;
        if (FAILED(hr = m_pImport->GetMemberRefProps( tkCA, &parent, NULL, 0, NULL, NULL, NULL)))
            return hr;
        tkCA = parent;
    } else if (TypeFromToken(tkCA) == mdtMethodDef) {
        mdToken parent;
        if (FAILED(hr = m_pImport->GetMemberProps( tkCA, &parent, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL)))
            return hr;
        tkCA = parent;
    }

    if (TypeFromToken(tkCA) == mdtTypeRef) {
        // A TypeRef
        if (FAILED(hr = m_pImport->GetTypeRefProps(tkCA, NULL, NULL, 0, &cchName)))
            return hr;
        if ((name = new WCHAR[cchName+1]) == NULL)
            return E_OUTOFMEMORY;
        hr = m_pImport->GetTypeRefProps(tkCA, NULL, name, cchName, &cchName);
    } else {
        ASSERT(TypeFromToken(tkCA) == mdtTypeDef);
        if (FAILED(hr = m_pImport->GetTypeDefProps(tkCA, NULL, 0, &cchName, NULL, NULL)))
            return hr;
        if ((name = new WCHAR[cchName+1]) == NULL)
            return E_OUTOFMEMORY;
        hr = m_pImport->GetTypeDefProps(tkCA, name, cchName, &cchName, NULL, NULL);
    }
    if (SUCCEEDED(hr))
        *ppszName = name;
    else
        delete [] name;
    return hr;
}

HRESULT CAssemblyFile::ReadCryptoKey(PublicKeyBlob **ppPubKey, DWORD &dwLen)
{
    *ppPubKey = NULL;
    dwLen = 0;
    PBYTE buffer = NULL;
    DWORD result = FALSE;
    HRESULT hr = E_FAIL;
    
    if ((!m_KeyFile || m_KeyFile[0] == 0) && 
        (!m_KeyName || m_KeyName[0] == 0))
        return S_FALSE;

    // Use the given name if it exists
    if (m_KeyName == NULL || m_KeyName[0] == L'\0') {

        // Let mscorsn to create the temporary name for us
        if (m_KeyName) VSFree((LPWSTR)m_KeyName);
        m_KeyName = NULL;

        m_isAutoKeyName = true;
    } else
        m_isAutoKeyName = false;

    // try by name
    if (!m_isAutoKeyName) {
        // Try to get the public key for the originator, by name
        if ((result = StrongNameGetPublicKey( m_KeyName, m_pbKeyPair,
            m_cbKeyPair, (BYTE**)&buffer, &dwLen)) != TRUE) {
            ASSERT(buffer == NULL && dwLen == 0);
            hr = StrongNameErrorInfo();
            ASSERT(HRESULT_SEVERITY(hr) == SEVERITY_ERROR);
        } else {
            hr = S_OK;
        }
    }

    // try by file
    if (FAILED(hr) && m_KeyFile) {
        if (SUCCEEDED(hr = ReadCryptoFile())) {
            if (!m_isAutoKeyName) {
                // try installing the file
                if ((result = StrongNameKeyInstall( m_KeyName, m_pbKeyPair, m_cbKeyPair)) == TRUE) {
                    if (FAILED(result = CloseCryptoFile()))
                        return ReportError(result);
                    hr = S_OK;
                } else {
                    hr = StrongNameErrorInfo();
                    ASSERT(FAILED(hr));
                }
            }
            
            // try to get the public key part
            if (SUCCEEDED(hr) &&
                (result = StrongNameGetPublicKey( m_KeyName, m_pbKeyPair, m_cbKeyPair, &buffer, &dwLen)) == FALSE)
                hr = StrongNameErrorInfo();

        } else if (m_isAutoKeyName) {
            return ReportError( ERR_CryptoFileFailed, mdTokenNil, NULL, m_KeyFile, ErrorHR(hr));
        }
    }

    // try a public-only key blob
    if (FAILED(hr) && m_doHalfSign && m_pbKeyPair && // Do we have /a.sign- and a KeyBlob?
        IsValidPublicKeyBlob((PublicKeyBlob*)m_pbKeyPair, m_cbKeyPair)) {

        // Just copy the file bits
        *ppPubKey = (PublicKeyBlob*)VSAlloc(m_cbKeyPair);
        memcpy(*ppPubKey, m_pbKeyPair, m_cbKeyPair);
        dwLen = m_cbKeyPair;
        // Note that at this point m_pbKeyPair is not really a KeyPair!
        return CloseCryptoFile();

    } else if (SUCCEEDED(hr)) {
        size_t len = VAL32(((PublicKeyBlob*)buffer)->cbPublicKey) + (sizeof(ULONG) * 3);
        void *temp = VSAlloc((DWORD)len);
        memcpy(temp, buffer, len);
        StrongNameFreeBuffer((BYTE*)buffer);
        *ppPubKey = (PublicKeyBlob*)temp;

        return S_OK;

    } else {
        // This is the first Crypto-call so we need to check for SN_* errors
        if (hr == SN_CRYPTOAPI_CALL_FAILED ||
            hr == SN_NO_SUITABLE_CSP) {
            hr = ReportError(ERR_CryptoFailed);
        } else if (m_pbKeyPair && IsValidPublicKeyBlob((PublicKeyBlob*)m_pbKeyPair, m_cbKeyPair) &&
                   !m_doHalfSign) {
            // Give both errors if appropriate
            if (!m_isAutoKeyName) {
                bool temp = false;
                ReportError( ERR_CryptoNoKeyContainer, mdTokenNil, &temp, m_KeyName);
            }
            hr = ReportError(ERR_NeedPrivateKey, mdTokenNil, NULL, m_KeyFile);
        } else if (m_KeyName && !m_isAutoKeyName) {
            hr = ReportError( ERR_CryptoNoKeyContainer, mdTokenNil, NULL, m_KeyName);
        } else {
            hr = ReportError(hr);
        }

        // The call failed so pretend like there's no key-info
        StrongNameFreeBuffer((BYTE*)buffer);
        CloseCryptoFile();

        return hr;
    }
}

HRESULT CAssemblyFile::ReadCryptoFile()
{
    ASSERT(m_KeyFile);
    HRESULT hr = S_OK;

    // Open and Read in the Key Blob
    m_hKeyFile = OpenFileEx( m_KeyFile, &m_cbKeyPair, IsInMemory() ? NULL : m_Path);
    if (m_hKeyFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CRYPTO_FAILED;
    }

    m_hKeyMap = CreateFileMappingA( m_hKeyFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (m_hKeyMap == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CRYPTO_FAILED;
    }

    m_pbKeyPair = (PBYTE)MapViewOfFile( m_hKeyMap, FILE_MAP_READ, 0, 0, 0);
    if (m_pbKeyPair == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CRYPTO_FAILED;
    }

    return hr;

CRYPTO_FAILED:

    // The call failed so pretend like there's no key-info
    CloseCryptoFile(); // Ignore any errors from this
    return hr;
}

HRESULT CAssemblyFile::CloseCryptoFile()
{
    if (m_pbKeyPair != NULL)
        UnmapViewOfFile(m_pbKeyPair);
    m_pbKeyPair = NULL;
    m_cbKeyPair = 0;

    if (m_hKeyMap != NULL)
        CloseHandle(m_hKeyMap);
    m_hKeyMap = NULL;

    if (m_hKeyFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hKeyFile);
    m_hKeyFile = INVALID_HANDLE_VALUE;

    return S_OK;
}

HRESULT CAssemblyFile::SignAssembly()
{
    if (!m_doHalfSign && (m_pbKeyPair != NULL || (m_KeyName && *m_KeyName && !m_isAutoKeyName))){
        if (IsInMemory()) {
            VSFAIL("You can't fully sign an InMemory assembly!");
            return E_INVALIDARG;
        }
        if (StrongNameSignatureGeneration( m_Path, m_KeyName,
            m_pbKeyPair, m_cbKeyPair,
            NULL, NULL) == FALSE) {
            DWORD err = StrongNameErrorInfo();
            return ReportError(HRESULT_FROM_WIN32(err));
        }
        ASSERT(StrongNameSignatureVerification(m_Path, SN_INFLAG_FORCE_VER | SN_INFLAG_INSTALL | SN_INFLAG_ALL_ACCESS, NULL));
        return S_OK;
    }
    return S_FALSE;
}

// NOTE: rev is optional (may be NULL), but build is NOT
HRESULT CAssemblyFile::MakeVersion(WORD *rev, WORD *build)
{
    tm temptime;
    time_t current, initial;
    double diff;
    bool BadTime = false;

    // Get the current time
    time( &current);
    if (current < 0 && rev)
        // We have a date previous to 1970, and they want a revision number
        // Can't do it, so just fail here
        return E_FAIL;
    else if (current < (60 * 60 * 24) && !rev) {
        // Keep adding a day until we are just above 1 day
        // Thus arriving at the number of seconds since midnight (GMT) plus 1 day
        // so it will always be after 1/1/1970 even in local time
        while (current < (60 * 60 * 24))
            current += (60 * 60 * 24);
        BadTime = true;
    }
    // Use local time
    temptime = *localtime( &current);
    if (temptime.tm_year < 100 && !rev) {
        // If we only want a build number, then
        // make sure the year is greater than 2000
        temptime.tm_year = 100;
        BadTime = true;
    }
    // convert to useful struct
    current = mktime(&temptime);
    if (current == (time_t)-1)
        return E_FAIL;

    // Make Jan. 1 2000, 00:00:00.00
    memset(&temptime, 0, sizeof(temptime));
    temptime.tm_year = 100; // 2000 - 1900
    temptime.tm_mday = 1; // Jan. 1
    temptime.tm_isdst = -1; // Let the CRT figure out if it is Daylight savings time
    initial = mktime(&temptime);


    // Get difference in seconds
    diff = (double)current - (double)initial;
    if (diff < 0.0 && rev) {
        return E_FAIL;
    }

    if (rev != NULL) {
        ASSERT(diff >= 0.0);
        *rev = (WORD)floor(diff / (60 * 60 * 24)); // divide by 60 seconds, 60 minutes, 24 hours, to get difference in days
    }

    if (build != NULL) {
        if (diff < 0.0) {
            diff = fmod(diff, 60 * 60 * 24) + (60 * 60 * 24); // make it positive by moving it and adding 1 day
            ASSERT(diff >= 0.0);
            BadTime = true;
        }
        *build = (WORD)(fmod(diff, 60 * 60 * 24) / 2); // Get the seconds since midnight div 2 for 2 second granularity
        ASSERT(*build != 65535);
    } else {
        return E_INVALIDARG;
    }

    return BadTime ? E_FAIL : S_OK;
}

#undef SAFEFREE
