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
#include "common.h"
#include "comclass.h"
#include "commodule.h"
#include "commember.h"
#include "comdynamic.h"
#include "comclass.h"
#include "reflectclasswriter.h"
#include "class.h"
#include "corpolicy.h"
#include "security.h"
#include "gcscan.h"
#include "ceesectionstring.h"
#include "comvariant.h"
#include <cor.h>
#include "reflectutil.h"

//
// NOTE: this macro must be used prior to entering a helper method frame
//

#define GET_MODULE(pThis) \
    ((Module*)((REFLECTMODULEBASEREF)ObjectToOBJECTREF(pThis))->GetData())

#define STATE_EMPTY 0
#define STATE_ARRAY 1

//SIG_* are defined in DescriptorInfo.cs and must be kept in sync.
#define SIG_BYREF        0x0001
#define SIG_DEFAULTVALUE 0x0002
#define SIG_IN           0x0004
#define SIG_INOUT        0x0008
#define SIG_STANDARD     0x0001
#define SIG_VARARGS      0x0002

// This function will help clean up after a ISymUnmanagedWriter (if it can't
// clean up on it's own
void __stdcall CleanUpAfterISymUnmanagedWriter(void * data)
{
    CGrowableStream * s = (CGrowableStream*)data;
    s->Release();
}// CleanUpAfterISymUnmanagedWriter
    

//inline Module *COMModule::ValidateThisRef(REFLECTMODULEBASEREF pThis)
//{
//    THROWSCOMPLUSEXCEPTION();
//
//    if (pThis == NULL)
//        COMPlusThrow(kNullReferenceException, L"NullReference_This");
//
//    Module* pModule = (Module*) pThis->GetData();
//    _ASSERTE(pModule);  
//    return pModule;
//}    

//****************************************
// This function creates a dynamic module underneath the current assembly.
//****************************************
FCIMPL4(Object*, COMModule::DefineDynamicModule, AssemblyBaseObject* containingAssemblyUNSAFE, BYTE emitSymbolInfo, StringObject* filenameUNSAFE, StackCrawlMark* stackMark)
{
    THROWSCOMPLUSEXCEPTION();   

    ASSEMBLYREF containingAssembly  = (ASSEMBLYREF) containingAssemblyUNSAFE;
    STRINGREF   filename            = (STRINGREF)   filenameUNSAFE;
    OBJECTREF   refModule           = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, containingAssembly, filename);

    Assembly        *pAssembly;
    InMemoryModule *mod;

    _ASSERTE(containingAssembly);
    pAssembly = containingAssembly->GetAssembly();
    _ASSERTE(pAssembly);

    // We should never get a dynamic module for the system domain
    _ASSERTE(pAssembly->Parent() != SystemDomain::System());

    // always create a dynamic module. Note that the name conflict
    // checking is done in managed side.
    mod = COMDynamicWrite::CreateDynamicModule(pAssembly,
                                               filename);

    mod->SetCreatingAssembly( SystemDomain::GetCallersAssembly( stackMark ) );

    // get the corresponding managed ModuleBuilder class
    refModule = (OBJECTREF) mod->GetExposedModuleBuilderObject();  
    _ASSERTE(refModule);    

    // If we need to emit symbol info, we setup the proper symbol
    // writer for this module now.
    if (emitSymbolInfo)
    {
        WCHAR* filenameTemp = NULL;
        
        if ((filename != NULL) &&
            (filename->GetStringLength() > 0))
            filenameTemp = filename->GetBuffer();
        
        _ASSERTE(mod->IsReflection());
        ReflectionModule *rm = mod->GetReflectionModule();
        
        // Create a stream for the symbols to be emitted into. This
        // lives on the Module for the life of the Module.
        CGrowableStream *pStream = new CGrowableStream();
        //pStream->AddRef(); // The Module will keep a copy for it's own use.
        mod->SetInMemorySymbolStream(pStream);

        // Create an ISymUnmanagedWriter and initialize it with the
        // stream and the proper file name. This symbol writer will be
        // replaced with new ones periodically as the symbols get
        // retrieved by the debugger.
        ISymUnmanagedWriter *pWriter;
        
        HRESULT hr = FakeCoCreateInstance(CLSID_CorSymWriter,
                                          IID_ISymUnmanagedWriter,
                                          (void**)&pWriter);

        if (SUCCEEDED(hr))
        {
            // The other reference is given to the Sym Writer
            // But, the writer takes it's own reference.
            hr = pWriter->Initialize(mod->GetEmitter(),
                                     filenameTemp,
                                     (IStream*)pStream,
                                     TRUE);

            if (SUCCEEDED(hr))
            {
                // Send along some cleanup information
                HelpForInterfaceCleanup *hlp = new HelpForInterfaceCleanup;
                hlp->pData = pStream;
                hlp->pFunction = CleanUpAfterISymUnmanagedWriter;
            
                rm->SetISymUnmanagedWriter(pWriter, hlp);

                // Remember the address of where we've got our
                // ISymUnmanagedWriter stored so we can pass it over
                // to the managed symbol writer object that most of
                // reflection emit will use to write symbols.
                REFLECTMODULEBASEREF ro = (REFLECTMODULEBASEREF)refModule;
                ro->SetInternalSymWriter(rm->GetISymUnmanagedWriterAddr());
            }
        }
        else
        {
            COMPlusThrowHR(hr);
        }
    }
    
    HELPER_METHOD_FRAME_END();

    // Assign the return value  
    return OBJECTREFToObject(refModule);
}
FCIMPLEND

// IsDynamic
// This method will return a boolean value indicating if the Module
//  supports Dynamic IL.    
FCIMPL1(INT32, COMModule::IsDynamic, ReflectModuleBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);
    return (pModule->IsReflection()) ? 1 : 0;  
}
FCIMPLEND

// GetCaller
FCIMPL1(Object*, COMModule::GetCaller, StackCrawlMark* stackMark)
{
    THROWSCOMPLUSEXCEPTION();
    
    OBJECTREF refModule = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refModule);

    // Assign the return value

    Module* pModule = SystemDomain::GetCallersModule(stackMark);
    if(pModule != NULL) 
    {
        refModule = (OBJECTREF) pModule->GetExposedModuleObject();
    }
    // Return the object

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refModule);
}
FCIMPLEND


//**************************************************
// LoadInMemoryTypeByName
// Explicitly loading an in memory type
//**************************************************
FCIMPL2(Object*, COMModule::LoadInMemoryTypeByName, ReflectModuleBaseObject* refThisUNSAFE, StringObject* strFullNameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();


    OBJECTREF       Throwable = NULL;
    OBJECTREF       ret = NULL;
    STRINGREF           strFullName = (STRINGREF) strFullNameUNSAFE;

    TypeHandle      typeHnd;
    UINT            resId = IDS_CLASSLOAD_GENERIC;
    IMetaDataImport *pImport = NULL;
    RefClassWriter  *pRCW;
    mdTypeDef       td;
    LPCWSTR         wzFullName;
    HRESULT         hr = S_OK;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*             pThisModule = GET_MODULE(refThisUNSAFE);
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, Throwable, strFullName);


    if (!pThisModule->IsReflection())
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");

    pRCW = ((ReflectionModule*) pThisModule)->GetClassWriter();
    _ASSERTE(pRCW);

    // it is ok to use public import API because this is a dynamic module anyway. We are also receiving Unicode full name as
    // parameter.
    pImport = pRCW->GetImporter();

    wzFullName = strFullName->GetBuffer();
    if (wzFullName == NULL)
        IfFailGo( E_FAIL );

    // look up the handle
    IfFailGo( pImport->FindTypeDefByName(wzFullName, mdTokenNil, &td) );     

    BEGIN_ENSURE_PREEMPTIVE_GC();
    typeHnd = pThisModule->GetClassLoader()->LoadTypeHandle(pThisModule, td, &Throwable);
    END_ENSURE_PREEMPTIVE_GC();

    if (typeHnd.IsNull() ||
        (Throwable != NULL) ||
        (typeHnd.GetModule() != pThisModule))
        goto ErrExit;
    ret = typeHnd.CreateClassObj();
