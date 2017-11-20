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
#include "ml.h"
#include "mlgen.h"
#include "custommarshalerinfo.h"

#ifndef _MLINFO_H_
#define _MLINFO_H_

struct NativeTypeParamInfo;


#define VARIABLESIZE ((BYTE)(-1))

enum 
{
    MarshalerFlag_ReturnsComByref              = 0x01,
    MarshalerFlag_ReturnsNativeByref           = 0x02,
    MarshalerFlag_InOnly                       = 0x04,
};


union MLOverrideArgs
{
    UINT8           m_arrayMarshalerID;
    UINT16          m_blittablenativesize;
    MethodTable    *m_pMT;
	class MarshalInfo *m_pMLInfo;
    struct _na
    {
        VARTYPE         m_vt;
        MethodTable    *m_pMT;
        UINT16          m_optionalbaseoffset; //for fast marshaling, offset of dataptr if known and less than 64k (0 otherwise)
    } na;

    struct _mm
    {
        MethodTable *m_pMT;
        MethodDesc  *m_pCopyCtor;
        MethodDesc  *m_pDtor;
    } mm;
};



class EEMarshalingData
{
public:
    EEMarshalingData(BaseDomain *pDomain, LoaderHeap *pHeap, Crst *pCrst);
    ~EEMarshalingData();

    // EEMarshalingData's are always allocated on the loader heap so we need to redefine
    // the new and delete operators to ensure this.
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *pMem);

    // This method returns the custom marshaling helper associated with the name cookie pair. If the 
    // CM info has not been created yet for this pair then it will be created and returned.
    CustomMarshalerHelper *GetCustomMarshalerHelper(Assembly *pAssembly, TypeHandle hndManagedType, LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes);

    // This method returns the custom marshaling info associated with shared CM helper.
    CustomMarshalerInfo *GetCustomMarshalerInfo(SharedCustomMarshalerHelper *pSharedCMHelper);


private:
    EECMHelperHashTable                 m_CMHelperHashtable;
    EEPtrHashTable                      m_SharedCMHelperToCMInfoMap;
    LoaderHeap *                        m_pHeap;
    BaseDomain *                        m_pDomain;
    CMINFOLIST                          m_pCMInfoList;
};


class MarshalInfo
{
  public:

    enum MarshalType
    {

#define DEFINE_MARSHALER_TYPE(mtype, mclass) mtype,
#include "mtypes.h"

        MARSHAL_TYPE_UNKNOWN
    };

    enum MarshalScenario
    {
        MARSHAL_SCENARIO_NDIRECT,
    };

    void *operator new(size_t size, void *pInPlace)
    {
        return pInPlace;
    }

    MarshalInfo() {}


    MarshalInfo(Module* pModule,
                SigPointer sig,
                mdToken token,
                MarshalScenario ms,
                BYTE nlType,
                BYTE nlFlags,
                BOOL isParam,
                UINT paramidx    // parameter # for use in error messages (ignored if not parameter)

#ifdef CUSTOMER_CHECKED_BUILD
                ,
                MethodDesc* pMD = NULL
#endif
#ifdef _DEBUG
                ,
                LPCUTF8 pDebugName = NULL,
                LPCUTF8 pDebugClassName = NULL,
                LPCUTF8 pDebugNameSpace = NULL,
                UINT    argidx = 0  // 0 for return value, -1 for field
#endif

                );

    // These methods retrieve the information for different element types.
    HRESULT HandleArrayElemType(char *achDbgContext, 
                                NativeTypeParamInfo *pParamInfo, 
                                UINT16 optbaseoffset, 
                                TypeHandle elemTypeHnd, 
                                int iRank, 
                                BOOL fNoLowerBounds, 
                                BOOL isParam, 
                                BOOL isSysArray, 
                                Assembly *pAssembly);

    void GenerateArgumentML(MLStubLinker*   psl,
                            MLStubLinker*   pslPost,
                            BOOL comToNative);
    void GenerateReturnML(MLStubLinker* psl,
                          MLStubLinker* pslPost,
                          BOOL comToNative,
                          BOOL retval);
    void GenerateSetterML(MLStubLinker*psl,
                          MLStubLinker* pslPost);
    void GenerateGetterML(MLStubLinker* psl,
                          MLStubLinker* pslPost,
                          BOOL retval);
    UINT16 EmitCreateOpcode(MLStubLinker *psl);

