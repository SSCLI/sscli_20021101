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

#if defined(_MSC_VER)
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#endif  // defined(_MSC_VER)

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Fri Nov 30 01:13:15 2001
 */
/* Compiler settings for scciface.idl:
    Oicf, W0, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __scciface_h__
#define __scciface_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICSError_FWD_DEFINED__
#define __ICSError_FWD_DEFINED__
typedef interface ICSError ICSError;
#endif 	/* __ICSError_FWD_DEFINED__ */


#ifndef __ICSErrorContainer_FWD_DEFINED__
#define __ICSErrorContainer_FWD_DEFINED__
typedef interface ICSErrorContainer ICSErrorContainer;
#endif 	/* __ICSErrorContainer_FWD_DEFINED__ */


#ifndef __VCPropertyContainer_FWD_DEFINED__
#define __VCPropertyContainer_FWD_DEFINED__

#ifdef __cplusplus
typedef class VCPropertyContainer VCPropertyContainer;
#else
typedef struct VCPropertyContainer VCPropertyContainer;
#endif /* __cplusplus */

#endif 	/* __VCPropertyContainer_FWD_DEFINED__ */



#ifndef __ICSSourceText_FWD_DEFINED__
#define __ICSSourceText_FWD_DEFINED__
typedef interface ICSSourceText ICSSourceText;
#endif 	/* __ICSSourceText_FWD_DEFINED__ */


#ifndef __ICSCompilerHost_FWD_DEFINED__
#define __ICSCompilerHost_FWD_DEFINED__
typedef interface ICSCompilerHost ICSCompilerHost;
#endif 	/* __ICSCompilerHost_FWD_DEFINED__ */


#ifndef __ICSCommandLineCompilerHost_FWD_DEFINED__
#define __ICSCommandLineCompilerHost_FWD_DEFINED__
typedef interface ICSCommandLineCompilerHost ICSCommandLineCompilerHost;
#endif 	/* __ICSCommandLineCompilerHost_FWD_DEFINED__ */


#ifndef __ICSInteriorTree_FWD_DEFINED__
#define __ICSInteriorTree_FWD_DEFINED__
typedef interface ICSInteriorTree ICSInteriorTree;
#endif 	/* __ICSInteriorTree_FWD_DEFINED__ */


#ifndef __ICSSourceData_FWD_DEFINED__
#define __ICSSourceData_FWD_DEFINED__
typedef interface ICSSourceData ICSSourceData;
#endif 	/* __ICSSourceData_FWD_DEFINED__ */


#ifndef __ICSSourceModuleEvents_FWD_DEFINED__
#define __ICSSourceModuleEvents_FWD_DEFINED__
typedef interface ICSSourceModuleEvents ICSSourceModuleEvents;
#endif 	/* __ICSSourceModuleEvents_FWD_DEFINED__ */


#ifndef __ICSSourceModule_FWD_DEFINED__
#define __ICSSourceModule_FWD_DEFINED__
typedef interface ICSSourceModule ICSSourceModule;
#endif 	/* __ICSSourceModule_FWD_DEFINED__ */


#ifndef __ICSNameTable_FWD_DEFINED__
#define __ICSNameTable_FWD_DEFINED__
typedef interface ICSNameTable ICSNameTable;
#endif 	/* __ICSNameTable_FWD_DEFINED__ */


#ifndef __ICSLexer_FWD_DEFINED__
#define __ICSLexer_FWD_DEFINED__
typedef interface ICSLexer ICSLexer;
#endif 	/* __ICSLexer_FWD_DEFINED__ */


#ifndef __ICSParser_FWD_DEFINED__
#define __ICSParser_FWD_DEFINED__
typedef interface ICSParser ICSParser;
#endif 	/* __ICSParser_FWD_DEFINED__ */


#ifndef __ICSCompilerConfig_FWD_DEFINED__
#define __ICSCompilerConfig_FWD_DEFINED__
typedef interface ICSCompilerConfig ICSCompilerConfig;
#endif 	/* __ICSCompilerConfig_FWD_DEFINED__ */


#ifndef __ICSInputSet_FWD_DEFINED__
#define __ICSInputSet_FWD_DEFINED__
typedef interface ICSInputSet ICSInputSet;
#endif 	/* __ICSInputSet_FWD_DEFINED__ */


#ifndef __ICSCompileProgress_FWD_DEFINED__
#define __ICSCompileProgress_FWD_DEFINED__
typedef interface ICSCompileProgress ICSCompileProgress;
#endif 	/* __ICSCompileProgress_FWD_DEFINED__ */


#ifndef __ICSCompiler_FWD_DEFINED__
#define __ICSCompiler_FWD_DEFINED__
typedef interface ICSCompiler ICSCompiler;
#endif 	/* __ICSCompiler_FWD_DEFINED__ */


#ifndef __ICSCompilerFactory_FWD_DEFINED__
#define __ICSCompilerFactory_FWD_DEFINED__
typedef interface ICSCompilerFactory ICSCompilerFactory;
#endif 	/* __ICSCompilerFactory_FWD_DEFINED__ */


#ifndef __ICSharpProjectSite_FWD_DEFINED__
#define __ICSharpProjectSite_FWD_DEFINED__
typedef interface ICSharpProjectSite ICSharpProjectSite;
#endif 	/* __ICSharpProjectSite_FWD_DEFINED__ */


#ifndef __ICSharpProjectRoot_FWD_DEFINED__
#define __ICSharpProjectRoot_FWD_DEFINED__
typedef interface ICSharpProjectRoot ICSharpProjectRoot;
#endif 	/* __ICSharpProjectRoot_FWD_DEFINED__ */


#ifndef __ICSharpBuildMgrSite_FWD_DEFINED__
#define __ICSharpBuildMgrSite_FWD_DEFINED__
typedef interface ICSharpBuildMgrSite ICSharpBuildMgrSite;
#endif 	/* __ICSharpBuildMgrSite_FWD_DEFINED__ */


#ifndef __ICSharpProjectHost_FWD_DEFINED__
#define __ICSharpProjectHost_FWD_DEFINED__
typedef interface ICSharpProjectHost ICSharpProjectHost;
#endif 	/* __ICSharpProjectHost_FWD_DEFINED__ */


#ifndef __CSharpProjectConfigProperties_FWD_DEFINED__
#define __CSharpProjectConfigProperties_FWD_DEFINED__
typedef interface CSharpProjectConfigProperties CSharpProjectConfigProperties;
#endif 	/* __CSharpProjectConfigProperties_FWD_DEFINED__ */


#ifndef __CSharpProjectProperties_FWD_DEFINED__
#define __CSharpProjectProperties_FWD_DEFINED__
typedef interface CSharpProjectProperties CSharpProjectProperties;
#endif 	/* __CSharpProjectProperties_FWD_DEFINED__ */


#ifndef __ICSharpGeneralPropPage_FWD_DEFINED__
#define __ICSharpGeneralPropPage_FWD_DEFINED__
typedef interface ICSharpGeneralPropPage ICSharpGeneralPropPage;
#endif 	/* __ICSharpGeneralPropPage_FWD_DEFINED__ */


#ifndef __ICSharpWebSettingsPropPage_FWD_DEFINED__
#define __ICSharpWebSettingsPropPage_FWD_DEFINED__
typedef interface ICSharpWebSettingsPropPage ICSharpWebSettingsPropPage;
#endif 	/* __ICSharpWebSettingsPropPage_FWD_DEFINED__ */


#ifndef __ICSharpDesignerPropPage_FWD_DEFINED__
#define __ICSharpDesignerPropPage_FWD_DEFINED__
typedef interface ICSharpDesignerPropPage ICSharpDesignerPropPage;
#endif 	/* __ICSharpDesignerPropPage_FWD_DEFINED__ */


#ifndef __ICSharpCommonBuildPropPage_FWD_DEFINED__
#define __ICSharpCommonBuildPropPage_FWD_DEFINED__
typedef interface ICSharpCommonBuildPropPage ICSharpCommonBuildPropPage;
#endif 	/* __ICSharpCommonBuildPropPage_FWD_DEFINED__ */


#ifndef __ICSharpBuildPropPage_FWD_DEFINED__
#define __ICSharpBuildPropPage_FWD_DEFINED__
typedef interface ICSharpBuildPropPage ICSharpBuildPropPage;
#endif 	/* __ICSharpBuildPropPage_FWD_DEFINED__ */


#ifndef __ICSharpAdvancedPropPage_FWD_DEFINED__
#define __ICSharpAdvancedPropPage_FWD_DEFINED__
typedef interface ICSharpAdvancedPropPage ICSharpAdvancedPropPage;
#endif 	/* __ICSharpAdvancedPropPage_FWD_DEFINED__ */


#ifndef __ICSharpDebugPropPage_FWD_DEFINED__
#define __ICSharpDebugPropPage_FWD_DEFINED__
typedef interface ICSharpDebugPropPage ICSharpDebugPropPage;
#endif 	/* __ICSharpDebugPropPage_FWD_DEFINED__ */


#ifndef __CSharpGeneral_FWD_DEFINED__
#define __CSharpGeneral_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpGeneral CSharpGeneral;
#else
typedef struct CSharpGeneral CSharpGeneral;
#endif /* __cplusplus */

#endif 	/* __CSharpGeneral_FWD_DEFINED__ */


#ifndef __CSharpVersion_FWD_DEFINED__
#define __CSharpVersion_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpVersion CSharpVersion;
#else
typedef struct CSharpVersion CSharpVersion;
#endif /* __cplusplus */

#endif 	/* __CSharpVersion_FWD_DEFINED__ */


#ifndef __CSharpWebSettings_FWD_DEFINED__
#define __CSharpWebSettings_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpWebSettings CSharpWebSettings;
#else
typedef struct CSharpWebSettings CSharpWebSettings;
#endif /* __cplusplus */

#endif 	/* __CSharpWebSettings_FWD_DEFINED__ */


#ifndef __CSharpDesigner_FWD_DEFINED__
#define __CSharpDesigner_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpDesigner CSharpDesigner;
#else
typedef struct CSharpDesigner CSharpDesigner;
#endif /* __cplusplus */

#endif 	/* __CSharpDesigner_FWD_DEFINED__ */


#ifndef __CSharpCommonBuild_FWD_DEFINED__
#define __CSharpCommonBuild_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpCommonBuild CSharpCommonBuild;
#else
typedef struct CSharpCommonBuild CSharpCommonBuild;
#endif /* __cplusplus */

#endif 	/* __CSharpCommonBuild_FWD_DEFINED__ */


#ifndef __CSharpBuild_FWD_DEFINED__
#define __CSharpBuild_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpBuild CSharpBuild;
#else
typedef struct CSharpBuild CSharpBuild;
#endif /* __cplusplus */

#endif 	/* __CSharpBuild_FWD_DEFINED__ */


#ifndef __CSharpAdvanced_FWD_DEFINED__
#define __CSharpAdvanced_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpAdvanced CSharpAdvanced;
#else
typedef struct CSharpAdvanced CSharpAdvanced;
#endif /* __cplusplus */

#endif 	/* __CSharpAdvanced_FWD_DEFINED__ */


#ifndef __CSharpDebug_FWD_DEFINED__
#define __CSharpDebug_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSharpDebug CSharpDebug;
#else
typedef struct CSharpDebug CSharpDebug;
#endif /* __cplusplus */

#endif 	/* __CSharpDebug_FWD_DEFINED__ */


#ifndef __ICSharpEventsRoot_FWD_DEFINED__
#define __ICSharpEventsRoot_FWD_DEFINED__
typedef interface ICSharpEventsRoot ICSharpEventsRoot;
#endif 	/* __ICSharpEventsRoot_FWD_DEFINED__ */


#ifndef __CSCompilerFactory_FWD_DEFINED__
#define __CSCompilerFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class CSCompilerFactory CSCompilerFactory;
#else
typedef struct CSCompilerFactory CSCompilerFactory;
#endif /* __cplusplus */

#endif 	/* __CSCompilerFactory_FWD_DEFINED__ */


#ifndef __ICSharpSettingsPage_FWD_DEFINED__
#define __ICSharpSettingsPage_FWD_DEFINED__
typedef interface ICSharpSettingsPage ICSharpSettingsPage;
#endif 	/* __ICSharpSettingsPage_FWD_DEFINED__ */


#ifndef __ICSharpPropertyContainer_FWD_DEFINED__
#define __ICSharpPropertyContainer_FWD_DEFINED__
typedef interface ICSharpPropertyContainer ICSharpPropertyContainer;
#endif 	/* __ICSharpPropertyContainer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_scciface_0000 */
/* [local] */ 


#if defined(_MSC_VER)
#ifdef SCCOMP_EXPORTS
#define SCCOMP_API __declspec(dllexport)
#else
#define SCCOMP_API __declspec(dllimport)
#endif
#else   // defined(_MSC_VER)
#define SCCOMP_API
#endif  // defined(_MSC_VER)


#define MIDL_POSDATA POSDATA
struct POSDATA;
#define MIDL_BASENODE BASENODE
struct BASENODE;
#define MIDL_TOKENDATA TOKENDATA
struct TOKENDATA;
#define MIDL_NAME NAME
struct NAME;
#define MIDL_LEXDATA LEXDATA
struct LEXDATA;
typedef 
enum _ERRORKIND
    {	ERROR_FATAL	= 0,
	ERROR_ERROR	= ERROR_FATAL + 1,
	ERROR_WARNING	= ERROR_ERROR + 1
    } 	ERRORKIND;

typedef 
enum _ERRORCATEGORY
    {	EC_TOKENIZATION	= 0,
	EC_TOPLEVELPARSE	= EC_TOKENIZATION + 1,
	EC_METHODPARSE	= EC_TOPLEVELPARSE + 1,
	EC_COMPILATION	= EC_METHODPARSE + 1
    } 	ERRORCATEGORY;

