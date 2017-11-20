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

#include "binder.h"

#include "field.h"
#include "excep.h"

#include "message.h"
#include "comnumber.h"
#include "eeconfig.h"

void Binder::Init(Module *pModule,
                  const ClassDescription *pClassDescriptions,
                  DWORD cClassDescriptions,
                  const MethodDescription *pMethodDescriptions,
                  DWORD cMethodDescriptions,
                  const FieldDescription *pFieldDescriptions,
                  DWORD cFieldDescriptions,
                  const TypeDescription *pTypeDescriptions,
                  DWORD cTypeDescriptions)
{
    m_pModule = pModule;
    pModule->m_pBinder = this;

    m_classDescriptions = pClassDescriptions;
    m_methodDescriptions = pMethodDescriptions;
    m_fieldDescriptions = pFieldDescriptions;
    m_typeDescriptions = pTypeDescriptions;

    //
    // Initialize Class RID array
    //

    _ASSERTE(cClassDescriptions < USHRT_MAX);
    m_cClassRIDs = (USHORT) cClassDescriptions;
    m_pClassRIDs = new USHORT [m_cClassRIDs];
    ZeroMemory(m_pClassRIDs, m_cClassRIDs * sizeof(*m_pClassRIDs));

    //
    // Initialize Method RID array
    //

    _ASSERTE(cMethodDescriptions < USHRT_MAX);
    m_cMethodRIDs = (USHORT) cMethodDescriptions;
    m_pMethodRIDs = new USHORT [m_cMethodRIDs];
    ZeroMemory(m_pMethodRIDs, m_cMethodRIDs * sizeof(*m_pMethodRIDs));

    
    //
    // Initialize Field RID array
    //

    _ASSERTE(cFieldDescriptions < USHRT_MAX);
    m_cFieldRIDs = (USHORT) cFieldDescriptions;
    m_pFieldRIDs = new USHORT [m_cFieldRIDs];
    ZeroMemory(m_pFieldRIDs, m_cFieldRIDs * sizeof(*m_pFieldRIDs));

    //
    // Initialize TypeHandle array
    //

    _ASSERTE(cTypeDescriptions < USHRT_MAX);
    m_cTypeHandles = (USHORT) cTypeDescriptions;
    m_pTypeHandles = new TypeHandle [m_cTypeHandles];
    ZeroMemory(m_pTypeHandles, m_cTypeHandles * sizeof(*m_pTypeHandles));
}

void Binder::Destroy()
{
    if (m_pClassRIDs != NULL
        && !m_pModule->IsPreloadedObject(m_pClassRIDs))
        delete [] m_pClassRIDs;

    if (m_pMethodRIDs != NULL
        && !m_pModule->IsPreloadedObject(m_pMethodRIDs))
        delete [] m_pMethodRIDs;

    if (m_pFieldRIDs != NULL
        && !m_pModule->IsPreloadedObject(m_pFieldRIDs))
        delete [] m_pFieldRIDs;

    if (m_pTypeHandles != NULL)
        delete [] m_pTypeHandles;
}


mdTypeDef Binder::GetTypeDef(BinderClassID id) 
{
    _ASSERTE(id != CLASS__NIL);
    _ASSERTE(id <= m_cClassRIDs);
    if (m_pClassRIDs[id-1] == 0)
        LookupClass(id);
    return TokenFromRid(m_pClassRIDs[id-1], mdtTypeDef);
}

mdMethodDef Binder::GetMethodDef(BinderMethodID id)
{
    _ASSERTE(id != METHOD__NIL);
    _ASSERTE(id <= m_cMethodRIDs);
    if (m_pMethodRIDs[id-1] == 0)
        LookupMethod(id);
    return TokenFromRid(m_pMethodRIDs[id-1], mdtMethodDef);
}

mdFieldDef Binder::GetFieldDef(BinderFieldID id)
{
    _ASSERTE(id != FIELD__NIL);
    _ASSERTE(id <= m_cFieldRIDs);
    if (m_pFieldRIDs[id-1] == 0)
        LookupField(id);
    return TokenFromRid(m_pFieldRIDs[id-1], mdtFieldDef);
}

