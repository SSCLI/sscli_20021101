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
/*  NSTRUCT.CPP:
 *
 */

#include "common.h"
#include "vars.hpp"
#include "class.h"
#include "ceeload.h"
#include "excep.h"
#include "nstruct.h"
#include "corjit.h"
#include "comstring.h"
#include "field.h"
#include "frames.h"
#include "gcscan.h"
#include "ndirect.h"
#include "comdelegate.h"
#include "eeconfig.h"
#include "comdatetime.h"
#include "olevariant.h"

#include <cor.h>
#include <corpriv.h>
#include <corerror.h>


BOOL IsStructMarshalable(EEClass *pcls);  //From COMNDirect.cpp


//=======================================================================
// A database of NFT types.
//=======================================================================
struct NFTDataBaseEntry {
    UINT32            m_cbNativeSize;     // native size of field (0 if not constant)
};

static const NFTDataBaseEntry NFTDataBase[] =
{
#undef DEFINE_NFT
#define DEFINE_NFT(name, nativesize) { nativesize },
#include "nsenums.h"
};




//=======================================================================
// This is invoked from the class loader while building a EEClass.
// This function should check if explicit layout metadata exists.
//
// Returns:
//  S_OK    - yes, there's layout metadata
//  S_FALSE - no, there's no layout.
//  fail    - couldn't tell because of metadata error
//
// If S_OK,
//   *pNLType            gets set to nltAnsi or nltUnicode
//   *pPackingSize       declared packing size
//   *pfExplicitoffsets  offsets explicit in metadata or computed?
//=======================================================================
HRESULT HasLayoutMetadata(IMDInternalImport *pInternalImport, mdTypeDef cl, EEClass *pParentClass, BYTE *pPackingSize, BYTE *pNLTType, BOOL *pfExplicitOffsets)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

//    if (pParentClass && pParentClass->HasLayout()) {
//       *pPackingSize      = pParentClass->GetLayoutInfo()->GetDeclaredPackingSize();
//        *pNLTType          = pParentClass->GetLayoutInfo()->GetNLType();
//        *pfExplicitOffsets = !(pParentClass->GetLayoutInfo()->IsAutoOffset());
//        return S_OK;
//    }


    HRESULT hr;
    ULONG clFlags;
#ifdef _DEBUG
    clFlags = 0xcccccccc;
#endif


    pInternalImport->GetTypeDefProps(
        cl,     
        &clFlags,
        NULL);

    if (IsTdAutoLayout(clFlags)) {


        {
            //
            //
            ULONG cbTotalSize = 0;
            if (SUCCEEDED(pInternalImport->GetClassTotalSize(cl, &cbTotalSize)) && cbTotalSize != 0)
            {
                if (pParentClass && pParentClass->IsValueTypeClass())
                {
                    HENUMInternal hEnumField;
                    HRESULT hr;
                    hr = pInternalImport->EnumInit(mdtFieldDef, cl, &hEnumField);
                    if (SUCCEEDED(hr))
                    {
                        ULONG numFields = pInternalImport->EnumGetCount(&hEnumField);
                        pInternalImport->EnumClose(&hEnumField);
                        if (numFields == 0)
                        {
                            *pfExplicitOffsets = FALSE;
                            *pNLTType = nltAnsi;
                            *pPackingSize = 1;
                            return S_OK;
                        }
                    }
                }
            }
        }

        return S_FALSE;
    } else if (IsTdSequentialLayout(clFlags)) {
        *pfExplicitOffsets = FALSE;
    } else if (IsTdExplicitLayout(clFlags)) {
        *pfExplicitOffsets = TRUE;
    } else {
        BAD_FORMAT_ASSERT(!"Wrapper classes must be SequentialLayout or ExplicitLayout");
        return COR_E_TYPELOAD;
    }

    // We now know this class has seq. or explicit layout. Ensure the parent does too.
    if (pParentClass && !(pParentClass->IsObjectClass() || pParentClass->IsValueTypeClass()) && !(pParentClass->HasLayout()))
    {
        BAD_FORMAT_ASSERT(!"Layout class must derive from Object or another layout class");
        return COR_E_TYPELOAD;
    }

    if (IsTdAnsiClass(clFlags)) {
        *pNLTType = nltAnsi;
    } else if (IsTdUnicodeClass(clFlags)) {
        *pNLTType = nltUnicode;
    } else if (IsTdAutoClass(clFlags)) {
        *pNLTType = (NDirectOnUnicodeSystem() ? nltUnicode : nltAnsi);
    } else {
        BAD_FORMAT_ASSERT(!"Bad stringformat value in wrapper class.");
        return COR_E_TYPELOAD;
    }

    DWORD dwPackSize;
    hr = pInternalImport->GetClassPackSize(cl, &dwPackSize);
    if (FAILED(hr) || dwPackSize == 0) {
        dwPackSize = DEFAULT_PACKING_SIZE;
    }
    *pPackingSize = (BYTE)dwPackSize;
    //printf("Packsize = %lu\n", dwPackSize);

    return S_OK;
}



#ifdef _DEBUG
#define REDUNDANCYWARNING(when) if (when) LOG((LF_SLOP, LL_INFO100, "%s.%s: Redundant nativetype metadata.\n", szClassName, szFieldName))
#else
#define REDUNDANCYWARNING(when)
#endif