ErrExit:
    if (FAILED(hr) && (hr != CLDB_E_RECORD_NOTFOUND))
        COMPlusThrowHR(hr);

    if (ret == NULL) 
    {
        if (Throwable == NULL)
        {
            CQuickBytes bytes;
            LPSTR szClassName;
            DWORD cClassName;

            // Get the UTF8 version of strFullName
            szClassName = GetClassStringVars(strFullName, &bytes, 
                                             &cClassName, true);
            pThisModule->GetAssembly()->PostTypeLoadException(szClassName, resId, &Throwable);
        }
        COMPlusThrow(Throwable);
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(ret);

}
FCIMPLEND

//**************************************************
// GetClassToken
// This function will return the type token given full qual name. If the type
// is defined locally, we will return the TypeDef token. Or we will return a TypeRef token 
// with proper resolution scope calculated.
//**************************************************
FCIMPL5(mdTypeRef, COMModule::GetClassToken, ReflectModuleBaseObject*   refThisUNSAFE, 
                                             StringObject*              strFullNameUNSAFE, 
                                             ReflectModuleBaseObject*   refedModuleUNSAFE, 
                                             StringObject*              strRefedModuleFileNameUNSAFE,
                                             INT32 tkResolutionArg)
{
    THROWSCOMPLUSEXCEPTION();

    mdTypeRef               tr              = 0;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*                 pThisModule     = GET_MODULE(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();

    RefClassWriter      *pRCW;
    HRESULT             hr;
    mdToken             tkResolution = mdTokenNil;
    Module              *pRefedModule;
    Assembly            *pThisAssembly;
    Assembly            *pRefedAssembly;
    IMetaDataEmit       *pEmit;
    IMetaDataImport     *pImport;
    IMetaDataAssemblyEmit *pAssemblyEmit = NULL;

    struct _gc
    {
        STRINGREF               strFullName;
        REFLECTMODULEBASEREF    refedModule;
        STRINGREF               strRefedModuleFileName;
    } gc;

    gc.strFullName              = (STRINGREF)               strFullNameUNSAFE;
    gc.refedModule              = (REFLECTMODULEBASEREF)    refedModuleUNSAFE;
    gc.strRefedModuleFileName   = (STRINGREF)               strRefedModuleFileNameUNSAFE;


    GCPROTECT_BEGIN(gc);

    if (!pThisModule->IsReflection())
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");

    pRCW = ((ReflectionModule*) pThisModule)->GetClassWriter();
    _ASSERTE(pRCW);

    pEmit = pRCW->GetEmitter(); 
    pImport = pRCW->GetImporter();

    if (gc.strFullName == NULL) {
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    }    

    _ASSERTE(gc.refedModule);

    pRefedModule = (Module*) gc.refedModule->GetData();
    _ASSERTE(pRefedModule);

    pThisAssembly = pThisModule->GetClassLoader()->GetAssembly();
    pRefedAssembly = pRefedModule->GetClassLoader()->GetAssembly();
    if (pThisModule == pRefedModule)
    {
        // referenced type is from the same module so we must be able to find a TypeDef.
        hr = pImport->FindTypeDefByName(
            gc.strFullName->GetBuffer(),
            RidFromToken(tkResolutionArg) ? tkResolutionArg : mdTypeDefNil,
            &tr); 

        // We should not fail in find the TypeDef. If we do, something is wrong in our managed code.
        _ASSERTE(SUCCEEDED(hr));
        goto ErrExit;
    }

    if (RidFromToken(tkResolutionArg))
    {
        // reference to nested type
        tkResolution = tkResolutionArg;
    }
    else
    {
        // reference to top level type
        if ( pThisAssembly != pRefedAssembly )
        {
            // Generate AssemblyRef
            IfFailGo( pEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );
            tkResolution = pThisAssembly->AddAssemblyRef(pRefedAssembly, pAssemblyEmit);

            // Add the assembly ref token and the manifest module it is referring to this module's rid map.
            if( !pThisModule->StoreAssemblyRef(tkResolution, pRefedAssembly) )
            {
                IfFailGo(E_OUTOFMEMORY);
            }
        }
        else
        {
            _ASSERTE(pThisModule != pRefedModule);
            // Generate ModuleRef
            if (gc.strRefedModuleFileName != NULL)
            {
                IfFailGo(pEmit->DefineModuleRef(gc.strRefedModuleFileName->GetBuffer(), &tkResolution));
            }
            else
            {
                _ASSERTE(!"E_NYI!");
                COMPlusThrow(kInvalidOperationException, L"InvalidOperation_MetaDataError");    
            }
        }
    }

    IfFailGo( pEmit->DefineTypeRefByName(tkResolution, gc.strFullName->GetBuffer(), &tr) );  
ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();
    if (FAILED(hr))
    {
        // failed in defining PInvokeMethod
        if (hr == E_OUTOFMEMORY)
            COMPlusThrowOM();
        else
            COMPlusThrowHR(hr);    
    }

    GCPROTECT_END();

    HELPER_METHOD_FRAME_END_POLL();

    return tr;
}
FCIMPLEND


/*=============================GetArrayMethodToken==============================
**Action:
**Returns:
**Arguments: REFLECTMODULEBASEREF refThis
**           U1ARRAYREF     sig
**           STRINGREF      methodName
**           int            tkTypeSpec
**Exceptions:
==============================================================================*/
FCIMPL6(INT32, COMModule::GetArrayMethodToken, ReflectModuleBaseObject* refThisUNSAFE, 
                                               INT32 tkTypeSpec, 
                                               StringObject* methodNameUNSAFE, 
                                               U1Array* signatureUNSAFE,
                                               INT32 sigLength,
                                               INT32 baseToken)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    RefClassWriter* pRCW;   
    PCCOR_SIGNATURE pvSig;
    LPCWSTR         methName;
    mdMemberRef memberRefE; 
    HRESULT hr;

    STRINGREF   methodName  = (STRINGREF)   methodNameUNSAFE;
    U1ARRAYREF  signature   = (U1ARRAYREF)  signatureUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_2(methodName, signature);

    if (!methodName)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    if (!tkTypeSpec) 
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Type");

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    //Get the signature.  Because we generated it with a call to GetSignature, it's already in the current scope.
    pvSig = (PCCOR_SIGNATURE)signature->GetDataPtr();
    
    methName = methodName->GetBuffer();

    hr = pRCW->GetEmitter()->DefineMemberRef(tkTypeSpec, methName, pvSig, sigLength, &memberRefE); 
    if (FAILED(hr)) 
    {
        _ASSERTE(!"Failed on DefineMemberRef");
        COMPlusThrowHR(hr);
    }

    HELPER_METHOD_FRAME_END();

    return (INT32)memberRefE;
}
FCIMPLEND