//
// Raw retrieve structures from ID.  Use these when 
// you don't care about class Restore or .cctors.
//

MethodTable *Binder::RawGetClass(BinderClassID id)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != CLASS__NIL);
    _ASSERTE(id <= m_cClassRIDs);
    TypeHandle th = m_pModule->LookupTypeDef(TokenFromRid(m_pClassRIDs[id-1], mdtTypeDef));
    _ASSERTE(!th.IsNull());
    _ASSERTE(th.IsUnsharedMT());
    return th.AsMethodTable();
}

MethodDesc *Binder::RawGetMethod(BinderMethodID id)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != METHOD__NIL);
    _ASSERTE(id <= m_cMethodRIDs);
    const MethodDescription *d = m_methodDescriptions + id - 1;
    MethodTable *pMT = RawGetClass(d->classID);
    _ASSERTE(pMT != NULL);
    MethodDesc *pMD = pMT->GetMethodDescForSlot(m_pMethodRIDs[id-1]-1);
    _ASSERTE(pMD != NULL);
    return pMD;
}

FieldDesc *Binder::RawGetField(BinderFieldID id)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != FIELD__NIL);
    _ASSERTE(id <= m_cFieldRIDs);
    const FieldDescription *f = m_fieldDescriptions + id - 1;
    MethodTable *pMT = RawGetClass(f->classID);
    _ASSERTE(pMT != NULL);

    // Can't do this because the class may not be restored yet.
    // _ASSERTE(m_pFieldRIDs[id-1]-1 < (pMT->GetClass()->GetNumStaticFields() 
    //                                  + pMT->GetClass()->GetNumIntroducedInstanceFields()));

    FieldDesc *pFD = pMT->GetClass()->GetFieldDescListRaw() + m_pFieldRIDs[id-1]-1;
    _ASSERTE(pFD != NULL);
    return pFD;
}

TypeHandle Binder::RawGetType(BinderTypeID id)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != TYPE__NIL);
    _ASSERTE(id <= m_cTypeHandles);
    TypeHandle th = m_pTypeHandles[id-1];
    _ASSERTE(!th.IsNull());
    return th;
}

//
// Inline function to check a class for init & restore
//

inline void Binder::CheckInit(MethodTable *pMT)
{
    if (!pMT->IsClassInited())
        InitClass(pMT);
}



//
// Fetch retrieve structures from ID - doesn't
// trigger class init
//

MethodTable *Binder::FetchClass(BinderClassID id)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(id != CLASS__NIL);
    _ASSERTE(id <= m_cClassRIDs);

    MethodTable *pMT;

    if (m_pClassRIDs[id-1] == 0)
        pMT = LookupClass(id);
    else
    {
        pMT = RawGetClass(id);
        pMT->CheckRestore();
    }

    return pMT;
}

MethodDesc *Binder::FetchMethod(BinderMethodID id)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(id != METHOD__NIL);
    _ASSERTE(id <= m_cMethodRIDs);

    MethodDesc *pMD;

    if (m_pMethodRIDs[id-1] == 0)
        pMD = LookupMethod(id);
    else
    {
        pMD = RawGetMethod(id);
        pMD->GetMethodTable()->CheckRestore();
    }

    // Initialize the sig here where it's safe.  (Otherwise it would typically happen
    // during a MethodDesc::Call.)
    m_methodDescriptions[id-1].sig->GetBinarySig();

    return pMD;
}

FieldDesc *Binder::FetchField(BinderFieldID id)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(id != FIELD__NIL);
    _ASSERTE(id <= m_cFieldRIDs);

    FieldDesc *pFD;

    if (m_pFieldRIDs[id-1] == 0)
        pFD = LookupField(id);
    else
    {
        pFD = RawGetField(id);
        pFD->GetMethodTableOfEnclosingClass()->CheckRestore();
    }

    return pFD;
}

