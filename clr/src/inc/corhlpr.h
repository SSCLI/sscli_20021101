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
/*****************************************************************************
 **                                                                         **
 ** Corhlpr.h -                                                               
                                                                            **
 **                                                                         **
 *****************************************************************************/


#ifndef __CORHLPR_H__
#define __CORHLPR_H__

#include "cor.h"
#include "corhdr.h"
#include "corerror.h"



//*****************************************************************************
// There are a set of macros commonly used in the helpers which you will want
// to override to get richer behavior.  The following defines what is needed
// if you chose not to do the extra work.
//*****************************************************************************
#ifndef IfFailGoto
#define IfFailGoto(EXPR, LABEL) \
do { hr = (EXPR); if(FAILED(hr)) { goto LABEL; } } while (0)
#endif

#ifndef IfFailGo
#define IfFailGo(EXPR) IfFailGoto(EXPR, ErrExit)
#endif

#ifndef IfFailRet
#define IfFailRet(EXPR) do { hr = (EXPR); if(FAILED(hr)) { return (hr); } } while (0)
#endif

#ifndef _ASSERTE
#define _ASSERTE(expr)
#endif


#ifndef BIGENDIAN
#define VAL16(x) x
#define VAL32(x) x
#endif


//*****************************************************************************
//
//***** Utility helpers
//
//*****************************************************************************


#define MAX_CLASSNAME_LENGTH 1024

//*****************************************************************************
//
// **** CQuickBytes
// This helper class is useful for cases where 90% of the time you allocate 512
// or less bytes for a data structure.  This class contains a 512 byte buffer.
// Alloc() will return a pointer to this buffer if your allocation is small
// enough, otherwise it asks the heap for a larger buffer which is freed for
// you.  No mutex locking is required for the small allocation case, making the
// code run faster, less heap fragmentation, etc...  Each instance will allocate
// 520 bytes, so use accordinly.
//
//*****************************************************************************
template <DWORD SIZE, DWORD INCREMENT> 
class CQuickMemoryBase
{
public:
    void Init()
    {
        pbBuff = 0;
        iSize = 0;
        cbTotal = SIZE;
    }

    void Destroy()
    {
        if (pbBuff)
        {
            free(pbBuff);
            pbBuff = 0;
        }
    }

    void *Alloc(SIZE_T iItems)
    {
        iSize = iItems;
        if (iItems <= SIZE)
        {
            cbTotal = SIZE;
            return (&rgData[0]);
        }
        else
        {
            if (pbBuff) free(pbBuff);
            pbBuff = malloc(iItems);
            cbTotal = pbBuff ? iItems : 0;
            return (pbBuff);
        }
    }

    HRESULT ReSize(SIZE_T iItems)
    {
        void *pbBuffNew;
        if (iItems <= cbTotal)
        {
            iSize = iItems;
            return NOERROR;
        }

        pbBuffNew = malloc(iItems + INCREMENT);
        if (!pbBuffNew)
            return E_OUTOFMEMORY;
        if (pbBuff) 
        {
            memcpy(pbBuffNew, pbBuff, cbTotal);
            free(pbBuff);
        }
        else
        {
            _ASSERTE(cbTotal == SIZE);
            memcpy(pbBuffNew, rgData, cbTotal);
        }
        cbTotal = iItems + INCREMENT;
        iSize = iItems;
        pbBuff = pbBuffNew;
        return NOERROR;
        
    }

    operator PVOID()
    { return ((pbBuff) ? pbBuff : &rgData[0]); }

    void *Ptr()
    { return ((pbBuff) ? pbBuff : &rgData[0]); }

    SIZE_T Size()
    { return (iSize); }

    SIZE_T MaxSize()
    { return (cbTotal); }

    void Maximize()
    {
#ifdef _DEBUG
        HRESULT hr =
#endif
		ReSize(MaxSize());
        _ASSERTE(hr == NOERROR);
    }

    void        *pbBuff;
    SIZE_T      iSize;              // number of bytes used
    SIZE_T      cbTotal;            // total bytes allocated in the buffer
    // use UINT64 to enforce the alignment of the memory
    UINT64 rgData[SIZE/sizeof(UINT64)];
};

// These should be multiples of 8 so that data can be naturally aligned.
#define     CQUICKBYTES_BASE_SIZE           512
#define     CQUICKBYTES_INCREMENTAL_SIZE    128

