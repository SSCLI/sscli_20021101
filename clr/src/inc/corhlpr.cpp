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
/****************************************************************************
 **                                                                        **
 ** Corhlpr.h - signature helpers.                                         **
 **                                                                        **
 ****************************************************************************/

#include "corhlpr.h"
#include <stdlib.h>

/*************************************************************************************
*
* get number of bytes consumed by one argument/return type
*
*************************************************************************************/

HRESULT _CountBytesOfOneArg(
    PCCOR_SIGNATURE pbSig, 
    ULONG       *pcbTotal)
{
    ULONG       cb;
    ULONG       cbTotal;
    CorElementType ulElementType;
    ULONG       ulData;
    ULONG       ulTemp;
    int         iData;
    mdToken     tk;
    ULONG       cArg;
    ULONG       callingconv;
    ULONG       cArgsIndex;
    HRESULT     hr = NOERROR;

    _ASSERTE(pcbTotal);

    cbTotal = CorSigUncompressElementType(pbSig, &ulElementType);
    while (CorIsModifierElementType((CorElementType) ulElementType))
    {
        cbTotal += CorSigUncompressElementType(&pbSig[cbTotal], &ulElementType);
    }
    switch (ulElementType)
    {
        case ELEMENT_TYPE_SZARRAY:
		case 0x1e /* obsolete */:
            // skip over base type
            IfFailGo( _CountBytesOfOneArg(&pbSig[cbTotal], &cb) );
            cbTotal += cb;
            break;

        case ELEMENT_TYPE_FNPTR:
            cbTotal += CorSigUncompressData (&pbSig[cbTotal], &callingconv);

            // remember number of bytes to represent the arg counts
            cbTotal += CorSigUncompressData (&pbSig[cbTotal], &cArg);

            // how many bytes to represent the return type
            IfFailGo( _CountBytesOfOneArg( &pbSig[cbTotal], &cb) );
            cbTotal += cb;
    
            // loop through argument
            for (cArgsIndex = 0; cArgsIndex < cArg; cArgsIndex++)
            {
                IfFailGo( _CountBytesOfOneArg( &pbSig[cbTotal], &cb) );
                cbTotal += cb;
            }

            break;

        case ELEMENT_TYPE_ARRAY:
            // syntax : ARRAY BaseType <rank> [i size_1... size_i] [j lowerbound_1 ... lowerbound_j]

            // skip over base type
            IfFailGo( _CountBytesOfOneArg(&pbSig[cbTotal], &cb) );
            cbTotal += cb;

            // Parse for the rank
            cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulData);

            // if rank == 0, we are done
            if (ulData == 0)
                break;

            // any size of dimension specified?
            cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulData);
            while (ulData--)
            {
                cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulTemp);
            }

            // any lower bound specified?
            cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulData);

            while (ulData--)
            {
                cbTotal += CorSigUncompressSignedInt(&pbSig[cbTotal], &iData);
            }

            break;
        case ELEMENT_TYPE_VALUETYPE:
        case ELEMENT_TYPE_CLASS:
		case ELEMENT_TYPE_CMOD_REQD:
		case ELEMENT_TYPE_CMOD_OPT:
            // count the bytes for the token compression
            cbTotal += CorSigUncompressToken(&pbSig[cbTotal], &tk);
            if ( ulElementType == ELEMENT_TYPE_CMOD_REQD ||
		         ulElementType == ELEMENT_TYPE_CMOD_OPT)
            {
                // skip over base type
                IfFailGo( _CountBytesOfOneArg(&pbSig[cbTotal], &cb) );
                cbTotal += cb;
            }
            break;
        default:
            break;
    }

    *pcbTotal = cbTotal;
ErrExit:
    return hr;
}


