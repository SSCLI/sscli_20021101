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

#include "common.h"
#include "field.h"
#include "comdynamic.h"
#include "commodule.h"
#include "reflectclasswriter.h"
#include "corerror.h"
#include "iceefilegen.h"
#include <utilcode.h>
#include "comstring.h"
#include "comdatetime.h"  // DateTime <-> OleAut date conversions
#include "strongname.h"
#include "ceefilegenwriter.h"
#include "comclass.h"


//This structure is used in CWSetMethodIL to walk the exceptions.
//It maps to M/R/Reflection/__ExceptionInstance.class
//DO NOT MOVE ANY OF THE FIELDS                                           
#include <pshpack1.h>
typedef struct {
    INT32 m_exceptionType;
    INT32 m_start;
    INT32 m_end;
    INT32 m_filterOffset;
    INT32 m_handle;
    INT32 m_handleEnd;
    INT32 m_type;
} ExceptionInstance;
#include <poppack.h>


//*************************************************************
// 
// Defining a type into metadata of this dynamic module
//
//*************************************************************
FCIMPL8(UINT32, COMDynamicWrite::CWCreateClass, Object* refThisUNSAFE, StringObject* strFullNameUNSAFE, UINT32 parent, I4Array* interfacesUNSAFE, UINT32 attr, ReflectModuleBaseObject* moduleUNSAFE, GUID guid, INT32 tkEnclosingType)
{
    mdTypeDef           classE; 
    
    struct _gc
    {
        OBJECTREF refThis;
        STRINGREF strFullName;
        I4ARRAYREF interfaces;
        REFLECTMODULEBASEREF module;
    } gc;
    gc.refThis = (OBJECTREF) refThisUNSAFE;
    gc.strFullName = (STRINGREF) strFullNameUNSAFE;
    gc.interfaces = (I4ARRAYREF) interfacesUNSAFE;
    gc.module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HRESULT             hr;
    mdTypeRef           *crImplements;
    RefClassWriter*     pRCW;   
    DWORD               numInterfaces;
    GUID *              pGuid=NULL;
    REFLECTMODULEBASEREF pReflect;

    _ASSERTE(gc.module);
    // Get the module for the creator of the ClassWriter
    pReflect = (REFLECTMODULEBASEREF) gc.module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule); 

    pRCW = GetReflectionModule(pModule)->GetClassWriter();
    _ASSERTE(pRCW);

    // Set up the Interfaces
    if (gc.interfaces!=NULL) {
        unsigned        i = 0;
        crImplements = (mdTypeRef *)_alloca(32 * sizeof(mdTypeRef));
        numInterfaces = gc.interfaces->GetNumComponents();
        int *pImplements = (int *)gc.interfaces->GetDataPtr();
        for (i=0; i<numInterfaces; i++) {
            crImplements[i] = pImplements[i];
        }
        crImplements[i]=mdTypeRefNil;
    } else {
        crImplements=NULL;
    }

    //We know that this space has been allocated because value types cannot be null.
    //If both halves of the guid are 0, we were handed the empty guid and so should
    //pass null to DefineTypeDef.
    //
    if (((INT64 *)(&guid))[0]!=0 && ((INT64 *)(&guid))[1]!=0) {
        pGuid = &guid;
    }

    if (RidFromToken(tkEnclosingType))
    {
        // defining nested type
        hr = pRCW->GetEmitter()->DefineNestedType(gc.strFullName->GetBuffer(), 
                                               attr, 
                                               parent == 0 ? mdTypeRefNil : parent, 
                                               crImplements,  
                                               tkEnclosingType,
                                               &classE);
    }
    else
    {
        // top level type
        hr = pRCW->GetEmitter()->DefineTypeDef(gc.strFullName->GetBuffer(), 
                                               attr, 
                                               parent == 0 ? mdTypeRefNil : parent, 
                                               crImplements,  
                                               &classE);
    }

    if (hr == META_S_DUPLICATE) 
    {
        COMPlusThrow(kArgumentException, L"Argument_DuplicateTypeName");
    } 

    if (FAILED(hr)) {
        _ASSERTE(!"DefineTypeDef Failed");
        COMPlusThrowHR(hr);    
    }

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return (INT32)classE;
}
FCIMPLEND

