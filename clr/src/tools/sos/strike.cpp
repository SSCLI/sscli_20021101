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
* STRIKE.C                                                                  *
*   Routines for the NTSD extension - STRIKE                                *
*                                                                           *
* History:                                                                  *
*                                                                                          *
*                                                                           *
*                                                                           *
\***************************************************************************/

#ifndef PLATFORM_UNIX
#include <wchar.h>
//#include <heap.h>
//#include <ntsdexts.h>
#include <windows.h>
#else
#include <gdbwrap.h>
#endif // PLATFORM_UNIX


#define NOEXTAPI
#define KDEXT_64BIT
#include <wdbgexts.h>
#undef DECLARE_API

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <malloc.h>
#include <stddef.h>

#include "strike.h"

#ifndef PLATFORM_UNIX
#include <dbghelp.h>
#endif

#include "../../inc/corhdr.h"
#include "../../inc/cor.h"

#define  CORHANDLE_MASK 0x1

#include "eestructs.h"

#define DEFINE_EXT_GLOBALS

#include "data.h"
#include "disasm.h"

BOOL CallStatus;
DWORD_PTR EEManager = NULL;
int DebugVersionDll = -1;
BOOL ControlC = FALSE;

IMetaDataDispenserEx *pDisp = NULL;
WCHAR g_mdName[mdNameLen];

#include "util.h"


#ifdef _MSC_VER
#pragma warning(default:4244)
#pragma warning(default:4189)
#endif //_MSC_VER



extern "C"
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#ifndef PLATFORM_UNIX
        CoInitialize(0);
        CoCreateInstance(CLSID_CorMetaDataDispenser, NULL, CLSCTX_INPROC_SERVER, IID_IMetaDataDispenserEx, (void**)&pDisp);
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (lpReserved == 0)
        {
            mdImportSet.Destroy();
        }
	if (pDisp)
            pDisp->Release();
        if (DllPath) {
            delete DllPath;
        }
#ifndef PLATFORM_UNIX
        CoUninitialize();
#endif
    }
    return true;
}


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to get the MethodDesc for a given eip     *  
*                                                                      *
\**********************************************************************/
DECLARE_API (IP2MD)
{
    INIT_API ();
    DWORD_PTR IP = (DWORD_PTR)GetExpression(args);
    if (IP == 0)
    {
        ExtOut("%s is not IP\n", args);
        return Status;
    }
    JitType jitType;
    DWORD_PTR methodDesc;
    DWORD_PTR gcinfoAddr;
    IP2MethodDesc (IP, methodDesc, jitType, gcinfoAddr);
    if (methodDesc)
    {
        ExtOut("MethodDesc: 0x%p\n", (DWORD64)methodDesc);
        if (jitType == EJIT)
            ExtOut ("Jitted by EJIT\n");
        else if (jitType == JIT)
            ExtOut ("Jitted by normal JIT\n");
        else if (jitType == PJIT)
            ExtOut ("Jitted by PreJIT\n");
        DumpMDInfo (methodDesc);
    }
    else
    {
        ExtOut("%p not in jit code range\n", (ULONG64)IP);
    }
    return Status;
}

#ifndef PLATFORM_UNIX

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function displays the stack trace.  It looks at each DWORD   *  
*    on stack.  If the DWORD is a return address, the symbol name or
*    managed function name is displayed.                               *
*                                                                      *
\**********************************************************************/
void DumpStackInternal(PCSTR args)
{
    DumpStackFlag DSFlag;
    BOOL bSmart = FALSE;
    DSFlag.fEEonly = FALSE;
    DSFlag.top = 0;
    DSFlag.end = 0;

    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-EE", &DSFlag.fEEonly, COBOOL, FALSE},
        {"-smart", &bSmart, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&DSFlag.top, COHEX},
        {&DSFlag.end, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return;
    }
    
    ReloadSymbolWithLineInfo();
    
    ULONG64 StackOffset;
    g_ExtRegisters->GetStackOffset (&StackOffset);
    if (nArg == 0) {
        DSFlag.top = (DWORD_PTR)StackOffset;
    }
    size_t value;
    while (g_ExtData->ReadVirtual(DSFlag.top,&value,sizeof(size_t),NULL) != S_OK) {
        if (IsInterrupt())
            return;
        DSFlag.top = NextOSPageAddress (DSFlag.top);
    }
    
    if (nArg < 2) {
        // Find the current stack range
        NT_TIB teb;
        ULONG64 dwTebAddr=0;

        g_ExtSystem->GetCurrentThreadTeb (&dwTebAddr);
        if (SafeReadMemory ((ULONG_PTR)dwTebAddr, &teb, sizeof (NT_TIB), NULL))
        {
            if (DSFlag.top > (DWORD_PTR)teb.StackLimit
            && DSFlag.top <= (DWORD_PTR)teb.StackBase)
            {
                if (DSFlag.end == 0 || DSFlag.end > (DWORD_PTR)teb.StackBase)
                    DSFlag.end = (DWORD_PTR)teb.StackBase;
            }
        }
    }

    
    if (DSFlag.end == 0)
        DSFlag.end = DSFlag.top + 0xFFFF;
    
    if (DSFlag.end < DSFlag.top)
    {
        ExtOut ("Wrong optione: stack selection wrong\n");
        return;
    }

    if (!bSmart || DSFlag.top != (DWORD_PTR)StackOffset)
        DumpStackDummy (DSFlag);
    else
        DumpStackSmart (DSFlag);
}


