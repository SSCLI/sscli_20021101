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
////////////////////////////////////////////////////////////////////////////////
// COMDynamic.h
//  This module defines the native methods that are used for Dynamic IL generation  
//
// Date: November 1998
////////////////////////////////////////////////////////////////////////////////
#ifndef _COMDYNAMIC_H_
#define _COMDYNAMIC_H_

#include "comclass.h"
#include "iceefilegen.h"
#include "dbginterface.h"
#include "comvariant.h"

HRESULT STDMETHODCALLTYPE CreateICorModule(REFIID riid, void **pCorModule);

inline CorModule* allocateReflectionModule()
{
    CorModule *pReflectionModule;   
    HRESULT hr = CreateICorModule(IID_ICorReflectionModule, (void **)&pReflectionModule);   
    if (FAILED(hr)) 
        return NULL;    
    return pReflectionModule; 
}

typedef enum PEFileKinds {
    Dll = 0x1,
    ConsoleApplication = 0x2,
    WindowApplication = 0x3,
} PEFileKinds;

// COMDynamicWrite
// This class defines all the methods that implement the dynamic IL creation process
//  inside reflection.  
class COMDynamicWrite
{
private:

    static void UpdateMethodRVAs(IMetaDataEmit*, IMetaDataImport*, ICeeFileGen *, HCEEFILE, mdTypeDef td, HCEESECTION sdataSection);

public:

    // create a dynamic module with a given name. Note that we don't set the name of the dynamic module here.
    // The name of the dynamic module is set in the managed world
    //
    static InMemoryModule* CreateDynamicModule(Assembly* pContainingAssembly,
                                       STRINGREF &filename)
    {
        HRESULT     hr = NOERROR;
        THROWSCOMPLUSEXCEPTION();   

        _ASSERTE(pContainingAssembly);
        
        CorModule *pWrite;

        // allocate the dynamic module
        pWrite = allocateReflectionModule();  
        if (!pWrite) COMPlusThrowOM();
        
        // intiailize the dynamic module
        hr = pWrite->Initialize(CORMODULE_NEW, IID_ICeeGen, IID_IMetaDataEmit);
        if (FAILED(hr)) 
        {
            pWrite->Release();  
            COMPlusThrowOM(); 
        }            

        // Set the name for the debugger.
        if ((filename != NULL) &&
            (filename->GetStringLength() > 0))
        {
            ReflectionModule *rm = pWrite->GetModule()->GetReflectionModule();
            rm->SetFileName(filename->GetBuffer());
        }

        // link the dynamic module into the containing assembly
        hr = pContainingAssembly->AddModule(pWrite->GetModule(), mdFileNil, false);
        if (FAILED(hr))
        {
            pWrite->Release();
            COMPlusThrowOM();
        }
        InMemoryModule *retMod = pWrite->GetModule();
        pWrite->Release();
        return retMod;
    }   


    static CorModule* GetCorModule(Module* pModule, BOOL fNewAssembly = false)
    {
        THROWSCOMPLUSEXCEPTION();   
        _ASSERTE(pModule);
        Assembly* pCaller = pModule->GetAssembly();
        _ASSERTE(pCaller);

        Assembly* pAssem;
        if (fNewAssembly) {
            HRESULT hr = pCaller->GetDomain()->CreateAssembly(&pAssem);
            if (FAILED(hr))
                COMPlusThrowOM();
            pCaller->GetDomain()->AddAssembly(pAssem);
        } else
            pAssem = pCaller;

        CorModule *pWrite = pAssem->GetDynamicModule();

        if(pWrite == NULL) {
            pWrite = allocateReflectionModule();  
            if (!pWrite) COMPlusThrowOM();
            
            HRESULT hr = pWrite->Initialize(CORMODULE_NEW, IID_ICeeGen, IID_IMetaDataEmit);
            if (FAILED(hr)) {
                pWrite->Release();  
                COMPlusThrowOM();
            }

            pAssem->SetDynamicModule(pWrite, fNewAssembly? TRUE : FALSE);
        }
        return pWrite;
    }   
    
    // the module that it pass in is already the reflection module
    static ReflectionModule* GetReflectionModule(Module* pModule) 
    {    
        THROWSCOMPLUSEXCEPTION();   
        // _ASSERTE(pModule->ModuleType() == IID_ICorReflectionModule);
        return reinterpret_cast<ReflectionModule *>(pModule); 
    }   