typedef 
enum _CompilerOptions
    {	OPTID_WARNINGLEVEL	= 1,
	OPTID_WARNINGSAREERRORS	= OPTID_WARNINGLEVEL + 1,
	OPTID_CCSYMBOLS	= OPTID_WARNINGSAREERRORS + 1,
	OPTID_NOSTDLIB	= OPTID_CCSYMBOLS + 1,
	OPTID_EMITDEBUGINFO	= OPTID_NOSTDLIB + 1,
	OPTID_OPTIMIZATIONS	= OPTID_EMITDEBUGINFO + 1,
	OPTID_IMPORTS	= OPTID_OPTIMIZATIONS + 1,
	OPTID_INTERNALTESTS	= 8,
	OPTID_NOCODEGEN	= 14,
	OPTID_TIMING	= 15,
	OPTID_INCBUILD	= 17,
	OPTID_MODULES	= 18,
	OPTID_NOWARNLIST	= 20,
	OPTID_XML_DOCFILE	= 24,
	OPTID_CHECKED	= OPTID_XML_DOCFILE + 1,
	OPTID_UNSAFE	= OPTID_CHECKED + 1,
	OPTID_DEBUGTYPE	= OPTID_UNSAFE + 1,
	OPTID_LIBPATH	= OPTID_DEBUGTYPE + 1,
	LARGEST_OPTION_ID	= OPTID_LIBPATH + 1
    } 	CompilerOptions;

typedef 
enum WarningLevel
    {	WarningLevel_One	= 1,
	WarningLevel_Two	= WarningLevel_One + 1,
	WarningLevel_Three	= WarningLevel_Two + 1,
	WarningLevel_Four	= WarningLevel_Three + 1
    } 	WarningLevel;

typedef 
enum _OutputFileTypes
    {	OUTPUT_CONSOLE	= 0,
	OUTPUT_WINDOWS	= OUTPUT_CONSOLE + 1,
	OUTPUT_LIBRARY	= OUTPUT_WINDOWS + 1,
	OUTPUT_MODULE	= OUTPUT_LIBRARY + 1
    } 	OutputFileTypes;

typedef 
enum _CompilerOptionFlags
    {	COF_BOOLEAN	= 0x1,
	COF_HIDDEN	= 0x2,
	COF_DEFAULTON	= 0x4,
	COF_HASDEFAULT	= 0x4,
	COF_WARNONOLDUSE	= 0x8,
	COF_GRP_OUTPUT	= 0x1000,
	COF_GRP_INPUT	= 0x2000,
	COF_GRP_RES	= 0x3000,
	COF_GRP_CODE	= 0x4000,
	COF_GRP_ERRORS	= 0x5000,
	COF_GRP_LANGUAGE	= 0x6000,
	COF_GRP_MISC	= 0x7000,
	COF_GRP_ADVANCED	= 0x8000,
	COF_GRP_MASK	= 0xff000,
	COF_ARG_NONE	= 0,
	COF_ARG_FILELIST	= 0x100000,
	COF_ARG_FILE	= 0x200000,
	COF_ARG_SYMLIST	= 0x300000,
	COF_ARG_WILDCARD	= 0x400000,
	COF_ARG_TYPE	= 0x500000,
	COF_ARG_RESINFO	= 0x600000,
	COF_ARG_WARNLIST	= 0x700000,
	COF_ARG_ADDR	= 0x800000,
	COF_ARG_NUMBER	= 0x900000,
	COF_ARG_DEBUGTYPE	= 0xa00000,
	COF_ARG_NOCOLON	= 0x8000000,
	COF_ARG_MASK	= 0x7f00000
    } 	CompilerOptionFlags;

typedef 
enum _CompilerCreationFlags
    {	CCF_KEEPIDENTTABLES	= 0x1,
	CCF_KEEPNODETABLES	= 0x2,
	CCF_TRACKCOMMENTS	= 0x4,
	CCF_IDEUSAGE	= CCF_KEEPIDENTTABLES | CCF_KEEPNODETABLES | CCF_TRACKCOMMENTS
    } 	CompilerCreationFlags;

typedef 
enum _ParseTreeScope
    {	PTS_NAMESPACEBODY	= 0,
	PTS_TYPEBODY	= PTS_NAMESPACEBODY + 1,
	PTS_ENUMBODY	= PTS_TYPEBODY + 1,
	PTS_STATEMENT	= PTS_ENUMBODY + 1,
	PTS_EXPRESION	= PTS_STATEMENT + 1,
	PTS_TYPE	= PTS_EXPRESION + 1,
	PTS_MEMBER	= PTS_TYPE + 1,
	PTS_PARAMETER	= PTS_MEMBER + 1
    } 	ParseTreeScope;

typedef 
enum _ExtentFlags
    {	EF_ALL	= 0,
	EF_SINGLESTMT	= EF_ALL + 1
    } 	ExtentFlags;



































enum __MIDL___MIDL_itf_scciface_0000_0001
    {	DISPID_LangProp_ExeName	= 1000,
	DISPID_LangProp_CompilerOptions	= DISPID_LangProp_ExeName + 1,
	DISPID_LangProp_OutputType	= DISPID_LangProp_CompilerOptions + 1,
	DISPID_LangProp_OutputPath	= DISPID_LangProp_OutputType + 1,
	DISPID_LangProp_CSharpStartMode	= DISPID_LangProp_OutputPath + 1,
	DISPID_LangProp_StartApp	= DISPID_LangProp_CSharpStartMode + 1,
	DISPID_LangProp_StartAppPath	= DISPID_LangProp_StartApp + 1,
	DISPID_LangProp_CustomStartupArguments	= DISPID_LangProp_StartAppPath + 1,
	DISPID_LangProp_StartupURL	= DISPID_LangProp_CustomStartupArguments + 1,
	DISPID_LangProp_EnableASPDebugging	= DISPID_LangProp_StartupURL + 1,
	DISPID_LangProp_EnableASPXDebugging	= DISPID_LangProp_EnableASPDebugging + 1,
	DISPID_LangProp_ProjectTitle	= DISPID_LangProp_EnableASPXDebugging + 1,
	DISPID_LangProp_HelpFile	= DISPID_LangProp_ProjectTitle + 1,
	DISPID_LangProp_WebServer	= DISPID_LangProp_HelpFile + 1,
	DISPID_LangProp_ProjectName	= DISPID_LangProp_WebServer + 1,
	DISPID_LangProp_ProjectLocation	= DISPID_LangProp_ProjectName + 1,
	DISPID_LangProp_ReferencesPath	= DISPID_LangProp_ProjectLocation + 1
    } ;
typedef 
enum StartMode
    {	StartMode_Project	= 0,
	StartMode_Application	= StartMode_Project + 1,
	StartMode_URL	= StartMode_Application + 1
    } 	StartMode;


enum StartMode_Max
    {	StartMode_Max	= StartMode_URL
    } ;
typedef struct _KEYEDNODE
    {
    MIDL_NAME *pKey;
    MIDL_BASENODE *pNode;
    } 	KEYEDNODE;



extern RPC_IF_HANDLE __MIDL_itf_scciface_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_scciface_0000_v0_0_s_ifspec;


#ifndef __CSharp_LIBRARY_DEFINED__
#define __CSharp_LIBRARY_DEFINED__