//******************************************************************************
//
// GetMemberRefToken
// This function will return a MemberRef token given a MethodDef token and the module where the MethodDef/FieldDef is defined.
//
//******************************************************************************
FCIMPL4(INT32, COMModule::GetMemberRefToken, ReflectModuleBaseObject* refThisUNSAFE, ReflectModuleBaseObject* refedModuleUNSAFE, INT32 tr, INT32 token)
{
    THROWSCOMPLUSEXCEPTION();

    mdMemberRef             memberRefE      = 0; 

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*                 pModule         = GET_MODULE(refThisUNSAFE);
    REFLECTMODULEBASEREF    refedModule     = (REFLECTMODULEBASEREF) refedModuleUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refedModule);

    HRESULT                 hr; 
    RefClassWriter*         pRCW;   
    WCHAR*                  szName; 
    ULONG                   nameSize;    
    ULONG                   actNameSize     = 0;    
    ULONG                   cbComSig;   
    PCCOR_SIGNATURE         pvComSig;
    CQuickBytes             qbNewSig; 
    ULONG                   cbNewSig;   
    LPCUTF8                 szNameTmp;
    Module*                 pRefedModule;
    CQuickBytes             qbName;
    Assembly*               pRefedAssembly;
    Assembly*               pRefingAssembly;
    IMetaDataAssemblyEmit*  pAssemblyEmit   = NULL;
    mdTypeRef               tref;

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE( pRCW ); 

    pRefedModule = (Module *) refedModule->GetData();
    _ASSERTE( pRefedModule );
    
    if (TypeFromToken(token) == mdtMethodDef)
    {
        szNameTmp = pRefedModule->GetMDImport()->GetNameOfMethodDef(token);
        pvComSig = pRefedModule->GetMDImport()->GetSigOfMethodDef(
            token,
            &cbComSig);
    }
    else
    {
        szNameTmp = pRefedModule->GetMDImport()->GetNameOfFieldDef(token);
        pvComSig = pRefedModule->GetMDImport()->GetSigOfFieldDef(
            token,
            &cbComSig);
    }

    // translate the name to unicode string
    nameSize = (ULONG)strlen(szNameTmp);
    IfFailGo( qbName.ReSize((nameSize + 1) * sizeof(WCHAR)) );
    szName = (WCHAR *) qbName.Ptr();
    actNameSize = ::WszMultiByteToWideChar(CP_UTF8, 0, szNameTmp, -1, szName, nameSize + 1);

    // The unicode translation function cannot fail!!
    _ASSERTE(actNameSize);

    // Translate the method sig into this scope 
    //
    pRefedAssembly = pRefedModule->GetAssembly();
    pRefingAssembly = pModule->GetAssembly();
    IfFailGo( pRefingAssembly->GetSecurityModule()->GetEmitter()->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );

    IfFailGo( pRefedModule->GetMDImport()->TranslateSigWithScope(
        pRefedAssembly->GetManifestImport(), 
        NULL, 0,        // hash value
        pvComSig, 
        cbComSig, 
        pAssemblyEmit,  // Emit assembly scope.
        pRCW->GetEmitter(), 
        &qbNewSig, 
        &cbNewSig) );  

    if (TypeFromToken(tr) == mdtTypeDef)
    {
        // define a TypeRef using the TypeDef
        IfFailGo(DefineTypeRefHelper(pRCW->GetEmitter(), tr, &tref));
    }
    else 
        tref = tr;

    // Define the memberRef
    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(tref, szName, (PCCOR_SIGNATURE) qbNewSig.Ptr(), cbNewSig, &memberRefE) ); 

ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefToken failed!"); 
        COMPlusThrowHR(hr);    
    }

    HELPER_METHOD_FRAME_END();

    // assign output parameter
    return (INT32)memberRefE;
}
FCIMPLEND


//******************************************************************************
//
// Return a TypeRef token given a TypeDef token from the same emit scope
//
//******************************************************************************
HRESULT COMModule::DefineTypeRefHelper(
    IMetaDataEmit       *pEmit,         // given emit scope
    mdTypeDef           td,             // given typedef in the emit scope
    mdTypeRef           *ptr)           // return typeref
{
    IMetaDataImport     *pImport = NULL;
    WCHAR               szTypeDef[MAX_CLASSNAME_LENGTH + 1];
    mdToken             rs;             // resolution scope
    DWORD               dwFlags;
    HRESULT             hr;

    IfFailGo( pEmit->QueryInterface(IID_IMetaDataImport, (void **)&pImport) );
    IfFailGo( pImport->GetTypeDefProps(td, szTypeDef, MAX_CLASSNAME_LENGTH, NULL, &dwFlags, NULL) );
    if ( IsTdNested(dwFlags) )
    {
        mdToken         tdNested;
        IfFailGo( pImport->GetNestedClassProps(td, &tdNested) );
        IfFailGo( DefineTypeRefHelper( pEmit, tdNested, &rs) );
    }
    else
        rs = TokenFromRid( 1, mdtModule );

    IfFailGo( pEmit->DefineTypeRefByName( rs, szTypeDef, ptr) );

ErrExit:
    if (pImport)
        pImport->Release();
    return hr;
}   // DefineTypeRefHelper


//******************************************************************************
//
// Return a MemberRef token given a RuntimeMethodInfo
//
//******************************************************************************
FCIMPL3(INT32, COMModule::GetMemberRefTokenOfMethodInfo, ReflectModuleBaseObject* refThisUNSAFE, INT32 tr, ReflectBaseObject* methodUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    mdMemberRef             memberRefE      = 0; 

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*                 pModule         = GET_MODULE(refThisUNSAFE);
    REFLECTBASEREF          method          = (REFLECTBASEREF) methodUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(method);

    HRESULT         hr; 
    RefClassWriter  *pRCW;   
    ReflectMethod   *pRM;    
    MethodDesc      *pMeth;  
    WCHAR           *szName; 
    ULONG           nameSize;    
    ULONG           actNameSize = 0;    
    ULONG           cbComSig;   
    PCCOR_SIGNATURE pvComSig;
    CQuickBytes     qbNewSig; 
    ULONG           cbNewSig;   
    LPCUTF8         szNameTmp;
    CQuickBytes     qbName;
    Assembly        *pRefedAssembly;
    Assembly        *pRefingAssembly;
    IMetaDataAssemblyEmit *pAssemblyEmit = NULL;

    if (!method)  
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    pRM = (ReflectMethod*) method->GetData(); 
    _ASSERTE(pRM);  
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    // Otherwise, we want to return memberref token.
    if (pMeth->IsArray())
    {    
        _ASSERTE(!"Should not have come here!");
        COMPlusThrow(kNotSupportedException);    
    }
    if (pMeth->GetClass())
    {
        if (pMeth->GetClass()->GetModule() == pModule)
        {
            // If the passed in method is defined in the same module, just return the MethodDef token           
            memberRefE = pMeth->GetMemberDef();
            goto lExit;
        }
    }

    szNameTmp = pMeth->GetMDImport()->GetNameOfMethodDef(pMeth->GetMemberDef());
    pvComSig = pMeth->GetMDImport()->GetSigOfMethodDef(
        pMeth->GetMemberDef(),
        &cbComSig);

    // Translate the method sig into this scope 
    pRefedAssembly = pMeth->GetModule()->GetAssembly();
    pRefingAssembly = pModule->GetAssembly();
    IfFailGo( pRefingAssembly->GetSecurityModule()->GetEmitter()->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );

    IfFailGo( pMeth->GetMDImport()->TranslateSigWithScope(
        pRefedAssembly->GetManifestImport(), 
        NULL, 0,        // hash blob value
        pvComSig, 
        cbComSig, 
        pAssemblyEmit,  // Emit assembly scope.
        pRCW->GetEmitter(), 
        &qbNewSig, 
        &cbNewSig) );  

    // translate the name to unicode string
    nameSize = (ULONG)strlen(szNameTmp);
    IfFailGo( qbName.ReSize((nameSize + 1) * sizeof(WCHAR)) );
    szName = (WCHAR *) qbName.Ptr();
    actNameSize = ::WszMultiByteToWideChar(CP_UTF8, 0, szNameTmp, -1, szName, nameSize + 1);

    // The unicode translation function cannot fail!!
    _ASSERTE(actNameSize);

    // Define the memberRef
    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(tr, szName, (PCCOR_SIGNATURE) qbNewSig.Ptr(), cbNewSig, &memberRefE) ); 

ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefTokenOfMethodInfo Failed!"); 
        COMPlusThrowHR(hr);    
    }