//*****************************************************************************
// copy fixed part of VarArg signature to a buffer
//*****************************************************************************
HRESULT _GetFixedSigOfVarArg(           // S_OK or error.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob of COM+ method signature
    ULONG   cbSigBlob,                  // [IN] size of signature
    CQuickBytes *pqbSig,                // [OUT] output buffer for fixed part of VarArg Signature
    ULONG   *pcbSigBlob)                // [OUT] number of bytes written to the above output buffer
{
    HRESULT     hr = NOERROR;
    ULONG       cbCalling;
    ULONG       cbArgsNumber;           // number of bytes to store the original arg count
    ULONG       cbArgsNumberTemp;       // number of bytes to store the fixed arg count
    ULONG       cbTotal = 0;            // total of number bytes for return type + all fixed arguments
    ULONG       cbCur = 0;              // index through the pvSigBlob
    ULONG       cb;
    ULONG       cArg;
    ULONG       callingconv;
    ULONG       cArgsIndex;
    CorElementType ulElementType;
    BYTE        *pbSig;

    _ASSERTE (pvSigBlob && pcbSigBlob);

    // remember the number of bytes to represent the calling convention
    cbCalling = CorSigUncompressData (pvSigBlob, &callingconv);
    _ASSERTE (isCallConv(callingconv, IMAGE_CEE_CS_CALLCONV_VARARG));
    cbCur += cbCalling;

    // remember number of bytes to represent the arg counts
    cbArgsNumber= CorSigUncompressData (&pvSigBlob[cbCur], &cArg);
    cbCur += cbArgsNumber;

    // how many bytes to represent the return type
    IfFailGo( _CountBytesOfOneArg( &pvSigBlob[cbCur], &cb) );
    cbCur += cb;
    cbTotal += cb;
    
    // loop through argument until we found ELEMENT_TYPE_SENTINEL or run
    // out of arguments
    for (cArgsIndex = 0; cArgsIndex < cArg; cArgsIndex++)
    {
        _ASSERTE(cbCur < cbSigBlob);

        // peak the outter most ELEMENT_TYPE_*
        CorSigUncompressElementType (&pvSigBlob[cbCur], &ulElementType);
        if (ulElementType == ELEMENT_TYPE_SENTINEL)
            break;
        IfFailGo( _CountBytesOfOneArg( &pvSigBlob[cbCur], &cb) );
        cbTotal += cb;
        cbCur += cb;
    }

    cbArgsNumberTemp = CorSigCompressData(cArgsIndex, &cArg);

    // now cbCalling : the number of bytes needed to store the calling convention
    // cbArgNumberTemp : number of bytes to store the fixed arg count
    // cbTotal : the number of bytes to store the ret and fixed arguments

    *pcbSigBlob = cbCalling + cbArgsNumberTemp + cbTotal;

    // resize the buffer
    IfFailGo( pqbSig->ReSize(*pcbSigBlob) );
    pbSig = (BYTE *)pqbSig->Ptr();

    // copy over the calling convention
    cb = CorSigCompressData(callingconv, pbSig);

    // copy over the fixed arg count
    cbArgsNumberTemp = CorSigCompressData(cArgsIndex, &pbSig[cb]);

    // copy over the fixed args + ret type
    memcpy(&pbSig[cb + cbArgsNumberTemp], &pvSigBlob[cbCalling + cbArgsNumber], cbTotal);

ErrExit:
    return hr;
}





//*****************************************************************************
//
//***** File format helper classes
//
//*****************************************************************************

