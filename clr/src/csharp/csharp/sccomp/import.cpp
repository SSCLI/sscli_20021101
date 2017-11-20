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
// File: import.cpp
//
// Routines for importing COM+ metadata into the symbol table.
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"
#include "corerror.h"
#include "strongname.h"


/* Kinds of custom attributes we care about, for table below. */
enum IMPORTEDCUSTOMATTRKIND {
    CUSTOMATTR_NONE,
    CUSTOMATTR_ATTRIBUTEUSAGE_VOID,
    CUSTOMATTR_ATTRIBUTEUSAGE_VALIDON, 
    CUSTOMATTR_DEPRECATED_VOID,
    CUSTOMATTR_DEPRECATED_STR,
    CUSTOMATTR_DEPRECATED_STRBOOL,
    CUSTOMATTR_CONDITIONAL,
    CUSTOMATTR_DECIMALLITERAL,
    CUSTOMATTR_DEPRECATEDHACK,
    CUSTOMATTR_ATTRIBUTEHACK,
    CUSTOMATTR_CLSCOMPLIANT,
    CUSTOMATTR_PARAMS,
    CUSTOMATTR_DEFAULTMEMBER,
    CUSTOMATTR_DEFAULTMEMBER2,
    CUSTOMATTR_REQUIRED,
    CUSTOMATTR_COCLASS,

    CUSTOMATTR_MAX
};

/*
 * Table of custom attributes that we care about when importing.
 *
 * The following table defines all the custom attribute kinds we care about
 * on import, by class name and constructor signature.
 */
struct IMPORTEDCUSTOMATTR {
    IMPORTEDCUSTOMATTRKIND attrKind;
    PREDEFNAME className;
    int cbSig;             // if <0, don't check sig.
    COR_SIGNATURE sig[8];  // Increase size this if needed.
};

const IMPORTEDCUSTOMATTR g_importedAttributes[CUSTOMATTR_MAX-1] = {
    { CUSTOMATTR_DEPRECATED_VOID,    PN_OBSOLETE_CLASS,  3, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID} },
    { CUSTOMATTR_DEPRECATED_STR,     PN_OBSOLETE_CLASS,  4, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING } },
    { CUSTOMATTR_DEPRECATED_STRBOOL, PN_OBSOLETE_CLASS,  5, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING, ELEMENT_TYPE_BOOLEAN } },
    { CUSTOMATTR_ATTRIBUTEUSAGE_VOID,   PN_ATTRIBUTEUSAGE_CLASS,  3, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID} },
    { CUSTOMATTR_ATTRIBUTEUSAGE_VALIDON,PN_ATTRIBUTEUSAGE_CLASS, -1, {0} },
    { CUSTOMATTR_CONDITIONAL,        PN_CONDITIONAL_CLASS,    4, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING } },
    { CUSTOMATTR_DECIMALLITERAL,     PN_DECIMALLITERAL_CLASS, 8, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 5, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U1, ELEMENT_TYPE_U1, ELEMENT_TYPE_U4, ELEMENT_TYPE_U4, ELEMENT_TYPE_U4}},
    { CUSTOMATTR_DEPRECATEDHACK,     PN_DEPRECATEDHACK_CLASS, 0, {0}},
    { CUSTOMATTR_CLSCOMPLIANT,       PN_CLSCOMPLIANT,         4, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN }},
    { CUSTOMATTR_PARAMS,             PN_PARAMARRAY_CLASS,     -1, {0} },
    { CUSTOMATTR_DEFAULTMEMBER,      PN_DEFAULTMEMBER_CLASS,  4, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING } },
    { CUSTOMATTR_DEFAULTMEMBER2,     PN_DEFAULTMEMBER_CLASS2, 4, { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING } },
    { CUSTOMATTR_REQUIRED,           PN_REQUIRED_CLASS,       -1, {0} },
    { CUSTOMATTR_COCLASS,            PN_COCLASS_CLASS,        -1, {0} },

};


/*
 * Constructor
 */
IMPORTER::IMPORTER()
{
    inited = false;
}


/*
 * Destructor
 */
IMPORTER::~IMPORTER()
{
    Term();
}

/*
 * Semi-generic method to check for failure of a meta-data method.
 */
__forceinline void IMPORTER::CheckHR(HRESULT hr, INFILESYM *infile)
{
    if (FAILED(hr))
        MetadataFailure(hr, infile);
    SetErrorInfo(0, NULL); // Clear out any stale ErrorInfos
}

/*
 * Semi-generic method to check for failure of a meta-data method. Passed
 * an error ID in cases where the generic one is no good.
 */
__forceinline void IMPORTER::CheckHR(int errid, HRESULT hr, INFILESYM *infile)
{
    if (FAILED(hr))
        MetadataFailure(errid, hr, infile);
    SetErrorInfo(0, NULL); // Clear out any stale ErrorInfos
}

/*
 * Handle a generic meta-data API failure.
 */
void IMPORTER::MetadataFailure(HRESULT hr, INFILESYM *infile)
{
    MetadataFailure(FTL_MetadataImportFailure, hr, infile);
}

/*
 * Handle an API failure. The passed error code is expected to take one insertion string
 * that is filled in with the HRESULT.
 */
void IMPORTER::MetadataFailure(int errid, HRESULT hr, INFILESYM *infile)
{
    compiler()->Error(NULL, errid, compiler()->ErrHR(hr), infile ? infile->name->text : L"");
}


/*
 * Initialize everything.
 */
void IMPORTER::Init()
{
    inited = true;
}

/*
 * Terminate everything.
 */
void IMPORTER::Term()
{
    if (inited) {
        POUTFILESYM outfile;
        PINFILESYM  infile;

        // Close all the outstanding meta-data scopes. They are all
        // children of the empty outfile symbol.
        outfile = compiler()->symmgr.GetMDFileRoot();

        if (outfile) {
            // Go through all the inputfile symbols.
            for (infile = outfile->firstInfile();
                 infile != NULL;
                 infile = infile->nextInfile())
            {
                if (! infile->isSource) {
                    if (infile->metaimport) {
                        for (DWORD i = 0; i < infile->dwScopes; i++) {
                            if (infile->metaimport[i] != NULL) {
                                // Release the meta-data import scope.
                                infile->metaimport[i]->Release();
                                infile->metaimport[i] = NULL;
                            }
                        }
                        // We used the Symbol allocator, so don't need to free this
                        // delete [] infile->metaimport;
                    }
                    infile->metaimport = NULL;
                }
                if (! infile->isSource && infile->assemimport) {
                    // Release the meta-data import scope.
                    infile->assemimport->Release();
                    infile->assemimport = NULL;
                }
            }
        }

        inited = false;
    }
}


/*
 * Import one type. Create a symbol for the type in the symbol
 * table. The symbol is created "undeclared" - no information about
 * it is known except its name.
 *
 * returns false if a fatal(for this input file) error occured
 *
 *                                                                                                                              
 */
AGGSYM *IMPORTER::ImportOneTypeBase(PINFILESYM infile, DWORD scope, mdTypeDef token, bool mustExist, DWORD *pFlags, bool * isValueType)
{
    WCHAR fullNameText[MAX_FULLNAME_SIZE];  // full name of type.
    ULONG cchFullNameText;
    WCHAR * namespaceText;                  // namespace part
    ULONG cchNamespaceText;
    WCHAR  *typenameText;                   // type name part.
    PNAME typeName;

    PNSSYM ns;                              // enclosing namespace.
    PPARENTSYM parent;                      // parent namespacedecl or class
    PAGGSYM sym;                            // class/interface/enum symbol.
    PSYM symConflict;                  // Possibly conflicting symbol.
    mdToken tkExtends;

    // Get namespace, name, and flags for the type.
    TimerStart(TIME_IMI_GETTYPEDEFPROPS);
    CheckHR(infile->metaimport[scope]->GetTypeDefProps(token,
            fullNameText, lengthof(fullNameText), &cchFullNameText,      // Type name
            pFlags,                                                      // Flags
            &tkExtends                                                   // Extends
           ), infile);
    TimerStop(TIME_IMI_GETTYPEDEFPROPS);

    // Check to see if we extend System.ValueType, and are thus a value type.
    WCHAR baseClassText[MAX_FULLNAME_SIZE]; // name of the base class.
    *isValueType = false;
    if (! IsNilToken(tkExtends)) {
        if (GetTypeRefFullName(infile, scope, tkExtends, baseClassText, lengthof(baseClassText)) &&
            (wcscmp(baseClassText, compiler()->symmgr.GetFullName(PT_VALUE)) == 0 ||
			 wcscmp(baseClassText, compiler()->symmgr.GetFullName(PT_ENUM)) == 0))
        {
			// Exception: System.Enum is NOT a value type!
			if (wcscmp(fullNameText, compiler()->symmgr.GetFullName(PT_ENUM)) != 0)
                *isValueType = true;
        }
    }

    // Split full name into namespace and type name.
    typenameText = wcsrchr(fullNameText, L'.');
    if (typenameText) {
        namespaceText = fullNameText;
        cchNamespaceText = (ULONG)(typenameText - namespaceText);
        typenameText++;
    }
    else { // no namespace
        namespaceText = NULL; 
        cchNamespaceText = 0;
        typenameText = fullNameText;
    }

    // Nested classes are handled differently..
    if (IsTdNested(*pFlags)) {
        mdToken tkOuter;

        // Get the class this class is nested within.
        TimerStart(TIME_IMI_GETNESTEDCLASSPROPS);
        CheckHR(infile->metaimport[scope]->GetNestedClassProps(token, &tkOuter), infile);
        TimerStop(TIME_IMI_GETNESTEDCLASSPROPS);

        parent = ResolveTypeRef(infile, scope, tkOuter, false);
        if (!parent)
            return NULL;    // Not valid if we can't find the parent class
    }
    else  {
        // Find the correct namespace.
        ns = ResolveNamespace(namespaceText, cchNamespaceText, infile, mustExist);
        if (!ns)
            return NULL;         // some error occurred.

        // Get a parent declaration.
        parent = GetNSDecl(ns, infile, mustExist);
    }

    if (!typenameText || parent->getInputFile() != infile)
        return NULL;       // Not valid.

    // Place name of type in name table.
    typeName = compiler()->namemgr->AddString(typenameText);

    // See if something is conflicting.
    sym = NULL;
    symConflict = compiler()->symmgr.LookupGlobalSym(typeName, parent->getScope(), MASK_ALL);
    if (symConflict) {
        if (symConflict->kind == SK_NSSYM) {
            if (!mustExist) {
                // A user-defined namespace is conflicting with an imported type name.
                // Issue an error.
                compiler()->clsDeclRec.errorStrSymbol(NULL, infile, compiler()->ErrSym(symConflict), symConflict, ERR_NamespaceTypeConflict);
            }
            return NULL;
        }

        if (symConflict->kind == SK_AGGSYM &&
            (symConflict->asAGGSYM()->getInputFile() == infile))
        {
            // Must be a parent class already of some other class already seen
            // in the same file. Fill in the info below.
            sym = symConflict->asAGGSYM();
        }
        else {
            // Must be defined in some user class or previous metadata -- ignore.
            if (symConflict->kind == SK_AGGSYM) {
                AGGSYM * conflict = symConflict->asAGGSYM();
                if (!pFlags || IsTdPublic(*pFlags) || IsTdNestedPublic(*pFlags) ||
                    IsTdNestedFamily(*pFlags) || IsTdNestedFamORAssem(*pFlags) ||
                    (IsTdNestedFamANDAssem(*pFlags) && parent->getAssemblyIndex() == 0))
                    conflict->hasMultipleDefs = true;
                if (conflict->hasExternalAccess() && conflict->getAssemblyIndex() == 0 && infile->assemblyIndex == 0) {
                    compiler()->clsDeclRec.errorFileSymbol( NULL, infile, ERR_MultipleLocalTypeDefs, symConflict);
                }
            }
            return NULL;
        }
    }

    // Create the symbol for the new type.
    if (!sym)
    {
        if (mustExist) {
            return NULL;
        }
        sym = compiler()->symmgr.CreateAggregate(typeName, parent);
    }

    // Remember the token.
    // Set what we can on the symbol structure from the flags.

    ASSERT(!sym->isPrepared && !sym->isDefined);
    sym->tokenImport = token;
    sym->importScope = scope;

    return sym;
}

/*
 * Import one type. Create a symbol for the type in the symbol
 * table. The symbol is created "undeclared" - no information about
 * it is known except its name.
 *
 * returns false if a fatal(for this input file) error occured
 */
AGGSYM *IMPORTER::ImportOneType(PINFILESYM infile, DWORD scope, mdTypeDef token)
{
    DWORD flags;
    bool isValueType;
    AGGSYM *sym = ImportOneTypeBase(infile, scope, token, false, &flags, &isValueType);
    if (!sym)
        return NULL;

    sym->access = AccessFromTypeFlags(flags, sym);

    if (isValueType) {
	// struct or enum
	sym->isStruct = true;
	sym->isSealed = true;
    }
    else if (IsTdInterface(flags)) {
        // interface
        sym->isInterface = true;
        sym->isAbstract = true;
    }
    else {
        // regular class or delegate
        sym->isClass = true;
        sym->isAbstract = (flags & tdAbstract) ? true : false;
        sym->isSealed = (flags & tdSealed) ? true : false;
	}
    sym->isComImport = !!IsTdImport(flags);

	ASSERT(!(sym->isEnum && (sym->isStruct || sym->isClass || sym->isDelegate)));

    return sym;
}


/*
 * Opens a metadata file for read.
 * the returned interface must be Release()ed by the caller.
 */
IMetaDataImport *IMPORTER::OpenMetadataFile(LPCWSTR fileName, bool readWrite)
{
    IMetaDataDispenser * dispenser = compiler()->GetMetadataDispenser();

    IMetaDataImport *metaimport = NULL;
    HRESULT hr;

    // Get importation interface.
    TimerStart(TIME_IMD_OPENSCOPE);
    hr = dispenser->OpenScope(fileName,
                                 readWrite ? (ofWrite | ofRead | ofNoTypeLib) : ofRead | ofNoTypeLib,
                                 IID_IMetaDataImport,
                                 (IUnknown **) & metaimport);
    TimerStop(TIME_IMD_OPENSCOPE);

    if (FAILED(hr))
        compiler()->Error(NULL, FTL_MetadataCantOpenFile, compiler()->ErrHR(hr), fileName);

    return metaimport;
}

/*
 * Shared code to prepare a file for importing of it's types.
 */
void IMPORTER::InitImportFile(PINFILESYM infile)
{
}

/*
 * Import all the types defined in this metadata scope.
 * Details of the types (e.g., members thereof) are not
 * imported.
 */
void IMPORTER::ImportTypes(PINFILESYM infile)
{
    SETLOCATIONFILE(infile);

    HCORENUM enumTypes;
    mdTypeDef typedefs[32];
    ULONG cTypedefs, iTypedef;

    InitImportFile(infile);

    // Enumeration all the types in each scope.
    for (DWORD i = 0; i < infile->dwScopes; i++) {
        enumTypes = 0;
        CheckHR(FTL_MetadataCantOpenFile,
            compiler()->linker->GetScope(compiler()->assemID, infile->mdImpFile, i, infile->metaimport + i),
            infile);
        if (infile->metaimport[i] == NULL)
            // A non-metadata file
            continue;
        do {
            // Get next batch of types.
            TimerStart(TIME_IMI_ENUMTYPEDEFS);
            CheckHR(infile->metaimport[i]->EnumTypeDefs(&enumTypes, typedefs, lengthof(typedefs), &cTypedefs), infile);
            TimerStop(TIME_IMI_ENUMTYPEDEFS);

            // Process each type.
            for (iTypedef = 0; iTypedef < cTypedefs; ++iTypedef) {
                ImportOneType(infile, i, typedefs[iTypedef]);
            }
        } while (cTypedefs > 0);
        infile->metaimport[i]->CloseEnum(enumTypes);
    }

    // Import module-level CLS attributes if we're doing an add-module.
    if (infile->isAddedModule) {
        IMPORTED_CUSTOM_ATTRIBUTES attributes;
        mdModule impModule;

        CheckHR(infile->metaimport[0]->GetModuleFromScope( &impModule), infile);
        ImportCustomAttributes(infile, 0, &attributes, impModule);

        infile->hasCLSattribute = attributes.hasCLSattribute;
        infile->isCLS = attributes.isCLS;
    }
}


/*
 * Import all top level types from all metadata scopes.
 */
void IMPORTER::ImportAllTypes()
{
    SETLOCATIONSTAGE(IMPORT);

    POUTFILESYM outfile;
    PINFILESYM  infile;

    outfile = compiler()->symmgr.GetMDFileRoot();

    if (outfile) {
        // Go through all the inputfile symbols.
        for (infile = outfile->firstInfile();
             infile != NULL;
             infile = infile->nextInfile())
        {
            if (!infile->isSource) {
                OpenAssembly(infile);
            }
        }
    }
}

/*
 * Given a name space, find or create a corresponding NSDECL symbol for this
 * input file.
 */
PNSDECLSYM IMPORTER::GetNSDecl(NSSYM * ns, PINFILESYM infile, bool mustExist)
{
    //
    // first search existing declarations for this namespace
    // for a declaration with this inputfile
    //
    PNSDECLSYM search;
    PNSSYM parentNS;
    PNSDECLSYM parentDecl;

    search = ns->firstChild->asNSDECLSYM();
    while (search && (search->getInputFile() != infile)) {
        search = search->nextDeclaration;
    }

    if (!search && !mustExist) {
        //
        // didn't find an existing declaration for this namespace/file combo
        // create one
        //
        parentNS = ns->getParentNS();
        if (parentNS)
            parentDecl = GetNSDecl(parentNS, infile, false);
        else
            parentDecl = NULL;

        search = compiler()->symmgr.CreateNamespaceDeclaration(ns, parentDecl, infile, NULL);
        search->usingClausesResolved = true;
        search->isPrepared = true;
        if (!parentDecl) {
            ASSERT(!infile->rootDeclaration);
            infile->rootDeclaration = search;
        }
        ASSERT(infile->rootDeclaration);
    }

    return search;
}

/*
 * Convert a fully qualified namespace string to a namespace symbol. If something
 * other than a namespace is already declared with the same name,
 * then an error is reported and NULL is returned. Otherwise, an existing namespace
 * symbol is returned if present, or a new namespace symbol is created.
 */
PNSSYM IMPORTER::ResolveNamespace(WCHAR * namespaceText, int cch, PINFILESYM infile, bool mustExist)
{
    WCHAR * p;
    WCHAR * start;
    PNAME name;
    PNSSYM ns;

    ns = compiler()->symmgr.GetRootNS();  // start at the root namespace.
    start = p = namespaceText;

    for (;;) {
        // Advance p to the end of the next segment.
        while (p - namespaceText < cch && *p != L'.') {
            ++p;
        }

        // Find/Create a sub-namespace with this name.
        if (p != start) {
            name = compiler()->namemgr->AddString(start, (int)(p - start));

            // check for existing namespace
            PSYM symConflict;
            symConflict = compiler()->symmgr.LookupGlobalSym(name, ns, MASK_ALL);
            if (symConflict) {
                if (symConflict->kind == SK_NSSYM) {
                    ns = symConflict->asNSSYM(); // Already have the right namespace.
                } else {

                    if (mustExist) {
                        // Uh-oh, we have a conflicting type with the namespace.
                        // Report the error.
                        compiler()->clsDeclRec.errorStrSymbol(NULL, infile, compiler()->ErrSym(symConflict), symConflict, ERR_NamespaceTypeConflict);
                    }
                    ns = NULL;
                }
            } else if (!mustExist) {
                ns = compiler()->symmgr.CreateNamespace(name, ns);
            } else {
                ns = NULL;
            }
        }

        // Are we done?
        if (!ns || p - namespaceText >= cch) {
            return ns;
        }

        // Continue to the next segment.
        ++p;
        start = p;
    }
}


/*
 * Resolves a fully qualified type name to an actual aggregate type. If the class is not found,
 * then a new class in the given input file is created. If a conflict with a namespace is found,
 * an error is reported. Inner classes must be seperated with the inner class seperator.
 * Only check for inner class seperator if nameIsNested is true
 */