lExit: ;
    HELPER_METHOD_FRAME_END();

    // assign output parameter
    return memberRefE;

}
FCIMPLEND


//******************************************************************************
//
// Return a MemberRef token given a RuntimeFieldInfo
//
//******************************************************************************
FCIMPL3(mdMemberRef, COMModule::GetMemberRefTokenOfFieldInfo, ReflectModuleBaseObject* refThisUNSAFE, INT32 tr, ReflectBaseObject* fieldUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    mdMemberRef     memberRefE  = 0; 

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*         pModule     = GET_MODULE(refThisUNSAFE);
    REFLECTBASEREF  field       = (REFLECTBASEREF) fieldUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(field);

    HRESULT         hr; 
    WCHAR           *szName; 
    ULONG           nameSize;    
    FieldDesc       *pField;
    RefClassWriter* pRCW;   
    ULONG           actNameSize = 0;    
    ULONG           cbComSig;   
    PCCOR_SIGNATURE pvComSig;
    LPCUTF8         szNameTmp;
    CQuickBytes     qbNewSig;
    ULONG           cbNewSig;   
    CQuickBytes     qbName;
    Assembly        *pRefedAssembly;
    Assembly        *pRefingAssembly;
    IMetaDataAssemblyEmit *pAssemblyEmit = NULL;

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);
    ReflectField* pRF = (ReflectField*) field->GetData();
    pField = pRF->pField; 
    _ASSERTE(pField);

    if (TypeFromToken(tr) == mdtTypeDef)
    {
        // If the passed in method is defined in the same module, just return the FieldDef token           
        memberRefE = pField->GetMemberDef();
        goto lExit;
    }

    // get the field name and sig
    szNameTmp = pField->GetMDImport()->GetNameOfFieldDef(pField->GetMemberDef());
    pvComSig = pField->GetMDImport()->GetSigOfFieldDef(pField->GetMemberDef(), &cbComSig);

    // translate the name to unicode string
    nameSize = (ULONG)strlen(szNameTmp);
    IfFailGo( qbName.ReSize((nameSize + 1) * sizeof(WCHAR)) );
    szName = (WCHAR *) qbName.Ptr();
    actNameSize = ::WszMultiByteToWideChar(CP_UTF8, 0, szNameTmp, -1, szName, nameSize + 1);
    
    // The unicode translation function cannot fail!!
    _ASSERTE(actNameSize);

    pRefedAssembly = pField->GetModule()->GetAssembly();
    pRefingAssembly = pModule->GetAssembly();
    IfFailGo( pRefingAssembly->GetSecurityModule()->GetEmitter()->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );

    // Translate the field signature this scope  
    IfFailGo( pField->GetMDImport()->TranslateSigWithScope(
        pRefedAssembly->GetManifestImport(), 
        NULL, 0,            // hash value
        pvComSig, 
        cbComSig, 
        pAssemblyEmit,      // Emit assembly scope.
        pRCW->GetEmitter(), 
        &qbNewSig, 
        &cbNewSig) );  

    _ASSERTE(!pField->GetMethodTableOfEnclosingClass()->HasSharedMethodTable());

    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(tr, szName, (PCCOR_SIGNATURE) qbNewSig.Ptr(), cbNewSig, &memberRefE) ); 

ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefTokenOfFieldInfo Failed on Field"); 
        COMPlusThrowHR(hr);    
    }

lExit: ;
    HELPER_METHOD_FRAME_END();

    return memberRefE;  
}
FCIMPLEND

//******************************************************************************
//
// Return a MemberRef token given a Signature
//
//******************************************************************************
FCIMPL5(INT32, COMModule::GetMemberRefTokenFromSignature, ReflectModuleBaseObject* refThisUNSAFE, 
                                                      INT32 tr,
                                                      StringObject* strMemberNameUNSAFE,
                                                      U1Array* signatureUNSAFE,
                                                      INT32 sigLength)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    mdMemberRef     memberRefE; 

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    STRINGREF               strMemberName   = (STRINGREF)   strMemberNameUNSAFE;
    U1ARRAYREF              signature       = (U1ARRAYREF)  signatureUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_2(strMemberName, signature);

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(tr, 
                                                  strMemberName->GetBuffer(), 
                                                  (PCCOR_SIGNATURE) signature->GetDataPtr(), 
                                                  sigLength, 
                                                  &memberRefE) ); 

ErrExit:
    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefTokenOfFieldInfo Failed on Field"); 
        COMPlusThrowHR(hr);    
    }
    HELPER_METHOD_FRAME_END();
    return memberRefE;  
}
FCIMPLEND

//******************************************************************************
//
// SetFieldRVAContent
// This function is used to set the FieldRVA with the content data
//
//******************************************************************************
FCIMPL4(void, COMModule::SetFieldRVAContent, ReflectModuleBaseObject* refThisUNSAFE, INT32 tkField, U1Array* contentUNSAFE, INT32 length)
{
    THROWSCOMPLUSEXCEPTION();

    RefClassWriter      *pRCW;   
    ICeeGen             *pGen;
    HRESULT             hr;
    DWORD               dwRVA;
    void                *pvBlob;

    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    U1ARRAYREF  content = (U1ARRAYREF) contentUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(content);

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    pGen = pRCW->GetCeeGen();

    // Create the .sdata section if not created
    if (((ReflectionModule*) pModule)->m_sdataSection == 0)
        IfFailGo( pGen->GetSectionCreate (".sdata", sdReadWrite, &((ReflectionModule*) pModule)->m_sdataSection) );

    // Get the size of current .sdata section. This will be the RVA for this field within the section
    IfFailGo( pGen->GetSectionDataLen(((ReflectionModule*) pModule)->m_sdataSection, &dwRVA) );
	dwRVA = (dwRVA + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1);         

    // allocate the space in .sdata section
    IfFailGo( pGen->GetSectionBlock(((ReflectionModule*) pModule)->m_sdataSection, length, sizeof(DWORD), (void**) &pvBlob) );

    // copy over the initialized data if specified
    if (content != NULL)
        memcpy(pvBlob, content->GetDataPtr(), length);

    // set FieldRVA into metadata. Note that this is not final RVA in the image if save to disk. We will do another round of fix up upon save.
    IfFailGo( pRCW->GetEmitter()->SetFieldRVA(tkField, dwRVA) );

ErrExit:
    if (FAILED(hr))
    {
        // failed in Setting ResolutionScope
        COMPlusThrowHR(hr);
    }
   
    HELPER_METHOD_FRAME_END();
   
}
FCIMPLEND