    UINT16 GetComArgSize() { return m_comArgSize; }
    UINT16 GetNativeArgSize() { return m_nativeArgSize; }

    UINT16 GetNativeSize() { return nativeSize(m_type); }
    MarshalType GetMarshalType() { return m_type; }

    BOOL   IsFpu()
    {
        return m_type == MARSHAL_TYPE_FLOAT || m_type == MARSHAL_TYPE_DOUBLE;
    }


private:

    struct ItfMarshalInfo {
        MethodTable *pClassMT;
        MethodTable *pItfMT;
    };
    	
    void GetItfMarshalInfo(ItfMarshalInfo *pInfo);

    MarshalType     m_type;
    BOOL            m_byref;
    BOOL            m_in;
    BOOL            m_out;
    EEClass         *m_pClass;  // Used if this is a true class
    TypeHandle      m_hndArrayElemType;
    VARTYPE         m_arrayElementType;
    int             m_iArrayRank;
    BOOL            m_nolowerbounds;  // if managed type is SZARRAY, don't allow lower bounds

    // for NT_ARRAY only
    UINT16          m_countParamIdx;  // index of "sizeis" parameter
    UINT32          m_multiplier;     // multipler for "sizeis"
    UINT32          m_additive;       // additive for 'sizeis"

    UINT16          m_nativeArgSize;
    UINT16          m_comArgSize;

    MarshalScenario m_ms;
    BYTE            m_nlType;
    BYTE            m_nlFlags;
    BOOL            m_fAnsi;

    // Information used by NT_CUSTOMMARSHALER.
    CustomMarshalerHelper *m_pCMHelper;
    VARTYPE         m_CMVt;

    MLOverrideArgs  m_args;

    static const BYTE m_localSizes[];

    // static const BYTE m_comSizes[];
    // static const BYTE m_nativeSizes[];
    UINT16          comSize(MarshalType mtype);
    UINT16          nativeSize(MarshalType mtype);

    UINT            m_paramidx;
    UINT            m_resID;     // resource ID for error message (if any)

#if defined(_DEBUG)
     LPCUTF8        m_strDebugMethName;
     LPCUTF8        m_strDebugClassName;
     LPCUTF8        m_strDebugNameSpace;
     UINT           m_iArg;  // 0 for return value, -1 for field
#endif

    static const BYTE m_flags[];

    static const BYTE m_unmarshalN2CNeeded[];
    static const BYTE m_unmarshalC2NNeeded[];

    VOID EmitOrThrowInteropParamException(MLStubLinker *psl, UINT resID, UINT paramIdx);

#ifdef _DEBUG
    VOID DumpMarshalInfo(Module* pModule, SigPointer sig, mdToken token, MarshalScenario ms, BYTE nlType, BYTE nlFlags);
#endif

#ifdef CUSTOMER_CHECKED_BUILD
    VOID OutputCustomerCheckedBuildMarshalInfo(MethodDesc* pMD, SigPointer sig, Module* pModule);
    VOID MarshalTypeToString(CQuickArray<WCHAR> *pStrMarshalType);
    VOID VarTypeToString(VARTYPE vt, CQuickArray<WCHAR> *pStrVarType);
#endif

};

//===================================================================================
// Throws an exception indicating a param has invalid element type / native type
// information.
//===================================================================================
VOID ThrowInteropParamException(UINT resID, UINT paramIdx);

//===================================================================================
// Post-patches ML stubs for the sizeis feature.
//===================================================================================
VOID PatchMLStubForSizeIs(BYTE *pMLCode, UINT32 numArgs, MarshalInfo *pMLInfo);

//===================================================================================
// Support routines for storing ML stubs in prejit files
//===================================================================================

VOID CollateParamTokens(IMDInternalImport *pInternalImport, mdMethodDef md, ULONG numargs, mdParamDef *aParams);

#endif // _MLINFO_H_