extern "C" {

/***************************************************************************/
/* Note that this construtor does not set the LocalSig, but has the
   advantage that it does not have any dependancy on EE structures.
   inside the EE use the FunctionDesc constructor */

void __stdcall DecoderInit(void * pThis, COR_ILMETHOD* header) 
{
    memset(pThis, 0, sizeof(COR_ILMETHOD_DECODER));
    if (header->Tiny.IsTiny()) {
        ((COR_ILMETHOD_DECODER*)pThis)->SetMaxStack(header->Tiny.GetMaxStack());
        ((COR_ILMETHOD_DECODER*)pThis)->Code = header->Tiny.GetCode();
        ((COR_ILMETHOD_DECODER*)pThis)->SetCodeSize(header->Tiny.GetCodeSize());
        ((COR_ILMETHOD_DECODER*)pThis)->SetFlags(CorILMethod_TinyFormat);
        return;
    }
    if (header->Fat.IsFat()) {
        _ASSERTE((((size_t) header) & 3) == 0);        // header is aligned
        *((COR_ILMETHOD_FAT*) pThis) = header->Fat;
        ((COR_ILMETHOD_DECODER*)pThis)->Code = header->Fat.GetCode();
        _ASSERTE(header->Fat.GetSize() >= 3);        // Size if valid
        ((COR_ILMETHOD_DECODER*)pThis)->Sect = header->Fat.GetSect();
        if (((COR_ILMETHOD_DECODER*)pThis)->Sect != 0 && ((COR_ILMETHOD_DECODER*)pThis)->Sect->Kind() == CorILMethod_Sect_EHTable) {
            ((COR_ILMETHOD_DECODER*)pThis)->EH = (COR_ILMETHOD_SECT_EH*) ((COR_ILMETHOD_DECODER*)pThis)->Sect;
            ((COR_ILMETHOD_DECODER*)pThis)->Sect = ((COR_ILMETHOD_DECODER*)pThis)->Sect->Next();
        }
        return;
    }
    // so we don't asert on trash  _ASSERTE(!"Unknown format");
}

// Calculate the total method size. First get address of end of code. If there are no sections, then
// the end of code addr marks end of COR_ILMETHOD. Otherwise find addr of end of last section and use it
// to mark end of COR_ILMETHD. Assumes that the code is directly followed
// by each section in the on-disk format
int __stdcall DecoderGetOnDiskSize(void * pThis, COR_ILMETHOD* header)
{
    BYTE *lastAddr = (BYTE*)((COR_ILMETHOD_DECODER*)pThis)->Code + ((COR_ILMETHOD_DECODER*)pThis)->GetCodeSize();    // addr of end of code
    const COR_ILMETHOD_SECT *sect = ((COR_ILMETHOD_DECODER*)pThis)->EH;
	if (sect != 0 && sect->Next() == 0)
		lastAddr = (BYTE *)(&sect->Data()[sect->DataSize()]);
	else
	{
		const COR_ILMETHOD_SECT *nextSect;
		for (sect = ((COR_ILMETHOD_DECODER*)pThis)->Sect; 
			 sect; sect = nextSect) {
			nextSect = sect->Next();
			if (nextSect == 0) {
				// sect points to the last section, so set lastAddr
				lastAddr = (BYTE *)(&sect->Data()[sect->DataSize()]);
				break;
			}
		}
    }
    return (int)(lastAddr - (BYTE*)header);
}

/*********************************************************************/
/* APIs for emitting sections etc */

unsigned __stdcall IlmethodSize(COR_ILMETHOD_FAT* header, BOOL moreSections)
{
    if (header->GetMaxStack() <= 8 && (header->GetFlags() & ~CorILMethod_FormatMask) == 0
        && header->GetLocalVarSigTok() == 0 && header->GetCodeSize() < 64 && !moreSections)
        return(sizeof(COR_ILMETHOD_TINY));

    return(sizeof(COR_ILMETHOD_FAT));
}

/*********************************************************************/
        // emit the header (bestFormat) return amount emitted   
unsigned __stdcall IlmethodEmit(unsigned size, COR_ILMETHOD_FAT* header, 
                  BOOL moreSections, BYTE* outBuff)
{
#ifdef _DEBUG
    BYTE* origBuff = outBuff;
#endif
    if (size == 1) {
            // Tiny format
        *outBuff++ = (BYTE) (CorILMethod_TinyFormat | (header->GetCodeSize() << 2));
    }
    else {
            // Fat format
        _ASSERTE((((size_t) outBuff) & 3) == 0);               // header is dword aligned
        COR_ILMETHOD_FAT* fatHeader = (COR_ILMETHOD_FAT*) outBuff;
        outBuff += sizeof(COR_ILMETHOD_FAT);
        *fatHeader = *header;
        fatHeader->SetFlags(fatHeader->GetFlags() | CorILMethod_FatFormat);
        _ASSERTE((fatHeader->GetFlags() & CorILMethod_FormatMask) == CorILMethod_FatFormat);
        if (moreSections)
            fatHeader->SetFlags(fatHeader->GetFlags() | CorILMethod_MoreSects);
        fatHeader->SetSize(sizeof(COR_ILMETHOD_FAT) / 4);
    }
    _ASSERTE(&origBuff[size] == outBuff);
    return(size);
}

/*********************************************************************/
/* static */
COR_ILMETHOD_SECT_EH_CLAUSE_FAT* __stdcall SectEH_EHClause(void *pSectEH, unsigned idx, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* buff)
{    
    if (((COR_ILMETHOD_SECT_EH *)pSectEH)->IsFat()) 
        return(&(((COR_ILMETHOD_SECT_EH *)pSectEH)->Fat.Clauses[idx])); 

    // mask to remove sign extension - cast just wouldn't work 
    buff->SetFlags((CorExceptionFlag)((((COR_ILMETHOD_SECT_EH *)pSectEH)->Small.Clauses[idx].GetFlags())&0x0000ffff)); 
    buff->SetClassToken(((COR_ILMETHOD_SECT_EH *)pSectEH)->Small.Clauses[idx].GetClassToken());
    buff->SetTryOffset(((COR_ILMETHOD_SECT_EH *)pSectEH)->Small.Clauses[idx].GetTryOffset());
    buff->SetTryLength(((COR_ILMETHOD_SECT_EH *)pSectEH)->Small.Clauses[idx].GetTryLength()); 
    buff->SetHandlerLength(((COR_ILMETHOD_SECT_EH *)pSectEH)->Small.Clauses[idx].GetHandlerLength());
    buff->SetHandlerOffset(((COR_ILMETHOD_SECT_EH *)pSectEH)->Small.Clauses[idx].GetHandlerOffset()); 
    return(buff);   
}   
/*********************************************************************/
        // compute the size of the section (best format)    
        // codeSize is the size of the method   
    // deprecated
unsigned __stdcall SectEH_SizeWithCode(unsigned ehCount, unsigned codeSize)
{
    return((ehCount)? SectEH_SizeWorst(ehCount) : 0);
}

    // will return worse-case size and then Emit will return actual size
unsigned __stdcall SectEH_SizeWorst(unsigned ehCount)
{
    return((ehCount)? (COR_ILMETHOD_SECT_EH_FAT::Size(ehCount)) : 0);
}

    // will return exact size which will match the size returned by Emit
unsigned __stdcall SectEH_SizeExact(unsigned ehCount, COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses)
{
    if (ehCount == 0)
        return(0);

    unsigned smallSize = COR_ILMETHOD_SECT_EH_SMALL::Size(ehCount);
    if (smallSize > COR_ILMETHOD_SECT_SMALL_MAX_DATASIZE)
            return(COR_ILMETHOD_SECT_EH_FAT::Size(ehCount));
    for (unsigned i = 0; i < ehCount; i++) {
        if (clauses[i].GetTryOffset() > 0xFFFF ||
                clauses[i].GetTryLength() > 0xFF ||
                clauses[i].GetHandlerOffset() > 0xFFFF ||
                clauses[i].GetHandlerLength() > 0xFF) {
            return(COR_ILMETHOD_SECT_EH_FAT::Size(ehCount));
        }
    }
    return smallSize;
}

/*********************************************************************/

        // emit the section (best format);  
unsigned __stdcall SectEH_Emit(unsigned size, unsigned ehCount,   
                  COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses,   
                  BOOL moreSections, BYTE* outBuff,
                  ULONG* ehTypeOffsets)
{
    if (size == 0)
       return(0);

    _ASSERTE((((size_t) outBuff) & 3) == 0);               // header is dword aligned
    BYTE* origBuff = outBuff;
    if (ehCount <= 0)
        return 0;

    // Initialize the ehTypeOffsets array.
    if (ehTypeOffsets)
    {
        for (unsigned int i = 0; i < ehCount; i++)
            ehTypeOffsets[i] = (ULONG) -1;
    }

    if (COR_ILMETHOD_SECT_EH_SMALL::Size(ehCount) < COR_ILMETHOD_SECT_SMALL_MAX_DATASIZE) {
        COR_ILMETHOD_SECT_EH_SMALL* EHSect = (COR_ILMETHOD_SECT_EH_SMALL*) outBuff;
		unsigned i;
        for (i = 0; i < ehCount; i++) {
            if (clauses[i].GetTryOffset() > 0xFFFF ||
                    clauses[i].GetTryLength() > 0xFF ||
                    clauses[i].GetHandlerOffset() > 0xFFFF ||
                    clauses[i].GetHandlerLength() > 0xFF) {
                break;  // fall through and generate as FAT
            }
            _ASSERTE((clauses[i].GetFlags() & ~0xFFFF) == 0);
            _ASSERTE((clauses[i].GetTryOffset() & ~0xFFFF) == 0);
            _ASSERTE((clauses[i].GetTryLength() & ~0xFF) == 0);
            _ASSERTE((clauses[i].GetHandlerOffset() & ~0xFFFF) == 0);
            _ASSERTE((clauses[i].GetHandlerLength() & ~0xFF) == 0);
            EHSect->Clauses[i].SetFlags((CorExceptionFlag) clauses[i].GetFlags());
            EHSect->Clauses[i].SetTryOffset(clauses[i].GetTryOffset());
            EHSect->Clauses[i].SetTryLength(clauses[i].GetTryLength());
            EHSect->Clauses[i].SetHandlerOffset(clauses[i].GetHandlerOffset());
            EHSect->Clauses[i].SetHandlerLength(clauses[i].GetHandlerLength());
            EHSect->Clauses[i].SetClassToken(clauses[i].GetClassToken());
        }
        if (i >= ehCount) {
            // if actually got through all the clauses and they are small enough
            EHSect->Kind = CorILMethod_Sect_EHTable;     
            if (moreSections)  
                EHSect->Kind |= CorILMethod_Sect_MoreSects;       
            EHSect->DataSize = EHSect->Size(ehCount);    
            EHSect->Reserved = 0;          
            _ASSERTE(EHSect->DataSize == EHSect->Size(ehCount)); // make sure didn't overflow    
            outBuff = (BYTE*) &EHSect->Clauses[ehCount];  
            // Set the offsets for the exception type tokens.
            if (ehTypeOffsets)
            {
                for (unsigned int i = 0; i < ehCount; i++) {
                    if (EHSect->Clauses[i].GetFlags() == COR_ILEXCEPTION_CLAUSE_NONE)
                    {
                        _ASSERTE(! IsNilToken(EHSect->Clauses[i].GetClassToken()));
                        ehTypeOffsets[i] = (ULONG)((BYTE *)&EHSect->Clauses[i].ClassToken - origBuff);
                    }
                }
            }
            return(size);
        }
    }
    // either total size too big or one of constituent elements too big (eg. offset or length)
    COR_ILMETHOD_SECT_EH_FAT* EHSect = (COR_ILMETHOD_SECT_EH_FAT*) outBuff;
    EHSect->SetKind(CorILMethod_Sect_EHTable | CorILMethod_Sect_FatFormat);
    if (moreSections)
        EHSect->SetKind(EHSect->GetKind() | CorILMethod_Sect_MoreSects);

    EHSect->SetDataSize(EHSect->Size(ehCount));
    memcpy(EHSect->Clauses, clauses, ehCount * sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
    outBuff = (BYTE*) &EHSect->Clauses[ehCount];
    _ASSERTE(&origBuff[size] == outBuff);
    // Set the offsets for the exception type tokens.
    if (ehTypeOffsets)
    {
        for (unsigned int i = 0; i < ehCount; i++) {
            if (EHSect->Clauses[i].GetFlags() == COR_ILEXCEPTION_CLAUSE_NONE)
            {
                _ASSERTE(! IsNilToken(EHSect->Clauses[i].GetClassToken()));
                ehTypeOffsets[i] = (ULONG)((BYTE *)&EHSect->Clauses[i].ClassToken - origBuff);
            }
        }
    }
    return(size);
}

} // extern "C"


