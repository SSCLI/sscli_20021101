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
// File: alink.h
//
// main ALink interface
// ===========================================================================
#ifndef __alink_h__
#define __alink_h__

#ifndef DECLSPEC_SELECT_ANY
#define DECLSPEC_SELECT_ANY __declspec(selectany)
#endif // DECLSPEC_SELECT_ANY

EXTERN_GUID(LIBID_ALinkLib, 0x4e9db6fb, 0xff03, 0x4398, 0x9f, 0xcf, 0xea, 0xa0, 0xc5, 0x53, 0x11, 0xab);
EXTERN_GUID(CLSID_AssemblyLinker, 0xf7e02368, 0xa7f4, 0x471f, 0x8c, 0x5e, 0x98, 0x39, 0xed, 0x57, 0xcb, 0x5e);


typedef enum _AssemblyOptions {
    optAssemTitle   = 0,    // String
    optAssemDescription,    // String
    optAssemConfig,         // String
    optAssemOS,             // String encoded as: "dwOSPlatformId.dwOSMajorVersion.dwOSMinorVersion"
    optAssemProcessor,      // ULONG
    optAssemLocale,         // String
    optAssemVersion,        // String encoded as: "Major.Minor.Build.Revision"
    optAssemCompany,        // String
    optAssemProduct,        // String
    optAssemProductVersion, // String (aka InformationalVersion)
    optAssemCopyright,      // String
    optAssemTrademark,      // String
    optAssemKeyFile,        // String (filename)
    optAssemAlgID,          // ULONG
    optAssemFlags,          // ULONG
    optAssemHalfSign,       // Bool   (aka DelaySign)
    optAssemSatelliteVer,   // String encoded as: "Major.Minor.Build.Revision"

    optLastAssemOption
}   AssemblyOptions;


typedef enum _AssemblyFlags {
    afNone              = 0x00000000, // Normal case
    afInMemory          = 0x00000001, // An InMemory single-file assembly the filename == AssemblyName
    afCleanModules      = 0x00000002, // Use DeleteToken and Merging to remove the AssemblyAttributesGoHere
    afNoRefHash         = 0x00000004, // Do not generate hashes for AssemblyRefs
}   AssemblyFlags;

//-------------------------------------
//--- IALink
//-------------------------------------
//---
extern "C" const GUID IID_IALink;

#define AssemblyIsUBM ((mdAssembly)mdAssemblyNil)
#define MAX_IDENT_LEN 2048

#ifndef HALINKENUM
#define HALINKENUM  void*
#endif