HRESULT ParseNativeType(Module *pModule,
                        PCCOR_SIGNATURE     pCOMSignature,
                        BYTE nlType,      // nltype (from @dll.struct)
                        LayoutRawFieldInfo * const pfwalk,
                        PCCOR_SIGNATURE     pNativeType,
                        ULONG               cbNativeType,
                        IMDInternalImport  *pInternalImport,
                        mdTypeDef           cl,
                        OBJECTREF          *pThrowable
#ifdef _DEBUG
                        ,
                        LPCUTF8           szClassName,
                        LPCUTF8              szFieldName
#endif

)
{
    CANNOTTHROWCOMPLUSEXCEPTION();


#define INITFIELDMARSHALER(nfttype, fmtype, args) \
do {\
_ASSERTE(sizeof(fmtype) <= MAXFIELDMARSHALERSIZE);\
pfwalk->m_nft = (nfttype); \
new ( &(pfwalk->m_FieldMarshaler) ) fmtype args;\
} while(0)

    BOOL fAnsi = (nlType == nltAnsi);

    pfwalk->m_nft = NFT_NONE;


    BYTE ntype;
    BOOL  fDefault;
    if (cbNativeType == 0) {
        ntype = NATIVE_TYPE_MAX;
        fDefault = TRUE;
    } else {
        ntype = *( ((BYTE*&)pNativeType)++ );
        cbNativeType--;
        fDefault = FALSE;
    }


    FieldSig fsig(pCOMSignature, pModule);
    switch (fsig.GetFieldTypeNormalized()) {

        case ELEMENT_TYPE_CHAR:
            if (fDefault) {
                    if (fAnsi) {
                        INITFIELDMARSHALER(NFT_ANSICHAR, FieldMarshaler_Ansi, ());
                    } else {
                        INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
                    }
            } else if (ntype == NATIVE_TYPE_I1 || ntype == NATIVE_TYPE_U1) {

                    REDUNDANCYWARNING( fAnsi );

                    INITFIELDMARSHALER(NFT_ANSICHAR, FieldMarshaler_Ansi, ());
            } else if (ntype == NATIVE_TYPE_I2 || ntype == NATIVE_TYPE_U2) {
                    REDUNDANCYWARNING( !fAnsi );
                    INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
            } else {
                    INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_CHAR));
            }
            break;

        case ELEMENT_TYPE_BOOLEAN:
            if (fDefault) {
                    INITFIELDMARSHALER(NFT_WINBOOL, FieldMarshaler_WinBool, ());
            } else if (ntype == NATIVE_TYPE_BOOLEAN) {
                    REDUNDANCYWARNING(TRUE);
                    INITFIELDMARSHALER(NFT_WINBOOL, FieldMarshaler_WinBool, ());
            } else if (ntype == NATIVE_TYPE_U1 || ntype == NATIVE_TYPE_I1) {
                    INITFIELDMARSHALER(NFT_CBOOL, FieldMarshaler_CBool, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_BOOLEAN));
            }
            break;


        case ELEMENT_TYPE_I1:
            if (fDefault || ntype == NATIVE_TYPE_I1 || ntype == NATIVE_TYPE_U1) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY1, FieldMarshaler_Copy1, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I1));
            }
            break;

        case ELEMENT_TYPE_U1:
            if (fDefault || ntype == NATIVE_TYPE_U1 || ntype == NATIVE_TYPE_I1) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY1, FieldMarshaler_Copy1, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I1));
            }
            break;

        case ELEMENT_TYPE_I2:
            if (fDefault || ntype == NATIVE_TYPE_I2 || ntype == NATIVE_TYPE_U2) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I2));
            }
            break;

        case ELEMENT_TYPE_U2:
            if (fDefault || ntype == NATIVE_TYPE_U2 || ntype == NATIVE_TYPE_I2) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I2));
            }
            break;

        case ELEMENT_TYPE_I4:
            if (fDefault || ntype == NATIVE_TYPE_I4 || ntype == NATIVE_TYPE_U4) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I4));
            }
            break;
        case ELEMENT_TYPE_U4:
            if (fDefault || ntype == NATIVE_TYPE_U4 || ntype == NATIVE_TYPE_I4) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I4));
            }
            break;

        case ELEMENT_TYPE_I8:
            if (fDefault || ntype == NATIVE_TYPE_I8 || ntype == NATIVE_TYPE_U8) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I8));
            }
            break;

        case ELEMENT_TYPE_U8:
            if (fDefault || ntype == NATIVE_TYPE_U8 || ntype == NATIVE_TYPE_I8) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I8));
            }
            break;

        case ELEMENT_TYPE_I: //fallthru
        case ELEMENT_TYPE_U:
            if (fDefault || ntype == NATIVE_TYPE_INT || ntype == NATIVE_TYPE_UINT) {
                REDUNDANCYWARNING(!fDefault);
                if (sizeof(LPVOID)==4) {
                    INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
                } else {
                    INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
                }
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I));
            }
            break;

        case ELEMENT_TYPE_R4:
            if (fDefault || ntype == NATIVE_TYPE_R4) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_R4));
            }
            break;
        case ELEMENT_TYPE_R8:
            if (fDefault || ntype == NATIVE_TYPE_R8) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_R8));
            }
            break;

        case ELEMENT_TYPE_R:
            if (fDefault) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_R));
            }
            break;

        case ELEMENT_TYPE_PTR:
            if (fDefault) {
                REDUNDANCYWARNING(!fDefault);
                switch (sizeof(LPVOID)) {
                    case 4:
                            INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
                        break;
                    case 8:
                            INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
                        break;
                    default:
                        ;
                }
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_PTR));
            }
            break;

        case ELEMENT_TYPE_VALUETYPE: {
                EEClass *pNestedClass = fsig.GetTypeHandle(pThrowable).GetClass();
                if (!pNestedClass) {
                    return E_OUTOFMEMORY;
                } else {
                    if ((fDefault || ntype == NATIVE_TYPE_STRUCT) && fsig.IsClass(g_DateClassName)) {
                        REDUNDANCYWARNING(!fDefault);
                        INITFIELDMARSHALER(NFT_DATE, FieldMarshaler_Date, ());
                    } else if ((fDefault || ntype == NATIVE_TYPE_STRUCT) && fsig.IsClass(g_DecimalClassName)) {
                        REDUNDANCYWARNING(!fDefault);
                    } else if ((fDefault || ntype == NATIVE_TYPE_STRUCT) && pNestedClass->HasLayout() && IsStructMarshalable(pNestedClass)) {
                        REDUNDANCYWARNING(!fDefault);
                        INITFIELDMARSHALER(NFT_NESTEDVALUECLASS, FieldMarshaler_NestedValueClass, (pNestedClass->GetMethodTable()));
                    } else {
                        if (!(pNestedClass->HasLayout()) || !IsStructMarshalable(pNestedClass)) {
                            INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_NOLAYOUT));
                        } else {
                            INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (ntype == NATIVE_TYPE_LPSTRUCT ? IDS_EE_BADPINVOKEFIELD_NOLPSTRUCT : IDS_EE_BADPINVOKEFIELD_CLASS));
                        }
                    }
                }
            }
            break;

        case ELEMENT_TYPE_CLASS: {
                    // review is this correct for arrays?
                EEClass *pNestedClass = fsig.GetTypeHandle(pThrowable).GetClass();
                if (!pNestedClass) {
                    if (pThrowableAvailable(pThrowable))
                        *pThrowable = NULL;
                    INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD));
                }
                else
                {
                    if ((pNestedClass->IsObjectClass() || GetAppDomain()->IsSpecialObjectClass(pNestedClass->GetMethodTable())) && 
                        (fDefault || ntype == NATIVE_TYPE_IUNKNOWN)
                        ) {
                        INITFIELDMARSHALER(NFT_INTERFACE, FieldMarshaler_Interface, (NULL, NULL));
                    }
                    else
                    {
                        if (ntype == NATIVE_TYPE_CUSTOMMARSHALER)
                        {
                            INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_NOCUSTOM));
                        }
    
                        if (pNestedClass == g_pStringClass->GetClass()) {
                            if (fDefault) {
                                if (fAnsi) {
                                    INITFIELDMARSHALER(NFT_STRINGANSI, FieldMarshaler_StringAnsi, ());
                                } else {
                                    INITFIELDMARSHALER(NFT_STRINGUNI, FieldMarshaler_StringUni, ());
                                }
                            } else {
                                switch (ntype) {
                                    case NATIVE_TYPE_LPSTR:
                                        REDUNDANCYWARNING(fAnsi);
                                        INITFIELDMARSHALER(NFT_STRINGANSI, FieldMarshaler_StringAnsi, ());
                                        break;
                                    case NATIVE_TYPE_LPWSTR:
                                        REDUNDANCYWARNING(!fAnsi);
                                        INITFIELDMARSHALER(NFT_STRINGUNI, FieldMarshaler_StringUni, ());
                                        break;
                                    case NATIVE_TYPE_LPTSTR:
                                        if (NDirectOnUnicodeSystem()) {
                                            INITFIELDMARSHALER(NFT_STRINGUNI, FieldMarshaler_StringUni, ());
                                        } else {
                                            INITFIELDMARSHALER(NFT_STRINGANSI, FieldMarshaler_StringAnsi, ());
                                        }
                                        break;
                                    case NATIVE_TYPE_FIXEDSYSSTRING:
                                        {
                                            ULONG nchars;
                                            ULONG udatasize = CorSigUncompressedDataSize(pNativeType);

                                            if (cbNativeType < udatasize) {
                                                return E_FAIL;
                                            }
                                            nchars = CorSigUncompressData(pNativeType);
                                            cbNativeType -= udatasize;
        
                                            if (nchars == 0) {
                                                return E_FAIL;
                                            }
        
            
                                            if (fAnsi) {
                                                INITFIELDMARSHALER(NFT_FIXEDSTRINGANSI, FieldMarshaler_FixedStringAnsi, (nchars));
                                            } else {
                                                INITFIELDMARSHALER(NFT_FIXEDSTRINGUNI, FieldMarshaler_FixedStringUni, (nchars));
                                            }
                                        }
                                        break;
                                    default:
                                        INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_STRING));
                                        break;
                                }
                            }
                        } else if (COMDelegate::IsDelegate(pNestedClass)) {
                            if ( (fDefault || ntype == NATIVE_TYPE_FUNC ) ) {
                                REDUNDANCYWARNING(!fDefault);
                                INITFIELDMARSHALER(NFT_DELEGATE, FieldMarshaler_Delegate, ());
                            } else {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_DELEGATE));
                            }

                        } else if (pNestedClass->HasLayout() && IsStructMarshalable(pNestedClass)) {
                            if ( (fDefault || ntype == NATIVE_TYPE_STRUCT ) ) {
                                INITFIELDMARSHALER(NFT_NESTEDLAYOUTCLASS, FieldMarshaler_NestedLayoutClass, (pNestedClass->GetMethodTable()));
                            } else {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (ntype == NATIVE_TYPE_LPSTRUCT ? IDS_EE_BADPINVOKEFIELD_NOLPSTRUCT : IDS_EE_BADPINVOKEFIELD_CLASS));
                            }

                        } else {
                            if (fsig.IsClass("System.Text.StringBuilder"))
                            {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_NOSTRINGBUILDERFIELD));
                            }
                            else
                            {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_NOLAYOUT));
                            }
                        }
                    }
                }
            }
            break;

        case ELEMENT_TYPE_SZARRAY: {
            SigPointer elemType;
            ULONG      elemCount;
            fsig.GetProps().GetSDArrayElementProps(&elemType, &elemCount);
            CorElementType etyp = elemType.PeekElemType();

                if (ntype == NATIVE_TYPE_FIXEDARRAY) {

                    ULONG nelems;
                    ULONG udatasize = CorSigUncompressedDataSize(pNativeType);

                    if (cbNativeType < udatasize) {
                        return E_FAIL;
                    }
                    nelems = CorSigUncompressData(pNativeType);
                    cbNativeType -= udatasize;

                    if (nelems == 0) {
                        return E_FAIL;
                    }

                    switch (etyp) {
                        case ELEMENT_TYPE_I1:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I1, nelems, 0));
                            break;
    
                        case ELEMENT_TYPE_U1:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U1, nelems, 0));
                            break;
    
                        case ELEMENT_TYPE_I2:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I2, nelems, 1));
                            break;
    
                        case ELEMENT_TYPE_U2:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U2, nelems, 1));
                            break;
    
                        IN_WIN32(case ELEMENT_TYPE_I:)
                        case ELEMENT_TYPE_I4:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I4, nelems, 2));
                            break;
    
                        IN_WIN32(case ELEMENT_TYPE_U:)
                        case ELEMENT_TYPE_U4:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U4, nelems, 2));
                            break;
    
                        IN_WIN64(case ELEMENT_TYPE_I:)
                        case ELEMENT_TYPE_I8:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I8, nelems, 3));
                            break;
    
                        IN_WIN64(case ELEMENT_TYPE_U:)
                        case ELEMENT_TYPE_U8:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U8, nelems, 3));
                            break;
    
                        case ELEMENT_TYPE_R4:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_R4, nelems, 2));
                            break;
    
                        case ELEMENT_TYPE_R8:
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_R8, nelems, 3));
                            break;
    
                        case ELEMENT_TYPE_BOOLEAN:
                            INITFIELDMARSHALER(NFT_FIXEDBOOLARRAY, FieldMarshaler_FixedBoolArray, (nelems));
                            break;
    
                        case ELEMENT_TYPE_CHAR:
                            if (fAnsi) {
                                INITFIELDMARSHALER(NFT_FIXEDCHARARRAYANSI, FieldMarshaler_FixedCharArrayAnsi, (nelems));
                            } else {
                                INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_CHAR, nelems, 1));
                            }
                            break;
    
                        default:
                            break;
                    }
                }

            }
            break;

        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_ARRAY:
            break;

        default:
            // let it fall thru as NFT_NONE
            break;

    }

    if (pfwalk->m_nft == NFT_NONE) {
        INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD));
    }
    return S_OK;