PAGGSYM IMPORTER::ResolveTypeName(LPWSTR className, PINFILESYM inputfile, DWORD scope, mdToken tkResolutionScope, bool mustExist, bool nameIsNested)
{
    PNSSYM ns = NULL;
    PPARENTSYM parentDecl;
    PPARENTSYM parentScope;
    PNAME name;
    PAGGSYM sym;                            // class/interface/enum symbol.
    PSYM symConflict;                       // Possibly conflicting symbol.
    WCHAR * nestedName = NULL;

    if (IsNilToken(tkResolutionScope))
    {
        // The class name may contain assembnly info
        // for now, just strip it out
        WCHAR * assemblyInfo = wcschr(className, L','); // the first comma separates the class name from the assembly name
        if (assemblyInfo != NULL) {
            // Use _alloca becuase the name will be small (currently I think it's limited to 2K
            // and then we can just assign it back into the original buffer, so the rest of the
            // functions can be ignorante about assemblies.
            LPWSTR temp = (LPWSTR)_alloca((assemblyInfo + 1 - className) * sizeof(WCHAR));
            wcsncpy(temp, className, assemblyInfo - className);
            temp[assemblyInfo - className]  = L'\0';
            className = temp;
        }
    }

    if ((TypeFromToken(tkResolutionScope) == mdtTypeRef || TypeFromToken(tkResolutionScope) == mdtTypeDef))
    {
        // Nested class. Get the parent class from the resolution scope.
        parentScope = parentDecl = ResolveTypeRef(inputfile, scope, tkResolutionScope, false);
        if (!parentScope)
            return NULL; // Not valid if we can't find the parent class
    }
    else {
        WCHAR * namespaceName;
        int cchNamespace;
        WCHAR * baseName = NULL;

        if (nameIsNested && (nestedName = wcschr(className, NESTED_CLASS_SEP))) {
            // Find the last dot before the nested class separator
            WCHAR * t = wcschr(className, L'.');
            while (t && t < nestedName) {
                baseName = t;
                t = wcschr(baseName, L'.');
            }
        } else {
            // Split full name into namespace and type name.
            baseName = wcsrchr(className, L'.');
        }

        if (baseName) {
            namespaceName = className;
            cchNamespace = (int)(baseName - className);
            className = baseName + 1;
        }
        else { // no namespace
            namespaceName = NULL; 
            cchNamespace = 0;
        }

        // Convert namespace name into a namespace symbol.
        ns = ResolveNamespace(namespaceName, cchNamespace, inputfile, mustExist);
        if (!ns)
            return NULL;

        parentDecl = NULL;
        parentScope = ns;
    }

    sym = NULL;
    // Place name of class in name table.
    while (className) {
        if (nestedName) {
            name = compiler()->namemgr->AddString(nestedName, (int)(nestedName - className));
            className = nestedName + 1;
            nestedName = wcschr(nestedName, NESTED_CLASS_SEP);
        } else {
            name = compiler()->namemgr->AddString(className);
            className = NULL;
        }
        if (sym)
            parentScope = parentDecl = sym;

RE_LOOKUP:
	    sym = NULL;
        symConflict = compiler()->symmgr.LookupGlobalSym(name, parentScope, MASK_ALL);
        if (symConflict) {
            if (symConflict->kind == SK_NSSYM) {
                // A user-defined namespace is conflicting with an imported type name.
                // Issue an error.
                // CONSIDER: can we set the error location better?
                compiler()->clsDeclRec.errorStrSymbol(NULL, inputfile, compiler()->ErrSym(symConflict), symConflict, ERR_NamespaceTypeConflict);
            }

            if (symConflict->kind == SK_AGGSYM)
            {
                // Already have the correct class..
                sym = symConflict->asAGGSYM();
            }
            else {
                // Not a valid class -- bogus.
                return NULL;
            }
        }

        if (!sym) {
            // If none found, then create a symbol for the new type.
            if (!mustExist) {
                if (!parentDecl)
                    parentDecl = GetNSDecl(ns, inputfile, false);
		        else if (parentDecl->getInputFile() != inputfile) {
			        // The outer class is not in the same input file as the unresolved inner-class
			        // so to keep our error reporting straight,
			        // change the class name to be the fully scoped name (with dots)
			        // and attach it directly to the global scope for the correct input file
			        WCHAR buffer[MAX_FULLNAME_SIZE];
			        size_t len = 0;
			        if (METADATAHELPER::GetFullMetadataName( parentScope, buffer, MAX_FULLNAME_SIZE) &&
				        ((len = wcslen(buffer)) + wcslen(name->text) + 1) < lengthof(buffer)) 
					{
				        buffer[len++] = L'.';
				        wcscpy(buffer + len, name->text);
				        name = compiler()->namemgr->AddString(buffer);
			        }

			        // REDO the lookup in case we've already seen this type before and already
			        // created the bogus type
			        parentDecl = inputfile->rootDeclaration;
			        parentScope = inputfile->rootDeclaration->namespaceSymbol;
			        goto RE_LOOKUP;
		        }

                sym = compiler()->symmgr.CreateAggregate(name, parentDecl);
            } else {
                // early out
                return NULL;
            }
        }
    }

    return sym;
}


/*
 * Resolves a typeref or typedef to just a top-level class name. If it can't be resolved, or resolves to a nested class,
 * then false is returned.
 */
bool IMPORTER::GetTypeRefFullName(PINFILESYM inputfile, DWORD scope, mdToken token, WCHAR * fullnameText, ULONG cchBuffer)
{
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    mdToken tkResolutionScope = mdTokenNil;
    ULONG cchFullNameText;
    DWORD flags;
    HRESULT hr;

    if (TypeFromToken(token) == mdtTypeRef) {
        // Get the full name of the referenced type.
        TimerStart(TIME_IMI_GETTYPEREFPROPS);
        hr = metaimport->GetTypeRefProps(
		    token,					    // typeref token					
                    &tkResolutionScope,                             // resolution scope
                    fullnameText, cchBuffer, &cchFullNameText);     // Type name
        TimerStop(TIME_IMI_GETTYPEREFPROPS);

        if (FAILED(hr) || TypeFromToken(tkResolutionScope) == mdtTypeRef || TypeFromToken(tkResolutionScope) == mdtTypeDef) 
            return false; // failure or nested type.
    }
    else {
        TimerStart(TIME_IMI_GETTYPEDEFPROPS);
        hr = metaimport->GetTypeDefProps(token,
                fullnameText, cchBuffer, &cchFullNameText,      // Type name
                &flags,                                         // Flags
                NULL);                                          // Extends
        TimerStop(TIME_IMI_GETTYPEDEFPROPS);

        if (FAILED(hr) || IsTdNested(flags))
            return false; // failure or nested type
    }

    return true;
}


/*
 * Resolves a typeref or typedef to an actual class. If the class is unknown, then NULL is returned (but
 * no error is reported to the user.)  If mustExist is true then NEVER creates ANY symbols
 */
PAGGSYM IMPORTER::ResolveTypeRef(PINFILESYM inputfile, DWORD scope, mdToken token, bool mustExist)
{
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR fullNameText[MAX_FULLNAME_SIZE];  // full name of type.
    ULONG cchFullNameText;
    mdToken tkResolutionScope = mdTokenNil;
    DWORD flags;

    if (TypeFromToken(token) == mdtTypeRef) {
        // Get the full name of the referenced type.
        TimerStart(TIME_IMI_GETTYPEREFPROPS);
        CheckHR(metaimport->GetTypeRefProps(
		    token,						         // typeref token					
                    &tkResolutionScope,                                          // resolution scope
                    fullNameText, lengthof(fullNameText), &cchFullNameText),     // Type name
		    inputfile);
        TimerStop(TIME_IMI_GETTYPEREFPROPS);
    }
    else {
        TimerStart(TIME_IMI_GETTYPEDEFPROPS);
        CheckHR(metaimport->GetTypeDefProps(token,
                fullNameText, lengthof(fullNameText), &cchFullNameText,      // Type name
                &flags,                                                        // Flags
                NULL                                                         // Extends
               ), inputfile);
        TimerStop(TIME_IMI_GETTYPEDEFPROPS);

        if (IsTdNested(flags)) {
            // Get the class this class is nested within.
            TimerStart(TIME_IMI_GETNESTEDCLASSPROPS);
            CheckHR(metaimport->GetNestedClassProps(token, &tkResolutionScope), inputfile);
            TimerStop(TIME_IMI_GETNESTEDCLASSPROPS);
        }
    }

    // Resolve the type name.
    AGGSYM * sym = ResolveTypeName(fullNameText, inputfile, scope, tkResolutionScope, mustExist, false);

    if (!mustExist && sym && ! sym->getInputFile()->isSource && sym->tokenImport == 0)
    {
        // We have a typeref that couldn't be resolved to a class that wasn't defined in source or in metadata.
        // Remember the typeref token so that we can give a good error message if we ever attempt to 
        // use (declare) this type.
        sym->tokenImport = token;
        sym->importScope = scope;
    }

    return sym;
}


/*
 * Resolve a base class or interface name. Similar to ResolveTypeRef,
 * but reports error on failure, and forces the base class or interface to
 * be declared.
 */
PAGGSYM IMPORTER::ResolveBaseRef(PINFILESYM inputfile, DWORD scope, mdToken tokenBase, PAGGSYM symDerived)
{
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    PAGGSYM symBase;

    symBase = ResolveTypeRef(inputfile, scope, tokenBase, true);

    if (! symBase) {
        // Couldn't resolve base class. Give a good error message.
        mdToken resolutionScope = 0;
        WCHAR fullName[MAX_FULLNAME_SIZE * 2];
        WCHAR assemblyName[MAX_FULLNAME_SIZE];

        fullName[0] = L'\0';
        assemblyName[0] = L'\0';

        TimerStart(TIME_IMI_GETTYPEREFPROPS);
        metaimport->GetTypeRefProps(
		    tokenBase,						    // typeref token					
                    &resolutionScope,                                       // resolution scope
                    fullName, lengthof(fullName), NULL);                    // name
        TimerStop(TIME_IMI_GETTYPEREFPROPS);

        GetTypeRefAssemblyName(inputfile, scope, tokenBase, assemblyName, lengthof(assemblyName));

        compiler()->Error(ERRLOC(inputfile), ERR_CantImportBase, compiler()->ErrSym(symDerived), fullName, assemblyName);
    }

    return symBase;
}

/*
 * Given a typeref, get the name of the assembly (or module) that the typeref refers to. 
 * Used for better error reporting.
 */
void IMPORTER::GetTypeRefAssemblyName(PINFILESYM inputfile, DWORD scope, mdToken token, WCHAR * assemblyName, LONG cchAssemblyName)
{
    IMetaDataImport * metaimport = inputfile->metaimport[scope];

    ASSERT(TypeFromToken(token) == mdtTypeRef);

    do {
        // From the type ref, get it's resolution scope.
        TimerStart(TIME_IMI_GETTYPEREFPROPS);
        metaimport->GetTypeRefProps(
		    token,						    // typeref token					
                    &token,                                                 // resolution scope
                    NULL, 0, NULL);                                         // name
        TimerStop(TIME_IMI_GETTYPEREFPROPS);

        // repeat until we get out of the typeref chain of resolution scopes.
    } while (TypeFromToken(token) == mdtTypeRef);

    if (TypeFromToken(token) == mdtAssemblyRef) {
        inputfile->assemimport->GetAssemblyRefProps(
            token,                
            NULL, NULL,       
            assemblyName, cchAssemblyName, NULL,  
            NULL, NULL, NULL, NULL);
    }
    else if (TypeFromToken(token) == mdtModuleRef) {
        metaimport->GetModuleRefProps(
            token,
            assemblyName, cchAssemblyName, NULL);
    }
    else {
        ASSERT(0); // Bad resolution scope.
    }
}


/*
 * Imports an interface declared on a class or interface.
 */
PAGGSYM IMPORTER::ImportInterface(PINFILESYM inputfile, DWORD scope, mdInterfaceImpl tokenIntf, PAGGSYM symDerived)
{
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    mdToken tokenInterface;

    // Get typeref and flags.
    TimerStart(TIME_IMI_GETINTERFACEIMPLPROPS);
    CheckHR(metaimport->GetInterfaceImplProps(tokenIntf, NULL, &tokenInterface), inputfile);
    TimerStop(TIME_IMI_GETINTERFACEIMPLPROPS);

    return ResolveBaseRef(inputfile, scope, tokenInterface, symDerived);
}

/*
 * From a flags field, return the access level
 */
ACCESS IMPORTER::ConvertAccessLevel(DWORD flags, const SYM * sym)
{
    ASSERT(fdPublic == mdPublic);
    ASSERT(fdPrivate == mdPrivate);
    ASSERT(fdFamily == mdFamily);
    ASSERT(fdAssembly == mdAssem);
    ASSERT(fdFamANDAssem == mdFamANDAssem);
    ASSERT(fdFamORAssem == mdFamORAssem);
    ASSERT(fdFieldAccessMask == mdMemberAccessMask);


    flags &= mdMemberAccessMask;

    switch (flags) {
    case fdPrivate:
        return ACC_PRIVATE;
    case fdFamily:
        return ACC_PROTECTED;
    case fdPublic:
        return ACC_PUBLIC;
    case fdAssembly:
        return ACC_INTERNAL;
    case fdFamANDAssem:
        // We don't support this directly, treat as protected in our own assembly, internal if in another assembly.
        return sym->getAssemblyIndex() == 0 ? ACC_PROTECTED : ACC_INTERNAL;
    case fdFamORAssem:
        // treat internal protected members in another assembly as just protected
        // as we can't see the internal part, and we want overridesd in this assembly to be protected
        return sym->getAssemblyIndex() == 0 ? ACC_INTERNALPROTECTED : ACC_PROTECTED;
    default:
        return ACC_PRIVATE;
    }
}

/*
 * From a flags field, set the access level of a symbol.
 */
void IMPORTER::SetAccessLevel(PSYM sym, DWORD flags)
{
    sym->access = ConvertAccessLevel(flags, sym);
}


/*
 * Import a single signature definition type. Return NULL if we don't
 * have a corresponding type. sigPtr is updated to point just beyond
 * the end of the type.
 *
 * Note that the type returned is not necessarily declared, which
 * is what we want.
 *
 * Signatures don't distinguish between ref and out parameters,
 * so we just return the base type and return that some byref is present
 * via the returned isByref flags.
 */
PTYPESYM IMPORTER::ImportSigType(PINFILESYM inputfile, DWORD scope, PCCOR_SIGNATURE * sigPtr,
                                 bool canBeVoid, bool canBeByref,
                                 bool *isCmodOpt, bool mustExist)
{
    ASSERT(inputfile->metaimport[scope]);
    PCCOR_SIGNATURE sig = *sigPtr;
    PTYPESYM sym = NULL;
    mdTypeRef token;

    switch (*sig++) {
    case ELEMENT_TYPE_END:
        sym = NULL; break;            // Bogus.

    case ELEMENT_TYPE_VOID:
        if (canBeVoid)
            sym = compiler()->symmgr.GetVoid();
        else
            sym = NULL;        // Void is invalid
        break;

    case ELEMENT_TYPE_BOOLEAN:
        sym = compiler()->symmgr.GetPredefType(PT_BOOL, false); break;

    case ELEMENT_TYPE_CHAR:
        sym = compiler()->symmgr.GetPredefType(PT_CHAR, false); break;

    case ELEMENT_TYPE_U1:
        sym = compiler()->symmgr.GetPredefType(PT_BYTE, false); break;

    case ELEMENT_TYPE_I2:
        sym = compiler()->symmgr.GetPredefType(PT_SHORT, false); break;

    case ELEMENT_TYPE_I4:
        sym = compiler()->symmgr.GetPredefType(PT_INT, false); break;

    case ELEMENT_TYPE_I8:
        sym = compiler()->symmgr.GetPredefType(PT_LONG, false); break;

    case ELEMENT_TYPE_R4:
        sym = compiler()->symmgr.GetPredefType(PT_FLOAT, false); break;

    case ELEMENT_TYPE_R8:
        sym = compiler()->symmgr.GetPredefType(PT_DOUBLE, false); break;

    case ELEMENT_TYPE_STRING:
        sym = compiler()->symmgr.GetPredefType(PT_STRING, false); break;

    case ELEMENT_TYPE_OBJECT:
        sym = compiler()->symmgr.GetPredefType(PT_OBJECT, false); break;

    case ELEMENT_TYPE_I1:      
        sym = compiler()->symmgr.GetPredefType(PT_SBYTE, false); break;

    case ELEMENT_TYPE_U2:    
        sym = compiler()->symmgr.GetPredefType(PT_USHORT, false); break;

    case ELEMENT_TYPE_U4:
        sym = compiler()->symmgr.GetPredefType(PT_UINT, false); break;

    case ELEMENT_TYPE_U8:
        sym = compiler()->symmgr.GetPredefType(PT_ULONG, false); break;

    case ELEMENT_TYPE_I:
        sym = compiler()->symmgr.GetPredefType(PT_INTPTR, false); break;

    case ELEMENT_TYPE_U:
        sym = compiler()->symmgr.GetPredefType(PT_UINTPTR, false); break;

    case ELEMENT_TYPE_TYPEDBYREF:
        sym = compiler()->symmgr.GetPredefType(PT_REFANY, false); break;

    case ELEMENT_TYPE_VALUETYPE:
        // Element of struct type.
        token = CorSigUncompressToken(sig);  // updates sig.
        sym = ResolveTypeRef(inputfile, scope, token, mustExist);
        break;

    case ELEMENT_TYPE_CLASS:
        // Element of class or struct type.
        token = CorSigUncompressToken(sig);  // updates sig.
        sym = ResolveTypeRef(inputfile, scope, token, mustExist);

        // ELEMENT_TYPE_CLASS followe by value type means the "boxed" version, which 
        // we don't support. Check for this case and return NULL.
        if (sym && sym->isStructType())
            sym = NULL;
        break;

    case ELEMENT_TYPE_SZARRAY:
        // Single-dimensional array with 0 lower bound
        sym = ImportSigType(inputfile, scope, &sig, false, false, isCmodOpt, mustExist);  // Get element type.
        if (sym) {
            sym = compiler()->symmgr.GetArray(sym, 1);
        }
        break;


    case ELEMENT_TYPE_ARRAY:
        // Multi-dimensional array. We only support arrays
        // with unspecified length and zero lower bound.

        int rank, lowBound, numRanks;
        bool badArray;

        badArray = false;
        sym = ImportSigType(inputfile, scope, &sig, false, false, isCmodOpt, mustExist);  // Get element type.

        rank = CorSigUncompressData(sig);  // Get rank.
        if (rank == 0) {
            // Unknown rank -- we don't support at all 
            sym = NULL;
            break;
        }

        // Get sizes of each rank.
        numRanks = CorSigUncompressData(sig);
        if (numRanks > 0) {
            // We don't support sizing of arrays.
            badArray = true;
            for (int i = 0; i < numRanks; ++i)
                CorSigUncompressData(sig); // skip the sizes.
        }

        // Get lower bounds of each rank.
        numRanks = *sig++;
        while (numRanks--) {
            lowBound = CorSigUncompressData(sig);
            if (lowBound != 0)
                badArray = true;  // We don't support non-zero lower bounds.
        }

        // Get the array symbol, if its an array type that we support.
        if (badArray || sym == NULL)
            sym = NULL;
        else
            sym = compiler()->symmgr.GetArray(sym, rank);
        break;

    case ELEMENT_TYPE_PTR:
        // Pointer type. Note that void * is a valid type here.
        sym = ImportSigType(inputfile, scope, &sig, true, false, isCmodOpt, mustExist);
        if (sym != NULL)
            sym = compiler()->symmgr.GetPtrType(sym);
        break;

    case ELEMENT_TYPE_BYREF:
        // Byref param - could be ref or out, so just return indication of that.
        sym = ImportSigType(inputfile, scope, &sig, false, false, isCmodOpt, mustExist);
        if (canBeByref && sym)
            sym = compiler()->symmgr.GetParamModifier(sym, false);
        else
            sym = NULL;     // byref isn't allowed here.
        break;

    case ELEMENT_TYPE_CMOD_OPT:
        token = CorSigUncompressToken(sig);  // Ignore the following optional token

        // get the 'real' type here
        sym = ImportSigType(inputfile, scope, &sig, canBeVoid, canBeByref, isCmodOpt, mustExist);
        if (isCmodOpt) {
            *isCmodOpt = true;
        }

        break;

    case ELEMENT_TYPE_CMOD_REQD:
        token = CorSigUncompressToken(sig);  // Ignore the following optional token

        // get the 'real' type here
        ImportSigType(inputfile, scope, &sig, canBeVoid, canBeByref, isCmodOpt, mustExist);

        // We aren't required to understand this, since we don't, just return NULL.
        sym = NULL;
        break;

    default:
        // Something we don't know about.
        if (CorIsModifierElementType((CorElementType) *(sig - 1))) {
            // Consume what is modified.
            ImportSigType(inputfile, scope, &sig, true, true, isCmodOpt, mustExist);
        }
        sym = NULL;
    }

    // Update the signature pointer of the called.
    *sigPtr = sig;
    return sym;
}


/*
 * Import a field type from a signature. Return NULL is type is not supported.
 */
PTYPESYM IMPORTER::ImportFieldType(PINFILESYM inputfile, DWORD scope, PCCOR_SIGNATURE sig
                                   , bool * isVolatile
                                   )
{
    ASSERT(inputfile->metaimport[scope]);
    PTYPESYM sym;

    // Format of field signature is IMAGE_CEE_CS_CALLCONV_FIELD, followed by one type.

    if (*sig++ != IMAGE_CEE_CS_CALLCONV_FIELD)
        return NULL;        // Bogus!
    *isVolatile = false;
    if (*sig == ELEMENT_TYPE_CMOD_REQD || *sig == ELEMENT_TYPE_CMOD_OPT) {
        bool isRequired = (*sig == ELEMENT_TYPE_CMOD_REQD);

        ++sig;
        mdToken token = CorSigUncompressToken(sig); // updates sig
        TYPESYM * modifier = ResolveTypeRef(inputfile, scope, token, true);

        if (modifier != NULL && modifier->isPredefType(PT_VOLATILEMOD)) {
            *isVolatile = true;
        }
        else if (isRequired) {
            return NULL;  // required modifier that we don't understand, can't import type at all.
        }
    }

    sym = ImportSigType(inputfile, scope, &sig, false, false, NULL, false);

    return sym;
}