DECLARE_API (DumpStack)
{
    INIT_API();
    DumpStackInternal (args);
    return Status;
}


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function displays the stack trace for threads that EE knows  *  
*    from ThreadStore.                                                 *
*                                                                      *
\**********************************************************************/
DECLARE_API (EEStack)
{
    INIT_API();

    CHAR control[80] = "\0";
    BOOL bEEOnly = FALSE;
    BOOL bDumb = TRUE;
    BOOL bAllEEThread = TRUE;

    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-EE", &bEEOnly, COBOOL, FALSE},
        {"-smart", &bDumb, COBOOL, FALSE},
        {"-short", &bAllEEThread, COBOOL, FALSE}
    };

    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),NULL,0,NULL)) {
        return Status;
    }

    if (bEEOnly) {
        strcat (control," -EE");
    }
    if (!bDumb) {
        strcat (control," -smart");
    }
    ULONG Tid;
    g_ExtSystem->GetCurrentThreadId(&Tid);

    DWORD_PTR *threadList = NULL;
    int numThread = 0;
    GetThreadList (threadList, numThread);
    ToDestroy des0((void**)&threadList);
    
    int i;
    Thread vThread;
    for (i = 0; i < numThread; i ++)
    {
        if (IsInterrupt())
            break;
        DWORD_PTR dwAddr = threadList[i];
        vThread.Fill (dwAddr);
        ULONG id=0;
        if (g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id) != S_OK)
            continue;
        ExtOut ("---------------------------------------------\n");
        ExtOut ("Thread %3d\n", id);
        BOOL doIt = FALSE;
        if (bAllEEThread) {
            doIt = TRUE;
        }
        else if (vThread.m_dwLockCount > 0 || (int)vThread.m_pFrame != -1 || (vThread.m_State & Thread::TS_Hijacked)) {
            doIt = TRUE;
        }
        else {
            ULONG64 IP;
            g_ExtRegisters->GetInstructionOffset (&IP);
            JitType jitType;
            DWORD_PTR methodDesc;
            DWORD_PTR gcinfoAddr;
            IP2MethodDesc ((DWORD_PTR)IP, methodDesc, jitType, gcinfoAddr);
            if (methodDesc)
            {
                doIt = TRUE;
            }
        }
        if (doIt) {
            g_ExtSystem->SetCurrentThreadId(id);
            DumpStackInternal (control);
        }
    }

    g_ExtSystem->SetCurrentThreadId(Tid);

    return Status;
}
#endif //!PLATFORM_UNIX


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the address and name of all       *
*    Managed Objects on the stack.                                     *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpStackObjects)
{
    INIT_API();
    
    size_t StackTop = 0;
    size_t StackBottom = 0;

    while (isspace (args[0]))
        args ++;
    PCSTR pch = args;
    char* endptr;
    
    if (pch[0] == '\0')
    {
        ULONG64 StackOffset;
        g_ExtRegisters->GetStackOffset (&StackOffset);

        StackTop = (DWORD_PTR)StackOffset;
    }
    else
    {
        char buffer[80];
        StackTop = strtoul (pch, &endptr, 16);
        if (endptr[0] != '\0' && !isspace (endptr[0]))
        {
            strncpy (buffer,pch,79);
            buffer[79] = '\0';
            char * tmp = buffer;
            while (tmp[0] != '\0' && !isspace (tmp[0]))
                tmp ++;
            tmp[0] = '\0';
            StackTop = (DWORD_PTR)GetExpression(buffer);
            if (StackTop == 0)
            {
                ExtOut ("wrong option: %s\n", pch);
                return Status;
            }
            pch += strlen(buffer);
        }
        else
            pch = endptr;
        while (pch[0] != '\0' && isspace (pch[0]))
            pch ++;
        if (pch[0] != '\0')
        {
            StackBottom = strtoul (pch, &endptr, 16);
            if (endptr[0] != '\0' && !isspace (endptr[0]))
            {
                strncpy (buffer,pch,79);
                buffer[79] = '\0';
                char * tmp = buffer;
                while (tmp[0] != '\0' && !isspace (tmp[0]))
                    tmp ++;
                tmp[0] = '\0';
                StackBottom = (DWORD_PTR)GetExpression(buffer);
                if (StackBottom == 0)
                {
                    ExtOut ("wrong option: %s\n", pch);
                    return Status;
                }
            }
        }
    }

#ifndef PLATFORM_UNIX
    NT_TIB teb;
    ULONG64 dwTebAddr=0;
    g_ExtSystem->GetCurrentThreadTeb (&dwTebAddr);
    if (SafeReadMemory ((ULONG_PTR)dwTebAddr, &teb, sizeof (NT_TIB), NULL))
    {
        if (StackTop > (DWORD_PTR)teb.StackLimit
        && StackTop <= (DWORD_PTR)teb.StackBase)
        {
            if (StackBottom == 0 || StackBottom > (DWORD_PTR)teb.StackBase)
                StackBottom = (DWORD_PTR)teb.StackBase;
        }
    }
#endif
    if (StackBottom == 0)
        StackBottom = StackTop + 0xFFFF;
    
    if (StackBottom < StackTop)
    {
        ExtOut ("Wrong optione: stack selection wrong\n");
        return Status;
    }

    DumpStackObjectsHelper (StackTop, StackBottom);
    return Status;
}




/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a MethodDesc      *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpMD)
{
    DWORD_PTR dwStartAddr;

    INIT_API();
    
    dwStartAddr = (DWORD_PTR)GetExpression(args);
    DumpMDInfo (dwStartAddr);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of an EEClass from   *  
*    a given address
*                                                                      *
\**********************************************************************/
DECLARE_API (DumpClass)
{
    DWORD_PTR dwStartAddr;
    EEClass EECls;
    EEClass *pEECls = &EECls;
    
    INIT_API();

    BOOL bDumpChain = FALSE;
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-chain", &bDumpChain, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&dwStartAddr, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    if (nArg == 0) {
        ExtOut ("Missing EEClass address\n");
        return Status;
    }
    DWORD_PTR dwAddr = dwStartAddr;
    if (!IsEEClass (dwAddr))
    {
        ExtOut ("%p is not an EEClass\n", (ULONG64)dwStartAddr);
        return Status;
    }
    pEECls->Fill (dwStartAddr);
    if (!CallStatus)
    {
	    ExtOut( "DumpClass : ReadProcessMemory failed.\r\n" );
	    return Status;
    }
    
    ExtOut("Class Name : ");
    NameForEEClass (pEECls, g_mdName);
    ExtOut("%S", g_mdName);
    ExtOut ("\n");

    MethodTable vMethTable;
    moveN (vMethTable, pEECls->m_pMethodTable);
    WCHAR fileName[MAX_PATH+1];
    FileNameForMT (&vMethTable, fileName);
    ExtOut("mdToken : %p (%S)\n",(ULONG64)pEECls->m_cl, fileName);

    ExtOut("Parent Class : %p\r\n",(ULONG64)(UINT_PTR)pEECls->m_pParentClass);

    ExtOut("ClassLoader : %p\r\n",(ULONG64)(UINT_PTR)pEECls->m_pLoader);

    ExtOut("Method Table : %p\r\n",(ULONG64)(UINT_PTR)pEECls->m_pMethodTable);

    ExtOut("Vtable Slots : %x\r\n",pEECls->m_wNumVtableSlots);

    ExtOut("Total Method Slots : %x\r\n",pEECls->m_wNumMethodSlots);

    ExtOut("Class Attributes : %x : ",pEECls->m_dwAttrClass);
    if (IsTdInterface(pEECls->m_dwAttrClass))
    {
        ExtOut ("Interface, ");
    }
    if (IsTdAbstract(pEECls->m_dwAttrClass))
    {
        ExtOut ("Abstract, ");
    }
    if (IsTdImport(pEECls->m_dwAttrClass))
    {
        ExtOut ("ComImport, ");
    }
    
    ExtOut ("\n");
    
    
    ExtOut("Flags : %x\r\n",pEECls->m_VMFlags);

    ExtOut("NumInstanceFields: %x\n", pEECls->m_wNumInstanceFields);
    ExtOut("NumStaticFields: %x\n", pEECls->m_wNumStaticFields);
    ExtOut("ThreadStaticOffset: %x\n", pEECls->m_wThreadStaticOffset);
    ExtOut("ThreadStaticsSize: %x\n", pEECls->m_wThreadStaticsSize);
    ExtOut("ContextStaticOffset: %x\n", pEECls->m_wContextStaticOffset);
    ExtOut("ContextStaticsSize: %x\n", pEECls->m_wContextStaticsSize);
    
    if (pEECls->m_wNumInstanceFields + pEECls->m_wNumStaticFields > 0)
    {
        ExtOut ("FieldDesc*: %p\n", (ULONG64)(UINT_PTR)pEECls->m_pFieldDescList);
        DisplayFields(pEECls);
    }

    if (bDumpChain) {
    }
    return Status;
}