class CQuickBytesBase : public CQuickMemoryBase<CQUICKBYTES_BASE_SIZE, CQUICKBYTES_INCREMENTAL_SIZE>
{
};

class CQuickBytes : public CQuickBytesBase
{
public:
    CQuickBytes()
    {
        Init();
    }

    ~CQuickBytes()
    {
        Destroy();
    }
};

/* to be used as static variable - no constructor/destructor, assumes zero 
   initialized memory */
class CQuickBytesStatic : public CQuickBytesBase
{
};

template <DWORD CQUICKBYTES_BASE_SPECIFY_SIZE> 
class CQuickBytesSpecifySizeBase : public CQuickMemoryBase<CQUICKBYTES_BASE_SPECIFY_SIZE, CQUICKBYTES_INCREMENTAL_SIZE>
{
};

template <DWORD CQUICKBYTES_BASE_SPECIFY_SIZE> 
class CQuickBytesSpecifySize : public CQuickBytesSpecifySizeBase<CQUICKBYTES_BASE_SPECIFY_SIZE>
{
public:
    CQuickBytesSpecifySize()
    {
        Init();
    }

    ~CQuickBytesSpecifySize()
    {
        Destroy();
    }
};

/* to be used as static variable - no constructor/destructor, assumes zero 
   initialized memory */
template <DWORD CQUICKBYTES_BASE_SPECIFY_SIZE> 
class CQuickBytesSpecifySizeStatic : public CQuickBytesSpecifySizeBase<CQUICKBYTES_BASE_SPECIFY_SIZE>
{
};

template <class T> class CQuickArrayBase : public CQuickBytesBase
{
public:
    T* Alloc(int iItems) 
        { return (T*)CQuickBytesBase::Alloc(iItems * sizeof(T)); }
    HRESULT ReSize(SIZE_T iItems) 
        { return CQuickBytesBase::ReSize(iItems * sizeof(T)); }
    T* Ptr() 
        { return (T*) CQuickBytesBase::Ptr(); }
    SIZE_T Size()
        { return CQuickBytesBase::Size() / sizeof(T); }
    SIZE_T MaxSize()
        { return CQuickBytesBase::cbTotal / sizeof(T); }

    T& operator[] (SIZE_T ix)
    { _ASSERTE(ix < Size());
        return *(Ptr() + ix);
    }
};

template <class T> class CQuickArray : public CQuickArrayBase<T>
{
public:
    CQuickArray<T>()
    { 
        Init();
    }

    ~CQuickArray<T>()
    { 
        Destroy();
    }
};

/* to be used as static variable - no constructor/destructor, assumes zero 
   initialized memory */
template <class T> class CQuickArrayStatic : public CQuickArrayBase<T>
{
};

typedef CQuickArrayBase<WCHAR> CQuickWSTRBase;
typedef CQuickArray<WCHAR> CQuickWSTR;
typedef CQuickArrayStatic<WCHAR> CQuickWSTRStatic;

typedef CQuickArrayBase<CHAR> CQuickSTRBase;
typedef CQuickArray<CHAR> CQuickSTR;
typedef CQuickArrayStatic<CHAR> CQuickSTRStatic;


//*****************************************************************************
//
//***** Signature helpers
//
//*****************************************************************************

inline bool isCallConv(unsigned sigByte, CorCallingConvention conv)
{
    return ((sigByte & IMAGE_CEE_CS_CALLCONV_MASK) == (unsigned) conv); 
}

HRESULT _CountBytesOfOneArg(
    PCCOR_SIGNATURE pbSig, 
    ULONG       *pcbTotal);

HRESULT _GetFixedSigOfVarArg(           // S_OK or error.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob of CLR signature
    ULONG   cbSigBlob,                  // [IN] size of signature
    CQuickBytes *pqbSig,                // [OUT] output buffer for fixed part of VarArg Signature
    ULONG   *pcbSigBlob);               // [OUT] number of bytes written to the above output buffer




//*****************************************************************************
//
//***** File format helper classes
//
//*****************************************************************************



//*****************************************************************************
typedef struct tagCOR_ILMETHOD_SECT_SMALL : IMAGE_COR_ILMETHOD_SECT_SMALL {
        //Data follows  
    const BYTE* Data() const { return(((const BYTE*) this) + sizeof(struct tagCOR_ILMETHOD_SECT_SMALL)); }    
} COR_ILMETHOD_SECT_SMALL;