/*
 * Import a constant value. Return false if the value can't be imported.
 *
 * constVal is the location to put the constant value, and valType is the type that the
 * constant value must be.
 *
 * constType and constValue, constLen are the COM+ metadata type and value and length (in characters).
 */
bool IMPORTER::ImportConstant(CONSTVAL * constVal, ULONG constLen, PTYPESYM valType, DWORD constType, const void * constValue)
{
    __int64 intValue;
    double floatValue;

    // Read the COM+ value into either intValue or floatValue.
    switch (constType)
    {
    case ELEMENT_TYPE_BOOLEAN:
    case ELEMENT_TYPE_I1:
        intValue = *(__int8 *)constValue; goto INTVALUE;
    case ELEMENT_TYPE_U1:
        intValue = *(unsigned __int8 *)constValue; goto INTVALUE;
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_U2:
        intValue = GET_UNALIGNED_VAL16(constValue); goto INTVALUE;
    case ELEMENT_TYPE_U4:
        intValue = GET_UNALIGNED_VAL32(constValue); goto INTVALUE;
    case ELEMENT_TYPE_U8:
        intValue = GET_UNALIGNED_VAL64(constValue); goto INTVALUE;
    case ELEMENT_TYPE_I2:
        intValue = (INT16)GET_UNALIGNED_VAL16(constValue); goto INTVALUE;
    case ELEMENT_TYPE_I4:
        intValue = (INT32)GET_UNALIGNED_VAL32(constValue); goto INTVALUE;
    case ELEMENT_TYPE_I8:
        intValue = (INT64)GET_UNALIGNED_VAL64(constValue); goto INTVALUE;
    case ELEMENT_TYPE_R4:
        {
            __int32 Value = GET_UNALIGNED_VAL32(constValue);
            floatValue = (float &)Value;
            goto FLOATVALUE;
        }
    case ELEMENT_TYPE_R8:
        {
            __int64 Value = GET_UNALIGNED_VAL64(constValue);
            floatValue = (double &) Value;
            goto FLOATVALUE;
        }
    case ELEMENT_TYPE_STRING:
    {
        // String constant.

        STRCONST * strConst;

        // Must have a string type to put it in.
        if (! valType->isPredefType(PT_STRING))
            return false;

        // Allocate memory for the string constant.
        strConst = (STRCONST *) compiler()->globalSymAlloc.Alloc(sizeof(STRCONST));

        // Read the string constant. Currently this is zero terminated, and may be NULL for empty string.
        // The "null" constant is persisted as ELEMENT_TYPE_CLASS.
        if (constValue == NULL) {
            strConst->length = 0;
            strConst->text = NULL;
        }
        else {
            int cch = (int)constLen;
            strConst->length = cch;
            strConst->text = (WCHAR *) compiler()->globalSymAlloc.Alloc(cch * sizeof(WCHAR));
            memcpy(strConst->text, constValue, cch * sizeof(WCHAR));
            SwapStringLength(strConst->text, cch);
        }

        constVal->strVal = strConst;
        return true;
    }

    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
        // Only reasonable value is null.
        if (constValue != NULL && 
            constLen != 0 &&
            (constLen != 4 || GET_UNALIGNED_VAL32(constValue)))
            return false;

        if (valType->isPredefType(PT_STRING)) {
            // Strings must use STRCONST structure to hold NULL.
            STRCONST * strConst;

            // Allocate memory for the string constant.
            strConst = (STRCONST *) compiler()->globalSymAlloc.Alloc(sizeof(STRCONST));
            strConst->length = 0;
            strConst->text = NULL;
            constVal->strVal = strConst;
        }

        // Use integer constant to represent null constant.
        constVal->iVal = 0;
        return true;

    default:
        return false;
    }

    // we never reach here.

INTVALUE:
    // An integer value was read. Coerce it to the type of integer
    // value that we were expecting.

    // NOTE: fundType is known after resolving base classes (and asserts that fact)
    compiler()->clsDeclRec.resolveInheritanceRec(valType->asAGGSYM());
    switch (valType->fundType()) {
    case FT_I1:
        if (valType->isPredefType(PT_BOOL)) {
            constVal->iVal = ((intValue != 0) ? true : false);
            break;
        }
        // else FALL THROUGH to usual int case.
    case FT_U1:
    case FT_I2:
    case FT_U2:
    case FT_I4:
    case FT_U4:
        constVal->iVal = (int) intValue; break;

    case FT_I8:
    case FT_U8:
        // 64-bit constant must be allocated.
        constVal->longVal = (__int64 *) compiler()->globalSymAlloc.Alloc(sizeof(__int64));
        *constVal->longVal = intValue;
        break;

    default:
        // Not a valid type to hold an integer.
        return false;
    }

    return true;

FLOATVALUE:
    // A floating point value was read. Coerce it to the type of float
    // that we were expecting.

    switch (valType->fundType()) {
    case FT_R4:
    case FT_R8:
        // 64-bit constant must be allocated.
        constVal->doubleVal = (double *) compiler()->globalSymAlloc.Alloc(sizeof(double));
        *constVal->doubleVal = floatValue;
        break;

    default:
        // Not a valid type to hold a float.
        return false;
    }

    return true;
}


/*
 * Import a fielddef and create a corresponding MEMBVARSYM symbol. If we can't import
 * the field because it has attributes that we don't support and can't safely ignore,
 * we set the "isBogus" flag on the field. If we ever try to use that field, we'll give
 * an error (but if we don't use the field, the user never knows or cares).
 */
void IMPORTER::ImportField(PINFILESYM inputfile, AGGSYM * parent, mdFieldDef tokenField)
{
    DWORD scope = parent->getImportScope();
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR fieldnameText[MAX_IDENT_SIZE];     // name of field
    ULONG cchFieldnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    DWORD constType;
    LPCVOID constValue;
    ULONG constLen;
    PNAME name;
    MEMBVARSYM * sym;
    PTYPESYM type;
    bool isVolatile;

    // Get properties of the field from metadata.
    TimerStart(TIME_IMI_GETFIELDPROPS);
    CheckHR(metaimport->GetFieldProps(
		tokenField, 		                                    // The field for which to get props.
		NULL,		                                            // Put field's class here.
		fieldnameText, lengthof(fieldnameText), & cchFieldnameText, // Field name
		& flags,		                                    // Field flags
		& signature, & cbSignature,                                 // Field signature
		& constType, & constValue, &constLen),                      // Field constant value
                inputfile);
    TimerStop(TIME_IMI_GETFIELDPROPS);

    // Get name.
    if (cchFieldnameText <= 1)
        return;
    name = compiler()->namemgr->AddString(fieldnameText, cchFieldnameText - 1);

    // Import the type of the field.
    type = ImportFieldType(inputfile, scope, signature, &isVolatile);

    // Enums are a bit special. Non-static fields serve only to record the
    // underlying integral type, and are otherwise ignored. Static fields are
    // enumerators and must be of the enum type. (We change other integral ones to the
    // enum type because it's probably what the emitting compiler meant.)
    if (parent->isEnum) {
        if (flags & fdStatic) {
            if (type != parent) {
                if (type->fundType() <= FT_LASTINTEGRAL)
                    type = parent;   // If it's integral, assume it's meant to be the enum type.
                else
                    type = NULL;     // Bogus type.
            }
        }
        else {
            // Assuming its an integral type, use it to set the
            // enum base type.
            // NOTE: This gets done in the resolve inheritance phase now
            if (type->isNumericType() && type->fundType() <= FT_LASTINTEGRAL)
                ASSERT(parent->underlyingType == type->asAGGSYM());
            return;
        }
    }

    // Declare a field. If we get a name conflict, just ignore the whole field, since
    // there's not much usefulness in report this to the user.
    // Check for conflict.
    if (compiler()->symmgr.LookupGlobalSym(name, parent, MASK_ALL)) {
        return;  // Already declared one.
    }
    sym = compiler()->symmgr.CreateMembVar(name, parent);
    sym->tokenImport = tokenField;

    // Record the type.
    sym->type = type;
    if (!sym->type) {
        sym->type = compiler()->symmgr.GetErrorSym();
        sym->isBogus = true;
    }


    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, tokenField);

    sym->isDeprecated = attributes.isDeprecated;
    sym->deprecatedMessage = attributes.deprecatedString;
    sym->isDeprecatedError = attributes.isDeprecatedError;
    if (attributes.hasCLSattribute) {
        sym->hasCLSattribute = true;
        sym->isCLS = attributes.isCLS;
    }

    SetAccessLevel(sym, flags);

    sym->isStatic = (flags & fdStatic) ? true : false;
    sym->isVolatile = isVolatile;

    if ((flags & fdLiteral) && constType != ELEMENT_TYPE_VOID && 
        (! sym->type->isPredefType(PT_DECIMAL))) 
    {
        // A compile time constant.
        sym->isConst = true;
        sym->isStatic = true;

        if (! sym->isBogus) {
            // Try to import the value from COM+ metadata.
            if ( ! ImportConstant(&sym->constVal, constLen, type, constType, constValue))
                sym->isBogus = true;
        }
    }
    else if (flags & fdInitOnly) {
        // A readonly field.
        sym->isReadOnly = true;
    }

    // decimal literals are stored in a custom blob since they can't be represented MD directly
    if ((flags & fdInitOnly) && (flags & fdStatic) && sym->type->isPredefType(PT_DECIMAL)) {

        if (attributes.hasDecimalLiteral) {
            sym->constVal.decVal = (DECIMAL*) compiler()->globalSymAlloc.Alloc(sizeof(DECIMAL));
            *sym->constVal.decVal = attributes.decimalLiteral;
            sym->isConst = true;
            sym->isReadOnly = false;
        }
    }

    if (parent->isEnum && !sym->isConst) {
        // Enum members better be read-only constants.
        sym->isBogus = true;
    }
}

/*
 * Read all the custom attributes on a members and handle them appropriately
 */
void IMPORTER::ImportCustomAttributes(PINFILESYM inputfile, DWORD scope, IMPORTED_CUSTOM_ATTRIBUTES * attributes, mdToken token)
{
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    HCORENUM corenum;           // For enumerating tokens.
    HALINKENUM hEnum;           // For enumerating tokens
    mdToken attributesBuffer[32];
    ULONG cAttributes, iAttribute;
    corenum = 0;

    memset(attributes, 0, sizeof(*attributes));
    attributes->conditionalHead = &attributes->conditionalSymbols;
    if (token == mdtAssembly) {
        CheckHR(compiler()->linker->ImportTypes(compiler()->assemID, inputfile->mdImpFile , scope, &hEnum, NULL, NULL), inputfile);
    }

    do {
        // Get next batch of attributes.
        TimerStart(TIME_IMI_ENUMCUSTOMATTRIBUTES);
        if (token == mdtAssembly) 
            CheckHR(compiler()->linker->EnumCustomAttributes(hEnum, 0, attributesBuffer, lengthof(attributesBuffer), &cAttributes), inputfile);
        else
            CheckHR(metaimport->EnumCustomAttributes(&corenum, token, 0, attributesBuffer, lengthof(attributesBuffer), &cAttributes), inputfile);
        TimerStop(TIME_IMI_ENUMCUSTOMATTRIBUTES);

        // Process each attribute.
        for (iAttribute = 0; iAttribute < cAttributes; ++iAttribute) {
            mdToken attrToken;
            const void * pvData;
            ULONG cbSize;

            TimerStart(TIME_IMI_GETCUSTOMATTRIBUTEPROPS);
            CheckHR(metaimport->GetCustomAttributeProps(
                attributesBuffer[iAttribute],
                NULL,
                &attrToken,
                &pvData,
                &cbSize), inputfile);
            TimerStop(TIME_IMI_GETCUSTOMATTRIBUTEPROPS);

            ImportOneCustomAttribute(inputfile, scope, attributes, attrToken, pvData, cbSize);
        }
    } while (cAttributes > 0);

    if (token == mdtAssembly) {
        CheckHR(compiler()->linker->CloseEnum(hEnum), inputfile);
    }
    else {
        metaimport->CloseEnum(corenum);
    }
}

/*
 * Handle one custom attribute read in.
 */
void IMPORTER::ImportOneCustomAttribute(PINFILESYM inputfile, DWORD scope, IMPORTED_CUSTOM_ATTRIBUTES * attributes, mdToken attrToken, const void * pvData, ULONG cbSize)
{
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    mdToken typeToken;
    WCHAR ctornameText[MAX_IDENT_SIZE];     // name of field
    ULONG cchCtornameText;
    WCHAR fullNameText[MAX_FULLNAME_SIZE];
    ULONG cchFullNameText;
    PNAME typeName;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature = 0;
    IMPORTEDCUSTOMATTRKIND attrKind;

    if (TypeFromToken(attrToken) == mdtMemberRef) {
        // Make sure that data is in the correct format. Check the prolog, then
        // move beyond it.
        if (cbSize < sizeof(WORD) || GET_UNALIGNED_VAL16(pvData) != 1)
            return;
        cbSize -= sizeof(WORD);
        pvData = (WORD *)pvData + 1;

        // Get name of member and signature.
        TimerStart(TIME_IMI_GETMEMBERREFPROPS);
        CheckHR(metaimport->GetMemberRefProps(
                    attrToken,
                    &typeToken,
		    ctornameText, lengthof(ctornameText), & cchCtornameText, // Field name
                    &signature, &cbSignature), inputfile);
        TimerStop(TIME_IMI_GETMEMBERREFPROPS);

        if (cchCtornameText == 0 || wcscmp(ctornameText, compiler()->namemgr->GetPredefName(PN_CTOR)->text) != 0)
            return ;  // Member ref didn't reference a constructor.
    } else if (TypeFromToken(attrToken) == mdtMethodDef) {
        // Make sure that data is in the correct format. Check the prolog, then
        // move beyond it.
        if (cbSize < sizeof(WORD) || GET_UNALIGNED_VAL16(pvData) != 1)
            return;
        cbSize -= sizeof(WORD);
        pvData = (WORD *)pvData + 1;

        // Get name of member and signature.
        TimerStart(TIME_IMI_GETMETHODPROPS);
        CheckHR(metaimport->GetMethodProps(
                    attrToken,
                    &typeToken,
                    ctornameText, lengthof(ctornameText), & cchCtornameText, // Field name
                    NULL,
                    &signature, &cbSignature,
                    NULL, NULL), inputfile);
        TimerStop(TIME_IMI_GETMETHODPROPS);

        if (wcscmp(ctornameText, compiler()->namemgr->GetPredefName(PN_CTOR)->text) != 0)
            return ;  // Member ref didn't reference a constructor.
    }
    else if (TypeFromToken(attrToken) == mdtTypeRef) {
        typeToken = attrToken;
        cbSignature = 0;
    }
    else
        return;  // We only handle TypeRefs or MemberRefs.

    if (TypeFromToken(typeToken) == mdtTypeRef) {
        // Get name of type.
        TimerStart(TIME_IMI_GETTYPEREFPROPS);
        CheckHR(metaimport->GetTypeRefProps(
		    typeToken,						   // typeref token					
                    NULL,                                                  // resolution scope
                    fullNameText, lengthof(fullNameText), &cchFullNameText),// name
		    inputfile);
        TimerStop(TIME_IMI_GETTYPEREFPROPS);
    } else if (TypeFromToken(typeToken) == mdtTypeDef) {
        // Get name of type.
        TimerStart(TIME_IMI_GETTYPEDEFPROPS);
        CheckHR(metaimport->GetTypeDefProps(
		    typeToken,						    // typeref token					
                    fullNameText, lengthof(fullNameText), &cchFullNameText,  // name
                    NULL, NULL),
		    inputfile);
        TimerStop(TIME_IMI_GETTYPEDEFPROPS);
    } else
        return;

    // Convert name to a NAME by looking up in hash table.
    typeName = compiler()->namemgr->LookupString(fullNameText);
    if (typeName == NULL)
        return;  // Not a known name.

    // Match type name and signature against list of predefined ones.
    attrKind = CUSTOMATTR_NONE;
    for (unsigned int i = 0; i < lengthof(g_importedAttributes); ++i) {
        if (compiler()->namemgr->GetPredefName(g_importedAttributes[i].className) == typeName &&
            (g_importedAttributes[i].cbSig < 0 ||
             ((unsigned) g_importedAttributes[i].cbSig == cbSignature &&
             memcmp(g_importedAttributes[i].sig, signature, cbSignature) == 0)))
        {
            attrKind = g_importedAttributes[i].attrKind;
            break;
        }
    }

    if (attrKind == CUSTOMATTR_NONE)
        return;         // Not a predefined custom attribute kind.

    // We found and attribute kind that we know about. Grab any information from the binary data, and
    // apply it to the symbol.
    switch (attrKind) {
    case CUSTOMATTR_DEPRECATED_VOID:
        attributes->isDeprecated = true;
        break;

    case CUSTOMATTR_DEPRECATED_STR:
        attributes->isDeprecated = true;
        attributes->deprecatedString = ImportCustomAttrArgString(& pvData, & cbSize);
        break;

    case CUSTOMATTR_DEPRECATED_STRBOOL:
        attributes->isDeprecated = true;
        attributes->deprecatedString = ImportCustomAttrArgString(& pvData, & cbSize);
        attributes->isDeprecatedError = ImportCustomAttrArgBool(& pvData, & cbSize);
        break;

    case CUSTOMATTR_ATTRIBUTEUSAGE_VOID:
        {
            attributes->attributeKind = catAll;
            int numberOfNamedArguments = ImportCustomAttrArgWORD(&pvData, &cbSize);
            for (int i = 0; i < numberOfNamedArguments; i += 1) {
                LPCWSTR name;
                TYPESYM *type;
                ImportNamedCustomAttrArg(&pvData, &cbSize, &name, &type);
                if (!wcscmp(name, compiler()->namemgr->GetPredefName(PN_VALIDON)->text)) {
                    attributes->allowMultiple = !!(CorAttributeTargets) ImportCustomAttrArgInt(&pvData, &cbSize);
                } else if (!wcscmp(name, compiler()->namemgr->GetPredefName(PN_ALLOWMULTIPLE)->text)) {
                    attributes->allowMultiple = (ImportCustomAttrArgBYTE(&pvData, &cbSize) ? true : false);
                } else if (!wcscmp(name, compiler()->namemgr->GetPredefName(PN_INHERITED)->text)) {
                    ImportCustomAttrArgBYTE(&pvData, &cbSize);
                } else {
                    ASSERT(!"Unknown named argument for imported attributeusage");
                    break;
                }
            }
        }
        break;

    case CUSTOMATTR_ATTRIBUTEUSAGE_VALIDON:
        {
            attributes->attributeKind = (CorAttributeTargets) ImportCustomAttrArgInt(& pvData, & cbSize);
            int numberOfNamedArguments = ImportCustomAttrArgWORD(&pvData, &cbSize);
            for (int i = 0; i < numberOfNamedArguments; i += 1) {
                LPCWSTR name;
                TYPESYM *type;
                ImportNamedCustomAttrArg(&pvData, &cbSize, &name, &type);
                if (!wcscmp(name, compiler()->namemgr->GetPredefName(PN_ALLOWMULTIPLE)->text)) {
                    attributes->allowMultiple = (ImportCustomAttrArgBYTE(&pvData, &cbSize) ? true : false);
                } else if (!wcscmp(name, compiler()->namemgr->GetPredefName(PN_INHERITED)->text)) {
                    ImportCustomAttrArgBYTE(&pvData, &cbSize);
                } else {
                    ASSERT(!"Unknown named argument for imported attributeusage");
                    break;
                }
            }
        }
        break;

    case CUSTOMATTR_CONDITIONAL:
        {
        //
        // convert the string to a name and return it
        //
        compiler()->symmgr.AddToGlobalNameList(
                compiler()->namemgr->AddString(ImportCustomAttrArgString(& pvData, & cbSize)), 
                &attributes->conditionalHead);
        }
        break;

    case CUSTOMATTR_DECIMALLITERAL:
        if ((cbSize + sizeof(WORD)) == sizeof(DecimalConstantBuffer)) {
			DecimalConstantBuffer *buffer = (DecimalConstantBuffer*)(((BYTE*)pvData)-(int)sizeof(WORD));
            DECIMAL_SCALE(attributes->decimalLiteral)	= buffer->scale;
            DECIMAL_SIGN(attributes->decimalLiteral)	= buffer->sign;
            DECIMAL_HI32(attributes->decimalLiteral)	= GET_UNALIGNED_VAL32(&buffer->hi);
            DECIMAL_MID32(attributes->decimalLiteral)	= GET_UNALIGNED_VAL32(&buffer->mid);
            DECIMAL_LO32(attributes->decimalLiteral)	= GET_UNALIGNED_VAL32(&buffer->low);
            attributes->hasDecimalLiteral = true;
        }
        break;

    case CUSTOMATTR_DEPRECATEDHACK:
        attributes->isDeprecated = true;
        break;

    case CUSTOMATTR_CLSCOMPLIANT:
        attributes->hasCLSattribute = true;
        attributes->isCLS = ImportCustomAttrArgBool(& pvData, & cbSize);
        break;

    case CUSTOMATTR_PARAMS:
        attributes->isParamListArray = true;
        break;

    case CUSTOMATTR_REQUIRED:
        attributes->hasRequiredAttribute = true;
        break;

    case CUSTOMATTR_DEFAULTMEMBER2:
    case CUSTOMATTR_DEFAULTMEMBER:
        attributes->defaultMember = ImportCustomAttrArgString(& pvData, & cbSize);
        break;

    case CUSTOMATTR_COCLASS:
        attributes->CoClassName = ImportCustomAttrArgString(& pvData, & cbSize);
        break;

    default:
        break;
    }
}