#undef INITFIELDMARSHALER
}


//=======================================================================
// Called from the clsloader to load up and summarize the field metadata
// for layout classes.
//
// Warning: This function can load other classes (esp. for nested structs.)
//=======================================================================
HRESULT CollectLayoutFieldMetadata(
   mdTypeDef cl,                // cl of the NStruct being loaded
   BYTE packingSize,            // packing size (from @dll.struct)
   BYTE nlType,                 // nltype (from @dll.struct)
   BOOL fExplicitOffsets,       // explicit offsets?
   EEClass *pParentClass,       // the loaded superclass
   ULONG cMembers,              // total number of members (methods + fields)
   HENUMInternal *phEnumField,  // enumerator for field
   Module* pModule,             // Module that defines the scope, loader and heap (for allocate FieldMarshalers)
   EEClassLayoutInfo *pEEClassLayoutInfoOut,  // caller-allocated structure to fill in.
   LayoutRawFieldInfo *pInfoArrayOut, // caller-allocated array to fill in.  Needs room for cMember+1 elements
   OBJECTREF *pThrowable
)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr;
    MD_CLASS_LAYOUT classlayout;
    mdFieldDef      fd;
    ULONG           ulOffset;
    ULONG           cFields = 0;

    _ASSERTE(pModule);
    ClassLoader* pLoader = pModule->GetClassLoader();

    _ASSERTE(pLoader);
    LoaderHeap *pLoaderHeap = pLoader->GetHighFrequencyHeap();     // heap to allocate FieldMarshalers out of

    IMDInternalImport *pInternalImport = pModule->GetMDImport();    // Internal interface for the NStruct being loaded.


#ifdef _DEBUG
    LPCUTF8 szName; 
    LPCUTF8 szNamespace; 
    pInternalImport->GetNameOfTypeDef(cl, &szName, &szNamespace);