/************************************/
/* NOTE this structure must be DWORD aligned!! */
typedef struct tagCOR_ILMETHOD_SECT_FAT : IMAGE_COR_ILMETHOD_SECT_FAT {
        //Data follows  
    const BYTE* Data() const { return(((const BYTE*) this) + sizeof(struct tagCOR_ILMETHOD_SECT_FAT)); }  

        //Endian-safe wrappers
    unsigned GetKind() const {
        /* return Kind; */
        return *(BYTE*)this;
    }
    void SetKind(unsigned kind) {
        /* Kind = kind; */
        *(BYTE*)this = (BYTE)kind;
    }

    unsigned GetDataSize() const {
        /* return DataSize; */
        BYTE* p = (BYTE*)this;
        return ((unsigned)*(p+1)) |
            (((unsigned)*(p+2)) << 8) |
            (((unsigned)*(p+3)) << 16);
    }
    void SetDataSize(unsigned datasize) {
        /* DataSize = dataSize; */
        BYTE* p = (BYTE*)this;
        *(p+1) = (BYTE)(datasize);
        *(p+2) = (BYTE)(datasize >> 8);
        *(p+3) = (BYTE)(datasize >> 16);
    }
} COR_ILMETHOD_SECT_FAT;

typedef struct tagCOR_ILMETHOD_SECT_EH_CLAUSE_FAT : public IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT {
    //Endian-safe wrappers
    CorExceptionFlag GetFlags() const {
        return (CorExceptionFlag)VAL32((unsigned)Flags);
    }
    void SetFlags(CorExceptionFlag flags) {
        Flags = (CorExceptionFlag)VAL32((unsigned)flags);
    }

    DWORD GetTryOffset() const {
        return VAL32(TryOffset);
    }
    void SetTryOffset(DWORD Offset) {
        TryOffset = VAL32(Offset);
    }

    DWORD GetTryLength() const {
        return VAL32(TryLength);
    }
    void SetTryLength(DWORD Length) {
        TryLength = VAL32(Length);
    }

    DWORD GetHandlerOffset() const {
        return VAL32(HandlerOffset);
    }
    void SetHandlerOffset(DWORD Offset) {
        HandlerOffset = VAL32(Offset);
    }

    DWORD GetHandlerLength() const {
        return VAL32(HandlerLength);
    }
    void SetHandlerLength(DWORD Length) {
        HandlerLength = VAL32(Length);
    }

    DWORD GetClassToken() const {
        return VAL32(ClassToken);
    }
    void SetClassToken(DWORD tok) {
        ClassToken = VAL32(tok);
    }

    DWORD GetFilterOffset() const {
        return VAL32(FilterOffset);
    }
    void SetFilterOffset(DWORD offset) {
        FilterOffset = VAL32(offset);
    }

} COR_ILMETHOD_SECT_EH_CLAUSE_FAT;

//*****************************************************************************
struct COR_ILMETHOD_SECT_EH_FAT : public COR_ILMETHOD_SECT_FAT {
    static unsigned Size(unsigned ehCount) {    
        return (sizeof(COR_ILMETHOD_SECT_EH_FAT) +  
                sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT) * (ehCount-1)); 
        }   
                    
    COR_ILMETHOD_SECT_EH_CLAUSE_FAT Clauses[1];     // actually variable size   
};

typedef struct tagCOR_ILMETHOD_SECT_EH_CLAUSE_SMALL : public IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL {
    //Endian-safe wrappers
    CorExceptionFlag GetFlags() const {
        return (CorExceptionFlag)VAL16((SHORT)Flags);
    }
    void SetFlags(CorExceptionFlag flags) {
        Flags = (CorExceptionFlag)VAL16((SHORT)flags);
    }

    DWORD GetTryOffset() const {
        return VAL16(TryOffset);
    }
    void SetTryOffset(DWORD Offset) {
        TryOffset = VAL16(Offset);
    }

    DWORD GetTryLength() const {
        return TryLength;
    }
    void SetTryLength(DWORD Length) {
        TryLength = Length;
    }

    DWORD GetHandlerOffset() const {
        return VAL16(HandlerOffset);
    }
    void SetHandlerOffset(DWORD Offset) {
        HandlerOffset = VAL16(Offset);
    }

    DWORD GetHandlerLength() const {
        return HandlerLength;
    }
    void SetHandlerLength(DWORD Length) {
        HandlerLength = Length;
    }

    DWORD GetClassToken() const {
        return VAL32(ClassToken);
    }
    void SetClassToken(DWORD tok) {
        ClassToken = VAL32(tok);
    }

    DWORD GetFilterOffset() const {
        return VAL32(FilterOffset);
    }
    void SetFilterOffset(DWORD offset) {
        FilterOffset = VAL32(offset);
    }
} COR_ILMETHOD_SECT_EH_CLAUSE_SMALL;