TypeHandle Binder::FetchType(BinderTypeID id)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(id != TYPE__NIL);
    _ASSERTE(id <= m_cTypeHandles);

    TypeHandle th = m_pTypeHandles[id-1];
    if (th.IsNull())
        th = LookupType(id);

    return th;
}

//
// Normal retrieve structures from ID
//

MethodTable *Binder::GetClass(BinderClassID id)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pMT = FetchClass(id);

    CheckInit(pMT);

    return pMT;
}

MethodDesc *Binder::GetMethod(BinderMethodID id)
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc *pMD = FetchMethod(id);

    CheckInit(pMD->GetMethodTable());

    return pMD;
}

FieldDesc *Binder::GetField(BinderFieldID id)
{
    THROWSCOMPLUSEXCEPTION();

    FieldDesc *pFD = FetchField(id);

    CheckInit(pFD->GetMethodTableOfEnclosingClass());

    return pFD;
}

TypeHandle Binder::GetType(BinderTypeID id)
{
    THROWSCOMPLUSEXCEPTION();

    TypeHandle th = FetchType(id);

    return th;
}

//
// Offsets - these could conceivably be implemented
// more efficiently than accessing the Desc info.
// 

DWORD Binder::GetFieldOffset(BinderFieldID id)
{ 
    return GetField(id)->GetOffset(); 
}

BOOL Binder::IsClass(MethodTable *pMT, BinderClassID id)
{
    if (m_pClassRIDs[id-1] == 0)
        return LookupClass(id, FALSE) == pMT;
    else
        return RawGetClass(id) == pMT;
}

BOOL Binder::IsType(TypeHandle th, BinderTypeID id)
{
    if (RawGetType(id) == th)
        return TRUE;
    if (!m_pTypeHandles[id-1].IsNull())
        return FALSE;
    else
        return LookupType(id, FALSE) == th;
}

void Binder::InitClass(MethodTable *pMT)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);

    if (!pMT->CheckRunClassInit(&throwable))
        COMPlusThrow(throwable);

    GCPROTECT_END();
}

MethodTable *Binder::LookupClass(BinderClassID id, BOOL fLoad)
{
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != CLASS__NIL);
    _ASSERTE(id <= m_cClassRIDs);

    MethodTable *pMT;

    const ClassDescription *d = m_classDescriptions + id - 1;

    NameHandle nh(d->name);

    if (!fLoad)
    {
        nh.SetTokenNotToLoad(tdAllTypes);
        pMT = m_pModule->GetClassLoader()->FindTypeHandle(&nh).AsMethodTable();
        if (pMT == NULL)
            return NULL;
    }
    else
    {
        THROWSCOMPLUSEXCEPTION();

        BEGIN_ENSURE_COOPERATIVE_GC();
        OBJECTREF pThrowable = NULL;
        GCPROTECT_BEGIN(pThrowable);

        pMT = m_pModule->GetClassLoader()->FindTypeHandle(&nh, &pThrowable).AsMethodTable();
        if (pMT == NULL)
        {
            _ASSERTE(!"EE expects class to exist");
            COMPlusThrow(pThrowable);
        }
        GCPROTECT_END();
        END_ENSURE_COOPERATIVE_GC();
    }

    _ASSERTE(pMT->GetModule() == m_pModule);

    mdTypeDef td = pMT->GetClass()->GetCl();

    _ASSERTE(!IsNilToken(td));

    _ASSERTE(RidFromToken(td) <= USHRT_MAX);
    m_pClassRIDs[id-1] = (USHORT) RidFromToken(td);

    m_pModule->StoreTypeDef(td, pMT);

    return pMT;
}

MethodDesc *Binder::LookupMethod(BinderMethodID id)
{
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != METHOD__NIL);
    _ASSERTE(id <= m_cMethodRIDs);

    THROWSCOMPLUSEXCEPTION();

    const MethodDescription *d = m_methodDescriptions + id - 1;

    MethodTable *pMT = FetchClass(d->classID);

    MethodDesc *pMD = pMT->GetClass()->FindMethod(d->name, d->sig);

    _ASSERTE(pMD != NULL || !"EE expects method to exist");

    _ASSERTE(pMD->GetSlot()+1 <= USHRT_MAX);
    m_pMethodRIDs[id-1] = (USHORT) pMD->GetSlot()+1;

    // Go ahead and fill in the rid map since we're here anyway
    m_pModule->StoreMethodDef(pMD->GetMemberDef(), pMD);

    return pMD;
}