#endif

    BOOL fHasNonTrivialParent = pParentClass &&
                                !pParentClass->IsObjectClass() &&
                                !GetAppDomain()->IsSpecialObjectClass(pParentClass->GetMethodTable()) &&
                                !pParentClass->IsValueTypeClass();


    //====================================================================
    // First, some validation checks.
    //====================================================================
    if (fHasNonTrivialParent && !(pParentClass->HasLayout()))
    {
        pModule->GetAssembly()->PostTypeLoadException(pInternalImport, cl,
                                                      IDS_CLASSLOAD_NSTRUCT_PARENT, pThrowable);
        return COR_E_TYPELOAD;
    }




    hr = pInternalImport->GetClassLayoutInit(
                                     cl,
                                     &classlayout);
    if (FAILED(hr)) {
        BAD_FORMAT_ASSERT(!"Couldn't get classlayout.");
        return hr;
    }

    

    pEEClassLayoutInfoOut->m_DeclaredPackingSize = packingSize;
    pEEClassLayoutInfoOut->m_nlType              = nlType;
    pEEClassLayoutInfoOut->m_fAutoOffset         = !fExplicitOffsets;
    pEEClassLayoutInfoOut->m_numCTMFields        = fHasNonTrivialParent ? ((LayoutEEClass*)pParentClass)->GetLayoutInfo()->m_numCTMFields : 0;
    pEEClassLayoutInfoOut->m_pFieldMarshalers    = NULL;
    pEEClassLayoutInfoOut->m_fBlittable          = TRUE;
    if (fHasNonTrivialParent)
    {
        pEEClassLayoutInfoOut->m_fBlittable = (pParentClass->IsBlittable());
    }

    LayoutRawFieldInfo *pfwalk = pInfoArrayOut;

    LayoutRawFieldInfo **pSortArray = (LayoutRawFieldInfo**)_alloca(cMembers * sizeof(LayoutRawFieldInfo*));
    LayoutRawFieldInfo **pSortArrayEnd = pSortArray;

    ULONG   maxRid = pInternalImport->GetCountWithTokenKind(mdtFieldDef);
    //=====================================================================
    // Phase 1: Figure out the NFT of each field based on both the COM+
    // signature of the field and the NStruct metadata. 
    //=====================================================================
    ULONG i;
    for (i = 0; pInternalImport->EnumNext(phEnumField, &fd); i++) {
        DWORD       dwFieldAttrs;
        // MD Val.check: token validity
        ULONG       rid = RidFromToken(fd);
        if((rid == 0)||(rid > maxRid))
        {
            BAD_FORMAT_ASSERT(!"Invalid Field Token");
            return COR_E_TYPELOAD;
        }

        dwFieldAttrs = pInternalImport->GetFieldDefProps(fd);

        PCCOR_SIGNATURE pNativeType = NULL;
        ULONG       cbNativeType;
        if ( !(IsFdStatic(dwFieldAttrs)) ) {
            PCCOR_SIGNATURE pCOMSignature;
            ULONG       cbCOMSignature;

            if (IsFdHasFieldMarshal(dwFieldAttrs)) {
                hr = pInternalImport->GetFieldMarshal(fd, &pNativeType, &cbNativeType);
                if (FAILED(hr)) {
                    cbNativeType = 0;
                }
            } else {
                cbNativeType = 0;
            }


            pCOMSignature = pInternalImport->GetSigOfFieldDef(fd,&cbCOMSignature);

            hr = ::validateTokenSig(fd,pCOMSignature,cbCOMSignature,dwFieldAttrs,pInternalImport);
            if(FAILED(hr)) return hr;

            // fill the appropriate entry in pInfoArrayOut
            pfwalk->m_MD = fd;
            pfwalk->m_nft = NULL;
            pfwalk->m_offset = (UINT32) -1;
            pfwalk->m_sequence = 0;

#ifdef _DEBUG
            LPCUTF8 szFieldName = pInternalImport->GetNameOfFieldDef(fd);
#endif

            hr = ParseNativeType(pModule,
                                 pCOMSignature,
                                 nlType,
                                 pfwalk,
                                 pNativeType,
                                 cbNativeType,
                                 pInternalImport,
                                 cl,
                                 pThrowable

#ifdef _DEBUG
                                 ,
                                 szName,
                                 szFieldName
#endif
                                );

            if (FAILED(hr)) {
                return hr;
            }


            BOOL    resetBlittable = TRUE;

            // if it's a simple copy...
            if (pfwalk->m_nft == NFT_COPY1    ||
                pfwalk->m_nft == NFT_COPY2    ||
                pfwalk->m_nft == NFT_COPY4    ||
                pfwalk->m_nft == NFT_COPY8)
            {
                resetBlittable = FALSE;
            }

            // Or if it's a nested value class that is itself blittable...
            if (pfwalk->m_nft == NFT_NESTEDVALUECLASS)
            {
                FieldMarshaler *pFM = (FieldMarshaler*)&(pfwalk->m_FieldMarshaler);
                _ASSERTE(pFM->IsNestedValueClassMarshaler());
                if (((FieldMarshaler_NestedValueClass *) pFM)->IsBlittable())
                {
                    resetBlittable = FALSE;
                }
            }

            // ...Otherwise, this field prevents blitting
            if (resetBlittable)
                pEEClassLayoutInfoOut->m_fBlittable          = FALSE;

            cFields++;
            pfwalk++;
        }
    }

    _ASSERTE(i == cMembers);

    // NULL out the last entry
    pfwalk->m_MD = mdFieldDefNil;
    
    
    //
    // fill in the layout information 
    //
    
    // pfwalk points to the beginging of the array
    pfwalk = pInfoArrayOut;

    while (SUCCEEDED(hr = pInternalImport->GetClassLayoutNext(
                                     &classlayout,
                                     &fd,
                                     &ulOffset)) &&
                                     fd != mdFieldDefNil)
    {
        // watch for the last entry: must be mdFieldDefNil
        while ((mdFieldDefNil != pfwalk->m_MD)&&(pfwalk->m_MD < fd))
        {
            pfwalk++;
        }
        if(pfwalk->m_MD != fd) continue;
        // if we haven't found a matching token, it must be a static field with layout -- ignore it
        _ASSERTE(pfwalk->m_MD == fd);
        if (!fExplicitOffsets) {
            // ulOffset is the sequence
            pfwalk->m_sequence = ulOffset;
        }
        else {

            // ulOffset is the explicit offset
            pfwalk->m_offset = ulOffset;
            pfwalk->m_sequence = (ULONG) -1;

            if (pParentClass && pParentClass->HasLayout()) {
                // Treat base class as an initial member.
                if (!SafeAddUINT32(&(pfwalk->m_offset), pParentClass->GetLayoutInfo()->GetNativeSize()))
                {
                    return E_OUTOFMEMORY;
                }
            }


        }

    }
    if (FAILED(hr)) {
        return hr;
    }

    
    // now sort the array
    if (!fExplicitOffsets) { // sort sequential by ascending sequence
        for (i = 0; i < cFields; i++) {
            LayoutRawFieldInfo**pSortWalk = pSortArrayEnd;
            while (pSortWalk != pSortArray) {
                if (pInfoArrayOut[i].m_sequence >= (*(pSortWalk-1))->m_sequence) {
                    break;
                }
                pSortWalk--;
            }
            // pSortWalk now points to the target location for new FieldInfo.
            MoveMemory(pSortWalk + 1, pSortWalk, (pSortArrayEnd - pSortWalk) * sizeof(LayoutRawFieldInfo*));
            *pSortWalk = &pInfoArrayOut[i];
            pSortArrayEnd++;
        }
    }
    else // no sorting for explicit layout
    {
        for (i = 0; i < cFields; i++) {
            if(pInfoArrayOut[i].m_MD != mdFieldDefNil && pInfoArrayOut[i].m_offset == (UINT32) -1) {

                LPCUTF8 szFieldName;
                szFieldName = pInternalImport->GetNameOfFieldDef(pInfoArrayOut[i].m_MD);

                pModule->GetAssembly()->PostTypeLoadException(pInternalImport, 
                                                              cl,
                                                              szFieldName,
                                                              IDS_CLASSLOAD_NSTRUCT_EXPLICIT_OFFSET, 
                                                              pThrowable);
                return COR_E_TYPELOAD;
            }
                
            *pSortArrayEnd = &pInfoArrayOut[i];
            pSortArrayEnd++;
        }
    }

    //=====================================================================
    // Phase 2: Compute the native size (in bytes) of each field.
    // Store this in pInfoArrayOut[].cbNativeSize;
    //=====================================================================


    // Now compute the native size of each field
    for (pfwalk = pInfoArrayOut; pfwalk->m_MD != mdFieldDefNil; pfwalk++) {
        UINT8 nft = pfwalk->m_nft;
        pEEClassLayoutInfoOut->m_numCTMFields++;

        // If the NFT's size never changes, it is stored in the database.
        UINT32 cbNativeSize = NFTDataBase[nft].m_cbNativeSize;

        if (cbNativeSize == 0) {
            // Size of 0 means NFT's size is variable, so we have to figure it
            // out case by case.
            cbNativeSize = ((FieldMarshaler*)&(pfwalk->m_FieldMarshaler))->NativeSize();
        }
        pfwalk->m_cbNativeSize = cbNativeSize;
    }

    if (pEEClassLayoutInfoOut->m_numCTMFields) {
        pEEClassLayoutInfoOut->m_pFieldMarshalers = (FieldMarshaler*)(pLoaderHeap->AllocMem(MAXFIELDMARSHALERSIZE * pEEClassLayoutInfoOut->m_numCTMFields));
        if (!pEEClassLayoutInfoOut->m_pFieldMarshalers) {
            return E_OUTOFMEMORY;
        }

        // Bring in the parent's fieldmarshalers
        if (fHasNonTrivialParent)
        {
            EEClassLayoutInfo *pParentLayoutInfo = ((LayoutEEClass*)pParentClass)->GetLayoutInfo();
            UINT numChildCTMFields = pEEClassLayoutInfoOut->m_numCTMFields - pParentLayoutInfo->m_numCTMFields;
            memcpyNoGCRefs( ((BYTE*)pEEClassLayoutInfoOut->m_pFieldMarshalers) + MAXFIELDMARSHALERSIZE*numChildCTMFields,
                            pParentLayoutInfo->m_pFieldMarshalers,
                            MAXFIELDMARSHALERSIZE * (pParentLayoutInfo->m_numCTMFields) );
        }

    }

    //=====================================================================
    // Phase 3: If NStruct requires autooffsetting, compute the offset
    // of each field and the size of the total structure. We do the layout
    // according to standard VC layout rules:
    //
    //   Each field has an alignment requirement. The alignment-requirement
    //   of a scalar field is the smaller of its size and the declared packsize.
    //   The alighnment-requirement of a struct field is the smaller of the
    //   declared packsize and the largest of the alignment-requirement
    //   of its fields. The alignment requirement of an array is that
    //   of one of its elements.
    //
    //   In addition, each struct gets padding at the end to ensure
    //   that an array of such structs contain no unused space between
    //   elements.
    //=====================================================================
    BYTE   LargestAlignmentRequirement = 1;
    UINT32 cbCurOffset = 0;

    if (pParentClass && pParentClass->HasLayout()) {
        // Treat base class as an initial member.
        if (!SafeAddUINT32(&cbCurOffset, pParentClass->GetLayoutInfo()->GetNativeSize()))
        {
            return E_OUTOFMEMORY;
        }


        BYTE alignmentRequirement = min(packingSize, pParentClass->GetLayoutInfo()->GetLargestAlignmentRequirementOfAllMembers());
        LargestAlignmentRequirement = max(LargestAlignmentRequirement, alignmentRequirement);
                                          
    }
    unsigned calcTotalSize = 1;         // The current size of the structure as a whole, we start at 1, because we 
                                        // disallow 0 sized structures. 
    LayoutRawFieldInfo **pSortWalk;
    for (pSortWalk = pSortArray, i=cFields; i; i--, pSortWalk++) {
        pfwalk = *pSortWalk;

        BYTE alignmentRequirement = ((FieldMarshaler*)&(pfwalk->m_FieldMarshaler))->AlignmentRequirement();
        _ASSERTE(alignmentRequirement == 1 ||
                 alignmentRequirement == 2 ||
                 alignmentRequirement == 4 ||
                 alignmentRequirement == 8);

        alignmentRequirement = min(alignmentRequirement, packingSize);
        LargestAlignmentRequirement = max(LargestAlignmentRequirement, alignmentRequirement);

        // This assert means I forgot to special-case some NFT in the
        // above switch.
        _ASSERTE(alignmentRequirement <= 8);

        // Check if this field is overlapped with other(s)
        pfwalk->m_fIsOverlapped = FALSE;
        if (fExplicitOffsets) {
            LayoutRawFieldInfo *pfwalk1;
            DWORD dwBegin = pfwalk->m_offset;
            DWORD dwEnd = dwBegin+pfwalk->m_cbNativeSize;
            for (pfwalk1 = pInfoArrayOut; pfwalk1 < pfwalk; pfwalk1++) {
                if((pfwalk1->m_offset >= dwEnd) || (pfwalk1->m_offset+pfwalk1->m_cbNativeSize <= dwBegin)) continue;
                pfwalk->m_fIsOverlapped = TRUE;
                pfwalk1->m_fIsOverlapped = TRUE;
            }
        }
        else {
            // Insert enough padding to align the current data member.
            while (cbCurOffset % alignmentRequirement) {
                if (!SafeAddUINT32(&cbCurOffset, 1))
                    return E_OUTOFMEMORY;
            }

            // Insert current data member.
            pfwalk->m_offset = cbCurOffset;
            cbCurOffset += pfwalk->m_cbNativeSize;      // if we overflow we will catch it below
        } 

        unsigned fieldEnd = pfwalk->m_offset + pfwalk->m_cbNativeSize;
        if (fieldEnd < pfwalk->m_offset)
            return E_OUTOFMEMORY;

            // size of the structure is the size of the last field.  
        if (fieldEnd > calcTotalSize)
            calcTotalSize = fieldEnd;
    }

    ULONG clstotalsize = 0;
    pInternalImport->GetClassTotalSize(cl, &clstotalsize);

    if (clstotalsize != 0) {

        if (pParentClass && pParentClass->HasLayout()) {
            // Treat base class as an initial member.

            UINT32 parentSize = pParentClass->GetLayoutInfo()->GetNativeSize();
            if (clstotalsize + parentSize < clstotalsize)
            {
                return E_OUTOFMEMORY;
            }
            clstotalsize += parentSize;
        }

            // they can't give us a bad size (too small).
        if (clstotalsize < calcTotalSize)
        {
            pModule->GetAssembly()->PostTypeLoadException(pInternalImport, cl,
                                                          IDS_CLASSLOAD_BADFORMAT, pThrowable);
            return COR_E_TYPELOAD;
        }
        calcTotalSize = clstotalsize;   // use the size they told us 
    } 
    else {
            // The did not give us an explict size, so lets round up to a good size (for arrays) 
        while (calcTotalSize % LargestAlignmentRequirement) {
            if (!SafeAddUINT32(&calcTotalSize, 1))
                return E_OUTOFMEMORY;
        }
    }

    // We'll cap the total native size at a (somewhat) arbitrary limit to ensure
    // that we don't expose some overflow bug later on.
    if (calcTotalSize >= 0x7ffffff0)
        return E_OUTOFMEMORY;

    pEEClassLayoutInfoOut->m_cbNativeSize = calcTotalSize;

        // The packingSize acts as a ceiling on all individual alignment
    // requirements so it follows that the largest alignment requirement
    // is also capped.
    _ASSERTE(LargestAlignmentRequirement <= packingSize);
    pEEClassLayoutInfoOut->m_LargestAlignmentRequirementOfAllMembers = LargestAlignmentRequirement;