//*****************************************************************************
struct COR_ILMETHOD_SECT_EH_SMALL : public COR_ILMETHOD_SECT_SMALL {
    static unsigned Size(unsigned ehCount) {    
        return (sizeof(COR_ILMETHOD_SECT_EH_SMALL) +    
                sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_SMALL) * (ehCount-1));   
        }   
                    
    WORD Reserved;                                  // alignment padding    
    COR_ILMETHOD_SECT_EH_CLAUSE_SMALL Clauses[1];   // actually variable size   
};


/************************************/
/* NOTE this structure must be DWORD aligned!! */
struct COR_ILMETHOD_SECT 
{
    bool More() const           { return((AsSmall()->Kind & CorILMethod_Sect_MoreSects) != 0); }    
    CorILMethodSect Kind() const{ return((CorILMethodSect) (AsSmall()->Kind & CorILMethod_Sect_KindMask)); }    
    const COR_ILMETHOD_SECT* Next() const   {   
        if (!More()) return(0); 
        if (IsFat()) return(((COR_ILMETHOD_SECT*) &AsFat()->Data()[AsFat()->GetDataSize()])->Align());    
        return(((COR_ILMETHOD_SECT*) &AsSmall()->Data()[AsSmall()->DataSize])->Align()); 
        }   
    const COR_ILMETHOD_SECT* NextLoc() const   {   
        if (IsFat()) return(((COR_ILMETHOD_SECT*) &AsFat()->Data()[AsFat()->GetDataSize()])->Align());    
        return(((COR_ILMETHOD_SECT*) &AsSmall()->Data()[AsSmall()->DataSize])->Align()); 
        }   
    const BYTE* Data() const {  
        if (IsFat()) return(AsFat()->Data());   
        return(AsSmall()->Data());  
        }   
    unsigned DataSize() const { 
        if (IsFat()) return(AsFat()->GetDataSize()); 
        return(AsSmall()->DataSize);    
        }   

    friend struct COR_ILMETHOD; 
    friend struct tagCOR_ILMETHOD_FAT; 
    friend struct tagCOR_ILMETHOD_TINY;    
    bool IsFat() const                            { return((AsSmall()->Kind & CorILMethod_Sect_FatFormat) != 0); }  
    const COR_ILMETHOD_SECT* Align() const        { return((COR_ILMETHOD_SECT*) ((((UINT_PTR) this) + 3) & ~3));  } 
protected:
    const COR_ILMETHOD_SECT_FAT*   AsFat() const  { return((COR_ILMETHOD_SECT_FAT*) this); }    
    const COR_ILMETHOD_SECT_SMALL* AsSmall() const{ return((COR_ILMETHOD_SECT_SMALL*) this); }  

public:
    // The body is either a COR_ILMETHOD_SECT_SMALL or COR_ILMETHOD_SECT_FAT    
    // (as indicated by the CorILMethod_Sect_FatFormat bit  
    union { 
        COR_ILMETHOD_SECT_EH_SMALL Small;   
        COR_ILMETHOD_SECT_EH_FAT Fat;   
        };  
};


/***********************************/
// exported functions (implementation in Format\Format.cpp:
extern "C" {
COR_ILMETHOD_SECT_EH_CLAUSE_FAT* __stdcall SectEH_EHClause(void *pSectEH, unsigned idx, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* buff);
        // compute the size of the section (best format)    
        // codeSize is the size of the method   
    // deprecated
unsigned __stdcall SectEH_SizeWithCode(unsigned ehCount, unsigned codeSize);  

    // will return worse-case size and then Emit will return actual size
unsigned __stdcall SectEH_SizeWorst(unsigned ehCount);  

    // will return exact size which will match the size returned by Emit
unsigned __stdcall SectEH_SizeExact(unsigned ehCount, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses);  

        // emit the section (best format);  
unsigned __stdcall SectEH_Emit(unsigned size, unsigned ehCount,   
                  COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses,
                  BOOL moreSections, BYTE* outBuff,
                  ULONG* ehTypeOffsets = 0);
} // extern "C"