// CWSetParentType
// ClassWriter.InternalSetParentType -- This function will reset the parent class in metadata
FCIMPL3(void, COMDynamicWrite::CWSetParentType, UINT32 tdType, UINT32 tkParent, ReflectModuleBaseObject* moduleUNSAFE)
{
    REFLECTMODULEBASEREF      pReflect  = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pReflect);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (pReflect == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    HRESULT hr;   

    RefClassWriter* pRCW;   

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Define the Method    
    IfFailGo( pRCW->GetEmitHelper()->SetTypeParent(tdType, tkParent) );
ErrExit:
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineMethod Failed on Method"); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// CWAddInterfaceImpl
// ClassWriter.InternalAddInterfaceImpl -- This function will add another interface impl
FCIMPL3(void, COMDynamicWrite::CWAddInterfaceImpl, UINT32 tdType, UINT32 tkInterface, ReflectModuleBaseObject* moduleUNSAFE)
{
    REFLECTMODULEBASEREF      pReflect  = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pReflect);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (pReflect == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    HRESULT hr;   

    RefClassWriter* pRCW;   

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    IfFailGo( pRCW->GetEmitHelper()->AddInterfaceImpl(tdType, tkInterface) );

ErrExit:
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineMethod Failed on Method"); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


// CWCreateMethod
// ClassWriter.CreateMethod -- This function will create a method within the class

FCIMPL6(UINT32, COMDynamicWrite::CWCreateMethod, UINT32 handle, StringObject* nameUNSAFE, U1Array* signatureUNSAFE, UINT32 sigLength, UINT32 attributes, ReflectModuleBaseObject* moduleUNSAFE)
{
    mdMethodDef memberE; 
    
    struct _gc
    {
        STRINGREF name;
        U1ARRAYREF signature;
        REFLECTMODULEBASEREF module;
    } gc;
    gc.name = (STRINGREF) nameUNSAFE;
    gc.signature = (U1ARRAYREF) signatureUNSAFE;
    gc.module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   
    HRESULT hr; 
    PCCOR_SIGNATURE pcSig;
    

    _ASSERTE(gc.name);   
    RefClassWriter* pRCW;   

    REFLECTMODULEBASEREF      pReflect;
    pReflect = (REFLECTMODULEBASEREF) gc.module;
    _ASSERTE(pReflect);
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE(gc.signature);
    //Ask module for our signature.
    pcSig = (PCCOR_SIGNATURE)gc.signature->GetDataPtr();

    // Define the Method    
    IfFailGo( pRCW->GetEmitter()->DefineMethod(handle,      //ParentTypeDef
                                          gc.name->GetBuffer(), //Name of Member
                                          attributes,               //Member Attributes (public, etc);
                                          pcSig,                    //Blob value of a COM+ signature
                                          sigLength,            //Size of the signature blob
                                          0,                        //Code RVA
                                          miIL | miManaged,         //Implementation Flags is default to managed IL
                                          &memberE) );              //[OUT]methodToken


ErrExit:
    if (FAILED(hr)) 
    {   
        _ASSERTE(!"DefineMethod Failed on Method"); 
        COMPlusThrowHR(hr);
    }   

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return (INT32) memberE;
}
FCIMPLEND


/*================================CWCreateField=================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL6(mdFieldDef, COMDynamicWrite::CWCreateField, UINT32 handle, StringObject* nameUNSAFE, U1Array* signatureUNSAFE, UINT32 sigLength, UINT32 attr, ReflectModuleBaseObject* moduleUNSAFE)
{
    mdFieldDef retVal;
    struct _gc
    {
        STRINGREF name;
        U1ARRAYREF signature;
        REFLECTMODULEBASEREF module;
    } gc;
    gc.name = (STRINGREF) nameUNSAFE;
    gc.signature = (U1ARRAYREF) signatureUNSAFE;
    gc.module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    mdFieldDef FieldDef;
    HRESULT                     hr;
    PCCOR_SIGNATURE pcSig;
    RefClassWriter* pRCW;   

    _ASSERTE(gc.signature);

    //Verify the arguments that we can.
    if (gc.name==NULL) {
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    }

    // Get the RefClassWriter
    REFLECTMODULEBASEREF      pReflect;
    pReflect = (REFLECTMODULEBASEREF) gc.module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter();
    _ASSERTE(pRCW);

    //Ask module for our signature.
    pcSig = (PCCOR_SIGNATURE)gc.signature->GetDataPtr();

    //Emit the field.
    IfFailGo( pRCW->GetEmitter()->DefineField(handle, 
                                         gc.name->GetBuffer(), attr, pcSig,
                                         sigLength, ELEMENT_TYPE_VOID, NULL,
                                         (ULONG) -1, &FieldDef) );


ErrExit:
    if (FAILED(hr)) 
    {
        _ASSERTE(!"DefineField Failed");
        COMPlusThrowHR(hr);
    }    
    retVal = FieldDef;

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


// CWSetMethodIL
// ClassWriter.InternalSetMethodIL -- This function will create a method within the class
FCIMPL11(void, COMDynamicWrite::CWSetMethodIL, UINT32 handle, BYTE isInitLocal, U1Array* bodyUNSAFE, U1Array* localSigUNSAFE, UINT32 sigLength, UINT32 maxStackSize, UINT32 numExceptions, PTRArray* exceptionsUNSAFE, I4Array* tokenFixupsUNSAFE, I4Array* rvaFixupsUNSAFE, ReflectModuleBaseObject* moduleUNSAFE)
{
    struct _gc
    {
        U1ARRAYREF body;
        U1ARRAYREF localSig;
        PTRARRAYREF exceptions;
        I4ARRAYREF tokenFixups;
        I4ARRAYREF rvaFixups;
        REFLECTMODULEBASEREF module;
    } gc;
    gc.body = (U1ARRAYREF) bodyUNSAFE;
    gc.localSig = (U1ARRAYREF) localSigUNSAFE;
    gc.exceptions = (PTRARRAYREF) exceptionsUNSAFE;
    gc.tokenFixups = (I4ARRAYREF) tokenFixupsUNSAFE;
    gc.rvaFixups = (I4ARRAYREF) rvaFixupsUNSAFE;
    gc.module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HRESULT hr;
    HCEESECTION         ilSection;
    INT32               *piRelocs;
    INT32               relocCount=0;
    mdSignature         pmLocalSigToken;

    // Get the RefClassWriter   
    RefClassWriter*     pRCW;   

    REFLECTMODULEBASEREF pReflect;
    
    pReflect = (REFLECTMODULEBASEREF) gc.module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter();
    _ASSERTE(pRCW);
    _ASSERTE(gc.localSig);

    PCCOR_SIGNATURE pcSig = (PCCOR_SIGNATURE)gc.localSig->GetDataPtr();
    _ASSERTE(*pcSig == IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);

    if (sigLength==2 && pcSig[0]==0 && pcSig[1]==0) 
    { 
        
        //This is an empty local variable sig
        pmLocalSigToken=0;
    } 
    else 
    {
        if (FAILED(hr = pRCW->GetEmitter()->GetTokenFromSig( pcSig, sigLength, &pmLocalSigToken))) 
        {

            COMPlusThrowHR(hr);    
        }
    }

    COR_ILMETHOD_FAT fatHeader; 

    // set fatHeader.Flags to CorILMethod_InitLocals if user wants to zero init the stack frame.
    //
    fatHeader.SetFlags(isInitLocal ? CorILMethod_InitLocals : 0);
    fatHeader.SetMaxStack(maxStackSize);
    fatHeader.SetLocalVarSigTok(pmLocalSigToken);
    fatHeader.SetCodeSize((gc.body!=NULL)?gc.body->GetNumComponents():0);  
    bool moreSections            = (numExceptions != 0);    

    unsigned codeSizeAligned     = fatHeader.GetCodeSize();  
    if (moreSections)   
        codeSizeAligned          = (codeSizeAligned + 3) & ~3;    // to insure EH section aligned 
    unsigned headerSize          = COR_ILMETHOD::Size(&fatHeader, numExceptions != 0);    

    //Create the exception handlers.
    COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses = numExceptions <= 0 ? NULL :
        (COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)_alloca(sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT)*numExceptions);   

    if (numExceptions > 0) 
    {
        _ASSERTE(gc.exceptions); 
        _ASSERTE(gc.exceptions->GetNumComponents() == numExceptions);

        ExceptionInstance *exceptions = (ExceptionInstance *)gc.exceptions->GetDataPtr();
        for (unsigned int i = 0; i < numExceptions; i++) 
        {
            clauses[i].SetFlags((CorExceptionFlag)(exceptions[i].m_type | COR_ILEXCEPTION_CLAUSE_OFFSETLEN));
            clauses[i].SetTryOffset(exceptions[i].m_start);
            clauses[i].SetTryLength(exceptions[i].m_end - exceptions[i].m_start);         
            clauses[i].SetHandlerOffset(exceptions[i].m_handle);
            clauses[i].SetHandlerLength(exceptions[i].m_handleEnd - exceptions[i].m_handle);
            if (exceptions[i].m_type == COR_ILEXCEPTION_CLAUSE_FILTER)
            {
                clauses[i].SetFilterOffset(exceptions[i].m_filterOffset);
            }
            else if (exceptions[i].m_type!=COR_ILEXCEPTION_CLAUSE_FINALLY)
            {
                clauses[i].SetClassToken(exceptions[i].m_exceptionType);
            }
            else
            {
                clauses[i].SetClassToken(mdTypeRefNil);
            }
        }
    }
    
    unsigned ehSize          = COR_ILMETHOD_SECT_EH::Size(numExceptions, clauses);    
    unsigned totalSize       = + headerSize + codeSizeAligned + ehSize; 
    ICeeGen* pGen = pRCW->GetCeeGen();
    BYTE* buf = NULL;
    ULONG methodRVA;
    pGen->AllocateMethodBuffer(totalSize, &buf, &methodRVA);    
    if (buf == NULL)
        COMPlusThrowOM();
        
    _ASSERTE(buf != NULL);
    _ASSERTE((((size_t) buf) & 3) == 0);   // header is dword aligned  

#ifdef _DEBUG
    BYTE* endbuf = &buf[totalSize];
#endif

    // Emit the header  
    buf += COR_ILMETHOD::Emit(headerSize, &fatHeader, moreSections, buf);   

    //Emit the code    
    //The fatHeader.CodeSize is a hack to see if we have an interface or an
    //abstract method.  Force enough verification in native to ensure that
    //this is true.
    if (fatHeader.GetCodeSize()!=0) {
        memcpy(buf,gc.body->GetDataPtr(), fatHeader.GetCodeSize());
     }
    buf += codeSizeAligned;
        
    // Emit the eh  
    ULONG* ehTypeOffsets = 0;
    if (ehSize > 0) {
        // Allocate space for the the offsets to the TypeTokens in the Exception headers
        // in the IL stream.
        ehTypeOffsets = (ULONG *)_alloca(sizeof(ULONG) * numExceptions);
        // Emit the eh.  This will update the array ehTypeOffsets with offsets
        // to Exception type tokens.  The offsets are with reference to the
        // beginning of eh section.
        buf += COR_ILMETHOD_SECT_EH::Emit(ehSize, numExceptions, clauses,
                                          false, buf, ehTypeOffsets);
    }   
    _ASSERTE(buf == endbuf);    

    //Get the IL Section.
    if (FAILED(pGen->GetIlSection(&ilSection))) {
        _ASSERTE(!"Unable to get the .il Section.");
        FATAL_EE_ERROR();
    }

    // Token Fixup data...
    ULONG ilOffset = methodRVA + headerSize;

    //Add all of the relocs based on the info which I saved from ILGenerator.
    //Add the RVA Fixups
    if (gc.rvaFixups!=NULL) {
        relocCount = gc.rvaFixups->GetNumComponents();
        piRelocs = (INT32 *)gc.rvaFixups->GetDataPtr();
        for (int i=0; i<relocCount; i++) {
            if (FAILED(pGen->AddSectionReloc(ilSection, piRelocs[i] + ilOffset, ilSection,srRelocAbsolute))) {
                _ASSERTE(!"Unable to add RVA Reloc.");
                FATAL_EE_ERROR();
            }
        }
    }

    if (gc.tokenFixups!=NULL) {
        //Add the Token Fixups
        relocCount = gc.tokenFixups->GetNumComponents();
        piRelocs = (INT32 *)gc.tokenFixups->GetDataPtr();
        for (int i=0; i<relocCount; i++) {
            if (FAILED(pGen->AddSectionReloc(ilSection, piRelocs[i] + ilOffset, ilSection,srRelocMapToken))) {
                _ASSERTE(!"Unable to add Token Reloc.");
                FATAL_EE_ERROR();
            }
        }
    }

    if (ehTypeOffsets) {
        // Add token fixups for exception type tokens.
        for (unsigned int i=0; i < numExceptions; i++) {
            if (ehTypeOffsets[i] != (ULONG) -1) {
                if (FAILED(pGen->AddSectionReloc(
                            ilSection,
                            ehTypeOffsets[i] + codeSizeAligned + ilOffset,
                            ilSection,srRelocMapToken))) {
                    _ASSERTE(!"Unable to add Exception Type Token Reloc.");
                    FATAL_EE_ERROR();
                }
            }
        }
    }

    
    //nasty interface hack.  What does this mean for abstract methods?
    if (fatHeader.GetCodeSize()!=0) {
        DWORD       dwImplFlags;
        //Set the RVA of the method.
        pRCW->GetMDImport()->GetMethodImplProps( handle, NULL, &dwImplFlags );
        dwImplFlags |= miManaged | miIL;
        hr = pRCW->GetEmitter()->SetMethodProps(handle, (DWORD) -1, methodRVA, dwImplFlags);
        if (FAILED(hr)) {
            _ASSERTE(!"SetMethodProps Failed on Method");
            FATAL_EE_ERROR();
        }
    }

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// CWTermCreateClass
// ClassWriter.TermCreateClass --
FCIMPL3(Object*, COMDynamicWrite::CWTermCreateClass, Object* refThisUNSAFE, UINT32 handle, ReflectModuleBaseObject* moduleUNSAFE)
{
    OBJECTREF refRetVal = NULL;
    OBJECTREF refThis = (OBJECTREF) refThisUNSAFE;
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, module);
    //-[autocvtpro]-------------------------------------------------------

    RefClassWriter*         pRCW;   
    REFLECTMODULEBASEREF    pReflect;
    OBJECTREF               Throwable = NULL;
    UINT                    resId = IDS_CLASSLOAD_GENERIC;
 
    THROWSCOMPLUSEXCEPTION();
    
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    // Use the same service, regardless of whether we are generating a normal
    // class, or the special class for the module that holds global functions
    // & methods.
    GetReflectionModule(pModule)->AddClass(handle); 

    GCPROTECT_BEGIN(Throwable);
    
    // manually load the class if it is not the global type
    if (!IsNilToken(handle))
    {
        TypeHandle typeHnd;
        
        BEGIN_ENSURE_PREEMPTIVE_GC();
        typeHnd = pModule->GetClassLoader()->LoadTypeHandle(pModule, handle, &Throwable);
        END_ENSURE_PREEMPTIVE_GC();

        if (typeHnd.IsNull() ||
            (Throwable != NULL) ||
            (typeHnd.GetModule() != pModule))
        {
            // error handling code
            if (Throwable == NULL)
                pModule->GetAssembly()->PostTypeLoadException(pRCW->GetMDImport(), handle, resId, &Throwable);

            COMPlusThrow(Throwable);
        }

        refRetVal = typeHnd.CreateClassObj();
    }
    GCPROTECT_END();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


/*============================InternalSetPInvokeData============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL6(void, COMDynamicWrite::InternalSetPInvokeData, ReflectModuleBaseObject* moduleUNSAFE, StringObject* dllNameUNSAFE, StringObject* functionNameUNSAFE, UINT32 token, UINT32 linkType, UINT32 linkFlags)
{
    struct _gc
    {
        REFLECTMODULEBASEREF module;
        STRINGREF dllName;
        STRINGREF functionName;
    } gc;
    gc.module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    gc.dllName = (STRINGREF) dllNameUNSAFE;
    gc.functionName = (STRINGREF) functionNameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    
    RefClassWriter      *pRCW;
    DWORD               dwMappingFlags = 0;
    mdModuleRef         mrImportDll = mdTokenNil;
    HRESULT             hr;

    _ASSERTE(gc.dllName);

    Module* pModule = (Module *)gc.module->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    IfFailGo(pRCW->GetEmitter()->DefineModuleRef(gc.dllName->GetBufferNullable(), &mrImportDll));
    dwMappingFlags = linkType | linkFlags;
    IfFailGo(pRCW->GetEmitter()->DefinePinvokeMap(
        token,                        // the method token 
        dwMappingFlags,                     // the mapping flags
        gc.functionName->GetBuffer(),    // function name
        mrImportDll));

    pRCW->GetEmitter()->SetMethodProps(token, (DWORD) -1, 0x0, miIL);
ErrExit:
    if (FAILED(hr))
    {
        COMPlusThrowHR(hr);    
    }

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND
    

/*============================CWDefineProperty============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL8(UINT32, COMDynamicWrite::CWDefineProperty, ReflectModuleBaseObject* moduleUNSAFE, UINT32 handle, StringObject* nameUNSAFE, UINT32 attr, U1Array* signatureUNSAFE, UINT32 sigLength, UINT32 tkNotifyChanging, UINT32 tkNotifyChanged)
{
    mdProperty      pr; 
    
    struct _gc
    {
        REFLECTMODULEBASEREF module;
        STRINGREF name;
        U1ARRAYREF signature;
    } gc;
    gc.module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    gc.name = (STRINGREF) nameUNSAFE;
    gc.signature = (U1ARRAYREF) signatureUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    PCCOR_SIGNATURE pcSig;
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) gc.module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE((gc.signature != NULL) && (gc.name!= NULL));

    //Ask module for our signature.
    pcSig = (PCCOR_SIGNATURE)gc.signature->GetDataPtr();

    // Define the Property
    hr = pRCW->GetEmitter()->DefineProperty(
            handle,                         // ParentTypeDef
            gc.name->GetBuffer(),           // Name of Member
            attr,                     // property Attributes (prDefaultProperty, etc);
            pcSig,                          // Blob value of a COM+ signature
            sigLength,                // Size of the signature blob
            ELEMENT_TYPE_VOID,              // don't specify the default value
            0,                              // no default value
            (ULONG) -1,                     // optional length
            mdMethodDefNil,                 // no setter
            mdMethodDefNil,                 // no getter
            NULL,                           // no other methods
            &pr);

    if (FAILED(hr)) {   
        _ASSERTE(!"DefineProperty Failed on Property"); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return (INT32)pr;
}
FCIMPLEND



/*============================CWDefineEvent============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL5(UINT32, COMDynamicWrite::CWDefineEvent, ReflectModuleBaseObject* moduleUNSAFE, UINT32 handle, StringObject* nameUNSAFE, UINT32 attr, UINT32 eventtype)
{
    mdProperty      ev; 

    
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    STRINGREF name = (STRINGREF) nameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(module, name);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;


    _ASSERTE(name);   

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Define the Event
    hr = pRCW->GetEmitHelper()->DefineEventHelper(
            handle,                 // ParentTypeDef
            name->GetBuffer(),      // Name of Member
            attr,                       // property Attributes (prDefaultProperty, etc);
            eventtype,              // the event type. Can be TypeDef or TypeRef
            &ev);

    if (FAILED(hr)) 
    {   
        _ASSERTE(!"DefineEvent Failed on Event"); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();

    return (INT32)ev;
}
FCIMPLEND





/*============================CWDefineMethodSemantics============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL4(void, COMDynamicWrite::CWDefineMethodSemantics, ReflectModuleBaseObject* moduleUNSAFE, UINT32 association, UINT32 attr, UINT32 method)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(module);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    

    // Define the MethodSemantics
    hr = pRCW->GetEmitHelper()->DefineMethodSemanticsHelper(
            association,
            attr,
            method);
    if (FAILED(hr)) 
    {   
        _ASSERTE(!"DefineMethodSemantics Failed on "); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


/*============================CWSetMethodImpl============================
** To set a Method's Implementation flags
==============================================================================*/
FCIMPL3(void, COMDynamicWrite::CWSetMethodImpl, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkMethod, UINT32 attr)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(module);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the methodimpl flags
    hr = pRCW->GetEmitter()->SetMethodImplFlags(
            tkMethod,
            attr);                // change the impl flags
    if (FAILED(hr)) {   
        _ASSERTE(!"SetMethodImplFlags Failed"); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


/*============================CWDefineMethodImpl============================
** Define a MethodImpl record
==============================================================================*/
FCIMPL4(void, COMDynamicWrite::CWDefineMethodImpl, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkType, UINT32 tkBody, UINT32 tkDecl)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(module);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the methodimpl flags
    hr = pRCW->GetEmitter()->DefineMethodImpl(
            tkType,
            tkBody,
            tkDecl);                  // change the impl flags
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineMethodImpl Failed"); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


/*============================CWGetTokenFromSig============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL3(int, COMDynamicWrite::CWGetTokenFromSig, ReflectModuleBaseObject* moduleUNSAFE, U1Array* signatureUNSAFE, UINT32 sigLength)
{
    int retVal;
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    U1ARRAYREF signature = (U1ARRAYREF) signatureUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(module, signature);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    mdSignature     sig; 
    PCCOR_SIGNATURE pcSig;
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE(signature);

    // get the signature
    pcSig = (PCCOR_SIGNATURE)signature->GetDataPtr();

    // Define the signature
    hr = pRCW->GetEmitter()->GetTokenFromSig(
            pcSig,                          // Signature blob
            sigLength,                      // blob length
            &sig);                          // returned token

    if (FAILED(hr)) {   
        _ASSERTE(!"GetTokenFromSig Failed"); 
        COMPlusThrowHR(hr);    
    }   


    // Return the token via the hidden param. 
    retVal = (INT32)sig;   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND



/*============================CWSetParamInfo============================
**Action: Helper to set parameter information
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL5(int, COMDynamicWrite::CWSetParamInfo, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkMethod, UINT32 iSequence, UINT32 iAttributes, StringObject* strParamNameUNSAFE)
{
    int retVal;
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    STRINGREF strParamName = (STRINGREF) strParamNameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(module, strParamName);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;
    WCHAR*          wzParamName = strParamName->GetBufferNullable();  
    mdParamDef      pd;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the methodimpl flags
    hr = pRCW->GetEmitter()->DefineParam(
            tkMethod,
            iSequence,            // sequence of the parameter
            wzParamName, 
            iAttributes,          // change the impl flags
            ELEMENT_TYPE_VOID,
            0,
            (ULONG) -1,
            &pd);
    if (FAILED(hr)) {   
        _ASSERTE(!"DefineParam Failed on "); 
        COMPlusThrowHR(hr);    
    }   
    retVal = (INT32)pd;   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}   // COMDynamicWrite::CWSetParamInfo
FCIMPLEND



/*============================CWSetMarshal============================
**Action: Helper to set marshal information
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL4(void, COMDynamicWrite::CWSetMarshal, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tk, U1Array* ubMarshalUNSAFE, UINT32 cbMarshal)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    U1ARRAYREF ubMarshal = (U1ARRAYREF) ubMarshalUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_2(module, ubMarshal);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    PCCOR_SIGNATURE pcMarshal;
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    _ASSERTE(ubMarshal);

    // get the signature
    pcMarshal = (PCCOR_SIGNATURE)ubMarshal->GetDataPtr();

    // Define the signature
    hr = pRCW->GetEmitter()->SetFieldMarshal(
            tk,
            pcMarshal,                      // marshal blob
            cbMarshal);               // blob length

    if (FAILED(hr)) {   
        _ASSERTE(!"Set FieldMarshal is failing"); 
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}   // COMDynamicWrite::CWSetMarshal
FCIMPLEND



/*============================CWSetConstantValue============================
**Action: Helper to set constant value to field or parameter
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL3(void, COMDynamicWrite::CWSetConstantValue, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tk, VariantData* pvar)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(module);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    REFLECTMODULEBASEREF    pReflect; 
    RefClassWriter* pRCW;   
    Module          *pModule;
    DWORD           dwCPlusTypeFlag = 0;
    void            *pValue = NULL;
    OBJECTREF       obj;
    int             strLen;
    BYTE            tempBuff[4];


    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    // Initialize the type
    dwCPlusTypeFlag = pvar->GetType(); 

    switch (pvar->GetType())
    {
        case CV_BOOLEAN:
        case CV_I1:
            pValue = tempBuff;
            *(INT8 *)tempBuff = pvar->GetDataAsInt8();
            break;
        case CV_U1:
            pValue = tempBuff;
            *(UINT8 *)tempBuff = pvar->GetDataAsUInt8();
            break;

        case CV_I2:
            pValue = tempBuff;
            *(INT16 *)tempBuff = pvar->GetDataAsInt16();
            break;
        case CV_U2:
        case CV_CHAR:
            pValue = tempBuff;
            *(UINT16 *)tempBuff = pvar->GetDataAsUInt16();
            break;

        case CV_I4:
            pValue = tempBuff;
            *(INT32 *)tempBuff = pvar->GetDataAsInt32();
            break;
        case CV_U4:
        case CV_R4: 
            pValue = tempBuff;
            *(UINT32 *)tempBuff = pvar->GetDataAsUInt32();
            break;

        case CV_I8:
        case CV_U8:
        case CV_R8:
            pValue = pvar->GetData();
            break;

        case CV_DATETIME: //date is a I8 representation
        case CV_CURRENCY: // currency is a I8 representation
            dwCPlusTypeFlag = ELEMENT_TYPE_I8;
            pValue = pvar->GetData();
            break;

        case CV_STRING:
            if (pvar->GetObjRef() == NULL) 
            {
                pValue = NULL;
            }
            else
            {
                RefInterpretGetStringValuesDangerousForGC((STRINGREF) (pvar->GetObjRef()), (WCHAR **)&pValue, &strLen);
            }
            dwCPlusTypeFlag = ELEMENT_TYPE_STRING;
            break;

        case CV_DECIMAL:
            // Decimal is 12 bytes. Don't know how to represent this
        case CV_OBJECT:
            // for DECIMAL and Object, we only support NULL default value.
            // This is a constraint from MetaData.
            //
            obj = pvar->GetObjRef();
            if ((obj!=NULL) && (obj->GetData()))
            {
                // can only accept the NULL object
                COMPlusThrow(kArgumentException, L"Argument_BadConstantValue");    
            }

            // fail through

        case CV_NULL:
            dwCPlusTypeFlag = ELEMENT_TYPE_CLASS;
            pValue = NULL;
            break;

        case CV_ENUM:
            // always map the enum value to a I4 value
            dwCPlusTypeFlag = ELEMENT_TYPE_I4;
            pValue = tempBuff;
            *(INT32 *)tempBuff = pvar->GetDataAsInt32();
            break;

        case CV_EMPTY:
            dwCPlusTypeFlag = ELEMENT_TYPE_CLASS;
            pValue = NULL;
            break;

        default:
        case CV_TIMESPAN:
            _ASSERTE(!"Not valid type!");

            // cannot specify default value
            COMPlusThrow(kArgumentException, L"Argument_BadConstantValue");    
            break;
    }

    if (TypeFromToken(tk) == mdtFieldDef)
    {
        hr = pRCW->GetEmitter()->SetFieldProps( 
            tk,                         // [IN] The FieldDef.
            ULONG_MAX,                  // [IN] Field attributes.
            dwCPlusTypeFlag,            // [IN] Flag for the value type, selected ELEMENT_TYPE_*
            pValue,                     // [IN] Constant value.
            (ULONG) -1);                // [IN] Optional length.
    }
    else if (TypeFromToken(tk) == mdtProperty)
    {
        hr = pRCW->GetEmitter()->SetPropertyProps( 
            tk,                         // [IN] The PropertyDef.
            ULONG_MAX,                  // [IN] Property attributes.
            dwCPlusTypeFlag,            // [IN] Flag for the value type, selected ELEMENT_TYPE_*
            pValue,                     // [IN] Constant value.
            (ULONG) -1,                 // [IN] Optional length.
            mdMethodDefNil,             // [IN] Getter method.
            mdMethodDefNil,             // [IN] Setter method.
            NULL);                      // [IN] Other methods.
    }
    else
    {
        hr = pRCW->GetEmitter()->SetParamProps( 
            tk,                   // [IN] The ParamDef.
            NULL,
            ULONG_MAX,                  // [IN] Parameter attributes.
            dwCPlusTypeFlag,            // [IN] Flag for the value type, selected ELEMENT_TYPE_*
            pValue,                     // [IN] Constant value.
            (ULONG) -1);                // [IN] Optional length.
    }
    if (FAILED(hr)) {   
        _ASSERTE(!"Set default value is failing"); 
        COMPlusThrow(kArgumentException, L"Argument_BadConstantValue");    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}   // COMDynamicWrite::CWSetConstantValue
FCIMPLEND


/*============================CWSetFieldLayoutOffset============================
**Action: set fieldlayout of a field
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL3(void, COMDynamicWrite::CWSetFieldLayoutOffset, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tkField, UINT32 iOffset)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(module);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;

    _ASSERTE(RidFromToken(tkField) != mdTokenNil);

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Set the field layout
    hr = pRCW->GetEmitHelper()->SetFieldLayoutHelper(
            tkField,                  // field 
            iOffset);                 // layout offset

    if (FAILED(hr)) {   
        _ASSERTE(!"SetFieldLayoutHelper failed");
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


/*============================CWSetClassLayout============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL4(void, COMDynamicWrite::CWSetClassLayout, ReflectModuleBaseObject* moduleUNSAFE, UINT32 handle, UINT32 iPackSize, UINT32 iTotalSize)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(module);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();   

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    REFLECTMODULEBASEREF    pReflect; 
    Module          *pModule;


    _ASSERTE(handle);

    // Get the RefClassWriter   
    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    
    pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    // Define the packing size and total size of a class
    hr = pRCW->GetEmitter()->SetClassLayout(
            handle,                   // Typedef
            iPackSize,                // packing size
            NULL,                     // no field layout 
            iTotalSize);              // total size for the type

    if (FAILED(hr)) {   
        _ASSERTE(!"SetClassLayout failed");
        COMPlusThrowHR(hr);    
    }   

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

/*===============================UpdateMethodRVAs===============================
**Action: Update the RVAs in all of the methods associated with a particular typedef
**        to prior to emitting them to a PE.
**Returns: Void
**Arguments:
**Exceptions:
==============================================================================*/
void COMDynamicWrite::UpdateMethodRVAs(IMetaDataEmit *pEmitNew,
                                  IMetaDataImport *pImportNew,
                                  ICeeFileGen *pCeeFileGen, 
                                  HCEEFILE ceeFile, 
                                  mdTypeDef td,
                                  HCEESECTION sdataSection)
{
    THROWSCOMPLUSEXCEPTION();

    HCORENUM    hEnum=0;
    ULONG       methRVA;
    ULONG       newMethRVA;
    ULONG       sdataSectionRVA = 0;
    mdMethodDef md;
    mdFieldDef  fd;
    ULONG       count;
    DWORD       dwFlags=0;
    DWORD       implFlags=0;
    HRESULT     hr;

    // Look at the typedef flags.  Skip tdimport classes.
    if (!IsNilToken(td))
    {
        IfFailGo(pImportNew->GetTypeDefProps(td, 0,0,0, &dwFlags, 0));
        if (IsTdImport(dwFlags))
            goto ErrExit;
    }
    
    //Get an enumerator and use it to walk all of the methods defined by td.
    while ((hr = pImportNew->EnumMethods(
        &hEnum, 
        td, 
        &md, 
        1, 
        &count)) == S_OK) {
        
        IfFailGo( pImportNew->GetMethodProps(
            md, 
            NULL, 
            NULL,           // don't get method name
            0, 
            NULL, 
            &dwFlags, 
            NULL, 
            NULL, 
            &methRVA, 
            &implFlags) );

        // If this method isn't implemented here, don't bother correcting it's RVA
        // Otherwise, get the correct RVA from our ICeeFileGen and put it back into our local
        // copy of the metadata
        //
        if ( IsMdAbstract(dwFlags) || IsMdPinvokeImpl(dwFlags) ||
             IsMiNative(implFlags) || IsMiRuntime(implFlags) ||
             IsMiForwardRef(implFlags))
        {
            continue;
        }
            
        IfFailGo( pCeeFileGen->GetMethodRVA(ceeFile, methRVA, &newMethRVA) );
        IfFailGo( pEmitNew->SetRVA(md, newMethRVA) );
    }
        
    if (hEnum) {
        pImportNew->CloseEnum( hEnum);
    }
    hEnum = 0;

    // Walk through all of the Field belongs to this TypeDef. If field is marked as fdHasFieldRVA, we need to update the
    // RVA value.
    while ((hr = pImportNew->EnumFields(
        &hEnum, 
        td, 
        &fd, 
        1, 
        &count)) == S_OK) {
        
        IfFailGo( pImportNew->GetFieldProps(
            fd, 
            NULL,           // don't need the parent class
            NULL,           // don't get method name
            0, 
            NULL, 
            &dwFlags,       // field flags
            NULL,           // don't need the signature
            NULL, 
            NULL,           // don't need the constant value
            0,
            NULL) );

        if ( IsFdHasFieldRVA(dwFlags) )
        {            
            if (sdataSectionRVA == 0)
            {
                IfFailGo( pCeeFileGen->GetSectionCreate (ceeFile, ".sdata", sdReadWrite, &(sdataSection)) );
                IfFailGo( pCeeFileGen->GetSectionRVA(sdataSection, &sdataSectionRVA) );
            }

            IfFailGo( pImportNew->GetRVA(fd, &methRVA, NULL) );
            newMethRVA = methRVA + sdataSectionRVA;
            IfFailGo( pEmitNew->SetFieldRVA(fd, newMethRVA) );
        }
    }
        
    if (hEnum) {
        pImportNew->CloseEnum( hEnum);
    }
    hEnum = 0;

ErrExit:
    if (FAILED(hr)) {   
        _ASSERTE(!"UpdateRVA failed");
        COMPlusThrowHR(hr);    
    }   
}

FCIMPL5(void, COMDynamicWrite::CWInternalCreateCustomAttribute, UINT32 token, UINT32 conTok, U1Array* blobUNSAFE, ReflectModuleBaseObject* moduleUNSAFE, BYTE toDisk)
{
    U1ARRAYREF blob = (U1ARRAYREF) blobUNSAFE;
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_2(blob, module);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HRESULT hr;
    mdCustomAttribute retToken;

    // Get the RefClassWriter   
    REFLECTMODULEBASEREF    pReflect = (REFLECTMODULEBASEREF) module;
    _ASSERTE(pReflect);
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    RefClassWriter* pRCW = GetReflectionModule(pModule)->GetClassWriter();
    _ASSERTE(pRCW);

    if (toDisk && pRCW->GetOnDiskEmitter())
    {
        hr = pRCW->GetOnDiskEmitter()->DefineCustomAttribute(
                token,
                conTok,
                blob->GetDataPtr(),
                blob->GetNumComponents(),
                &retToken); 
    }
    else
    {
        hr = pRCW->GetEmitter()->DefineCustomAttribute(
                token,
                conTok,
                blob->GetDataPtr(),
                blob->GetNumComponents(),
                &retToken); 
    }

    if (FAILED(hr))
    {
        // See if the metadata engine gave us any error information.
        IErrorInfo *pIErrInfo;
        if (GetErrorInfo(0, &pIErrInfo) == S_OK)
        {
            BSTR bstrMessage = NULL;
            if (SUCCEEDED(pIErrInfo->GetDescription(&bstrMessage)) && bstrMessage != NULL)
            {
                LPWSTR wszMessage = (LPWSTR)_alloca(*((DWORD*)bstrMessage - 1) + sizeof(WCHAR));
                wcscpy(wszMessage, (LPWSTR)bstrMessage);
                SysFreeString(bstrMessage);
                pIErrInfo->Release();
                COMPlusThrow(kArgumentException, IDS_EE_INVALID_CA_EX, wszMessage);
            }
            pIErrInfo->Release();
        }

        COMPlusThrow(kArgumentException, IDS_EE_INVALID_CA);
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND



//=============================PreSavePEFile=====================================*/
// PreSave the PEFile
//==============================================================================*/
FCIMPL1(void, COMDynamicWrite::PreSavePEFile, Object* refThisUNSAFE)
{
    OBJECTREF refThis = (OBJECTREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(refThis);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    RefClassWriter  *pRCW;   
    ICeeGen         *pICG;
    HCEEFILE        ceeFile=NULL;
    ICeeFileGen     *pCeeFileGen;
    HRESULT         hr=S_OK;
    IMetaDataDispenserEx *pDisp = NULL;
    IMetaDataEmit   *pEmitNew = NULL;
    IMetaDataEmit   *pEmit = NULL;
    IMetaDataImport *pImport = NULL;
    IUnknown        *pUnknown = NULL;
    IMapToken       *pIMapToken = NULL;
    REFLECTMODULEBASEREF pReflect;
    VARIANT         varOption;
    ISymUnmanagedWriter *pWriter = NULL;
    CSymMapToken    *pSymMapToken = NULL;
    CQuickArray<WCHAR> cqModuleName;
    ULONG           cchName;

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) refThis;    

    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    pICG = pRCW->GetCeeGen(); //This method is actually misnamed. It returns an ICeeGen.
    _ASSERTE(pICG);

    IfFailGo ( pRCW->EnsureCeeFileGenCreated() );

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    // We should not have the on disk emitter yet
    if (pRCW->GetOnDiskEmitter() != NULL)
        pRCW->SetOnDiskEmitter(NULL);

    // Get the dispenser.
    if (FAILED(MetaDataGetDispenser(CLSID_CorMetaDataDispenser,IID_IMetaDataDispenserEx,(void**)&pDisp))) {
        _ASSERTE(!"Unable to get dispenser.");
        FATAL_EE_ERROR();
    }

    //Get the emitter and the importer
    pImport = pRCW->GetImporter();
    pEmit = pRCW->GetEmitter();
    _ASSERTE(pEmit && pImport);

    // Set the option on the dispenser turn on duplicate check for TypeDef and moduleRef
    V_VT(&varOption) = VT_UI4;
    V_I4(&varOption) = MDDupDefault | MDDupTypeDef | MDDupModuleRef | MDDupExportedType | MDDupAssemblyRef;
    IfFailGo( pDisp->SetOption(MetaDataCheckDuplicatesFor, &varOption) );

    V_VT(&varOption) = VT_UI4;
    V_I4(&varOption) = MDRefToDefNone;
    IfFailGo( pDisp->SetOption(MetaDataRefToDefCheck, &varOption) );

    //Define an empty scope
    IfFailGo( pDisp->DefineScope(CLSID_CorMetaDataRuntime, 0, IID_IMetaDataEmit, (IUnknown**)&pEmitNew));

    // Token can move upon merge. Get the IMapToken from the CeeFileGen that is created for save
    // and pass it to merge to receive token movement notification.
    // Note that this is not a long term fix. We are relying on the fact that those tokens embedded
    // in PE cannot move after the merge. These tokens are TypeDef, TypeRef, MethodDef, FieldDef, MemberRef,
    // TypeSpec, UserString. If this is no longer true, we can break!
    //
    // Note that we don't need to release pIMapToken because it is not AddRef'ed in the GetIMapTokenIfaceEx.
    //
    IfFailGo( pCeeFileGen->GetIMapTokenIfaceEx(ceeFile, pEmit, &pUnknown));

    IfFailGo( pUnknown->QueryInterface(IID_IMapToken, (void **) &pIMapToken));

    // get the unmanaged writer.
    pWriter = GetReflectionModule(pModule)->GetISymUnmanagedWriter();
    pSymMapToken = new CSymMapToken(pWriter, pIMapToken);
    if (!pSymMapToken)
        IfFailGo(E_OUTOFMEMORY);


    //Merge the old tokens into the new (empty) scope
    //This is a copy.
    IfFailGo( pEmitNew->Merge(pImport, pSymMapToken, NULL));
    IfFailGo( pEmitNew->MergeEnd());

    // Update the Module name in the new scope.
    IfFailGo(pImport->GetScopeProps(0, 0, &cchName, 0));
    IfFailGo(cqModuleName.ReSize(cchName));
    IfFailGo(pImport->GetScopeProps(cqModuleName.Ptr(), cchName, &cchName, 0));
    IfFailGo(pEmitNew->SetModuleProps(cqModuleName.Ptr()));

    // cache the pEmitNew to RCW!!
    pRCW->SetOnDiskEmitter(pEmitNew);

ErrExit:
    //Release the interfaces.  This should free some of the associated resources.
    if (pEmitNew)
        pEmitNew->Release();
    if (pDisp)
        pDisp->Release();
   
    if (pIMapToken)
        pIMapToken->Release();

    if (pSymMapToken)
        pSymMapToken->Release();

    if (FAILED(hr)) 
    {
        COMPlusThrowHR(hr);
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//=============================SavePEFile=====================================*/
// Save the PEFile to disk
//==============================================================================*/
FCIMPL5(void, COMDynamicWrite::SavePEFile, Object* refThisUNSAFE, StringObject* peNameUNSAFE, UINT32 entryPoint, UINT32 fileKind, UINT32 isManifestFile)
{
    OBJECTREF refThis = (OBJECTREF) refThisUNSAFE;
    STRINGREF peName = (STRINGREF) peNameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_2(refThis, peName);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    RefClassWriter  *pRCW;   
    ICeeGen         *pICG;
    HCEEFILE        ceeFile=NULL;
    ICeeFileGen     *pCeeFileGen;
    HRESULT         hr=S_OK;
    HCORENUM        hTypeDefs=0;
    mdTypeDef       td;
    ULONG           count;
    IMetaDataEmit   *pEmitNew = 0;
    IMetaDataImport *pImportNew = 0;
    REFLECTMODULEBASEREF pReflect;
    Assembly        *pAssembly;
    ULONG           newMethRVA;
    DWORD           metaDataSize;   
    BYTE            *metaData;
    ULONG           metaDataOffset;
    HCEESECTION     pILSection;
    ISymUnmanagedWriter *pWriter = NULL;


    if (peName==NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    if (peName->GetStringLength()==0)
        COMPlusThrow(kFormatException, L"Format_StringZeroLength");

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pAssembly = pModule->GetAssembly();
    _ASSERTE( pAssembly );

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    pICG = pRCW->GetCeeGen(); //This method is actually misnamed. It returns an ICeeGen.
    _ASSERTE(pICG);

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    pEmitNew = pRCW->GetOnDiskEmitter();
    _ASSERTE(pEmitNew);

    //Get the emitter and the importer 

    if (pAssembly->IsDynamic() && isManifestFile)
    {
        // manifest is stored in this file

        // Allocate space for a strong name signature if an originator was supplied
        // (this doesn't strong name the assembly, but it makes it possible to do so
        // as a post processing step).
        if (pAssembly->IsStrongNamed())
            IfFailGo(pAssembly->AllocateStrongNameSignature(pCeeFileGen, ceeFile));
    }

    //Set the Output FileName
    IfFailGo( pCeeFileGen->SetOutputFileName(ceeFile, peName->GetBuffer()) );
    
    //Set the Entry Point or throw the dll switch if we're creating a dll.
    if (entryPoint!=0) 
    {
        IfFailGo( pCeeFileGen->SetEntryPoint(ceeFile, entryPoint) );
    }

    switch (fileKind)
    {
        case Dll:
        {
            IfFailGo( pCeeFileGen->SetDllSwitch(ceeFile, true) );
            break;
        }
        case WindowApplication:
        {
            // window application. Set the SubSystem
            IfFailGo( pCeeFileGen->SetSubsystem(ceeFile, IMAGE_SUBSYSTEM_WINDOWS_GUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION) );
            break;
        }
        case ConsoleApplication:
        {
            // Console application. Set the SubSystem
            IfFailGo( pCeeFileGen->SetSubsystem(ceeFile, IMAGE_SUBSYSTEM_WINDOWS_CUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION) );
            break;
        }
        default:
        {
            _ASSERTE("Unknown file kind!");
            break;
        }
    }

    IfFailGo( pCeeFileGen->GetIlSection(ceeFile, &pILSection) );
    IfFailGo( pEmitNew->GetSaveSize(cssAccurate, &metaDataSize) );
    IfFailGo( pCeeFileGen->GetSectionBlock(pILSection, metaDataSize, sizeof(DWORD), (void**) &metaData) );
    IfFailGo( pCeeFileGen->GetSectionDataLen(pILSection, &metaDataOffset) );
    metaDataOffset -= metaDataSize;

    // get the unmanaged writer.
    pWriter = GetReflectionModule(pModule)->GetISymUnmanagedWriter();
    IfFailGo( EmitDebugInfoBegin(pModule, pCeeFileGen, ceeFile, pILSection, peName->GetBuffer(), pWriter) );

    if (pAssembly->IsDynamic() && pRCW->m_ulResourceSize)
    {
        // There are manifest in this file

        IfFailGo( pCeeFileGen->GetMethodRVA(ceeFile, 0, &newMethRVA) );            

        // Point to manifest resource
        IfFailGo( pCeeFileGen->SetManifestEntry( ceeFile, pRCW->m_ulResourceSize, newMethRVA ) );
    }

    IfFailGo( pCeeFileGen->LinkCeeFile(ceeFile) );

    // Get the import interface from the new Emit interface.
    IfFailGo( pEmitNew->QueryInterface(IID_IMetaDataImport, (void **)&pImportNew));


    //Enumerate the TypeDefs and update method RVAs.
    while ((hr = pImportNew->EnumTypeDefs( &hTypeDefs, &td, 1, &count)) == S_OK) 
    {
        UpdateMethodRVAs(pEmitNew, pImportNew, pCeeFileGen, ceeFile, td, ((ReflectionModule*) pModule)->m_sdataSection);
    }

    if (hTypeDefs) 
    {
        pImportNew->CloseEnum(hTypeDefs);
    }
    hTypeDefs=0;
    
    //Update Global Methods.
    UpdateMethodRVAs(pEmitNew, pImportNew, pCeeFileGen, ceeFile, 0, ((ReflectionModule*) pModule)->m_sdataSection);
    

    //Emit the MetaData 
    // IfFailGo( pCeeFileGen->EmitMetaDataEx(ceeFile, pEmitNew));
    IfFailGo( pCeeFileGen->EmitMetaDataAt(ceeFile, pEmitNew, pILSection, metaDataOffset, metaData, metaDataSize) );

    // finish the debugging info emitting after the metadata save so that token remap will be caught correctly
    IfFailGo( EmitDebugInfoEnd(pModule, pCeeFileGen, ceeFile, pILSection, peName->GetBuffer(), pWriter) );

    //Generate the CeeFile
    IfFailGo(pCeeFileGen->GenerateCeeFile(ceeFile) );

    // Strong name sign the resulting assembly if required.
    if (pAssembly->IsDynamic() && isManifestFile && pAssembly->IsStrongNamed())
        IfFailGo(pAssembly->SignWithStrongName(peName->GetBuffer()));

ErrExit:

    pRCW->SetOnDiskEmitter(NULL);

    //Release the interfaces.  This should free some of the associated resources.
    if (pImportNew)
        pImportNew->Release();

    //Release our interfaces if we allocated them to begin with
    pRCW->DestroyCeeFileGen();

    //Check all file IO errors. If so, throw IOException. Otherwise, just throw the hr.
    if (FAILED(hr))
    {
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
            SCODE       scode = HRESULT_CODE(hr);
            WCHAR       wzErrorInfo[MAX_PATH];
            WszFormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                0, 
                hr,
                0,
                wzErrorInfo,
                MAX_PATH,
                0);
            if (IsWin32IOError(scode))
            {
                COMPlusThrowHR(COR_E_IO, wzErrorInfo);
            }
            else
            {
                COMPlusThrowHR(hr, wzErrorInfo);
            }
        }
        COMPlusThrowHR(hr);
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//=============================EmitDebugInfoBegin============================*/
// Phase 1 of emit debugging directory and symbol file.
//===========================================================================*/
HRESULT COMDynamicWrite::EmitDebugInfoBegin(Module *pModule,
                                       ICeeFileGen *pCeeFileGen,
                                       HCEEFILE ceeFile,
                                       HCEESECTION pILSection,
                                       WCHAR *filename,
                                       ISymUnmanagedWriter *pWriter)
{
    HRESULT hr = S_OK;  






    // If we were emitting symbols for this dynamic module, go ahead
    // and fill out the debug directory and save off the symbols now.
    if (pWriter != NULL)
    {
        IMAGE_DEBUG_DIRECTORY  debugDirIDD;
        DWORD                  debugDirDataSize;
        BYTE                  *debugDirData;

        // Grab the debug info.
        IfFailGo(pWriter->GetDebugInfo(NULL, 0, &debugDirDataSize, NULL));

            
        // Is there any debug info to emit?
        if (debugDirDataSize > 0)
        {
            // Make some room for the data.
            debugDirData = (BYTE*)_alloca(debugDirDataSize);

            // Actually get the data now.
            IfFailGo(pWriter->GetDebugInfo(&debugDirIDD,
                                             debugDirDataSize,
                                             NULL,
                                             debugDirData));


            // Grab the timestamp of the PE file.
            time_t fileTimeStamp;


            IfFailGo(pCeeFileGen->GetFileTimeStamp(ceeFile, &fileTimeStamp));


            // Fill in the directory entry.
            debugDirIDD.TimeDateStamp = VAL32(fileTimeStamp);
            debugDirIDD.AddressOfRawData = 0;

            // Grab memory in the section for our stuff.
            HCEESECTION sec = pILSection;
            BYTE *de;

            IfFailGo(pCeeFileGen->GetSectionBlock(sec,
                                                    sizeof(debugDirIDD) +
                                                    debugDirDataSize,
                                                    4,
                                                    (void**) &de) );


            // Where did we get that memory?
            ULONG deOffset;
            IfFailGo(pCeeFileGen->GetSectionDataLen(sec, &deOffset));


            deOffset -= (sizeof(debugDirIDD) + debugDirDataSize);

            // Setup a reloc so that the address of the raw data is
            // setup correctly.
            debugDirIDD.PointerToRawData = VAL32(deOffset + sizeof(debugDirIDD));
                    
            IfFailGo(pCeeFileGen->AddSectionReloc(
                                          sec,
                                          deOffset +
                                          offsetof(IMAGE_DEBUG_DIRECTORY, PointerToRawData),
                                          sec, srRelocFilePos));


                    
            // Emit the directory entry.
            IfFailGo(pCeeFileGen->SetDirectoryEntry(
                                          ceeFile,
                                          sec,
                                          IMAGE_DIRECTORY_ENTRY_DEBUG,
                                          sizeof(debugDirIDD),
                                          deOffset));


            // Copy the debug directory into the section.
            memcpy(de, &debugDirIDD, sizeof(debugDirIDD));
            memcpy(de + sizeof(debugDirIDD), debugDirData, debugDirDataSize);

        }
    }
ErrExit:
    return hr;
}


//=============================EmitDebugInfoEnd==============================*/
// Phase 2 of emit debugging directory and symbol file.
//===========================================================================*/
HRESULT COMDynamicWrite::EmitDebugInfoEnd(Module *pModule,
                                       ICeeFileGen *pCeeFileGen,
                                       HCEEFILE ceeFile,
                                       HCEESECTION pILSection,
                                       WCHAR *filename,
                                       ISymUnmanagedWriter *pWriter)
{
    HRESULT hr = S_OK;
    
    CGrowableStream *pStream = NULL;

    // If we were emitting symbols for this dynamic module, go ahead
    // and fill out the debug directory and save off the symbols now.
    if (pWriter != NULL)
    {
        // Now go ahead and save off the symbol file and release the
        // writer.
        IfFailGo( pWriter->Close() );




        // How big of a stream to we have now?
        pStream = pModule->GetInMemorySymbolStream();
        _ASSERTE(pStream != NULL);

        STATSTG SizeData = {0};
        DWORD streamSize = 0;

        IfFailGo(pStream->Stat(&SizeData, STATFLAG_NONAME));

        streamSize = SizeData.cbSize.u.LowPart;

        if (SizeData.cbSize.u.HighPart > 0)
        {
            IfFailGo( E_OUTOFMEMORY );

        }

        SIZE_T fnLen = wcslen(filename);
        WCHAR *dot = wcsrchr(filename, L'.');
        SIZE_T dotOffset = dot - filename;

        if (dot && (fnLen - dotOffset >= 4))
        {
            WCHAR *fn = (WCHAR*)_alloca((fnLen + 2) * sizeof(WCHAR));
            wcscpy(fn, filename);


            fn[dotOffset + 1] = L'i';
            fn[dotOffset + 2] = L'l';
            fn[dotOffset + 3] = L'd';
            fn[dotOffset + 4] = L'b';
            fn[dotOffset + 5] = L'\0';

            HANDLE pdbFile = WszCreateFile(fn,
                                           GENERIC_WRITE,
                                           0,
                                           NULL,
                                           CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL);

            if (pdbFile != INVALID_HANDLE_VALUE)
            {
                DWORD dummy;
                BOOL succ = WriteFile(pdbFile,
                                      pStream->GetBuffer(),
                                      streamSize,
                                      &dummy, NULL);

                if (!succ)
                {
                    IfFailGo( HRESULT_FROM_WIN32(GetLastError()) );

                }

                CloseHandle(pdbFile);
            }
            else
            {
                IfFailGo( HRESULT_FROM_WIN32(GetLastError()) );

            }
        }
        else
        {
            IfFailGo( E_INVALIDARG );

        }
    }

ErrExit:
    // No one else will ever need this writer again...
    GetReflectionModule(pModule)->SetISymUnmanagedWriter(NULL);
//    GetReflectionModule(pModule)->SetSymbolStream(NULL);

    return hr;
}




//=============================SetResourceCounts=====================================*/
// ecall for setting the number of embedded resources to be stored in this module
//==============================================================================*/
FCIMPL2(void, COMDynamicWrite::SetResourceCounts, Object* refThisUNSAFE, UINT32 iCount)
{
    OBJECTREF refThis = (OBJECTREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(refThis);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    REFLECTMODULEBASEREF pReflect;
    RefClassWriter  *pRCW;   

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);
    
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//=============================AddResource=====================================*/
// ecall for adding embedded resource to this module
//==============================================================================*/
FCIMPL6(void, COMDynamicWrite::AddResource, Object* refThisUNSAFE, StringObject* strNameUNSAFE, U1Array* byteResUNSAFE, UINT32 iByteCount, UINT32 uFileTk, UINT32 iAttribute)
{
    struct _gc
    {
        OBJECTREF refThis;
        STRINGREF strName;
        U1ARRAYREF byteRes;
    } gc;
    gc.refThis = (OBJECTREF) refThisUNSAFE;
    gc.strName = (STRINGREF) strNameUNSAFE;
    gc.byteRes = (U1ARRAYREF) byteResUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (gc.refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    REFLECTMODULEBASEREF pReflect;
    RefClassWriter  *pRCW;   
    ICeeGen         *pICG;
    HCEEFILE        ceeFile=NULL;
    ICeeFileGen     *pCeeFileGen;
    HRESULT         hr=S_OK;
    HCEESECTION     hSection;
    ULONG           ulOffset;
    BYTE            *pbBuffer;
    IMetaDataAssemblyEmit *pAssemEmitter = NULL;
    Assembly        *pAssembly;
    mdManifestResource mr;
    mdFile          tkFile;
    IMetaDataEmit   *pOnDiskEmit;
    IMetaDataAssemblyEmit *pOnDiskAssemblyEmit = NULL;

    //Get the information for the Module and get the ICeeGen from it. 
    pReflect = (REFLECTMODULEBASEREF) gc.refThis;
    
    Module* pModule = (Module*) pReflect->GetData();
    _ASSERTE(pModule);

    pRCW = GetReflectionModule(pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    pICG = pRCW->GetCeeGen(); //This method is actually misnamed. It returns an ICeeGen.
    _ASSERTE(pICG);

    pAssembly = pModule->GetAssembly();
    _ASSERTE( pAssembly && pAssembly->IsDynamic() );

    IfFailGo( pRCW->EnsureCeeFileGenCreated() );

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    pOnDiskEmit = pRCW->GetOnDiskEmitter();

    // First, put it into .rdata section. The only reason that we choose .rdata section at
    // this moment is because this is the first section on the PE file. We don't need to deal with
    // reloc. Actually, I don't know how to deal with the reloc with CeeFileGen given that the reloc
    // position is not in the same file!

    // Get the .rdata section
    IfFailGo( pCeeFileGen->GetRdataSection(ceeFile, &hSection) );

    // the current section data length is the RVA
    IfFailGo( pCeeFileGen->GetSectionDataLen(hSection, &ulOffset) );

    // Allocate a block of space fromt he .rdata section
    IfFailGo( pCeeFileGen->GetSectionBlock(
        hSection,           // from .rdata section
        iByteCount + sizeof(DWORD),   // number of bytes that we need
        1,                  // alignment
        (void**) &pbBuffer) ); 

    // now copy over the resource
    memcpy( pbBuffer, &iByteCount, sizeof(DWORD) );
    memcpy( pbBuffer + sizeof(DWORD), gc.byteRes->GetDataPtr(), iByteCount );

    // track the total resource size so far.
    pRCW->m_ulResourceSize += sizeof(DWORD) + iByteCount;
    tkFile = RidFromToken(uFileTk) ? uFileTk : mdFileNil;
    if (tkFile != mdFileNil)
    {
        IfFailGo( pOnDiskEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pOnDiskAssemblyEmit) );
        
        // The resource is stored in a file other than the manifest file
        IfFailGo(pOnDiskAssemblyEmit->DefineManifestResource(
            gc.strName->GetBuffer(),
            mdFileNil,              // implementation -- should be file token of this module in the manifest
            ulOffset,               // offset to this file -- need to be adjusted upon save
            iAttribute,             // resource flag
            &mr));                  // manifest resource token
    }

    // Add an entry into the ManifestResource table for this resource
    // The RVA is ulOffset
    pAssemEmitter = pAssembly->GetOnDiskMDAssemblyEmitter();
    IfFailGo(pAssemEmitter->DefineManifestResource(
        gc.strName->GetBuffer(),
        tkFile,                 // implementation -- should be file token of this module in the manifest
        ulOffset,               // offset to this file -- need to be adjusted upon save
        iAttribute,             // resource flag
        &mr));                  // manifest resource token

    pRCW->m_tkFile = tkFile;

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();
    if (pOnDiskAssemblyEmit)
        pOnDiskAssemblyEmit->Release();
    if (FAILED(hr)) 
    {
        COMPlusThrowHR(hr);
    }

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//============================AddDeclarativeSecurity============================*/
// Add a declarative security serialized blob and a security action code to a
// given parent (class or method).
//==============================================================================*/
FCIMPL4(void, COMDynamicWrite::CWAddDeclarativeSecurity, ReflectModuleBaseObject* moduleUNSAFE, UINT32 tk, DWORD action, U1Array* blobUNSAFE)
{
    REFLECTMODULEBASEREF module = (REFLECTMODULEBASEREF) moduleUNSAFE;
    U1ARRAYREF blob = (U1ARRAYREF) blobUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_2(module, blob);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HRESULT                 hr;
    RefClassWriter*         pRCW;   
    REFLECTMODULEBASEREF    pReflect;
    Module*                 pModule;
    mdPermission            tkPermission;
    void const*             pData;
    DWORD                   cbData;

    pReflect = (REFLECTMODULEBASEREF) module;
    pModule = (Module*) pReflect->GetData();
    pRCW = GetReflectionModule(pModule)->GetClassWriter();

    if (blob != NULL) {
        pData = blob->GetDataPtr();
        cbData = blob->GetNumComponents();
    } else {
        pData = NULL;
        cbData = 0;
    }

    IfFailGo( pRCW->GetEmitHelper()->AddDeclarativeSecurityHelper(tk,
                                                             action,
                                                             pData,
                                                             cbData,
                                                             &tkPermission) );
ErrExit:
    if (FAILED(hr)) 
    {
        _ASSERTE(!"AddDeclarativeSecurity failed");
        COMPlusThrowHR(hr);
    }
    else if (hr == META_S_DUPLICATE)
    {
        COMPlusThrow(kInvalidOperationException, IDS_EE_DUPLICATE_DECLSEC);
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


CSymMapToken::CSymMapToken(ISymUnmanagedWriter *pWriter, IMapToken *pMapToken)
{
    m_cRef = 1;
    m_pWriter = pWriter;
    m_pMapToken = pMapToken;
    if (m_pWriter)
        m_pWriter->AddRef();
    if (m_pMapToken)
        m_pMapToken->AddRef();
} // CSymMapToken::CSymMapToken()



//*********************************************************************
//
// CSymMapToken's destructor
//
//*********************************************************************
CSymMapToken::~CSymMapToken()
{
    if (m_pWriter)
        m_pWriter->Release();
    if (m_pMapToken)
        m_pMapToken->Release();
}   // CSymMapToken::~CMapToken()


ULONG CSymMapToken::AddRef()
{
    return InterlockedIncrement(&m_cRef);
} // CSymMapToken::AddRef()



ULONG CSymMapToken::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (!cRef)
        delete this;
    return (cRef);
} // CSymMapToken::Release()


HRESULT CSymMapToken::QueryInterface(REFIID riid, void **ppUnk)
{
    *ppUnk = 0;

    if (riid == IID_IMapToken)
        *ppUnk = (IUnknown *) (IMapToken *) this;
    else
        return (E_NOINTERFACE);
    AddRef();
    return (S_OK);
}   // CSymMapToken::QueryInterface



//*********************************************************************
//
// catching the token mapping
//
//*********************************************************************
HRESULT CSymMapToken::Map(
    mdToken     tkFrom, 
    mdToken     tkTo)
{
    HRESULT         hr = NOERROR;
    if (m_pWriter)
        IfFailGo( m_pWriter->RemapToken(tkFrom, tkTo) );
    if (m_pMapToken)
        IfFailGo( m_pMapToken->Map(tkFrom, tkTo) );
ErrExit:
    return hr;
}