#ifdef _DEBUG
    {
        LOG((LF_INTEROP, LL_INFO100000, "\n\n"));
        LOG((LF_INTEROP, LL_INFO100000, "Packsize      = %lu\n", (ULONG)packingSize));
        LOG((LF_INTEROP, LL_INFO100000, "Max align req = %lu\n", (ULONG)(pEEClassLayoutInfoOut->m_LargestAlignmentRequirementOfAllMembers)));
        LOG((LF_INTEROP, LL_INFO100000, "----------------------------\n"));
        for (pfwalk = pInfoArrayOut; pfwalk->m_MD != mdFieldDefNil; pfwalk++) {
            LPCUTF8 fieldname = "??";
            fieldname = pInternalImport->GetNameOfFieldDef(pfwalk->m_MD);
            LOG((LF_INTEROP, LL_INFO100000, "+%-5lu  ", (ULONG)(pfwalk->m_offset)));
            LOG((LF_INTEROP, LL_INFO100000, "%s", fieldname));
            LOG((LF_INTEROP, LL_INFO100000, "\n"));
        }
        LOG((LF_INTEROP, LL_INFO100000, "+%-5lu   EOS\n", (ULONG)(pEEClassLayoutInfoOut->m_cbNativeSize)));
    }
#endif
    return S_OK;
}