//******************************************************************************
//
// GetStringConstant
// If this is a dynamic module, this routine will define a new 
//  string constant or return the token of an existing constant.    
//
//******************************************************************************
FCIMPL2(mdString, COMModule::GetStringConstant, ReflectModuleBaseObject* refThisUNSAFE, StringObject* strValueUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    RefClassWriter* pRCW;   
    mdString strRef;   
    HRESULT hr;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    // Verify that the module is a dynamic module...    
    Module* pModule = GET_MODULE(refThisUNSAFE);

    STRINGREF strValue = (STRINGREF) strValueUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(strValue);

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    // If they didn't pass a String throw...    
    if (!strValue)    
        COMPlusThrow(kArgumentNullException,L"ArgumentNull_String");

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    hr = pRCW->GetEmitter()->DefineUserString(strValue->GetBuffer(), 
            strValue->GetStringLength(), &strRef);
    if (FAILED(hr)) {   
        _ASSERTE(!"Unknown failure in DefineUserString");    
        COMPlusThrowHR(hr);    
    }   

    HELPER_METHOD_FRAME_END();

    return strRef;  
}
FCIMPLEND


/*=============================SetModuleProps==============================
// SetModuleProps
==============================================================================*/
FCIMPL2(void, COMModule::SetModuleProps, ReflectModuleBaseObject* refThisUNSAFE, StringObject* strModuleNameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    RefClassWriter      *pRCW;
    HRESULT             hr;
    IMetaDataEmit       *pEmit;

    if (refThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Module*     pModule         = GET_MODULE(refThisUNSAFE);
    STRINGREF   strModuleName   = (STRINGREF)strModuleNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(strModuleName);

    if (!pModule->IsReflection())
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter();
    _ASSERTE(pRCW);

    pEmit = pRCW->GetEmitter(); 

    IfFailGo( pEmit->SetModuleProps(strModuleName->GetBuffer()) );

ErrExit:
    if (FAILED(hr))
    {
        // failed in Setting ResolutionScope
        COMPlusThrowHR(hr);    
    }

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


//***********************************************************
// Helper function to form array in signature. Call within unmanaged code only.
//***********************************************************
unsigned COMModule::GetSigForTypeHandle(TypeHandle typeHnd, PCOR_SIGNATURE sigBuff, unsigned buffLen, IMetaDataEmit* emit, IMDInternalImport *pInternalImport, int baseToken) 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    unsigned i = 0;
    CorElementType type = typeHnd.GetSigCorElementType();
    if (i < buffLen)
        sigBuff[i] = type;
    i++;

    _ASSERTE(type != ELEMENT_TYPE_OBJECT && type != ELEMENT_TYPE_STRING);

    if (CorTypeInfo::IsPrimitiveType(type))
        return(i);
    else if (type == ELEMENT_TYPE_VALUETYPE || type == ELEMENT_TYPE_CLASS) {
        if (i + 4 > buffLen)
            return(i + 4);
        _ASSERTE(baseToken);
        i += CorSigCompressToken(baseToken, &sigBuff[i]);
        return(i);
    }

        // Only parameterized types from here on out.  
    i += GetSigForTypeHandle(typeHnd.AsTypeDesc()->GetTypeParam(), &sigBuff[i], buffLen - i, emit, pInternalImport, baseToken);
    if (type == ELEMENT_TYPE_SZARRAY || type == ELEMENT_TYPE_PTR)
        return(i);

    _ASSERTE(type == ELEMENT_TYPE_ARRAY);
    if (i + 6 > buffLen)
        return(i + 6);

    i += CorSigCompressData(typeHnd.AsArray()->GetRank(), &sigBuff[i]);
    sigBuff[i++] = 0;       // bound count
    sigBuff[i++] = 0;       // lower bound count
    return(i);
}



//******************************************************************************
//
// Return a type spec token given a reflected type
//
//******************************************************************************
FCIMPL3(mdTypeSpec, COMModule::GetTypeSpecToken, ReflectModuleBaseObject* refThisUNSAFE, ReflectClassBaseObject* arrayClassUNSAFE, INT32 baseToken)
{
    THROWSCOMPLUSEXCEPTION();

    COR_SIGNATURE   aBuff[32];
    PCOR_SIGNATURE  buff = aBuff;
    ULONG           cSig;
    mdTypeSpec      ts;
    RefClassWriter  *pRCW; 
    HRESULT         hr = NOERROR;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    REFLECTCLASSBASEREF arrayClass = (REFLECTCLASSBASEREF)arrayClassUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(arrayClass);

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    TypeHandle      typeHnd = ((ReflectClass *) arrayClass->GetData())->GetTypeHandle();
    _ASSERTE(typeHnd.IsTypeDesc());

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    cSig = GetSigForTypeHandle(typeHnd, buff, 32, pRCW->GetEmitter(), pRCW->GetMDImport(), baseToken);
    if (cSig >= 32) {
        ULONG buffSize = cSig + 1;
        buff = (PCOR_SIGNATURE) _alloca(buffSize + 1);
        cSig = GetSigForTypeHandle(typeHnd, buff, buffSize, pRCW->GetEmitter(), pRCW->GetMDImport(), baseToken);
        _ASSERTE(cSig < buffSize);
    }
    hr = pRCW->GetEmitter()->GetTokenFromTypeSpec(buff, cSig, &ts);  
    _ASSERTE(SUCCEEDED(hr));

    HELPER_METHOD_FRAME_END();
    return ts;

}
FCIMPLEND


//******************************************************************************
//
// Return a type spec token given a byte array
//
//******************************************************************************
FCIMPL3(mdTypeSpec, COMModule::GetTypeSpecTokenWithBytes, ReflectModuleBaseObject* refThisUNSAFE, U1Array* signatureUNSAFE, INT32 sigLength)
{
    THROWSCOMPLUSEXCEPTION();

    mdTypeSpec      ts;
    RefClassWriter  *pRCW; 
    HRESULT         hr = NOERROR;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    U1ARRAYREF  signature   = (U1ARRAYREF) signatureUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(signature);

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    hr = pRCW->GetEmitter()->GetTokenFromTypeSpec((PCCOR_SIGNATURE)signature->GetDataPtr(), sigLength, &ts);  
    _ASSERTE(SUCCEEDED(hr));

    HELPER_METHOD_FRAME_END();
    return ts;

}
FCIMPLEND

HRESULT COMModule::ClassNameFilter(IMDInternalImport *pInternalImport, mdTypeDef* rgTypeDefs, 
    DWORD* pdwNumTypeDefs, LPUTF8 szPrefix, DWORD cPrefix, bool bCaseSensitive)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(rgTypeDefs && pdwNumTypeDefs && szPrefix);

    bool    bIsPrefix   = false;
    DWORD   dwCurIndex;
    DWORD   i;
    int     cbLen;
    CQuickBytes qbFullName;
    int     iRet;

    // Check to see if wzPrefix requires an exact match of method names or is just a prefix
    if(szPrefix[cPrefix-1] == '*')
    {
        bIsPrefix = true;
        cPrefix--;
    }

    // Now get the properties and then names of each of these classes
    for(i = 0, dwCurIndex = 0; i < *pdwNumTypeDefs; i++)
    {
        LPCUTF8 szcTypeDefName;
        LPCUTF8 szcTypeDefNamespace;
        LPUTF8  szFullName;

        pInternalImport->GetNameOfTypeDef(rgTypeDefs[i], &szcTypeDefName,
            &szcTypeDefNamespace);

        cbLen = ns::GetFullLength(szcTypeDefNamespace, szcTypeDefName);
        qbFullName.ReSize(cbLen);
        szFullName = (LPUTF8) qbFullName.Ptr();

        // Create the full name from the parts.
        iRet = ns::MakePath(szFullName, cbLen, szcTypeDefNamespace, szcTypeDefName);
        _ASSERTE(iRet);


        // If an exact match is required
        if(!bIsPrefix && strlen(szFullName) != cPrefix)
            continue;

        if(!bCaseSensitive && _strnicmp(szPrefix, szFullName, cPrefix))
            continue;

        // Check that the prefix matches
        if(bCaseSensitive && strncmp(szPrefix, szFullName, cPrefix))
            continue;

        // It passed, so copy it to the end of PASSED methods
        rgTypeDefs[dwCurIndex++] = rgTypeDefs[i];
    }

    // The current index is the number that passed
    *pdwNumTypeDefs = dwCurIndex;
    
    return ERROR_SUCCESS;
}