/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a MethodTable     *  
*    from a given address                                              *
*                                                                      *
\**********************************************************************/
DECLARE_API (DumpMT)
{
    DWORD_PTR dwStartAddr;
    MethodTable vMethTable;
    
    INIT_API();
    
    BOOL bDumpMDTable = FALSE;
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-MD", &bDumpMDTable, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&dwStartAddr, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    if (nArg == 0) {
        ExtOut ("Missing MethodTable address\n");
        return Status;
    }

    dwStartAddr = dwStartAddr&~3;
    
    if (!IsMethodTable (dwStartAddr))
    {
        ExtOut ("%p is not a MethodTable\n", (ULONG64)dwStartAddr);
        return Status;
    }
    if (dwStartAddr == MTForFreeObject()) {
        ExtOut ("Free MethodTable\n");
        return Status;
    }
    
    vMethTable.Fill (dwStartAddr);
    if (!CallStatus)
        return Status;
    
    ExtOut("EEClass : %p\r\n",(ULONG64)(UINT_PTR)vMethTable.m_pEEClass);

    ExtOut("Module : %p\r\n",(ULONG64)(UINT_PTR)vMethTable.m_pModule);

    EEClass eeclass;
    DWORD_PTR dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
    eeclass.Fill (dwAddr);
    if (!CallStatus)
        return Status;
    WCHAR fileName[MAX_PATH+1];
    if (eeclass.m_cl == 0x2000000)
    {
        ArrayClass vArray;
        dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
        vArray.Fill (dwAddr);
        ExtOut("Array: Rank %d, Type %s\n", vArray.m_dwRank,
                ElementTypeName(vArray.m_ElementType));
        //ExtOut ("Name: ");
        //ExtOut ("\n");
        dwAddr = (DWORD_PTR) vArray.m_ElementTypeHnd.m_asMT;
        while (dwAddr&2) {
            if (IsInterrupt())
                return Status;
            ParamTypeDesc param;
            DWORD_PTR dwTDAddr = dwAddr&~2;
            param.Fill(dwTDAddr);
            dwAddr = (DWORD_PTR)param.m_Arg.m_asMT;
        }
        NameForMT (dwAddr, g_mdName);
        ExtOut ("Element Type: %S\n", g_mdName);
    }
    else
    {
        FileNameForMT (&vMethTable, fileName);
        NameForToken(fileName, eeclass.m_cl, g_mdName);
        ExtOut ("Name: %S\n", g_mdName);
        ExtOut("mdToken: %08x ", eeclass.m_cl);
        ExtOut( " (%ws)\n",
                 fileName[0] ? fileName : L"Unknown Module" );
        ExtOut("MethodTable Flags : %x\r\n",vMethTable.m_wFlags & 0xFFFF0000); // low WORD is m_ComponentSize
        if (vMethTable.GetComponentSize())
            ExtOut ("Number of elements in array: %x\n",
                     vMethTable.GetComponentSize());
        ExtOut("Number of IFaces in IFaceMap : %x\r\n",
                vMethTable.m_wNumInterface);
        
        ExtOut("Interface Map : %p\r\n",(ULONG64)(UINT_PTR)vMethTable.m_pIMap);
        
        ExtOut("Slots in VTable : %d\r\n",vMethTable.m_cbSlots);
    }

    if (bDumpMDTable)
    {
        ExtOut ("--------------------------------------\n");
        ExtOut ("MethodDesc Table\n");
        ExtOut ("  Entry  MethodDesc   JIT   Name\n");
//                12345678 12345678    PreJIT xxxxxxxx
        DWORD_PTR dwAddr = vMethTable.m_Vtable[0];
        for (DWORD n = 0; n < vMethTable.m_cbSlots; n ++)
        {
            DWORD_PTR entry;
            moveN (entry, dwAddr);
            JitType jitType;
            DWORD_PTR methodDesc=0;
            DWORD_PTR gcinfoAddr;
            IP2MethodDesc (entry, methodDesc, jitType, gcinfoAddr);
            if (!methodDesc)
            {
                methodDesc = entry + 5;
            }
            ExtOut ("%p %p    ", (ULONG64)entry, (ULONG64)methodDesc);
            if (jitType == EJIT)
                ExtOut ("EJIT  ");
            else if (jitType == JIT)
                ExtOut ("JIT   ");
            else if (jitType == PJIT)
                ExtOut ("PreJIT");
            else
                ExtOut ("None  ");
            
            MethodDesc vMD;
            DWORD_PTR dwMDAddr = methodDesc;
            vMD.Fill (dwMDAddr);
            
            CQuickBytes fullname;
            FullNameForMD (&vMD, &fullname);
            ExtOut (" %S\n", (WCHAR*)fullname.Ptr());
            dwAddr += sizeof(PVOID);
        }
    }
    return Status;
}

extern size_t Align (size_t nbytes);

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of an object from a  *  
*    given address
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpObj)    
{
    INIT_API();
    DWORD_PTR p_Object = (DWORD_PTR)GetExpression (args);
    if (p_Object == 0)
        return Status;
    DWORD_PTR p_MT;
    moveN (p_MT, p_Object);
    p_MT = p_MT&~3;

    if (!IsMethodTable (p_MT))
    {
        ExtOut ("%s is not a managed object\n", args);
        return Status;
    }

    if (p_MT == MTForFreeObject()) {
        ExtOut ("Free Object\n");
        DWORD size = ObjectSize (p_Object);
        ExtOut ("Size %d(0x%x) bytes\n", size, size);
        return Status;
    }

    DWORD_PTR size = 0;
    MethodTable vMethTable;
    DWORD_PTR dwAddr = p_MT;
    vMethTable.Fill (dwAddr);
    NameForMT (vMethTable, g_mdName);
    ExtOut ("Name: %S\n", g_mdName);
    ExtOut ("MethodTable 0x%p\n", (ULONG64)p_MT);
    ExtOut ("EEClass 0x%p\n", (ULONG64)(UINT_PTR)vMethTable.m_pEEClass);
    size = ObjectSize (p_Object);
    ExtOut ("Size  %d(0x%x) bytes\n", size,size);
    EEClass vEECls;
    dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
    vEECls.Fill (dwAddr);
    if (!CallStatus)
        return Status;
    if (vEECls.m_cl == 0x2000000)
    {
        ArrayClass vArray;
        dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
        vArray.Fill (dwAddr);
        ExtOut("Array: Rank %d, Type %s\n", vArray.m_dwRank,
                ElementTypeName(vArray.m_ElementType));
        //ExtOut ("Name: ");
        //ExtOut ("\n");
        dwAddr = (DWORD_PTR) vArray.m_ElementTypeHnd.m_asMT;
        NameForMT (dwAddr, g_mdName);
        ExtOut ("Element Type: %S\n", g_mdName);
        if (vArray.m_ElementType == 3)
        {
            ExtOut ("Content:\n");
            dwAddr = p_Object + 4;
            DWORD_PTR num;
            moveN (num, dwAddr);
            PrintString (dwAddr+4, TRUE, num);
            ExtOut ("\n");
        }
    }
    else
    {
        FileNameForMT (&vMethTable, g_mdName);
        ExtOut("mdToken: %08x ", vEECls.m_cl);
        ExtOut( " (%ws)\n",
                 g_mdName[0] ? g_mdName : L"Unknown Module" );
    }
    
    if (p_MT == MTForString())
    {
        ExtOut ("String: ");
        StringObjectContent (p_Object);
        ExtOut ("\n");
    }
    else if (p_MT == MTForObject())
    {
        ExtOut ("Object\n");
    }

    if (vEECls.m_wNumInstanceFields + vEECls.m_wNumStaticFields > 0)
    {
        ExtOut ("FieldDesc*: %p\n", (ULONG64)(UINT_PTR)vEECls.m_pFieldDescList);
        DisplayFields(&vEECls, p_Object, TRUE);
    }
    return Status;
}


