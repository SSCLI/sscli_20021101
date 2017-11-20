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
// File: timerids.h
//
// Defined the timerids. Each timer id specifying a section of code
// that can be timed.
//
// The columns represent:
//   ID used to represent the code section programmatically
//   name used output the code section
//   subtotal id: all code section with the same subtotal id get a subtotal.
//                they must be adjacent to each-other.
// ===========================================================================


TIMERID(TIME_COMPILE,                           "Miscellaneous compilation tasks",          0)
TIMERID(TIME_COMPILELOOP,                       "Main compile loop",                        0)
TIMERID(TIME_EMITTYPEDEFS,                      "Emit typedefs",                            0)
TIMERID(TIME_EMITMEMBERDEFS,                    "Emit memberdefs",                          0)
TIMERID(TIME_COMPILENAMESPACE,                  "Compile namespace",                        0)
TIMERID(TIME_CREATETOKENSTREAM,                 "Create token stream",                      0)
TIMERID(TIME_SCANTOKEN,                         "Scan a token",                             0)
TIMERID(TIME_PARSETOPLEVEL,                     "Top-level parse",                          0)
TIMERID(TIME_PARSEINTERIOR,                     "Method body parse",                        0)
TIMERID(TIME_DECLARE,                           "Create namespace/type symbols",            0)
TIMERID(TIME_DEFINE,                            "Create member symbols",                    0)
TIMERID(TIME_PREPARE,                           "Validate member declarations",             0)
TIMERID(TIME_INCBUILD,                          "Incremental build processing",             0)
TIMERID(TIME_BINDMETHOD,                        "Method body bind and type-check",          0)
TIMERID(TIME_BINDPARTTWO,                       "Method body binding 2nd pass",             0)
TIMERID(TIME_ILGEN,                             "IL generation",                            0) 
TIMERID(TIME_ASSEMBLY,                          "Assembly/manifest/resource processing",    0)
TIMERID(TIME_ADDMODULE,                         "Adding a module to this assembly",         0)
TIMERID(TIME_COMPILEDOCCOMMENT,                 "Compile XML DocComments",                  0)

TIMERID(TIME_HOST_GETSOURCEMODULE,              "ISCCompilerHost::GetSourceModule",         1)
TIMERID(TIME_HOST_GETSOURCEDATA,                "ISCSourceModule::GetSourceData",           1)