    // CWCreateClass    
    // ClassWriter.InternalDefineClass -- This function will create the class's metadata definition  
    //struct _CWCreateClassArgs { 
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
    //    DECLARE_ECALL_I4_ARG(INT32, tkEnclosingType); 
    //    DECLARE_ECALL_OBJECTREF_ARG(GUID, guid);
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    //    DECLARE_ECALL_I4_ARG(UINT32, attr); 
    //    DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, interfaces);
    //    DECLARE_ECALL_I4_ARG(UINT32, parent); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strFullName);   
    //    DECLARE_ECALL_OBJECTREF_ARG(UINT32*, retRef);
    //};  
    static FCDECL8(UINT32, CWCreateClass, Object* refThisUNSAFE, StringObject* strFullNameUNSAFE, UINT32 parent, I4Array* interfacesUNSAFE, UINT32 attr, ReflectModuleBaseObject* moduleUNSAFE, GUID guid, INT32 tkEnclosingType);

    // CWSetParentType    
    // ClassWriter.InternalSetParentType -- This function will reset the parent class in metadata
    //struct _CWSetParentTypeArgs { 
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
    //    DECLARE_ECALL_I4_ARG(UINT32, tkParent); 
    //    DECLARE_ECALL_I4_ARG(UINT32, tdType); 
    //};  
    static FCDECL3(void, CWSetParentType, UINT32 tdType, UINT32 tkParent, ReflectModuleBaseObject* moduleUNSAFE);

    // CWAddInterfaceImpl    
    // ClassWriter.InternalAddInterfaceImpl -- This function will add another interface impl
    //struct _CWAddInterfaceImplArgs { 
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
    //    DECLARE_ECALL_I4_ARG(UINT32, tkInterface); 
    //    DECLARE_ECALL_I4_ARG(UINT32, tdType); 
    //};  
    static FCDECL3(void, CWAddInterfaceImpl, UINT32 tdType, UINT32 tkInterface, ReflectModuleBaseObject* moduleUNSAFE);