// GetClass
// Given a class name, this method will look for that class
//  with in the module. 
FCIMPL5(Object*, COMModule::GetClass, ReflectModuleBaseObject* refThisUNSAFE, StringObject* refClassNameUNSAFE, BYTE bIgnoreCase, BYTE bThrowOnError, StackCrawlMark* stackMark)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF Throwable = NULL;
    OBJECTREF ret = NULL;
    TypeHandle typeHnd;
    UINT resId = IDS_CLASSLOAD_GENERIC;
    
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*     pModule         = GET_MODULE(refThisUNSAFE);
    STRINGREF   refClassName    = (STRINGREF)refClassNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, Throwable, refClassName);
    
    if (refClassName == NULL)
        COMPlusThrow(kArgumentNullException,L"ArgumentNull_String");

    CQuickBytes bytes;
    LPSTR szClassName;
    DWORD cClassName;

    // Get the UTF8 version of refClassName
    szClassName = GetClassStringVars(refClassName, &bytes, 
                                     &cClassName, true);

    if(!cClassName)
        COMPlusThrow(kArgumentException, L"Format_StringZeroLength");
    if (cClassName >= MAX_CLASSNAME_LENGTH)
        COMPlusThrow(kArgumentException, L"Argument_TypeNameTooLong");

    if (szClassName[0] == '\0')
        COMPlusThrow(kArgumentException, L"Argument_InvalidName");


    if (NULL == NormalizeArrayTypeName(szClassName, cClassName)) 
        resId = IDS_CLASSLOAD_BAD_NAME; 
    else {
        NameHandle typeName(szClassName);
        if (bIgnoreCase)
            typeName.SetCaseInsensitive();
    
        typeHnd = pModule->GetAssembly()->FindNestedTypeHandle(&typeName, &Throwable);
        if (typeHnd.IsNull() ||
            (Throwable != NULL) ||
            (typeHnd.GetModule() != pModule))
            goto Done;
    
        // Check we have access to this type. Rules are:
        //  o  Public types are always visible.
        //  o  Callers within the same assembly (or interop) can access all types.
        //  o  Access to all other types requires ReflectionPermission.TypeInfo.
        EEClass *pClass = typeHnd.GetClassOrTypeParam();
        _ASSERTE(pClass);
        if (!IsTdPublic(pClass->GetProtection())) {
            EEClass *pCallersClass = SystemDomain::GetCallersClass(stackMark);
            Assembly *pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
            if (pCallersAssembly && // full trust for interop
                !ClassLoader::CanAccess(pCallersClass,
                                        pCallersAssembly,
                                        pClass,
                                        pClass->GetAssembly(),
                                        pClass->GetAttrClass())) 
                // This is not legal if the user doesn't have reflection permission
                COMMember::g_pInvokeUtil->CheckSecurity();
        }
    
        // If they asked for the transparent proxy lets ignore it...
        ret = typeHnd.CreateClassObj();
    }

Done:
    if (ret == NULL) {
        if (bThrowOnError) {
            if (Throwable == NULL)
                pModule->GetAssembly()->PostTypeLoadException(szClassName, resId, &Throwable);

            COMPlusThrow(Throwable);
        }
    }

    HELPER_METHOD_FRAME_END();
    return (ret!=NULL) ? OBJECTREFToObject(ret) : NULL;
}
FCIMPLEND


// GetName
// This routine will return the name of the module as a String
FCIMPL1(Object*, COMModule::GetName, ReflectModuleBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF       modName = NULL;
    LPCSTR szName;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, modName);

    if (pModule->IsResource())
    {
        pModule->GetAssembly()->GetManifestImport()->GetFileProps(pModule->GetModuleRef(),
                                                                  &szName,
                                                                  NULL,
                                                                  NULL,
                                                                  NULL);
    }
    else
    {
    pModule->GetMDImport()->GetScopeProps(&szName,0);
    }

    modName = COMString::NewString(szName);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(modName);
}
FCIMPLEND


/*============================GetFullyQualifiedName=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL1(Object*, COMModule::GetFullyQualifiedName,  ReflectModuleBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    STRINGREF name=NULL;
    HRESULT hr = S_OK;

    WCHAR wszBuffer[64];

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, name);
    
    if (pModule->IsPEFile()) {
        LPCWSTR fileName = pModule->GetFileName();
        if (*fileName != 0) {
            name = COMString::NewString(fileName);
        } else {
            hr = LoadStringRC(IDS_EE_NAME_UNKNOWN, wszBuffer, sizeof( wszBuffer ) / sizeof( WCHAR ), true );
            if (SUCCEEDED(hr))
                name = COMString::NewString(wszBuffer);
            else
                COMPlusThrowHR(hr);
        }
    } else if (pModule->IsInMemory()) {
        hr = LoadStringRC(IDS_EE_NAME_INMEMORYMODULE, wszBuffer, sizeof( wszBuffer ) / sizeof( WCHAR ), true );
        if (SUCCEEDED(hr))
            name = COMString::NewString(wszBuffer);
        else
            COMPlusThrowHR(hr);
    } else {
        hr = LoadStringRC(IDS_EE_NAME_INTEROP, wszBuffer, sizeof( wszBuffer ) / sizeof( WCHAR ), true );
        if (SUCCEEDED(hr))
            name = COMString::NewString(wszBuffer);
        else
            COMPlusThrowHR(hr);
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(name);
}
FCIMPLEND

/*===================================GetHINST===================================
**Action:  Returns the hinst for this module.
**Returns:
**Arguments: refThis
**Exceptions: None.
==============================================================================*/
FCIMPL1(HINSTANCE, COMModule::GetHINST, ReflectModuleBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    HMODULE hMod;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    // This returns the base address - this will work for either HMODULE or HCORMODULES
    // Other modules should have zero base
    hMod = (HMODULE) pModule->GetILBase();

    //If we don't have an hMod, set it to -1 so that they know that there's none
    //available
    if (!hMod) {
        (*((INT32 *)&hMod))=-1;
    }
    
    HELPER_METHOD_FRAME_END();
    
    return (HINSTANCE)hMod;
}
FCIMPLEND