TIMERID(TIME_IMD_OPENSCOPE,                     "IMetadataDispenser::OpenScope",            2)
TIMERID(TIME_IMI_COUNTENUM,                     "IMetadataImport::CountEnum",               2)
TIMERID(TIME_IMI_ENUMTYPEDEFS,                  "IMetadataImport::EnumTypeDefs",            2)
TIMERID(TIME_IMI_GETTYPEDEFPROPS,               "IMetadataImport::GetTypeDefProps",         2)
TIMERID(TIME_IMI_GETTYPEREFPROPS,               "IMetadataImport::GetTypeRefProps",         2)
TIMERID(TIME_IMI_GETINTERFACEIMPLPROPS,         "IMetadataImport::GetInterfaceImplProps",   2)
TIMERID(TIME_IMI_GETFIELDPROPS,                 "IMetadataImport::GetFieldProps",           2)
TIMERID(TIME_IMI_GETCUSTOMATTRIBUTEBYNAME,      "IMetadataImport::GetCustomAttributeByName",2)
TIMERID(TIME_IMI_GETPARAMFORMETHODINDEX,        "IMetadataImport::GetParamForMethodIndex",  2)
TIMERID(TIME_IMI_GETPARAMPROPS,                 "IMetadataImport::GetParamProps",           2)
TIMERID(TIME_IMI_GETMETHODPROPS,                "IMetadataImport::GetMethodProps",          2)
TIMERID(TIME_IMI_GETMEMBERREFPROPS,             "IMetadataImport::GetMemberRefProps",       2)
TIMERID(TIME_IMI_GETMODULEREFPROPS,             "IMetadataImport::GetModuleRefProps",       2)
TIMERID(TIME_IMI_GETPROPERTYPROPS,              "IMetadataImport::GetPropertyProps",        2)
TIMERID(TIME_IMI_ENUMINTERFACEIMPLS,            "IMetadataImport::EnumInterfaceImpls",      2)
TIMERID(TIME_IMI_ENUMFIELDS,                    "IMetadataImport::EnumFields",              2)
TIMERID(TIME_IMI_ENUMMETHODS,                   "IMetadataImport::EnumMethods",             2)
TIMERID(TIME_IMI_ENUMMETHODIMPLS,               "IMetadataImport::EnumMethodImpls",         2)
TIMERID(TIME_IMI_ENUMMODULEREFS,                "IMetadataImport::EnumModuleRefs",          2)
TIMERID(TIME_IMI_ENUMPROPERTIES,                "IMetadataImport::EnumProperties",          2)
TIMERID(TIME_IMI_ENUMTYPEREFS,                  "IMetadataImport::EnumTypeRefs",            2)
TIMERID(TIME_IMI_ENUMCUSTOMATTRIBUTES,          "IMetadataImport::EnumCustomAttributes",    2)
TIMERID(TIME_IMI_ENUMPERMISSIONSETS,            "IMetadataImport::EnumPermissionSets",      2)
TIMERID(TIME_IMI_GETCUSTOMATTRIBUTEPROPS,       "IMetadataImport::GetCustomAttributeProps", 2)
TIMERID(TIME_IMI_ENUMEVENTS,                    "IMetadataImport::EnumEvents",              2)
TIMERID(TIME_IMI_GETEVENTPROPS,                 "IMetadataImport::GetEventProps",           2)
TIMERID(TIME_IMI_GETNESTEDCLASSPROPS,           "IMetadataImport::GetNestedClassProps",     2)
TIMERID(TIME_IMAI_ENUMFILES,                    "IMetadataAssemblyImport::EnumFiles",       2)
TIMERID(TIME_IMAI_GETFILEPROPS,                 "IMetadataAssemblyImport::GetFileProps",    2)
TIMERID(TIME_IMAI_ENUMCOMTYPES,                 "IMetadataAssemblyImport::EnumComTypes",    2)
TIMERID(TIME_IMAI_GETCOMTYPEPROPS,              "IMetadataAssemblyImport::GetComTypeProps", 2)