    // CWCreateMethod   
    // ClassWriter.InternalDefineMethod -- This function will create a method within the class  
    //struct _CWCreateMethodArgs {    
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    //    DECLARE_ECALL_I4_ARG(UINT32, attributes);
    //    DECLARE_ECALL_I4_ARG(UINT32, sigLength);
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);   
    //    DECLARE_ECALL_I4_ARG(UINT32, handle);             // struct TypeToken
    //    DECLARE_ECALL_OBJECTREF_ARG(UINT32*, retRef);
    //};  
    static FCDECL6(UINT32, CWCreateMethod, UINT32 handle, StringObject* nameUNSAFE, U1Array* signatureUNSAFE, UINT32 sigLength, UINT32 attributes, ReflectModuleBaseObject* moduleUNSAFE);

    // CWSetMethodIL    
    // ClassWriter.InternalSetMethodIL -- This function will create a method within the class   
    //struct _CWSetMethodILArgs { 
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    //    DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, rvaFixups);
    //    DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, tokenFixups);
    //    DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, exceptions);
    //    DECLARE_ECALL_I4_ARG(UINT32, numExceptions); 
    //    DECLARE_ECALL_I4_ARG(UINT32, maxStackSize);
    //    DECLARE_ECALL_I4_ARG(UINT32, sigLength);
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, localSig); 
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, body);     
    //    DECLARE_ECALL_I4_ARG(UINT32, isInitLocal);         
    //    DECLARE_ECALL_I4_ARG(UINT32, handle);              // struct MethodToken 
    //};  

    static FCDECL11(void, CWSetMethodIL, UINT32 handle, BYTE isInitLocal, U1Array* bodyUNSAFE, U1Array* localSigUNSAFE, UINT32 sigLength, UINT32 maxStackSize, UINT32 numExceptions, PTRArray* exceptionsUNSAFE, I4Array* tokenFixupsUNSAFE, I4Array* rvaFixupsUNSAFE, ReflectModuleBaseObject* moduleUNSAFE);

    // CWTermCreateClass    
    // ClassWriter.TermCreateClass --   
    //struct _CWTermCreateClassArgs { 
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    //    DECLARE_ECALL_I4_ARG(UINT32, handle);                 // struct TypeToken
    //};  
    static FCDECL3(Object*, CWTermCreateClass, Object* refThisUNSAFE, UINT32 handle, ReflectModuleBaseObject* moduleUNSAFE);

    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    //    DECLARE_ECALL_I4_ARG(UINT32, attr); 
    //    DECLARE_ECALL_I4_ARG(UINT32, sigLength);
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    //    DECLARE_ECALL_I4_ARG(UINT32, handle);                 // struct TypeToken
    //} _cwCreateFieldArgs;
    static FCDECL6(mdFieldDef, CWCreateField, UINT32 handle, StringObject* nameUNSAFE, U1Array* signatureUNSAFE, UINT32 sigLength, UINT32 attr, ReflectModuleBaseObject* moduleUNSAFE);

    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
    //} _PreSavePEFileArgs;
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
    //    DECLARE_ECALL_I4_ARG(UINT32, isManifestFile);
    //    DECLARE_ECALL_I4_ARG(UINT32, fileKind);
    //    DECLARE_ECALL_I4_ARG(UINT32, entryPoint);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, peName);
    //} _SavePEFileArgs;
    static FCDECL1(void, PreSavePEFile, Object* refThisUNSAFE);
    static FCDECL5(void, SavePEFile, Object* refThisUNSAFE, StringObject* peNameUNSAFE, UINT32 entryPoint, UINT32 fileKind, UINT32 isManifestFile);


    // not an ecall!
    static HRESULT COMDynamicWrite::EmitDebugInfoBegin(
        Module *pModule,
        ICeeFileGen *pCeeFileGen,
        HCEEFILE ceeFile,
        HCEESECTION pILSection,
        WCHAR *filename,
        ISymUnmanagedWriter *pWriter);

    // not an ecall!
    static HRESULT COMDynamicWrite::EmitDebugInfoEnd(
        Module *pModule,
        ICeeFileGen *pCeeFileGen,
        HCEEFILE ceeFile,
        HCEESECTION pILSection,
        WCHAR *filename,
        ISymUnmanagedWriter *pWriter);

    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
    //    DECLARE_ECALL_I4_ARG(UINT32, iCount);
    //} _setResourceCountArgs;
    static FCDECL2(void, SetResourceCounts, Object* refThisUNSAFE, UINT32 iCount);

    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
    //    DECLARE_ECALL_I4_ARG(UINT32, iAttribute);
    //    DECLARE_ECALL_I4_ARG(UINT32, tkFile);
    //    DECLARE_ECALL_I4_ARG(UINT32, iByteCount);
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, byteRes); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strName);
    //} _addResourceArgs;
    static FCDECL6(void, AddResource, Object* refThisUNSAFE, StringObject* strNameUNSAFE, U1Array* byteResUNSAFE, UINT32 iByteCount, UINT32 tkFile, UINT32 iAttribute);

    //typedef struct {
    //    DECLARE_ECALL_I4_ARG(UINT32, linkFlags);
    //    DECLARE_ECALL_I4_ARG(UINT32, linkType);
    //    DECLARE_ECALL_I4_ARG(UINT32, token);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, functionName);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, dllName);
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    //} _internalSetPInvokeDataArgs;
    static FCDECL6(void, InternalSetPInvokeData, ReflectModuleBaseObject* moduleUNSAFE, StringObject* dllNameUNSAFE, StringObject* functionNameUNSAFE, UINT32 token, UINT32 linkType, UINT32 linkFlags);

    // DefineProperty's argument
    //struct _CWDefinePropertyArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, tkNotifyChanged);                // NotifyChanged
    //    DECLARE_ECALL_I4_ARG(UINT32, tkNotifyChanging);               // NotifyChanging
    //    DECLARE_ECALL_I4_ARG(UINT32, sigLength);                      // property signature length
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);           // property siganture
    //    DECLARE_ECALL_I4_ARG(UINT32, attr);                           // property flags
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);                 // property name
    //    DECLARE_ECALL_I4_ARG(UINT32, handle);                         // type that will contain this property
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);    // dynamic module
    //    DECLARE_ECALL_OBJECTREF_ARG(UINT32*, retRef);                 // return PropertyToken here
    //};  
    static FCDECL8(UINT32, CWDefineProperty, ReflectModuleBaseObject* moduleUNSAFE, UINT32 handle, StringObject* nameUNSAFE, UINT32 attr, U1Array* signatureUNSAFE, UINT32 sigLength, UINT32 tkNotifyChanging, UINT32 tkNotifyChanged);


    // DefineEvent's argument
    //struct _CWDefineEventArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, eventtype);
    //    DECLARE_ECALL_I4_ARG(UINT32, attr);                         // event flags
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);               // event name
    //    DECLARE_ECALL_I4_ARG(UINT32, handle);                       // type that will contain this event
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //    DECLARE_ECALL_OBJECTREF_ARG(UINT32*, retRef);               // return PropertyToken here
    //};  
    static FCDECL5(UINT32, CWDefineEvent, ReflectModuleBaseObject* moduleUNSAFE, UINT32 handle, StringObject* nameUNSAFE, UINT32 attr, UINT32 eventtype);

    // functions to set Setter, Getter, Reset, TestDefault, and other methods
    //struct _CWDefineMethodSemanticsArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, method);                       // Method to associate with parent tk
    //    DECLARE_ECALL_I4_ARG(UINT32, attr);                         // MethodSemantics
    //    DECLARE_ECALL_I4_ARG(UINT32, association);                  // Parent tokens. It can be property or event
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL4(void, CWDefineMethodSemantics, ReflectModuleBaseObject* moduleUNSAFE, UINT32 association, UINT32 attr, UINT32 method);

    // functions to set method's implementation flag
    //struct _CWSetMethodImplArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, attr);                         // MethodImpl flags
    //    DECLARE_ECALL_I4_ARG(UINT32, tkMethod);                     // method token
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL3(void, CWSetMethodImpl, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkMethod, UINT32 attr);

    // functions to create MethodImpl record
    //struct _CWDefineMethodImplArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, tkDecl);                       // MethodImpl flags
    //    DECLARE_ECALL_I4_ARG(UINT32, tkBody);                       // MethodImpl flags
    //    DECLARE_ECALL_I4_ARG(UINT32, tkType);                       // method token
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL4(void, CWDefineMethodImpl, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkType, UINT32 tkBody, UINT32 tkDecl);

    // GetTokenFromSig's argument
    //struct _CWGetTokenFromSigArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, sigLength);                    // property signature length
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);         // property siganture
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL3(int, CWGetTokenFromSig, ReflectModuleBaseObject* moduleUNSAFE, U1Array* signatureUNSAFE, UINT32 sigLength);

    // Set Field offset
    //struct _CWSetFieldLayoutOffsetArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, iOffset);                      // MethodSemantics
    //    DECLARE_ECALL_I4_ARG(UINT32, tkField);                      // Parent tokens. It can be property or event
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL3(void, CWSetFieldLayoutOffset, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkField, UINT32 iOffset);

    // Set classlayout info
    //struct _CWSetClassLayoutArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, iTotalSize);                   // Size of type
    //    DECLARE_ECALL_I4_ARG(UINT32, iPackSize);                    // packing size
    //    DECLARE_ECALL_I4_ARG(UINT32, handle);                       // type that will contain this layout info
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL4(void, CWSetClassLayout, ReflectModuleBaseObject* moduleUNSAFE, UINT32 handle, UINT32 iPackSize, UINT32 iTotalSize);

    // Set a custom attribute
    //struct _CWInternalCreateCustomAttributeArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, toDisk);                       // emit CA to on disk metadata
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, blob);              // customattribute blob
    //    DECLARE_ECALL_I4_ARG(UINT32, conTok);                       // The constructor token
    //    DECLARE_ECALL_I4_ARG(UINT32, token);                        // Token to apply the custom attribute to
    //};  
    static FCDECL5(void, CWInternalCreateCustomAttribute, UINT32 token, UINT32 conTok, U1Array* blobUNSAFE, ReflectModuleBaseObject* moduleUNSAFE, BYTE toDisk);

    // functions to set ParamInfo
    //struct _CWSetParamInfoArgs { 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strParamName);       // parameter name, can be NULL
    //    DECLARE_ECALL_I4_ARG(UINT32, iAttributes);                  // parameter attributes
    //    DECLARE_ECALL_I4_ARG(UINT32, iSequence);                    // sequence of the parameter
    //    DECLARE_ECALL_I4_ARG(UINT32, tkMethod);                     // method token
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL5(int, CWSetParamInfo, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkMethod, UINT32 iSequence, UINT32 iAttributes, StringObject* strParamNameUNSAFE);

    // functions to set FieldMarshal
    //struct _CWSetMarshalArgs { 
    //    DECLARE_ECALL_I4_ARG(UINT32, cbMarshal);                    // number of bytes in Marshal info
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, ubMarshal);         // Marshal info in bytes
    //    DECLARE_ECALL_I4_ARG(UINT32, tk);                           // field or param token
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL4(void, CWSetMarshal, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tk, U1Array* ubMarshalUNSAFE, UINT32 cbMarshal);

    // functions to set default value
    //struct _CWSetConstantValueArgs { 
    //    DECLARE_ECALL_OBJECTREF_ARG(VariantData, varValue);         // constant value
    //    DECLARE_ECALL_I4_ARG(UINT32, tk);                           // field or param token
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};  
    static FCDECL3(void, CWSetConstantValue, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tk, VariantData* pvar);

    // functions to add declarative security
    //struct _CWAddDeclarativeSecurityArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, blob);              // permission set blob
    //    DECLARE_ECALL_I4_ARG(DWORD, action);                        // security action
    //    DECLARE_ECALL_I4_ARG(UINT32, tk);                           // class or method token
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    //};
    static FCDECL4(void, CWAddDeclarativeSecurity, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tk, DWORD action, U1Array* blobUNSAFE);
};



//*********************************************************************
//
// This CSymMapToken class implemented the IMapToken. It is used in catching
// token remap information from Merge and send the notifcation to CeeFileGen
// and SymbolWriter
//
//*********************************************************************
class CSymMapToken : public IMapToken
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, PVOID *pp);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP Map(mdToken tkImp, mdToken tkEmit);
    CSymMapToken(ISymUnmanagedWriter *pWriter, IMapToken *pMapToken);
	~CSymMapToken();
private:
    LONG        m_cRef;
    ISymUnmanagedWriter *m_pWriter;
    IMapToken   *m_pMapToken;
};

#endif  // _COMDYNAMIC_H_   