FieldDesc *Binder::LookupField(BinderFieldID id)
{
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != FIELD__NIL);
    _ASSERTE(id <= m_cFieldRIDs);

    THROWSCOMPLUSEXCEPTION();

    const FieldDescription *d = m_fieldDescriptions + id - 1;

    MethodTable *pMT = FetchClass(d->classID);

    FieldDesc *pFD;

    pFD = pMT->GetClass()->FindField(d->name, d->sig);

    _ASSERTE(pFD != NULL || !"EE expects field to exist");
    _ASSERTE(pFD - pMT->GetClass()->GetFieldDescListRaw() >= 0);
    _ASSERTE(pFD - pMT->GetClass()->GetFieldDescListRaw() < (pMT->GetClass()->GetNumStaticFields() 
                                                             + pMT->GetClass()->GetNumIntroducedInstanceFields()));
    _ASSERTE(pFD - pMT->GetClass()->GetFieldDescListRaw() + 1 < USHRT_MAX);

    USHORT index = (USHORT)(pFD - pMT->GetClass()->GetFieldDescListRaw());

    m_pFieldRIDs[id-1] = index+1;

    // Go ahead and fill in the rid map since we're here anyway
    m_pModule->StoreFieldDef(pFD->GetMemberDef(), pFD);

    return pFD;
}

TypeHandle Binder::LookupType(BinderTypeID id, BOOL fLoad)
{
    _ASSERTE(m_pModule != NULL);
    _ASSERTE(id != TYPE__NIL);
    _ASSERTE(id <= m_cTypeHandles);

    THROWSCOMPLUSEXCEPTION();

    TypeHandle th;

    const TypeDescription *d = m_typeDescriptions + id - 1;

    OBJECTREF pThrowable = NULL;
    GCPROTECT_BEGIN(pThrowable);

    NameHandle nh(d->type, TypeHandle(GetClass(d->classID)), d->rank);
    if (!fLoad)
        nh.SetTokenNotToLoad(tdAllTypes);
    th = m_pModule->GetClassLoader()->FindTypeHandle(&nh, &pThrowable);

    GCPROTECT_END();

    if (th.IsNull())
    {
        if (fLoad)
            COMPlusThrow(pThrowable);
        return TypeHandle();
    }

    m_pTypeHandles[id-1] = th;

    return th;
}

BOOL Binder::IsException(MethodTable *pMT, RuntimeExceptionKind kind)
{
    return IsClass(pMT, (BinderClassID) (kind + CLASS__MSCORLIB_COUNT));
}

MethodTable *Binder::GetException(RuntimeExceptionKind kind)
{

    return GetClass((BinderClassID) (kind + CLASS__MSCORLIB_COUNT));
}

MethodTable *Binder::FetchException(RuntimeExceptionKind kind)
{
    return FetchClass((BinderClassID) (kind + CLASS__MSCORLIB_COUNT));
}


BOOL Binder::IsElementType(MethodTable *pMT, CorElementType type)
{
    BinderClassID id = (BinderClassID) (type + CLASS__MSCORLIB_COUNT + kLastException);

    _ASSERTE(GetClassName(id) != NULL);

    return IsClass(pMT, id);
}

MethodTable *Binder::GetElementType(CorElementType type)
{
    BinderClassID id = (BinderClassID) (type + CLASS__MSCORLIB_COUNT + kLastException);

    _ASSERTE(GetClassName(id) != NULL);

    return GetClass(id);
}

MethodTable *Binder::FetchElementType(CorElementType type)
{
    BinderClassID id = (BinderClassID) (type + CLASS__MSCORLIB_COUNT + kLastException);

    _ASSERTE(GetClassName(id) != NULL);

    return FetchClass(id);
}