struct COR_ILMETHOD_SECT_EH : public COR_ILMETHOD_SECT
{
    unsigned EHCount() const {  
        return (unsigned)(IsFat() ? (Fat.GetDataSize() / sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT)) : 
                        (Small.DataSize / sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_SMALL))); 
    }   

        // return one clause in its fat form.  Use 'buff' if needed 
    const COR_ILMETHOD_SECT_EH_CLAUSE_FAT* EHClause(unsigned idx, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* buff) const
    { return SectEH_EHClause((void *)this, idx, buff); };
        // compute the size of the section (best format)    
        // codeSize is the size of the method   
    // deprecated
    unsigned static Size(unsigned ehCount, unsigned codeSize)
    { return SectEH_SizeWithCode(ehCount, codeSize); };

    // will return worse-case size and then Emit will return actual size
    unsigned static Size(unsigned ehCount)
    { return SectEH_SizeWorst(ehCount); };

    // will return exact size which will match the size returned by Emit
    unsigned static Size(unsigned ehCount, const COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses)
    { return SectEH_SizeExact(ehCount, (COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)clauses);  };

        // emit the section (best format);  
    unsigned static Emit(unsigned size, unsigned ehCount,   
                  const COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses,   
                  bool moreSections, BYTE* outBuff,
                  ULONG* ehTypeOffsets = 0)
    { return SectEH_Emit(size, ehCount,
                         (COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)clauses,
                         moreSections, outBuff, ehTypeOffsets); };
};


/***************************************************************************/
/* Used when the method is tiny (< 64 bytes), and there are no local vars */
typedef struct tagCOR_ILMETHOD_TINY : IMAGE_COR_ILMETHOD_TINY
{
    bool     IsTiny() const         { return((Flags_CodeSize & (CorILMethod_FormatMask >> 1)) == CorILMethod_TinyFormat); } 
    unsigned GetCodeSize() const    { return(((unsigned) Flags_CodeSize) >> (CorILMethod_FormatShift-1)); } 
    unsigned GetMaxStack() const    { return(8); }  
    BYTE*    GetCode() const        { return(((BYTE*) this) + sizeof(struct tagCOR_ILMETHOD_TINY)); } 
    DWORD    GetLocalVarSigTok() const  { return(0); }  
    COR_ILMETHOD_SECT* GetSect() const { return(0); }   
} COR_ILMETHOD_TINY;


/************************************/
// This strucuture is the 'fat' layout, where no compression is attempted. 
// Note that this structure can be added on at the end, thus making it extensible
typedef struct tagCOR_ILMETHOD_FAT : IMAGE_COR_ILMETHOD_FAT
{
        //Endian-safe wrappers
    unsigned GetSize() const {
        /* return Size; */
        BYTE* p = (BYTE*)this;
        return *(p+1) >> 4;
    }
    void SetSize(unsigned size) {
        /* Size = size; */
        BYTE* p = (BYTE*)this;
        *(p+1) = (BYTE)((*(p+1) & 0x0F) | (size << 4));
    }

    unsigned GetFlags() const {
        /* return Flags; */
        BYTE* p = (BYTE*)this;
        return ((unsigned)*(p+0)) | (( ((unsigned)*(p+1)) << 8) & 0x0F);
    }
    void SetFlags(unsigned flags) {
        /* flags = Flags; */
        BYTE* p = (BYTE*)this;
        *p = (BYTE)flags;
        *(p+1) = (BYTE)((*(p+1) & 0xF0) | ((flags >> 8) & 0x0F));
    }

    bool IsFat() const {
        /* return((IMAGE_COR_ILMETHOD_FAT::GetFlags() & CorILMethod_FormatMask) == CorILMethod_FatFormat); */
        return (*(BYTE*)this & CorILMethod_FormatMask) == CorILMethod_FatFormat;
    }

    unsigned GetMaxStack() const {
        /* return MaxStack; */
        return VAL16(*(UINT16*)((BYTE*)this+2));
    }
    void SetMaxStack(unsigned maxStack) {
        /* MaxStack = maxStack; */
        *(UINT16*)((BYTE*)this+2) = VAL16((UINT16)maxStack);
    }

    unsigned GetCodeSize() const        { return VAL32(CodeSize); }
    void SetCodeSize(DWORD Size)        { CodeSize = VAL32(Size); }

    mdToken  GetLocalVarSigTok() const      { return VAL32(LocalVarSigTok); } 
    void SetLocalVarSigTok(mdSignature tok) { LocalVarSigTok = VAL32(tok); }

    BYTE* GetCode() const {
        return(((BYTE*) this) + 4*GetSize());
    }

    const COR_ILMETHOD_SECT* GetSect() const {
        /* if (!(GetFlags() & CorILMethod_MoreSects)) return(0); */
        if (!(*(BYTE*)this & CorILMethod_MoreSects)) return (0);
        return(((COR_ILMETHOD_SECT*) (GetCode() + GetCodeSize()))->Align());
    }
} COR_ILMETHOD_FAT;