/*
 * Import a single custom attribute argument that is a string. Updates *ppvData and *pcbSize 
 */
WCHAR * IMPORTER::ImportCustomAttrArgString(LPCVOID * ppvData, ULONG * pcbSize)
{
    PCCOR_SIGNATURE psigData;
    PCSTR pstrData;
    int cbString, cchString;
    WCHAR * str;

    // String is stored as compressed length, UTF8.
    psigData = (PCCOR_SIGNATURE) *ppvData;
    if (*psigData == 0xFF) {
        pstrData = (PCSTR) (psigData + 1);
        str = NULL;
    }
    else {
        cbString = CorSigUncompressData(psigData);
        pstrData = (PCSTR) psigData;
        cchString = UnicodeLengthOfUTF8(pstrData, cbString) + 1;
        str = (WCHAR *) compiler()->globalSymAlloc.Alloc(cchString * sizeof(WCHAR));
        UTF8ToUnicode(pstrData, cbString, str, cchString);
        str[cchString - 1] = L'\0';
        pstrData += cbString;
    }

    *pcbSize -= (ULONG)((BYTE *)pstrData - (BYTE *) *ppvData);
    *ppvData = pstrData;

    return str;
}


/*
 * Import a single custom attribute argument that is a bool. Updates *ppvData and *pcbSize 
 */
bool IMPORTER::ImportCustomAttrArgBool(LPCVOID * ppvData, ULONG * pcbSize)
{
    const BYTE * pbData = (BYTE *) *ppvData;
    BYTE b = *pbData++;
    *ppvData = pbData;
    *pcbSize -= sizeof(BYTE);

    return b ? true : false;
}


/*
 * Import a single custom attribute argument that is a int32. Updates *ppvData and *pcbSize 
 */
int IMPORTER::ImportCustomAttrArgInt(LPCVOID * ppvData, ULONG * pcbSize)
{
    const __int32 * pbData = (__int32 *) *ppvData;
    int i = GET_UNALIGNED_VAL32(pbData);
    pbData++;
    *ppvData = pbData;
    *pcbSize -= sizeof(__int32);

    return i;
}

/*
 * Import a single custom attribute argument that is a int16. Updates *ppvData and *pcbSize 
 */
WORD IMPORTER::ImportCustomAttrArgWORD(LPCVOID * ppvData, ULONG * pcbSize)
{
    const WORD * pbData = (WORD *) *ppvData;
    WORD w = GET_UNALIGNED_VAL16(pbData);
    pbData++;
    *ppvData = pbData;
    *pcbSize -= sizeof(WORD);

    return w;
}

/*
 * Import a single custom attribute argument that is a int16. Updates *ppvData and *pcbSize 
 */
BYTE IMPORTER::ImportCustomAttrArgBYTE(LPCVOID * ppvData, ULONG * pcbSize)
{
    const BYTE * pbData = (BYTE *) *ppvData;
    BYTE b = *pbData++;
    *ppvData = pbData;
    *pcbSize -= sizeof(BYTE);

    return b;
}

static const PREDEFTYPE g_stTOptMap[] = 
{
    PT_BOOL,
    PT_CHAR,
    PT_SBYTE,
    PT_BYTE,
    PT_SHORT,
    PT_USHORT,
    PT_INT,
    PT_UINT,
    PT_LONG,
    PT_ULONG,
    PT_FLOAT,
    PT_DOUBLE,
    PT_STRING,
};

/*
 * Imports a single named custom attribute argument.
 */
void IMPORTER::ImportNamedCustomAttrArg(LPCVOID *ppvData, ULONG *pcbSize, LPCWSTR *pname, TYPESYM **type)
{
    ImportCustomAttrArgBYTE(ppvData, pcbSize);

    //
    // read in the type
    //
    CorSerializationType st = (CorSerializationType) ImportCustomAttrArgBYTE(ppvData, pcbSize);
    if (st >= SERIALIZATION_TYPE_BOOLEAN && st <= SERIALIZATION_TYPE_STRING) {  
        *type = compiler()->symmgr.GetPredefType(g_stTOptMap[st], false);
    } else {
        switch (st) {
	case SERIALIZATION_TYPE_SZARRAY:
	case SERIALIZATION_TYPE_TYPE:
	case SERIALIZATION_TYPE_TAGGED_OBJECT:
            ASSERT(!"NYI: named custom attr arg type");
            break;

	case SERIALIZATION_TYPE_ENUM:
        {
            LPCWSTR className;
            className = ImportCustomAttrArgString(ppvData, pcbSize);
            ASSERT(!wcscmp(className, L"System.AttributeTargets"));
            *type = compiler()->symmgr.GetPredefType(PT_ATTRIBUTETARGETS, false);
        }
	default:
            break;
        }
    }

    //
    // read in the name
    //
    *pname = ImportCustomAttrArgString(ppvData, pcbSize);
}


/*
 * Just return the number of parameters from a signature.
 */
int IMPORTER::GetSignatureCParams(PCCOR_SIGNATURE sig)
{
    // Skip signature.
    sig++;

    // Get param count.
    return CorSigUncompressData(sig);
}

/*
 * Import a method/property ret type and params from a signature. Sets the isBogus flag
 * if the signature has a type we don't handle.
 */
void IMPORTER::ImportMethodOrPropSignature(PINFILESYM inputfile, mdMethodDef token,
                                           PCCOR_SIGNATURE sig, PMETHPROPSYM sym)
{
    ASSERT(inputfile->metaimport);

    bool isBogus = false;
    bool isVarargs = false;
    bool isCmodOpt = false;
    bool isParams = false;
    ImportSignature(inputfile, sym->getImportScope(), token, sig, (sym->kind == SK_METHSYM), sym->isStatic, &sym->retType, 
                    &sym->cParams, &sym->params, &isBogus, &isVarargs, &isParams, &isCmodOpt);
    sym->isBogus = isBogus;
    sym->isVarargs = isVarargs;
    sym->isParamArray = isParams;
    sym->hasCmodOpt = isCmodOpt;
}

void IMPORTER::ImportSignature(
    PINFILESYM inputfile, 
    DWORD scope,
    mdMethodDef token,
    PCCOR_SIGNATURE sig, 
    bool isMethod, 
    bool isStatic, 
    TYPESYM **retType,
    unsigned short *cParams, 
    TYPESYM *** params, 
    bool *isBogus,
    bool *isVarargs,
    bool *isParams,
    bool *isCmodOpt)
{
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    int iParam;
    BYTE callConv;
    PTYPESYM * paramTypes;
    HRESULT hr;

    // Format of method signature is calling convention, followed by param count,
    // return type, then param types.

    // Deal with calling convention. Must be default or varargs for methods, default or property for properties.
    callConv = *sig++;
    if (((callConv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_DEFAULT) &&
        ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_UNMGD) &&
        ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_PROPERTY || isMethod) &&
        ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_VARARG || !isMethod))
    {
        *isBogus = true;        // We don't support that calling convention.
        *retType = compiler()->symmgr.GetErrorSym();
        return;
    }

    // Presence of this pointer should match the static flag.
    if (callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS) {
        if (isStatic) {
            *isBogus = true;
            *retType = compiler()->symmgr.GetErrorSym();
            return;
        }
    }
    else {
    //    if (! sym->isStatic)
    //        sym->isBogus = true;
    }

    // Check for varargs
    if ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) == IMAGE_CEE_CS_CALLCONV_VARARG) {
        *isVarargs = true;
    }

    // Get param count.
    *cParams = (unsigned short) CorSigUncompressData(sig);
    unsigned short cParamsVarArgs = *cParams + ((*isVarargs) ? 1 : 0);

    // Get return type.
    *retType = ImportSigType(inputfile, scope, &sig, true, false, isCmodOpt, false);
    if (! *retType) {
        *isBogus = true;
        *retType = compiler()->symmgr.GetErrorSym();
        return;
    }


    // Alloc some space on the stack for the param types.
    paramTypes = (PTYPESYM *) _alloca(cParamsVarArgs * sizeof(PTYPESYM));

    // Grab the parameters.
    for (iParam = 0; iParam < *cParams; ++iParam) {
        PTYPESYM type;

        // Get the type of this next parameter.
        if (*isBogus)
            type = NULL;  // something went wrong earlier -- don't continue reading types.
        else
            type = ImportSigType(inputfile, scope, &sig, false, true, isCmodOpt, false);

        // check for out param
        if (type && type->kind == SK_PARAMMODSYM && type->asPARAMMODSYM()->isRef && isMethod) {
            // It's a byref parameter. We need to get the param flags to determine
            // if it is ref or out calling mode.
            ULONG sequenceNum = iParam + 1;
            mdParamDef paramToken;

            // Get the param flags for this parameter.
            TimerStart(TIME_IMI_GETPARAMFORMETHODINDEX);
            hr = metaimport->GetParamForMethodIndex(
                token,
                sequenceNum,
                &paramToken);
            TimerStop(TIME_IMI_GETPARAMFORMETHODINDEX);

            // If a parameter record is missing, that's OK, just assume flags are zero.
            if (SUCCEEDED(hr)) {
                DWORD paramFlags;
                TimerStart(TIME_IMI_GETPARAMPROPS);
                CheckHR(metaimport->GetParamProps(
                                    paramToken, 
                                    NULL,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL,
                                    &paramFlags,
                                    NULL,
                                    NULL,
                                    NULL), inputfile);
                TimerStop(TIME_IMI_GETPARAMPROPS);

			    // Is it a ref or out? If only pdOut is set, it's an out, otherwise a ref.
                if ((paramFlags & (pdOut | pdIn))== pdOut)
                    type = compiler()->symmgr.GetParamModifier(type->asPARAMMODSYM()->paramType(), true);
            }
        }

        // Save the param type.
        paramTypes[iParam] = type;
        if (!type) {
            *isBogus = true;
            paramTypes[iParam] = compiler()->symmgr.GetErrorSym();
        }
    }

    if (*isVarargs) {
        paramTypes[*cParams] = compiler()->symmgr.GetArglistSym();
        *cParams = cParamsVarArgs;
    } else if (iParam && isMethod) {
        // maybe its a param array...
        if (paramTypes[iParam - 1]->kind == SK_ARRAYSYM && paramTypes[iParam - 1]->asARRAYSYM()->rank == 1) {
            *isParams = !!IsParamArray(inputfile, scope, token, iParam);
        }
    }

    // Record the signature and param count.
    *params = compiler()->symmgr.AllocParams(cParamsVarArgs, paramTypes);
}

BOOL IMPORTER::IsParamArray(
    PINFILESYM inputfile, 
    DWORD scope,
    mdMethodDef methodToken, 
    int iParam
    )
{
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];

    mdParamDef paramToken;
    // Get the param flags for this parameter.
    TimerStart(TIME_IMI_GETPARAMFORMETHODINDEX);
    HRESULT hr = metaimport->GetParamForMethodIndex(
        methodToken,
        iParam,
        &paramToken);
    TimerStop(TIME_IMI_GETPARAMFORMETHODINDEX);

    if (SUCCEEDED(hr)) { // if failed, could be just because parameter doesn't exist.
        IMPORTED_CUSTOM_ATTRIBUTES attributes;
        ImportCustomAttributes(inputfile, scope, &attributes, paramToken);

        if (attributes.isParamListArray) {
            return true;
        }
    }
    
    return false;
}

/*
 * Import a methoddef and create a corresponding METHSYM symbol. If we can't import
 * the method because it has attributes/types that we don't support and can't safely ignore,
 * we set the "isBogus" flag on the method. If we ever try to use that method, we'll give
 * an error (but if we don't use the method, the user never knows or cares).
 */
void IMPORTER::ImportMethod(PINFILESYM inputfile, AGGSYM * parent, mdMethodDef tokenMethod)
{
    DWORD scope = parent->getImportScope();
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR methodnameText[MAX_IDENT_SIZE];     // name of method
    ULONG cchMethodnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    PNAME name;
    METHSYM * sym;
    bool isImplicit = false;
    bool isExplicit = false;
    bool isOperator = false;

    // Get method properties.
    TimerStart(TIME_IMI_GETMETHODPROPS);
    CheckHR(metaimport->GetMethodProps(
		tokenMethod, 					                // The method for which to get props.
		NULL,				                                // Put method's class here.
		methodnameText,	lengthof(methodnameText), &cchMethodnameText,   // Method name
		& flags,			                                // Put flags here.
		& signature, & cbSignature,		                        // Method signature
		NULL,			                                        // codeRVA
		NULL), inputfile);                                              // Impl. Flags
    TimerStop(TIME_IMI_GETMETHODPROPS);

    if (cchMethodnameText <= 1)
        return;
        
    //BASSERT(lstrcmpW(methodnameText, L"WriteLine"));

    // If the name contains a '.', and is not a "special" name, then we won't be able to access if,
    // and its quite probabaly a explicit method implementation. Just don't import it. 
    //
    if (!(flags & mdRTSpecialName) && wcschr(methodnameText, L'.') != NULL)
        return;

    name = compiler()->namemgr->AddString(methodnameText, cchMethodnameText - 1);

    // Declare a method.
    sym = compiler()->symmgr.CreateMethod(name, parent);
    ASSERT(sym);
    sym->tokenImport = tokenMethod;

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, tokenMethod);

    // Set attributes of the method.
    SetAccessLevel(sym, flags);

    sym->conditionalSymbols = attributes.conditionalSymbols;

    sym->isDeprecated = attributes.isDeprecated;
    sym->deprecatedMessage = attributes.deprecatedString;
    sym->isDeprecatedError = attributes.isDeprecatedError;
    if (attributes.hasCLSattribute) {
        sym->hasCLSattribute = true;
        sym->isCLS = attributes.isCLS;
    }

    sym->isStatic = (flags & mdStatic) ? true : false;
    sym->isAbstract = (flags & mdAbstract) ? true : false;
    sym->isCtor = (flags & mdRTSpecialName) ? (name == compiler()->namemgr->GetPredefName((flags & mdStatic) ? PN_STATCTOR : PN_CTOR)) : false;
    sym->isVirtual = ((flags & mdVirtual) && !sym->isCtor && !(flags & mdFinal)) ? true : false;
    sym->isMetadataVirtual = (flags & mdVirtual) ? true : false;
    sym->isOperator = isOperator;
    sym->isImplicit = isImplicit;
    sym->isExplicit = isExplicit;
    sym->isHideByName = (flags & mdHideBySig) ? false : true;

    // we are importing an abstract method which
    // is not marked as virtual, currently code generation
    // keys on the isVirtual property to set the dispatch type
    // on function calls.
    //
    // if this fires, find out who generated the metadata
    // and get them to fix their metadata generation
    //
    ASSERT(sym->isVirtual || !sym->isAbstract);
    sym->isVirtual = (sym->isVirtual || sym->isAbstract) ? true : false;

    sym->isVirtual = sym->isVirtual && !sym->isStatic;


    // Set the method signature.
    ImportMethodOrPropSignature(inputfile, tokenMethod, signature, sym);


    //
    // convert special methods into operators.
    // If we don't recognize this as a C#-like operator then we just ignore the special
    // bit and make it a regular method
    //
    if ((flags & mdSpecialName) && !(flags & mdRTSpecialName)) {
		if (name == compiler()->namemgr->GetPredefName(PN_OPEXPLICITMN)) {
            sym->isOperator = true;
            sym->isExplicit = true;
            parent->hasConversion = true;
        }
        else if (name == compiler()->namemgr->GetPredefName(PN_OPIMPLICITMN)) {
            sym->isOperator = false;
            sym->isImplicit = true;
            parent->hasConversion = true;
        }
        else {
            OPERATOR iOp = compiler()->clsDeclRec.operatorOfName(name);
            if (OP_LAST != iOp && OP_EXPLICIT != iOp && OP_IMPLICIT != iOp) {
                 sym->isOperator = true;
            }
        }
    }

    if ((flags & mdVirtual) && !(flags & mdNewSlot) && parent->baseClass) {
        // NOTE: The isOverride bit is NOT valid until
        // AFTER the prepare stage.
        // for now just set to keep track of the !mdNewSlot bit
        // We will fix this up later in CLSDREC::setOverrideBits()
        // during the prepare stage
        sym->isOverride = true;
    }

    if (sym->isCtor && !sym->isStatic && parent->isStruct && !sym->cParams) {
        parent->hasZeroParamConstructor = true;
    }
}

/*
 * Import a propertydef.
 */
void IMPORTER::ImportProperty(PINFILESYM inputfile, PAGGSYM parent, mdProperty tokenProperty, PNAME defaultMemberName)
{
    DWORD scope = parent->getImportScope();
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR propnameText[MAX_IDENT_SIZE];     // name of property
    ULONG cchPropnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    PNAME name;
    PROPSYM *sym;
    mdToken tokenSetter, tokenGetter;
    METHSYM * methSet, * methGet;

    // Get property properties.
    TimerStart(TIME_IMI_GETPROPERTYPROPS);
    CheckHR(metaimport->GetPropertyProps(
		tokenProperty, 					                // The method for which to get props.
		NULL,				                                // Put method's class here.
		propnameText, lengthof(propnameText), &cchPropnameText,         // Method name
		& flags,			                                // Put flags here.
		& signature, & cbSignature,		                        // Method signature
                NULL, NULL, NULL,                                               // Default value
                & tokenSetter, & tokenGetter,                                   // Setter, getter,
                NULL, 0, NULL), inputfile);                                     // Other methods
    TimerStop(TIME_IMI_GETPROPERTYPROPS);

//    BASSERT(lstrcmpW(propnameText, L"ItemByIndex"));

    // Get name.
    if (cchPropnameText <= 1)
        return;
    name = compiler()->namemgr->AddString(propnameText, cchPropnameText - 1);

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, tokenProperty); 

    // Declare a property. Default properties with >0 args are actually indexers.
    methSet = methGet = NULL;
    if (tokenGetter != mdMethodDefNil) {
        methGet = FindMethodDef(inputfile, parent, tokenGetter);
    }
    if (tokenSetter != mdMethodDefNil) {
        methSet = FindMethodDef(inputfile, parent, tokenSetter);
    }
    if (GetSignatureCParams(signature) > 0 && (name == defaultMemberName || 
         (methGet && methGet->name == defaultMemberName) ||
         (methSet && methSet->name == defaultMemberName))) {
        sym = compiler()->symmgr.CreateIndexer(name, parent);
    } else {
        sym = compiler()->symmgr.CreateProperty(name, parent);
    }

    ASSERT(sym);
    sym->tokenImport = tokenProperty;

    // Set the method signature.
    ImportMethodOrPropSignature(inputfile, tokenProperty, signature, sym);

    // handle indexed properties
    if (sym->cParams > 0) {
        if (!sym->isIndexer()) {
            // non-default indexed property
            sym->isBogus = true;
        } else {
            for (int i = 0; i < sym->cParams; i += 1) {
                if (sym->params[i]->kind == SK_PARAMMODSYM) {
                    // INDEXERS can't have ref or out parameters
                    sym->isBogus = true;
                    break;
                }
            }
        }
    }
    else
    {
        if (sym->isIndexer()) {
            sym->isBogus = true;
        }
    }

    // Find the accessor methods.
    if (tokenGetter != mdMethodDefNil) {
        sym->methGet = methGet;

        //
        // check that the accessor is OK and it matches the property signature
        //
        if (!methGet || methGet->isBogus ||
            sym->retType != methGet->retType ||
            sym->cParams != methGet->cParams) {

            sym->isBogus = true;
        } else {
            if (methGet->params != sym->params) {
                // The parameters must be the same.
                // Although it is legal to allow the parameters to differ in ref/out-ness
                // all properties and indexers are bogus if they have ref or out parameters
                // so it doesn't matter
                sym->isBogus = true;
            }
            if (methGet->isParamArray)
                sym->isParamArray = true;
        }
    }
    if (tokenSetter != mdMethodDefNil) {
        sym->methSet = methSet;

        //
        // check that the accessor is OK and it matches the property signature
        //
        if (!methSet || methSet->isBogus || 
            methSet->retType != compiler()->symmgr.GetVoid() || 
            methSet->cParams != (sym->cParams + 1) ||
            sym->retType != methSet->params[sym->cParams]) {

            sym->isBogus = true;
        } else {
            // Can't do "if (methSet->params != sym->params)" because
            // of the set value argument
            if (memcmp(methSet->params, sym->params, sym->cParams * sizeof(sym->params[0]))) {
                // The parameters must be the same.
                // Although it is legal to allow the parameters to differ in ref/out-ness
                // all properties and indexers are bogus if they have ref or out parameters
                // so it doesn't matter
                sym->isBogus = true;
            }
            
            // check for set only indexer with paramarray
            if (methSet->cParams > 1 && methSet->params[methSet->cParams-2]->kind == SK_ARRAYSYM && methSet->params[methSet->cParams-2]->asARRAYSYM()->rank == 1) {
                methSet->isParamArray = sym->isParamArray = !!IsParamArray(inputfile, scope, methSet->tokenImport, methSet->cParams-1);
            }
        }
    }

    // Set attributes of the property by synthesizing them from the accessors.
    // If no accessors, or accessors disagree, it is bogus.
    if (!methSet && !methGet) {
        sym->isBogus = true;
    }
    else if (methGet && !methSet) {
        sym->access = methGet->access;
        sym->isStatic = methGet->isStatic;
        sym->isHideByName = methGet->isHideByName;
    }
    else if (methSet && !methGet) {
        sym->access = methSet->access;
        sym->isStatic = methSet->isStatic;
        sym->isHideByName = methSet->isHideByName;
    }
    else {
        sym->access = methGet->access;
        sym->isStatic = methGet->isStatic;
        sym->isHideByName = methGet->isHideByName;
        if (methGet->access != methSet->access || methGet->isStatic != methSet->isStatic || methGet->isHideByName != methSet->isHideByName || methSet->isParamArray != methGet->isParamArray)
            sym->isBogus = true;
    }

    //
    // only flag imported methods as accessors if the property is accesible
    //
    if (!sym->isBogus) {
        if (sym->methGet)
            sym->methGet->isPropertyAccessor = true;
        if (sym->methSet)
            sym->methSet->isPropertyAccessor = true;
        ASSERT (!sym->useMethInstead);
    } else {
        sym->useMethInstead = (sym->methSet && !sym->methSet->isBogus) || (sym->methGet && !sym->methGet->isBogus);
    }

    sym->isDeprecated = attributes.isDeprecated;
    sym->deprecatedMessage = attributes.deprecatedString;
    sym->isDeprecatedError = attributes.isDeprecatedError;
    if (attributes.hasCLSattribute) {
        sym->hasCLSattribute = true;
        sym->isCLS = attributes.isCLS;
        if (methGet) {
            methGet->hasCLSattribute = true;
            methGet->isCLS = attributes.isCLS;
        }
        if (methSet) {
            methSet->hasCLSattribute = true;
            methSet->isCLS = attributes.isCLS;
        }
    }

    if (sym->isDeprecated) {
        if (methGet) {
            methGet->isDeprecated = true;
            methGet->isDeprecatedError = sym->isDeprecatedError;
            methGet->deprecatedMessage = sym->deprecatedMessage;
        }
        if (methSet) {
            methSet->isDeprecated = true;
            methSet->isDeprecatedError = sym->isDeprecatedError;
            methSet->deprecatedMessage = sym->deprecatedMessage;
        }
    }
}