static Object* GetClassesInner(Module* pModule, StackCrawlMark* stackMark);

// Get class will return an array contain all of the classes
//  that are defined within this Module.    
FCIMPL2(Object*, COMModule::GetClasses, ReflectModuleBaseObject* refThisUNSAFE, StackCrawlMark* stackMark)
{
    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*     pModule     = GET_MODULE(refThisUNSAFE);
    OBJECTREF   refRetVal   = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    refRetVal = (OBJECTREF) GetClassesInner(pModule, stackMark);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

Object* GetClassesInner(Module* pModule, StackCrawlMark* stackMark)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr;
    DWORD           dwNumTypeDefs = 0;
    DWORD           i;
    mdTypeDef*      rgTypeDefs;
    IMDInternalImport *pInternalImport;
    PTRARRAYREF     refArrClasses = NULL;
    PTRARRAYREF     xcept;
    DWORD           cXcept;
    HENUMInternal   hEnum;
    bool            bSystemAssembly;    // Don't expose transparent proxy
    bool            bCheckedAccess = false;
    bool            bAllowedAccess = false;
	int             AllocSize = 0;


    if (pModule->IsResource()) {
        refArrClasses = (PTRARRAYREF) AllocateObjectArray(0, g_pRefUtil->GetTrueType(RC_Class));
        goto lExit;
    }

    pInternalImport = pModule->GetMDImport();

    // Get the count of typedefs
    hr = pInternalImport->EnumTypeDefInit(&hEnum);

    if(FAILED(hr)) {
        _ASSERTE(!"GetCountTypeDefs failed.");
        COMPlusThrowHR(hr);    
    }
    dwNumTypeDefs = pInternalImport->EnumTypeDefGetCount(&hEnum);

    // Allocate an array for all the typedefs
    rgTypeDefs = (mdTypeDef*) _alloca(sizeof(mdTypeDef) * dwNumTypeDefs);

    // Get the typedefs
    for (i=0; pInternalImport->EnumTypeDefNext(&hEnum, &rgTypeDefs[i]); i++) {

        // Filter out types we can't access.
        if (bCheckedAccess && bAllowedAccess)
            continue;

        DWORD dwFlags;
        pInternalImport->GetTypeDefProps(rgTypeDefs[i], &dwFlags, NULL);

        mdTypeDef mdEncloser = rgTypeDefs[i];
        while (SUCCEEDED(pInternalImport->GetNestedClassProps(mdEncloser, &mdEncloser)) &&
               IsTdNestedPublic(dwFlags))
            pInternalImport->GetTypeDefProps(mdEncloser,
                                             &dwFlags,
                                             NULL);

        // Public types always accessible.
        if (IsTdPublic(dwFlags))
            continue;

        // Need to perform a more heavyweight check. Do this once, since the
        // result is valid for all non-public types within a module.
        if (!bCheckedAccess) {

            Assembly *pCaller = SystemDomain::GetCallersAssembly(stackMark);
            if (pCaller == NULL || pCaller == pModule->GetAssembly())
                // Assemblies can access all their own types (and interop
                // callers always get access).
                bAllowedAccess = true;
            else {
                // For the cross assembly case caller needs
                // ReflectionPermission.TypeInfo (the CheckSecurity call will
                // throw if this isn't granted).
                COMPLUS_TRY {
                    COMMember::g_pInvokeUtil->CheckSecurity();
                    bAllowedAccess = true;
                } COMPLUS_CATCH {
                } COMPLUS_END_CATCH
            }
            bCheckedAccess = true;
        }

        if (bAllowedAccess)
            continue;

        // Can't access this type, remove it from the list.
        i--;
    }

    pInternalImport->EnumTypeDefClose(&hEnum);

    // Account for types we skipped.
    dwNumTypeDefs = i;

    // Allocate the COM+ array
    bSystemAssembly = (pModule->GetAssembly() == SystemDomain::SystemAssembly());
    AllocSize = (!bSystemAssembly || (bCheckedAccess && !bAllowedAccess)) ? dwNumTypeDefs : dwNumTypeDefs - 1;
    refArrClasses = (PTRARRAYREF) AllocateObjectArray(
        AllocSize, g_pRefUtil->GetTrueType(RC_Class));
    GCPROTECT_BEGIN(refArrClasses);

    // Allocate an array to store the references in
    xcept = (PTRARRAYREF) AllocateObjectArray(dwNumTypeDefs,g_pExceptionClass);
    GCPROTECT_BEGIN(xcept);

    cXcept = 0;
    

    OBJECTREF throwable = 0;
    GCPROTECT_BEGIN(throwable);
    // Now create each COM+ Method object and insert it into the array.
    int curPos = 0;
    for(i = 0; i < dwNumTypeDefs; i++)
    {
        // Get the VM class for the current class token
        _ASSERTE(pModule->GetClassLoader());
        NameHandle name(pModule, rgTypeDefs[i]);
        EEClass* pVMCCurClass = pModule->GetClassLoader()->LoadTypeHandle(&name, &throwable).GetClass();
        if (bSystemAssembly) {
            if (pVMCCurClass->GetMethodTable()->IsTransparentProxyType())
                continue;
        }
        if (throwable != 0) {
            refArrClasses->ClearAt(i);
            xcept->SetAt(cXcept++, throwable);
            throwable = 0;
        }
        else {
            _ASSERTE("LoadClass failed." && pVMCCurClass);

            // Get the COM+ Class object
            OBJECTREF refCurClass = pVMCCurClass->GetExposedClassObject();
            _ASSERTE("GetExposedClassObject failed." && refCurClass != NULL);

            refArrClasses->SetAt(curPos++, refCurClass);
        }
    }
    GCPROTECT_END();    //throwable

    // check if there were exceptions thrown
    if (cXcept > 0) {
        PTRARRAYREF xceptRet = (PTRARRAYREF) AllocateObjectArray(cXcept,g_pExceptionClass);
        GCPROTECT_BEGIN(xceptRet);
        for (i=0;i<cXcept;i++) {
            xceptRet->SetAt(i, xcept->GetAt(i));
        }
        OBJECTREF except = COMMember::g_pInvokeUtil->CreateClassLoadExcept((OBJECTREF*) &refArrClasses,(OBJECTREF*) &xceptRet);
        COMPlusThrow(except);
        GCPROTECT_END();
    }

    // Assign the return value to the COM+ array
    GCPROTECT_END();
    GCPROTECT_END();

lExit:
    return OBJECTREFToObject(refArrClasses);
}


FCIMPL1(Object*, COMModule::GetAssembly,  ReflectModuleBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module*         pModule     = GET_MODULE(refThisUNSAFE);
    ASSEMBLYREF     result      = NULL;
        

    Assembly *pAssembly = pModule->GetAssembly();
    _ASSERTE(pAssembly);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, result);
    result = (ASSEMBLYREF) pAssembly->GetExposedObject();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(result);
}
FCIMPLEND