extern "C" {
/************************************/
// exported functions (impl. Format\Format.cpp)
unsigned __stdcall IlmethodSize(COR_ILMETHOD_FAT* header, BOOL MoreSections);    
        // emit the header (bestFormat) return amount emitted   
unsigned __stdcall IlmethodEmit(unsigned size, COR_ILMETHOD_FAT* header, 
                  BOOL moreSections, BYTE* outBuff);    
}

struct COR_ILMETHOD
{
        // a COR_ILMETHOD header should not be decoded by hand.  Instead us 
        // COR_ILMETHOD_DECODER to decode it.   
    friend class COR_ILMETHOD_DECODER;  

        // compute the size of the header (best format) 
    unsigned static Size(const COR_ILMETHOD_FAT* header, bool MoreSections)
    { return IlmethodSize((COR_ILMETHOD_FAT*)header,MoreSections); };
        // emit the header (bestFormat) return amount emitted   
    unsigned static Emit(unsigned size, const COR_ILMETHOD_FAT* header, 
                  bool moreSections, BYTE* outBuff)
    { return IlmethodEmit(size, (COR_ILMETHOD_FAT*)header, moreSections, outBuff); };

//private:
    union   
    {   
        COR_ILMETHOD_TINY       Tiny;   
        COR_ILMETHOD_FAT        Fat;    
    };  
        // Code follows the Header, then immedately after the code comes    
        // any sections (COR_ILMETHOD_SECT).    
};

extern "C" {
/***************************************************************************/
/* COR_ILMETHOD_DECODER is the only way functions internal to the EE should
   fetch data from a COR_ILMETHOD.  This way any dependancy on the file format
   (and the multiple ways of encoding the header) is centralized to the 
   COR_ILMETHOD_DECODER constructor) */
    void __stdcall DecoderInit(void * pThis, COR_ILMETHOD* header);
    int  __stdcall DecoderGetOnDiskSize(void * pThis, COR_ILMETHOD* header);
} // extern "C"

class COR_ILMETHOD_DECODER : public COR_ILMETHOD_FAT
{
public:
        // Typically the ONLY way you should access COR_ILMETHOD is through
		// this constructor so format changes are easier.
    COR_ILMETHOD_DECODER(const COR_ILMETHOD* header) { DecoderInit(this,(COR_ILMETHOD*)header); };

        // The above variant of the constructor can not do a 'complete' job, because
        // it can not look up the local variable signature meta-data token.
        // This method should be used when you have access to the Meta data API
        // If the consturction fails, the 'Code' field is set to
    COR_ILMETHOD_DECODER(COR_ILMETHOD* header, void *pInternalImport = NULL,
	                                            bool verify = false);

    unsigned EHCount() const {  
        if (EH == 0) return(0); 
        else return(EH->EHCount()); 
        }   

    // returns total size of method for use in copying
    int GetOnDiskSize(const COR_ILMETHOD* header) { return DecoderGetOnDiskSize(this,(COR_ILMETHOD*)header); };

    // Flags        these are available because we inherit COR_ILMETHOD_FAT 
    // MaxStack 
    // CodeSize 
    const BYTE* Code;   
    PCCOR_SIGNATURE LocalVarSig;        // pointer to signature blob, or 0 if none  
    const COR_ILMETHOD_SECT_EH* EH;     // eh table if any  0 if none   
    const COR_ILMETHOD_SECT* Sect;      // additional sections  0 if none   
};

STDAPI_(void)   ReleaseFusionInterfaces();

BOOL STDMETHODCALLTYPE   BeforeFusionShutdown();
void STDMETHODCALLTYPE   DontReleaseFusionInterfaces();

#endif // __CORHLPR_H__