/*
 * Import an eventdef.
 */
void IMPORTER::ImportEvent(PINFILESYM inputfile, PAGGSYM parent, mdEvent tokenEvent)
{
    DWORD scope = parent->getImportScope();
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR eventnameText[MAX_IDENT_SIZE];     // name of event
    ULONG cchEventnameText;
    PNAME name;
    EVENTSYM * event;
    DWORD flags;
    mdToken tokenEventType, tokenAdd, tokenRemove;
    PMETHSYM methAdd, methRemove;

    // Get Event propertys
    TimerStart(TIME_IMI_GETEVENTPROPS);
    CheckHR(metaimport->GetEventProps(
        tokenEvent,                         // [IN] event token 
        NULL,                               // [OUT] typedef containing the event declarion.    
        eventnameText,                      // [OUT] Event name 
        lengthof(eventnameText),            // [IN] the count of wchar of szEvent   
        &cchEventnameText,                  // [OUT] actual count of wchar for event's name 
        & flags,                            // [OUT] Event flags.   
        & tokenEventType,                   // [OUT] EventType class    
        & tokenAdd,                         // [OUT] AddOn method of the event  
        & tokenRemove,                      // [OUT] RemoveOn method of the event   
        NULL,                               // [OUT] Fire method of the event   
        NULL,                               // [OUT] other method of the event  
        0,                                  // [IN] size of rmdOtherMethod  
        NULL), inputfile);                  // [OUT] total number of other method of this event 
    TimerStop(TIME_IMI_GETEVENTPROPS);

    // Get name.
    if (cchEventnameText <= 1)
        return;
    name = compiler()->namemgr->AddString(eventnameText, cchEventnameText - 1);

    // Declare an event symbol.
    event = compiler()->symmgr.CreateEvent(name, parent);
    event->tokenImport = tokenEvent;

    // Get the event type.
    event->type = ResolveTypeRef(inputfile, scope, tokenEventType, false);

    // Find the accessor methods. They must be present, and have a signature of void XXX(EventType handler);
    methAdd = methRemove = NULL;
    if (tokenAdd != mdMethodDefNil) {
        event->methAdd = methAdd = FindMethodDef(inputfile, parent, tokenAdd);   
        if (methAdd->cParams != 1 || methAdd->params[0] != event->type || methAdd->retType != compiler()->symmgr.GetVoid())
            event->isBogus = true;
    }
    if (tokenRemove != mdMethodDefNil) {
        event->methRemove = methRemove = FindMethodDef(inputfile, parent, tokenRemove);
        if (methRemove->cParams != 1 || methRemove->params[0] != event->type || methRemove->retType != compiler()->symmgr.GetVoid())
            event->isBogus = true;
    }

    // Set attributes of the event by synthesizing from the accessors. If accessors disaggree, it is bogus.
    if (methAdd && methRemove) {
        event->access = methAdd->access;
        event->isStatic = methAdd->isStatic;
        if (methRemove->access != methAdd->access || methRemove->isStatic != methAdd->isStatic)
            event->isBogus = true;
    }
    else
        event->isBogus = true;  // must have both accessors.
    
    // If the event is OK, flags the accessors.
    if (!event->isBogus) {
        event->methAdd->isEventAccessor = true;
        event->methRemove->isEventAccessor = true;
        ASSERT(!event->useMethInstead);
    } else {
        event->useMethInstead = ((event->methAdd && !event->methAdd->isBogus) || (event->methRemove && !event->methRemove->isBogus));
    }

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, tokenEvent); 

    event->isDeprecated = attributes.isDeprecated;
    event->deprecatedMessage = attributes.deprecatedString;
    event->isDeprecatedError = attributes.isDeprecatedError;
    if (attributes.hasCLSattribute) {
        event->hasCLSattribute = true;
        event->isCLS = attributes.isCLS;
    }

    if (event->isDeprecated) {
        if (methAdd) {
            methAdd->isDeprecatedError = event->isDeprecatedError;
            methAdd->deprecatedMessage = event->deprecatedMessage;
        }
        if (methRemove) {
            methRemove->isDeprecatedError = event->isDeprecatedError;
            methRemove->deprecatedMessage = event->deprecatedMessage;
        }
    }
}

/*
 * Given a methoddef token and a parent, find the corresponding symbol.
 * There are two possible strategies here. One, do a linear search over
 * all methods in the parent and march on the token. Two, get the
 * name of the method def and then to a regular lookup. We chose the
 * second, which should be faster, unless the metadata call to get
 * the name is really slow.
 */