TIMERID(TIME_IMD_DEFINESCOPE,                   "IMetaDataDispenser::DefineScope",          3)        
TIMERID(TIME_IME_GETTOKENFROMSIG,               "IMetaDataEmit::GetTokenFromSig",           3) 
TIMERID(TIME_IME_DEFINEMEMBERREF,               "IMetaDataEmit::DefineMemberRef",           3)
TIMERID(TIME_IME_DEFINEIMPORTMEMBER,            "IMetaDataEmit::DefineImportMember",        3)
TIMERID(TIME_IME_DEFINETYPEREFBYNAME,           "IMetaDataEmit::DefineTypeRefByName",       3)
TIMERID(TIME_IME_DEFINEUSERSTRING,              "IMetaDataEmit::DefineUserString",          3)
TIMERID(TIME_IME_DEFINETYPEDEF,                 "IMetaDataEmit::DefineTypeDef",             3)
TIMERID(TIME_IME_DEFINEFIELD,                   "IMetaDataEmit::DefineField",               3)
TIMERID(TIME_IME_SETTYPEDEFPROPS,               "IMetaDataEmit::SetTypeDefProps",           3)
TIMERID(TIME_IME_SETCLASSLAYOUT,                "IMetaDataEmit::SetClassLayout",            3)
TIMERID(TIME_IME_SETFIELDMARSHAL,               "IMetaDataEmit::SetFieldMarshal",           3)
TIMERID(TIME_IME_DEFINEPARAM,                   "IMetaDataEmit::DefineParam",               3)
TIMERID(TIME_IME_SETPARAMPROPS,                 "IMetaDataEmit::SetParamProps",             3)
TIMERID(TIME_IME_DEFINEMETHOD,                  "IMetaDataEmit::DefineMethod",              3)
TIMERID(TIME_IME_DEFINEMETHODIMPL,              "IMetaDataEmit::DefineMethodImpl",          3)
TIMERID(TIME_IME_DEFINEPINVOKEMAP,              "IMetaDataEmit::DefinePInvokeMap",          3)
TIMERID(TIME_IME_SETRVA,                        "IMetaDataEmit::SetRVA",                    3)
TIMERID(TIME_IME_SETFIELDRVA,                   "IMetaDataEmit::SetFieldRVA",               3)
TIMERID(TIME_IME_DEFINEPERMISSIONSET,           "IMetaDataEmit::DefinePermissionSet",       3)
TIMERID(TIME_IME_DEFINEMODULEREF,               "IMetaDataEmit::DefineModuleRef",           3)
TIMERID(TIME_IME_DEFINEPROPERTY,                "IMetaDataEmit::DefineProperty",            3)
TIMERID(TIME_IME_DEFINECUSTOMATTRIBUTE,         "IMetaDataEmit::DefineCustomAttribute",     3)
TIMERID(TIME_IME_DEFINESECURITYATTRIBUTESET,    "IMetaDataEmit::DefineSecurityAttributeSet",3)
TIMERID(TIME_IME_DELETETOKEN,                   "IMetaDataEmit::DeleteToken",               3)
TIMERID(TIME_IME_SETFIELDPROPS,                 "IMetaDataEmit::SetFieldProps",             3)
TIMERID(TIME_IME_SETPROPERTYPROPS,              "IMetaDataEmit::SetPropertyProps",          3)
TIMERID(TIME_IME_SETNESTEDCLASSPROPS,           "IMetaDataEmit::SetNestedClassProps",       3)
TIMERID(TIME_IME_DEFINENESTEDTYPE,              "IMetaDataEmit::DefineNestedType",          3)
TIMERID(TIME_IME_GETTOKENFROMTYPESPEC,          "IMetaDataEmit::GetTokenFromTypeSpec",      3)
TIMERID(TIME_IME_DEFINEEVENT,                   "IMetaDataEmit::DefineEvent",               3)
TIMERID(TIME_IME_TRANSLATESIGWITHSCOPE,         "IMetaDataEmit::TranslateSigWithScope",     3)
TIMERID(TIME_IME_SETMETHODIMPLFLAGS,            "IMetaDataEmit::SetMethodImplFlags",        3)
TIMERID(TIME_IME_SETEVENTPROPS,                 "IMetaDataEmit::SetEventProps",             3)
TIMERID(TIME_IME_SETMETHODPROPS,                "IMetaDataEmit::SetMethodProps",            3)
TIMERID(TIME_IME_DELETECLASSLAYOUT,             "IMetaDataEmit::DeleteClassLayout",         3)
TIMERID(TIME_IME_DELETEFIELDMARSHAL,            "IMetaDataEmit::DeleteFieldMarshal",        3)
TIMERID(TIME_IME_DELETEPINVOKEMAP,              "IMetaDataEmit::DeletePinvokeMap",          3)
TIMERID(TIME_IMDE_SETSYMBOLBINDINGKEY,          "IMetaDataDebugEmit::SetSymbolBindingKey",  3)
TIMERID(TIME_IMDE_DEFINESOURCEFILE,             "IMetaDataDebugEmit::DefineSourceFile",     3)
TIMERID(TIME_IMDE_DEFINEBLOCK,                  "IMetaDataDebugEmit::DefineBlock",          3)
TIMERID(TIME_IMDE_DEFINELOCALVARSCOPE,          "IMetaDataDebugEmit::DefineLocalVarScope",  3)
TIMERID(TIME_IMDE_DEFINELOCALVAR,               "IMetaDataDebugEmit::DefineLocalVar",       3)
TIMERID(TIME_IMAE_DEFINEASSEMBLY,               "IMetaDataAssemblyEmit::DefineAssembly",    3)
TIMERID(TIME_IMAE_DEFINECOMTYPE,                "IMetaDataAssemblyEmit::DefineComType",     3)
TIMERID(TIME_IMAE_DEFINEMANIFESTRESOURCE,       "IMetaDataAssemblyEmit::DefineManifestResource", 3)
TIMERID(TIME_IMAE_DEFINEFILE,                   "IMetaDataAssemblyEmit::DefineFile",        3)
TIMERID(TIME_IMAE_DEFINEASSEMBLYREF,            "IMetaDataAseemblyEmit::DefineAssemblyRef", 3)