///////////////////////////////////////////////////////////////////////////////////
// Mscorlib:
///////////////////////////////////////////////////////////////////////////////////

const Binder::ClassDescription Binder::MscorlibClassDescriptions[] =
{
#define DEFINE_CLASS(i,n,s)        { g_ ## n ## NS "." # s },
#include "mscorlib.h"

    // Include all exception types here
#define EXCEPTION_BEGIN_DEFINE(ns, reKind, hr) { ns "." # reKind },
#define EXCEPTION_ADD_HR(hr)
#define EXCEPTION_END_DEFINE()
#include "rexcep.h"
#undef EXCEPTION_BEGIN_DEFINE
#undef EXCEPTION_ADD_HR
#undef EXCEPTION_END_DEFINE

    // Now include all signature types

#define TYPEINFO(e,c,s,g,ia,ip,if,im)   { c },
#include "cortypeinfo.h"
#undef TYPEINFO
};

const Binder::MethodDescription Binder::MscorlibMethodDescriptions[] = 
{
#define DEFINE_METHOD(c,i,s,g)          { CLASS__ ## c , # s, & gsig_ ## g },
#include "mscorlib.h"
};


const Binder::FieldDescription Binder::MscorlibFieldDescriptions[] =
{
#define DEFINE_FIELD(c,i,s,g)           { CLASS__ ## c , # s, & gsig_Fld_ ## g },
#ifdef _DEBUG
#define DEFINE_FIELD_U(c,i,s,g,uc,uf)   { CLASS__ ## c , # s, & gsig_Fld_ ## g },
#endif
#include "mscorlib.h"
};

const Binder::TypeDescription Binder::MscorlibTypeDescriptions[] = 
{
    { CLASS__BYTE,          ELEMENT_TYPE_SZARRAY,       1 },
    { CLASS__OBJECT,        ELEMENT_TYPE_SZARRAY,       1 },
    { CLASS__VARIANT,       ELEMENT_TYPE_SZARRAY,       1 },
    { CLASS__VOID,          ELEMENT_TYPE_PTR,           0 },
};

#ifdef _DEBUG

const Binder::FieldOffsetCheck Binder::MscorlibFieldOffsets[] =
{
#define DEFINE_FIELD_U(c,i,s,g,uc,uf)  { FIELD__ ## c ## __ ## i, offsetof(uc, uf) },   
#include "mscorlib.h"
    { (BinderFieldID) 0 }
};

const Binder::ClassSizeCheck Binder::MscorlibClassSizes[] =
{
#define DEFINE_CLASS_U(i,n,s,uc)  { CLASS__ ## i, sizeof(uc) },
#include "mscorlib.h"
    { (BinderClassID) 0 }
};