PMETHSYM IMPORTER::FindMethodDef(PINFILESYM inputfile, PAGGSYM parent, mdMethodDef token)
{
    DWORD scope = parent->getImportScope();
    ASSERT(inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR methodnameText[MAX_IDENT_SIZE];     // name of method
    ULONG cchMethodnameText;
    PNAME name;
    PMETHSYM sym;

    // Get method properties.
    TimerStart(TIME_IMI_GETMETHODPROPS);
    CheckHR(metaimport->GetMethodProps(
		token, 					                        // The method for which to get props.
		NULL,				                                // Put method's class here.
		methodnameText,	lengthof(methodnameText), &cchMethodnameText,   // Method name
		NULL,			                                        // Put flags here.
		NULL, NULL,		                                        // Method signature
		NULL,			                                        // codeRVA
		NULL), inputfile);                                              // Impl. Flags
    TimerStop(TIME_IMI_GETMETHODPROPS);

    // Convert name to a NAME.
    if (cchMethodnameText <= 1)
        return NULL;
    name = compiler()->namemgr->LookupString(methodnameText, cchMethodnameText - 1);
    if (!name)
        return NULL;

    // Search the parent for methods with this name until one with a matching
    // token is found.
    sym = compiler()->symmgr.LookupGlobalSym(name, parent, MASK_METHSYM)->asMETHSYM();
    while (sym) {
        // Found the correct one?
        if (sym->tokenImport == token)
            return sym;

        // Got to the next one with the same name.
        sym = compiler()->symmgr.LookupNextSym(sym, parent, MASK_METHSYM)->asMETHSYM();
    }

    // No method with this name and token in the parent.
    return NULL;
}

/*
 *Given type flags, get the access level. The sym is passed in just to make a determination on
 * what assembly it is in; it is not changed.
 */
ACCESS IMPORTER::AccessFromTypeFlags(unsigned flags, const AGGSYM * sym)
{
    switch (flags & tdVisibilityMask)
    {
    case tdNotPublic:               return ACC_INTERNAL;
    case tdPublic:                  return ACC_PUBLIC;
    case tdNestedPublic:            return ACC_PUBLIC;
    case tdNestedPrivate:           return ACC_PRIVATE;
    case tdNestedFamily:            return ACC_PROTECTED;
    case tdNestedAssembly:          return ACC_INTERNAL;
    case tdNestedFamANDAssem:       return sym->getAssemblyIndex() == 0 ? ACC_PROTECTED : ACC_INTERNAL;  // We don't support this directly, treat as protected in our own assembly, internal if in another assembly.
    case tdNestedFamORAssem:        return ACC_INTERNALPROTECTED; 

    default:                        ASSERT(0); return ACC_PRIVATE;
    }
}

void IMPORTER::GetBaseTokenAndFlags(AGGSYM *sym, AGGSYM  **base, DWORD *flags)
{
    SETLOCATIONSTAGE(IMPORT);
    SETLOCATIONSYM(sym);

    IMetaDataImport * metaimport;
    PINFILESYM inputfile;
    mdToken baseToken;

    // Get the meta-data import interface.
    inputfile = sym->getInputFile();
    metaimport = inputfile->metaimport[sym->getImportScope()];
    ASSERT(metaimport);

    TimerStart(TIME_IMI_GETTYPEDEFPROPS);
    CheckHR(metaimport->GetTypeDefProps(sym->tokenImport,
            NULL, 0, NULL,            // Type name
            flags,                    // Flags
            &baseToken                // Extends
           ), inputfile);
    TimerStop(TIME_IMI_GETTYPEDEFPROPS);

    if (baseToken != mdTypeRefNil && baseToken != mdTypeDefNil)
        *base = ResolveBaseRef(sym->getInputFile(), sym->getImportScope(), baseToken, sym);
    else
        *base = NULL;
}

bool IMPORTER::HasIfaceListChanged(AGGSYM *sym)
{
    IMetaDataImport * metaimport;
    PINFILESYM inputfile;
    HCORENUM corenum;           // For enumerating tokens.
    mdToken tokens[32];
    ULONG cTokens, iToken;

    // Get the meta-data import interface.
    inputfile = ((AGGSYM*)sym)->getInputFile();
    metaimport = inputfile->metaimport[sym->getImportScope()];
    ASSERT(metaimport);

    // Import interfaces.
    PSYMLIST ifaceList = sym->allIfaceList;
    corenum = 0;
    do {
        // Get next batch of interfaces.
        TimerStart(TIME_IMI_ENUMINTERFACEIMPLS);
        CheckHR(metaimport->EnumInterfaceImpls(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMINTERFACEIMPLS);

        // Process each interface.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            if (!ifaceList) return true;

            mdToken tokenInterface;

            // Get typeref and flags.
            TimerStart(TIME_IMI_GETINTERFACEIMPLPROPS);
            CheckHR(metaimport->GetInterfaceImplProps(tokens[iToken], NULL, &tokenInterface), inputfile);
            TimerStop(TIME_IMI_GETINTERFACEIMPLPROPS);

            if (ifaceList->sym->asAGGSYM()->tokenImport != tokenInterface  &&
                ifaceList->sym->asAGGSYM() != ResolveTypeRef(inputfile, sym->getImportScope(), tokenInterface, true)) return true;

            ifaceList = ifaceList->next;
        }
    } while (cTokens > 0);

    metaimport->CloseEnum(corenum);

    if (ifaceList) return true;

    return false;
}

/*
 * resolves the inheritance hierarchy (and interfaces) for a type
 * returns false if a change in the inheritance hierarchy was detected in the incremental scenario
 */
bool IMPORTER::ResolveInheritance(AGGSYM *sym)
{
    SETLOCATIONSTAGE(IMPORT);
    SETLOCATIONSYM(sym);

    IMetaDataImport * metaimport;
    PINFILESYM inputfile;
    DWORD flags, scope;
    AGGSYM *base = NULL;

    HCORENUM corenum;           // For enumerating tokens.
    mdToken tokens[32];
    ULONG cTokens, iToken;

    ASSERT(!sym->hasResolvedBaseClasses);
    sym->isResolvingBaseClasses = true;

    // Get the meta-data import interface.
    inputfile = sym->getInputFile();
    scope = sym->getImportScope();
    metaimport = inputfile->metaimport[scope];
    ASSERT(metaimport);


    // Import base class. Set fundemental type.
    sym->baseClass = NULL;

    GetBaseTokenAndFlags(sym, &base, &flags);

    // Set attributes on the symbol structure from the flags.
    sym->access = AccessFromTypeFlags(flags, sym);

    // Note: structs and enums are checked below.

    if (IsTdInterface(flags)) {
        // interface
        sym->isInterface = true;
        sym->isAbstract = true;
    }
    else {
        // regular class
        sym->isClass = true;
        sym->isAbstract = (flags & tdAbstract) ? true : false;
        sym->isSealed = (flags & tdSealed) ? true : false;
    }

    if (sym->isClass || sym->isStruct) {
        if (base) {
            // this is for incremental rebuild
            if (!compiler()->clsDeclRec.resolveInheritanceRec(base))
            {
                sym->isResolvingBaseClasses = false;
                return false;
            }
            sym->baseClass = base;
        }
        else {
            // Ensure that the base class of everything is object, except object itself.

            SYM * object = compiler()->symmgr.GetPredefType(PT_OBJECT, false);
            if (sym != object)
                sym->baseClass = object->asAGGSYM();

        }
    }

    // we now consider the base class resolved
    sym->isResolvingBaseClasses = false;
    sym->hasResolvedBaseClasses = true;

    // If the base class is "Enum", then it is an enum.
    if (sym->baseClass->isPredefType(PT_ENUM)) {
        sym->isEnum = true;
        sym->isStruct = false;
        sym->isClass = false;

        // need to pick up the underlying type here
        // for incremental build
        HCORENUM corenum = 0;
        mdToken tokens[32];
        ULONG cTokens, iToken;
        do {
            // Get next batch of fields.
            TimerStart(TIME_IMI_ENUMFIELDS);
            CheckHR(metaimport->EnumFields(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
            TimerStop(TIME_IMI_ENUMFIELDS);

            // Process each fields.
            for (iToken = 0; iToken < cTokens; ++iToken) {
                DWORD flags;
                PCCOR_SIGNATURE signature;
                ULONG cbSignature;

                // Get properties of the field from metadata.
                TimerStart(TIME_IMI_GETFIELDPROPS);
                CheckHR(metaimport->GetFieldProps(
		            tokens[iToken],		                                    // The field for which to get props.
		            NULL,		                                            // Put field's class here.
		            NULL, NULL, NULL,                                       // Field name
		            & flags,		                                        // Field flags
		            & signature, & cbSignature,                             // Field signature
		            NULL, NULL, NULL),                  // Field constant value
                            inputfile);
                TimerStop(TIME_IMI_GETFIELDPROPS);

                // Enums are a bit special. Non-static fields serve only to record the
                // underlying integral type, and are otherwise ignored. Static fields are
                // enumerators and must be of the enum type. (We change other integral ones to the
                // enum type because it's probably what the emitting compiler meant.)
                if (!(flags & fdStatic)) {

                    // Import the type of the field.
                    PTYPESYM type;
                    bool dummy;
                    type = ImportFieldType(inputfile, scope, signature, &dummy);

                    // Assuming its an integral type, use it to set the
                    // enum base type.
                    if (type->isNumericType())
                    {
                        compiler()->clsDeclRec.resolveInheritanceRec(type->asAGGSYM());
                        if (type->fundType() <= FT_LASTINTEGRAL)
                            sym->underlyingType = type->asAGGSYM();
                    }
                    break;
                }
            }
        } while (cTokens > 0);
        metaimport->CloseEnum(corenum);

        if (!sym->underlyingType)
        {
            // couldn't find an underlying type
            // treat it as a struct
            sym->isStruct = true;
            sym->isSealed = true;
            sym->isClass = false;
        }
    }
    else if (sym->baseClass->isPredefType(PT_VALUE) && ! sym->isPredefType(PT_ENUM)) {
        sym->isStruct = true;
        sym->isSealed = true;
        sym->isClass = false;
    }

    // Set inherited bits.
    if (sym->baseClass) {
        AGGSYM * baseClass = sym->baseClass;
        if (baseClass->isAttribute)
            sym->isAttribute = true;
        if (baseClass->isSecurityAttribute)
            sym->isSecurityAttribute = true;
        if (baseClass->isMarshalByRef)
            sym->isMarshalByRef = true;
    }

    // Import interfaces.
    sym->ifaceList = 0;                     // Start with empty interface list.
    PSYMLIST * pLink = & sym->ifaceList;

    corenum = 0;
    do {
        // Get next batch of interfaces.
        TimerStart(TIME_IMI_ENUMINTERFACEIMPLS);
        CheckHR(metaimport->EnumInterfaceImpls(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMINTERFACEIMPLS);

        // Process each interface.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            // Get the interface.
            AGGSYM* symIface = ImportInterface(inputfile, scope, tokens[iToken], sym);

            // Add to the interface list if interesting.
            if (symIface) {
                // this is for incremental rebuild
                if (!compiler()->clsDeclRec.resolveInheritanceRec(symIface))
                {
                    sym->isResolvingBaseClasses = false;
                    sym->ifaceList = NULL;
                    sym->baseClass = NULL;
                    return false;
                }
                compiler()->symmgr.AddToGlobalSymList(symIface, & pLink);
            }
        }
    } while (cTokens > 0);

    metaimport->CloseEnum(corenum);

    compiler()->clsDeclRec.combineInterfaceLists(sym);

    return true;
}

/*
 * Given a type that previous imported via ImportAllTypes, make it
 * "declared" by importing all of its members and declaring
 * its base classes/interfaces.
 */
void IMPORTER::DefineImportedType(AGGSYM * sym)
{
    SETLOCATIONSTAGE(IMPORT);
    SETLOCATIONSYM(sym);

#ifdef DEBUG
    compiler()->haveDefinedAnyType = true;
#endif

    IMetaDataImport * metaimport;
    INFILESYM *inputfile;
    DWORD scope;

    HCORENUM corenum;           // For enumerating tokens.
    mdToken tokens[32];
    ULONG cTokens, iToken;

    //BASSERT(lstrcmpW(L"QuickSort", sym->name->text));

    ASSERT(! sym->isDefined);

    // Get the meta-data import interface.
    scope = sym->getImportScope();
    inputfile = sym->getInputFile();
    metaimport = inputfile->metaimport[scope];
    ASSERT(metaimport);

    ASSERT(!sym->isResolvingBaseClasses);
    if (!sym->hasResolvedBaseClasses)
    {
        ResolveInheritance(sym);
    }
    ASSERT(sym->hasResolvedBaseClasses && !sym->isResolvingBaseClasses);

    // At this point, the class should be considered declared.
    sym->isDefined = true;

    // Unmanaged value types are handled at all -- don't import anything about them.
    if (sym->isBogus)
        return;

    //
    // Check to see if the class has any explicit interface impls
    //
    if (sym->isStruct) {
        corenum = 0;
        mdToken tokensBody[1];
        mdToken tokensDecl[1];
        // Get next batch of fields.
        TimerStart(TIME_IMI_ENUMMETHODIMPLS);
        CheckHR(metaimport->EnumMethodImpls(&corenum, sym->tokenImport, tokensBody, tokensDecl, lengthof(tokensBody), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMMETHODIMPLS);

        if (cTokens) {
            sym->hasExplicitImpl = true;
        }
        metaimport->CloseEnum(corenum);
    }

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, sym->tokenImport); 

    sym->isBogus |= attributes.hasRequiredAttribute;
    if (! sym->isInterface) {
        corenum = 0;
        do {
            // Get next batch of fields.
            TimerStart(TIME_IMI_ENUMFIELDS);
            CheckHR(metaimport->EnumFields(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
            TimerStop(TIME_IMI_ENUMFIELDS);

            // Process each fields.
            for (iToken = 0; iToken < cTokens; ++iToken) {
                ImportField(inputfile, sym, tokens[iToken]);
            }
        } while (cTokens > 0);
        metaimport->CloseEnum(corenum);
    } else {
        if (sym->isComImport && attributes.CoClassName) {
            ASSERT(sym->underlyingType == NULL);
            sym->comImportCoClass = attributes.CoClassName;
        }
    }

    //ASSERT(lstrcmpW(L"DateTime", sym->name->text));

    // Import methods. Enums don't have them.
    if (! sym->isEnum) {
        corenum = 0;
        do {
            // Get next batch of methods.
            TimerStart(TIME_IMI_ENUMMETHODS);
            CheckHR(metaimport->EnumMethods(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
            TimerStop(TIME_IMI_ENUMMETHODS);

            // Process each method.
            for (iToken = 0; iToken < cTokens; ++iToken) {
                ImportMethod(inputfile, sym, tokens[iToken]);
            }
        } while (cTokens > 0);
        metaimport->CloseEnum(corenum);

        // get method impls
        corenum = 0;

        mdToken tokensDecl[32];
        do {
            // get next batch of methodimpls
            TimerStart(TIME_IMI_ENUMMETHODIMPLS);
            CheckHR(metaimport->EnumMethodImpls(&corenum, sym->tokenImport, tokens, tokensDecl, lengthof(tokens), &cTokens), inputfile);
            TimerStop(TIME_IMI_ENUMMETHODIMPLS);

            for (iToken = 0; iToken < cTokens; iToken += 1) {
                // find the type token of the impl's decl
                mdToken tokenClassOfImpl;
                if (TypeFromToken(tokensDecl[iToken]) == mdtMethodDef)
                {
                    // Get method properties.
                    TimerStart(TIME_IMI_GETMETHODPROPS);
                    CheckHR(metaimport->GetMethodProps(
		                tokensDecl[iToken],		                        // The method for which to get props.
		                &tokenClassOfImpl,                              // Put method's class here.
		                0,	0, 0,                                       // Method name
		                NULL,			                                        // Put flags here.
		                NULL, NULL,		                                        // Method signature
		                NULL,			                                        // codeRVA
		                NULL), inputfile);                                              // Impl. Flags
                    TimerStop(TIME_IMI_GETMETHODPROPS);
                } else {
                    ASSERT(TypeFromToken(tokensDecl[iToken]) == mdtMemberRef);
                    TimerStart(TIME_IMI_GETMEMBERREFPROPS);
                    CheckHR(metaimport->GetMemberRefProps(tokensDecl[iToken], &tokenClassOfImpl, 0, 0, 0, 0, 0), inputfile);
                    TimerStop(TIME_IMI_GETMEMBERREFPROPS);
                }

                // a method impl to a class type means the method must be an override
                AGGSYM *implClass = ResolveTypeRef(inputfile, scope, tokenClassOfImpl, true);
                if (implClass && implClass->isClass)
                {
                    METHSYM *method = FindMethodDef(inputfile, sym, tokens[iToken]);
                    if (method->isVirtual)
                        method->isOverride = true;
                }
            }
        } while (cTokens > 0);
        metaimport->CloseEnum(corenum);
    }

    if (! sym->isEnum) {
        // Import properties. These must be done after methods, because properties refer to methods.
        PNAME defaultMemberName = NULL;

        if (attributes.defaultMember)
            defaultMemberName = compiler()->namemgr->AddString(attributes.defaultMember);

        corenum = 0;
        do {
            // Get next batch of properties.
            TimerStart(TIME_IMI_ENUMPROPERTIES);
            CheckHR(metaimport->EnumProperties(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
            TimerStop(TIME_IMI_ENUMPROPERTIES);

            // Process each property.
            for (iToken = 0; iToken < cTokens; ++iToken) {
                ImportProperty(inputfile, sym, tokens[iToken], defaultMemberName);
            }
        } while (cTokens > 0);
        metaimport->CloseEnum(corenum);

        // Import events. These must be done after methods, because events refer to methods.
        corenum = 0;
        do {
            // Get next batch of events.
            TimerStart(TIME_IMI_ENUMEVENTS);
            CheckHR(metaimport->EnumEvents(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
            TimerStop(TIME_IMI_ENUMEVENTS);

            // Process each event.
            for (iToken = 0; iToken < cTokens; ++iToken) {
                ImportEvent(inputfile, sym, tokens[iToken]);
            }
        } while (cTokens > 0);
        metaimport->CloseEnum(corenum);
    }

    if (sym->baseClass && !sym->isAbstract &&
        ((sym->baseClass->isPredefType(PT_DELEGATE)) ||
         (sym->baseClass->isPredefType(PT_MULTIDEL)))) {

        // we have something which looks kinda like a delegate
        // check if it really is a C# delegate
        // or is just a class derived from delegate
        bool foundInvoke = false;
        bool foundConstructor = false;
        FOREACHCHILD(sym, child)
            switch(child->kind) {
            case SK_METHSYM:
            {
                // check for constructor or invoke
                METHSYM *method = child->asMETHSYM();
                if (method->name == compiler()->namemgr->GetPredefName(PN_INVOKE)) {
                    if (!foundInvoke && (method->access == ACC_PUBLIC)) {
                        foundInvoke = true;
                        if (method->isBogus)  sym->isBogus = true;
                        continue;
                    }
                } else if (method->isCtor) {
                    if (!foundConstructor &&
                        method->access == ACC_PUBLIC &&
                        method->cParams == 2 &&
                        method->params[0] == compiler()->symmgr.GetObject() &&
                        (method->params[1]->isPredefType(PT_UINTPTR) ||
                        method->params[1]->isPredefType(PT_INTPTR))) 
                    {
                        foundConstructor = true;
                        if (method->isBogus)  sym->isBogus = true;
                        continue;
                    }
                }

                // found a non-delegatelike method
                break;
            }

            // ignore compound types
            case SK_PTRSYM:
            case SK_ARRAYSYM:
            case SK_PARAMMODSYM:
                break;

            // other members are ignored
            default:
                break;
            }
        ENDFOREACHCHILD

        sym->isDelegate = foundInvoke && foundConstructor;
    }

    if (sym->baseClass) {
        sym->hasConversion |= sym->baseClass->hasConversion;
    }

    if (sym->isEnum && sym->underlyingType == NULL) {
        // enum didn't have a valid underlying type that we like.
        // treat it like a struct instead of an enum.
        sym->isEnum = false;
        sym->isStruct = true;
    }

    sym->isDeprecated = attributes.isDeprecated;
    sym->deprecatedMessage = attributes.deprecatedString;
    sym->isDeprecatedError = attributes.isDeprecatedError;
    if (attributes.hasCLSattribute) {
        sym->hasCLSattribute = true;
        sym->isCLS = attributes.isCLS;
    }

    // don't overwrite default attributeClass for security attributes
    if (attributes.attributeKind != 0) {
        sym->attributeClass = attributes.attributeKind;
        sym->isMultipleAttribute = attributes.allowMultiple;
    }
    else if (sym->isAttribute)
    {
        sym->attributeClass = sym->baseClass->attributeClass;
        sym->isMultipleAttribute = sym->baseClass->isMultipleAttribute;
    }

    if (inputfile->isSource)
    {
        ASSERT(compiler()->IsIncrementalRebuild() && compiler()->HaveTypesBeenDefined());

        FOREACHCHILD(sym, child)
            SETLOCATIONSYM(child);
            switch (child->kind)
            {
            case SK_MEMBVARSYM:
                ASSERT (!child->asMEMBVARSYM()->tokenEmit);
                child->asMEMBVARSYM()->tokenEmit = child->asMEMBVARSYM()->tokenImport;
                ASSERT(child->asMEMBVARSYM()->tokenEmit);
                break;
            case SK_METHSYM:
                ASSERT (!child->asMETHSYM()->tokenEmit);
                child->asMETHSYM()->tokenEmit = child->asMETHSYM()->tokenImport;
                ASSERT(child->asMETHSYM()->tokenEmit);
                break;
            case SK_PROPSYM:
                ASSERT(!child->asPROPSYM()->tokenEmit);
                child->asPROPSYM()->tokenEmit = child->asPROPSYM()->tokenImport;
                ASSERT(child->asPROPSYM()->tokenEmit);
                break;
            case SK_EVENTSYM:
                ASSERT(!child->asEVENTSYM()->tokenEmit);
                child->asEVENTSYM()->tokenEmit = child->asEVENTSYM()->tokenImport;
                ASSERT(child->asEVENTSYM()->tokenEmit);
                break;
            default:
                break;
            }

        ENDFOREACHCHILD;

        sym->isMemberDefsEmitted = true;
    }
}

/*
 * We know that the type hasn't changed, but we've already declared all of its members
 * Match the members up with their metadata tokens.
 *                                       
 */
void IMPORTER::ReadExistingMemberTokens(AGGSYM *sym)
{

    IMetaDataImport * metaimport;
    PINFILESYM inputfile;

    HCORENUM corenum;           // For enumerating tokens.
    mdToken tokens[32];
    ULONG cTokens, iToken;

    ASSERT(sym->isDefined);

    // Get the meta-data import interface.
    inputfile = sym->getInputFile();
    metaimport = inputfile->metaimport[sym->getImportScope()];
    ASSERT(metaimport);

    // update the type metadata
    compiler()->emitter.ReemitAggregateFlags(sym);
    compiler()->emitter.DeleteRelatedTypeTokens(inputfile, sym->tokenEmit);

    // Import fields. Interfaces don't have them.
    corenum = 0;
    do {
        // Get next batch of fields.
        TimerStart(TIME_IMI_ENUMFIELDS);
        CheckHR(metaimport->EnumFields(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMFIELDS);

        // Process each fields.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            MEMBVARSYM *field = ReadExistingField(inputfile, sym, tokens[iToken]);
            if (!field) {
                compiler()->emitter.DeleteToken(tokens[iToken]);
            } else if (field != MAGIC_SYMBOL_VALUE) {
                compiler()->emitter.ReemitMembVar(field);
            } else {
                // a magic enum member just leave the token
            }
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    // Import methods.
    corenum = 0;
    do {
        // Get next batch of methods.
        TimerStart(TIME_IMI_ENUMMETHODS);
        CheckHR(metaimport->EnumMethods(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMMETHODS);

        // Process each method.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            METHSYM *method = ReadExistingMethod(inputfile, sym, tokens[iToken]);
            if (!method) {
                compiler()->emitter.DeleteToken(tokens[iToken]);
            } else {
                compiler()->emitter.ReemitMethod(method);
            }
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    // Emit the children.
    FOREACHCHILD(sym, child)

        SETLOCATIONSYM(child);

        switch (child->kind) {
        case SK_MEMBVARSYM:
            if (!child->asMEMBVARSYM()->tokenEmit) {
                compiler()->emitter.EmitMembVarDef(child->asMEMBVARSYM());
            }
            ASSERT(child->asMEMBVARSYM()->tokenEmit);
            break;
        case SK_METHSYM:
            if (!child->asMETHSYM()->tokenEmit) {
                compiler()->emitter.EmitMethodDef(child->asMETHSYM());
            }
            ASSERT(child->asMETHSYM()->tokenEmit);
            break;
	default:
	    break;
        }
    ENDFOREACHCHILD

    // Import properties. These must be done after methods, because properties refer to methods.
    corenum = 0;
    do {
        // Get next batch of properties.
        TimerStart(TIME_IMI_ENUMPROPERTIES);
        CheckHR(metaimport->EnumProperties(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMPROPERTIES);

        // Process each property.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            PROPSYM *property = ReadExistingProperty(inputfile, sym, tokens[iToken]);
            if (!property) {
                compiler()->emitter.DeleteToken(tokens[iToken]);
            } else {
                compiler()->emitter.ReemitProperty(property);
            }
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    // Import events. These must be done after methods, because events refer to methods.
    corenum = 0;
    do {
        // Get next batch of events.
        TimerStart(TIME_IMI_ENUMEVENTS);
        CheckHR(metaimport->EnumEvents(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMEVENTS);

        // Process each event.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            EVENTSYM *event = ReadExistingEvent(inputfile, sym, tokens[iToken]);
            if (!event) {
                compiler()->emitter.DeleteToken(tokens[iToken]);
            } else {
                compiler()->emitter.ReemitEvent(event);
            }
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    // Properties/events must be done after methods.
    FOREACHCHILD(sym, child)
        if (child->kind == SK_PROPSYM) {

            SETLOCATIONSYM(child);

            if (!child->asPROPSYM()->tokenEmit) {
                compiler()->emitter.EmitPropertyDef(child->asPROPSYM());
            }
            ASSERT(child->asPROPSYM()->tokenEmit || child->asPROPSYM()->explicitImpl);
        }
        else if (child->kind == SK_EVENTSYM) {

            SETLOCATIONSYM(child);

            if (!child->asEVENTSYM()->tokenEmit) {
                compiler()->emitter.EmitEventDef(child->asEVENTSYM());
            }
            ASSERT(child->asEVENTSYM()->explicitImpl || child->asEVENTSYM()->tokenEmit);
        }
    ENDFOREACHCHILD
}

/*
 * Match the fieldToken with a SYM which must already exist in the given cls.
 */
MEMBVARSYM * IMPORTER::ReadExistingField(PINFILESYM inputfile, AGGSYM *cls, mdToken tokenField)
{
    DWORD scope = cls->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);

    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR fieldnameText[MAX_IDENT_SIZE];     // name of field
    ULONG cchFieldnameText;
    PNAME name;
    MEMBVARSYM * sym;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    bool isVolatile;
    DWORD flags;

    // Get properties of the field from metadata.
    TimerStart(TIME_IMI_GETFIELDPROPS);
    CheckHR(metaimport->GetFieldProps(
		tokenField, 	                                            // The field for which to get props.
		NULL,		                                                // Put field's class here.
		fieldnameText, lengthof(fieldnameText), & cchFieldnameText, // Field name
		&flags,                                                     // Field flags
		& signature, & cbSignature,                                 // Field signature
		NULL, NULL, NULL), inputfile);                              // Field constant value
    TimerStop(TIME_IMI_GETFIELDPROPS);

    // Get name.
    if (cchFieldnameText <= 1)
        return NULL;
    name = compiler()->namemgr->AddString(fieldnameText, cchFieldnameText - 1);

    // handle "special" enum meber
    if (cls->isEnum && name == compiler()->namemgr->GetPredefName(PN_ENUMVALUE)) {
        return (MEMBVARSYM*) MAGIC_SYMBOL_VALUE;
    }

    //
    // find the field by name
    //
    sym = compiler()->symmgr.LookupGlobalSym(name, cls, MASK_MEMBVARSYM)->asMEMBVARSYM();
    if (!sym) return NULL;

    // Import the type of the field.
    if (sym->type != ImportFieldType(inputfile, scope, signature, &isVolatile)) return NULL;
    if (!!sym->isVolatile != !!isVolatile) return NULL;

    // static must match because we can't reset the siganture
    // and the signature includes staticness
    if (!!sym->isStatic != !!(flags & fdStatic)) return NULL;

    //
    // set the fields metadata tokens
    //
    sym->tokenEmit = tokenField;

    return sym;
}

/*
 * Match the methodToken with a SYM which must already exist in the given cls.
 */
METHSYM * IMPORTER::ReadExistingMethod(PINFILESYM inputfile, AGGSYM *cls, mdToken methodToken)
{
    DWORD scope = cls->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);

    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR methodnameText[MAX_IDENT_SIZE];     // name of method
    ULONG cchMethodnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    PNAME name;
    METHSYM * sym = NULL;

    bool isBogus = false;
    bool isVarargs = false;
    bool isParams = false;
    TYPESYM *retType = NULL;
    TYPESYM **paramTypes = NULL;
    unsigned short cParams = 0;

    // Get method properties.
    TimerStart(TIME_IMI_GETMETHODPROPS);
    CheckHR(metaimport->GetMethodProps(
		methodToken, 					                // The method for which to get props.
		NULL,				                                // Put method's class here.
		methodnameText,	lengthof(methodnameText), &cchMethodnameText,   // Method name
		& flags,			                                // Put flags here.
		& signature, & cbSignature,		                        // Method signature
		NULL,			                                        // codeRVA
		NULL), inputfile);                                              // Impl. Flags
    TimerStop(TIME_IMI_GETMETHODPROPS);

    if (cchMethodnameText <= 1)
        return NULL;
        
    // Set the method signature.
    ImportSignature(inputfile, scope, methodToken, signature, true, !!(flags & mdStatic),
                    &retType, &cParams, &paramTypes, &isBogus, &isVarargs, &isParams, NULL);
    if (isBogus) return NULL;

    // If the name contains a '.', and is not a "special" name, then we won't be able to access if,
    // and its quite probabaly a explicit method implementation. Just don't import it. 
    //
    if (!(flags & mdRTSpecialName) && wcspbrk(methodnameText, L".$") != NULL)
    {
        //
        // find explicit impl
        //
        PARENTSYM *parent = compiler()->symmgr.GetRootNS();
        WCHAR *start = methodnameText;
        WCHAR *end;
        while (NULL != (end = wcspbrk(start, L".$"))) {
            *end = 0;
            parent = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->AddString(start), parent, MASK_ALL)->asPARENTSYM();
            start = end + 1;
        }
        name = compiler()->namemgr->AddString(start);
        sym = compiler()->symmgr.LookupGlobalSym(NULL, cls, MASK_METHSYM)->asMETHSYM();
        while (sym) {
            METHSYM *explicitImpl = sym->explicitImpl->asMETHSYM();
            if (explicitImpl->name == name &&
                explicitImpl->getClass() == parent && 
                explicitImpl->params == paramTypes &&
                explicitImpl->retType == retType &&
                !!explicitImpl->isVarargs == !!isVarargs &&
                (explicitImpl->isPropertyAccessor || !!explicitImpl->isParamArray == !!isParams))
            {
                sym->tokenEmit = methodToken;
                return sym;
            }
            sym = compiler()->symmgr.LookupNextSym(sym, cls, MASK_METHSYM)->asMETHSYM();
        }

        return NULL;
    }

    name = compiler()->namemgr->AddString(methodnameText, cchMethodnameText - 1);

    //
    // find method, and check signature
    //
    SYM *searchSym = NULL;
    do
    {
        searchSym = compiler()->clsDeclRec.findNextName(name, (AGGSYM*)cls, searchSym);
        if (!searchSym || searchSym->parent != (AGGSYM*)cls || searchSym->kind != SK_METHSYM) {
            return NULL;
        }
        sym = searchSym->asMETHSYM();
    } while ((sym->params           != paramTypes) || 
             (sym->retType          != retType) || 
             (!!sym->isStatic       != !!(flags & mdStatic)) || 
             (!!sym->isVarargs      != !!isVarargs));

    sym->tokenImport = sym->tokenEmit = methodToken;

    return sym;
}

/*
 * Reads in existing parameter tokens
 */
void IMPORTER::ReadExistingParameterTokens(METHSYM *method, METHINFO *info)
{
    INFILESYM* inputfile = method->getInputFile();
    ASSERT(inputfile->isSource && inputfile->metaimport[method->getImportScope()]);

    IMetaDataImport * metaimport = inputfile->metaimport[method->getImportScope()];

    TimerStart(TIME_IMI_GETPARAMFORMETHODINDEX);
    HRESULT hr = metaimport->GetParamForMethodIndex(
        method->tokenEmit,
        0,
        &info->returnValueInfo.tokenEmit);
    // return value param may not exist
    if (hr != CLDB_E_RECORD_NOTFOUND) {
        CheckHR(hr, inputfile);
        compiler()->emitter.DeleteRelatedParamTokens(inputfile, info->returnValueInfo.tokenEmit);
    }
    TimerStop(TIME_IMI_GETPARAMFORMETHODINDEX);

    for (ULONG i = 1; i <= method->cParams; i += 1) {
        TimerStart(TIME_IMI_GETPARAMFORMETHODINDEX);
        hr = metaimport->GetParamForMethodIndex(
            method->tokenEmit,
            i,
            &info->paramInfos[i-1].tokenEmit);
        // params must exist
        CheckHR(hr, inputfile);
        TimerStop(TIME_IMI_GETPARAMFORMETHODINDEX);
        compiler()->emitter.DeleteRelatedParamTokens(inputfile, info->paramInfos[i-1].tokenEmit);
    }
}

/*
 * Match the propertyToken with a SYM which must already exist in the given cls.
 */
PROPSYM * IMPORTER::ReadExistingProperty(PINFILESYM inputfile, AGGSYM *cls, mdToken propertyToken)
{
    DWORD scope = cls->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);

    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR propnameText[MAX_IDENT_SIZE];     // name of method
    ULONG cchPropnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    PNAME name, realName;
    PROPSYM *sym = NULL;

    bool isBogus = false;
    bool isVarargs = false;
    bool isParams = false;
    TYPESYM *retType = NULL;
    TYPESYM **paramTypes = NULL;
    unsigned short cParams = 0;

    // Get property properties.
    TimerStart(TIME_IMI_GETPROPERTYPROPS);
    CheckHR(metaimport->GetPropertyProps(
                propertyToken, 					                // The method for which to get props.
                NULL,				                                // Put method's class here.
                propnameText, lengthof(propnameText), &cchPropnameText,         // Method name
                & flags,			                                // Put flags here.
                & signature, & cbSignature,		                        // Method signature
                NULL, NULL, NULL,                                               // Default value
                NULL, NULL,                                                     // Setter, getter
                NULL, 0, NULL), inputfile);                                     // Other methods
    TimerStop(TIME_IMI_GETPROPERTYPROPS);

    // Get name.
    if (cchPropnameText <= 1)
        return NULL;
    name = compiler()->namemgr->AddString(propnameText, cchPropnameText - 1);

    // Get the property signature.
    ImportSignature(inputfile, scope, propertyToken, signature, false, false,
                    &retType, &cParams, &paramTypes, &isBogus, &isVarargs, &isParams, NULL);
    if (isBogus) return NULL;

    if (cParams != 0) {
        realName = name;
        name = compiler()->namemgr->GetPredefName(PN_INDEXERINTERNAL);
    } else {
        realName = NULL;
    }

    //
    // find property, and check signature
    //
    SYM *searchSym = NULL;
    do 
    {
        searchSym = compiler()->clsDeclRec.findNextName(name, cls, searchSym);
        if (searchSym && searchSym->kind == SK_EVENTSYM) continue;
        if (!searchSym || searchSym->parent != cls || searchSym->kind != SK_PROPSYM) return NULL;
        sym = searchSym->asPROPSYM();
    } while (sym->params != paramTypes);
    if (sym->retType != retType) return NULL;
    if (!!sym->isVarargs != !!isVarargs) return NULL;
    if (sym->isIndexer() && (sym->getRealName() != realName)) return NULL;

    sym->tokenEmit = propertyToken;

    return sym;
}

EVENTSYM * IMPORTER::ReadExistingEvent(PINFILESYM inputfile, AGGSYM * cls, mdToken eventToken)
{
    ASSERT(inputfile->isSource && inputfile->metaimport[cls->getImportScope()]);
    IMetaDataImport * metaimport = inputfile->metaimport[cls->getImportScope()];
    WCHAR eventnameText[MAX_IDENT_SIZE];     // name of event
    ULONG cchEventnameText;
    PNAME name;
    EVENTSYM * event;

    // Get Event propertys
    TimerStart(TIME_IMI_GETEVENTPROPS);
    CheckHR(metaimport->GetEventProps(
        eventToken,                         // [IN] event token 
        NULL,                               // [OUT] typedef containing the event declarion.    
        eventnameText,                      // [OUT] Event name 
        lengthof(eventnameText),            // [IN] the count of wchar of szEvent   
        &cchEventnameText,                  // [OUT] actual count of wchar for event's name 
        NULL,                               // [OUT] Event flags.   
        NULL,                               // [OUT] EventType class    
        NULL,                               // [OUT] AddOn method of the event  
        NULL,                               // [OUT] RemoveOn method of the event   
        NULL,                               // [OUT] Fire method of the event   
        NULL,                               // [OUT] other method of the event  
        0,                                  // [IN] size of rmdOtherMethod  
        NULL), inputfile);                  // [OUT] total number of other method of this event 
    TimerStop(TIME_IMI_GETEVENTPROPS);

    // Get name.
    if (cchEventnameText <= 1)
        return NULL;
    name = compiler()->namemgr->AddString(eventnameText, cchEventnameText - 1);

    // Find the event.
    event = compiler()->symmgr.LookupGlobalSym(name, cls, MASK_EVENTSYM)->asEVENTSYM();
    if (!event) return NULL;

    // Set the metadata token.
    event->tokenEmit = eventToken;
    return event;
}

AGGSYM *IMPORTER::HasTypeDefChanged(PINFILESYM infile, DWORD scope, mdTypeDef token)
{
    DWORD flags;
    bool isValueType;
    AGGSYM *sym = ImportOneTypeBase(infile, scope, token, true, &flags, &isValueType);
    if (!sym)
        return NULL;

    if (sym->access != AccessFromTypeFlags(flags, sym))
        return NULL;

    if (isValueType)
    {
        // need this for the incremental case
        if (!sym->isStruct && !sym->isEnum)
            return NULL;
        if (!sym->isSealed)
            return NULL;

    }
    else if (IsTdInterface(flags))
    {
        // interface
        if (!sym->isInterface)
            return NULL;
        if (!sym->isAbstract)
            return NULL;
    }
    else
    {
        // regular class
        if (!sym->isClass && !sym->isDelegate)
            return NULL;
        if (!!sym->isAbstract != !!(flags & tdAbstract))
            return NULL;
        if (!!sym->isSealed != !!(flags & tdSealed))
            return NULL;
	}
	ASSERT(!(sym->isEnum && (sym->isStruct || sym->isClass || sym->isDelegate)));

    return sym;
}

/*
 * Check this type to see if it has had an interface change.
 */
bool IMPORTER::HasInterfaceChange(const AGGSYM *sym)
{
    IMetaDataImport * metaimport;
    PINFILESYM inputfile;
    DWORD scope;

    HCORENUM corenum;           // For enumerating tokens.
    mdToken tokens[32];
    ULONG cTokens, iToken;

    ASSERT(!sym->isResolvingBaseClasses && sym->hasResolvedBaseClasses && sym->isDefined);

    // Get the meta-data import interface.
    inputfile = ((AGGSYM*)sym)->getInputFile();
    scope = sym->getImportScope();
    metaimport = inputfile->metaimport[scope];
    ASSERT(metaimport);


    // we don't have a way to delete MethodImpl records
    // so we need to verify that all existing MethodImpl records will be reused
    // and if they aren't then we need to do a full rebuild
    //
    corenum = 0;
    do {
        mdToken declTokens[32];

        // get next batch of method impls
        TimerStart(TIME_IMI_ENUMMETHODIMPLS);
        CheckHR(metaimport->EnumMethodImpls(&corenum, sym->tokenImport, tokens, declTokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMMETHODIMPLS);

        // process each method impl
        for (iToken = 0; iToken < cTokens; ++iToken) {
            if (HasMethodImplExist(inputfile, sym, tokens[iToken], declTokens[iToken])) {
                // If test flags is set dump log
                if (compiler()->options.logIncrementalBuild)
                    printf("Incremental Build: MethodImpl is changed!\n");
                compiler()->SetFullRebuild();
                return true;
            }
        }
    } while (cTokens > 0);


    unsigned cFields = 0;
    unsigned cMethods = 0;
    unsigned cProperties = 0;
    unsigned cEvents = 0;

    // Import fields. Interfaces don't have them.
    corenum = 0;
    do {
        // Get next batch of fields.
        TimerStart(TIME_IMI_ENUMFIELDS);
        CheckHR(metaimport->EnumFields(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMFIELDS);

        // Process each fields.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            if (HasFieldChanged(inputfile, sym, tokens[iToken], &cFields)) return true;
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    // Import methods.
    corenum = 0;
    do {
        // Get next batch of methods.
        TimerStart(TIME_IMI_ENUMMETHODS);
        CheckHR(metaimport->EnumMethods(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMMETHODS);

        // Process each method.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            if (HasMethodChanged(inputfile, sym, tokens[iToken], &cMethods)) return true;
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    corenum = 0;
    do {
        // Get next batch of properties.
        TimerStart(TIME_IMI_ENUMPROPERTIES);
        CheckHR(metaimport->EnumProperties(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMPROPERTIES);

        // Process each property.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            if (HasPropertyChanged(inputfile, sym, tokens[iToken], &cProperties)) return true;
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    // Import events. These must be done after methods, because events refer to methods.
    corenum = 0;
    do {
        // Get next batch of events.
        TimerStart(TIME_IMI_ENUMEVENTS);
        CheckHR(metaimport->EnumEvents(&corenum, sym->tokenImport, tokens, lengthof(tokens), &cTokens), inputfile);
        TimerStop(TIME_IMI_ENUMEVENTS);

        // Process each event.
        for (iToken = 0; iToken < cTokens; ++iToken) {
            if (HasEventChanged(inputfile, sym, tokens[iToken], &cEvents)) return true;
        }
    } while (cTokens > 0);
    metaimport->CloseEnum(corenum);

    //
    // Check that we haven't added any members
    //
    unsigned cNewFields = 0;
    unsigned cNewMethods = 0;
    unsigned cNewProperties = 0;
    unsigned cNewEvents = 0;
    FOREACHCHILD(sym, child)
        if (child->access != ACC_PRIVATE) {
            switch (child->kind) {
            case SK_MEMBVARSYM:     
                cNewFields += 1;        
                break;
            case SK_METHSYM:        
                cNewMethods += 1;       
                break;
            case SK_PROPSYM:
                cNewProperties += 1;    
                // don't have symbols for accessors until the prepare stage
                // if the count is the same then nothing has changed
                if (child->asPROPSYM()->parseTree->asANYPROPERTY()->pGet) {
                    cNewMethods += 1;
                }
                if (child->asPROPSYM()->parseTree->asANYPROPERTY()->pSet) {
                    cNewMethods += 1;
                }
                break;
            case SK_EVENTSYM:       
                cNewEvents += 1;    
                break;
            default:                                        
                break;
            }
        }
    ENDFOREACHCHILD;
    if (cFields     != cNewFields       || 
        cMethods    != cNewMethods      || 
        cProperties != cNewProperties   || 
        cEvents     != cNewEvents) {
        return true;
    }

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, sym->tokenImport);

    if (sym->attributeClass != attributes.attributeKind ||
        sym->isMultipleAttribute != attributes.allowMultiple) {

        return true;
    }

    if (sym->isDeprecated != attributes.isDeprecated ||
        !sym->deprecatedMessage != !attributes.deprecatedString ||
        (sym->deprecatedMessage && wcscmp(sym->deprecatedMessage, attributes.deprecatedString)) ||
        sym->isDeprecatedError != attributes.isDeprecatedError ||
        sym->hasCLSattribute != sym->hasCLSattribute ||
        sym->isCLS != attributes.isCLS) {

        return true;
    }

    return false;
}

/*
 * check a field to see if it has changed since the last incremental build
 */
bool IMPORTER::HasFieldChanged(PINFILESYM inputfile, const AGGSYM *parent, mdToken tokenField, unsigned *cFields)
{
    DWORD scope = parent->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);

    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR fieldnameText[MAX_IDENT_SIZE];     // name of field
    ULONG cchFieldnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    DWORD constType;
    LPCVOID constValue;
    ULONG constLen;
    PNAME name;
    const MEMBVARSYM * sym;
    PTYPESYM type;
    bool isVolatile;

    // Get properties of the field from metadata.
    TimerStart(TIME_IMI_GETFIELDPROPS);
    CheckHR(metaimport->GetFieldProps(
		tokenField, 		                                    // The field for which to get props.
		NULL,		                                            // Put field's class here.
		fieldnameText, lengthof(fieldnameText), & cchFieldnameText, // Field name
		& flags,		                                    // Field flags
		& signature, & cbSignature,                                 // Field signature
		& constType, & constValue, & constLen), inputfile);         // Field constant value
    TimerStop(TIME_IMI_GETFIELDPROPS);

    // don't include privates
    if (ACC_PRIVATE == ConvertAccessLevel(flags, parent)) return false;

    // Get name.
    if (cchFieldnameText <= 1)
        return true;
    name = compiler()->namemgr->AddString(fieldnameText, cchFieldnameText - 1);

    // Import the type of the field.
    type = ImportFieldType(inputfile, scope, signature, &isVolatile);
    if (!type) return true;

    // handle "special" enum meber
    if (parent->isEnum && name == compiler()->namemgr->GetPredefName(PN_ENUMVALUE)) {
        if (type != parent->underlyingType) return true;
        return false;
    }

    //
    // find the field by name and type
    //
    sym = compiler()->symmgr.LookupGlobalSym(name, (AGGSYM*)parent, MASK_MEMBVARSYM)->asMEMBVARSYM();
    if (!sym) return true;

    // Enums are a bit special. Non-static fields serve only to record the
    // underlying integral type, and are otherwise ignored. Static fields are
    // enumerators and must be of the enum type. (We change other integral ones to the
    // enum type because it's probably what the emitting compiler meant.)
    if (parent->isEnum) {
        if (flags & fdStatic) {
            if ((const TYPESYM*)type != parent) {
                if (type->fundType() <= FT_LASTINTEGRAL)
                    type = (TYPESYM*)parent;   // If it's integral, assume it's meant to be the enum type.
                else
                    type = NULL;     // Bogus type.
            }
        }
        else {
            // Assuming its an integral type, use it to set the
            // enum base type.
            if (type->isNumericType() && type->fundType() <= FT_LASTINTEGRAL) {
                if (parent->underlyingType != type->asAGGSYM()) return true;
            } else {
                return true;
            }
        }
    }
    if (sym->type != type) return true;


    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, tokenField);

    if (sym->isDeprecated != attributes.isDeprecated ||
        !sym->deprecatedMessage != !attributes.deprecatedString ||
        (sym->deprecatedMessage && wcscmp(sym->deprecatedMessage, attributes.deprecatedString)) ||
        sym->isDeprecatedError != attributes.isDeprecatedError ||
        sym->hasCLSattribute != sym->hasCLSattribute ||
        sym->isCLS != attributes.isCLS) {

        return true;
    }

    // Set attributes of the field.
    if (sym->access != ConvertAccessLevel(flags, sym)) return true;

    if (!!sym->isStatic != !!(flags & fdStatic)) return true;
    if (!!sym->isVolatile != !!isVolatile) return true;
    ASSERT(!sym->isUnevaled);
    if (((flags & fdLiteral) || (flags & fdInitOnly)) 
        && (flags & fdStatic) && constType != ELEMENT_TYPE_VOID 
        && (! sym->type->isPredefType(PT_DECIMAL))) {
        if (sym->isConst != true) return true;

        // Try to import the value from COM+ metadata.
        CONSTVAL constVal;
        if ( ! ImportConstant(&constVal, constLen, type, constType, constValue)) return true;
        switch (type->fundType()) {
        case FT_I1:
        case FT_U1:
        case FT_I2:
        case FT_U2:
        case FT_I4:
        case FT_U4:
            if (constVal.iVal != sym->constVal.iVal) return true; 
            break;

        case FT_I8:
        case FT_U8:
            if (*constVal.longVal != *sym->constVal.longVal) return true; 
            break;

        case FT_R4:
        case FT_R8:
            // 64-bit constant must be allocated.
            if (*constVal.doubleVal != *sym->constVal.doubleVal) return true;
            break;

        default:
            if (constType == ELEMENT_TYPE_STRING) {
                if (constVal.strVal->length != sym->constVal.strVal->length ||
                    wcsncmp(constVal.strVal->text, sym->constVal.strVal->text, sym->constVal.strVal->length)) return true;
            } else if (constType == ELEMENT_TYPE_CLASS) {
                if (constVal.iVal != sym->constVal.iVal) return true;
            } else {
                // unknown constant type
                return true;
            }
        }
    }
    else {
        if (sym->isConst) return true;
    }

    // decimal literals are stored in a custom blob since they can't be represented MD directly
    if ((flags & fdInitOnly) && (flags & fdStatic) && sym->type->isPredefType(PT_DECIMAL)) {

        if (!!sym->isConst != !!attributes.hasDecimalLiteral) return true;
        if (attributes.hasDecimalLiteral) {
            if (memcmp(sym->constVal.decVal, &attributes.decimalLiteral, sizeof(DECIMAL))) return true;
        }
    }
    else {
        // Check read only flag.
        if (!!sym->isReadOnly != !!(flags & fdInitOnly))
            return true;
    }


    if (parent->isEnum && !sym->isConst) {
        // Enum members better be read-only constants.
        return true;
    }

    *cFields += 1;
    return false;
}

/*
 * Checks that a method impl record will be reused. Returns true if not reused.
 */
bool IMPORTER::HasMethodImplExist(PINFILESYM inputfile, const AGGSYM *cls, mdToken bodyToken, mdToken declToken)
{
    DWORD scope = cls->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);

    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR methodnameText[MAX_IDENT_SIZE];     // name of method
    ULONG cchMethodnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    PNAME methodName;
    size_t ifaceNameLength;

    bool isBogus = false;
    bool isVarargs = false;
    bool isParams = false;
    TYPESYM *retType = NULL;
    TYPESYM **paramTypes = NULL;
    unsigned short cParams = 0;
    bool couldBeGet = false, couldBeSet = false;
    bool couldBeAdd = false, couldBeRemove = false;
    NAME *propertyName = NULL;
    NAME *eventName = NULL;

    // Get method properties.
    TimerStart(TIME_IMI_GETMETHODPROPS);
    CheckHR(metaimport->GetMethodProps(
		bodyToken, 					                // The method for which to get props.
		NULL,				                                // Put method's class here.
		methodnameText,	lengthof(methodnameText), &cchMethodnameText,   // Method name
		& flags,			                                // Put flags here.
		& signature, & cbSignature,		                        // Method signature
		NULL,			                                        // codeRVA
		NULL), inputfile);                                              // Impl. Flags
    TimerStop(TIME_IMI_GETMETHODPROPS);

    // If the name doesn't contain a '.' then it isn't an explicit interface impl
    // and it must be a override. We can't verify that we'll reemit the override impl
    // so just assume that it has changed.
    if (cchMethodnameText <= 1 || wcschr(methodnameText, L'.') == NULL)
        return true;

    ifaceNameLength = wcsrchr(methodnameText, L'.') - methodnameText;
    methodName = compiler()->namemgr->AddString(methodnameText + ifaceNameLength + 1);

    // Set the method signature.
    ImportSignature(inputfile, scope, bodyToken, signature, true, !!(flags & mdStatic),
                    &retType, &cParams, &paramTypes, &isBogus, &isVarargs, &isParams, NULL);
    if (isBogus) return true;

    // check for possible accessor
    if (wcslen(methodName->text) > wcslen(L"get_")) {

        couldBeGet = !wcsncmp(methodName->text, L"get_", wcslen(L"get_"));
        couldBeSet = !wcsncmp(methodName->text, L"set_", wcslen(L"set_"));

        if (couldBeGet || couldBeSet) {
            propertyName = compiler()->namemgr->AddString(methodName->text + wcslen(L"get_"));
        } else if (retType->kind == SK_VOIDSYM && cParams == 1) {

            couldBeAdd = !wcsncmp(methodName->text, L"add_", wcslen(L"add_"));
            couldBeRemove = !wcsncmp(methodName->text, L"remove_", wcslen(L"remove_"));
            if (couldBeAdd) {
                eventName = compiler()->namemgr->AddString(methodName->text + wcslen(L"get_"));
            } else if (couldBeRemove && methodName->text[wcslen(L"remove_")]) {
                eventName = compiler()->namemgr->AddString(methodName->text + wcslen(L"remove_"));
            }
        }
    }

    //
    // find method, and check signature
    //
    for (SYM *searchSym = compiler()->symmgr.LookupGlobalSym(NULL, (AGGSYM*)cls, MASK_ALL);
         searchSym != NULL;
         searchSym = compiler()->symmgr.LookupNextSym(searchSym, searchSym->parent, MASK_ALL)) {

        BASENODE *ifaceNameTree;
        if (searchSym->kind == SK_METHSYM) {

            const METHSYM * sym = searchSym->asMETHSYM();
            if (sym->params != paramTypes || sym->retType != retType) {
                continue;
            }
            if (!sym->parseTree->InGroup(NG_METHOD)) {
                // property or event accessor
                continue;
            }
            BASENODE * nameTree = sym->parseTree->asANYMETHOD()->pName;
            if (methodName != nameTree->asDOT()->p2->asNAME()->pName) {
                continue;
            }
            ifaceNameTree = nameTree->asDOT()->p1;
        } else if (propertyName && searchSym->kind == SK_PROPSYM) {
            PROPSYM * sym = searchSym->asPROPSYM();

            // check name and signature
            BASENODE *nameTree = sym->parseTree->asANYPROPERTY()->pName;
            if (sym->cParams > 0) {
                if (sym->getRealName()) {
                    continue;
                }
                if (couldBeSet) {
                    if ((sym->cParams + 1) != cParams) {
                        continue;
                    }
		    int i;
                    for (i = 0; i < sym->cParams; i += 1) {
                        if (sym->params[i] != paramTypes[i]) {
                            continue;
                        }
                    }
                    if (sym->retType != paramTypes[i]) {
                        continue;
                    }
                }
                ifaceNameTree = nameTree;

            } else {
                if (propertyName != nameTree->asDOT()->p2->asNAME()->pName) {
                    continue;
                }
                if (couldBeSet && (!paramTypes || sym->retType != paramTypes[0] || retType->kind != SK_VOIDSYM)) {
                    continue;
                }
                ifaceNameTree = nameTree->asDOT()->p1;
            }
            if (couldBeGet && (sym->retType != retType || sym->params != paramTypes)) {
                continue;
            }

            // check accessor type
            if (couldBeGet && !sym->parseTree->asANYPROPERTY()->pGet) {
                continue;
            }
            if (couldBeSet && !sym->parseTree->asANYPROPERTY()->pSet) {
                continue;
            }
        } else if (eventName && searchSym->kind == SK_EVENTSYM) {
            EVENTSYM *sym = searchSym->asEVENTSYM();

            // check name and signature
            BASENODE *nameTree = sym->parseTree->asANYPROPERTY()->pName;
            if (eventName != nameTree->asDOT()->p2->asNAME()->pName) {
                continue;
            }
            if (sym->type != paramTypes[0]) {
                continue;
            }
            ifaceNameTree = nameTree->asDOT()->p1;
        } else {
            continue;
        }

        // signature and member name match
        // now need a match on the interface
        if (ifaceNameTree) {
            AGGSYM *iface = compiler()->clsDeclRec.bindTypeName(ifaceNameTree, (AGGSYM*) cls, BTF_NOERRORS, NULL)->asAGGSYM();
            if (!iface) return true;

            WCHAR nameBuffer[MAX_FULLNAME_SIZE];
            METADATAHELPER::GetFullMetadataName(iface, nameBuffer, lengthof(nameBuffer));
            if (!wcsncmp(nameBuffer, methodnameText, ifaceNameLength) && !nameBuffer[ifaceNameLength]) {
                return false;
            }
        }
    }

    return true;
}

/*
 * Check this method to see if it has changed since the last incremental build
 */
bool IMPORTER::HasMethodChanged(PINFILESYM inputfile, const AGGSYM *cls, mdToken methodToken, unsigned * cMethods)
{
    DWORD scope = cls->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);

    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR methodnameText[MAX_IDENT_SIZE];     // name of method
    ULONG cchMethodnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    PNAME name;
    const METHSYM * sym = NULL;
    bool isImplicit = false;
    bool isExplicit = false;
    bool isOperator = false;

    bool isBogus = false;
    bool isVarargs = false;
    bool isParams = false;
    TYPESYM *retType = NULL;
    TYPESYM **paramTypes = NULL;
    unsigned short cParams = 0;

    // Get method properties.
    TimerStart(TIME_IMI_GETMETHODPROPS);
    CheckHR(metaimport->GetMethodProps(
		methodToken, 					                // The method for which to get props.
		NULL,				                                // Put method's class here.
		methodnameText,	lengthof(methodnameText), &cchMethodnameText,   // Method name
		& flags,			                                // Put flags here.
		& signature, & cbSignature,		                        // Method signature
		NULL,			                                        // codeRVA
		NULL), inputfile);                                              // Impl. Flags
    TimerStop(TIME_IMI_GETMETHODPROPS);

    // don't include privates
    if (ACC_PRIVATE == ConvertAccessLevel(flags, cls)) return false;
    
    if (cchMethodnameText <= 1)
        return true;

    // If the name contains a '.', and is not a "special" name, then we won't be able to access if,
    // and its quite probabaly a explicit method implementation. Just don't import it. 
    if (!(flags & mdRTSpecialName) && wcschr(methodnameText, L'.') != NULL)
        return false;

    name = compiler()->namemgr->AddString(methodnameText, cchMethodnameText - 1);

    // Set the method signature.
    ImportSignature(inputfile, scope, methodToken, signature, true, !!(flags & mdStatic),
                    &retType, &cParams, &paramTypes, &isBogus, &isVarargs, &isParams, NULL);
    if (isBogus) return true;

    //
    // find method, and check signature
    //
    SYM *searchSym = compiler()->symmgr.LookupGlobalSym(name, (AGGSYM*)cls, MASK_ALL);
    if (searchSym && searchSym->kind != SK_METHSYM) {
        sym = NULL;
    } else {
        sym = searchSym->asMETHSYM();
        while (sym && (sym->params != paramTypes || sym->retType != retType)) {
            searchSym = compiler()->symmgr.LookupNextSym(searchSym, searchSym->parent, MASK_ALL);
            sym = searchSym->asMETHSYM();
        }
    }
    if (!sym) {
        // if we don't find a public method
        // then it could be a property accessor
        // return no change, but bump the number of non-private methods
        // we'll match accessor changes by the total count of methods later
        *cMethods += 1;
        return false;
    }

    if (!!sym->isVarargs != !!isVarargs) return true;
    if (!sym->isPropertyAccessor && !!sym->isParamArray != !!isParams) return true;

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, methodToken);

    // Set attributes of the method.
    if (sym->access != ConvertAccessLevel(flags, sym)) return true;

    if (!!sym->isStatic != !!(flags & mdStatic)) return true;
    if (!!sym->isAbstract != !!(flags & mdAbstract)) return true;
    if (!!sym->isCtor != !!((flags & mdRTSpecialName) ? (name == compiler()->namemgr->GetPredefName((flags & mdStatic) ? PN_STATCTOR : PN_CTOR)) : false)) return true;
    if (!!sym->isVirtual != !!((flags & mdVirtual) && !sym->isCtor && !(flags & mdFinal))) return true;
    if (sym->isVirtual || sym->isMetadataVirtual) {
        if (sym->isOverride && !sym->requiresMethodImplForOverride) {
            if (flags & mdNewSlot) return true;
        } else {
            if (!(flags & mdNewSlot)) return true;
        }
    }


    // we are importing an abstract method which
    // is not marked as virtual, currently code generation
    // keys on the isVirtual property to set the dispatch type
    // on function calls.
    //
    // if this fires, find out who generated the metadata
    // and get them to fix their metadata generation
    //
    ASSERT(sym->isVirtual || !sym->isAbstract);

    //
    // convert special methods into operators.
    // If we don't recognize this as a C#-like operator then we just ignore the special
    // bit and make it a regular method
    //
    if ((flags & mdSpecialName) && !(flags & mdRTSpecialName)) {
		if (name == compiler()->namemgr->GetPredefName(PN_OPEXPLICITMN)) {
            isOperator = true;
            isExplicit = true;
        }
        else if (name == compiler()->namemgr->GetPredefName(PN_OPIMPLICITMN)) {
            isOperator = true;
            isImplicit = true;
        }
        else {
            OPERATOR iOp = compiler()->clsDeclRec.operatorOfName(name);
            if (OP_LAST != iOp && OP_EXPLICIT != iOp && OP_IMPLICIT != iOp) {
                isOperator = true;
            }
        }
    }
    if (!!sym->isOperator != !!isOperator) return true;
    if (!!sym->isImplicit != !!isImplicit) return true;
    if (!!sym->isExplicit != !!isExplicit) return true;

    if (sym->conditionalSymbols != attributes.conditionalSymbols) {

        FOREACHNAMELIST(sym->conditionalSymbols, symName)
            NAMELIST *readNames = attributes.conditionalSymbols;
            while (readNames) {
                if (readNames->name == symName) {
                    break;
                }
                readNames = readNames->next;
            }
            if (!readNames) {
                return true;
            }
        ENDFOREACHNAMELIST
    }

    if (sym->isDeprecated != attributes.isDeprecated ||
        !sym->deprecatedMessage != !attributes.deprecatedString ||
        (sym->deprecatedMessage && wcscmp(sym->deprecatedMessage, attributes.deprecatedString)) ||
        sym->isDeprecatedError != attributes.isDeprecatedError ||
        sym->hasCLSattribute != sym->hasCLSattribute ||
        sym->isCLS != attributes.isCLS) {

        return true;
    }

    *cMethods += 1;
    return false;
}

//
// find out the property access level from an accessor
//
ACCESS IMPORTER::GetAccessOfMethod(mdToken methodToken, AGGSYM *cls)
{
    DWORD methodFlags;
    DWORD scope = cls->getImportScope();

    // Get method properties.
    TimerStart(TIME_IMI_GETMETHODPROPS);
    CheckHR(cls->getInputFile()->metaimport[scope]->GetMethodProps(
		methodToken, 			    // The method for which to get props.
		NULL,				    // Put method's class here.
		NULL,	0, NULL,                    // Method name
		& methodFlags,                      // Put flags here.
		NULL, NULL,		            // Method signature
		NULL,			            // codeRVA
		NULL), cls->getInputFile());        // Impl. Flags
    TimerStop(TIME_IMI_GETMETHODPROPS);

    // don't include privates
    return ConvertAccessLevel(methodFlags, cls);
}


/*
 * returns true if this property has changed since the last incremental build
 */
bool IMPORTER::HasPropertyChanged(PINFILESYM inputfile, const AGGSYM *cls, mdToken propertyToken, unsigned * cProperties)
{
    DWORD scope = cls->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);

    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR propnameText[MAX_IDENT_SIZE];     // name of method
    ULONG cchPropnameText;
    DWORD flags;
    PCCOR_SIGNATURE signature;
    ULONG cbSignature;
    PNAME name, realName;
    const PROPSYM *sym;
    mdToken tokenSetter, tokenGetter;

    bool isBogus = false;
    bool isVarargs = false;
    bool isParams = false;
    TYPESYM *retType = NULL;
    TYPESYM **paramTypes = NULL;
    unsigned short cParams = 0;
    ACCESS access;

    // Get property properties.
    TimerStart(TIME_IMI_GETPROPERTYPROPS);
    CheckHR(metaimport->GetPropertyProps(
                propertyToken, 					                // The method for which to get props.
                NULL,				                                // Put method's class here.
                propnameText, lengthof(propnameText), &cchPropnameText,         // Method name
                & flags,			                                // Put flags here.
                & signature, & cbSignature,		                        // Method signature
                NULL, NULL, NULL,                                               // Default value
                & tokenSetter, & tokenGetter,                                   // Setter, getter,
                NULL, 0, NULL), inputfile);                                     // Other methods
    TimerStop(TIME_IMI_GETPROPERTYPROPS);

    //
    // find out the property access level from an accessor
    //
    access = GetAccessOfMethod((tokenGetter != mdMethodDefNil) ? tokenGetter : tokenSetter, (AGGSYM*)cls);

    // don't include private members
    if (access == ACC_PRIVATE) return false;

    // Get name.
    if (cchPropnameText <= 1)
        return true;
    name = compiler()->namemgr->AddString(propnameText, cchPropnameText - 1);

    // Get the property signature.
    ImportSignature(inputfile, scope, propertyToken, signature, false, false,
                    &retType, &cParams, &paramTypes, &isBogus, &isVarargs, &isParams, NULL);
    if (isBogus) return true;

    if (cParams > 0) {
        realName = name;
        name = compiler()->namemgr->GetPredefName(PN_INDEXERINTERNAL);
    } else {
        realName = NULL;
    }

    //
    // find property, and check signature
    //
    sym = NULL;
    SYM *searchSym = compiler()->symmgr.LookupGlobalSym(name, (AGGSYM*)cls, MASK_ALL);
    while (searchSym) {
        if (searchSym->kind == SK_PROPSYM) {
            sym = searchSym->asPROPSYM();
            if (sym->params == paramTypes) break;
            sym = NULL;
        } else if (searchSym->kind != SK_EVENTSYM) {
            break;
        }

        searchSym = compiler()->symmgr.LookupNextSym(searchSym, searchSym->parent, MASK_ALL);
    }
    if (!sym) return true;

    // Check real name.
    if (cParams > 0) {
        if (!((PROPSYM*)sym)->isIndexer()) return true;
        if (((PROPSYM*)sym)->asINDEXERSYM()->realName != realName) return true;
    } else {
        if (((PROPSYM*)sym)->isIndexer()) return true;
    }

    // Check that accessors match
    if ((tokenGetter == mdMethodDefNil) != !sym->parseTree->asANYPROPERTY()->pGet) {
        return true;
    }
    if ((tokenSetter == mdMethodDefNil) != !sym->parseTree->asANYPROPERTY()->pSet) {
        return true;
    }

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, propertyToken);

    if (sym->isDeprecated != attributes.isDeprecated ||
        !sym->deprecatedMessage != !attributes.deprecatedString ||
        (sym->deprecatedMessage && wcscmp(sym->deprecatedMessage, attributes.deprecatedString)) ||
        sym->isDeprecatedError != attributes.isDeprecatedError ||
        sym->hasCLSattribute != sym->hasCLSattribute ||
        sym->isCLS != attributes.isCLS) {

        return true;
    }

    *cProperties += 1;
    return false;
}


/*
 * returns true if this event has changed since the last incremental build
 */
bool IMPORTER::HasEventChanged(PINFILESYM inputfile, const AGGSYM *cls, mdToken tokenEvent, unsigned *cEvents)
{
    DWORD scope = cls->getImportScope();
    ASSERT(inputfile->isSource && inputfile->metaimport[scope]);
    IMetaDataImport * metaimport = inputfile->metaimport[scope];
    WCHAR eventnameText[MAX_IDENT_SIZE];     // name of event
    ULONG cchEventnameText;
    PNAME name;
    EVENTSYM * event;
    DWORD flags;
    mdToken tokenEventType, tokenAdd, tokenRemove;
    TYPESYM * type;
    ACCESS access;

    // Get Event propertys
    TimerStart(TIME_IMI_GETEVENTPROPS);
    CheckHR(metaimport->GetEventProps(
        tokenEvent,                         // [IN] event token 
        NULL,                               // [OUT] typedef containing the event declarion.    
        eventnameText,                      // [OUT] Event name 
        lengthof(eventnameText),            // [IN] the count of wchar of szEvent   
        &cchEventnameText,                  // [OUT] actual count of wchar for event's name 
        & flags,                            // [OUT] Event flags.   
        & tokenEventType,                   // [OUT] EventType class    
        & tokenAdd,                         // [OUT] AddOn method of the event  
        & tokenRemove,                      // [OUT] RemoveOn method of the event   
        NULL,                               // [OUT] Fire method of the event   
        NULL,                               // [OUT] other method of the event  
        0,                                  // [IN] size of rmdOtherMethod  
        NULL), inputfile);                  // [OUT] total number of other method of this event 
    TimerStop(TIME_IMI_GETEVENTPROPS);

    access = GetAccessOfMethod(tokenAdd, (AGGSYM*)cls);

    // don't include private members
    if (access == ACC_PRIVATE) 
        return false;

    // Get name.
    if (cchEventnameText <= 1)
        return true;
    name = compiler()->namemgr->AddString(eventnameText, cchEventnameText - 1);

    // Find the event.
    event = compiler()->symmgr.LookupGlobalSym(name, (AGGSYM *)cls, MASK_EVENTSYM)->asEVENTSYM();
    if (!event) return true;

    if (event->access != access) 
        return true;

    // Get the event type.
    type = ResolveTypeRef(inputfile, scope, tokenEventType, false);
    if (type != event->type)  return true;

    // Find the accessor methods. 
    if (tokenAdd == mdMethodDefNil) return true;
    if (tokenRemove == mdMethodDefNil) return true;

    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, scope, &attributes, tokenEvent); 

    if (event->isDeprecated != attributes.isDeprecated ||
        !event->deprecatedMessage != !attributes.deprecatedString ||
        (event->deprecatedMessage && wcscmp(event->deprecatedMessage, attributes.deprecatedString)) ||
        event->isDeprecatedError != attributes.isDeprecatedError ||
        event->hasCLSattribute != event->hasCLSattribute ||
        event->isCLS != attributes.isCLS) 
    {
        return true;
    }

    *cEvents += 1;
    return false;
}

bool IMPORTER::HasAssemblyChanged(PINFILESYM inputfile, bool newAssemblyCLS, mdToken assemblyToken)
{
    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, 0, &attributes, assemblyToken); 

    return (!!attributes.isCLS != !!newAssemblyCLS);
}

bool IMPORTER::HasModuleChanged(PINFILESYM inputfile, bool newModuleCLS)
{
    //
    // Import all the attributes we are interested in at compile time
    //
    IMPORTED_CUSTOM_ATTRIBUTES attributes;
    ImportCustomAttributes(inputfile, 0, &attributes, TokenFromRid(1, mdtModule)); 

    return (!!attributes.isCLS != !!newModuleCLS);
}

/*
 * Opens an Assembly and adds all metadata files, but does
 * not add an AssemblyRef yet.
 * Returns true iff we imported the Assembly
 * Returns false if we had to 'suck-in' the file
 */
bool IMPORTER::OpenAssembly(PINFILESYM infile)
{
    ASSERT(!infile->assemimport && IsNilToken(infile->idLocalAssembly));

    mdAssembly              impAssem;

    ASSERT(infile->canRefThisFile);

    TimerStart(TIME_IMD_OPENSCOPE);
    CheckHR(FTL_MetadataCantOpenFile,
        compiler()->linker->ImportFile(infile->name->text, NULL, FALSE, &infile->mdImpFile, &infile->assemimport, &infile->dwScopes),
        infile);
    TimerStop(TIME_IMD_OPENSCOPE);


    if ( infile->assemimport == NULL) {
        ASSERT(infile->dwScopes == 1);
        if (!infile->isAddedModule) {
            // This is not an assembly and it wasn't added via addmodule
            compiler()->Error( NULL, ERR_ImportNonAssembly, infile->name->text);
            infile->canRefThisFile = false;
            return false;
        }

        // importing into the current assembly
        infile->assemblyIndex = 0;
        infile->idLocalAssembly = mdTokenNil;
        infile->isAddedModule = true;
        infile->assemimport = NULL;
    } else {
        if (infile->isAddedModule) {
            // This is an assembly and it was added via addmodule!
            compiler()->Error( NULL, ERR_AddModuleAssembly, infile->name->text);
            infile->canRefThisFile = false;
            return false;
        }


        //
        // assign this imported assembly a new assembly index
        // which we'll use until we begin emitting
        //
        infile->assemblyIndex = compiler()->currentAssemblyIndex++;
        infile->assemimport->GetAssemblyFromScope( &impAssem);


    }

    infile->metaimport = (IMetaDataImport**)compiler()->globalSymAlloc.AllocZero(sizeof(IMetaDataImport*) * infile->dwScopes);

    ImportTypes( infile );

    // Only import assembly level attributes if we are doing CLS checking
    // because there are no other attributes that we care about currently
    // We only check assembly attributes here, module attributes are in ImportTypes()
    if (compiler()->CheckForCLS()) {
        IMPORTED_CUSTOM_ATTRIBUTES attributes;
        ImportCustomAttributes(infile, 0, &attributes, mdtAssembly);

        infile->hasCLSattribute = true;
        // if we're doing CLS checking and the imported assembly doesn't have
        // the attribute assume the file is not compliant
        if (attributes.hasCLSattribute)
            infile->isCLS = attributes.isCLS;
    }

    return true;
}

//--------------------------------------------------------------------
// BinToUnicodeHex
//--------------------------------------------------------------------
VOID BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst)
{
    UINT x;
    UINT y;

#define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))

    for ( x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );  
        v = pSrc[x] & 0x0f;                 
        pDst[y++] = TOHEX( v ); 
    }                                    
    pDst[y] = '\0';
}


LPCWSTR IMPORTER::GetAssemblyName(INFILESYM *infile)
{
    if (!infile || infile->assemblyIndex == 0 || infile->isBCL) {
        return NULL;
    }

    if (!infile->assemblyName) {
        //
        // build the assembly name :
        //
        ASSERT(infile->assemimport != NULL);

        mdAssembly tkAsm;
        CheckHR(infile->assemimport->GetAssemblyFromScope( &tkAsm), infile);

        //
        // get required buffer size
        //
        ASSEMBLYMETADATA data;
        ULONG cbOriginator;
        ULONG cchName;
        memset(&data, 0, sizeof(data));
        CheckHR(infile->assemimport->GetAssemblyProps( 
            tkAsm, 
            NULL, &cbOriginator,// originator
            NULL,               // hask alg
            NULL, 0, &cchName,  // name
            &data,              // data
            NULL), infile);

        //
        // create string
        //
        const void *pOriginator;
        WCHAR *szName = (WCHAR*) _alloca(cchName * sizeof(WCHAR));
        data.szLocale = data.cbLocale ? (WCHAR*) _alloca(data.cbLocale * sizeof(WCHAR)) : NULL;
        CheckHR(infile->assemimport->GetAssemblyProps( 
            tkAsm, 
            &pOriginator, &cbOriginator,// originator
            NULL,               // hask alg
            szName, cchName, &cchName,  // name
            &data,              // data
            NULL), infile);

#define FMT_ASSEM_LONG L"%ws, Version=%hu.%hu.%hu.%hu, Culture=%ws, PublicKeyToken="
#define FMT_ASSEM_SHORT L"%ws, Version=%hu.%hu.%hu.%hu, Culture=%ws"
        WCHAR *buffer = (WCHAR*) _alloca((cchName+wcslen(FMT_ASSEM_LONG)+data.cbLocale+7*cbOriginator+1)*sizeof(WCHAR));
        swprintf(   buffer, 
                    (cbOriginator) 
                        ? L"%ws, Version=%hu.%hu.%hu.%hu, Culture=%ws, PublicKeyToken="
                        : L"%ws, Version=%hu.%hu.%hu.%hu, Culture=%ws",
                    szName,
                    data.usMajorVersion,
                    data.usMinorVersion,
                    data.usBuildNumber,
                    data.usRevisionNumber,
                    (data.szLocale && *data.szLocale) ? data.szLocale : L"neutral");

        // only add the strong name if the assembly being referenced has one
        if (cbOriginator)
        {
            BYTE *pStrongNameToken = 0;
            ULONG cbStrongNameToken = 0;
            CheckHR(StrongNameTokenFromPublicKey((BYTE*)pOriginator, cbOriginator, &pStrongNameToken, &cbStrongNameToken), infile);
            BinToUnicodeHex(pStrongNameToken, cbStrongNameToken, buffer+wcslen(buffer));
            StrongNameFreeBuffer(pStrongNameToken);
        }

        infile->assemblyName = compiler()->namemgr->AddString(buffer);
    }

    return infile->assemblyName->text;
}

#ifdef DEBUG
/*
 * Go through all types and declare all types imported from metadata. Used to test
 * the meta-data declaring code.
 */
void IMPORTER::DeclareAllTypes(PPARENTSYM parent)
{
    if (parent->kind == SK_NSSYM)
    {
        NSDECLSYM *declaration = parent->asNSSYM()->firstDeclaration();
        while (declaration)
        {
            DeclareAllTypes(declaration);
            declaration = declaration->nextDeclaration;
        }
    }
    else
    {
        FOREACHCHILD(parent, symChild)
            if (symChild->kind == SK_NSDECLSYM)
            {
                DeclareAllTypes(symChild->asNSDECLSYM());
            }
            else if (symChild->kind == SK_AGGSYM && symChild->asAGGSYM()->getInputFile() &&
                     !symChild->asAGGSYM()->getInputFile()->isSource && !symChild->isPrepared &&
                     symChild->asAGGSYM()->getInputFile()->metaimport &&
                     TypeFromToken(symChild->asAGGSYM()->tokenImport) == mdtTypeDef)
            {
                if (!symChild->asAGGSYM()->isDefined) 
                    DefineImportedType(symChild->asAGGSYM());
                DeclareAllTypes(symChild->asAGGSYM());
            }
        ENDFOREACHCHILD
    }
}
#endif //DEBUG