//=======================================================================
// For each reference-typed NStruct field, marshals the current COM+ value
// to a new native instance and stores it in the fixed portion of the NStruct.
//
// This function does not attempt to delete the native value that it overwrites.
//
// If pOptionalCleanupWorkList is non-null, this function also schedules
// (unconditionally) a nativedestroy on that field (note that if the
// contents of that field changes before the cleanupworklist fires,
// the new value is what will be destroyed. This is by design, as it
// unifies cleanup for [in,out] parameters.)
//=======================================================================
VOID LayoutUpdateNative(LPVOID *ppProtectedManagedData, UINT offsetbias, EEClass *pcls, BYTE* pNativeData, CleanupWorkList *pOptionalCleanupWorkList)
{
    THROWSCOMPLUSEXCEPTION();

    pcls->CheckRestore();

    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    while (numReferenceFields--) {

        DWORD internalOffset = pFieldMarshaler->m_pFD->GetOffset();

        if (pFieldMarshaler->IsScalarMarshaler()) {
            pFieldMarshaler->ScalarUpdateNative( internalOffset + offsetbias + (BYTE*)( *ppProtectedManagedData ),
                                                 pNativeData + pFieldMarshaler->m_dwExternalOffset );
        } else if (pFieldMarshaler->IsNestedValueClassMarshaler()) {
            pFieldMarshaler->NestedValueClassUpdateNative((const VOID **)ppProtectedManagedData, internalOffset + offsetbias, pNativeData + pFieldMarshaler->m_dwExternalOffset);
        } else {
            pFieldMarshaler->UpdateNative(
                                ObjectToOBJECTREF (*(Object**)(internalOffset + offsetbias + (BYTE*)( *ppProtectedManagedData ))),
                                pNativeData + pFieldMarshaler->m_dwExternalOffset
                             );
    
        }
        if (pOptionalCleanupWorkList) {
            pOptionalCleanupWorkList->ScheduleUnconditionalNStructDestroy(pFieldMarshaler, pNativeData + pFieldMarshaler->m_dwExternalOffset);
        }


        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }



}



VOID FmtClassUpdateNative(OBJECTREF *ppProtectedManagedData, BYTE *pNativeData)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = (*ppProtectedManagedData)->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pNativeData, (*ppProtectedManagedData)->GetData(), cbsize);
    } else {
        // This allows us to do a partial LayoutDestroyNative in the case of
        // a marshaling error on one of the fields.
        FillMemory(pNativeData, cbsize, 0);
        EE_TRY_FOR_FINALLY {
            LayoutUpdateNative( (VOID**)ppProtectedManagedData,
                                Object::GetOffsetOfFirstField(),
                                pcls,
                                pNativeData,
                                NULL
                              );
        } EE_FINALLY {
            if (GOT_EXCEPTION()) {
                LayoutDestroyNative(pNativeData, pcls);
                FillMemory(pNativeData, cbsize, 0);
            }
        } EE_END_FINALLY;
    }

}

VOID FmtClassUpdateNative(OBJECTREF pObj, BYTE *pNativeData)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pObj->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pNativeData, pObj->GetData(), cbsize);
    } else {
        GCPROTECT_BEGIN(pObj);
        FmtClassUpdateNative(&pObj, pNativeData);
        GCPROTECT_END();
    }
}



VOID FmtClassUpdateComPlus(OBJECTREF *ppProtectedManagedData, BYTE *pNativeData, BOOL fDeleteOld)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = (*ppProtectedManagedData)->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs((*ppProtectedManagedData)->GetData(), pNativeData, cbsize);
    } else {
        LayoutUpdateComPlus((VOID**)ppProtectedManagedData,
                            Object::GetOffsetOfFirstField(),
                            pcls,
                            (BYTE*)pNativeData,
                            fDeleteOld
                           );
    }
}


VOID FmtClassUpdateComPlus(OBJECTREF pObj, BYTE *pNativeData, BOOL fDeleteOld)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pObj->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pObj->GetData(), pNativeData, cbsize);
    } else {
        GCPROTECT_BEGIN(pObj);
        LayoutUpdateComPlus((VOID**)&pObj,
                            Object::GetOffsetOfFirstField(),
                            pcls,
                            (BYTE*)pNativeData,
                            fDeleteOld
                           );
        GCPROTECT_END();
    }
}





     


//=======================================================================
// For each reference-typed NStruct field, marshals the current COM+ value
// to a new COM+ instance and stores it in the GC portion of the NStruct.
//
// If fDeleteNativeCopies is true, it will also destroy the native version.
//
// NOTE: To avoid error-path leaks, this function attempts to destroy
// all of the native fields even if one or more of the conversions fail.
//=======================================================================
VOID LayoutUpdateComPlus(LPVOID *ppProtectedManagedData, UINT offsetbias, EEClass *pcls, BYTE *pNativeData, BOOL fDeleteNativeCopies)
{
    THROWSCOMPLUSEXCEPTION();

    pcls->CheckRestore();

    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    struct _gc {
        OBJECTREF pException;
        OBJECTREF pComPlusValue;
    } gc;
    gc.pException    = NULL;
    gc.pComPlusValue = NULL;
    GCPROTECT_BEGIN(gc);


    while (numReferenceFields--) {

        DWORD internalOffset = pFieldMarshaler->m_pFD->GetOffset();


        // Wrap UpdateComPlus in a catch block - even if this fails,
        // we have to destroy all of the native fields.
        COMPLUS_TRY {
            if (pFieldMarshaler->IsScalarMarshaler()) {
                pFieldMarshaler->ScalarUpdateComPlus( pNativeData + pFieldMarshaler->m_dwExternalOffset,
                                                      internalOffset + offsetbias + (BYTE*)(*ppProtectedManagedData) );
            } else if (pFieldMarshaler->IsNestedValueClassMarshaler()) {
                pFieldMarshaler->NestedValueClassUpdateComPlus(pNativeData + pFieldMarshaler->m_dwExternalOffset, ppProtectedManagedData, internalOffset + offsetbias);
            } else {
                pFieldMarshaler->UpdateComPlus(
                                    pNativeData + pFieldMarshaler->m_dwExternalOffset,
                                    &gc.pComPlusValue
                                 );
    
    
                SetObjectReferenceUnchecked( (OBJECTREF*) (internalOffset + offsetbias + (BYTE*)(*ppProtectedManagedData)), 
                                             gc.pComPlusValue );
    
            }
        } COMPLUS_CATCH {
            gc.pException = GETTHROWABLE();
        } COMPLUS_END_CATCH

        if (fDeleteNativeCopies) {
            pFieldMarshaler->DestroyNative(pNativeData + pFieldMarshaler->m_dwExternalOffset);
        }

        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }

    if (gc.pException != NULL) {
        COMPlusThrow(gc.pException);
    }

    GCPROTECT_END();


}









VOID LayoutDestroyNative(LPVOID pNative, EEClass *pcls)
{
    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();
    BYTE *pNativeData                     = (BYTE*)pNative;

    while (numReferenceFields--) {
        pFieldMarshaler->DestroyNative( pNativeData + pFieldMarshaler->m_dwExternalOffset );
        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }
}

VOID FmtClassDestroyNative(LPVOID pNative, EEClass *pcls)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if (pNative)
    {
        if (!(pcls->IsBlittable()))
        {
            _ASSERTE(pcls->HasLayout());
            LayoutDestroyNative(pNative, pcls);
        }
    }
}



VOID FmtValueTypeUpdateNative(LPVOID pProtectedManagedData, MethodTable *pMT, BYTE *pNativeData)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pMT->GetClass();
    _ASSERTE(pcls->IsValueClass() && (pcls->IsBlittable() || pcls->HasLayout()));
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pNativeData, pProtectedManagedData, cbsize);
    } else {
        // This allows us to do a partial LayoutDestroyNative in the case of
        // a marshaling error on one of the fields.
        FillMemory(pNativeData, cbsize, 0);
        EE_TRY_FOR_FINALLY {
            LayoutUpdateNative( (VOID**)pProtectedManagedData,
                                0,
                                pcls,
                                pNativeData,
                                NULL
                              );
        } EE_FINALLY {
            if (GOT_EXCEPTION()) {
                LayoutDestroyNative(pNativeData, pcls);
                FillMemory(pNativeData, cbsize, 0);
            }
        } EE_END_FINALLY;
    }

}

VOID FmtValueTypeUpdateComPlus(LPVOID pProtectedManagedData, MethodTable *pMT, BYTE *pNativeData, BOOL fDeleteOld)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pMT->GetClass();
    _ASSERTE(pcls->IsValueClass() && (pcls->IsBlittable() || pcls->HasLayout()));
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pProtectedManagedData, pNativeData, cbsize);
    } else {
        LayoutUpdateComPlus((VOID**)pProtectedManagedData,
                            0,
                            pcls,
                            (BYTE*)pNativeData,
                            fDeleteOld
                           );
    }
}








