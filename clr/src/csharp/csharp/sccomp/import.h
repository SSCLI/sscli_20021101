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
// File: import.h
//
// Defines the structures used to import COM+ metadata into the symbol table.
// ===========================================================================


#define MAX_IDENT_SIZE 512      // Maximum identifier size we allow.
#define MAX_FULLNAME_SIZE 2048  // Maximum namespace or fully qualified identifier size we allow.
#define NESTED_CLASS_SEP (L'+') // Separator for nested classes.

#define MAGIC_SYMBOL_VALUE ((SYM*)-1)

#include <pshpack1.h>
struct DecimalConstantBuffer {
	WORD format;
	BYTE scale;
	BYTE sign;
	DWORD hi;
	DWORD mid;
	DWORD low;
	WORD cNamedArgs;
};
#include <poppack.h>


class IMPORTER
{
public:
    IMPORTER();
    ~IMPORTER();
    void Init();
    void Term();

    COMPILER * compiler();

    void ImportAllTypes();
    void GetBaseTokenAndFlags(AGGSYM *sym, AGGSYM **base, DWORD *flags);
    ACCESS AccessFromTypeFlags(unsigned flags, const AGGSYM * sym);
    bool HasIfaceListChanged(AGGSYM *sym);
    bool ResolveInheritance(AGGSYM *sym);
    void DefineImportedType(PAGGSYM sym);
    void ImportTypes(PINFILESYM infile);
    void GetTypeRefAssemblyName(PINFILESYM inputfile, DWORD scope, mdToken token, WCHAR * assemblyName, LONG cchAssemblyName);
#ifdef DEBUG
    void DeclareAllTypes(PPARENTSYM parent);
#endif //DEBUG

    bool OpenAssembly( PINFILESYM infile);

    AGGSYM *ImportOneType(PINFILESYM infile, DWORD scope, mdTypeDef token);
    AGGSYM *HasTypeDefChanged(PINFILESYM infile, DWORD scope, mdTypeDef token);
    IMetaDataImport *OpenMetadataFile(LPCWSTR fileName, bool readWrite);

    bool HasInterfaceChange(const AGGSYM *cls);
    bool HasAssemblyChanged(PINFILESYM inputfile, bool newAssemblyCLS, mdToken assemblyToken);
    bool HasModuleChanged(PINFILESYM inputfile, bool newModuleCLS);
    void ReadExistingMemberTokens(AGGSYM *cls);
    void ReadExistingParameterTokens(METHSYM *method, METHINFO *info);

    PTYPESYM ImportSigType(PINFILESYM inputfile, DWORD scope, PCCOR_SIGNATURE * sigPtr, bool canBeVoid, bool canBeByref, bool *isCm, bool mustExist);

    LPCWSTR GetAssemblyName(INFILESYM *infile);

private:
    struct IMPORTED_CUSTOM_ATTRIBUTES
    {
        // decimal literal
        bool hasDecimalLiteral;
        DECIMAL decimalLiteral;

        // deprecated
        bool isDeprecated;
        bool isDeprecatedError;
        WCHAR *deprecatedString;

        // CLS
        bool hasCLSattribute;
        bool isCLS;

        // attribute
        bool allowMultiple;
        CorAttributeTargets attributeKind;

        // conditional
        NAMELIST **conditionalHead;
        NAMELIST *conditionalSymbols;

        // parameter lists
        bool isParamListArray;

        // RequiredCustomAttribute
        bool hasRequiredAttribute;

        // default member
        WCHAR * defaultMember;

        // ComImport/CoClass
        WCHAR * CoClassName;
    };

