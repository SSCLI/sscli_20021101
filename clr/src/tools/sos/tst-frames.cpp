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

#ifndef PLATFORM_UNIX
#pragma warning(disable:4100 4245)

#include <wchar.h>
#include <windows.h>
#else
#include "gdbwrap.h"
#endif

#include "dbgeng.h"
#include "util.h"
#include "strike.h"
#include "eestructs.h"
#include "dump-tables.h"
#include "tst-frames.h"
#include "get-table-info.h"
#include "tst-siginfo.h"

extern Frame *g_pFrameNukeList;

Frame *ResolveFrame(DWORD_PTR prFrame)
{
    if ((Frame *)prFrame == FRAME_TOP)
        return ((Frame *)FRAME_TOP);

    ClassDumpTable *pTable = GetClassDumpTable();

    if (pTable == NULL)
        return (FALSE);

    DWORD_PTR prVtable;
    safemove(prVtable, prFrame);

    Frame *pFrame;

    if (prVtable == NULL)
        return (FALSE);

#include <clear-class-dump-defs.h>

#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent)                    \
    else if (prVtable == pTable->p ## klass ## Vtable)                  \
    {                                                                   \
        klass *pNewFrame = new klass();                                 \
        pNewFrame->m_This = (Frame *)prFrame;                           \
        pNewFrame->Fill(prFrame);                                       \
        pFrame = (Frame *) pNewFrame;                                   \
        pFrame->m_pNukeNext = g_pFrameNukeList;                         \
        g_pFrameNukeList = pFrame;                                      \
    }
    
#include <frame-types.h>
#include <clear-class-dump-defs.h>

    else
    {
        pFrame = NULL;
    }

    return pFrame;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
CalleeSavedRegisters *FramedMethodFrame::GetCalleeSavedRegisters()
{
    safemove(m_savedRegs, (((BYTE*)m_vLoadAddr) - sizeof(CalleeSavedRegisters)));
    return &m_savedRegs;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
LPVOID *
FramedMethodFrame::GetReturnAddressPtr()
{
#if defined(_PPC_)
    return (LPVOID*)&(m_Link.SavedLR);
#else
    return (&m_ReturnAddress);
#endif
}

void FramedMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    MethodDesc *pFunc = GetFunction();

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    pRD->pPC  = (SLOT*) GetReturnAddressPtr();

#ifdef _X86_
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    pRD->pEdi = (DWORD*) &regs->edi;
    pRD->pEsi = (DWORD*) &regs->esi;
    pRD->pEbx = (DWORD*) &regs->ebx;
    pRD->pEbp = (DWORD*) &regs->ebp;
    pRD->SP  = (DWORD)((size_t)pRD->pPC + sizeof(void*));
#elif defined(_PPC_)
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    int i;
    for (i=0; i<NUM_CALLEESAVED_REGISTERS; ++i) {
        pRD->pR[i] = (DWORD*)&regs->r[i];
    }
    pRD->SP = m_Link.SavedSP;
#else
    PORTABILITY_ASSERT("FramedMethodFrame::UpdateRegDisplay is not implemented on this platform.");
#endif



    if (pFunc)
    {
        DWORD_PTR pdw = (DWORD_PTR) pFunc;

        MethodDesc vMD;
        vMD.Fill(pdw);

        pdw = (DWORD_PTR) pFunc;
        DWORD_PTR pMT;
        GetMethodTable((DWORD_PTR) pFunc, pMT);

        MethodTable vMT;
        vMT.Fill(pMT);

        DWORD_PTR pModule = (DWORD_PTR) vMT.m_pModule;
        Module vModule;
        vModule.Fill(pModule);

        WCHAR StringData[MAX_PATH+1];
        FileNameForModule(&vModule, StringData);
        if (StringData[0] == 0)
            return;

        IMetaDataImport *pImport = MDImportForModule (StringData);

        // Now get the signature
        mdTypeDef mdClass;
        DWORD dwAttr;
        PCCOR_SIGNATURE pvSigBlob;
        ULONG cbSigBlob;
        DWORD dwImplFlags;
        pImport->GetMethodProps(
            vMD.m_dwToken, &mdClass, NULL, 0, NULL, &dwAttr, &pvSigBlob, &cbSigBlob, NULL, &dwImplFlags);

        UINT cbStackPop = MetaSig::CbStackPop(0, pvSigBlob, dwImplFlags & mdStatic);

        pRD->SP += cbStackPop;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
void UnmanagedToManagedFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
#ifdef _X86_

    DWORD *savedRegs = (DWORD *)((size_t)this - (sizeof(CalleeSavedRegisters)));

    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;

    pRD->pEdi = savedRegs++;
    pRD->pEsi = savedRegs++;
    pRD->pEbx = savedRegs++;
    pRD->pEbp = savedRegs++;
    pRD->pPC  = (SLOT*)GetReturnAddressPtr();
    pRD->SP  = (DWORD)(size_t)pRD->pPC + sizeof(void*);

    pRD->SP += (DWORD) GetNumCallerStackBytes();

#elif defined(_PPC_)
    CalleeSavedRegisters *regs = (CalleeSavedRegisters*)((BYTE*)this - sizeof(CalleeSavedRegisters));
    int i;

    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;

    pRD->CR  = regs->cr;

    for (i = 0; i < NUM_CALLEESAVED_REGISTERS; i++) {
        pRD->pR[i] = (DWORD*) &(regs->r[i]);
    }

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        pRD->pF[i] = (DOUBLE*) &(regs->f[i]);
    }

    pRD->pPC  = (SLOT*)GetReturnAddressPtr();
    pRD->SP   = (DWORD)(size_t)(&m_Link);
#else
    PORTABILITY_ASSERT("UnmanagedToManagedFrame::UpdateRegDisplay is not implemented on this platform.");

#endif
}






/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
void FaultingExceptionFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    pRD->pPC = (SLOT*)GetReturnAddressPtr();

#ifdef _X86_
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    pRD->pEdi = (DWORD*) &regs->edi;
    pRD->pEsi = (DWORD*) &regs->esi;
    pRD->pEbx = (DWORD*) &regs->ebx;
    pRD->pEbp = (DWORD*) &regs->ebp;
    pRD->SP = m_Esp;
#elif defined(_PPC_)
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    int i;

    for (i = 0; i < NUM_CALLEESAVED_REGISTERS; i++) {
        pRD->pR[i] = (DWORD*) &(regs->r[i]);
    }

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        pRD->pF[i] = (DOUBLE*) &(regs->f[i]);
    }
    pRD->SP = m_Link.SavedSP;
#else   // _X86_
    PORTABILITY_ASSERT("FaultingExceptionFrame::UpdateRegDisplay is not implemented on this platform.");
#endif  // _X86_
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
void FuncEvalFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
LPVOID* FuncEvalFrame::GetReturnAddressPtr(void)
{
    return (NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
unsigned int FuncEvalFrame::GetFrameAttribs(void)
{
    return (0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
void HelperMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
Frame *Frame::Next()
{
    return (ResolveFrame((DWORD_PTR)m_Next));
}