//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedLayoutClass::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    UINT32     cbNativeSize = m_pNestedMethodTable->GetNativeSize();


    if (pComPlusValue == NULL) {
        ZeroMemory(pNativeValue, cbNativeSize);
    } else {
        GCPROTECT_BEGIN(pComPlusValue);
        LayoutUpdateNative((LPVOID*)&pComPlusValue, Object::GetOffsetOfFirstField(), m_pNestedMethodTable->GetClass(), (BYTE*)pNativeValue, NULL);
        GCPROTECT_END();
    }

}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedLayoutClass::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = AllocateObject(m_pNestedMethodTable);

    LayoutUpdateComPlus( (LPVOID*)ppProtectedComPlusValue,
                         Object::GetOffsetOfFirstField(),
                         m_pNestedMethodTable->GetClass(),
                         (BYTE *)pNativeValue,
                         FALSE);

}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedLayoutClass::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LayoutDestroyNative(pNativeValue, m_pNestedMethodTable->GetClass());

}



//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedLayoutClass::NativeSize()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetNativeSize();
}

//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedLayoutClass::AlignmentRequirement()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetLargestAlignmentRequirementOfAllMembers();
}



//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedValueClass::NestedValueClassUpdateNative(const VOID **ppProtectedComPlus, UINT startoffset, LPVOID pNative) const
{
    THROWSCOMPLUSEXCEPTION();

    // would be better to detect this at class load time (that have a nested value
    // class with no layout) but don't have a way to know
    if (! m_pNestedMethodTable->GetClass()->GetLayoutInfo())
        COMPlusThrow(kArgumentException, IDS_NOLAYOUT_IN_EMBEDDED_VALUECLASS);

    LayoutUpdateNative((LPVOID*)ppProtectedComPlus, startoffset, m_pNestedMethodTable->GetClass(), (BYTE*)pNative, NULL);
}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedValueClass::NestedValueClassUpdateComPlus(const VOID *pNative, LPVOID *ppProtectedComPlus, UINT startoffset) const
{
    THROWSCOMPLUSEXCEPTION();

    // would be better to detect this at class load time (that have a nested value
    // class with no layout) but don't have a way to know
    if (! m_pNestedMethodTable->GetClass()->GetLayoutInfo())
        COMPlusThrow(kArgumentException, IDS_NOLAYOUT_IN_EMBEDDED_VALUECLASS);

    LayoutUpdateComPlus( (LPVOID*)ppProtectedComPlus,
                         startoffset,
                         m_pNestedMethodTable->GetClass(),
                         (BYTE *)pNative,
                         FALSE);
    

}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedValueClass::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LayoutDestroyNative(pNativeValue, m_pNestedMethodTable->GetClass());
}



//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedValueClass::NativeSize()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    // this can't be marshalled as native type if no layout, so we allow the 
    // native size info to be created if available, but the size will only
    // be valid for native, not unions. Marshaller will throw exception if
    // try to marshall a value class with no layout
    if (m_pNestedMethodTable->GetClass()->HasLayout())
        return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetNativeSize();
    return 0;
}

//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedValueClass::AlignmentRequirement()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    // this can't be marshalled as native type if no layout, so we allow the 
    // native size info to be created if available, but the alignment will only
    // be valid for native, not unions. Marshaller will throw exception if
    // try to marshall a value class with no layout
    if (m_pNestedMethodTable->GetClass()->HasLayout())
        return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetLargestAlignmentRequirementOfAllMembers();
    return 1;
}






//=======================================================================
// CoTask Uni <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringUni::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((LPWSTR*)pNativeValue) = NULL;
    } else {
        DWORD nc   = pString->GetStringLength();
        if (nc > 0x7ffffff0)
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        }
        LPWSTR wsz = (LPWSTR)CoTaskMemAlloc( (nc + 1) * sizeof(WCHAR) );
        if (!wsz) {
            COMPlusThrowOM();
        }
        CopyMemory(wsz, pString->GetBuffer(), nc*sizeof(WCHAR));
        wsz[nc] = L'\0';
        *((LPWSTR*)pNativeValue) = wsz;

        //printf("Created UniString %lxh\n", *(LPWSTR*)pNativeValue);
    }
}


//=======================================================================
// CoTask Uni <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringUni::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    LPCWSTR wsz = *((LPCWSTR*)pNativeValue);
    if (!wsz) {
        pString = NULL;
    } else {
        DWORD length = (DWORD)wcslen(wsz);
        if (length > 0x7ffffff0)
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        }

        pString = COMString::NewString(wsz, length);
    }
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}


//=======================================================================
// CoTask Uni <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringUni::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LPWSTR wsz = *((LPWSTR*)pNativeValue);
    *((LPWSTR*)pNativeValue) = NULL;
    if (wsz) {
        CoTaskMemFree(wsz);
    }
}










//=======================================================================
// CoTask Ansi <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringAnsi::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((LPSTR*)pNativeValue) = NULL;
    } else {

        DWORD nc   = pString->GetStringLength();
        if (nc > 0x7ffffff0)
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        }
        LPSTR sz = (LPSTR)CoTaskMemAlloc( (nc + 1) * 2 /* 2 for MBCS */ );
        if (!sz) {
            COMPlusThrowOM();
        }

        if (nc == 0) {
            *sz = '\0';
        } else {
            int nbytes = WszWideCharToMultiByte(CP_ACP,
                                     0,
                                     pString->GetBuffer(),
                                     nc,   // # of wchars in inbuffer
                                     sz,
                                     nc*2, // size in bytes of outbuffer
                                     NULL,
                                     NULL);
            if (!nbytes) {
                COMPlusThrow(kArgumentException, IDS_UNI2ANSI_FAILURE_IN_NSTRUCT);
            }
            sz[nbytes] = '\0';
        }



        *((LPSTR*)pNativeValue) = sz;

        //printf("Created AnsiString %lxh\n", *(LPSTR*)pNativeValue);
    }
}


//=======================================================================
// CoTask Ansi <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringAnsi::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString = NULL;
    LPCSTR sz = *((LPCSTR*)pNativeValue);
    if (!sz) {
        pString = NULL;
    } else {

        int cwsize = MultiByteToWideChar(CP_ACP,
                                         MB_PRECOMPOSED,
                                         sz,
                                         -1,  
                                         NULL,
                                         0);
        if (cwsize == 0) {
            COMPlusThrow(kArgumentException, IDS_ANSI2UNI_FAILURE_IN_NSTRUCT);
        } else if (cwsize < 0 || cwsize > 0x7ffffff0) {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        } else {
            CQuickBytes qb;
            //printf("MB2W returned %lu\n", cwsize);
            LPWSTR wsztemp = (LPWSTR)qb.Alloc(cwsize*sizeof(WCHAR));
            if (!wsztemp)
            {
                COMPlusThrowOM();
            }
            MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                sz,     
                                -1,     // # CHARs in inbuffer
                                wsztemp,
                                cwsize  // size (in WCHAR's) of outbuffer
                                );
            pString = COMString::NewString(wsztemp, (cwsize - 1));
        }


    }
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}


//=======================================================================
// CoTask Ansi <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringAnsi::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LPSTR sz = *((LPSTR*)pNativeValue);
    *((LPSTR*)pNativeValue) = NULL;
    if (sz) {
        CoTaskMemFree(sz);
    }
}










//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringUni::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((WCHAR*)pNativeValue) = L'\0';
    } else {
        DWORD nc = pString->GetStringLength();
        if (nc >= m_numchar) {
            nc = m_numchar - 1;
        }
        CopyMemory(pNativeValue, pString->GetBuffer(), nc*sizeof(WCHAR));
        ((WCHAR*)pNativeValue)[nc] = L'\0';
    }

}