    void CheckHR(HRESULT hr, INFILESYM *infile);
    void CheckHR(int errid, HRESULT hr, INFILESYM *infile);
    void MetadataFailure(HRESULT hr, INFILESYM *infile);
    void MetadataFailure(int errid, HRESULT hr, INFILESYM *infile);
    IMetaDataDispenser * GetDispenser();
    static ACCESS ConvertAccessLevel(DWORD flags, const SYM * sym);
    void SetAccessLevel(PSYM sym, DWORD flags);
    void InitImportFile(PINFILESYM infile);
    AGGSYM *ImportOneTypeBase(PINFILESYM infile, DWORD scope, mdTypeDef token, bool mustExist, DWORD *pFlags, bool * pIsValueType);
    bool ImportConstant(CONSTVAL * constVal, ULONG cch, PTYPESYM valType, DWORD constType, const void * constValue);
    PAGGSYM ImportInterface(PINFILESYM inputfile, DWORD scope, mdInterfaceImpl tokenIntf, PAGGSYM symDerived);
    void ImportField(PINFILESYM inputfile, PAGGSYM parent, mdFieldDef tokenField);
    void ImportMethod(PINFILESYM inputfile, PAGGSYM parent, mdMethodDef tokenMethod);
    void ImportProperty(PINFILESYM inputfile, PAGGSYM parent, mdProperty tokenProperty, PNAME defaultMember);
    void ImportEvent(PINFILESYM inputfile, PAGGSYM parent, mdEvent tokenProperty);
    PMETHSYM FindMethodDef(PINFILESYM inputfile, PAGGSYM parent, mdMethodDef token);
    PTYPESYM ImportFieldType(PINFILESYM inputfile, DWORD scope, PCCOR_SIGNATURE sig, bool * isVolatile);
    int GetSignatureCParams(PCCOR_SIGNATURE sig);
    void ImportMethodOrPropSignature(PINFILESYM inputfile, mdMethodDef token, PCCOR_SIGNATURE sig, PMETHPROPSYM sym);
    ACCESS GetAccessOfMethod(mdToken methodToken, AGGSYM *cls);
    void ImportSignature(PINFILESYM inputfile, DWORD scope, mdMethodDef token,
                                   PCCOR_SIGNATURE sig, bool isMethod, bool isStatic, TYPESYM **retType,
                                   unsigned short *cParams, TYPESYM *** paramTypes, bool *isBogus,
                                   bool *isVarargs, bool * isParams, bool *isCmodOpt);
    BOOL IsParamArray(PINFILESYM inputfile, DWORD scope, mdMethodDef methodToken, int iParam);
    PNSDECLSYM GetNSDecl(NSSYM * ns, PINFILESYM infile, bool mustExist);
    bool GetTypeRefFullName(PINFILESYM inputfile, DWORD scope, mdToken token, WCHAR * fullnameText, ULONG cchBuffer);
    PNSSYM ResolveNamespace(WCHAR * namespaceText, int cch, PINFILESYM infile, bool mustExist);
	public:
    PAGGSYM ResolveTypeName(LPWSTR className, PINFILESYM inputfile, DWORD scope, mdToken tkResolutionScope, bool mustExist, bool nameIsNested);
    PAGGSYM ResolveTypeRef(PINFILESYM inputfile, DWORD scope, mdToken tokenBase, bool mustExist);
	private:
    PAGGSYM ResolveBaseRef(PINFILESYM inputfile, DWORD scope, mdToken tokenBase, PAGGSYM symDerived);

    bool HasFieldChanged(PINFILESYM inputfile, const AGGSYM *cls, mdToken fieldToken, unsigned * cFields);
    bool HasMethodChanged(PINFILESYM inputfile, const AGGSYM *cls, mdToken methodToken, unsigned * cMethos);
    bool HasPropertyChanged(PINFILESYM inputfile, const AGGSYM *cls, mdToken propertyToken, unsigned *cProperties);
    bool HasEventChanged(PINFILESYM inputfile, const AGGSYM *cls, mdToken eventToken, unsigned *cEvents);
    bool HasMethodImplExist(PINFILESYM inputfile, const AGGSYM *cls, mdToken bodyToken, mdToken declToken);

    MEMBVARSYM *ReadExistingField(PINFILESYM inputfile, AGGSYM *cls, mdToken fieldToken);
    METHSYM *   ReadExistingMethod(PINFILESYM inputfile, AGGSYM *cls, mdToken methodToken);
    PROPSYM *   ReadExistingProperty(PINFILESYM inputfile, AGGSYM *cls, mdToken propertyToken);
    EVENTSYM *  ReadExistingEvent(PINFILESYM inputfile, AGGSYM *cls, mdToken eventToken);

    void ImportCustomAttributes(PINFILESYM inputfile, DWORD scope, IMPORTED_CUSTOM_ATTRIBUTES *attributes, mdToken token);
    void ImportOneCustomAttribute(PINFILESYM inputfile, DWORD scope, IMPORTED_CUSTOM_ATTRIBUTES *attributes, mdToken attrToken, const void * pvData, ULONG cbSize);
    bool ImportCustomAttrArgBool(LPCVOID * ppvData, ULONG * pcbSize);
    WCHAR * ImportCustomAttrArgString(LPCVOID * ppvData, ULONG * pcbSize);
    int ImportCustomAttrArgInt(LPCVOID * ppvData, ULONG * pcbSize);
    WORD ImportCustomAttrArgWORD(LPCVOID * ppvData, ULONG * pcbSize);
    BYTE ImportCustomAttrArgBYTE(LPCVOID * ppvData, ULONG * pcbSize);
    void ImportNamedCustomAttrArg(LPCVOID *ppvData, ULONG *pcbSize, LPCWSTR *pname, TYPESYM **type);

    bool inited;
};