TIMERID(TIME_ISW_DEFINEDOCUMENT,                "ISymUnmanagedWriter::DefineDocument",      4)
TIMERID(TIME_ISW_DEFINELOCALVAR,                "ISymUnmanagedWriter::DefineLocalVariable", 4)
TIMERID(TIME_ISW_DEFINECONSTANT,                "ISymUnmanagedWriter::DefineConstant",      4)
TIMERID(TIME_ISW_DEFINESEQUENCEPOINTS,          "ISymUnmanagedWriter::DefineSequencePoints",4)
TIMERID(TIME_ISW_OPENSCOPE,                     "ISymUnmanagedWriter::OpenScope"           ,4)
TIMERID(TIME_ISW_CLOSESCOPE,                    "ISymUnmanagedWriter::CloseScope"          ,4)
TIMERID(TIME_ISW_OPENMETHOD,                    "ISymUnmanagedWriter::OpenMethod"          ,4)
TIMERID(TIME_ISW_CLOSEMETHOD,                   "ISymUnmanagedWriter::CloseMethod"         ,4)
TIMERID(TIME_ISW_GETDEBUGCVINFO,                "ISymUnmanagedWriter::GetDebugCVInfo",      4)
TIMERID(TIME_ISW_SETUSERENTRYPOINT,             "ISymUnmanagedWriter::SetUserEntryPoint",   4)

TIMERID(TIME_CREATECEEFILEGEN,                  "CreateCeeFileGen",                         5)
TIMERID(TIME_DESTROYCEEFILEGEN,                 "DestroyCeeFileGen",                        5)
TIMERID(TIME_ICFG_CREATECEEFILE,                "ICeeFileGen::CreateCeeFile",               5)
TIMERID(TIME_ICFG_GETILSECTION,                 "ICeeFileGen::GetIlSection",                5)
TIMERID(TIME_ICFG_GETRDATASECTION,              "ICeeFileGen::GetRdataSection",             5)
TIMERID(TIME_ICFG_SETOUTPUTFILENAME,            "ICeeFileGen::SetOutputFileName",           5)
TIMERID(TIME_ICFG_SETCOMIMAGEFLAGS,             "ICeeFileGen::SetComImageFlags",            5)
TIMERID(TIME_ICFG_SETIMAGEBASE,                 "ICeeFileGen::SetImageBase",                5)
TIMERID(TIME_ICFG_SETFILEALIGNMENT,             "ICeeFileGen::SetFileAlignment",            5)
TIMERID(TIME_ICFG_SETRESOURCEFILENAME,          "ICeeFileGen::SetResourceFileName",         5)
TIMERID(TIME_ICFG_SETSUBSYSTEM,                 "ICeeFileGen::SetSubSystem",                5)
TIMERID(TIME_ICFG_EMITMETADATAEX,               "ICeeFileGen::EmitMetaDataEx",              5)
TIMERID(TIME_ICFG_GENERATECEEFILE,              "ICeeFileGen::GenerateCeeFile",             5)
TIMERID(TIME_ICFG_SETDLLSWITCH,                 "ICeeFileGen::SetDllSwitch",                5)
TIMERID(TIME_ICFG_SETENTRYPOINT,                "ICeeFileGen::SetEntryPoint",               5)
TIMERID(TIME_ICFG_GETSECTIONBLOCK,              "ICeeFileGen::GetSectionBlock",             5)
TIMERID(TIME_ICFG_GETSECTIONDATALEN,            "ICeeFileGen::GetSectionDataLen",           5)
TIMERID(TIME_ICFG_GETMETHODRVA,                 "ICeeFileGen::GetMethodRVA",                5)
TIMERID(TIME_ICFG_ADDSECTIONRELOC,              "ICeeFileGen::AddSectionReloc",             5)
TIMERID(TIME_ICFG_SETMANIFESTENTRY,             "ICeeFileGen::SetManifestEntry",            5)
TIMERID(TIME_ICFG_SETDIRECTORYENTRY,            "ICeeFileGen::SetDirectoryEntry",           5)
TIMERID(TIME_ICFG_SETSTRONGNAMEENTRY,           "ICeeFileGen::SetStrongNameEntry",          5)
TIMERID(TIME_ICFG_COMPUTESECTIONPOINTER,        "ICeeFileGen::ComputeSectionPointer",       5)

TIMERID(TIME_SN_FREEBUFFER,                     "StrongNameFreeBuffer",                     6)
TIMERID(TIME_SN_GETPUBLICKEY,                   "StrongNameGetPublicKey",                   6)
TIMERID(TIME_SN_SIGNATUREGENERATION,            "StrongNameSignatureGeneration",            6)
TIMERID(TIME_SN_KEYINSTALL,                     "StrongNameKeyInstall",                     6)