FCIMPL1(INT32, COMModule::IsResource, ReflectModuleBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);
    return pModule->IsResource();
}
FCIMPLEND

FCIMPL1(Object*, COMModule::GetMethods, ReflectModuleBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    PTRARRAYREF     refArrMethods = NULL;

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(refThisUNSAFE);

    REFLECTMODULEBASEREF    refThis = (REFLECTMODULEBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refArrMethods, refThis);

    ReflectMethodList* pML = (ReflectMethodList*) refThis->GetGlobals();
    if (pML == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobals(pModule);
        refThis->SetGlobals(pGlobals);
        pML = (ReflectMethodList*) refThis->GetGlobals();
        _ASSERTE(pML);
    }


    // Lets make the reflection class into
    //  the declaring class...
    ReflectClass* pRC = 0;
    if (pML->dwMethods > 0) {
        EEClass* pEEC = pML->methods[0].pMethod->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }       
    }

    // Create an array of Methods...
    refArrMethods = g_pRefUtil->CreateClassArray(RC_Method,pRC,pML,BINDER_AllLookup, true);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refArrMethods);
}
FCIMPLEND

FCIMPL6(Object*, COMModule::InternalGetMethod, ReflectModuleBaseObject* refThisUNSAFE,
                                               StringObject* nameUNSAFE,
                                               INT32 invokeAttr,
                                               INT32 callConv,
                                               PTRArray* argTypesUNSAFE,
                                               INT32 argCnt)
{
    THROWSCOMPLUSEXCEPTION();

    PTRARRAYREF  refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);

    struct _gc
    {
        REFLECTMODULEBASEREF    refThis;
        STRINGREF               name;
        PTRARRAYREF             argTypes;
    } gc;

    gc.refThis  = (REFLECTMODULEBASEREF)    refThisUNSAFE;
    gc.name     = (STRINGREF)               nameUNSAFE;
    gc.argTypes = (PTRARRAYREF)             argTypesUNSAFE;

    GCPROTECT_BEGIN(gc);

    HELPER_METHOD_POLL();

    bool    addPriv;
    bool    ignoreCase;
    bool    checkCall;

    //Check Security
    if (invokeAttr & BINDER_NonPublic)
        addPriv = true;
    else
        addPriv = false;


    ignoreCase = (invokeAttr & BINDER_IgnoreCase) ? true : false;

    // Check the calling convention.
    checkCall = (callConv == Any_CC) ? false : true;

    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    // Convert the name into UTF8
    szName = GetClassStringVars(gc.name, &bytes, &cName);

    Module* pModule = (Module*) gc.refThis->GetData();
    _ASSERTE(pModule);

    ReflectMethodList* pML = (ReflectMethodList*) gc.refThis->GetGlobals();
    if (pML == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobals(pModule);
        gc.refThis->SetGlobals(pGlobals);
        pML = (ReflectMethodList*) gc.refThis->GetGlobals();
        _ASSERTE(pML);
    }

    ReflectClass* pRC = 0;
    if (pML->dwMethods > 0) {
        EEClass* pEEC = pML->methods[0].pMethod->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }       
    }

    // Find methods.....
    LPVOID pv;
    pv = COMMember::g_pInvokeUtil->FindMatchingMethods(invokeAttr,
                                                         szName,
                                                         cName,
                                                         (gc.argTypes != NULL) ? &(gc.argTypes) : NULL,
                                                         argCnt,
                                                         checkCall,
                                                         callConv,
                                                         pRC,
                                                         pML,
                                                         g_pRefUtil->GetTrueType(RC_Method),
                                                         true);

    refRetVal = (PTRARRAYREF)ObjectToOBJECTREF((Object*) pv);

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

// GetFields
// Return an array of fields.
FCIMPL1(Object*, COMModule::GetFields, ReflectModuleBaseObject* vRefThis)
{
    THROWSCOMPLUSEXCEPTION();
    Object*          rv;

    if (vRefThis == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(vRefThis);

    HELPER_METHOD_FRAME_BEGIN_RET_1(vRefThis);    // Set up a frame

    PTRARRAYREF     refArrFields;

    ReflectFieldList* pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
    if (pFL == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobalFields(pModule);
        ((REFLECTMODULEBASEREF)vRefThis)->SetGlobalFields(pGlobals);
        pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
        _ASSERTE(pFL);
    }

    // Lets make the reflection class into
    //  the declaring class...
    ReflectClass* pRC = 0;
    if (pFL->dwFields > 0) {
        EEClass* pEEC = pFL->fields[0].pField->GetMethodTableOfEnclosingClass()->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }       
    }

    // Create an array of Methods...
    refArrFields = g_pRefUtil->CreateClassArray(RC_Field,pRC,pFL,BINDER_AllLookup, true);
    rv = OBJECTREFToObject(refArrFields);
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND

// GetFields
// Return the specified fields
FCIMPL3(Object*, COMModule::GetField, ReflectModuleBaseObject* vRefThis, StringObject* name, INT32 bindingAttr)
{
    THROWSCOMPLUSEXCEPTION();
    Object*          rv = 0;

    // Get the Module
    if (vRefThis == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Module* pModule = GET_MODULE(vRefThis);

    HELPER_METHOD_FRAME_BEGIN_RET_2(vRefThis, name);    // Set up a frame
    RefSecContext   sCtx;

    DWORD       i;
    ReflectField* pTarget = 0;
    ReflectClass* pRC = 0;
	MethodTable* pParentMT = NULL;

    // Convert the name into UTF8
    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    // Convert the name into UTF8
    szName = GetClassStringVars((STRINGREF)name, &bytes, &cName);

    // Find the list of global fields.
    ReflectFieldList* pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
    if (pFL == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobalFields(pModule);
        ((REFLECTMODULEBASEREF)vRefThis)->SetGlobalFields(pGlobals);
        pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
        _ASSERTE(pFL);
    }
    if (pFL->dwFields == 0)
        goto LExit;

    // Lets make the reflection class into
    //  the declaring class...
    if (pFL->dwFields > 0) {
        EEClass* pEEC = pFL->fields[0].pField->GetMethodTableOfEnclosingClass()->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }
    }

    pParentMT = pRC->GetClass()->GetMethodTable();

    // Walk each field...
    for (i=0;i<pFL->dwFields;i++) {
        // Get the FieldDesc
        if (COMClass::MatchField(pFL->fields[i].pField,cName,szName,pRC,bindingAttr) &&
            InvokeUtil::CheckAccess(&sCtx, pFL->fields[i].pField->GetFieldProtection(), pParentMT, 0)) {
            if (pTarget)
                COMPlusThrow(kAmbiguousMatchException);
            pTarget = &pFL->fields[i];
        }
    }

    // If we didn't find any methods then return
    if (pTarget == 0)
        goto LExit;
    rv = OBJECTREFToObject(pTarget->GetFieldInfo(pRC));
LExit:;
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND


// This code should be moved out of Variant and into Type.
FCIMPL1(INT32, COMModule::GetSigTypeFromClassWrapper, ReflectClassBaseObject* refType)
{
    VALIDATEOBJECTREF(refType);
    _ASSERTE(refType->GetData());

    ReflectClass* pRC = (ReflectClass*) refType->GetData();

    // Find out if this type is a primitive or a class object
    return pRC->GetSigElementType();
    
}
FCIMPLEND