HeapStat *gStat = NULL;

void PrintGCStat ()
{
    if (gStat)
    {
#ifdef PLATFORM_UNIX
        ExtOut ("GC Statistics not available on Unix\n");
#else
        ExtOut ("Statistics:\n");
        ExtOut ("%8s %8s %9s %s\n",
                 "MT", "Count", "TotalSize", "Class Name");
        __try 
        {
            gStat->Sort();
        } __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ExtOut ("exception during sorting\n");
            gStat->Delete();
            return;
        }        
        __try 
        {
            gStat->Print();
        } __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ExtOut ("exception during printing\n");
            gStat->Delete();
            return;
        }        
#endif //!PLATFORM_UNIX
        gStat->Delete();
    }
}


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function dumps what is in the syncblock cache.  By default   *  
*    it dumps all active syncblocks.  Using -all to dump all syncblocks
*                                                                      *
\**********************************************************************/
DECLARE_API(SyncBlk)
{
    INIT_API();

    BOOL bDumpAll = FALSE;
    size_t nbAsked = 0;
    
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-all", &bDumpAll, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&nbAsked, COSIZE_T}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    DWORD_PTR p_s_pSyncBlockCache = GetAddressOf (offset_class_Global_Variables,
      offset_member_Global_Variables::g_SyncBlockCacheInstance);

    SyncBlockCache s_pSyncBlockCache;
    s_pSyncBlockCache.Fill (p_s_pSyncBlockCache);
    if (!CallStatus)
    {
        ExtOut ("Can not get mscoree!g_SyncBlockCacheInstance\n");
        return Status;
    }
    
    DWORD_PTR p_g_pSyncTable = GetAddressOf (offset_class_Global_Variables, 
      offset_member_Global_Variables::g_pSyncTable);

    DWORD_PTR pSyncTable;
    moveN (pSyncTable, p_g_pSyncTable);
    pSyncTable += SyncTableEntry::size();
    SyncTableEntry v_SyncTableEntry;
    if (s_pSyncBlockCache.m_FreeSyncTableIndex < 2)
        return Status;
    DWORD_PTR dwAddr;
    SyncBlock vSyncBlock;
    ULONG offsetHolding = 
        AwareLock::GetFieldOffset(offset_member_AwareLock::m_HoldingThread);
    ULONG offsetLinkSB = 
        WaitEventLink::GetFieldOffset(offset_member_WaitEventLink::m_LinkSB);
    ExtOut ("Index SyncBlock MonitorHeld Recursion   Thread  ThreadID     Object Waiting\n");
    ULONG freeCount = 0;
    ULONG CCWCount = 0;
    ULONG RCWCount = 0;
    ULONG CFCount = 0;
    for (DWORD nb = 1; nb < s_pSyncBlockCache.m_FreeSyncTableIndex; nb++)
    {
        if (IsInterrupt())
            return Status;
        if (nbAsked && nb != nbAsked) {
            pSyncTable += SyncTableEntry::size();
            continue;
        }
        dwAddr = (DWORD_PTR)pSyncTable;
        v_SyncTableEntry.Fill(dwAddr);
        if (v_SyncTableEntry.m_SyncBlock == 0) {
            if (bDumpAll || nbAsked == nb) {
                ExtOut ("%5d ", nb);
                ExtOut ("%08x  ", 0);
                ExtOut ("%11s ", " ");
                ExtOut ("%9s ", " ");
                ExtOut ("%8s ", " ");
                ExtOut ("%10s" , " ");
                ExtOut ("  %08x", (v_SyncTableEntry.m_Object));
                ExtOut ("\n");
            }
            pSyncTable += SyncTableEntry::size();
            continue;
        }
        dwAddr = v_SyncTableEntry.m_SyncBlock;
        vSyncBlock.Fill (dwAddr);
        BOOL bPrint = (bDumpAll || nb == nbAsked);
        if (!bPrint && v_SyncTableEntry.m_SyncBlock != 0
            && vSyncBlock.m_Monitor.m_MonitorHeld > 0
            && (v_SyncTableEntry.m_Object & 0x1) == 0)
            bPrint = TRUE;
        if (bPrint)
        {
            ExtOut ("%5d ", nb);
            ExtOut ("%08x  ", v_SyncTableEntry.m_SyncBlock);
            ExtOut ("%11d ", vSyncBlock.m_Monitor.m_MonitorHeld);
            ExtOut ("%9d ", vSyncBlock.m_Monitor.m_Recursion);
        }
        DWORD_PTR p_thread;
        p_thread = v_SyncTableEntry.m_SyncBlock + offsetHolding;
        DWORD_PTR thread = vSyncBlock.m_Monitor.m_HoldingThread;
        if (bPrint)
            ExtOut ("%8x ", thread);
        DWORD_PTR threadID = 0;
        if (thread != 0)
        {
            Thread vThread;
            threadID = thread;
            vThread.Fill (threadID);
            if (bPrint)
            {
                ExtOut ("%5x", vThread.m_ThreadId);
#ifdef PLATFORM_WIN32
                ULONG id;
                if (g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id) == S_OK)
                {
                    ExtOut ("%4d ", id);
                }
                else
#endif //PLATFORM_WIN32
                {
                    ExtOut (" XXX ");
                }
            }
        }
        else
        {
            if (bPrint)
                ExtOut ("    none  ");
        }
        if (bPrint) {
            if (v_SyncTableEntry.m_Object & 0x1) {
                ExtOut ("  %8d", (v_SyncTableEntry.m_Object & ~0x1)>>1);
            }
            else {
                ExtOut ("  %p", (ULONG64)v_SyncTableEntry.m_Object);
                NameForObject((DWORD_PTR)v_SyncTableEntry.m_Object, g_mdName);
                ExtOut (" %S", g_mdName);
            }
        }
        if (v_SyncTableEntry.m_Object & 0x1) {
            freeCount ++;
            if (bPrint) {
                ExtOut (" Free");
            }
        }
        else {
            if (vSyncBlock.m_pComData) {
                switch (vSyncBlock.m_pComData & 3) {
                case 0:
                    CCWCount ++;
                    break;
                case 1:
                    RCWCount ++;
                    break;
                case 3:
                    CFCount ++;
                    break;
                }
            }
        }

        if (v_SyncTableEntry.m_SyncBlock != 0
            && vSyncBlock.m_Monitor.m_MonitorHeld > 1
            && vSyncBlock.m_Link.m_pNext > 0)
        {
            ExtOut (" ");
            DWORD_PTR pHead = (DWORD_PTR)vSyncBlock.m_Link.m_pNext;
            DWORD_PTR pNext = pHead;
            Thread vThread;
    
            while (1)
            {
                if (IsInterrupt())
                    return Status;
                DWORD_PTR pWaitEventLink = pNext - offsetLinkSB;
                WaitEventLink vWaitEventLink;
                vWaitEventLink.Fill(pWaitEventLink);
                if (!CallStatus) {
                    break;
                }
                DWORD_PTR dwAddr = (DWORD_PTR)vWaitEventLink.m_Thread;
                ExtOut ("%x ", dwAddr);
                vThread.Fill (dwAddr);
                if (!CallStatus) {
                    break;
                }
                if (bPrint)
                    ExtOut ("%x,", vThread.m_ThreadId);
                pNext = (DWORD_PTR)vWaitEventLink.m_LinkSB.m_pNext;
                if (pNext == 0)
                    break;
            }            
        }
        if (bPrint)
            ExtOut ("\n");
        pSyncTable += SyncTableEntry::size();
    }
    
    ExtOut ("-----------------------------\n");
    ExtOut ("Total           %d\n", s_pSyncBlockCache.m_FreeSyncTableIndex);
    ExtOut ("ComCallWrapper  %d\n", CCWCount);
    ExtOut ("ComPlusWrapper  %d\n", RCWCount);
    ExtOut ("ComClassFactory %d\n", CFCount);
    ExtOut ("Free            %d\n", freeCount);

    return Status;
}


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a Module          *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpModule)
{
    INIT_API();
    DWORD_PTR p_ModuleAddr = (DWORD_PTR)GetExpression (args);
    if (p_ModuleAddr == 0)
        return Status;
    Module v_Module;
    v_Module.Fill (p_ModuleAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill Module\n");
        return Status;
    }
    WCHAR FileName[MAX_PATH+1];
    FileNameForModule (&v_Module, FileName);
    ExtOut ("Name %ws\n", FileName[0] ? FileName : L"Unknown Module");
    ExtOut ("dwFlags %08x\n", v_Module.m_dwFlags);
    ExtOut ("Attribute ");
    if (v_Module.m_dwFlags & Module::IS_IN_MEMORY)
        ExtOut ("%s", "InMemory ");
    if (v_Module.m_dwFlags & Module::IS_PRELOAD)
        ExtOut ("%s", "Preload ");
    if (v_Module.m_dwFlags & Module::IS_PEFILE)
        ExtOut ("%s", "PEFile ");
    if (v_Module.m_dwFlags & Module::IS_REFLECTION)
        ExtOut ("%s", "Reflection ");
    if (v_Module.m_dwFlags & Module::IS_PRECOMPILE)
        ExtOut ("%s", "PreCompile ");
    if (v_Module.m_dwFlags & Module::IS_EDIT_AND_CONTINUE)
        ExtOut ("%s", "Edit&Continue ");
    if (v_Module.m_dwFlags & Module::SUPPORTS_UPDATEABLE_METHODS)
        ExtOut ("%s", "SupportsUpdateableMethods");
    ExtOut ("\n");
    ExtOut ("Assembly %p\n", (ULONG64)(UINT_PTR)v_Module.m_pAssembly);

    ExtOut ("LoaderHeap* %p\n", (ULONG64)(UINT_PTR)v_Module.m_pLookupTableHeap);
    ExtOut ("TypeDefToMethodTableMap* %p\n",
             (ULONG64)(UINT_PTR)v_Module.m_TypeDefToMethodTableMap.pTable);
    ExtOut ("TypeRefToMethodTableMap* %p\n",
             (ULONG64)(UINT_PTR)v_Module.m_TypeRefToMethodTableMap.pTable);
    ExtOut ("MethodDefToDescMap* %p\n",
             (ULONG64)(UINT_PTR)v_Module.m_MethodDefToDescMap.pTable);
    ExtOut ("FieldDefToDescMap* %p\n",
             (ULONG64)(UINT_PTR)v_Module.m_FieldDefToDescMap.pTable);
    ExtOut ("MemberRefToDescMap* %p\n",
             (ULONG64)(UINT_PTR)v_Module.m_MemberRefToDescMap.pTable);
    ExtOut ("FileReferencesMap* %p\n",
             (ULONG64)(UINT_PTR)v_Module.m_FileReferencesMap.pTable);
    ExtOut ("AssemblyReferencesMap* %p\n",
             (ULONG64)(UINT_PTR)v_Module.m_AssemblyReferencesMap.pTable);
    
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a Domain          *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpDomain)
{
    INIT_API();
    DWORD_PTR p_DomainAddr = (DWORD_PTR)GetExpression (args);
    
    AppDomain v_AppDomain;
    if (p_DomainAddr)
    {
        ExtOut ("Domain: %p\n", (ULONG64)p_DomainAddr);
        v_AppDomain.Fill (p_DomainAddr);
        if (!CallStatus)
        {
            ExtOut ("Fail to fill AppDomain\n");
            return Status;
        }
        DomainInfo (&v_AppDomain);
        return Status;
    }
    
    // List all domain
    int numDomain;
    DWORD_PTR *domainList = NULL;
    GetDomainList (domainList, numDomain);
    ToDestroy des0 ((void**)&domainList);
    
    // The first one is the system domain.
    p_DomainAddr = domainList[0];
    ExtOut ("--------------------------------------\n");
    ExtOut ("System Domain: %p\n", (ULONG64)p_DomainAddr);
    v_AppDomain.Fill (p_DomainAddr);
    DomainInfo (&v_AppDomain);

    // The second one is the shared domain.
    p_DomainAddr = domainList[1];
    ExtOut ("--------------------------------------\n");
    ExtOut ("Shared Domain: %x\n", p_DomainAddr);
    SharedDomainInfo (p_DomainAddr);

    int n;
    int n0 = 2;
    for (n = n0; n < numDomain; n++)
    {
        if (IsInterrupt())
            break;

        p_DomainAddr = domainList[n];
        ExtOut ("--------------------------------------\n");
        ExtOut ("Domain %d: %x\n", n-n0+1, p_DomainAddr);
        if (p_DomainAddr == 0) {
            continue;
        }
        // Check if this domain already appears.
        int i;
        for (i = 0; i < n; i ++)
        {
            if (domainList[i] == p_DomainAddr)
                break;
        }
        if (i < n)
        {
            ExtOut ("Same as Domain %d\n", i-n0);
        }
        else
        {
            v_AppDomain.Fill (p_DomainAddr);
            DomainInfo (&v_AppDomain);
        }
    }
    return Status;
}


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a Assembly        *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpAssembly)
{
    INIT_API();
    DWORD_PTR p_AssemblyAddr = (DWORD_PTR)GetExpression (args);
    if (p_AssemblyAddr == 0)
        return Status;
    Assembly v_Assembly;
    v_Assembly.Fill (p_AssemblyAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill Assembly\n");
        return Status;
    }
    ExtOut ("Parent Domain: %p\n", (ULONG64)(UINT_PTR)v_Assembly.m_pDomain);
    ExtOut ("Name: ");
    PrintString ((DWORD_PTR) v_Assembly.m_psName);
    ExtOut ("\n");
    AssemblyInfo (&v_Assembly);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a ClassLoader     *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpLoader)
{
    INIT_API();
    DWORD_PTR p_ClassLoaderAddr = (DWORD_PTR)GetExpression (args);
    if (p_ClassLoaderAddr == 0)
        return Status;
    ClassLoader v_ClassLoader;
    v_ClassLoader.Fill (p_ClassLoaderAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill ClassLoader\n");
        return Status;
    }
    ExtOut ("Assembly: %p\n", (ULONG64)(UINT_PTR)v_ClassLoader.m_pAssembly);
    ExtOut ("Next ClassLoader: %p\n", (ULONG64)(UINT_PTR)v_ClassLoader.m_pNext);
    ClassLoaderInfo(&v_ClassLoader);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the managed threads               *
*                                                                      *
\**********************************************************************/
DECLARE_API(Threads)
{
    INIT_API();

    DWORD_PTR p_g_pThreadStore = GetAddressOf (offset_class_Global_Variables, 
      offset_member_Global_Variables::g_pThreadStore);

    DWORD_PTR g_pThreadStore;
    HRESULT hr;
    if (FAILED (hr = g_ExtData->ReadVirtual (p_g_pThreadStore, &g_pThreadStore,
                                             sizeof (DWORD_PTR), NULL)))
        return hr;
    if (g_pThreadStore == 0)
        return S_FALSE;
    
    ThreadStore vThreadStore;
    DWORD_PTR dwAddr = g_pThreadStore;
    vThreadStore.Fill (dwAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill ThreadStore\n");
        return Status;
    }

    ExtOut ("ThreadCount: %d\n", vThreadStore.m_ThreadCount);
    ExtOut ("UnstartedThread: %d\n", vThreadStore.m_UnstartedThreadCount);
    ExtOut ("BackgroundThread: %d\n", vThreadStore.m_BackgroundThreadCount);
    ExtOut ("PendingThread: %d\n", vThreadStore.m_PendingThreadCount);
    ExtOut ("DeadThread: %d\n", vThreadStore.m_DeadThreadCount);
    
    
    DWORD_PTR *threadList = NULL;
    int numThread = 0;
    GetThreadList (threadList, numThread);
    ToDestroy des0((void**)&threadList);
    
    static DWORD_PTR FinalizerThreadAddr = 0;
    if (FinalizerThreadAddr == 0)
    {
        FinalizerThreadAddr = GetAddressOf (offset_class_GCHeap, 
          offset_member_GCHeap::FinalizerThread);

    }
    DWORD_PTR finalizerThread;
    moveN (finalizerThread, FinalizerThreadAddr);

    static DWORD_PTR GcThreadAddr = 0;
    if (GcThreadAddr == 0)
    {
        GcThreadAddr = GetAddressOf (offset_class_GCHeap, 
          offset_member_GCHeap::GcThread);

    }
    DWORD_PTR GcThread;
    moveN (GcThread, GcThreadAddr);

    
    ExtOut ("                             PreEmptive        Lock  \n");
    ExtOut ("       ID ThreadOBJ    State     GC     Domain Count APT Exception\n");
    int i;
    Thread vThread;
    for (i = 0; i < numThread; i ++)
    {
        if (IsInterrupt())
            break;
        DWORD_PTR dwAddr = threadList[i];
        vThread.Fill (dwAddr);
        if (!IsKernelDebugger()) {
#ifdef PLATFORM_WIN32
            ULONG id=0;
            if (g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id) == S_OK)
            {
                ExtOut ("%3d ", id);
            }
            else
#endif //PLATFORM_WIN32
            {
                ExtOut ("XXX ");
            }
        }
        else
            ExtOut ("    ");
        
        ExtOut ("%5x %p  %8x", vThread.m_ThreadId, (ULONG64)threadList[i],
                vThread.m_State);
        if (vThread.m_fPreemptiveGCDisabled == 1)
            ExtOut (" Disabled");
        else
            ExtOut (" Enabled ");

        Context vContext;
        DWORD_PTR dwAddrTmp = (DWORD_PTR)vThread.m_Context;
        vContext.Fill (dwAddrTmp);
        if (vThread.m_pDomain)
            ExtOut (" %p", (ULONG64)(UINT_PTR)vThread.m_pDomain);
        else
        {
            ExtOut (" %p", (ULONG64)(UINT_PTR)vContext.m_pDomain);
        }
        ExtOut (" %5d", vThread.m_dwLockCount);


        if (threadList[i] == finalizerThread)
            ExtOut (" (Finalizer)");
        if (threadList[i] == GcThread)
            ExtOut (" (GC)");
        if (vThread.m_State & Thread::TS_ThreadPoolThread) {
            if (vThread.m_State & Thread::TS_TPWorkerThread) {
                ExtOut (" (Threadpool Worker)");
            }
            else
                ExtOut (" (Threadpool Completion Port)");
        }
        if (!SafeReadMemory((DWORD_PTR)vThread.m_LastThrownObjectHandle,
                            &dwAddr,
                            sizeof(dwAddr), NULL))
            goto end_of_loop;
        if (dwAddr)
        {
            DWORD_PTR MTAddr;
            if (!SafeReadMemory(dwAddr, &MTAddr, sizeof(MTAddr), NULL))
                goto end_of_loop;
            MethodTable vMethTable;
            vMethTable.Fill (MTAddr);
            NameForMT (vMethTable, g_mdName);
            ExtOut (" %S", g_mdName);
        }
end_of_loop:
        ExtOut ("\n");
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the managed threadpool            *
*                                                                      *
\**********************************************************************/
DECLARE_API(ThreadPool)
{
    INIT_API();

    static DWORD_PTR cpuUtilizationAddr = 0;
    if (cpuUtilizationAddr == 0) {
        cpuUtilizationAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::cpuUtilization);
    }
    long lValue;
    if (g_ExtData->ReadVirtual(cpuUtilizationAddr,&lValue,sizeof(lValue),NULL) == S_OK
        && lValue != 0) {
        ExtOut ("CPU utilization %d%%\n", lValue);
    }
    
    ExtOut ("Worker Thread:");
    static DWORD_PTR NumWorkerThreadsAddr = 0;
    if (NumWorkerThreadsAddr == 0) {
        NumWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumWorkerThreads);
    }
    int iValue;
    if (g_ExtData->ReadVirtual(NumWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Total: %d", iValue);
    }
    
    static DWORD_PTR NumRunningWorkerThreadsAddr = 0;
    if (NumRunningWorkerThreadsAddr == 0) {
        NumRunningWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumRunningWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(NumRunningWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Running: %d", iValue);
    }
    
    static DWORD_PTR NumIdleWorkerThreadsAddr = 0;
    if (NumIdleWorkerThreadsAddr == 0) {
        NumIdleWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumIdleWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(NumIdleWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Idle: %d", iValue);
    }
    
    static DWORD_PTR MaxLimitTotalWorkerThreadsAddr = 0;
    if (MaxLimitTotalWorkerThreadsAddr == 0) {
        MaxLimitTotalWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::MaxLimitTotalWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(MaxLimitTotalWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" MaxLimit: %d", iValue);
    }
    
    static DWORD_PTR MinLimitTotalWorkerThreadsAddr = 0;
    if (MinLimitTotalWorkerThreadsAddr == 0) {
        MinLimitTotalWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::MinLimitTotalWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(MinLimitTotalWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" MinLimit: %d", iValue);
    }
    ExtOut ("\n");
    
    static DWORD_PTR NumQueuedWorkRequestsAddr = 0;
    if (NumQueuedWorkRequestsAddr == 0) {
        NumQueuedWorkRequestsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumQueuedWorkRequests);
    }
    if (g_ExtData->ReadVirtual(NumQueuedWorkRequestsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut ("Work Request in Queue: %d\n", iValue);
    }

    if (iValue > 0) {
        // Display work request
        static DWORD_PTR FQueueUserWorkItemCallback = 0;
        static DWORD_PTR FtimerDeleteWorkItem = 0;
        static DWORD_PTR FAsyncCallbackCompletion = 0;
        static DWORD_PTR FAsyncTimerCallbackCompletion = 0;

        if (FQueueUserWorkItemCallback == 0) {
            FQueueUserWorkItemCallback = GetAddressOf (offset_class_Global_Variables, 
              offset_member_Global_Variables::QueueUserWorkItemCallback);
        }
        if (FtimerDeleteWorkItem == 0) {
            FtimerDeleteWorkItem = GetAddressOf (offset_class_TimerNative, 
              offset_member_TimerNative::timerDeleteWorkItem);
        }
        if (FAsyncCallbackCompletion == 0) {
            FAsyncCallbackCompletion = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::AsyncCallbackCompletion);
        }
        if (FAsyncTimerCallbackCompletion == 0) {
            FAsyncTimerCallbackCompletion = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::AsyncTimerCallbackCompletion);
        }

        static DWORD_PTR headAddr = 0;
        static DWORD_PTR tailAddr = 0;
        if (headAddr == 0) {
            headAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::WorkRequestHead);
        }
        if (tailAddr == 0) {
            tailAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::WorkRequestTail);
        }
        DWORD_PTR head;
        g_ExtData->ReadVirtual(headAddr,&head,sizeof(head),NULL);
        DWORD_PTR dwAddr = head;
        WorkRequest work;
        while (dwAddr) {
            if (IsInterrupt())
                break;
            work.Fill(dwAddr);
            if ((DWORD_PTR)work.Function == FQueueUserWorkItemCallback)
                ExtOut ("QueueUserWorkItemCallback DelegateInfo@%p\n", (ULONG64)(UINT_PTR)work.Context);
            else if ((DWORD_PTR)work.Function == FtimerDeleteWorkItem)
                ExtOut ("timerDeleteWorkItem TimerDeleteInfo@%p\n", (ULONG64)(UINT_PTR)work.Context);
            else if ((DWORD_PTR)work.Function == FAsyncCallbackCompletion)
                ExtOut ("AsyncCallbackCompletion AsyncCallback@%p\n", (ULONG64)(UINT_PTR)work.Context);
            else if ((DWORD_PTR)work.Function == FAsyncTimerCallbackCompletion)
                ExtOut ("AsyncTimerCallbackCompletion TimerInfo@%p\n", (ULONG64)(UINT_PTR)work.Context);
            else
                ExtOut ("Unknown %p\n", (ULONG64)(UINT_PTR)work.Context);
            
            dwAddr = (DWORD_PTR)work.next;
        }
    }
    ExtOut ("--------------------------------------\n");

    static DWORD_PTR NumTimersAddr = 0;
    if (NumTimersAddr == 0) {
        NumTimersAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumTimers);
    }
    DWORD dValue;
    if (g_ExtData->ReadVirtual(NumTimersAddr,&dValue,sizeof(dValue),NULL) == S_OK) {
        ExtOut ("Number of Timers: %d\n", dValue);
    }

    ExtOut ("--------------------------------------\n");
    

    ExtOut ("\n");

    return Status;
}