/* library CSharp */
/* [helpstringdll][helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CSharp;

#ifndef __ICSError_INTERFACE_DEFINED__
#define __ICSError_INTERFACE_DEFINED__

/* interface ICSError */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSError;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4CEEE5D4-FBB0-42ac-B558-5D764C8240C7")
    ICSError : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetErrorInfo( 
            long *piErrorID,
            ERRORKIND *pKind,
            LPCWSTR *ppszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocationCount( 
            long *piCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocationAt( 
            long iIndex,
            LPCWSTR *ppszFileName,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUnmappedLocationAt( 
            long iIndex,
            LPCWSTR *ppszFileName,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSErrorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSError * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSError * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSError * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorInfo )( 
            ICSError * This,
            long *piErrorID,
            ERRORKIND *pKind,
            LPCWSTR *ppszText);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocationCount )( 
            ICSError * This,
            long *piCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocationAt )( 
            ICSError * This,
            long iIndex,
            LPCWSTR *ppszFileName,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetUnmappedLocationAt )( 
            ICSError * This,
            long iIndex,
            LPCWSTR *ppszFileName,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd);
        
        END_INTERFACE
    } ICSErrorVtbl;

    interface ICSError
    {
        CONST_VTBL struct ICSErrorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSError_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSError_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSError_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSError_GetErrorInfo(This,piErrorID,pKind,ppszText)	\
    (This)->lpVtbl -> GetErrorInfo(This,piErrorID,pKind,ppszText)

#define ICSError_GetLocationCount(This,piCount)	\
    (This)->lpVtbl -> GetLocationCount(This,piCount)

#define ICSError_GetLocationAt(This,iIndex,ppszFileName,pposStart,pposEnd)	\
    (This)->lpVtbl -> GetLocationAt(This,iIndex,ppszFileName,pposStart,pposEnd)

#define ICSError_GetUnmappedLocationAt(This,iIndex,ppszFileName,pposStart,pposEnd)	\
    (This)->lpVtbl -> GetUnmappedLocationAt(This,iIndex,ppszFileName,pposStart,pposEnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSError_GetErrorInfo_Proxy( 
    ICSError * This,
    long *piErrorID,
    ERRORKIND *pKind,
    LPCWSTR *ppszText);


void __RPC_STUB ICSError_GetErrorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSError_GetLocationCount_Proxy( 
    ICSError * This,
    long *piCount);


void __RPC_STUB ICSError_GetLocationCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSError_GetLocationAt_Proxy( 
    ICSError * This,
    long iIndex,
    LPCWSTR *ppszFileName,
    MIDL_POSDATA *pposStart,
    MIDL_POSDATA *pposEnd);


void __RPC_STUB ICSError_GetLocationAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSError_GetUnmappedLocationAt_Proxy( 
    ICSError * This,
    long iIndex,
    LPCWSTR *ppszFileName,
    MIDL_POSDATA *pposStart,
    MIDL_POSDATA *pposEnd);


void __RPC_STUB ICSError_GetUnmappedLocationAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSError_INTERFACE_DEFINED__ */


#ifndef __ICSErrorContainer_INTERFACE_DEFINED__
#define __ICSErrorContainer_INTERFACE_DEFINED__

/* interface ICSErrorContainer */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSErrorContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("376A7828-DB0D-4cfd-956E-5143F71F5CCD")
    ICSErrorContainer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContainerInfo( 
            ERRORCATEGORY *pCategory,
            DWORD_PTR *pdwID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorCount( 
            long *piWarnings,
            long *piErrors,
            long *piFatals,
            long *piTotal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorAt( 
            long iIndex,
            ICSError **ppError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSErrorContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSErrorContainer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSErrorContainer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSErrorContainer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContainerInfo )( 
            ICSErrorContainer * This,
            ERRORCATEGORY *pCategory,
            DWORD_PTR *pdwID);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorCount )( 
            ICSErrorContainer * This,
            long *piWarnings,
            long *piErrors,
            long *piFatals,
            long *piTotal);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorAt )( 
            ICSErrorContainer * This,
            long iIndex,
            ICSError **ppError);
        
        END_INTERFACE
    } ICSErrorContainerVtbl;

    interface ICSErrorContainer
    {
        CONST_VTBL struct ICSErrorContainerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSErrorContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSErrorContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSErrorContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSErrorContainer_GetContainerInfo(This,pCategory,pdwID)	\
    (This)->lpVtbl -> GetContainerInfo(This,pCategory,pdwID)

#define ICSErrorContainer_GetErrorCount(This,piWarnings,piErrors,piFatals,piTotal)	\
    (This)->lpVtbl -> GetErrorCount(This,piWarnings,piErrors,piFatals,piTotal)

#define ICSErrorContainer_GetErrorAt(This,iIndex,ppError)	\
    (This)->lpVtbl -> GetErrorAt(This,iIndex,ppError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSErrorContainer_GetContainerInfo_Proxy( 
    ICSErrorContainer * This,
    ERRORCATEGORY *pCategory,
    DWORD_PTR *pdwID);


void __RPC_STUB ICSErrorContainer_GetContainerInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSErrorContainer_GetErrorCount_Proxy( 
    ICSErrorContainer * This,
    long *piWarnings,
    long *piErrors,
    long *piFatals,
    long *piTotal);


void __RPC_STUB ICSErrorContainer_GetErrorCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSErrorContainer_GetErrorAt_Proxy( 
    ICSErrorContainer * This,
    long iIndex,
    ICSError **ppError);


void __RPC_STUB ICSErrorContainer_GetErrorAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSErrorContainer_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_VCPropertyContainer;

#ifdef __cplusplus

class DECLSPEC_UUID("A54AAE97-30C2-11D3-87BF-A04A4CC10000")
VCPropertyContainer;
#endif


#ifndef __ICSSourceText_INTERFACE_DEFINED__
#define __ICSSourceText_INTERFACE_DEFINED__

/* interface ICSSourceText */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSSourceText;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E3783F08-E098-11d2-B554-00C04F68D4DB")
    ICSSourceText : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            LPCWSTR *ppszText,
            MIDL_POSDATA *pposEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            LPCWSTR *ppszText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSSourceTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSSourceText * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSSourceText * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSSourceText * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ICSSourceText * This,
            LPCWSTR *ppszText,
            MIDL_POSDATA *pposEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            ICSSourceText * This,
            LPCWSTR *ppszText);
        
        
        END_INTERFACE
    } ICSSourceTextVtbl;

    interface ICSSourceText
    {
        CONST_VTBL struct ICSSourceTextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSSourceText_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSSourceText_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSSourceText_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSSourceText_GetText(This,ppszText,pposEnd)	\
    (This)->lpVtbl -> GetText(This,ppszText,pposEnd)

#define ICSSourceText_GetName(This,ppszText)	\
    (This)->lpVtbl -> GetName(This,ppszText)

#define ICSSourceText_AdviseChangeEvents(This,pSink,pdwCookie)	\
    (This)->lpVtbl -> AdviseChangeEvents(This,pSink,pdwCookie)

#define ICSSourceText_UnadviseChangeEvents(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseChangeEvents(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSSourceText_GetText_Proxy( 
    ICSSourceText * This,
    LPCWSTR *ppszText,
    MIDL_POSDATA *pposEnd);


void __RPC_STUB ICSSourceText_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceText_GetName_Proxy( 
    ICSSourceText * This,
    LPCWSTR *ppszText);


void __RPC_STUB ICSSourceText_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);




#endif 	/* __ICSSourceText_INTERFACE_DEFINED__ */


#ifndef __ICSCompilerHost_INTERFACE_DEFINED__
#define __ICSCompilerHost_INTERFACE_DEFINED__

/* interface ICSCompilerHost */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSCompilerHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D2686AF8-E098-11d2-B554-00C04F68D4DB")
    ICSCompilerHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportErrors( 
            ICSErrorContainer *pErrors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceModule( 
            LPCWSTR pszFileName,
            ICSSourceModule **ppModule) = 0;
        
        virtual void STDMETHODCALLTYPE OnConfigChange( 
            long iOptionID,
            VARIANT varNewValue) = 0;
        
        virtual void STDMETHODCALLTYPE OnCatastrophicError( 
            BOOL fException,
            DWORD dwException,
            void *pAddress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSCompilerHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSCompilerHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSCompilerHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSCompilerHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReportErrors )( 
            ICSCompilerHost * This,
            ICSErrorContainer *pErrors);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceModule )( 
            ICSCompilerHost * This,
            LPCWSTR pszFileName,
            ICSSourceModule **ppModule);
        
        void ( STDMETHODCALLTYPE *OnConfigChange )( 
            ICSCompilerHost * This,
            long iOptionID,
            VARIANT varNewValue);
        
        void ( STDMETHODCALLTYPE *OnCatastrophicError )( 
            ICSCompilerHost * This,
            BOOL fException,
            DWORD dwException,
            void *pAddress);
        
        END_INTERFACE
    } ICSCompilerHostVtbl;

    interface ICSCompilerHost
    {
        CONST_VTBL struct ICSCompilerHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSCompilerHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSCompilerHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSCompilerHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSCompilerHost_ReportErrors(This,pErrors)	\
    (This)->lpVtbl -> ReportErrors(This,pErrors)

#define ICSCompilerHost_GetSourceModule(This,pszFileName,ppModule)	\
    (This)->lpVtbl -> GetSourceModule(This,pszFileName,ppModule)

#define ICSCompilerHost_OnConfigChange(This,iOptionID,varNewValue)	\
    (This)->lpVtbl -> OnConfigChange(This,iOptionID,varNewValue)

#define ICSCompilerHost_OnCatastrophicError(This,fException,dwException,pAddress)	\
    (This)->lpVtbl -> OnCatastrophicError(This,fException,dwException,pAddress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSCompilerHost_ReportErrors_Proxy( 
    ICSCompilerHost * This,
    ICSErrorContainer *pErrors);


void __RPC_STUB ICSCompilerHost_ReportErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerHost_GetSourceModule_Proxy( 
    ICSCompilerHost * This,
    LPCWSTR pszFileName,
    ICSSourceModule **ppModule);


void __RPC_STUB ICSCompilerHost_GetSourceModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSCompilerHost_OnConfigChange_Proxy( 
    ICSCompilerHost * This,
    long iOptionID,
    VARIANT varNewValue);


void __RPC_STUB ICSCompilerHost_OnConfigChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSCompilerHost_OnCatastrophicError_Proxy( 
    ICSCompilerHost * This,
    BOOL fException,
    DWORD dwException,
    void *pAddress);


void __RPC_STUB ICSCompilerHost_OnCatastrophicError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSCompilerHost_INTERFACE_DEFINED__ */


#ifndef __ICSCommandLineCompilerHost_INTERFACE_DEFINED__
#define __ICSCommandLineCompilerHost_INTERFACE_DEFINED__

/* interface ICSCommandLineCompilerHost */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSCommandLineCompilerHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BD7CF4EF-1821-4719-AA60-DF81B9EAFA01")
    ICSCommandLineCompilerHost : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE NotifyBinaryFile( 
            LPCWSTR pszFileName) = 0;
        
        virtual void STDMETHODCALLTYPE NotifyMetadataFile( 
            LPCWSTR pszFileName,
            LPCWSTR pszCorSystemDirectory) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSCommandLineCompilerHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSCommandLineCompilerHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSCommandLineCompilerHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSCommandLineCompilerHost * This);
        
        void ( STDMETHODCALLTYPE *NotifyBinaryFile )( 
            ICSCommandLineCompilerHost * This,
            LPCWSTR pszFileName);
        
        void ( STDMETHODCALLTYPE *NotifyMetadataFile )( 
            ICSCommandLineCompilerHost * This,
            LPCWSTR pszFileName,
            LPCWSTR pszCorSystemDirectory);
        
        END_INTERFACE
    } ICSCommandLineCompilerHostVtbl;

    interface ICSCommandLineCompilerHost
    {
        CONST_VTBL struct ICSCommandLineCompilerHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSCommandLineCompilerHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSCommandLineCompilerHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSCommandLineCompilerHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSCommandLineCompilerHost_NotifyBinaryFile(This,pszFileName)	\
    (This)->lpVtbl -> NotifyBinaryFile(This,pszFileName)

#define ICSCommandLineCompilerHost_NotifyMetadataFile(This,pszFileName,pszCorSystemDirectory)	\
    (This)->lpVtbl -> NotifyMetadataFile(This,pszFileName,pszCorSystemDirectory)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE ICSCommandLineCompilerHost_NotifyBinaryFile_Proxy( 
    ICSCommandLineCompilerHost * This,
    LPCWSTR pszFileName);


void __RPC_STUB ICSCommandLineCompilerHost_NotifyBinaryFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSCommandLineCompilerHost_NotifyMetadataFile_Proxy( 
    ICSCommandLineCompilerHost * This,
    LPCWSTR pszFileName,
    LPCWSTR pszCorSystemDirectory);


void __RPC_STUB ICSCommandLineCompilerHost_NotifyMetadataFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSCommandLineCompilerHost_INTERFACE_DEFINED__ */


#ifndef __ICSInteriorTree_INTERFACE_DEFINED__
#define __ICSInteriorTree_INTERFACE_DEFINED__

/* interface ICSInteriorTree */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSInteriorTree;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4D5D4C20-EE19-11d2-B556-00C04F68D4DB")
    ICSInteriorTree : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTree( 
            MIDL_BASENODE **ppNode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrors( 
            ICSErrorContainer **ppErrors) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSInteriorTreeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSInteriorTree * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSInteriorTree * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSInteriorTree * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTree )( 
            ICSInteriorTree * This,
            MIDL_BASENODE **ppNode);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrors )( 
            ICSInteriorTree * This,
            ICSErrorContainer **ppErrors);
        
        END_INTERFACE
    } ICSInteriorTreeVtbl;

    interface ICSInteriorTree
    {
        CONST_VTBL struct ICSInteriorTreeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSInteriorTree_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSInteriorTree_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSInteriorTree_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSInteriorTree_GetTree(This,ppNode)	\
    (This)->lpVtbl -> GetTree(This,ppNode)

#define ICSInteriorTree_GetErrors(This,ppErrors)	\
    (This)->lpVtbl -> GetErrors(This,ppErrors)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSInteriorTree_GetTree_Proxy( 
    ICSInteriorTree * This,
    MIDL_BASENODE **ppNode);


void __RPC_STUB ICSInteriorTree_GetTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInteriorTree_GetErrors_Proxy( 
    ICSInteriorTree * This,
    ICSErrorContainer **ppErrors);


void __RPC_STUB ICSInteriorTree_GetErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSInteriorTree_INTERFACE_DEFINED__ */


#ifndef __ICSSourceData_INTERFACE_DEFINED__
#define __ICSSourceData_INTERFACE_DEFINED__

/* interface ICSSourceData */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSSourceData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0C12CCE0-E21B-11d2-B555-00C04F68D4DB")
    ICSSourceData : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSourceModule( 
            ICSSourceModule **ppModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLexResults( 
            MIDL_LEXDATA *pLexData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSingleTokenPos( 
            long iToken,
            MIDL_POSDATA *pposToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSingleTokenData( 
            long iToken,
            MIDL_TOKENDATA *pTokenData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTokenText( 
            long iTokenId,
            LPCWSTR *ppszText,
            long *piLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsInsideComment( 
            const MIDL_POSDATA pos,
            BOOL *pfInComment) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ParseTopLevel( 
            MIDL_BASENODE **ppTree) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrors( 
            ERRORCATEGORY iCategory,
            ICSErrorContainer **ppErrors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInteriorParseTree( 
            MIDL_BASENODE *pNode,
            ICSInteriorTree **ppTree) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LookupNode( 
            MIDL_NAME *pKey,
            long iOrdinal,
            MIDL_BASENODE **ppNode,
            long *piGlobalOrdinal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNodeKeyOrdinal( 
            MIDL_BASENODE *pNode,
            MIDL_NAME **ppKey,
            long *piKeyOrdinal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGlobalKeyArray( 
            KEYEDNODE *pKeyedNodes,
            long iSize,
            long *piCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ParseForErrors( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindLeafNode( 
            const MIDL_POSDATA pos,
            MIDL_BASENODE **ppNode,
            ICSInteriorTree **ppTree) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExtent( 
            MIDL_BASENODE *pNode,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTokenExtent( 
            MIDL_BASENODE *pNode,
            long *piFirstToken,
            long *piLastToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDocComment( 
            MIDL_BASENODE *pNode,
            long *piIndex,
            long *piCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDocCommentPos( 
            long iComment,
            MIDL_POSDATA *loc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUpToDate( 
            BOOL *pfTokenized,
            BOOL *pfTopParsed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ForceUpdate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExtentEx( 
            MIDL_BASENODE *pNode,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd,
            ExtentFlags flags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSSourceDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSSourceData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSSourceData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSSourceData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceModule )( 
            ICSSourceData * This,
            ICSSourceModule **ppModule);
        
        HRESULT ( STDMETHODCALLTYPE *GetLexResults )( 
            ICSSourceData * This,
            MIDL_LEXDATA *pLexData);
        
        HRESULT ( STDMETHODCALLTYPE *GetSingleTokenPos )( 
            ICSSourceData * This,
            long iToken,
            MIDL_POSDATA *pposToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetSingleTokenData )( 
            ICSSourceData * This,
            long iToken,
            MIDL_TOKENDATA *pTokenData);
        
        HRESULT ( STDMETHODCALLTYPE *GetTokenText )( 
            ICSSourceData * This,
            long iTokenId,
            LPCWSTR *ppszText,
            long *piLen);
        
        HRESULT ( STDMETHODCALLTYPE *IsInsideComment )( 
            ICSSourceData * This,
            const MIDL_POSDATA pos,
            BOOL *pfInComment);
        
        HRESULT ( STDMETHODCALLTYPE *ParseTopLevel )( 
            ICSSourceData * This,
            MIDL_BASENODE **ppTree);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrors )( 
            ICSSourceData * This,
            ERRORCATEGORY iCategory,
            ICSErrorContainer **ppErrors);
        
        HRESULT ( STDMETHODCALLTYPE *GetInteriorParseTree )( 
            ICSSourceData * This,
            MIDL_BASENODE *pNode,
            ICSInteriorTree **ppTree);
        
        HRESULT ( STDMETHODCALLTYPE *LookupNode )( 
            ICSSourceData * This,
            MIDL_NAME *pKey,
            long iOrdinal,
            MIDL_BASENODE **ppNode,
            long *piGlobalOrdinal);
        
        HRESULT ( STDMETHODCALLTYPE *GetNodeKeyOrdinal )( 
            ICSSourceData * This,
            MIDL_BASENODE *pNode,
            MIDL_NAME **ppKey,
            long *piKeyOrdinal);
        
        HRESULT ( STDMETHODCALLTYPE *GetGlobalKeyArray )( 
            ICSSourceData * This,
            KEYEDNODE *pKeyedNodes,
            long iSize,
            long *piCopied);
        
        HRESULT ( STDMETHODCALLTYPE *ParseForErrors )( 
            ICSSourceData * This);
        
        HRESULT ( STDMETHODCALLTYPE *FindLeafNode )( 
            ICSSourceData * This,
            const MIDL_POSDATA pos,
            MIDL_BASENODE **ppNode,
            ICSInteriorTree **ppTree);
        
        HRESULT ( STDMETHODCALLTYPE *GetExtent )( 
            ICSSourceData * This,
            MIDL_BASENODE *pNode,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetTokenExtent )( 
            ICSSourceData * This,
            MIDL_BASENODE *pNode,
            long *piFirstToken,
            long *piLastToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetDocComment )( 
            ICSSourceData * This,
            MIDL_BASENODE *pNode,
            long *piIndex,
            long *piCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDocCommentPos )( 
            ICSSourceData * This,
            long iComment,
            MIDL_POSDATA *loc);
        
        HRESULT ( STDMETHODCALLTYPE *IsUpToDate )( 
            ICSSourceData * This,
            BOOL *pfTokenized,
            BOOL *pfTopParsed);
        
        HRESULT ( STDMETHODCALLTYPE *ForceUpdate )( 
            ICSSourceData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetExtentEx )( 
            ICSSourceData * This,
            MIDL_BASENODE *pNode,
            MIDL_POSDATA *pposStart,
            MIDL_POSDATA *pposEnd,
            ExtentFlags flags);
        
        END_INTERFACE
    } ICSSourceDataVtbl;

    interface ICSSourceData
    {
        CONST_VTBL struct ICSSourceDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSSourceData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSSourceData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSSourceData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSSourceData_GetSourceModule(This,ppModule)	\
    (This)->lpVtbl -> GetSourceModule(This,ppModule)

#define ICSSourceData_GetLexResults(This,pLexData)	\
    (This)->lpVtbl -> GetLexResults(This,pLexData)

#define ICSSourceData_GetSingleTokenPos(This,iToken,pposToken)	\
    (This)->lpVtbl -> GetSingleTokenPos(This,iToken,pposToken)

#define ICSSourceData_GetSingleTokenData(This,iToken,pTokenData)	\
    (This)->lpVtbl -> GetSingleTokenData(This,iToken,pTokenData)

#define ICSSourceData_GetTokenText(This,iTokenId,ppszText,piLen)	\
    (This)->lpVtbl -> GetTokenText(This,iTokenId,ppszText,piLen)

#define ICSSourceData_IsInsideComment(This,pos,pfInComment)	\
    (This)->lpVtbl -> IsInsideComment(This,pos,pfInComment)

#define ICSSourceData_ParseTopLevel(This,ppTree)	\
    (This)->lpVtbl -> ParseTopLevel(This,ppTree)

#define ICSSourceData_GetErrors(This,iCategory,ppErrors)	\
    (This)->lpVtbl -> GetErrors(This,iCategory,ppErrors)

#define ICSSourceData_GetInteriorParseTree(This,pNode,ppTree)	\
    (This)->lpVtbl -> GetInteriorParseTree(This,pNode,ppTree)

#define ICSSourceData_LookupNode(This,pKey,iOrdinal,ppNode,piGlobalOrdinal)	\
    (This)->lpVtbl -> LookupNode(This,pKey,iOrdinal,ppNode,piGlobalOrdinal)

#define ICSSourceData_GetNodeKeyOrdinal(This,pNode,ppKey,piKeyOrdinal)	\
    (This)->lpVtbl -> GetNodeKeyOrdinal(This,pNode,ppKey,piKeyOrdinal)

#define ICSSourceData_GetGlobalKeyArray(This,pKeyedNodes,iSize,piCopied)	\
    (This)->lpVtbl -> GetGlobalKeyArray(This,pKeyedNodes,iSize,piCopied)

#define ICSSourceData_ParseForErrors(This)	\
    (This)->lpVtbl -> ParseForErrors(This)

#define ICSSourceData_FindLeafNode(This,pos,ppNode,ppTree)	\
    (This)->lpVtbl -> FindLeafNode(This,pos,ppNode,ppTree)

#define ICSSourceData_GetExtent(This,pNode,pposStart,pposEnd)	\
    (This)->lpVtbl -> GetExtent(This,pNode,pposStart,pposEnd)

#define ICSSourceData_GetTokenExtent(This,pNode,piFirstToken,piLastToken)	\
    (This)->lpVtbl -> GetTokenExtent(This,pNode,piFirstToken,piLastToken)

#define ICSSourceData_GetDocComment(This,pNode,piIndex,piCount)	\
    (This)->lpVtbl -> GetDocComment(This,pNode,piIndex,piCount)

#define ICSSourceData_GetDocCommentPos(This,iComment,loc)	\
    (This)->lpVtbl -> GetDocCommentPos(This,iComment,loc)

#define ICSSourceData_IsUpToDate(This,pfTokenized,pfTopParsed)	\
    (This)->lpVtbl -> IsUpToDate(This,pfTokenized,pfTopParsed)

#define ICSSourceData_ForceUpdate(This)	\
    (This)->lpVtbl -> ForceUpdate(This)

#define ICSSourceData_GetExtentEx(This,pNode,pposStart,pposEnd,flags)	\
    (This)->lpVtbl -> GetExtentEx(This,pNode,pposStart,pposEnd,flags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSSourceData_GetSourceModule_Proxy( 
    ICSSourceData * This,
    ICSSourceModule **ppModule);


void __RPC_STUB ICSSourceData_GetSourceModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetLexResults_Proxy( 
    ICSSourceData * This,
    MIDL_LEXDATA *pLexData);


void __RPC_STUB ICSSourceData_GetLexResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetSingleTokenPos_Proxy( 
    ICSSourceData * This,
    long iToken,
    MIDL_POSDATA *pposToken);


void __RPC_STUB ICSSourceData_GetSingleTokenPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetSingleTokenData_Proxy( 
    ICSSourceData * This,
    long iToken,
    MIDL_TOKENDATA *pTokenData);


void __RPC_STUB ICSSourceData_GetSingleTokenData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetTokenText_Proxy( 
    ICSSourceData * This,
    long iTokenId,
    LPCWSTR *ppszText,
    long *piLen);


void __RPC_STUB ICSSourceData_GetTokenText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_IsInsideComment_Proxy( 
    ICSSourceData * This,
    const MIDL_POSDATA pos,
    BOOL *pfInComment);


void __RPC_STUB ICSSourceData_IsInsideComment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_ParseTopLevel_Proxy( 
    ICSSourceData * This,
    MIDL_BASENODE **ppTree);


void __RPC_STUB ICSSourceData_ParseTopLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetErrors_Proxy( 
    ICSSourceData * This,
    ERRORCATEGORY iCategory,
    ICSErrorContainer **ppErrors);


void __RPC_STUB ICSSourceData_GetErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetInteriorParseTree_Proxy( 
    ICSSourceData * This,
    MIDL_BASENODE *pNode,
    ICSInteriorTree **ppTree);


void __RPC_STUB ICSSourceData_GetInteriorParseTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_LookupNode_Proxy( 
    ICSSourceData * This,
    MIDL_NAME *pKey,
    long iOrdinal,
    MIDL_BASENODE **ppNode,
    long *piGlobalOrdinal);


void __RPC_STUB ICSSourceData_LookupNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetNodeKeyOrdinal_Proxy( 
    ICSSourceData * This,
    MIDL_BASENODE *pNode,
    MIDL_NAME **ppKey,
    long *piKeyOrdinal);


void __RPC_STUB ICSSourceData_GetNodeKeyOrdinal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetGlobalKeyArray_Proxy( 
    ICSSourceData * This,
    KEYEDNODE *pKeyedNodes,
    long iSize,
    long *piCopied);


void __RPC_STUB ICSSourceData_GetGlobalKeyArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_ParseForErrors_Proxy( 
    ICSSourceData * This);


void __RPC_STUB ICSSourceData_ParseForErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_FindLeafNode_Proxy( 
    ICSSourceData * This,
    const MIDL_POSDATA pos,
    MIDL_BASENODE **ppNode,
    ICSInteriorTree **ppTree);


void __RPC_STUB ICSSourceData_FindLeafNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetExtent_Proxy( 
    ICSSourceData * This,
    MIDL_BASENODE *pNode,
    MIDL_POSDATA *pposStart,
    MIDL_POSDATA *pposEnd);


void __RPC_STUB ICSSourceData_GetExtent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetTokenExtent_Proxy( 
    ICSSourceData * This,
    MIDL_BASENODE *pNode,
    long *piFirstToken,
    long *piLastToken);


void __RPC_STUB ICSSourceData_GetTokenExtent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetDocComment_Proxy( 
    ICSSourceData * This,
    MIDL_BASENODE *pNode,
    long *piIndex,
    long *piCount);


void __RPC_STUB ICSSourceData_GetDocComment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetDocCommentPos_Proxy( 
    ICSSourceData * This,
    long iComment,
    MIDL_POSDATA *loc);


void __RPC_STUB ICSSourceData_GetDocCommentPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_IsUpToDate_Proxy( 
    ICSSourceData * This,
    BOOL *pfTokenized,
    BOOL *pfTopParsed);


void __RPC_STUB ICSSourceData_IsUpToDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_ForceUpdate_Proxy( 
    ICSSourceData * This);


void __RPC_STUB ICSSourceData_ForceUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceData_GetExtentEx_Proxy( 
    ICSSourceData * This,
    MIDL_BASENODE *pNode,
    MIDL_POSDATA *pposStart,
    MIDL_POSDATA *pposEnd,
    ExtentFlags flags);


void __RPC_STUB ICSSourceData_GetExtentEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSSourceData_INTERFACE_DEFINED__ */


#ifndef __ICSSourceModuleEvents_INTERFACE_DEFINED__
#define __ICSSourceModuleEvents_INTERFACE_DEFINED__

/* interface ICSSourceModuleEvents */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSSourceModuleEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F78E9D16-E63F-11d2-B556-00C04F68D4DB")
    ICSSourceModuleEvents : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnModuleModified( void) = 0;
        
        virtual void STDMETHODCALLTYPE OnStateTransitionsChanged( 
            long iFirstLine,
            long iLastLine) = 0;
        
        virtual void STDMETHODCALLTYPE OnCommentTableChanged( 
            long iStart,
            long iOldEnd,
            long iNewEnd) = 0;
        
        virtual void STDMETHODCALLTYPE OnBeginTokenization( 
            long iStartingAtLine) = 0;
        
        virtual void STDMETHODCALLTYPE OnEndTokenization( 
            long iFirstLine,
            long iLastLine,
            long iDelta) = 0;
        
        virtual void STDMETHODCALLTYPE OnBeginParse( 
            MIDL_BASENODE *pNode) = 0;
        
        virtual void STDMETHODCALLTYPE OnEndParse( 
            MIDL_BASENODE *pNode) = 0;
        
        virtual void STDMETHODCALLTYPE ReportErrors( 
            ICSErrorContainer *pErrors) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSSourceModuleEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSSourceModuleEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSSourceModuleEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSSourceModuleEvents * This);
        
        void ( STDMETHODCALLTYPE *OnModuleModified )( 
            ICSSourceModuleEvents * This);
        
        void ( STDMETHODCALLTYPE *OnStateTransitionsChanged )( 
            ICSSourceModuleEvents * This,
            long iFirstLine,
            long iLastLine);
        
        void ( STDMETHODCALLTYPE *OnCommentTableChanged )( 
            ICSSourceModuleEvents * This,
            long iStart,
            long iOldEnd,
            long iNewEnd);
        
        void ( STDMETHODCALLTYPE *OnBeginTokenization )( 
            ICSSourceModuleEvents * This,
            long iStartingAtLine);
        
        void ( STDMETHODCALLTYPE *OnEndTokenization )( 
            ICSSourceModuleEvents * This,
            long iFirstLine,
            long iLastLine,
            long iDelta);
        
        void ( STDMETHODCALLTYPE *OnBeginParse )( 
            ICSSourceModuleEvents * This,
            MIDL_BASENODE *pNode);
        
        void ( STDMETHODCALLTYPE *OnEndParse )( 
            ICSSourceModuleEvents * This,
            MIDL_BASENODE *pNode);
        
        void ( STDMETHODCALLTYPE *ReportErrors )( 
            ICSSourceModuleEvents * This,
            ICSErrorContainer *pErrors);
        
        END_INTERFACE
    } ICSSourceModuleEventsVtbl;

    interface ICSSourceModuleEvents
    {
        CONST_VTBL struct ICSSourceModuleEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSSourceModuleEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSSourceModuleEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSSourceModuleEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSSourceModuleEvents_OnModuleModified(This)	\
    (This)->lpVtbl -> OnModuleModified(This)

#define ICSSourceModuleEvents_OnStateTransitionsChanged(This,iFirstLine,iLastLine)	\
    (This)->lpVtbl -> OnStateTransitionsChanged(This,iFirstLine,iLastLine)

#define ICSSourceModuleEvents_OnCommentTableChanged(This,iStart,iOldEnd,iNewEnd)	\
    (This)->lpVtbl -> OnCommentTableChanged(This,iStart,iOldEnd,iNewEnd)

#define ICSSourceModuleEvents_OnBeginTokenization(This,iStartingAtLine)	\
    (This)->lpVtbl -> OnBeginTokenization(This,iStartingAtLine)

#define ICSSourceModuleEvents_OnEndTokenization(This,iFirstLine,iLastLine,iDelta)	\
    (This)->lpVtbl -> OnEndTokenization(This,iFirstLine,iLastLine,iDelta)

#define ICSSourceModuleEvents_OnBeginParse(This,pNode)	\
    (This)->lpVtbl -> OnBeginParse(This,pNode)

#define ICSSourceModuleEvents_OnEndParse(This,pNode)	\
    (This)->lpVtbl -> OnEndParse(This,pNode)

#define ICSSourceModuleEvents_ReportErrors(This,pErrors)	\
    (This)->lpVtbl -> ReportErrors(This,pErrors)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE ICSSourceModuleEvents_OnModuleModified_Proxy( 
    ICSSourceModuleEvents * This);


void __RPC_STUB ICSSourceModuleEvents_OnModuleModified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSSourceModuleEvents_OnStateTransitionsChanged_Proxy( 
    ICSSourceModuleEvents * This,
    long iFirstLine,
    long iLastLine);


void __RPC_STUB ICSSourceModuleEvents_OnStateTransitionsChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSSourceModuleEvents_OnCommentTableChanged_Proxy( 
    ICSSourceModuleEvents * This,
    long iStart,
    long iOldEnd,
    long iNewEnd);


void __RPC_STUB ICSSourceModuleEvents_OnCommentTableChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSSourceModuleEvents_OnBeginTokenization_Proxy( 
    ICSSourceModuleEvents * This,
    long iStartingAtLine);


void __RPC_STUB ICSSourceModuleEvents_OnBeginTokenization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSSourceModuleEvents_OnEndTokenization_Proxy( 
    ICSSourceModuleEvents * This,
    long iFirstLine,
    long iLastLine,
    long iDelta);


void __RPC_STUB ICSSourceModuleEvents_OnEndTokenization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSSourceModuleEvents_OnBeginParse_Proxy( 
    ICSSourceModuleEvents * This,
    MIDL_BASENODE *pNode);


void __RPC_STUB ICSSourceModuleEvents_OnBeginParse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSSourceModuleEvents_OnEndParse_Proxy( 
    ICSSourceModuleEvents * This,
    MIDL_BASENODE *pNode);


void __RPC_STUB ICSSourceModuleEvents_OnEndParse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE ICSSourceModuleEvents_ReportErrors_Proxy( 
    ICSSourceModuleEvents * This,
    ICSErrorContainer *pErrors);


void __RPC_STUB ICSSourceModuleEvents_ReportErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSSourceModuleEvents_INTERFACE_DEFINED__ */


#ifndef __ICSSourceModule_INTERFACE_DEFINED__
#define __ICSSourceModule_INTERFACE_DEFINED__

/* interface ICSSourceModule */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSSourceModule;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D9BF2800-E098-11d2-B554-00C04F68D4DB")
    ICSSourceModule : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSourceData( 
            BOOL fBlockForNewest,
            ICSSourceData **ppSourceData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceText( 
            ICSSourceText **ppText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
                
        virtual HRESULT STDMETHODCALLTYPE GetMemoryConsumption( 
            long *piTopTree,
            long *piInteriorTrees,
            long *piInteriorNodes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSSourceModuleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSSourceModule * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSSourceModule * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSSourceModule * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceData )( 
            ICSSourceModule * This,
            BOOL fBlockForNewest,
            ICSSourceData **ppSourceData);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceText )( 
            ICSSourceModule * This,
            ICSSourceText **ppText);
        
        HRESULT ( STDMETHODCALLTYPE *Flush )( 
            ICSSourceModule * This);
        
        HRESULT ( STDMETHODCALLTYPE *DisconnectTextEvents )( 
            ICSSourceModule * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemoryConsumption )( 
            ICSSourceModule * This,
            long *piTopTree,
            long *piInteriorTrees,
            long *piInteriorNodes);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseChangeEvents )( 
            ICSSourceModule * This,
            ICSSourceModuleEvents *pSink,
            DWORD_PTR *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseChangeEvents )( 
            ICSSourceModule * This,
            DWORD_PTR dwCookie);
        
        END_INTERFACE
    } ICSSourceModuleVtbl;

    interface ICSSourceModule
    {
        CONST_VTBL struct ICSSourceModuleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSSourceModule_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSSourceModule_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSSourceModule_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSSourceModule_GetSourceData(This,fBlockForNewest,ppSourceData)	\
    (This)->lpVtbl -> GetSourceData(This,fBlockForNewest,ppSourceData)

#define ICSSourceModule_GetSourceText(This,ppText)	\
    (This)->lpVtbl -> GetSourceText(This,ppText)

#define ICSSourceModule_Flush(This)	\
    (This)->lpVtbl -> Flush(This)

#define ICSSourceModule_DisconnectTextEvents(This)	\
    (This)->lpVtbl -> DisconnectTextEvents(This)

#define ICSSourceModule_GetMemoryConsumption(This,piTopTree,piInteriorTrees,piInteriorNodes)	\
    (This)->lpVtbl -> GetMemoryConsumption(This,piTopTree,piInteriorTrees,piInteriorNodes)

#define ICSSourceModule_AdviseChangeEvents(This,pSink,pdwCookie)	\
    (This)->lpVtbl -> AdviseChangeEvents(This,pSink,pdwCookie)

#define ICSSourceModule_UnadviseChangeEvents(This,dwCookie)	\
    (This)->lpVtbl -> UnadviseChangeEvents(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSSourceModule_GetSourceData_Proxy( 
    ICSSourceModule * This,
    BOOL fBlockForNewest,
    ICSSourceData **ppSourceData);


void __RPC_STUB ICSSourceModule_GetSourceData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceModule_GetSourceText_Proxy( 
    ICSSourceModule * This,
    ICSSourceText **ppText);


void __RPC_STUB ICSSourceModule_GetSourceText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceModule_Flush_Proxy( 
    ICSSourceModule * This);


void __RPC_STUB ICSSourceModule_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceModule_DisconnectTextEvents_Proxy( 
    ICSSourceModule * This);


void __RPC_STUB ICSSourceModule_DisconnectTextEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceModule_GetMemoryConsumption_Proxy( 
    ICSSourceModule * This,
    long *piTopTree,
    long *piInteriorTrees,
    long *piInteriorNodes);


void __RPC_STUB ICSSourceModule_GetMemoryConsumption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceModule_AdviseChangeEvents_Proxy( 
    ICSSourceModule * This,
    ICSSourceModuleEvents *pSink,
    DWORD_PTR *pdwCookie);


void __RPC_STUB ICSSourceModule_AdviseChangeEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSSourceModule_UnadviseChangeEvents_Proxy( 
    ICSSourceModule * This,
    DWORD_PTR dwCookie);


void __RPC_STUB ICSSourceModule_UnadviseChangeEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSSourceModule_INTERFACE_DEFINED__ */


#ifndef __ICSNameTable_INTERFACE_DEFINED__
#define __ICSNameTable_INTERFACE_DEFINED__

/* interface ICSNameTable */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSNameTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3246651A-E0B4-11d2-B555-00C04F68D4DB")
    ICSNameTable : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Lookup( 
            LPCWSTR pszName,
            MIDL_NAME **ppName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LookupLen( 
            LPCWSTR pszName,
            long iLen,
            MIDL_NAME **ppName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Add( 
            LPCWSTR pszName,
            MIDL_NAME **ppName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddLen( 
            LPCWSTR pszName,
            long iLen,
            MIDL_NAME **ppName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsValidIdentifier( 
            LPCWSTR pszName,
            VARIANT_BOOL *pfValid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTokenText( 
            long iTokenId,
            LPCWSTR *ppszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPredefinedName( 
            long iPredefName,
            MIDL_NAME **ppName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreprocessorDirective( 
            MIDL_NAME *pName,
            long *piToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsKeyword( 
            MIDL_NAME *pName,
            long *piToken) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSNameTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSNameTable * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSNameTable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSNameTable * This);
        
        HRESULT ( STDMETHODCALLTYPE *Lookup )( 
            ICSNameTable * This,
            LPCWSTR pszName,
            MIDL_NAME **ppName);
        
        HRESULT ( STDMETHODCALLTYPE *LookupLen )( 
            ICSNameTable * This,
            LPCWSTR pszName,
            long iLen,
            MIDL_NAME **ppName);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            ICSNameTable * This,
            LPCWSTR pszName,
            MIDL_NAME **ppName);
        
        HRESULT ( STDMETHODCALLTYPE *AddLen )( 
            ICSNameTable * This,
            LPCWSTR pszName,
            long iLen,
            MIDL_NAME **ppName);
        
        HRESULT ( STDMETHODCALLTYPE *IsValidIdentifier )( 
            ICSNameTable * This,
            LPCWSTR pszName,
            VARIANT_BOOL *pfValid);
        
        HRESULT ( STDMETHODCALLTYPE *GetTokenText )( 
            ICSNameTable * This,
            long iTokenId,
            LPCWSTR *ppszText);
        
        HRESULT ( STDMETHODCALLTYPE *GetPredefinedName )( 
            ICSNameTable * This,
            long iPredefName,
            MIDL_NAME **ppName);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreprocessorDirective )( 
            ICSNameTable * This,
            MIDL_NAME *pName,
            long *piToken);
        
        HRESULT ( STDMETHODCALLTYPE *IsKeyword )( 
            ICSNameTable * This,
            MIDL_NAME *pName,
            long *piToken);
        
        END_INTERFACE
    } ICSNameTableVtbl;

    interface ICSNameTable
    {
        CONST_VTBL struct ICSNameTableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSNameTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSNameTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSNameTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSNameTable_Lookup(This,pszName,ppName)	\
    (This)->lpVtbl -> Lookup(This,pszName,ppName)

#define ICSNameTable_LookupLen(This,pszName,iLen,ppName)	\
    (This)->lpVtbl -> LookupLen(This,pszName,iLen,ppName)

#define ICSNameTable_Add(This,pszName,ppName)	\
    (This)->lpVtbl -> Add(This,pszName,ppName)

#define ICSNameTable_AddLen(This,pszName,iLen,ppName)	\
    (This)->lpVtbl -> AddLen(This,pszName,iLen,ppName)

#define ICSNameTable_IsValidIdentifier(This,pszName,pfValid)	\
    (This)->lpVtbl -> IsValidIdentifier(This,pszName,pfValid)

#define ICSNameTable_GetTokenText(This,iTokenId,ppszText)	\
    (This)->lpVtbl -> GetTokenText(This,iTokenId,ppszText)

#define ICSNameTable_GetPredefinedName(This,iPredefName,ppName)	\
    (This)->lpVtbl -> GetPredefinedName(This,iPredefName,ppName)

#define ICSNameTable_GetPreprocessorDirective(This,pName,piToken)	\
    (This)->lpVtbl -> GetPreprocessorDirective(This,pName,piToken)

#define ICSNameTable_IsKeyword(This,pName,piToken)	\
    (This)->lpVtbl -> IsKeyword(This,pName,piToken)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSNameTable_Lookup_Proxy( 
    ICSNameTable * This,
    LPCWSTR pszName,
    MIDL_NAME **ppName);


void __RPC_STUB ICSNameTable_Lookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_LookupLen_Proxy( 
    ICSNameTable * This,
    LPCWSTR pszName,
    long iLen,
    MIDL_NAME **ppName);


void __RPC_STUB ICSNameTable_LookupLen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_Add_Proxy( 
    ICSNameTable * This,
    LPCWSTR pszName,
    MIDL_NAME **ppName);


void __RPC_STUB ICSNameTable_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_AddLen_Proxy( 
    ICSNameTable * This,
    LPCWSTR pszName,
    long iLen,
    MIDL_NAME **ppName);


void __RPC_STUB ICSNameTable_AddLen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_IsValidIdentifier_Proxy( 
    ICSNameTable * This,
    LPCWSTR pszName,
    VARIANT_BOOL *pfValid);


void __RPC_STUB ICSNameTable_IsValidIdentifier_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_GetTokenText_Proxy( 
    ICSNameTable * This,
    long iTokenId,
    LPCWSTR *ppszText);


void __RPC_STUB ICSNameTable_GetTokenText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_GetPredefinedName_Proxy( 
    ICSNameTable * This,
    long iPredefName,
    MIDL_NAME **ppName);


void __RPC_STUB ICSNameTable_GetPredefinedName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_GetPreprocessorDirective_Proxy( 
    ICSNameTable * This,
    MIDL_NAME *pName,
    long *piToken);


void __RPC_STUB ICSNameTable_GetPreprocessorDirective_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSNameTable_IsKeyword_Proxy( 
    ICSNameTable * This,
    MIDL_NAME *pName,
    long *piToken);


void __RPC_STUB ICSNameTable_IsKeyword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSNameTable_INTERFACE_DEFINED__ */


#ifndef __ICSLexer_INTERFACE_DEFINED__
#define __ICSLexer_INTERFACE_DEFINED__

/* interface ICSLexer */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSLexer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("064C8C67-5F66-4bf1-AF98-ED8BFA7B690A")
    ICSLexer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInput( 
            LPCWSTR pszInputText,
            long iLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLexResults( 
            MIDL_LEXDATA *pLexData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RescanToken( 
            long iToken,
            MIDL_TOKENDATA *pTokenData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSLexerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSLexer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSLexer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSLexer * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetInput )( 
            ICSLexer * This,
            LPCWSTR pszInputText,
            long iLen);
        
        HRESULT ( STDMETHODCALLTYPE *GetLexResults )( 
            ICSLexer * This,
            MIDL_LEXDATA *pLexData);
        
        HRESULT ( STDMETHODCALLTYPE *RescanToken )( 
            ICSLexer * This,
            long iToken,
            MIDL_TOKENDATA *pTokenData);
        
        END_INTERFACE
    } ICSLexerVtbl;

    interface ICSLexer
    {
        CONST_VTBL struct ICSLexerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSLexer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSLexer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSLexer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSLexer_SetInput(This,pszInputText,iLen)	\
    (This)->lpVtbl -> SetInput(This,pszInputText,iLen)

#define ICSLexer_GetLexResults(This,pLexData)	\
    (This)->lpVtbl -> GetLexResults(This,pLexData)

#define ICSLexer_RescanToken(This,iToken,pTokenData)	\
    (This)->lpVtbl -> RescanToken(This,iToken,pTokenData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSLexer_SetInput_Proxy( 
    ICSLexer * This,
    LPCWSTR pszInputText,
    long iLen);


void __RPC_STUB ICSLexer_SetInput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSLexer_GetLexResults_Proxy( 
    ICSLexer * This,
    MIDL_LEXDATA *pLexData);


void __RPC_STUB ICSLexer_GetLexResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSLexer_RescanToken_Proxy( 
    ICSLexer * This,
    long iToken,
    MIDL_TOKENDATA *pTokenData);


void __RPC_STUB ICSLexer_RescanToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSLexer_INTERFACE_DEFINED__ */


#ifndef __ICSParser_INTERFACE_DEFINED__
#define __ICSParser_INTERFACE_DEFINED__

/* interface ICSParser */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSParser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E5B1B490-850B-4aad-9D68-B5B7CA2DAAD2")
    ICSParser : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInputText( 
            LPCWSTR pszInputText,
            long iLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputStream( 
            BYTE *pTokens,
            MIDL_POSDATA *pposTokens,
            long iTokens,
            LPCWSTR *ppszLines,
            long iLines) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateParseTree( 
            ParseTreeScope iScope,
            MIDL_BASENODE *pParent,
            MIDL_BASENODE **ppNode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateNode( 
            long iKind,
            MIDL_BASENODE **ppNode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSParserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSParser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSParser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSParser * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetInputText )( 
            ICSParser * This,
            LPCWSTR pszInputText,
            long iLen);
        
        HRESULT ( STDMETHODCALLTYPE *SetInputStream )( 
            ICSParser * This,
            BYTE *pTokens,
            MIDL_POSDATA *pposTokens,
            long iTokens,
            LPCWSTR *ppszLines,
            long iLines);
        
        HRESULT ( STDMETHODCALLTYPE *CreateParseTree )( 
            ICSParser * This,
            ParseTreeScope iScope,
            MIDL_BASENODE *pParent,
            MIDL_BASENODE **ppNode);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateNode )( 
            ICSParser * This,
            long iKind,
            MIDL_BASENODE **ppNode);
        
        END_INTERFACE
    } ICSParserVtbl;

    interface ICSParser
    {
        CONST_VTBL struct ICSParserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSParser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSParser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSParser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSParser_SetInputText(This,pszInputText,iLen)	\
    (This)->lpVtbl -> SetInputText(This,pszInputText,iLen)

#define ICSParser_SetInputStream(This,pTokens,pposTokens,iTokens,ppszLines,iLines)	\
    (This)->lpVtbl -> SetInputStream(This,pTokens,pposTokens,iTokens,ppszLines,iLines)

#define ICSParser_CreateParseTree(This,iScope,pParent,ppNode)	\
    (This)->lpVtbl -> CreateParseTree(This,iScope,pParent,ppNode)

#define ICSParser_AllocateNode(This,iKind,ppNode)	\
    (This)->lpVtbl -> AllocateNode(This,iKind,ppNode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSParser_SetInputText_Proxy( 
    ICSParser * This,
    LPCWSTR pszInputText,
    long iLen);


void __RPC_STUB ICSParser_SetInputText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSParser_SetInputStream_Proxy( 
    ICSParser * This,
    BYTE *pTokens,
    MIDL_POSDATA *pposTokens,
    long iTokens,
    LPCWSTR *ppszLines,
    long iLines);


void __RPC_STUB ICSParser_SetInputStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSParser_CreateParseTree_Proxy( 
    ICSParser * This,
    ParseTreeScope iScope,
    MIDL_BASENODE *pParent,
    MIDL_BASENODE **ppNode);


void __RPC_STUB ICSParser_CreateParseTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSParser_AllocateNode_Proxy( 
    ICSParser * This,
    long iKind,
    MIDL_BASENODE **ppNode);


void __RPC_STUB ICSParser_AllocateNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSParser_INTERFACE_DEFINED__ */


#ifndef __ICSCompilerConfig_INTERFACE_DEFINED__
#define __ICSCompilerConfig_INTERFACE_DEFINED__

/* interface ICSCompilerConfig */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSCompilerConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4D5D4C21-EE19-11d2-B556-00C04F68D4DB")
    ICSCompilerConfig : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOptionCount( 
            long *piCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOptionInfoAt( 
            long iIndex,
            long *piOptionID,
            LPCWSTR *ppszSwitchName,
            LPCWSTR *ppszDescription,
            DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOptionInfoAtEx( 
            long iIndex,
            long *piOptionID,
            LPCWSTR *ppszShortSwitchName,
            LPCWSTR *ppszLongSwitchName,
            LPCWSTR *ppszDescSwitchName,
            LPCWSTR *ppszDescription,
            DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetAllOptions( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOption( 
            long iOptionID,
            VARIANT vtValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOption( 
            long iOptionID,
            VARIANT *pvtValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommitChanges( 
            ICSError **ppError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompiler( 
            ICSCompiler **ppCompiler) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSCompilerConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSCompilerConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSCompilerConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSCompilerConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptionCount )( 
            ICSCompilerConfig * This,
            long *piCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptionInfoAt )( 
            ICSCompilerConfig * This,
            long iIndex,
            long *piOptionID,
            LPCWSTR *ppszSwitchName,
            LPCWSTR *ppszDescription,
            DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptionInfoAtEx )( 
            ICSCompilerConfig * This,
            long iIndex,
            long *piOptionID,
            LPCWSTR *ppszShortSwitchName,
            LPCWSTR *ppszLongSwitchName,
            LPCWSTR *ppszDescSwitchName,
            LPCWSTR *ppszDescription,
            DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ResetAllOptions )( 
            ICSCompilerConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetOption )( 
            ICSCompilerConfig * This,
            long iOptionID,
            VARIANT vtValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetOption )( 
            ICSCompilerConfig * This,
            long iOptionID,
            VARIANT *pvtValue);
        
        HRESULT ( STDMETHODCALLTYPE *CommitChanges )( 
            ICSCompilerConfig * This,
            ICSError **ppError);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompiler )( 
            ICSCompilerConfig * This,
            ICSCompiler **ppCompiler);
        
        END_INTERFACE
    } ICSCompilerConfigVtbl;

    interface ICSCompilerConfig
    {
        CONST_VTBL struct ICSCompilerConfigVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSCompilerConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSCompilerConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSCompilerConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSCompilerConfig_GetOptionCount(This,piCount)	\
    (This)->lpVtbl -> GetOptionCount(This,piCount)

#define ICSCompilerConfig_GetOptionInfoAt(This,iIndex,piOptionID,ppszSwitchName,ppszDescription,pdwFlags)	\
    (This)->lpVtbl -> GetOptionInfoAt(This,iIndex,piOptionID,ppszSwitchName,ppszDescription,pdwFlags)

#define ICSCompilerConfig_GetOptionInfoAtEx(This,iIndex,piOptionID,ppszShortSwitchName,ppszLongSwitchName,ppszDescSwitchName,ppszDescription,pdwFlags)	\
    (This)->lpVtbl -> GetOptionInfoAtEx(This,iIndex,piOptionID,ppszShortSwitchName,ppszLongSwitchName,ppszDescSwitchName,ppszDescription,pdwFlags)

#define ICSCompilerConfig_ResetAllOptions(This)	\
    (This)->lpVtbl -> ResetAllOptions(This)

#define ICSCompilerConfig_SetOption(This,iOptionID,vtValue)	\
    (This)->lpVtbl -> SetOption(This,iOptionID,vtValue)

#define ICSCompilerConfig_GetOption(This,iOptionID,pvtValue)	\
    (This)->lpVtbl -> GetOption(This,iOptionID,pvtValue)

#define ICSCompilerConfig_CommitChanges(This,ppError)	\
    (This)->lpVtbl -> CommitChanges(This,ppError)

#define ICSCompilerConfig_GetCompiler(This,ppCompiler)	\
    (This)->lpVtbl -> GetCompiler(This,ppCompiler)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSCompilerConfig_GetOptionCount_Proxy( 
    ICSCompilerConfig * This,
    long *piCount);


void __RPC_STUB ICSCompilerConfig_GetOptionCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerConfig_GetOptionInfoAt_Proxy( 
    ICSCompilerConfig * This,
    long iIndex,
    long *piOptionID,
    LPCWSTR *ppszSwitchName,
    LPCWSTR *ppszDescription,
    DWORD *pdwFlags);


void __RPC_STUB ICSCompilerConfig_GetOptionInfoAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerConfig_GetOptionInfoAtEx_Proxy( 
    ICSCompilerConfig * This,
    long iIndex,
    long *piOptionID,
    LPCWSTR *ppszShortSwitchName,
    LPCWSTR *ppszLongSwitchName,
    LPCWSTR *ppszDescSwitchName,
    LPCWSTR *ppszDescription,
    DWORD *pdwFlags);


void __RPC_STUB ICSCompilerConfig_GetOptionInfoAtEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerConfig_ResetAllOptions_Proxy( 
    ICSCompilerConfig * This);


void __RPC_STUB ICSCompilerConfig_ResetAllOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerConfig_SetOption_Proxy( 
    ICSCompilerConfig * This,
    long iOptionID,
    VARIANT vtValue);


void __RPC_STUB ICSCompilerConfig_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerConfig_GetOption_Proxy( 
    ICSCompilerConfig * This,
    long iOptionID,
    VARIANT *pvtValue);


void __RPC_STUB ICSCompilerConfig_GetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerConfig_CommitChanges_Proxy( 
    ICSCompilerConfig * This,
    ICSError **ppError);


void __RPC_STUB ICSCompilerConfig_CommitChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerConfig_GetCompiler_Proxy( 
    ICSCompilerConfig * This,
    ICSCompiler **ppCompiler);


void __RPC_STUB ICSCompilerConfig_GetCompiler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSCompilerConfig_INTERFACE_DEFINED__ */


#ifndef __ICSInputSet_INTERFACE_DEFINED__
#define __ICSInputSet_INTERFACE_DEFINED__

/* interface ICSInputSet */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSInputSet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4D5D4C22-EE19-11d2-B556-00C04F68D4DB")
    ICSInputSet : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCompiler( 
            ICSCompiler **ppCompiler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddSourceFile( 
            LPCWSTR pszFileName,
            FILETIME *pFT) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveSourceFile( 
            LPCWSTR pszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllSourceFiles( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddResourceFile( 
            LPCWSTR pszFileName,
            LPCWSTR pszIdent,
            BOOL bEmbed,
            BOOL bVis) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveResourceFile( 
            LPCWSTR pszFileName,
            LPCWSTR pszIdent,
            BOOL bEmbed,
            BOOL bVis) = 0;
        
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputFileName( 
            LPCWSTR pszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputFileType( 
            DWORD dwFileType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetImageBase( 
            DWORD dwImageBase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMainClass( 
            LPCWSTR pszFQClassName) = 0;
        
        
        virtual HRESULT STDMETHODCALLTYPE SetFileAlignment( 
            DWORD dwAlign) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSInputSetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSInputSet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSInputSet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSInputSet * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompiler )( 
            ICSInputSet * This,
            ICSCompiler **ppCompiler);
        
        HRESULT ( STDMETHODCALLTYPE *AddSourceFile )( 
            ICSInputSet * This,
            LPCWSTR pszFileName,
            FILETIME *pFT);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveSourceFile )( 
            ICSInputSet * This,
            LPCWSTR pszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveAllSourceFiles )( 
            ICSInputSet * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddResourceFile )( 
            ICSInputSet * This,
            LPCWSTR pszFileName,
            LPCWSTR pszIdent,
            BOOL bEmbed,
            BOOL bVis);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveResourceFile )( 
            ICSInputSet * This,
            LPCWSTR pszFileName,
            LPCWSTR pszIdent,
            BOOL bEmbed,
            BOOL bVis);
        
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputFileName )( 
            ICSInputSet * This,
            LPCWSTR pszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputFileType )( 
            ICSInputSet * This,
            DWORD dwFileType);
        
        HRESULT ( STDMETHODCALLTYPE *SetImageBase )( 
            ICSInputSet * This,
            DWORD dwImageBase);
        
        HRESULT ( STDMETHODCALLTYPE *SetMainClass )( 
            ICSInputSet * This,
            LPCWSTR pszFQClassName);
        
        
        HRESULT ( STDMETHODCALLTYPE *SetFileAlignment )( 
            ICSInputSet * This,
            DWORD dwAlign);
        
        END_INTERFACE
    } ICSInputSetVtbl;

    interface ICSInputSet
    {
        CONST_VTBL struct ICSInputSetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSInputSet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSInputSet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSInputSet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSInputSet_GetCompiler(This,ppCompiler)	\
    (This)->lpVtbl -> GetCompiler(This,ppCompiler)

#define ICSInputSet_AddSourceFile(This,pszFileName,pFT)	\
    (This)->lpVtbl -> AddSourceFile(This,pszFileName,pFT)

#define ICSInputSet_RemoveSourceFile(This,pszFileName)	\
    (This)->lpVtbl -> RemoveSourceFile(This,pszFileName)

#define ICSInputSet_RemoveAllSourceFiles(This)	\
    (This)->lpVtbl -> RemoveAllSourceFiles(This)

#define ICSInputSet_AddResourceFile(This,pszFileName,pszIdent,bEmbed,bVis)	\
    (This)->lpVtbl -> AddResourceFile(This,pszFileName,pszIdent,bEmbed,bVis)

#define ICSInputSet_RemoveResourceFile(This,pszFileName,pszIdent,bEmbed,bVis)	\
    (This)->lpVtbl -> RemoveResourceFile(This,pszFileName,pszIdent,bEmbed,bVis)


#define ICSInputSet_SetOutputFileName(This,pszFileName)	\
    (This)->lpVtbl -> SetOutputFileName(This,pszFileName)

#define ICSInputSet_SetOutputFileType(This,dwFileType)	\
    (This)->lpVtbl -> SetOutputFileType(This,dwFileType)

#define ICSInputSet_SetImageBase(This,dwImageBase)	\
    (This)->lpVtbl -> SetImageBase(This,dwImageBase)

#define ICSInputSet_SetMainClass(This,pszFQClassName)	\
    (This)->lpVtbl -> SetMainClass(This,pszFQClassName)


#define ICSInputSet_SetFileAlignment(This,dwAlign)	\
    (This)->lpVtbl -> SetFileAlignment(This,dwAlign)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSInputSet_GetCompiler_Proxy( 
    ICSInputSet * This,
    ICSCompiler **ppCompiler);


void __RPC_STUB ICSInputSet_GetCompiler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_AddSourceFile_Proxy( 
    ICSInputSet * This,
    LPCWSTR pszFileName,
    FILETIME *pFT);


void __RPC_STUB ICSInputSet_AddSourceFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_RemoveSourceFile_Proxy( 
    ICSInputSet * This,
    LPCWSTR pszFileName);


void __RPC_STUB ICSInputSet_RemoveSourceFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_RemoveAllSourceFiles_Proxy( 
    ICSInputSet * This);


void __RPC_STUB ICSInputSet_RemoveAllSourceFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_AddResourceFile_Proxy( 
    ICSInputSet * This,
    LPCWSTR pszFileName,
    LPCWSTR pszIdent,
    BOOL bEmbed,
    BOOL bVis);


void __RPC_STUB ICSInputSet_AddResourceFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_RemoveResourceFile_Proxy( 
    ICSInputSet * This,
    LPCWSTR pszFileName,
    LPCWSTR pszIdent,
    BOOL bEmbed,
    BOOL bVis);


void __RPC_STUB ICSInputSet_RemoveResourceFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);




HRESULT STDMETHODCALLTYPE ICSInputSet_SetOutputFileName_Proxy( 
    ICSInputSet * This,
    LPCWSTR pszFileName);


void __RPC_STUB ICSInputSet_SetOutputFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_SetOutputFileType_Proxy( 
    ICSInputSet * This,
    DWORD dwFileType);


void __RPC_STUB ICSInputSet_SetOutputFileType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_SetImageBase_Proxy( 
    ICSInputSet * This,
    DWORD dwImageBase);


void __RPC_STUB ICSInputSet_SetImageBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSInputSet_SetMainClass_Proxy( 
    ICSInputSet * This,
    LPCWSTR pszFQClassName);


void __RPC_STUB ICSInputSet_SetMainClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



HRESULT STDMETHODCALLTYPE ICSInputSet_SetFileAlignment_Proxy( 
    ICSInputSet * This,
    DWORD dwAlign);


void __RPC_STUB ICSInputSet_SetFileAlignment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSInputSet_INTERFACE_DEFINED__ */


#ifndef __ICSCompileProgress_INTERFACE_DEFINED__
#define __ICSCompileProgress_INTERFACE_DEFINED__

/* interface ICSCompileProgress */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSCompileProgress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("93615958-1FA4-4d6a-AA14-CB3B9A6B08AC")
    ICSCompileProgress : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportProgress( 
            LPCWSTR pszTask,
            long iTasksLeft,
            long iTotalTasks) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSCompileProgressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSCompileProgress * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSCompileProgress * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSCompileProgress * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReportProgress )( 
            ICSCompileProgress * This,
            LPCWSTR pszTask,
            long iTasksLeft,
            long iTotalTasks);
        
        END_INTERFACE
    } ICSCompileProgressVtbl;

    interface ICSCompileProgress
    {
        CONST_VTBL struct ICSCompileProgressVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSCompileProgress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSCompileProgress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSCompileProgress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSCompileProgress_ReportProgress(This,pszTask,iTasksLeft,iTotalTasks)	\
    (This)->lpVtbl -> ReportProgress(This,pszTask,iTasksLeft,iTotalTasks)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSCompileProgress_ReportProgress_Proxy( 
    ICSCompileProgress * This,
    LPCWSTR pszTask,
    long iTasksLeft,
    long iTotalTasks);


void __RPC_STUB ICSCompileProgress_ReportProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSCompileProgress_INTERFACE_DEFINED__ */


#ifndef __ICSCompiler_INTERFACE_DEFINED__
#define __ICSCompiler_INTERFACE_DEFINED__

/* interface ICSCompiler */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSCompiler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C7559EF8-E098-11d2-B554-00C04F68D4DB")
    ICSCompiler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateSourceModule( 
            ICSSourceText *pText,
            ICSSourceModule **ppModule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNameTable( 
            ICSNameTable **ppNameTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfiguration( 
            ICSCompilerConfig **ppConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddInputSet( 
            ICSInputSet **ppInputSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveInputSet( 
            ICSInputSet *pInputSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Compile( 
            ICSCompileProgress *pProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFileName( 
            LPCWSTR *ppszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateParser( 
            ICSParser **ppParser) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSCompilerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSCompiler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSCompiler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSCompiler * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSourceModule )( 
            ICSCompiler * This,
            ICSSourceText *pText,
            ICSSourceModule **ppModule);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameTable )( 
            ICSCompiler * This,
            ICSNameTable **ppNameTable);
        
        HRESULT ( STDMETHODCALLTYPE *Shutdown )( 
            ICSCompiler * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfiguration )( 
            ICSCompiler * This,
            ICSCompilerConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *AddInputSet )( 
            ICSCompiler * This,
            ICSInputSet **ppInputSet);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveInputSet )( 
            ICSCompiler * This,
            ICSInputSet *pInputSet);
        
        HRESULT ( STDMETHODCALLTYPE *Compile )( 
            ICSCompiler * This,
            ICSCompileProgress *pProgress);
        
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputFileName )( 
            ICSCompiler * This,
            LPCWSTR *ppszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *CreateParser )( 
            ICSCompiler * This,
            ICSParser **ppParser);
        
        END_INTERFACE
    } ICSCompilerVtbl;

    interface ICSCompiler
    {
        CONST_VTBL struct ICSCompilerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSCompiler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSCompiler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSCompiler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSCompiler_CreateSourceModule(This,pText,ppModule)	\
    (This)->lpVtbl -> CreateSourceModule(This,pText,ppModule)

#define ICSCompiler_GetNameTable(This,ppNameTable)	\
    (This)->lpVtbl -> GetNameTable(This,ppNameTable)

#define ICSCompiler_Shutdown(This)	\
    (This)->lpVtbl -> Shutdown(This)

#define ICSCompiler_GetConfiguration(This,ppConfig)	\
    (This)->lpVtbl -> GetConfiguration(This,ppConfig)

#define ICSCompiler_AddInputSet(This,ppInputSet)	\
    (This)->lpVtbl -> AddInputSet(This,ppInputSet)

#define ICSCompiler_RemoveInputSet(This,pInputSet)	\
    (This)->lpVtbl -> RemoveInputSet(This,pInputSet)

#define ICSCompiler_Compile(This,pProgress)	\
    (This)->lpVtbl -> Compile(This,pProgress)


#define ICSCompiler_GetOutputFileName(This,ppszFileName)	\
    (This)->lpVtbl -> GetOutputFileName(This,ppszFileName)

#define ICSCompiler_CreateParser(This,ppParser)	\
    (This)->lpVtbl -> CreateParser(This,ppParser)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSCompiler_CreateSourceModule_Proxy( 
    ICSCompiler * This,
    ICSSourceText *pText,
    ICSSourceModule **ppModule);


void __RPC_STUB ICSCompiler_CreateSourceModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompiler_GetNameTable_Proxy( 
    ICSCompiler * This,
    ICSNameTable **ppNameTable);


void __RPC_STUB ICSCompiler_GetNameTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompiler_Shutdown_Proxy( 
    ICSCompiler * This);


void __RPC_STUB ICSCompiler_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompiler_GetConfiguration_Proxy( 
    ICSCompiler * This,
    ICSCompilerConfig **ppConfig);


void __RPC_STUB ICSCompiler_GetConfiguration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompiler_AddInputSet_Proxy( 
    ICSCompiler * This,
    ICSInputSet **ppInputSet);


void __RPC_STUB ICSCompiler_AddInputSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompiler_RemoveInputSet_Proxy( 
    ICSCompiler * This,
    ICSInputSet *pInputSet);


void __RPC_STUB ICSCompiler_RemoveInputSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompiler_Compile_Proxy( 
    ICSCompiler * This,
    ICSCompileProgress *pProgress);


void __RPC_STUB ICSCompiler_Compile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



HRESULT STDMETHODCALLTYPE ICSCompiler_GetOutputFileName_Proxy( 
    ICSCompiler * This,
    LPCWSTR *ppszFileName);


void __RPC_STUB ICSCompiler_GetOutputFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompiler_CreateParser_Proxy( 
    ICSCompiler * This,
    ICSParser **ppParser);


void __RPC_STUB ICSCompiler_CreateParser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSCompiler_INTERFACE_DEFINED__ */


#ifndef __ICSCompilerFactory_INTERFACE_DEFINED__
#define __ICSCompilerFactory_INTERFACE_DEFINED__

/* interface ICSCompilerFactory */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSCompilerFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("694DD9B6-B865-4c5b-AD85-86356E9C88DC")
    ICSCompilerFactory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateNameTable( 
            ICSNameTable **ppTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCompiler( 
            DWORD dwFlags,
            ICSCompilerHost *pHost,
            ICSNameTable *pNameTable,
            ICSCompiler **ppCompiler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateLexer( 
            ICSNameTable *pNameTable,
            ICSLexer **ppLexer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckVersion( 
            HINSTANCE hInstance,
            BSTR *pbstrError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSCompilerFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSCompilerFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSCompilerFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSCompilerFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNameTable )( 
            ICSCompilerFactory * This,
            ICSNameTable **ppTable);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCompiler )( 
            ICSCompilerFactory * This,
            DWORD dwFlags,
            ICSCompilerHost *pHost,
            ICSNameTable *pNameTable,
            ICSCompiler **ppCompiler);
        
        HRESULT ( STDMETHODCALLTYPE *CreateLexer )( 
            ICSCompilerFactory * This,
            ICSNameTable *pNameTable,
            ICSLexer **ppLexer);
        
        HRESULT ( STDMETHODCALLTYPE *CheckVersion )( 
            ICSCompilerFactory * This,
            HINSTANCE hInstance,
            BSTR *pbstrError);
        
        END_INTERFACE
    } ICSCompilerFactoryVtbl;

    interface ICSCompilerFactory
    {
        CONST_VTBL struct ICSCompilerFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSCompilerFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSCompilerFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSCompilerFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSCompilerFactory_CreateNameTable(This,ppTable)	\
    (This)->lpVtbl -> CreateNameTable(This,ppTable)

#define ICSCompilerFactory_CreateCompiler(This,dwFlags,pHost,pNameTable,ppCompiler)	\
    (This)->lpVtbl -> CreateCompiler(This,dwFlags,pHost,pNameTable,ppCompiler)

#define ICSCompilerFactory_CreateLexer(This,pNameTable,ppLexer)	\
    (This)->lpVtbl -> CreateLexer(This,pNameTable,ppLexer)

#define ICSCompilerFactory_CheckVersion(This,hInstance,pbstrError)	\
    (This)->lpVtbl -> CheckVersion(This,hInstance,pbstrError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSCompilerFactory_CreateNameTable_Proxy( 
    ICSCompilerFactory * This,
    ICSNameTable **ppTable);


void __RPC_STUB ICSCompilerFactory_CreateNameTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerFactory_CreateCompiler_Proxy( 
    ICSCompilerFactory * This,
    DWORD dwFlags,
    ICSCompilerHost *pHost,
    ICSNameTable *pNameTable,
    ICSCompiler **ppCompiler);


void __RPC_STUB ICSCompilerFactory_CreateCompiler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerFactory_CreateLexer_Proxy( 
    ICSCompilerFactory * This,
    ICSNameTable *pNameTable,
    ICSLexer **ppLexer);


void __RPC_STUB ICSCompilerFactory_CreateLexer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSCompilerFactory_CheckVersion_Proxy( 
    ICSCompilerFactory * This,
    HINSTANCE hInstance,
    BSTR *pbstrError);


void __RPC_STUB ICSCompilerFactory_CheckVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSCompilerFactory_INTERFACE_DEFINED__ */


#ifndef __ICSharpProjectSite_INTERFACE_DEFINED__
#define __ICSharpProjectSite_INTERFACE_DEFINED__

/* interface ICSharpProjectSite */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSharpProjectSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5D41CF84-3A90-48d6-A445-CA22E125B691")
    ICSharpProjectSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompiler( 
            ICSCompiler **ppCompiler,
            ICSInputSet **ppInputSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckInputFileTimes( 
            FILETIME *pftOutput,
            BOOL *pfMustBuild) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BuildProject( 
            ICSCompileProgress *pProgress) = 0;
        
        
        virtual HRESULT STDMETHODCALLTYPE OnSourceFileAdded( 
            LPCWSTR pszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSourceFileRemoved( 
            LPCWSTR pszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnResourceFileAdded( 
            LPCWSTR pszFilename,
            LPCWSTR wszResourceName,
            BOOL bEmbedded) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnResourceFileRemoved( 
            LPCWSTR pszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnImportAdded( 
            LPCWSTR pszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnImportRemoved( 
            LPCWSTR pszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnOutputFileChanged( 
            LPCWSTR pszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnActiveConfigurationChanged( 
            LPCWSTR pszConfigName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnProjectLoadCompletion( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCodeModel( 
            IUnknown *pParent,
            void **ppCodeModel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateFileCodeModel( 
            LPCWSTR pszFileName,
            IUnknown *pParent,
            void **ppFileCodeModel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnModuleAdded( 
            LPCWSTR pszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnModuleRemoved( 
            LPCWSTR pszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValidStartupClasses( 
            LPCWSTR *ppszClassNames,
            long *piCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSharpProjectSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSharpProjectSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSharpProjectSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSharpProjectSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            ICSharpProjectSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCompiler )( 
            ICSharpProjectSite * This,
            ICSCompiler **ppCompiler,
            ICSInputSet **ppInputSet);
        
        HRESULT ( STDMETHODCALLTYPE *CheckInputFileTimes )( 
            ICSharpProjectSite * This,
            FILETIME *pftOutput,
            BOOL *pfMustBuild);
        
        HRESULT ( STDMETHODCALLTYPE *BuildProject )( 
            ICSharpProjectSite * This,
            ICSCompileProgress *pProgress);
        
        
        HRESULT ( STDMETHODCALLTYPE *OnSourceFileAdded )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *OnSourceFileRemoved )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *OnResourceFileAdded )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename,
            LPCWSTR wszResourceName,
            BOOL bEmbedded);
        
        HRESULT ( STDMETHODCALLTYPE *OnResourceFileRemoved )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *OnImportAdded )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *OnImportRemoved )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *OnOutputFileChanged )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *OnActiveConfigurationChanged )( 
            ICSharpProjectSite * This,
            LPCWSTR pszConfigName);
        
        HRESULT ( STDMETHODCALLTYPE *OnProjectLoadCompletion )( 
            ICSharpProjectSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCodeModel )( 
            ICSharpProjectSite * This,
            IUnknown *pParent,
            void **ppCodeModel);
        
        HRESULT ( STDMETHODCALLTYPE *CreateFileCodeModel )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFileName,
            IUnknown *pParent,
            void **ppFileCodeModel);
        
        HRESULT ( STDMETHODCALLTYPE *OnModuleAdded )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *OnModuleRemoved )( 
            ICSharpProjectSite * This,
            LPCWSTR pszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *GetValidStartupClasses )( 
            ICSharpProjectSite * This,
            LPCWSTR *ppszClassNames,
            long *piCount);
        
        END_INTERFACE
    } ICSharpProjectSiteVtbl;

    interface ICSharpProjectSite
    {
        CONST_VTBL struct ICSharpProjectSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSharpProjectSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSharpProjectSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSharpProjectSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSharpProjectSite_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define ICSharpProjectSite_GetCompiler(This,ppCompiler,ppInputSet)	\
    (This)->lpVtbl -> GetCompiler(This,ppCompiler,ppInputSet)

#define ICSharpProjectSite_CheckInputFileTimes(This,pftOutput,pfMustBuild)	\
    (This)->lpVtbl -> CheckInputFileTimes(This,pftOutput,pfMustBuild)

#define ICSharpProjectSite_BuildProject(This,pProgress)	\
    (This)->lpVtbl -> BuildProject(This,pProgress)


#define ICSharpProjectSite_OnSourceFileAdded(This,pszFilename)	\
    (This)->lpVtbl -> OnSourceFileAdded(This,pszFilename)

#define ICSharpProjectSite_OnSourceFileRemoved(This,pszFilename)	\
    (This)->lpVtbl -> OnSourceFileRemoved(This,pszFilename)

#define ICSharpProjectSite_OnResourceFileAdded(This,pszFilename,wszResourceName,bEmbedded)	\
    (This)->lpVtbl -> OnResourceFileAdded(This,pszFilename,wszResourceName,bEmbedded)

#define ICSharpProjectSite_OnResourceFileRemoved(This,pszFilename)	\
    (This)->lpVtbl -> OnResourceFileRemoved(This,pszFilename)

#define ICSharpProjectSite_OnImportAdded(This,pszFilename)	\
    (This)->lpVtbl -> OnImportAdded(This,pszFilename)

#define ICSharpProjectSite_OnImportRemoved(This,pszFilename)	\
    (This)->lpVtbl -> OnImportRemoved(This,pszFilename)

#define ICSharpProjectSite_OnOutputFileChanged(This,pszFileName)	\
    (This)->lpVtbl -> OnOutputFileChanged(This,pszFileName)

#define ICSharpProjectSite_OnActiveConfigurationChanged(This,pszConfigName)	\
    (This)->lpVtbl -> OnActiveConfigurationChanged(This,pszConfigName)

#define ICSharpProjectSite_OnProjectLoadCompletion(This)	\
    (This)->lpVtbl -> OnProjectLoadCompletion(This)

#define ICSharpProjectSite_CreateCodeModel(This,pParent,ppCodeModel)	\
    (This)->lpVtbl -> CreateCodeModel(This,pParent,ppCodeModel)

#define ICSharpProjectSite_CreateFileCodeModel(This,pszFileName,pParent,ppFileCodeModel)	\
    (This)->lpVtbl -> CreateFileCodeModel(This,pszFileName,pParent,ppFileCodeModel)

#define ICSharpProjectSite_OnModuleAdded(This,pszFilename)	\
    (This)->lpVtbl -> OnModuleAdded(This,pszFilename)

#define ICSharpProjectSite_OnModuleRemoved(This,pszFilename)	\
    (This)->lpVtbl -> OnModuleRemoved(This,pszFilename)

#define ICSharpProjectSite_GetValidStartupClasses(This,ppszClassNames,piCount)	\
    (This)->lpVtbl -> GetValidStartupClasses(This,ppszClassNames,piCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSharpProjectSite_Disconnect_Proxy( 
    ICSharpProjectSite * This);


void __RPC_STUB ICSharpProjectSite_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_GetCompiler_Proxy( 
    ICSharpProjectSite * This,
    ICSCompiler **ppCompiler,
    ICSInputSet **ppInputSet);


void __RPC_STUB ICSharpProjectSite_GetCompiler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_CheckInputFileTimes_Proxy( 
    ICSharpProjectSite * This,
    FILETIME *pftOutput,
    BOOL *pfMustBuild);


void __RPC_STUB ICSharpProjectSite_CheckInputFileTimes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_BuildProject_Proxy( 
    ICSharpProjectSite * This,
    ICSCompileProgress *pProgress);


void __RPC_STUB ICSharpProjectSite_BuildProject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);




HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnSourceFileAdded_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename);


void __RPC_STUB ICSharpProjectSite_OnSourceFileAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnSourceFileRemoved_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename);


void __RPC_STUB ICSharpProjectSite_OnSourceFileRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnResourceFileAdded_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename,
    LPCWSTR wszResourceName,
    BOOL bEmbedded);


void __RPC_STUB ICSharpProjectSite_OnResourceFileAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnResourceFileRemoved_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename);


void __RPC_STUB ICSharpProjectSite_OnResourceFileRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnImportAdded_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename);


void __RPC_STUB ICSharpProjectSite_OnImportAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnImportRemoved_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename);


void __RPC_STUB ICSharpProjectSite_OnImportRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnOutputFileChanged_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFileName);


void __RPC_STUB ICSharpProjectSite_OnOutputFileChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnActiveConfigurationChanged_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszConfigName);


void __RPC_STUB ICSharpProjectSite_OnActiveConfigurationChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnProjectLoadCompletion_Proxy( 
    ICSharpProjectSite * This);


void __RPC_STUB ICSharpProjectSite_OnProjectLoadCompletion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_CreateCodeModel_Proxy( 
    ICSharpProjectSite * This,
    IUnknown *pParent,
    void **ppCodeModel);


void __RPC_STUB ICSharpProjectSite_CreateCodeModel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_CreateFileCodeModel_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFileName,
    IUnknown *pParent,
    void **ppFileCodeModel);


void __RPC_STUB ICSharpProjectSite_CreateFileCodeModel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnModuleAdded_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename);


void __RPC_STUB ICSharpProjectSite_OnModuleAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_OnModuleRemoved_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR pszFilename);


void __RPC_STUB ICSharpProjectSite_OnModuleRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpProjectSite_GetValidStartupClasses_Proxy( 
    ICSharpProjectSite * This,
    LPCWSTR *ppszClassNames,
    long *piCount);


void __RPC_STUB ICSharpProjectSite_GetValidStartupClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSharpProjectSite_INTERFACE_DEFINED__ */



#ifndef __ICSharpBuildMgrSite_INTERFACE_DEFINED__
#define __ICSharpBuildMgrSite_INTERFACE_DEFINED__

/* interface ICSharpBuildMgrSite */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSharpBuildMgrSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6FF8B47A-5D24-4c8e-BB21-1A60CCFF9A8F")
    ICSharpBuildMgrSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitiateShutdown( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToHost( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSharpBuildMgrSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSharpBuildMgrSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSharpBuildMgrSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSharpBuildMgrSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitiateShutdown )( 
            ICSharpBuildMgrSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *BindToHost )( 
            ICSharpBuildMgrSite * This);
        
        END_INTERFACE
    } ICSharpBuildMgrSiteVtbl;

    interface ICSharpBuildMgrSite
    {
        CONST_VTBL struct ICSharpBuildMgrSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSharpBuildMgrSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSharpBuildMgrSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSharpBuildMgrSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSharpBuildMgrSite_InitiateShutdown(This)	\
    (This)->lpVtbl -> InitiateShutdown(This)

#define ICSharpBuildMgrSite_BindToHost(This)	\
    (This)->lpVtbl -> BindToHost(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSharpBuildMgrSite_InitiateShutdown_Proxy( 
    ICSharpBuildMgrSite * This);


void __RPC_STUB ICSharpBuildMgrSite_InitiateShutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpBuildMgrSite_BindToHost_Proxy( 
    ICSharpBuildMgrSite * This);


void __RPC_STUB ICSharpBuildMgrSite_BindToHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSharpBuildMgrSite_INTERFACE_DEFINED__ */


#ifndef __ICSharpProjectHost_INTERFACE_DEFINED__
#define __ICSharpProjectHost_INTERFACE_DEFINED__

/* interface ICSharpProjectHost */
/* [object][uuid] */ 


EXTERN_C const IID IID_ICSharpProjectHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1F3B9583-A66A-4be1-A15B-901DA4DB4ACF")
    ICSharpProjectHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BindToProject( 
            ICSharpProjectRoot *pProject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSharpProjectHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSharpProjectHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSharpProjectHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSharpProjectHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *BindToProject )( 
            ICSharpProjectHost * This,
            ICSharpProjectRoot *pProject);
        
        END_INTERFACE
    } ICSharpProjectHostVtbl;

    interface ICSharpProjectHost
    {
        CONST_VTBL struct ICSharpProjectHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSharpProjectHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSharpProjectHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSharpProjectHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSharpProjectHost_BindToProject(This,pProject)	\
    (This)->lpVtbl -> BindToProject(This,pProject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSharpProjectHost_BindToProject_Proxy( 
    ICSharpProjectHost * This,
    ICSharpProjectRoot *pProject);


void __RPC_STUB ICSharpProjectHost_BindToProject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSharpProjectHost_INTERFACE_DEFINED__ */




EXTERN_C const CLSID CLSID_CSharpGeneral;

#ifdef __cplusplus

class DECLSPEC_UUID("BA33721B-ABEC-4f28-81FB-007BBCF60BB9")
CSharpGeneral;
#endif

EXTERN_C const CLSID CLSID_CSharpVersion;

#ifdef __cplusplus

class DECLSPEC_UUID("9EC55CFE-5112-457a-BDD5-78DA54A0D412")
CSharpVersion;
#endif

EXTERN_C const CLSID CLSID_CSharpWebSettings;

#ifdef __cplusplus

class DECLSPEC_UUID("98715071-E40C-409d-9AEE-E31032E2709C")
CSharpWebSettings;
#endif

EXTERN_C const CLSID CLSID_CSharpDesigner;

#ifdef __cplusplus

class DECLSPEC_UUID("F65276D3-B79E-4fc9-BB11-3EFCEE90CE40")
CSharpDesigner;
#endif

EXTERN_C const CLSID CLSID_CSharpCommonBuild;

#ifdef __cplusplus

class DECLSPEC_UUID("BE9317FF-57B3-44a0-96DF-B257FAE43DB1")
CSharpCommonBuild;
#endif

EXTERN_C const CLSID CLSID_CSharpBuild;

#ifdef __cplusplus

class DECLSPEC_UUID("94E9C3FE-2BD9-4ead-AAAC-33C46C20FC81")
CSharpBuild;
#endif

EXTERN_C const CLSID CLSID_CSharpAdvanced;

#ifdef __cplusplus

class DECLSPEC_UUID("61F3E62C-A68E-49e0-9F88-D8EE8B7ACB95")
CSharpAdvanced;
#endif

EXTERN_C const CLSID CLSID_CSharpDebug;

#ifdef __cplusplus

class DECLSPEC_UUID("521DF5B1-EC68-4de3-8415-ABA698D35AFD")
CSharpDebug;
#endif

EXTERN_C const CLSID CLSID_CSCompilerFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("6A2E6C75-9F96-48d3-8AE0-B5DCC5B1F0A4")
CSCompilerFactory;
#endif
#endif /* __CSharp_LIBRARY_DEFINED__ */

#ifndef __ICSharpSettingsPage_INTERFACE_DEFINED__
#define __ICSharpSettingsPage_INTERFACE_DEFINED__

/* interface ICSharpSettingsPage */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICSharpSettingsPage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("51951F99-B921-4e8f-963B-017969E09F99")
    ICSharpSettingsPage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Dirty( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICSharpSettingsPageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICSharpSettingsPage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICSharpSettingsPage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICSharpSettingsPage * This);
        
        HRESULT ( STDMETHODCALLTYPE *Dirty )( 
            ICSharpSettingsPage * This);
        
        HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ICSharpSettingsPage * This);
        
        END_INTERFACE
    } ICSharpSettingsPageVtbl;

    interface ICSharpSettingsPage
    {
        CONST_VTBL struct ICSharpSettingsPageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICSharpSettingsPage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICSharpSettingsPage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICSharpSettingsPage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICSharpSettingsPage_Dirty(This)	\
    (This)->lpVtbl -> Dirty(This)

#define ICSharpSettingsPage_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICSharpSettingsPage_Dirty_Proxy( 
    ICSharpSettingsPage * This);


void __RPC_STUB ICSharpSettingsPage_Dirty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICSharpSettingsPage_Refresh_Proxy( 
    ICSharpSettingsPage * This);


void __RPC_STUB ICSharpSettingsPage_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICSharpSettingsPage_INTERFACE_DEFINED__ */



/* interface __MIDL_itf_scciface_0716 */
/* [local] */
SCCOMP_API HRESULT CreateCompilerFactory (ICSCompilerFactory **ppFactory);
SCCOMP_API HINSTANCE GetMessageDll();
#define SID_SCSharpLangService IID_ICSCompilerFactory
EXTERN_C const IID IID_ICSharpProjCommonPropertyTable;
EXTERN_C const IID IID_ICSharpProjConfigPropertyTable;
EXTERN_C const IID IID_CCSharpTempPECompiler;


extern RPC_IF_HANDLE __MIDL_itf_scciface_0716_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_scciface_0716_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