//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringUni::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    DWORD     ncActual;
    for (ncActual = 0; *(ncActual + (WCHAR*)pNativeValue) != L'\0' && ncActual < m_numchar; ncActual++) {
        //nothing
    }
    pString = COMString::NewString((const WCHAR *)pNativeValue, ncActual);
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}







//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringAnsi::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((CHAR*)pNativeValue) = L'\0';
    } else {
        DWORD nc = pString->GetStringLength();
        if (nc >= m_numchar) {
            nc = m_numchar - 1;
        }
        int cbwritten = WszWideCharToMultiByte(CP_ACP,
                                            0,
                                            pString->GetBuffer(),
                                            nc,         // # WCHAR's in inbuffer
                                            (CHAR*)pNativeValue,
                                            m_numchar,  // size (in CHAR's) out outbuffer
                                            NULL,
                                            NULL);
        ((CHAR*)pNativeValue)[cbwritten] = '\0';
    }

}


//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringAnsi::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;

    _ASSERTE(m_numchar != 0);  // should not have slipped past the metadata
    if (m_numchar == 0)
    {
        // but if it does, better to throw an exception tardily rather than
        // allow a memory corrupt.
        COMPlusThrow(kMarshalDirectiveException);
    }

    LPSTR tempbuf = (LPSTR)(_alloca(m_numchar + 2));
    if (!tempbuf) {
        COMPlusThrowOM();
    }
    memcpyNoGCRefs(tempbuf, pNativeValue, m_numchar);
    tempbuf[m_numchar-1] = '\0';
    tempbuf[m_numchar] = '\0';
    tempbuf[m_numchar+1] = '\0';

    LPWSTR    wsztemp = (LPWSTR)_alloca( m_numchar * sizeof(WCHAR) );
    int ncwritten = MultiByteToWideChar(CP_ACP,
                                        MB_PRECOMPOSED,
                                        tempbuf,
                                        -1,  // # of CHAR's in inbuffer
                                        wsztemp,
                                        m_numchar                       // size (in WCHAR) of outbuffer
                                        );

    if (!ncwritten)
    {
        // intentionally not throwing for MB2WC failure. We don't always know
        // whether to expect a valid string in the buffer and we don't want
        // to throw exceptions randomly.
        ncwritten++;
    }

    pString = COMString::NewString((const WCHAR *)wsztemp, ncwritten-1);
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}





                                                 





//=======================================================================
// CHAR[] <--> char[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedCharArrayAnsi::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    I2ARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;

    if (pArray == NULL) {
        FillMemory(pNativeValue, m_numElems * sizeof(CHAR), 0);
    } else {
        if (pArray->GetNumComponents() < m_numElems) {
            COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
        } else {
            WszWideCharToMultiByte(
                CP_ACP,
                0,
                (const WCHAR *)pArray->GetDataPtr(),
                m_numElems,   //# of WCHAR's in input buffer
                (CHAR*)pNativeValue,
                m_numElems * sizeof(CHAR), // size in bytes of outbuffer
                NULL,
                NULL);
        }
    }
}


//=======================================================================
// CHAR[] <--> char[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedCharArrayAnsi::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = AllocatePrimitiveArray(ELEMENT_TYPE_CHAR, m_numElems);
    if (!*ppProtectedComPlusValue) {
        COMPlusThrowOM();
    }
    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        (const CHAR *)pNativeValue,
                        m_numElems * sizeof(CHAR), // size, in bytes, of in buffer
                        (WCHAR*) ((*((I2ARRAYREF*)ppProtectedComPlusValue))->GetDirectPointerToNonObjectElements()),
                        m_numElems                 // size, in WCHAR's of outbuffer
                        );


}






//=======================================================================
// BOOL <--> boolean[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedBoolArray::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    I1ARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;

    if (pArray == NULL) {
        FillMemory(pNativeValue, m_numElems * sizeof(BOOL), 0);
    } else {
        if (pArray->GetNumComponents() < m_numElems) {
            COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
        } else {
            UINT32 nElems   = m_numElems;
            const I1 *pI1   = (const I1 *)(pArray->GetDirectPointerToNonObjectElements());
            BOOL     *pBool = (BOOL*)pNativeValue;
            while (nElems--) {
                *(pBool++) = (*(pI1++)) ? 1 : 0;
            }
        }
    }
}


//=======================================================================
// BOOL <--> boolean[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedBoolArray::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = AllocatePrimitiveArray(ELEMENT_TYPE_I1, m_numElems);
    if (!*ppProtectedComPlusValue) {
        COMPlusThrowOM();
    }
    UINT32 nElems     = m_numElems;
    const BOOL *pBool = (const BOOL*)pNativeValue;
    I1         *pI1   = (I1 *)((*(I1ARRAYREF*)ppProtectedComPlusValue)->GetDirectPointerToNonObjectElements());
    while (nElems--) {
        (*(pI1++)) = *(pBool++) ? 1 : 0;
    }


}





//=======================================================================
// scalar array
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedScalarArray::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    BASEARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;

    if (pArray == NULL) {
        FillMemory(pNativeValue, m_numElems << m_componentShift, 0);
    } else {
        if (pArray->GetNumComponents() < m_numElems) {
            COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
        } else {
            CopyMemory(pNativeValue,
                       pArray->GetDataPtr(),
                       m_numElems << m_componentShift);
        }
    }
}


//=======================================================================
// scalar array
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedScalarArray::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = AllocatePrimitiveArray(m_arrayType, m_numElems);
    if (!*ppProtectedComPlusValue) {
        COMPlusThrowOM();
    }
    memcpyNoGCRefs((*(BASEARRAYREF*)ppProtectedComPlusValue)->GetDataPtr(),
               pNativeValue,
               m_numElems << m_componentShift);


}






//=======================================================================
// function ptr <--> Delegate
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Delegate::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *((VOID**)pNativeValue) = COMDelegate::ConvertToCallback(pComPlusValue);

}


//=======================================================================
// function ptr <--> Delegate
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Delegate::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = COMDelegate::ConvertToDelegate(*(LPVOID*)pNativeValue);

}





//=======================================================================
// COM IP <--> interface
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Interface::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    GCPROTECT_BEGIN(pComPlusValue);
    *((IUnknown**)pNativeValue) = GetComIPFromObjectRef(&pComPlusValue);
    GCPROTECT_END();

}


//=======================================================================
// COM IP <--> interface
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Interface::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = GetObjectRefFromComIP(*((IUnknown**)pNativeValue), m_pClassMT);
}


//=======================================================================
// COM IP <--> interface
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Interface::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    IUnknown *punk = *((IUnknown**)pNativeValue);
    *((IUnknown**)pNativeValue) = NULL;
    ULONG cbRef = SafeRelease(punk);
    LogInteropRelease(punk, cbRef, "Field marshaler destroy native");
}



//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Date::ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
{
    *((DATE*)pNative) =  COMDateTime::TicksToDoubleDate(*((INT64*)pComPlus));
}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Date::ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
{
    *((INT64*)pComPlus) = COMDateTime::DoubleDateToTicks(*((DATE*)pNative));
}






//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Illegal::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    DefineFullyQualifiedNameForClassW();
    GetFullyQualifiedNameForClassW(m_pFD->GetEnclosingClass());

    LPCUTF8 szFieldName = m_pFD->GetName();
    MAKE_WIDEPTR_FROMUTF8(wszFieldName, szFieldName);

    COMPlusThrow(kTypeLoadException, m_resIDWhy, _wszclsname_, wszFieldName);

}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Illegal::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    DefineFullyQualifiedNameForClassW();
    GetFullyQualifiedNameForClassW(m_pFD->GetEnclosingClass());

    LPCUTF8 szFieldName = m_pFD->GetName();
    MAKE_WIDEPTR_FROMUTF8(wszFieldName, szFieldName);

    COMPlusThrow(kTypeLoadException, m_resIDWhy, _wszclsname_, wszFieldName);
}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Illegal::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
}