#ifndef PLATFORM_UNIX
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to unassembly a managed function.         *
*    It tries to print symbolic info for function call, contants...    *  
*                                                                      *
\**********************************************************************/
DECLARE_API(u)
{
    INIT_API();

    DWORD_PTR dwStartAddr;
    MethodDesc MD;

    dwStartAddr = (DWORD_PTR)GetExpression (args);
    
    DWORD_PTR tmpAddr = dwStartAddr;
    CodeInfo infoHdr;
    DWORD_PTR methodDesc = tmpAddr;
    if (!IsMethodDesc (tmpAddr))
    {
        tmpAddr = dwStartAddr;
        IP2MethodDesc (tmpAddr, methodDesc, infoHdr.jitType,
                       infoHdr.gcinfoAddr);
        if (!methodDesc || infoHdr.jitType == UNKNOWN)
        {
            // It is not managed code.
            ExtOut ("Unmanaged code\n");
            UnassemblyUnmanaged(dwStartAddr);
            return Status;
        }
        tmpAddr = methodDesc;
    }
    MD.Fill (tmpAddr);
    if (!CallStatus)
        return Status;
    

    if((MD.m_CodeOrIL & METHOD_IS_IL_FLAG) == METHOD_IS_IL_FLAG)
    {
        ExtOut("Not jitted yet\n");
        return Status;
    }

    CodeInfoForMethodDesc (MD, infoHdr);
    if (infoHdr.IPBegin == 0)
    {
        ExtOut("not a valid MethodDesc\n");
        return Status;
    }
    if (infoHdr.jitType == UNKNOWN)
    {
        ExtOut ("unknown Jit\n");
        return Status;
    }
    else if (infoHdr.jitType == EJIT)
    {
        ExtOut ("EJIT generated code\n");
    }
    else if (infoHdr.jitType == JIT)
    {
        ExtOut ("Normal JIT generated code\n");
    }
    else if (infoHdr.jitType == PJIT)
    {
        ExtOut ("preJIT generated code\n");
    }
    CQuickBytes fullname;
    FullNameForMD (&MD, &fullname);
    ExtOut ("%S\n", (WCHAR*)fullname.Ptr());
    
    ExtOut ("Begin %p, size %x\n", (ULONG64)infoHdr.IPBegin, infoHdr.methodSize);

    Unassembly (infoHdr.IPBegin, infoHdr.IPBegin+infoHdr.methodSize);

    return Status;
}
#endif //!PLATFORM_UNIX


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to search a DWORD on stack                *
*                                                                      *
\**********************************************************************/
DECLARE_API (SearchStack)
{
    INIT_API();

    DWORD_PTR num[2];
    int index = 0;
    
    while (isspace (args[0]))
        args ++;
    char buffer[100];
    strcpy (buffer, args);
    LPSTR pch = buffer;
    while (pch[0] != '\0')
    {
        if (IsInterrupt())
            return Status;
        while (isspace (pch[0]))
            pch ++;
        char *endptr;
        num[index] = strtoul (pch, &endptr, 16);
        if (pch == endptr)
        {
            ExtOut ("wrong argument\n");
            return Status;
        }
        index ++;
        if (index == 2)
            break;
        pch = endptr;
    }

    DWORD_PTR top;
    ULONG64 StackOffset;
    g_ExtRegisters->GetStackOffset (&StackOffset);
    if (index <= 1)
    {
        top = (DWORD_PTR)StackOffset;
    }
    else
    {
        top = num[1];
    }
    
    DWORD_PTR end = top + 0xFFFF;
    DWORD_PTR ptr = top & ~3;  // make certain dword aligned
    while (ptr < end)
    {
        if (IsInterrupt())
            return Status;
        DWORD_PTR value;
        moveN (value, ptr);
        if (value == num[0])
            ExtOut ("%p\n", (ULONG64)ptr);
        ptr += sizeof (DWORD_PTR);
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a CrawlFrame      *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API (DumpCrawlFrame)
{
    INIT_API();
    DWORD_PTR dwStartAddr = (DWORD_PTR)GetExpression(args);
    if (dwStartAddr == 0)
        return Status;
    
    CrawlFrame crawlFrame;
    moveN (crawlFrame, dwStartAddr);
    ExtOut ("MethodDesc: %p\n", (ULONG64)(UINT_PTR)crawlFrame.pFunc);
    REGDISPLAY RD;
    moveN (RD, crawlFrame.pRD);
#if defined(_X86_)
    DWORD_PTR Edi, Esi, Ebx, Edx, Ecx, Eax, Ebp, PC;
    moveN (Edi, RD.pEdi);
    moveN (Esi, RD.pEsi);
    moveN (Ebx, RD.pEbx);
    moveN (Edx, RD.pEdx);
    moveN (Ecx, RD.pEcx);
    moveN (Eax, RD.pEax);
    moveN (Ebp, RD.pEbp);
    moveN (PC, RD.pPC);
    
    ExtOut ("EDI=%8x ESI=%8x EBX=%8x EDX=%8x ", Edi, Esi, Ebx, Edx);
    ExtOut ("ECX=%8x EAX=%8x\n", Ecx, Eax);
    ExtOut ("EBP=%8x ESP=%8x PC=%8x\n", Ebp, RD.SP, PC);
#elif defined(_PPC_)
    DWORD SavedRegs[NUM_CALLEESAVED_REGISTERS];
    DWORD PC;
    int i;

    for (i=0; i<NUM_CALLEESAVED_REGISTERS; ++i) {
        moveN(SavedRegs[i], RD.pR[i]);
    }
    moveN(PC, RD.pPC);
    for (i=0; i<NUM_CALLEESAVED_REGISTERS; ++i) {
        ExtOut("r%d=%8x ", 13+i, SavedRegs[i]);
        if (i && i %4 == 0) {
            ExtOut("\n");
        }
    }
    ExtOut("SP=%8x PC=%8x\n", RD.SP, PC);
#else
    PORTABILITY_ASSERT("DumpCrawlFrame is NYI");
#endif
    return Status;
}





EEDllPath *DllPath = NULL;

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to set the symbol option suitable for     *  
*    strike.                                                           *
*                                                                      *
\**********************************************************************/
DECLARE_API (SymOpt)
{
    INIT_API();

#ifdef PLATFORM_UNIX
    ExtOut("SymOpt not supported on PLATFORM_UNIX\n");
#else
    ULONG Options;
    g_ExtSymbols->GetSymbolOptions(&Options);
    ULONG NewOptions = Options | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_NO_CPP;
    ExtOut ("%x\n", Options);
    if (Options != NewOptions)
        g_ExtSymbols->SetSymbolOptions(Options);
#endif // !PLATFORM_UNIX
    return Status;
}




/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the address of EE data for a      *  
*    metadata token.                                                   *
*                                                                      *
\**********************************************************************/
DECLARE_API (Token2EE)
{
    INIT_API();
    while (isspace (args[0]))
        args ++;
    char buffer[100];
    strcpy (buffer, args);
    LPSTR pch = buffer;
    LPSTR mName = buffer;
    while (!isspace (pch[0]) && pch[0] != '\0')
        pch ++;
    if (pch[0] == '\0')
    {
        ExtOut ("Usage: Token2EE module_name mdToken\n");
        return Status;
    }
    pch[0] = '\0';
    pch ++;
    char *endptr;
    ULONG token = strtoul (pch, &endptr, 16);
    pch = endptr;
    while (isspace (pch[0]))
        pch ++;
    if (pch[0] != '\0')
    {
        ExtOut ("Usage: Token2EE module_name mdToken\n");
        return Status;
    }

    int numModule;
    DWORD_PTR *moduleList = NULL;
    ModuleFromName(moduleList, mName, numModule);
    ToDestroy des0 ((void**)&moduleList);
    
    for (int i = 0; i < numModule; i ++)
    {
        if (IsInterrupt())
            break;

        Module vModule;
        DWORD_PTR dwAddr = moduleList[i];
        vModule.Fill (dwAddr);
        ExtOut ("--------------------------------------\n");
        GetInfoFromModule (vModule, token);
    }
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the address of EE data for a      *  
*    metadata token.
*                                                                      *
\**********************************************************************/
DECLARE_API (Name2EE)
{
    INIT_API();
    while (isspace (args[0]))
        args ++;
    char buffer[100];
    strcpy (buffer, args);
    LPSTR pch = buffer;
    LPSTR mName = buffer;
    while (!isspace (pch[0]) && pch[0] != '\0')
        pch ++;
    if (pch[0] == '\0')
    {
        ExtOut ("Usage: Name2EE module_name mdToken\n");
        return Status;
    }
    pch[0] = '\0';
    pch ++;
    char *name = pch;
    while (isspace (name[0]))
        name ++;
    pch = name;
    while (!isspace (pch[0]) && pch[0] != '\0')
        pch ++;
    pch[0] = '\0';

    int numModule;
    DWORD_PTR *moduleList = NULL;
    ModuleFromName(moduleList, mName, numModule);
    ToDestroy des0 ((void**)&moduleList);
    
    for (int i = 0; i < numModule; i ++)
    {
        if (IsInterrupt())
            break;

        Module vModule;
        DWORD_PTR dwAddr = moduleList[i];
        vModule.Fill (dwAddr);
        ExtOut ("--------------------------------------\n");
        GetInfoFromName(vModule, name);
    }
    return Status;
}




/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function displays the commands available in strike and the   *  
*    arguments passed into each.
*                                                                      *
\**********************************************************************/
extern "C" HRESULT CALLBACK
Help(PDEBUG_CLIENT Client, PCSTR Args)
{
    INIT_API();

    ExtOut( "Strike : Help\n" );
    ExtOut( "IP2MD <addr>         | Find MethodDesc from IP\n" );
    ExtOut( "DumpMD <addr>        | Dump MethodDesc info\n" );
    ExtOut( "DumpMT [-MD] <addr>  | Dump MethodTable info\n" );
    ExtOut( "DumpClass <addr>     | Dump EEClass info\n" );
    ExtOut( "DumpModule <addr>    | Dump EE Module info\n" );
    ExtOut( "DumpObj <addr>       | Dump an object on GC heap\n" );
#ifndef PLATFORM_UNIX
    ExtOut( "u [<MD>] [IP]        | Unassembly a managed code\n" );
    ExtOut( "DumpStack [-EE] [top stack [bottom stack]\n" );
    ExtOut( "EEStack [-short] [-EE] | List all stacks EE knows\n" );
#endif
    ExtOut( "Threads              | List managed threads\n" );
    ExtOut( "ThreadPool           | Display CLR threadpool state\n" );
    ExtOut( "DumpStackObjects [top stack [bottom stack]\n" );
    ExtOut( "SyncBlk [-all|#]     | List syncblock\n" );
    ExtOut( "DumpDomain [<addr>]  | List assemblies and modules in a domain\n" );
    ExtOut( "Token2EE             | Find memory address of EE data for metadata token\n");
    ExtOut( "Name2EE              | Find memory address of EE data given a class/method name\n");
    return Status;
}