void Binder::CheckMscorlib()
{
    const FieldOffsetCheck     *pOffsets = MscorlibFieldOffsets;

    while (pOffsets->fieldID != FIELD__NIL)
    {
        FieldDesc *pFD = g_Mscorlib.FetchField(pOffsets->fieldID);
        DWORD offset = pFD->GetOffset();

        if (!pFD->GetMethodTableOfEnclosingClass()->IsValueClass())
            offset += sizeof(ObjHeader);

        _ASSERTE(offset == pOffsets->expectedOffset
                 && "Managed class field offset does not match unmanaged class field offset");
        pOffsets++;
    }

    const ClassSizeCheck     *pSizes = MscorlibClassSizes;

    while (pSizes->classID != CLASS__NIL)
    {
        MethodTable *pMT = g_Mscorlib.FetchClass(pSizes->classID);
        DWORD size = pMT->GetClass()->GetNumInstanceFieldBytes();
        DWORD expected = pSizes->expectedSize - sizeof(void*);

        _ASSERTE(size == expected
                 && "Managed object size does not match unmanaged object size");
        pSizes++;
    }

    // check the consistency of BCL and VM
    // note: it is not enabled by default because of it is time consuming and 
    // changes the bootstrap sequence of the EE
    if (!g_pConfig->GetConfigDWORD(L"ConsistencyCheck", 0))
        return;

    //
    // VM referencing BCL (mscorlib.h)
    //
    for (BinderClassID cID = (BinderClassID) 1; cID <= g_Mscorlib.m_cClassRIDs; cID = (BinderClassID) (cID + 1)) {
        if (g_Mscorlib.GetClassName(cID) != NULL) // Allow for CorSigElement entries with no classes
            g_Mscorlib.FetchClass(cID);
    }

    for (BinderMethodID mID = (BinderMethodID) 1; mID <= g_Mscorlib.m_cMethodRIDs; mID = (BinderMethodID) (mID + 1))
        g_Mscorlib.FetchMethod(mID);

    for (BinderFieldID fID = (BinderFieldID) 1; fID <= g_Mscorlib.m_cFieldRIDs; fID = (BinderFieldID) (fID + 1))
        g_Mscorlib.FetchField(fID);

    //
    // BCL referencing VM (ecall.cpp)
    //
    HRESULT hr = S_OK;
    Module *pModule = g_Mscorlib.m_pModule;
    IMDInternalImport *pInternalImport = pModule->GetMDImport();

    HENUMInternal hEnum;

    // for all methods...
    IfFailGo(pInternalImport->EnumAllInit(mdtMethodDef, &hEnum));

    for (;;) {
        mdTypeDef td;
        mdTypeDef tdClass;
        DWORD dwImplFlags;

        if (!pInternalImport->EnumNext(&hEnum, &td))
            break;

        pInternalImport->GetMethodImplProps(td, NULL, &dwImplFlags);

        // ... that are internal calls ...
        if (!IsMiInternalCall(dwImplFlags))
            continue;

        IfFailGo(pInternalImport->GetParentToken(td, &tdClass));

        NameHandle className(pModule, tdClass);
        TypeHandle type;
        
        type = pModule->GetClassLoader()->LoadTypeHandle(&className, RETURN_ON_ERROR, FALSE);          
        if (type.IsNull()) {
            LPCUTF8 pszName = pInternalImport->GetNameOfMethodDef(tdClass);
            OutputDebugStringA(pszName);
            OutputDebugStringA("\n");
            _ASSERTE(false);
        }      

        MethodDesc *pMD = type.AsMethodTable()->GetClass()->FindMethod(td);;
        _ASSERTE(pMD);

        // ... check that the method is in the fcall table.
        if (GetIDForMethod(pMD) == 0xffff) {
            LPCUTF8 pszName = pInternalImport->GetNameOfMethodDef(td);
            OutputDebugStringA(pszName);
            OutputDebugStringA("\n");
            _ASSERTE(false);
        }
    }

    pInternalImport->EnumClose(&hEnum);

ErrExit:
    _ASSERTE(SUCCEEDED(hr));
}

#endif 

Binder g_Mscorlib;

void Binder::StartupMscorlib(Module *pModule)
{
    _ASSERTE(CLASS__MSCORLIB_COUNT < USHRT_MAX);
    _ASSERTE(METHOD__MSCORLIB_COUNT < USHRT_MAX);
    _ASSERTE(FIELD__MSCORLIB_COUNT < USHRT_MAX);
    _ASSERTE(TYPE__MSCORLIB_COUNT < USHRT_MAX);

    {
        g_Mscorlib.Init(pModule,
                        MscorlibClassDescriptions,
                        sizeof(MscorlibClassDescriptions)/sizeof(*MscorlibClassDescriptions),
                        MscorlibMethodDescriptions,
                        sizeof(MscorlibMethodDescriptions)/sizeof(*MscorlibMethodDescriptions),
                        MscorlibFieldDescriptions,
                        sizeof(MscorlibFieldDescriptions)/sizeof(*MscorlibFieldDescriptions),
                        MscorlibTypeDescriptions,
                        sizeof(MscorlibTypeDescriptions)/sizeof(*MscorlibTypeDescriptions));
    }
}