#undef  INTERFACE
#define INTERFACE IALink
interface
#if defined(_MSC_VER)
__declspec(uuid("C8E77F39-3604-4fd4-85CF-38BDEB233AD4"))
#endif  // defined(_MSC_VER)
IALink : IUnknown
{
    // Call to initialize everything
    STDMETHOD(Init)(
        IMetaDataDispenserEx*
                        pDispenser,         // IN - MetaData Dispenser
        IMetaDataError* pErrorHandler) PURE;// IN - optional Error handling interface

    // Call this to import UBMs and Assemblies.
    // Now you can use the same code to import Metadata from both!
    STDMETHOD(ImportFile)(
        LPCWSTR         pszFilename,        // IN - filename of the file to add (must be fully qualified)
        LPCWSTR         pszTargetName,      // IN - (optional) filename to copy bound modules to.
        BOOL            fSmartImport,       // IN - Set to TRUE if you want to use ImportTypes, etc.  FALSE means you'll do your own importing
        mdToken*        pImportToken,       // OUT- unique ID for the file (used to uniquely identify it) - it could be and assembly or file
        IMetaDataAssemblyImport**
                        ppAssemblyScope,    // OUT- assembly import scope (NULL if the file is not an assembly)
        DWORD*          pdwCountOfScopes) PURE;
                                            // OUT- Count of files/scopes imported due to this file
    
    // If you call this function, the linker assumes you are building an assembly
    // Do NOT call this if you are producing UBMs
    STDMETHOD(SetAssemblyFile)(
        LPCWSTR         pszFilename,        // IN - filename of the manifest file (fully-qualified)
        IMetaDataEmit*  pEmitter,           // IN - Emitter interface for this file
        AssemblyFlags   afFlags,            // IN - flags for ALink to generate a proper Assembly
        mdAssembly*     pAssemblyID) PURE;  // OUT- Unique ID for this assembly


    // Call this to add files to the assembly (or just to create UBMs)
    STDMETHOD(AddFile)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly to this file to
        LPCWSTR         pszFilename,        // IN - filename of the file to add (must be fully qualified)
        DWORD           dwFlags,            // IN - COM+ FileDef flags (ffContainsNoMetaData, ffWriteable, etc.)
        IMetaDataEmit*  pEmitter,           // IN - Emitter interface for file, so we can emit stuff if needed
        mdFile*         pFileToken) PURE;   // OUT- unique ID for the file (used to uniquely identify it)

    // Call this to add imports to the assembly
    STDMETHOD(AddImport)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly to this file to
        mdToken         ImportToken,        // IN - imported file to add
        DWORD           dwFlags,            // IN - COM+ FileDef flags (ffContainsNoMetaData, ffWriteable, etc.)
        mdFile*         pFileToken) PURE;   // OUT- unique ID for the file (used to uniquely identify it)

    // Get an import scope for
    STDMETHOD(GetScope)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly to import to
        mdToken         FileToken,          // IN - unique ID for the file to import from
        DWORD           dwScope,            // IN - Which scope to import (must be from 0 to dwCountOfScopes - 1)
        IMetaDataImport**
                        ppImportScope) PURE;// OUT- Import interface for scope

    STDMETHOD(GetAssemblyRefHash)(
        mdToken         FileToken,          // IN - assembly to Reference (hash)
        const void**    ppvHash,            // OUT- pointer to blob
        DWORD*          pcbHash) PURE;      // OUT- size (in bytes) of hash blob

    // Call this to begin importing types from each scope imported via ImportFile()
    STDMETHOD(ImportTypes)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly to import to
        mdToken         FileToken,          // IN - unique ID for the file to import from
        DWORD           dwScope,            // IN - Which scope to import (must be from 0 to dwCountOfScopes - 1)
        HALINKENUM*     phEnum,             // OUT- Enumerator handle for the types in this scope
        IMetaDataImport**
                        ppImportScope,      // OUT- optional Import interface to use
        DWORD*          pdwCountOfTypes) PURE;
                                            // OUT- optional count of types in this scope

    // Use this to get Assembly-level custom attributes
    STDMETHOD(EnumCustomAttributes)(
        HALINKENUM      hEnum,              // IN - Enumerator handle
        mdToken         tkType,             // IN - Type of interest, mdTokenNil for all
        mdCustomAttribute
                        rCustomValues[],    // OUT- Put custom attribute tokens here
        ULONG           cMax,               // IN - Size of rCustomValues
        ULONG*          pcCustomValues) PURE;
                                            // OUT- optional count of token values here.

    // Call this to enumerate each type in each scope
    STDMETHOD(EnumImportTypes)(
        HALINKENUM      hEnum,              // IN - Enumerator handle
        DWORD           dwMax,              // IN - Max count of types to return
        mdTypeDef       aTypeDefs[],        // OUT- Array of TypeDef tokens, maxlength = dwMax
        DWORD*          pdwCount) PURE;     // OUT- Actual count of types in aTypeDefs returned

    // Call this to close the enum, and free up it's resources
    STDMETHOD(CloseEnum)(HALINKENUM hEnum) PURE;

    // Call this to specify a type as being exportable
    STDMETHOD(ExportType)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly to export from
        mdToken         FileToken,          // IN - FileToken or AssemblyID of file that defines the type
        mdTypeDef       TypeToken,          // IN - TypeDef token for the type
        LPCWSTR         pszTypename,        // IN - Fully-qualified type name to export
        DWORD           dwFlags,            // IN - Flags for ComType (tdPublic, tdNested*, etc.)
        mdExportedType* pType) PURE;        // OUT- Actual ExportedType token emitted (keep this only for emitting nested types)

    // Call this for nested types - prefered, because it's faster
    STDMETHOD(ExportNestedType)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly to export from
        mdToken         FileToken,          // IN - FileToken or AssemblyID of file that defines the type
        mdTypeDef       TypeToken,          // IN - TypeDef token for the type
        mdExportedType  ParentType,         // IN - ExportedType token of parent type (outer class)
        LPCWSTR         pszTypename,        // IN - Fully-qualified type name to export
        DWORD           dwFlags,            // IN - Flags for ComType (tdNested*, etc.)
        mdExportedType* pType) PURE;        // OUT- Actual ExportedType token emitted (keep this only for emitting nested types)

    // Call this to declare an embedded resource
    // the compiler must actually embed the resource itself
    STDMETHOD(EmbedResource)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly
        mdToken         FileToken,          // IN - FileToken or AssemblyID of file that has the resource
        LPCWSTR         pszResourceName,    // IN - name of resource
        DWORD           dwOffset,           // IN - Offset of resource from RVA
        DWORD           dwFlags) PURE;      // IN - Flags, mrPublic, mrPrivate, etc.

    // Call this to link in a resource
    // This internally calls AddFile(), It is an error if AssemblyID == AssemblyIsUBM
    STDMETHOD(LinkResource)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly
        LPCWSTR         pszFileName,        // IN - name of file
        LPCWSTR         pszNewLocation,     // IN - (optional) new filename (if non-NULL, pszFileName will be copied to pszNewLocation)
        LPCWSTR         pszResourceName,    // IN - name of resource
        DWORD           dwFlags) PURE;      // IN - Flags, mrPublic, mrPrivate, etc.

    // Call this to get the resultion scope for a TypeRef
    // Do NOT call this for nested types
    // You do not need to call this if FileToken == TargetFile
    STDMETHOD(GetResolutionScope)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly
        mdToken         FileToken,          // IN - file that is needing a reference
        mdToken         TargetFile,         // IN - token from ImportFile that the type is defined in (not ImportScope)
        mdToken*        pScope) PURE;       // OUT- AssemblyRef or ModuleRef

    // Call to set assembly level settings
    // Returns S_FALSE if this overrides a previous setting
    STDMETHOD(SetAssemblyProps)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly
        mdToken         FileToken,          // IN - file that is defining the property (may be NULL if AssemblyID != AssemblyIsUBM)
        AssemblyOptions Option,             // IN - Option to set
        VARIANT         Value) PURE;        // IN - New value for option

    // Call to set assembly level CAs
    // Returns S_FALSE if this overrides a previous setting or CA
    STDMETHOD(EmitAssemblyCustomAttribute)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly
        mdToken         FileToken,          // IN - file that is defining the property (may be NULL if AssemblyID != AssemblyIsUBM)
        mdToken         tkType,             // IN - Type of the CustomAttribute.
        void const*     pCustomValue,       // IN - The custom value data.
        DWORD           cbCustomValue,      // IN - The custom value data length.
        BOOL            bSecurity,          // IN - True if the CA is a security attribute
        BOOL            bAllowMulti) PURE;  // IN - True if AllowMultiple=true


    // Call this after importing all other files and setting all options
    // Never call this for UBMs
    // This will create the actual manifest
    // After calling this if (*pdwReserveSize > 0) allocate space in the
    // PE image for the signature.  Pass the size and resulting RVA to ICeeFileGen::SetStrongNameEntry()
    // or set the appropriate entry in the COm+ header directly.
    STDMETHOD(EmitManifest)(
        mdAssembly      AssemblyID,         // IN - Unique ID for the assembly
        DWORD*          pdwReserveSize,     // OUT- Size to reserve in assembly file
        mdAssembly*     ptkManifest) PURE;  // OUT- (optional) Actualy assembly manifest token

    // Call this after closing all other files, but before closing assembly file
    // Never call this for UBMs
    STDMETHOD(PreCloseAssembly)(
        mdAssembly      AssemblyID) PURE;  // IN - Unique ID for the assembly

    // Call this after closing the assembly file
    // Call this to finish off a set of files and begin a new assembly or set of UBMs
    STDMETHOD(CloseAssembly)(
        mdAssembly      AssemblyID) PURE;   // IN - Unique ID for the assembly

    // New Custom Attributes have been merged into the emit scope
    // call this to have ALink 'read' them
    STDMETHOD(EndMerge)(
        mdAssembly      AssemblyID) PURE;   // IN - Unique ID for the assembly

    // Call this to pass flags into ALink when not creating an Assembly
    // (instead of calling SetAssemblyFile)
    STDMETHOD(SetNonAssemblyFlags)(
        AssemblyFlags   afFlags) PURE;      // IN - flags for ALink to generate a proper Assembly

    // DEPRECATED.
    // CAVEAT: Using pAssemblyScopeIn only works correctly for single file assemblies;
    //         for multi-file assemblies, pszFilename must exist on disk and be current.
    // Same as ImportFile but it is useful if you have an IMetaDataAssemblyImport but
    // pszFilename doesn't exist on disk.
    STDMETHOD(ImportFile2)(
        LPCWSTR         pszFilename,        // IN - filename of the file to add (must be fully qualified)
        LPCWSTR         pszTargetName,      // IN - (optional) filename to copy bound modules to.
        IMetaDataAssemblyImport* pAssemblyScopeIn, // IN - (optional) import scope for pszFilename
        BOOL            fSmartImport,       // IN - Set to TRUE if you want to use ImportTypes, etc.  FALSE means you'll do your own importing
        mdToken*        pImportToken,       // OUT- unique ID for the file (used to uniquely identify it) - it could be and assembly or file
        IMetaDataAssemblyImport**
                        ppAssemblyScope,    // OUT- assembly import scope (NULL if the file is not an assembly)
        DWORD*          pdwCountOfScopes) PURE;
                                            // OUT- Count of files/scopes imported due to this file

};

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI CreateALink(REFIID riid, IUnknown** ppInterface);
HINSTANCE WINAPI GetALinkMessageDll();

#ifdef __cplusplus
}
#endif

#endif


